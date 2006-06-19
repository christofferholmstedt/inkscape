#define __SP_SELECT_CONTEXT_C__

/*
 * Selection and transformation context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
  *
 * Copyright (C) 1999-2005 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <gdk/gdkkeysyms.h>
#include "macros.h"
#include "rubberband.h"
#include "document.h"
#include "selection.h"
#include "seltrans-handles.h"
#include "sp-cursor.h"
//#include "pixmaps/cursor-select-m.xpm" // These aren't used
//#include "pixmaps/cursor-select-d.xpm"
#include "pixmaps/handles.xpm"
#include <glibmm/i18n.h>

#include "select-context.h"
#include "selection-chemistry.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "sp-root.h"
#include "prefs-utils.h"
#include "tools-switch.h"
#include "message-stack.h"
#include "selection-describer.h"
#include "seltrans.h"

static void sp_select_context_class_init(SPSelectContextClass *klass);
static void sp_select_context_init(SPSelectContext *select_context);
static void sp_select_context_dispose(GObject *object);

static void sp_select_context_setup(SPEventContext *ec);
static void sp_select_context_set(SPEventContext *ec, gchar const *key, gchar const *val);
static gint sp_select_context_root_handler(SPEventContext *event_context, GdkEvent *event);
static gint sp_select_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);

static SPEventContextClass *parent_class;

static GdkCursor *CursorSelectMouseover = NULL;
static GdkCursor *CursorSelectDragging = NULL;
GdkPixbuf *handles[13];

static gint rb_escaped = 0; // if non-zero, rubberband was canceled by esc, so the next button release should not deselect
static gint drag_escaped = 0; // if non-zero, drag was canceled by esc

static gint xp = 0, yp = 0; // where drag started
static gint tolerance = 0;
static bool within_tolerance = false;

GtkType
sp_select_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPSelectContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_select_context_class_init,
            NULL, NULL,
            sizeof(SPSelectContext),
            4,
            (GInstanceInitFunc) sp_select_context_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPSelectContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_select_context_class_init(SPSelectContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_select_context_dispose;

    event_context_class->setup = sp_select_context_setup;
    event_context_class->set = sp_select_context_set;
    event_context_class->root_handler = sp_select_context_root_handler;
    event_context_class->item_handler = sp_select_context_item_handler;

    // cursors in select context
//    CursorSelectMouseover = sp_cursor_new_from_xpm(cursor_select_m_xpm , 1, 1);
//    CursorSelectDragging = sp_cursor_new_from_xpm(cursor_select_d_xpm , 1, 1);
    // selection handles
    handles[0]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_scale_nw_xpm);
    handles[1]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_scale_ne_xpm);
    handles[2]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_scale_h_xpm);
    handles[3]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_scale_v_xpm);
    handles[4]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_nw_xpm);
    handles[5]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_n_xpm);
    handles[6]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_ne_xpm);
    handles[7]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_e_xpm);
    handles[8]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_se_xpm);
    handles[9]  = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_s_xpm);
    handles[10] = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_sw_xpm);
    handles[11] = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_rotate_w_xpm);
    handles[12] = gdk_pixbuf_new_from_xpm_data((gchar const **)handle_center_xpm);

}

static void
sp_select_context_init(SPSelectContext *sc)
{
    sc->dragging = FALSE;
    sc->moved = FALSE;
    sc->button_press_shift = false;
    sc->button_press_ctrl = false;
    sc->button_press_alt = false;
    sc->_seltrans = NULL;
    sc->_describer = NULL;
}

static void
sp_select_context_dispose(GObject *object)
{
    SPSelectContext *sc = SP_SELECT_CONTEXT(object);
    SPEventContext * ec = SP_EVENT_CONTEXT (object);

    ec->enableGrDrag(false);

    if (sc->grabbed) {
        sp_canvas_item_ungrab(sc->grabbed, GDK_CURRENT_TIME);
        sc->grabbed = NULL;
    }

    delete sc->_seltrans;
    sc->_seltrans = NULL;
    delete sc->_describer;
    sc->_describer = NULL;

    if (CursorSelectDragging) {
        gdk_cursor_unref (CursorSelectDragging);
        CursorSelectDragging = NULL;
    }
    if (CursorSelectMouseover) {
        gdk_cursor_unref (CursorSelectMouseover);
        CursorSelectMouseover = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
sp_select_context_setup(SPEventContext *ec)
{
    SPSelectContext *select_context = SP_SELECT_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup) {
        ((SPEventContextClass *) parent_class)->setup(ec);
    }

    SPDesktop *desktop = ec->desktop;

    select_context->_describer = new Inkscape::SelectionDescriber(desktop->selection, desktop->messageStack());

    select_context->_seltrans = new Inkscape::SelTrans(desktop);

    sp_event_context_read(ec, "show");
    sp_event_context_read(ec, "transform");

    if (prefs_get_int_attribute("tools.select", "gradientdrag", 0) != 0) {
        ec->enableGrDrag();
    }
}

static void
sp_select_context_set(SPEventContext *ec, gchar const *key, gchar const *val)
{
    SPSelectContext *sc = SP_SELECT_CONTEXT(ec);

    if (!strcmp(key, "show")) {
        if (val && !strcmp(val, "outline")) {
            sc->_seltrans->setShow(Inkscape::SelTrans::SHOW_OUTLINE);
        } else {
            sc->_seltrans->setShow(Inkscape::SelTrans::SHOW_CONTENT);
        }
    }
}

static bool
sp_select_context_abort(SPEventContext *event_context)
{
    SPDesktop *desktop = event_context->desktop;
    SPSelectContext *sc = SP_SELECT_CONTEXT(event_context);
    Inkscape::SelTrans *seltrans = sc->_seltrans;

    if (sc->dragging) {
        if (sc->moved) { // cancel dragging an object
            seltrans->ungrab();
            sc->moved = FALSE;
            sc->dragging = FALSE;
            drag_escaped = 1;

            if (sc->item) {
                // only undo if the item is still valid
                if (SP_OBJECT_DOCUMENT( SP_OBJECT(sc->item))) {
                    sp_document_undo(sp_desktop_document(desktop));
                }

                sp_object_unref( SP_OBJECT(sc->item), NULL);
            } else if (sc->button_press_ctrl) {
                // NOTE:  This is a workaround to a bug.
                // When the ctrl key is held, sc->item is not defined
                // so in this case (only), we skip the object doc check
                sp_document_undo(sp_desktop_document(desktop));
            }
            sc->item = NULL;

            SP_EVENT_CONTEXT(sc)->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Move canceled."));
            return true;
        }
    } else {
        NR::Maybe<NR::Rect> const b = Inkscape::Rubberband::get()->getRectangle();
        if (b != NR::Nothing()) {
            Inkscape::Rubberband::get()->stop();
            rb_escaped = 1;
            SP_EVENT_CONTEXT(sc)->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Selection canceled."));
            return true;
        }
    }
    return false;
}

bool
key_is_a_modifier (guint key) {
    return (key == GDK_Alt_L ||
                key == GDK_Alt_R ||
                key == GDK_Control_L ||
                key == GDK_Control_R ||
                key == GDK_Shift_L ||
                key == GDK_Shift_R ||
                key == GDK_Meta_L ||  // Meta is when you press Shift+Alt (at least on my machine)
            key == GDK_Meta_R);
}

void
sp_select_context_up_one_layer(SPDesktop *desktop)
{
    /* Click in empty place, go up one level -- but don't leave a layer to root.
     *
     * (Rationale: we don't usually allow users to go to the root, since that
     * detracts from the layer metaphor: objects at the root level can in front
     * of or behind layers.  Whereas it's fine to go to the root if editing
     * a document that has no layers (e.g. a non-Inkscape document).)
     *
     * Once we support editing SVG "islands" (e.g. <svg> embedded in an xhtml
     * document), we might consider further restricting the below to disallow
     * leaving a layer to go to a non-layer.
     */
    SPObject *const current_layer = desktop->currentLayer();
    if (current_layer) {
        SPObject *const parent = SP_OBJECT_PARENT(current_layer);
        if ( parent
             && ( SP_OBJECT_PARENT(parent)
                  || !( SP_IS_GROUP(current_layer)
                        && ( SPGroup::LAYER
                             == SP_GROUP(current_layer)->layerMode() ) ) ) )
        {
            desktop->setCurrentLayer(parent);
            if (SP_IS_GROUP(current_layer) && SPGroup::LAYER != SP_GROUP(current_layer)->layerMode())
                sp_desktop_selection(desktop)->set(current_layer);
        }
    }
}

