#define __SP_NODE_CONTEXT_C__

/*
 * Node editing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <gdk/gdkkeysyms.h>
#include "macros.h"
#include <glibmm/i18n.h>
#include "display/sp-canvas-util.h"
#include "object-edit.h"
#include "sp-path.h"
#include "path-chemistry.h"
#include "rubberband.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "pixmaps/cursor-node.xpm"
#include "message-context.h"
#include "node-context.h"
#include "pixmaps/cursor-node-d.xpm"
#include "prefs-utils.h"
#include "xml/node-event-vector.h"
#include "style.h"
#include "splivarot.h"
#include "shape-editor.h"

static void sp_node_context_class_init(SPNodeContextClass *klass);
static void sp_node_context_init(SPNodeContext *node_context);
static void sp_node_context_dispose(GObject *object);

static void sp_node_context_setup(SPEventContext *ec);
static gint sp_node_context_root_handler(SPEventContext *event_context, GdkEvent *event);
static gint sp_node_context_item_handler(SPEventContext *event_context,
                                         SPItem *item, GdkEvent *event);

static SPEventContextClass *parent_class;

GType
sp_node_context_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPNodeContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_node_context_class_init,
            NULL, NULL,
            sizeof(SPNodeContext),
            4,
            (GInstanceInitFunc) sp_node_context_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPNodeContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_node_context_class_init(SPNodeContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_node_context_dispose;

    event_context_class->setup = sp_node_context_setup;
    event_context_class->root_handler = sp_node_context_root_handler;
    event_context_class->item_handler = sp_node_context_item_handler;
}

static void
sp_node_context_init(SPNodeContext *node_context)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(node_context);

    event_context->cursor_shape = cursor_node_xpm;
    event_context->hot_x = 1;
    event_context->hot_y = 1;

    node_context->leftalt = FALSE;
    node_context->rightalt = FALSE;
    node_context->leftctrl = FALSE;
    node_context->rightctrl = FALSE;
    
    new (&node_context->sel_changed_connection) sigc::connection();
}

static void
sp_node_context_dispose(GObject *object)
{
    SPNodeContext *nc = SP_NODE_CONTEXT(object);
    SPEventContext *ec = SP_EVENT_CONTEXT(object);

    ec->enableGrDrag(false);

    nc->sel_changed_connection.disconnect();
    nc->sel_changed_connection.~connection();

    delete nc->shape_editor;

    if (nc->_node_message_context) {
        delete nc->_node_message_context;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
sp_node_context_setup(SPEventContext *ec)
{
    SPNodeContext *nc = SP_NODE_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup)
        ((SPEventContextClass *) parent_class)->setup(ec);

    nc->sel_changed_connection.disconnect();
    nc->sel_changed_connection = sp_desktop_selection(ec->desktop)->connectChanged(sigc::bind(sigc::ptr_fun(&sp_node_context_selection_changed), (gpointer)nc));

    Inkscape::Selection *selection = sp_desktop_selection(ec->desktop);
    SPItem *item = selection->singleItem();

    nc->shape_editor = new ShapeEditor(ec->desktop);

    nc->rb_escaped = false;

    nc->cursor_drag = false;

    nc->added_node = false;

    nc->current_state = SP_NODE_CONTEXT_INACTIVE;

    if (item) {
        nc->shape_editor->set_item(item);
    }

    if (prefs_get_int_attribute("tools.nodes", "selcue", 0) != 0) {
        ec->enableSelectionCue();
    }

    if (prefs_get_int_attribute("tools.nodes", "gradientdrag", 0) != 0) {
        ec->enableGrDrag();
    }

    nc->_node_message_context = new Inkscape::MessageContext((ec->desktop)->messageStack());

    nc->shape_editor->update_statusbar();
}

/**
\brief  Callback that processes the "changed" signal on the selection;
destroys old and creates new nodepath and reassigns listeners to the new selected item's repr
*/
void
sp_node_context_selection_changed(Inkscape::Selection *selection, gpointer data)
{
    SPNodeContext *nc = SP_NODE_CONTEXT(data);

    // TODO: update ShapeEditorsCollective instead
    nc->shape_editor->unset_item();
    SPItem *item = selection->singleItem(); 
    nc->shape_editor->set_item(item);

    nc->shape_editor->update_statusbar();
}

