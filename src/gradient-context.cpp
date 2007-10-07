#define __SP_GRADIENT_CONTEXT_C__

/*
 * Gradient drawing and editing tool
 *
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include <gdk/gdkkeysyms.h>

#include "macros.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "message-context.h"
#include "message-stack.h"
#include "pixmaps/cursor-gradient.xpm"
#include "pixmaps/cursor-gradient-add.xpm"
#include "pixmaps/cursor-gradient-delete.xpm"
#include "gradient-context.h"
#include "gradient-chemistry.h"
#include <glibmm/i18n.h>
#include "prefs-utils.h"
#include "gradient-drag.h"
#include "gradient-chemistry.h"
#include "xml/repr.h"
#include "sp-item.h"
#include "display/sp-ctrlline.h"
#include "sp-linear-gradient.h"
#include "sp-radial-gradient.h"
#include "sp-stop.h"
#include "svg/css-ostringstream.h"
#include "svg/svg-color.h"
#include "snap.h"
#include "sp-namedview.h"



static void sp_gradient_context_class_init(SPGradientContextClass *klass);
static void sp_gradient_context_init(SPGradientContext *gr_context);
static void sp_gradient_context_dispose(GObject *object);

static void sp_gradient_context_setup(SPEventContext *ec);

static gint sp_gradient_context_root_handler(SPEventContext *event_context, GdkEvent *event);

static void sp_gradient_drag(SPGradientContext &rc, NR::Point const pt, guint state, guint32 etime);

static SPEventContextClass *parent_class;


GtkType sp_gradient_context_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPGradientContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_gradient_context_class_init,
            NULL, NULL,
            sizeof(SPGradientContext),
            4,
            (GInstanceInitFunc) sp_gradient_context_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPGradientContext", &info, (GTypeFlags) 0);
    }
    return type;
}

static void sp_gradient_context_class_init(SPGradientContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass *) g_type_class_peek_parent(klass);

    object_class->dispose = sp_gradient_context_dispose;

    event_context_class->setup = sp_gradient_context_setup;
    event_context_class->root_handler  = sp_gradient_context_root_handler;
}

static void sp_gradient_context_init(SPGradientContext *gr_context)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(gr_context);

    gr_context->cursor_addnode = false;
    event_context->cursor_shape = cursor_gradient_xpm;
    event_context->hot_x = 4;
    event_context->hot_y = 4;
    event_context->xp = 0;
    event_context->yp = 0;
    event_context->tolerance = 6;
    event_context->within_tolerance = false;
    event_context->item_to_select = NULL;
}

static void sp_gradient_context_dispose(GObject *object)
{
    SPGradientContext *rc = SP_GRADIENT_CONTEXT(object);
    SPEventContext *ec = SP_EVENT_CONTEXT(object);

    ec->enableGrDrag(false);

    if (rc->_message_context) {
        delete rc->_message_context;
    }

    rc->selcon->disconnect();
    delete rc->selcon;
    rc->subselcon->disconnect();
    delete rc->subselcon;

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

const gchar *gr_handle_descr [] = {
    N_("Linear gradient <b>start</b>"), //POINT_LG_BEGIN
    N_("Linear gradient <b>end</b>"),
    N_("Linear gradient <b>mid stop</b>"),
    N_("Radial gradient <b>center</b>"),
    N_("Radial gradient <b>radius</b>"),
    N_("Radial gradient <b>radius</b>"),
    N_("Radial gradient <b>focus</b>"), // POINT_RG_FOCUS
    N_("Radial gradient <b>mid stop</b>"),
    N_("Radial gradient <b>mid stop</b>")
};

static void 
gradient_selection_changed (Inkscape::Selection *, gpointer data)
{
    SPGradientContext *rc = (SPGradientContext *) data;

    GrDrag *drag = rc->_grdrag;
    Inkscape::Selection *selection = sp_desktop_selection(SP_EVENT_CONTEXT(rc)->desktop);
    guint n_obj = g_slist_length((GSList *) selection->itemList());

    if (!drag->isNonEmpty() || selection->isEmpty())
        return;
    guint n_tot = drag->numDraggers();
    guint n_sel = drag->numSelected();

    if (n_sel == 1) {
        if (drag->singleSelectedDraggerNumDraggables() == 1) {
            rc->_message_context->setF(Inkscape::NORMAL_MESSAGE,
                    _("%s selected out of %d gradient handles on %d selected object(s)"), gr_handle_descr[drag->singleSelectedDraggerSingleDraggableType()], n_tot, n_obj);
        } else {
            rc->_message_context->setF(Inkscape::NORMAL_MESSAGE,
                    _("One handle merging %d stops (drag with <b>Shift</b> to separate) selected out of %d gradient handles on %d selected object(s)"), drag->singleSelectedDraggerNumDraggables(), n_tot, n_obj);
        }
    } else if (n_sel > 1) {
        rc->_message_context->setF(Inkscape::NORMAL_MESSAGE,
                                   _("<b>%d</b> gradient handles selected out of %d on %d selected object(s)"), n_sel, n_tot, n_obj);
    } else if (n_sel == 0) {
        rc->_message_context->setF(Inkscape::NORMAL_MESSAGE,
                                   _("<b>No</b> gradient handles selected out of %d on %d selected object(s)"), n_tot, n_obj);
    }
}

static void
gradient_subselection_changed (gpointer, gpointer data)
{
    gradient_selection_changed (NULL, data);
}


static void sp_gradient_context_setup(SPEventContext *ec)
{
    SPGradientContext *rc = SP_GRADIENT_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup) {
        ((SPEventContextClass *) parent_class)->setup(ec);
    }

    if (prefs_get_int_attribute("tools.gradient", "selcue", 1) != 0) {
        ec->enableSelectionCue();
    }

    ec->enableGrDrag();
    Inkscape::Selection *selection = sp_desktop_selection(ec->desktop);

    rc->_message_context = new Inkscape::MessageContext(sp_desktop_message_stack(ec->desktop));

    rc->selcon = new sigc::connection (selection->connectChanged( sigc::bind (sigc::ptr_fun(&gradient_selection_changed), rc)));
    rc->subselcon = new sigc::connection (ec->desktop->connectToolSubselectionChanged(sigc::bind (sigc::ptr_fun(&gradient_subselection_changed), rc)));
    gradient_selection_changed(selection, rc);
}

void
sp_gradient_context_select_next (SPEventContext *event_context)
{
    GrDrag *drag = event_context->_grdrag;
    g_assert (drag);

    drag->select_next();
}

void
sp_gradient_context_select_prev (SPEventContext *event_context)
{
    GrDrag *drag = event_context->_grdrag;
    g_assert (drag);

    drag->select_prev();
}

// FIXME: make global function in libnr or somewhere.
static NR::Point
snap_vector_midpoint (NR::Point p, NR::Point begin, NR::Point end)
{
    double length = NR::L2(end - begin);
    NR::Point be = (end - begin) / length;
    double r = NR::dot(p - begin, be);

    if (r < 0.0) return begin;
    if (r > length) return end;

    return (begin + r * be);
}

static bool
sp_gradient_context_is_over_line (SPGradientContext *rc, SPItem *item, NR::Point event_p)
{
    SPDesktop *desktop = SP_EVENT_CONTEXT (rc)->desktop;

    //Translate mouse point into proper coord system
    rc->mousepoint_doc = desktop->w2d(event_p);

    SPCtrlLine* line = SP_CTRLLINE(item);

    NR::Point nearest = snap_vector_midpoint (rc->mousepoint_doc, line->s, line->e);
    double dist_screen = NR::L2 (rc->mousepoint_doc - nearest) * desktop->current_zoom();

    double tolerance = (double) SP_EVENT_CONTEXT(rc)->tolerance;

    bool close = (dist_screen < tolerance);

    return close;
}

static double
get_offset_between_points (NR::Point p, NR::Point begin, NR::Point end)
{
    double length = NR::L2(end - begin);
    NR::Point be = (end - begin) / length;
    double r = NR::dot(p - begin, be);

    if (r < 0.0) return 0.0;
    if (r > length) return 1.0;

    return (r / length);
}

std::vector<NR::Point>
sp_gradient_context_get_stop_intervals (GrDrag *drag, GSList **these_stops, GSList **next_stops)
{
    std::vector<NR::Point> coords;

    // for all selected draggers
    for (GList *i = drag->selected; i != NULL; i = i->next) {
        GrDragger *dragger = (GrDragger *) i->data;
        // remember the coord of the dragger to reselect it later
        coords.push_back(dragger->point);
        // for all draggables of dragger
        for (GSList const* j = dragger->draggables; j != NULL; j = j->next) { 
            GrDraggable *d = (GrDraggable *) j->data;

            // find the gradient
            SPGradient *gradient = sp_item_gradient (d->item, d->fill_or_stroke);
            SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (gradient, false);

            // these draggable types cannot have a next draggabe to insert a stop between them
            if (d->point_type == POINT_LG_END || 
                d->point_type == POINT_RG_FOCUS || 
                d->point_type == POINT_RG_R1 || 
                d->point_type == POINT_RG_R2) {
                continue;
            }

            // from draggables to stops
            SPStop *this_stop = sp_get_stop_i (vector, d->point_i);
            SPStop *next_stop = sp_next_stop (this_stop);
            SPStop *last_stop = sp_last_stop (vector);

            gint fs = d->fill_or_stroke;
            SPItem *item = d->item;
            gint type = d->point_type;
            gint p_i = d->point_i;

            // if there's a next stop,
            if (next_stop) {
                GrDragger *dnext = NULL;
                // find its dragger 
                // (complex because it may have different types, and because in radial,
                // more than one dragger may correspond to a stop, so we must distinguish)
                if (type == POINT_LG_BEGIN || type == POINT_LG_MID) {
                    if (next_stop == last_stop)
                        dnext = drag->getDraggerFor (item, POINT_LG_END, p_i+1, fs);
                    else
                        dnext = drag->getDraggerFor (item, POINT_LG_MID, p_i+1, fs);
                } else { // radial
                    if (type == POINT_RG_CENTER || type == POINT_RG_MID1) {
                        if (next_stop == last_stop)
                            dnext = drag->getDraggerFor (item, POINT_RG_R1, p_i+1, fs);
                        else 
                            dnext = drag->getDraggerFor (item, POINT_RG_MID1, p_i+1, fs);
                    } 
                    if ((type == POINT_RG_MID2) || 
                        (type == POINT_RG_CENTER && dnext && !dnext->isSelected())) {
                        if (next_stop == last_stop)
                            dnext = drag->getDraggerFor (item, POINT_RG_R2, p_i+1, fs);
                        else 
                            dnext = drag->getDraggerFor (item, POINT_RG_MID2, p_i+1, fs);
                    }
                }

                // remember the coords of the future dragger to select it
                coords.push_back(0.5*(dragger->point + dnext->point));

                // if both adjacent draggers selected,
                if (!g_slist_find(*these_stops, this_stop) && dnext && dnext->isSelected()) {
                    // do not insert a stop now, it will confuse the loop;
                    // just remember the stops
                    *these_stops = g_slist_prepend (*these_stops, this_stop);
                    *next_stops = g_slist_prepend (*next_stops, next_stop);
                }
            }
        }
    }
    return coords;
}

static void
sp_gradient_context_add_stops_between_selected_stops (SPGradientContext *rc)
{
    SPDocument *doc = NULL;
    GrDrag *drag = rc->_grdrag;

    GSList *these_stops = NULL;
    GSList *next_stops = NULL;

    std::vector<NR::Point> coords = sp_gradient_context_get_stop_intervals (drag, &these_stops, &next_stops);

    // now actually create the new stops
    GSList *i = these_stops;
    GSList *j = next_stops;
    for (; i != NULL && j != NULL; i = i->next, j = j->next) {
        SPStop *this_stop = (SPStop *) i->data;
        SPStop *next_stop = (SPStop *) j->data;
        gfloat offset = 0.5*(this_stop->offset + next_stop->offset);
        SPObject *parent = SP_OBJECT_PARENT(this_stop);
        if (SP_IS_GRADIENT (parent)) {
            doc = SP_OBJECT_DOCUMENT (parent);
            sp_vector_add_stop (SP_GRADIENT (parent), this_stop, next_stop, offset);
            sp_gradient_ensure_vector (SP_GRADIENT (parent));
        }
    }

    if (g_slist_length(these_stops) > 0 && doc) {
        sp_document_done (doc, SP_VERB_CONTEXT_GRADIENT, _("Add gradient stop"));
        drag->updateDraggers();
        // so that it does not automatically update draggers in idle loop, as this would deselect
        drag->local_change = true;
        // select all the old selected and new created draggers
        drag->selectByCoords(coords);
    }

    g_slist_free (these_stops);
    g_slist_free (next_stops);
}

double sqr(double x) {return x*x;}

static void
sp_gradient_simplify(SPGradientContext *rc, double tolerance)
{
    SPDocument *doc = NULL;
    GrDrag *drag = rc->_grdrag;

    GSList *these_stops = NULL;
    GSList *next_stops = NULL;

    std::vector<NR::Point> coords = sp_gradient_context_get_stop_intervals (drag, &these_stops, &next_stops);

    GSList *todel = NULL;

    GSList *i = these_stops;
    GSList *j = next_stops;
    for (; i != NULL && j != NULL; i = i->next, j = j->next) {
        SPStop *stop0 = (SPStop *) i->data;
        SPStop *stop1 = (SPStop *) j->data;

        gint i1 = g_slist_index(these_stops, stop1);
        if (i1 != -1) {
            GSList *next_next = g_slist_nth (next_stops, i1);
            if (next_next) {
                SPStop *stop2 = (SPStop *) next_next->data;

                if (g_slist_find(todel, stop0) || g_slist_find(todel, stop2))
                    continue;

                guint32 const c0 = sp_stop_get_rgba32(stop0);
                guint32 const c2 = sp_stop_get_rgba32(stop2);
                guint32 const c1r = sp_stop_get_rgba32(stop1);
                guint32 c1 = average_color (c0, c2, 
                       (stop1->offset - stop0->offset) / (stop2->offset - stop0->offset));

                double diff = 
                    sqr(SP_RGBA32_R_F(c1) - SP_RGBA32_R_F(c1r)) +
                    sqr(SP_RGBA32_G_F(c1) - SP_RGBA32_G_F(c1r)) +
                    sqr(SP_RGBA32_B_F(c1) - SP_RGBA32_B_F(c1r)) +
                    sqr(SP_RGBA32_A_F(c1) - SP_RGBA32_A_F(c1r));

                if (diff < tolerance)
                    todel = g_slist_prepend (todel, stop1);
            }
        }
    }

    for (i = todel; i != NULL; i = i->next) {
        SPStop *stop = (SPStop*) i->data;
        doc = SP_OBJECT_DOCUMENT (stop);
        Inkscape::XML::Node * parent = SP_OBJECT_REPR(stop)->parent();
        parent->removeChild(SP_OBJECT_REPR(stop));
    }

    if (g_slist_length(todel) > 0) {
        sp_document_done (doc, SP_VERB_CONTEXT_GRADIENT, _("Simplify gradient"));
        drag->local_change = true;
        drag->updateDraggers();
        drag->selectByCoords(coords);
    }

    g_slist_free (todel);
    g_slist_free (these_stops);
    g_slist_free (next_stops);
}



static void
sp_gradient_context_add_stop_near_point (SPGradientContext *rc, SPItem *item,  NR::Point mouse_p, guint32 etime)
{
    // item is the selected item. mouse_p the location in doc coordinates of where to add the stop

    SPEventContext *ec = SP_EVENT_CONTEXT(rc);
    SPDesktop *desktop = SP_EVENT_CONTEXT (rc)->desktop;

    double tolerance = (double) ec->tolerance;

    gfloat offset; // type of SPStop.offset = gfloat
    SPGradient *gradient;
    bool fill_or_stroke = true;
    bool r1_knot = false;

    bool addknot = false;
    do {
        gradient = sp_item_gradient (item, fill_or_stroke);
        if (SP_IS_LINEARGRADIENT(gradient)) {
            NR::Point begin   = sp_item_gradient_get_coords(item, POINT_LG_BEGIN, 0, fill_or_stroke);
            NR::Point end     = sp_item_gradient_get_coords(item, POINT_LG_END, 0, fill_or_stroke);

            NR::Point nearest = snap_vector_midpoint (mouse_p, begin, end);
            double dist_screen = NR::L2 (mouse_p - nearest) * desktop->current_zoom();
            if ( dist_screen < tolerance ) {
                // add the knot
                offset = get_offset_between_points(nearest, begin, end);
                addknot = true;
                break; // break out of the while loop: add only one knot
            }
        } else if (SP_IS_RADIALGRADIENT(gradient)) {
            NR::Point begin = sp_item_gradient_get_coords(item, POINT_RG_CENTER, 0, fill_or_stroke);
            NR::Point end   = sp_item_gradient_get_coords(item, POINT_RG_R1, 0, fill_or_stroke);
            NR::Point nearest = snap_vector_midpoint (mouse_p, begin, end);
            double dist_screen = NR::L2 (mouse_p - nearest) * desktop->current_zoom();
            if ( dist_screen < tolerance ) {
                offset = get_offset_between_points(nearest, begin, end);
                addknot = true;
                r1_knot = true;
                break; // break out of the while loop: add only one knot
            }

            end    = sp_item_gradient_get_coords(item, POINT_RG_R2, 0, fill_or_stroke);
            nearest = snap_vector_midpoint (mouse_p, begin, end);
            dist_screen = NR::L2 (mouse_p - nearest) * desktop->current_zoom();
            if ( dist_screen < tolerance ) {
                offset = get_offset_between_points(nearest, begin, end);
                addknot = true;
                r1_knot = false;
                break; // break out of the while loop: add only one knot
            }
        }
        fill_or_stroke = !fill_or_stroke;
    } while (!fill_or_stroke && !addknot) ;

    if (addknot) {
        SPGradient *vector = sp_gradient_get_forked_vector_if_necessary (gradient, false);
        SPStop* prev_stop = sp_first_stop(vector);
        SPStop* next_stop = sp_next_stop(prev_stop);
        while ( (next_stop) && (next_stop->offset < offset) ) {
            prev_stop = next_stop;
            next_stop = sp_next_stop(next_stop);
        }
        if (!next_stop) {
            // logical error: the endstop should have offset 1 and should always be more than this offset here
            return;
        }


        SPStop *newstop = sp_vector_add_stop (vector, prev_stop, next_stop, offset);

        sp_document_done (SP_OBJECT_DOCUMENT (vector), SP_VERB_CONTEXT_GRADIENT,
                  _("Add gradient stop"));

        ec->_grdrag->updateDraggers();
        sp_gradient_ensure_vector (gradient);

        if (vector->has_stops) {
            guint i = sp_number_of_stops_before_stop(vector, newstop);

            gradient = sp_item_gradient (item, fill_or_stroke);
            GrPointType pointtype = POINT_G_INVALID;
            if (SP_IS_LINEARGRADIENT(gradient)) {
                pointtype = POINT_LG_MID;
            } else if (SP_IS_RADIALGRADIENT(gradient)) {
                pointtype = r1_knot ? POINT_RG_MID1 : POINT_RG_MID2;
            }
            GrDragger *dragger = SP_EVENT_CONTEXT(rc)->_grdrag->getDraggerFor (item, pointtype, i, fill_or_stroke);
            if (dragger && (etime == 0) ) {
                ec->_grdrag->setSelected (dragger);
            } else {
                ec->_grdrag->grabKnot (item,
                                   pointtype,
                                   i,
                                   fill_or_stroke, 99999, 99999, etime);
            }
            ec->_grdrag->local_change = true;

        }
    }
}


static gint
sp_gradient_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    static bool dragging;

    SPDesktop *desktop = event_context->desktop;
    Inkscape::Selection *selection = sp_desktop_selection (desktop);

    SPGradientContext *rc = SP_GRADIENT_CONTEXT(event_context);

    event_context->tolerance = prefs_get_int_attribute_limited("options.dragtolerance", "value", 0, 0, 100);
    double const nudge = prefs_get_double_attribute_limited("options.nudgedistance", "value", 2, 0, 1000); // in px

    GrDrag *drag = event_context->_grdrag;
    g_assert (drag);

    gint ret = FALSE;
    switch (event->type) {
    case GDK_2BUTTON_PRESS:
        if ( event->button.button == 1 ) {
            bool over_line = false;
            SPCtrlLine *line = NULL;
            if (drag->lines) {
                for (GSList *l = drag->lines; (l != NULL) && (!over_line); l = l->next) {
                    line = (SPCtrlLine*) l->data;
                    over_line |= sp_gradient_context_is_over_line (rc, (SPItem*) line, NR::Point(event->motion.x, event->motion.y));
                }
            }
            if (over_line) {
                sp_gradient_context_add_stop_near_point(rc, SP_ITEM(selection->itemList()->data), rc->mousepoint_doc, event->button.time);
            } else {
                for (GSList const* i = selection->itemList(); i != NULL; i = i->next) {
                    SPItem *item = SP_ITEM(i->data);
                    SPGradientType new_type = (SPGradientType) prefs_get_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_LINEAR);
                    guint new_fill = prefs_get_int_attribute ("tools.gradient", "newfillorstroke", 1);

                    SPGradient *vector = sp_gradient_vector_for_object(sp_desktop_document(desktop), desktop,                                                                                   SP_OBJECT (item), new_fill);

                    SPGradient *priv = sp_item_set_gradient(item, vector, new_type, new_fill);
                    sp_gradient_reset_to_userspace(priv, item);
                }

                sp_document_done (sp_desktop_document (desktop), SP_VERB_CONTEXT_GRADIENT,
                                  _("Create default gradient"));
            }
            ret = TRUE;
        }
        break;
    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1 && !event_context->space_panning ) {
            NR::Point const button_w(event->button.x, event->button.y);

            // save drag origin
            event_context->xp = (gint) button_w[NR::X];
            event_context->yp = (gint) button_w[NR::Y];
            event_context->within_tolerance = true;

            // remember clicked item, disregarding groups, honoring Alt; do nothing with Crtl to
            // enable Ctrl+doubleclick of exactly the selected item(s)
            if (!(event->button.state & GDK_CONTROL_MASK))
                event_context->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, TRUE);

            dragging = true;
            /* Position center */
            NR::Point const button_dt = desktop->w2d(button_w);
            /* Snap center to nearest magnetic point */
            
            SnapManager const &m = desktop->namedview->snap_manager;
            rc->origin = m.freeSnap(Inkscape::Snapper::SNAPPOINT_NODE, button_dt, NULL).getPoint();

            ret = TRUE;
        }
        break;
    case GDK_MOTION_NOTIFY:
        if ( dragging
             && ( event->motion.state & GDK_BUTTON1_MASK ) && !event_context->space_panning )
        {
            if ( event_context->within_tolerance
                 && ( abs( (gint) event->motion.x - event_context->xp ) < event_context->tolerance )
                 && ( abs( (gint) event->motion.y - event_context->yp ) < event_context->tolerance ) ) {
                break; // do not drag if we're within tolerance from origin
            }
            // Once the user has moved farther than tolerance from the original location
            // (indicating they intend to draw, not click), then always process the
            // motion notify coordinates as given (no snapping back to origin)
            event_context->within_tolerance = false;

            NR::Point const motion_w(event->motion.x,
                                     event->motion.y);
            NR::Point const motion_dt = event_context->desktop->w2d(motion_w);

            sp_gradient_drag(*rc, motion_dt, event->motion.state, event->motion.time);

            ret = TRUE;
        } else {
            bool over_line = false;
            if (drag->lines) {
                for (GSList *l = drag->lines; l != NULL; l = l->next) {
                    over_line |= sp_gradient_context_is_over_line (rc, (SPItem*) l->data, NR::Point(event->motion.x, event->motion.y));
                }
            }

            if (rc->cursor_addnode && !over_line) {
                event_context->cursor_shape = cursor_gradient_xpm;
                sp_event_context_update_cursor(event_context);
                rc->cursor_addnode = false;
            } else if (!rc->cursor_addnode && over_line) {
                event_context->cursor_shape = cursor_gradient_add_xpm;
                sp_event_context_update_cursor(event_context);
                rc->cursor_addnode = true;
            }
        }
        break;
    case GDK_BUTTON_RELEASE:
        event_context->xp = event_context->yp = 0;
        if ( event->button.button == 1 && !event_context->space_panning ) {
            if ( (event->button.state & GDK_CONTROL_MASK) && (event->button.state & GDK_MOD1_MASK ) ) {
                bool over_line = false;
                SPCtrlLine *line = NULL;
                if (drag->lines) {
                    for (GSList *l = drag->lines; (l != NULL) && (!over_line); l = l->next) {
                        line = (SPCtrlLine*) l->data;
                        over_line |= sp_gradient_context_is_over_line (rc, (SPItem*) line, NR::Point(event->motion.x, event->motion.y));
                    }
                }
                if (over_line) {
                    sp_gradient_context_add_stop_near_point(rc, SP_ITEM(selection->itemList()->data), rc->mousepoint_doc, 0);
                    ret = TRUE;
                }
            } else {
                dragging = false;

                // unless clicked with Ctrl (to enable Ctrl+doubleclick).  (don't what this is for (johan))
                if (event->button.state & GDK_CONTROL_MASK) {
                    ret = TRUE;
                    break;
                }

                if (!event_context->within_tolerance) {
                    // we've been dragging, do nothing (grdrag handles that)
                } else if (event_context->item_to_select) {
                    // no dragging, select clicked item if any
                    if (event->button.state & GDK_SHIFT_MASK) {
                        selection->toggle(event_context->item_to_select);
                    } else {
                        selection->set(event_context->item_to_select);
                    }
                } else {
                    // click in an empty space; do the same as Esc
                    if (drag->selected) {
                        drag->deselectAll();
                    } else {
                        selection->clear();
                    }
                }

                event_context->item_to_select = NULL;
                ret = TRUE;
            }
        }
        break;
    case GDK_KEY_PRESS:
        switch (get_group0_keyval (&event->key)) {
        case GDK_Alt_L:
        case GDK_Alt_R:
        case GDK_Control_L:
        case GDK_Control_R:
        case GDK_Shift_L:
        case GDK_Shift_R:
        case GDK_Meta_L:  // Meta is when you press Shift+Alt (at least on my machine)
        case GDK_Meta_R:
            sp_event_show_modifier_tip (event_context->defaultMessageContext(), event,
                                        _("<b>Ctrl</b>: snap gradient angle"),
                                        _("<b>Shift</b>: draw gradient around the starting point"),
                                        NULL);
            break;

        case GDK_x:
        case GDK_X:
            if (MOD__ALT_ONLY) {
                desktop->setToolboxFocusTo ("altx-grad");
                ret = TRUE;
            }
            break;

        case GDK_A:
        case GDK_a:
            if (MOD__CTRL_ONLY && drag->isNonEmpty()) {
                drag->selectAll();
                ret = TRUE;
            }
            break;

        case GDK_L:
        case GDK_l:
            if (MOD__CTRL_ONLY && drag->isNonEmpty() && drag->hasSelection()) {
                sp_gradient_simplify(rc, 1e-4);
                ret = TRUE;
            }
            break;

        case GDK_Escape:
            if (drag->selected) {
                drag->deselectAll();
            } else {
                selection->clear();
            }
            ret = TRUE;
            //TODO: make dragging escapable by Esc
            break;

        case GDK_Left: // move handle left
        case GDK_KP_Left:
        case GDK_KP_4:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) drag->selected_move_screen(-10, 0); // shift
                    else drag->selected_move_screen(-1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) drag->selected_move(-10*nudge, 0); // shift
                    else drag->selected_move(-nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_Up: // move handle up
        case GDK_KP_Up:
        case GDK_KP_8:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) drag->selected_move_screen(0, 10); // shift
                    else drag->selected_move_screen(0, 1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) drag->selected_move(0, 10*nudge); // shift
                    else drag->selected_move(0, nudge); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_Right: // move handle right
        case GDK_KP_Right:
        case GDK_KP_6:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) drag->selected_move_screen(10, 0); // shift
                    else drag->selected_move_screen(1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) drag->selected_move(10*nudge, 0); // shift
                    else drag->selected_move(nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_Down: // move handle down
        case GDK_KP_Down:
        case GDK_KP_2:
            if (!MOD__CTRL) { // not ctrl
                if (MOD__ALT) { // alt
                    if (MOD__SHIFT) drag->selected_move_screen(0, -10); // shift
                    else drag->selected_move_screen(0, -1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT) drag->selected_move(0, -10*nudge); // shift
                    else drag->selected_move(0, -nudge); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_r:
        case GDK_R:
            if (MOD__SHIFT_ONLY) {
                // First try selected dragger
                if (drag && drag->selected) {
                    drag->selected_reverse_vector();
                } else { // If no drag or no dragger selected, act on selection (both fill and stroke gradients)
                    for (GSList const* i = selection->itemList(); i != NULL; i = i->next) {
                        sp_item_gradient_reverse_vector (SP_ITEM(i->data), true);
                        sp_item_gradient_reverse_vector (SP_ITEM(i->data), false);
                    }
                }
                // we did an undoable action
                sp_document_done (sp_desktop_document (desktop), SP_VERB_CONTEXT_GRADIENT,
                                  _("Invert gradient"));
                ret = TRUE;
            }
            break;

        case GDK_Insert:
        case GDK_KP_Insert:
            // with any modifiers:
            sp_gradient_context_add_stops_between_selected_stops (rc);
            ret = TRUE;
            break;

        case GDK_Delete:
        case GDK_KP_Delete:
        case GDK_BackSpace:
            if ( drag->selected ) {
                drag->deleteSelected(MOD__CTRL_ONLY);
                ret = TRUE;
            }
            break;
        default:
            break;
        }
        break;
    case GDK_KEY_RELEASE:
        switch (get_group0_keyval (&event->key)) {
        case GDK_Alt_L:
        case GDK_Alt_R:
        case GDK_Control_L:
        case GDK_Control_R:
        case GDK_Shift_L:
        case GDK_Shift_R:
        case GDK_Meta_L:  // Meta is when you press Shift+Alt
        case GDK_Meta_R:
            event_context->defaultMessageContext()->clear();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (!ret) {
        if (((SPEventContextClass *) parent_class)->root_handler) {
            ret = ((SPEventContextClass *) parent_class)->root_handler(event_context, event);
        }
    }

    return ret;
}

static void sp_gradient_drag(SPGradientContext &rc, NR::Point const pt, guint state, guint32 etime)
{
    SPDesktop *desktop = SP_EVENT_CONTEXT(&rc)->desktop;
    Inkscape::Selection *selection = sp_desktop_selection(desktop);
    SPDocument *document = sp_desktop_document(desktop);
    SPEventContext *ec = SP_EVENT_CONTEXT(&rc);

    if (!selection->isEmpty()) {
        int type = prefs_get_int_attribute ("tools.gradient", "newgradient", 1);
        int fill_or_stroke = prefs_get_int_attribute ("tools.gradient", "newfillorstroke", 1);

        SPGradient *vector;
        if (ec->item_to_select) {
            vector = sp_gradient_vector_for_object(document, desktop, ec->item_to_select, fill_or_stroke);
        } else {
            vector = sp_gradient_vector_for_object(document, desktop, SP_ITEM(selection->itemList()->data), fill_or_stroke);
        }

        // HACK: reset fill-opacity - that 0.75 is annoying; BUT remove this when we have an opacity slider for all tabs
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "fill-opacity", "1.0");

        for (GSList const *i = selection->itemList(); i != NULL; i = i->next) {

            //FIXME: see above
            sp_repr_css_change_recursive(SP_OBJECT_REPR(i->data), css, "style");

            sp_item_set_gradient(SP_ITEM(i->data), vector, (SPGradientType) type, fill_or_stroke);

            if (type == SP_GRADIENT_TYPE_LINEAR) {
                sp_item_gradient_set_coords (SP_ITEM(i->data), POINT_LG_BEGIN, 0, rc.origin, fill_or_stroke, true, false);
                sp_item_gradient_set_coords (SP_ITEM(i->data), POINT_LG_END, 0, pt, fill_or_stroke, true, false);
            } else if (type == SP_GRADIENT_TYPE_RADIAL) {
                sp_item_gradient_set_coords (SP_ITEM(i->data), POINT_RG_CENTER, 0, rc.origin, fill_or_stroke, true, false);
                sp_item_gradient_set_coords (SP_ITEM(i->data), POINT_RG_R1, 0, pt, fill_or_stroke, true, false);
            }
            SP_OBJECT (i->data)->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        if (ec->_grdrag) {
            ec->_grdrag->updateDraggers();
            // prevent regenerating draggers by selection modified signal, which sometimes
            // comes too late and thus destroys the knot which we will now grab:
            ec->_grdrag->local_change = true;
            // give the grab out-of-bounds values of xp/yp because we're already dragging
            // and therefore are already out of tolerance
            ec->_grdrag->grabKnot (SP_ITEM(selection->itemList()->data),
                                   type == SP_GRADIENT_TYPE_LINEAR? POINT_LG_END : POINT_RG_R1,
                                   -1, // ignore number (though it is always 1)
                                   fill_or_stroke, 99999, 99999, etime);
        }
        // We did an undoable action, but sp_document_done will be called by the knot when released

        // status text; we do not track coords because this branch is run once, not all the time
        // during drag
        int n_objects = g_slist_length((GSList *) selection->itemList());
        rc._message_context->setF(Inkscape::NORMAL_MESSAGE,
                                  ngettext("<b>Gradient</b> for %d object; with <b>Ctrl</b> to snap angle",
                                           "<b>Gradient</b> for %d objects; with <b>Ctrl</b> to snap angle", n_objects),
                                  n_objects);
    } else {
        sp_desktop_message_stack(desktop)->flash(Inkscape::WARNING_MESSAGE, _("Select <b>objects</b> on which to create gradient."));
    }
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