static gint
sp_select_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
    gint ret = FALSE;

    SPDesktop *desktop = event_context->desktop;
    SPSelectContext *sc = SP_SELECT_CONTEXT(event_context);
    Inkscape::SelTrans *seltrans = sc->_seltrans;

    tolerance = prefs_get_int_attribute_limited("options.dragtolerance", "value", 0, 0, 100);

    // make sure we still have valid objects to move around
    if (sc->item && SP_OBJECT_DOCUMENT( SP_OBJECT(sc->item))==NULL) {
        sp_select_context_abort(event_context);
    }

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {
                /* Left mousebutton */

                // save drag origin
                xp = (gint) event->button.x;
                yp = (gint) event->button.y;
                within_tolerance = true;

                // remember what modifiers were on before button press
                sc->button_press_shift = (event->button.state & GDK_SHIFT_MASK) ? true : false;
                sc->button_press_ctrl = (event->button.state & GDK_CONTROL_MASK) ? true : false;
                sc->button_press_alt = (event->button.state & GDK_MOD1_MASK) ? true : false;

                if (event->button.state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) {
                    // if shift or ctrl was pressed, do not move objects;
                    // pass the event to root handler which will perform rubberband, shift-click, ctrl-click, ctrl-drag
                } else {
                    sc->dragging = TRUE;
                    sc->moved = FALSE;

                    // remember the clicked item in sc->item:
                    sc->item = sp_event_context_find_item (desktop,
                                              NR::Point(event->button.x, event->button.y), event->button.state & GDK_MOD1_MASK, FALSE);
                    sp_object_ref(sc->item, NULL);

                    rb_escaped = drag_escaped = 0;

                    sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->drawing),
                                        GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK |
                                        GDK_POINTER_MOTION_MASK,
                                        NULL, event->button.time);
                    sc->grabbed = SP_CANVAS_ITEM(desktop->drawing);

                    ret = TRUE;
                }
            } else if (event->button.button == 3) {
                // right click; do not eat it so that right-click menu can appear, but cancel dragging & rubberband
                sp_select_context_abort(event_context);
            }
            break;

        case GDK_ENTER_NOTIFY:
        {
            GdkCursor *cursor = gdk_cursor_new(GDK_FLEUR);
            gdk_window_set_cursor(GTK_WIDGET(sp_desktop_canvas(desktop))->window, cursor);
            gdk_cursor_destroy(cursor);
            break;
        }

        case GDK_LEAVE_NOTIFY:
            gdk_window_set_cursor(GTK_WIDGET(sp_desktop_canvas(desktop))->window, event_context->cursor);
            break;

        case GDK_KEY_PRESS:
            if (get_group0_keyval (&event->key) == GDK_space) {
                if (sc->dragging && sc->grabbed) {
                    /* stamping mode: show content mode moving */
                    seltrans->stamp();
                    ret = TRUE;
                }
            }
            break;

        default:
            break;
    }

    if (!ret) {
        if (((SPEventContextClass *) parent_class)->item_handler)
            ret = ((SPEventContextClass *) parent_class)->item_handler(event_context, item, event);
    }

    return ret;
}