void
sp_node_context_show_modifier_tip(SPEventContext *event_context, GdkEvent *event)
{
    sp_event_show_modifier_tip
        (event_context->defaultMessageContext(), event,
         _("<b>Ctrl</b>: toggle node type, snap handle angle, move hor/vert; <b>Ctrl+Alt</b>: move along handles"),
         _("<b>Shift</b>: toggle node selection, disable snapping, rotate both handles"),
         _("<b>Alt</b>: lock handle length; <b>Ctrl+Alt</b>: move along handles"));
}


static gint
sp_node_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
    gint ret = FALSE;

    if (((SPEventContextClass *) parent_class)->item_handler)
        ret = ((SPEventContextClass *) parent_class)->item_handler(event_context, item, event);

    return ret;
}

static gint
sp_node_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    SPDesktop *desktop = event_context->desktop;
    Inkscape::Selection *selection = sp_desktop_selection (desktop);

    SPNodeContext *nc = SP_NODE_CONTEXT(event_context);
    double const nudge = prefs_get_double_attribute_limited("options.nudgedistance", "value", 2, 0, 1000); // in px
    event_context->tolerance = prefs_get_int_attribute_limited("options.dragtolerance", "value", 0, 0, 100); // read every time, to make prefs changes really live
    int const snaps = prefs_get_int_attribute("options.rotationsnapsperpi", "value", 12);
    double const offset = prefs_get_double_attribute_limited("options.defaultscale", "value", 2, 0, 1000);

    gint ret = FALSE;
    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1 && !event_context->space_panning) {
                // save drag origin
                event_context->xp = (gint) event->button.x;
                event_context->yp = (gint) event->button.y;
                event_context->within_tolerance = true;
                nc->shape_editor->cancel_hit();

                if (!(event->button.state & GDK_SHIFT_MASK)) {
                    if (!nc->drag) {
                        if (nc->shape_editor->has_nodepath() && selection->single() /* && item_over */) {
                            // save drag origin
                            bool over_stroke = nc->shape_editor->is_over_stroke(NR::Point(event->button.x, event->button.y), true);
                            //only dragging curves
                            if (over_stroke) {
                                ret = TRUE;
                                break;
                            }
                        }
                    }
                }
                NR::Point const button_w(event->button.x,
                                         event->button.y);
                NR::Point const button_dt(desktop->w2d(button_w));
                Inkscape::Rubberband::get()->start(desktop, button_dt);
                nc->current_state = SP_NODE_CONTEXT_INACTIVE;
                desktop->updateNow();
                ret = TRUE;
            }
            break;
        case GDK_MOTION_NOTIFY:
            if (event->motion.state & GDK_BUTTON1_MASK && !event_context->space_panning) {

                if ( event_context->within_tolerance
                     && ( abs( (gint) event->motion.x - event_context->xp ) < event_context->tolerance )
                     && ( abs( (gint) event->motion.y - event_context->yp ) < event_context->tolerance ) ) {
                    break; // do not drag if we're within tolerance from origin
                }

                // The path went away while dragging; throw away any further motion
                // events until the mouse pointer is released.
                
                if (nc->shape_editor->hits_curve() && !nc->shape_editor->has_nodepath()) {
                  break;
                }

                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to move the object, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                event_context->within_tolerance = false;

                // Once we determine what the user is doing (dragging either a node or the
                // selection rubberband), make sure we continue to perform that operation
                // until the mouse pointer is lifted.
                if (nc->current_state == SP_NODE_CONTEXT_INACTIVE) {
                    if (nc->shape_editor->hits_curve() && nc->shape_editor->has_nodepath()) {
                        nc->current_state = SP_NODE_CONTEXT_NODE_DRAGGING;
                    } else {
                        nc->current_state = SP_NODE_CONTEXT_RUBBERBAND_DRAGGING;
                    }
                }

                switch (nc->current_state) {
                    case SP_NODE_CONTEXT_NODE_DRAGGING:
                        {
                            nc->shape_editor->curve_drag (event->motion.x, event->motion.y);

                            gobble_motion_events(GDK_BUTTON1_MASK);
                            break;
                        }
                    case SP_NODE_CONTEXT_RUBBERBAND_DRAGGING:
                        if (Inkscape::Rubberband::get()->is_started()) {
                            NR::Point const motion_w(event->motion.x,
                                                event->motion.y);
                            NR::Point const motion_dt(desktop->w2d(motion_w));
                            Inkscape::Rubberband::get()->move(motion_dt);
                        }
                        break;
                }

                nc->drag = TRUE;
                ret = TRUE;
            } else {
                if (!nc->shape_editor->has_nodepath() || selection->singleItem() == NULL) {
                    break;
                }

                bool over_stroke = false;
                over_stroke = nc->shape_editor->is_over_stroke(NR::Point(event->motion.x, event->motion.y), false);

                if (nc->cursor_drag && !over_stroke) {
                    event_context->cursor_shape = cursor_node_xpm;
                    event_context->hot_x = 1;
                    event_context->hot_y = 1;
                    sp_event_context_update_cursor(event_context);
                    nc->cursor_drag = false;
                } else if (!nc->cursor_drag && over_stroke) {
                    event_context->cursor_shape = cursor_node_d_xpm;
                    event_context->hot_x = 1;
                    event_context->hot_y = 1;
                    sp_event_context_update_cursor(event_context);
                    nc->cursor_drag = true;
                }
            }
            break;

        case GDK_2BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
            if ( (event->button.button == 1) && (!nc->drag) && !event_context->space_panning) {
                // find out clicked item, disregarding groups, honoring Alt
                SPItem *item_clicked = sp_event_context_find_item (desktop,
                        NR::Point(event->button.x, event->button.y),
                        (event->button.state & GDK_MOD1_MASK) && !(event->button.state & GDK_CONTROL_MASK), TRUE);

                event_context->xp = event_context->yp = 0;

                bool over_stroke = false;
                if (nc->shape_editor->has_nodepath()) {
                    over_stroke = nc->shape_editor->is_over_stroke(NR::Point(event->button.x, event->button.y), false);
                }

                if (item_clicked || over_stroke) {
                    if (over_stroke || nc->added_node) {
                        switch (event->type) {
                            case GDK_BUTTON_RELEASE:
                                if (event->button.state & GDK_CONTROL_MASK && event->button.state & GDK_MOD1_MASK) {
                                    //add a node
                                    nc->shape_editor->add_node_near_point();
                                } else {
                                    if (nc->added_node) { // we just received double click, ignore release
                                        nc->added_node = false;
                                        break;
                                    }
                                    //select the segment
                                    if (event->button.state & GDK_SHIFT_MASK) {
                                        nc->shape_editor->select_segment_near_point(true);
                                    } else {
                                        nc->shape_editor->select_segment_near_point(false);
                                    }
                                    desktop->updateNow();
                                }
                                break;
                            case GDK_2BUTTON_PRESS:
                                //add a node
                                nc->shape_editor->add_node_near_point();
                                nc->added_node = true;
                                break;
                            default:
                                break;
                        }
                    } else if (event->button.state & GDK_SHIFT_MASK) {
                        selection->toggle(item_clicked);
                        desktop->updateNow();
                    } else {
                        selection->set(item_clicked);
                        desktop->updateNow();
                    }
                    ret = TRUE;
                    break;
                }
            } 
            if (event->type == GDK_BUTTON_RELEASE) {
                event_context->xp = event_context->yp = 0;
                if (event->button.button == 1) {
                    NR::Maybe<NR::Rect> b = Inkscape::Rubberband::get()->getRectangle();

                    if (nc->shape_editor->hits_curve() && !event_context->within_tolerance) { //drag curve
                        nc->shape_editor->finish_drag();
                    } else if (b && !event_context->within_tolerance) { // drag to select
                        nc->shape_editor->select_rect(*b, event->button.state & GDK_SHIFT_MASK);
                    } else {
                        if (!(nc->rb_escaped)) { // unless something was cancelled
                            if (nc->shape_editor->has_selection())
                                nc->shape_editor->deselect();
                            else
                                sp_desktop_selection(desktop)->clear();
                        }
                    }
                    ret = TRUE;
                    Inkscape::Rubberband::get()->stop();
                    desktop->updateNow();
                    nc->rb_escaped = false;
                    nc->drag = FALSE;
                    nc->shape_editor->cancel_hit();
                    nc->current_state = SP_NODE_CONTEXT_INACTIVE;
                }
            }
            break;
        case GDK_KEY_PRESS:
            switch (get_group0_keyval(&event->key)) {
                case GDK_Insert:
                case GDK_KP_Insert:
                    // with any modifiers
                    nc->shape_editor->add_node();
                    ret = TRUE;
                    break;
                case GDK_Delete:
                case GDK_KP_Delete:
                case GDK_BackSpace:
                    if (MOD__CTRL_ONLY) {
                        nc->shape_editor->delete_nodes();
                    } else {
                        nc->shape_editor->delete_nodes_preserving_shape();
                    }
                    ret = TRUE;
                    break;
                case GDK_C:
                case GDK_c:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->set_node_type(Inkscape::NodePath::NODE_CUSP);
                        ret = TRUE;
                    }
                    break;
                case GDK_S:
                case GDK_s:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->set_node_type(Inkscape::NodePath::NODE_SMOOTH);
                        ret = TRUE;
                    }
                    break;
                case GDK_Y:
                case GDK_y:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->set_node_type(Inkscape::NodePath::NODE_SYMM);
                        ret = TRUE;
                    }
                    break;
                case GDK_B:
                case GDK_b:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->break_at_nodes();
                        ret = TRUE;
                    }
                    break;
                case GDK_J:
                case GDK_j:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->join_nodes();
                        ret = TRUE;
                    }
                    break;
                case GDK_D:
                case GDK_d:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->duplicate_nodes();
                        ret = TRUE;
                    }
                    break;
                case GDK_L:
                case GDK_l:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->set_type_of_segments(NR_LINETO);
                        ret = TRUE;
                    }
                    break;
                case GDK_U:
                case GDK_u:
                    if (MOD__SHIFT_ONLY) {
                        nc->shape_editor->set_type_of_segments(NR_CURVETO);
                        ret = TRUE;
                    }
                    break;
                case GDK_R:
                case GDK_r:
                    if (MOD__SHIFT_ONLY) {
                        // FIXME: add top panel button
                        sp_selected_path_reverse();
                        ret = TRUE;
                    }
                    break;
                case GDK_Left: // move selection left
                case GDK_KP_Left:
                case GDK_KP_4:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes_screen(-10, 0); // shift
                            else nc->shape_editor->move_nodes_screen(-1, 0); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes(-10*nudge, 0); // shift
                            else nc->shape_editor->move_nodes(-nudge, 0); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Up: // move selection up
                case GDK_KP_Up:
                case GDK_KP_8:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes_screen(0, 10); // shift
                            else nc->shape_editor->move_nodes_screen(0, 1); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes(0, 10*nudge); // shift
                            else nc->shape_editor->move_nodes(0, nudge); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Right: // move selection right
                case GDK_KP_Right:
                case GDK_KP_6:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes_screen(10, 0); // shift
                            else nc->shape_editor->move_nodes_screen(1, 0); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes(10*nudge, 0); // shift
                            else nc->shape_editor->move_nodes(nudge, 0); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Down: // move selection down
                case GDK_KP_Down:
                case GDK_KP_2:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes_screen(0, -10); // shift
                            else nc->shape_editor->move_nodes_screen(0, -1); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) nc->shape_editor->move_nodes(0, -10*nudge); // shift
                            else nc->shape_editor->move_nodes(0, -nudge); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Escape:
                {
                    NR::Maybe<NR::Rect> const b = Inkscape::Rubberband::get()->getRectangle();
                    if (b) {
                        Inkscape::Rubberband::get()->stop();
                        nc->current_state = SP_NODE_CONTEXT_INACTIVE;
                        nc->rb_escaped = true;
                    } else {
                        if (nc->shape_editor->has_selection()) {
                            nc->shape_editor->deselect();
                        } else {
                            sp_desktop_selection(desktop)->clear();
                        }
                    }
                    ret = TRUE;
                    break;
                }

                case GDK_bracketleft:
                    if ( MOD__CTRL && !MOD__ALT && ( snaps != 0 ) ) {
                        if (nc->leftctrl)
                            nc->shape_editor->rotate_nodes (M_PI/snaps, -1, false);
                        if (nc->rightctrl)
                            nc->shape_editor->rotate_nodes (M_PI/snaps, 1, false);
                    } else if ( MOD__ALT && !MOD__CTRL ) {
                        if (nc->leftalt && nc->rightalt)
                            nc->shape_editor->rotate_nodes (1, 0, true);
                        else {
                            if (nc->leftalt)
                                nc->shape_editor->rotate_nodes (1, -1, true);
                            if (nc->rightalt)
                                nc->shape_editor->rotate_nodes (1, 1, true);
                        }
                    } else if ( snaps != 0 ) {
                        nc->shape_editor->rotate_nodes (M_PI/snaps, 0, false);
                    }
                    ret = TRUE;
                    break;
                case GDK_bracketright:
                    if ( MOD__CTRL && !MOD__ALT && ( snaps != 0 ) ) {
                        if (nc->leftctrl)
                            nc->shape_editor->rotate_nodes (-M_PI/snaps, -1, false);
                        if (nc->rightctrl)
                            nc->shape_editor->rotate_nodes (-M_PI/snaps, 1, false);
                    } else if ( MOD__ALT && !MOD__CTRL ) {
                        if (nc->leftalt && nc->rightalt)
                            nc->shape_editor->rotate_nodes (-1, 0, true);
                        else {
                            if (nc->leftalt)
                                nc->shape_editor->rotate_nodes (-1, -1, true);
                            if (nc->rightalt)
                                nc->shape_editor->rotate_nodes (-1, 1, true);
                        }
                    } else if ( snaps != 0 ) {
                        nc->shape_editor->rotate_nodes (-M_PI/snaps, 0, false);
                    }
                    ret = TRUE;
                    break;
                case GDK_less:
                case GDK_comma:
                    if (MOD__CTRL) {
                        if (nc->leftctrl)
                            nc->shape_editor->scale_nodes(-offset, -1);
                        if (nc->rightctrl)
                            nc->shape_editor->scale_nodes(-offset, 1);
                    } else if (MOD__ALT) {
                        if (nc->leftalt && nc->rightalt)
                            nc->shape_editor->scale_nodes_screen (-1, 0);
                        else {
                            if (nc->leftalt)
                                nc->shape_editor->scale_nodes_screen (-1, -1);
                            if (nc->rightalt)
                                nc->shape_editor->scale_nodes_screen (-1, 1);
                        }
                    } else {
                        nc->shape_editor->scale_nodes (-offset, 0);
                    }
                    ret = TRUE;
                    break;
                case GDK_greater:
                case GDK_period:
                    if (MOD__CTRL) {
                        if (nc->leftctrl)
                            nc->shape_editor->scale_nodes (offset, -1);
                        if (nc->rightctrl)
                            nc->shape_editor->scale_nodes (offset, 1);
                    } else if (MOD__ALT) {
                        if (nc->leftalt && nc->rightalt)
                            nc->shape_editor->scale_nodes_screen (1, 0);
                        else {
                            if (nc->leftalt)
                                nc->shape_editor->scale_nodes_screen (1, -1);
                            if (nc->rightalt)
                                nc->shape_editor->scale_nodes_screen (1, 1);
                        }
                    } else {
                        nc->shape_editor->scale_nodes (offset, 0);
                    }
                    ret = TRUE;
                    break;

                case GDK_Alt_L:
                    nc->leftalt = TRUE;
                    sp_node_context_show_modifier_tip(event_context, event);
                    break;
                case GDK_Alt_R:
                    nc->rightalt = TRUE;
                    sp_node_context_show_modifier_tip(event_context, event);
                    break;
                case GDK_Control_L:
                    nc->leftctrl = TRUE;
                    sp_node_context_show_modifier_tip(event_context, event);
                    break;
                case GDK_Control_R:
                    nc->rightctrl = TRUE;
                    sp_node_context_show_modifier_tip(event_context, event);
                    break;
                case GDK_Shift_L:
                case GDK_Shift_R:
                case GDK_Meta_L:
                case GDK_Meta_R:
                    sp_node_context_show_modifier_tip(event_context, event);
                    break;
                default:
                    ret = node_key(event);
                    break;
            }
            break;
        case GDK_KEY_RELEASE:
            switch (get_group0_keyval(&event->key)) {
                case GDK_Alt_L:
                    nc->leftalt = FALSE;
                    event_context->defaultMessageContext()->clear();
                    break;
                case GDK_Alt_R:
                    nc->rightalt = FALSE;
                    event_context->defaultMessageContext()->clear();
                    break;
                case GDK_Control_L:
                    nc->leftctrl = FALSE;
                    event_context->defaultMessageContext()->clear();
                    break;
                case GDK_Control_R:
                    nc->rightctrl = FALSE;
                    event_context->defaultMessageContext()->clear();
                    break;
                case GDK_Shift_L:
                case GDK_Shift_R:
                case GDK_Meta_L:
                case GDK_Meta_R:
                    event_context->defaultMessageContext()->clear();
                    break;
            }
            break;
        default:
            break;
    }

    if (!ret) {
        if (((SPEventContextClass *) parent_class)->root_handler)
            ret = ((SPEventContextClass *) parent_class)->root_handler(event_context, event);
    }

    return ret;
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