static gint
sp_select_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    SPItem *item = NULL;
    SPItem *item_at_point = NULL, *group_at_point = NULL, *item_in_group = NULL;
    gint ret = FALSE;

    SPDesktop *desktop = event_context->desktop;
    SPSelectContext *sc = SP_SELECT_CONTEXT(event_context);
    Inkscape::SelTrans *seltrans = sc->_seltrans;
    Inkscape::Selection *selection = sp_desktop_selection(desktop);
    gdouble const nudge = prefs_get_double_attribute_limited("options.nudgedistance", "value", 2, 0, 1000); // in px
    gdouble const offset = prefs_get_double_attribute_limited("options.defaultscale", "value", 2, 0, 1000);
    tolerance = prefs_get_int_attribute_limited("options.dragtolerance", "value", 0, 0, 100);
    int const snaps = prefs_get_int_attribute("options.rotationsnapsperpi", "value", 12);

    // make sure we still have valid objects to move around
    if (sc->item && SP_OBJECT_DOCUMENT( SP_OBJECT(sc->item))==NULL) {
        sp_select_context_abort(event_context);
    }

    switch (event->type) {
        case GDK_2BUTTON_PRESS:
            if (event->button.button == 1) {
                if (!selection->isEmpty()) {
                    SPItem *clicked_item = (SPItem *) selection->itemList()->data;
                    if (SP_IS_GROUP (clicked_item)) { // enter group
                        desktop->setCurrentLayer(reinterpret_cast<SPObject *>(clicked_item));
                        sp_desktop_selection(desktop)->clear();
                        sc->dragging = false;
                    } else { // switch tool
                        tools_switch_by_item (desktop, clicked_item);
                    }
                } else {
                    sp_select_context_up_one_layer(desktop);
                }
                ret = TRUE;
            }
            break;
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {

                // save drag origin
                xp = (gint) event->button.x;
                yp = (gint) event->button.y;
                within_tolerance = true;

                NR::Point const button_pt(event->button.x, event->button.y);
                NR::Point const p(desktop->w2d(button_pt));
                Inkscape::Rubberband::get()->start(desktop, p);
                sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                    GDK_KEY_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
                                    NULL, event->button.time);
                sc->grabbed = SP_CANVAS_ITEM(desktop->acetate);

                // remember what modifiers were on before button press
                sc->button_press_shift = (event->button.state & GDK_SHIFT_MASK) ? true : false;
                sc->button_press_ctrl = (event->button.state & GDK_CONTROL_MASK) ? true : false;
                sc->button_press_alt = (event->button.state & GDK_MOD1_MASK) ? true : false;

                sc->moved = FALSE;

                rb_escaped = drag_escaped = 0;

                ret = TRUE;
            } else if (event->button.button == 3) {
                // right click; do not eat it so that right-click menu can appear, but cancel dragging & rubberband
                sp_select_context_abort(event_context);
            }
            break;

        case GDK_MOTION_NOTIFY:
            if (event->motion.state & GDK_BUTTON1_MASK) {
                NR::Point const motion_pt(event->motion.x, event->motion.y);
                NR::Point const p(desktop->w2d(motion_pt));

                if ( within_tolerance
                     && ( abs( (gint) event->motion.x - xp ) < tolerance )
                     && ( abs( (gint) event->motion.y - yp ) < tolerance ) ) {
                    break; // do not drag if we're within tolerance from origin
                }
                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to move the object, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                within_tolerance = false;

                if (sc->button_press_ctrl || sc->button_press_alt) // if ctrl or alt was pressed and it's not click, we want to drag rather than rubberband
                    sc->dragging = TRUE;

                if (sc->dragging) {
                    /* User has dragged fast, so we get events on root (lauris)*/
                    // not only that; we will end up here when ctrl-dragging as well
                    // and also when we started within tolerance, but trespassed tolerance outside of item
                    Inkscape::Rubberband::get()->stop();
                    item_at_point = desktop->item_at_point(NR::Point(event->button.x, event->button.y), FALSE);
                    if (!item_at_point) // if no item at this point, try at the click point (bug 1012200)
                        item_at_point = desktop->item_at_point(NR::Point(xp, yp), FALSE);
                    if (item_at_point || sc->moved || sc->button_press_alt) {
                        // drag only if starting from an item, or if something is already grabbed, or if alt-dragging
                        if (!sc->moved) {
                            item_in_group = desktop->item_at_point(NR::Point(event->button.x, event->button.y), TRUE);
                            group_at_point = desktop->group_at_point(NR::Point(event->button.x, event->button.y));
                            // if neither a group nor an item (possibly in a group) at point are selected, set selection to the item at point
                            if ((!item_in_group || !selection->includes(item_in_group)) &&
                                (!group_at_point || !selection->includes(group_at_point))
                                && !sc->button_press_alt) {
                                // select what is under cursor
                                if (!seltrans->isEmpty()) {
                                    seltrans->resetState();
                                }
                                // when simply ctrl-dragging, we don't want to go into groups
                                if (item_at_point && !selection->includes(item_at_point))
                                    selection->set(item_at_point);
                            } // otherwise, do not change selection so that dragging selected-within-group items, as well as alt-dragging, is possible
                            seltrans->grab(p, -1, -1, FALSE);
                            sc->moved = TRUE;
                        }
                        if (!seltrans->isEmpty())
                            seltrans->moveTo(p, event->button.state);
                        if (desktop->scroll_to_point(&p))
                            // unfortunately in complex drawings, gobbling results in losing grab of the object, for some mysterious reason
                            ; //gobble_motion_events(GDK_BUTTON1_MASK);
                        ret = TRUE;
                    } else {
                        sc->dragging = FALSE;
                    }
                } else {
                    Inkscape::Rubberband::get()->move(p);
                    gobble_motion_events(GDK_BUTTON1_MASK);
                }
            }
            break;
        case GDK_BUTTON_RELEASE:
            xp = yp = 0;
            if ((event->button.button == 1) && (sc->grabbed)) {
                if (sc->dragging) {
                    if (sc->moved) {
                        // item has been moved
                        seltrans->ungrab();
                        sc->moved = FALSE;
                    } else if (sc->item && !drag_escaped) {
                        // item has not been moved -> simply a click, do selecting
                        if (!selection->isEmpty()) {
                            if (event->button.state & GDK_SHIFT_MASK) {
                                // with shift, toggle selection
                                seltrans->resetState();
                                selection->toggle(sc->item);
                            } else {
                                // without shift, increase state (i.e. toggle scale/rotation handles)
                                if (selection->includes(sc->item)) {
                                    seltrans->increaseState();
                                } else {
                                    seltrans->resetState();
                                    selection->set(sc->item);
                                }
                            }
                        } else { // simple or shift click, no previous selection
                            seltrans->resetState();
                            selection->set(sc->item);
                        }
                    }
                    sc->dragging = FALSE;
                    if (sc->item) {
                        sp_object_unref( SP_OBJECT(sc->item), NULL);
                    }
                    sc->item = NULL;
                } else {
                    NR::Maybe<NR::Rect> const b = Inkscape::Rubberband::get()->getRectangle();
                    if (b != NR::Nothing() && !within_tolerance) {
                        // this was a rubberband drag
                        Inkscape::Rubberband::get()->stop();
                        seltrans->resetState();
                        // find out affected items:
                        GSList *items = sp_document_items_in_box(sp_desktop_document(desktop), desktop->dkey, b.assume());
                        if (event->button.state & GDK_SHIFT_MASK) {
                            // with shift, add to selection
                            selection->addList (items);
                        } else {
                            // without shift, simply select anew
                            selection->setList (items);
                        }
                        g_slist_free (items);
                    } else { // it was just a click, or a too small rubberband
                        Inkscape::Rubberband::get()->stop();
                        if (sc->button_press_shift && !rb_escaped && !drag_escaped) {
                            // this was a shift-click, select what was clicked upon

                            sc->button_press_shift = false;

                            if (sc->button_press_ctrl) {
                                // go into groups, honoring Alt
                                item = sp_event_context_find_item (desktop,
                                                   NR::Point(event->button.x, event->button.y), event->button.state & GDK_MOD1_MASK, TRUE);
                                sc->button_press_ctrl = FALSE;
                            } else {
                                // don't go into groups, honoring Alt
                                item = sp_event_context_find_item (desktop,
                                                   NR::Point(event->button.x, event->button.y), event->button.state & GDK_MOD1_MASK, FALSE);
                            }

                            if (item) {
                                selection->toggle(item);
                                item = NULL;
                            }

                        } else if (sc->button_press_ctrl && !rb_escaped && !drag_escaped) { // ctrl-click

                            sc->button_press_ctrl = FALSE;

                            item = sp_event_context_find_item (desktop,
                                         NR::Point(event->button.x, event->button.y), event->button.state & GDK_MOD1_MASK, TRUE);

                            if (item) {
                                if (selection->includes(item)) {
                                    seltrans->increaseState();
                                } else {
                                    seltrans->resetState();
                                    selection->set(item);
                                }
                                item = NULL;
                            }

                        } else { // click without shift, simply deselect, unless with Alt or something was cancelled
                            if (!selection->isEmpty()) {
                                if (!(rb_escaped) && !(drag_escaped) && !(event->button.state & GDK_MOD1_MASK))
                                    selection->clear();
                                rb_escaped = 0;
                                ret = TRUE;
                            }
                        }
                    }
                    ret = TRUE;
                }
                if (sc->grabbed) {
                    sp_canvas_item_ungrab(sc->grabbed, event->button.time);
                    sc->grabbed = NULL;
                }
            }
            sc->button_press_shift = false;
            sc->button_press_ctrl = false;
            sc->button_press_alt = false;
            break;

        case GDK_KEY_PRESS: // keybindings for select context

            if (!key_is_a_modifier (get_group0_keyval (&event->key))) {
                    event_context->defaultMessageContext()->clear();
            } else {
                    sp_event_show_modifier_tip (event_context->defaultMessageContext(), event,
                                                _("<b>Ctrl</b>: select in groups, move hor/vert"),
                                                _("<b>Shift</b>: toggle select, force rubberband, disable snapping"),
                                                _("<b>Alt</b>: select under, move selected"));
                    break;
            }

            switch (get_group0_keyval (&event->key)) {
                case GDK_Left: // move selection left
                case GDK_KP_Left:
                case GDK_KP_4:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) sp_selection_move_screen(-10, 0); // shift
                            else sp_selection_move_screen(-1, 0); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) sp_selection_move(-10*nudge, 0); // shift
                            else sp_selection_move(-nudge, 0); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Up: // move selection up
                case GDK_KP_Up:
                case GDK_KP_8:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) sp_selection_move_screen(0, 10); // shift
                            else sp_selection_move_screen(0, 1); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) sp_selection_move(0, 10*nudge); // shift
                            else sp_selection_move(0, nudge); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Right: // move selection right
                case GDK_KP_Right:
                case GDK_KP_6:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) sp_selection_move_screen(10, 0); // shift
                            else sp_selection_move_screen(1, 0); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) sp_selection_move(10*nudge, 0); // shift
                            else sp_selection_move(nudge, 0); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Down: // move selection down
                case GDK_KP_Down:
                case GDK_KP_2:
                    if (!MOD__CTRL) { // not ctrl
                        if (MOD__ALT) { // alt
                            if (MOD__SHIFT) sp_selection_move_screen(0, -10); // shift
                            else sp_selection_move_screen(0, -1); // no shift
                        }
                        else { // no alt
                            if (MOD__SHIFT) sp_selection_move(0, -10*nudge); // shift
                            else sp_selection_move(0, -nudge); // no shift
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_Escape:
                    if (!sp_select_context_abort(event_context))
                        selection->clear();
                    ret = TRUE;
                    break;
                case GDK_a:
                case GDK_A:
                    if (MOD__CTRL_ONLY) {
                        sp_edit_select_all();
                        ret = TRUE;
                    }
                    break;
                case GDK_Tab: // Tab - cycle selection forward
                    if (!(MOD__CTRL_ONLY || (MOD__CTRL && MOD__SHIFT))) {
                        sp_selection_item_next();
                        ret = TRUE;
                    }
                    break;
                case GDK_ISO_Left_Tab: // Shift Tab - cycle selection backward
                    if (!(MOD__CTRL_ONLY || (MOD__CTRL && MOD__SHIFT))) {
                        sp_selection_item_prev();
                        ret = TRUE;
                    }
                    break;
                case GDK_space:
                    /* stamping mode: show outline mode moving */
                    /* FIXME: Is next condition ok? (lauris) */
                    if (sc->dragging && sc->grabbed) {
                        seltrans->stamp();
                        ret = TRUE;
                    }
                    break;
                case GDK_x:
                case GDK_X:
                    if (MOD__ALT_ONLY) {
                        desktop->setToolboxFocusTo ("altx");
                        ret = TRUE;
                    }
                    break;
                case GDK_bracketleft:
                    if (MOD__ALT) {
                        sp_selection_rotate_screen(selection, 1);
                    } else if (MOD__CTRL) {
                        sp_selection_rotate(selection, 90);
                    } else if (snaps) {
                        sp_selection_rotate(selection, 180/snaps);
                    }
                    ret = TRUE;
                    break;
                case GDK_bracketright:
                    if (MOD__ALT) {
                        sp_selection_rotate_screen(selection, -1);
                    } else if (MOD__CTRL) {
                        sp_selection_rotate(selection, -90);
                    } else if (snaps) {
                        sp_selection_rotate(selection, -180/snaps);
                    }
                    ret = TRUE;
                    break;
                case GDK_less:
                case GDK_comma:
                    if (MOD__ALT) {
                        sp_selection_scale_screen(selection, -2);
                    } else if (MOD__CTRL) {
                        sp_selection_scale_times(selection, 0.5);
                    } else {
                        sp_selection_scale(selection, -offset);
                    }
                    ret = TRUE;
                    break;
                case GDK_greater:
                case GDK_period:
                    if (MOD__ALT) {
                        sp_selection_scale_screen(selection, 2);
                    } else if (MOD__CTRL) {
                        sp_selection_scale_times(selection, 2);
                    } else {
                        sp_selection_scale(selection, offset);
                    }
                    ret = TRUE;
                    break;
                case GDK_Return:
                    if (MOD__CTRL_ONLY) {
                        if (selection->singleItem()) {
                            SPItem *clicked_item = selection->singleItem();
                            if (SP_IS_GROUP (clicked_item)) { // enter group
                                desktop->setCurrentLayer(reinterpret_cast<SPObject *>(clicked_item));
                                sp_desktop_selection(desktop)->clear();
                            } else {
                                SP_EVENT_CONTEXT(sc)->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Selected object is not a group. Cannot enter."));
                            }
                        }
                        ret = TRUE;
                    }
                    break;
                case GDK_BackSpace:
                    if (MOD__CTRL_ONLY) {
                        sp_select_context_up_one_layer(desktop);
                        ret = TRUE;
                    }
                    break;
                default:
                    break;
            }
            break;
        case GDK_KEY_RELEASE:
            if (key_is_a_modifier (get_group0_keyval (&event->key)))
                event_context->defaultMessageContext()->clear();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
