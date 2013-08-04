/** \file
 * Pen event context implementation.
 */

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdkkeysyms.h>
#include <cstring>
#include <string>

#include "pen-context.h"
#include "sp-namedview.h"
#include "sp-metrics.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "draw-anchor.h"
#include "message-stack.h"
#include "message-context.h"
#include "preferences.h"
#include "sp-path.h"
#include "display/sp-canvas.h"
#include "display/curve.h"
#include "pixmaps/cursor-pen.xpm"
#include "display/canvas-bpath.h"
#include "display/sp-ctrlline.h"
#include "display/sodipodi-ctrl.h"
#include <glibmm/i18n.h>
#include "helper/units.h"
#include "macros.h"
#include "context-fns.h"
#include "tools-switch.h"
#include "ui/control-manager.h"

using Inkscape::ControlManager;

static void spdc_pen_set_initial_point(SPPenContext *pc, Geom::Point const p);
static void spdc_pen_set_subsequent_point(SPPenContext *const pc, Geom::Point const p, bool statusbar, guint status = 0);
static void spdc_pen_set_ctrl(SPPenContext *pc, Geom::Point const p, guint state);
static void spdc_pen_finish_segment(SPPenContext *pc, Geom::Point p, guint state);

static void spdc_pen_finish(SPPenContext *pc, gboolean closed);

static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent);
static gint pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent);
static gint pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent);
static gint pen_handle_2button_press(SPPenContext *const pc, GdkEventButton const &bevent);
static gint pen_handle_key_press(SPPenContext *const pc, GdkEvent *event);
static void spdc_reset_colors(SPPenContext *pc);

static void pen_disable_events(SPPenContext *const pc);
static void pen_enable_events(SPPenContext *const pc);

static Geom::Point pen_drag_origin_w(0, 0);
static bool pen_within_tolerance = false;

static int pen_next_paraxial_direction(const SPPenContext *const pc, Geom::Point const &pt, Geom::Point const &origin, guint state);
static void pen_set_to_nearest_horiz_vert(const SPPenContext *const pc, Geom::Point &pt, guint const state, bool snap);

static int pen_last_paraxial_dir = 0; // last used direction in horizontal/vertical mode; 0 = horizontal, 1 = vertical


#include "tool-factory.h"

namespace {
	SPEventContext* createPenContext() {
		return new SPPenContext();
	}

	bool penContextRegistered = ToolFactory::instance().registerObject("/tools/freehand/pen", createPenContext);
}

const std::string& SPPenContext::getPrefsPath() {
	return SPPenContext::prefsPath;
}

const std::string SPPenContext::prefsPath = "/tools/freehand/pen";

SPPenContext::SPPenContext() : SPDrawContext() {
	this->polylines_only = false;
	this->polylines_paraxial = false;
	this->expecting_clicks_for_LPE = 0;

    this->cursor_shape = cursor_pen_xpm;
    this->hot_x = 4;
    this->hot_y = 4;

    this->npoints = 0;
    this->mode = MODE_CLICK;
    this->state = POINT;

    this->c0 = NULL;
    this->c1 = NULL;
    this->cl0 = NULL;
    this->cl1 = NULL;

    this->events_disabled = 0;

    this->num_clicks = 0;
    this->waiting_LPE = NULL;
    this->waiting_item = NULL;
}

SPPenContext::~SPPenContext() {
    if (this->c0) {
        sp_canvas_item_destroy(this->c0);
        this->c0 = NULL;
    }
    if (this->c1) {
        sp_canvas_item_destroy(this->c1);
        this->c1 = NULL;
    }
    if (this->cl0) {
        sp_canvas_item_destroy(this->cl0);
        this->cl0 = NULL;
    }
    if (this->cl1) {
        sp_canvas_item_destroy(this->cl1);
        this->cl1 = NULL;
    }

    if (this->expecting_clicks_for_LPE > 0) {
        // we received too few clicks to sanely set the parameter path so we remove the LPE from the item
        sp_lpe_item_remove_current_path_effect(this->waiting_item, false);
    }
}

void sp_pen_context_set_polyline_mode(SPPenContext *const pc) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint mode = prefs->getInt("/tools/freehand/pen/freehand-mode", 0);
    pc->polylines_only = (mode == 2 || mode == 3);
    pc->polylines_paraxial = (mode == 3);
}

/**
 * Callback to initialize SPPenContext object.
 */
void SPPenContext::setup() {
    SPDrawContext::setup();

    ControlManager &mgr = ControlManager::getManager();

    // Pen indicators
    this->c0 = mgr.createControl(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(this)), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(this->c0);

    this->c1 = mgr.createControl(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(this)), Inkscape::CTRL_TYPE_ADJ_HANDLE);
    mgr.track(this->c1);

    this->cl0 = mgr.createControlLine(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(this)));
    this->cl1 = mgr.createControlLine(sp_desktop_controls(SP_EVENT_CONTEXT_DESKTOP(this)));

    sp_canvas_item_hide(this->c0);
    sp_canvas_item_hide(this->c1);
    sp_canvas_item_hide(this->cl0);
    sp_canvas_item_hide(this->cl1);

    sp_event_context_read(this, "mode");

    this->anchor_statusbar = false;

    sp_pen_context_set_polyline_mode(this);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/freehand/pen/selcue")) {
        this->enableSelectionCue();
    }
}

static void pen_cancel (SPPenContext *const pc)
{
    pc->num_clicks = 0;
    pc->state = SPPenContext::STOP;
    spdc_reset_colors(pc);
    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);
    pc->message_context->clear();
    pc->message_context->flash(Inkscape::NORMAL_MESSAGE, _("Drawing cancelled"));

    pc->desktop->canvas->endForcedFullRedraws();
}

/**
 * Finalization callback.
 */
void SPPenContext::finish() {
    sp_event_context_discard_delayed_snap_event(this);

    if (this->npoints != 0) {
        pen_cancel(this);
    }

    SPDrawContext::finish();
}

/**
 * Callback that sets key to value in pen context.
 */
void SPPenContext::set(const Inkscape::Preferences::Entry& val) {
    Glib::ustring name = val.getEntryName();

    if (name == "mode") {
        if ( val.getString() == "drag" ) {
            this->mode = MODE_DRAG;
        } else {
            this->mode = MODE_CLICK;
        }
    }
}

/**
 * Snaps new node relative to the previous node.
 */
static void spdc_endpoint_snap(SPPenContext const *const pc, Geom::Point &p, guint const state)
{
    if ((state & GDK_CONTROL_MASK) && !pc->polylines_paraxial) { //CTRL enables angular snapping
        if (pc->npoints > 0) {
            spdc_endpoint_snap_rotation(pc, p, pc->p[0], state);
        }
    } else {
        // We cannot use shift here to disable snapping because the shift-key is already used
        // to toggle the paraxial direction; if the user wants to disable snapping (s)he will
        // have to use the %-key, the menu, or the snap toolbar
        if ((pc->npoints > 0) && pc->polylines_paraxial) {
            // snap constrained
            pen_set_to_nearest_horiz_vert(pc, p, state, true);
        } else {
            // snap freely
            boost::optional<Geom::Point> origin = pc->npoints > 0 ? pc->p[0] : boost::optional<Geom::Point>();
            spdc_endpoint_snap_free(pc, p, origin, state); // pass the origin, to allow for perpendicular / tangential snapping
        }
    }
}

/**
 * Snaps new node's handle relative to the new node.
 */
static void spdc_endpoint_snap_handle(SPPenContext const *const pc, Geom::Point &p, guint const state)
{
    g_return_if_fail(( pc->npoints == 2 ||
                       pc->npoints == 5   ));

    if ((state & GDK_CONTROL_MASK)) { //CTRL enables angular snapping
        spdc_endpoint_snap_rotation(pc, p, pc->p[pc->npoints - 2], state);
    } else {
        if (!(state & GDK_SHIFT_MASK)) { //SHIFT disables all snapping, except the angular snapping above
            boost::optional<Geom::Point> origin = pc->p[pc->npoints - 2];
            spdc_endpoint_snap_free(pc, p, origin, state);
        }
    }
}

bool SPPenContext::item_handler(SPItem* item, GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(this, event->button);
            break;
        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(this, event->button);
            break;
        default:
            break;
    }

    if (!ret) {
    	ret = SPDrawContext::item_handler(item, event);
    }

    return ret;
}

/**
 * Callback to handle all pen events.
 */
bool SPPenContext::root_handler(GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(this, event->button);
            break;

        case GDK_MOTION_NOTIFY:
            ret = pen_handle_motion_notify(this, event->motion);
            break;

        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(this, event->button);
            break;

        case GDK_2BUTTON_PRESS:
            ret = pen_handle_2button_press(this, event->button);
            break;

        case GDK_KEY_PRESS:
            ret = pen_handle_key_press(this, event);
            break;

        default:
            break;
    }

    if (!ret) {
    	ret = SPDrawContext::root_handler(event);
    }

    return ret;
}

/**
 * Handle mouse button press event.
 */
static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }

    SPDrawContext * const dc = SP_DRAW_CONTEXT(pc);
    SPDesktop * const desktop = SP_EVENT_CONTEXT_DESKTOP(dc);
    Geom::Point const event_w(bevent.x, bevent.y);
    Geom::Point event_dt(desktop->w2d(event_w));
    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);

    gint ret = FALSE;
    if (bevent.button == 1 && !event_context->space_panning
        // make sure this is not the last click for a waiting LPE (otherwise we want to finish the path)
        && pc->expecting_clicks_for_LPE != 1) {

        if (Inkscape::have_viable_layer(desktop, dc->message_context) == false) {
            return TRUE;
        }

        if (!pc->grab ) {
            // Grab mouse, so release will not pass unnoticed
            pc->grab = SP_CANVAS_ITEM(desktop->acetate);
            sp_canvas_item_grab(pc->grab, ( GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK   |
                                            GDK_BUTTON_RELEASE_MASK |
                                            GDK_POINTER_MOTION_MASK  ),
                                NULL, bevent.time);
        }

        pen_drag_origin_w = event_w;
        pen_within_tolerance = true;

        // Test whether we hit any anchor.
        SPDrawAnchor * const anchor = spdc_test_inside(pc, event_w);

        switch (pc->mode) {
            case SPPenContext::MODE_CLICK:
                // In click mode we add point on release
                switch (pc->state) {
                    case SPPenContext::POINT:
                    case SPPenContext::CONTROL:
                    case SPPenContext::CLOSE:
                        break;
                    case SPPenContext::STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = SPPenContext::POINT;
                        break;
                    default:
                        break;
                }
                break;
            case SPPenContext::MODE_DRAG:
                switch (pc->state) {
                    case SPPenContext::STOP:
                        // This is allowed, if we just canceled curve
                    case SPPenContext::POINT:
                        if (pc->npoints == 0) {

                            Geom::Point p;
                            if ((bevent.state & GDK_CONTROL_MASK) && (pc->polylines_only || pc->polylines_paraxial)) {
                                p = event_dt;
                                if (!(bevent.state & GDK_SHIFT_MASK)) {
                                    SnapManager &m = desktop->namedview->snap_manager;
                                    m.setup(desktop);
                                    m.freeSnapReturnByRef(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
                                    m.unSetup();
                                }
                              spdc_create_single_dot(event_context, p, "/tools/freehand/pen", bevent.state);
                              ret = TRUE;
                              break;
                            }

                            // TODO: Perhaps it would be nicer to rearrange the following case
                            // distinction so that the case of a waiting LPE is treated separately

                            // Set start anchor
                            pc->sa = anchor;
                            if (anchor && !sp_pen_context_has_waiting_LPE(pc)) {
                                // Adjust point to anchor if needed; if we have a waiting LPE, we need
                                // a fresh path to be created so don't continue an existing one
                                p = anchor->dp;
                                desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Continuing selected path"));
                            } else {
                                // This is the first click of a new curve; deselect item so that
                                // this curve is not combined with it (unless it is drawn from its
                                // anchor, which is handled by the sibling branch above)
                                Inkscape::Selection * const selection = sp_desktop_selection(desktop);
                                if (!(bevent.state & GDK_SHIFT_MASK) || sp_pen_context_has_waiting_LPE(pc)) {
                                    // if we have a waiting LPE, we need a fresh path to be created
                                    // so don't append to an existing one
                                    selection->clear();
                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating new path"));
                                } else if (selection->singleItem() && SP_IS_PATH(selection->singleItem())) {
                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Appending to selected path"));
                                }

                                // Create green anchor
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state);
                                pc->green_anchor = sp_draw_anchor_new(pc, pc->green_curve, TRUE, p);
                            }
                            spdc_pen_set_initial_point(pc, p);
                        } else {

                            // Set end anchor
                            pc->ea = anchor;
                            Geom::Point p;
                            if (anchor) {
                                p = anchor->dp;
                                // we hit an anchor, will finish the curve (either with or without closing)
                                // in release handler
                                pc->state = SPPenContext::CLOSE;

                                if (pc->green_anchor && pc->green_anchor->active) {
                                    // we clicked on the current curve start, so close it even if
                                    // we drag a handle away from it
                                    dc->green_closed = TRUE;
                                }
                                ret = TRUE;
                                break;

                            } else {
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state); // Snap node only if not hitting anchor.
                                spdc_pen_set_subsequent_point(pc, p, true);
                            }
                        }

                        pc->state = pc->polylines_only ? SPPenContext::POINT : SPPenContext::CONTROL;
                        ret = TRUE;
                        break;
                    case SPPenContext::CONTROL:
                        g_warning("Button down in CONTROL state");
                        break;
                    case SPPenContext::CLOSE:
                        g_warning("Button down in CLOSE state");
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    } else if (pc->expecting_clicks_for_LPE == 1 && pc->npoints != 0) {
        // when the last click for a waiting LPE occurs we want to finish the path
        spdc_pen_finish_segment(pc, event_dt, bevent.state);
        if (pc->green_closed) {
            // finishing at the start anchor, close curve
            spdc_pen_finish(pc, TRUE);
        } else {
            // finishing at some other anchor, finish curve but not close
            spdc_pen_finish(pc, FALSE);
        }

        ret = TRUE;
    } else if (bevent.button == 3 && pc->npoints != 0) {
        // right click - finish path
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }

    if (pc->expecting_clicks_for_LPE > 0) {
        --pc->expecting_clicks_for_LPE;
    }

    return ret;
}

/**
 * Handle motion_notify event.
 */
static gint pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent)
{
    gint ret = FALSE;

    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);
    SPDesktop * const dt = SP_EVENT_CONTEXT_DESKTOP(event_context);

    if (event_context->space_panning || mevent.state & GDK_BUTTON2_MASK || mevent.state & GDK_BUTTON3_MASK) {
        // allow scrolling
        return FALSE;
    }

    if (pc->events_disabled) {
        // skip motion events if pen events are disabled
        return FALSE;
    }

    Geom::Point const event_w(mevent.x,
                            mevent.y);
    if (pen_within_tolerance) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        gint const tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
        if ( Geom::LInfty( event_w - pen_drag_origin_w ) < tolerance ) {
            return FALSE;   // Do not drag if we're within tolerance from origin.
        }
    }
    // Once the user has moved farther than tolerance from the original location
    // (indicating they intend to move the object, not click), then always process the
    // motion notify coordinates as given (no snapping back to origin)
    pen_within_tolerance = false;

    // Find desktop coordinates
    Geom::Point p = dt->w2d(event_w);

    // Test, whether we hit any anchor
    SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);

    switch (pc->mode) {
        case SPPenContext::MODE_CLICK:
            switch (pc->state) {
                case SPPenContext::POINT:
                    if ( pc->npoints != 0 ) {
                        // Only set point, if we are already appending
                        spdc_endpoint_snap(pc, p, mevent.state);
                        spdc_pen_set_subsequent_point(pc, p, true);
                        ret = TRUE;
                    } else if (!sp_event_context_knot_mouseover(pc)) {
                        SnapManager &m = dt->namedview->snap_manager;
                        m.setup(dt);
                        m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                        m.unSetup();
                    }
                    break;
                case SPPenContext::CONTROL:
                case SPPenContext::CLOSE:
                    // Placing controls is last operation in CLOSE state
                    spdc_endpoint_snap(pc, p, mevent.state);
                    spdc_pen_set_ctrl(pc, p, mevent.state);
                    ret = TRUE;
                    break;
                case SPPenContext::STOP:
                    // This is perfectly valid
                    break;
                default:
                    break;
            }
            break;
        case SPPenContext::MODE_DRAG:
            switch (pc->state) {
                case SPPenContext::POINT:
                    if ( pc->npoints > 0 ) {
                        // Only set point, if we are already appending

                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, mevent.state);
                            spdc_pen_set_subsequent_point(pc, p, true, mevent.state);
                        } else {
                            spdc_pen_set_subsequent_point(pc, anchor->dp, false, mevent.state);
                        }

                        if (anchor && !pc->anchor_statusbar) {
                            pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path."));
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->message_context->clear();
                            pc->anchor_statusbar = false;
                        }

                        ret = TRUE;
                    } else {
                        if (anchor && !pc->anchor_statusbar) {
                            pc->message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point."));
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->message_context->clear();
                            pc->anchor_statusbar = false;
                        }
                        if (!sp_event_context_knot_mouseover(pc)) {
                            SnapManager &m = dt->namedview->snap_manager;
                            m.setup(dt);
                            m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                            m.unSetup();
                        }
                    }
                    break;
                case SPPenContext::CONTROL:
                case SPPenContext::CLOSE:
                    // Placing controls is last operation in CLOSE state

                    // snap the handle
                    spdc_endpoint_snap_handle(pc, p, mevent.state);

                    if (!pc->polylines_only) {
                        spdc_pen_set_ctrl(pc, p, mevent.state);
                    } else {
                        spdc_pen_set_ctrl(pc, pc->p[1], mevent.state);
                    }
                    gobble_motion_events(GDK_BUTTON1_MASK);
                    ret = TRUE;
                    break;
                case SPPenContext::STOP:
                    // This is perfectly valid
                    break;
                default:
                    if (!sp_event_context_knot_mouseover(pc)) {
                        SnapManager &m = dt->namedview->snap_manager;
                        m.setup(dt);
                        m.preSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE));
                        m.unSetup();
                    }
                    break;
            }
            break;
        default:
            break;
    }
    return ret;
}

/**
 * Handle mouse button release event.
 */
static gint pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent)
{
    if (pc->events_disabled) {
        // skip event processing if events are disabled
        return FALSE;
    }

    gint ret = FALSE;
    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);
    if ( revent.button == 1  && !event_context->space_panning) {

        SPDrawContext *dc = SP_DRAW_CONTEXT (pc);

        Geom::Point const event_w(revent.x,
                                revent.y);
        // Find desktop coordinates
        Geom::Point p = pc->desktop->w2d(event_w);

        // Test whether we hit any anchor.
        SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);

        switch (pc->mode) {
            case SPPenContext::MODE_CLICK:
                switch (pc->state) {
                    case SPPenContext::POINT:
                        if ( pc->npoints == 0 ) {
                            // Start new thread only with button release
                            if (anchor) {
                                p = anchor->dp;
                            }
                            pc->sa = anchor;
                            spdc_pen_set_initial_point(pc, p);
                        } else {
                            // Set end anchor here
                            pc->ea = anchor;
                            if (anchor) {
                                p = anchor->dp;
                            }
                        }
                        pc->state = SPPenContext::CONTROL;
                        ret = TRUE;
                        break;
                    case SPPenContext::CONTROL:
                        // End current segment
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        pc->state = SPPenContext::POINT;
                        ret = TRUE;
                        break;
                    case SPPenContext::CLOSE:
                        // End current segment
                        if (!anchor) {   // Snap node only if not hitting anchor
                            spdc_endpoint_snap(pc, p, revent.state);
                        }
                        spdc_pen_finish_segment(pc, p, revent.state);
                        spdc_pen_finish(pc, TRUE);
                        pc->state = SPPenContext::POINT;
                        ret = TRUE;
                        break;
                    case SPPenContext::STOP:
                        // This is allowed, if we just canceled curve
                        pc->state = SPPenContext::POINT;
                        ret = TRUE;
                        break;
                    default:
                        break;
                }
                break;
            case SPPenContext::MODE_DRAG:
                switch (pc->state) {
                    case SPPenContext::POINT:
                    case SPPenContext::CONTROL:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        break;
                    case SPPenContext::CLOSE:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        if (pc->green_closed) {
                            // finishing at the start anchor, close curve
                            spdc_pen_finish(pc, TRUE);
                        } else {
                            // finishing at some other anchor, finish curve but not close
                            spdc_pen_finish(pc, FALSE);
                        }
                        break;
                    case SPPenContext::STOP:
                        // This is allowed, if we just cancelled curve
                        break;
                    default:
                        break;
                }
                pc->state = SPPenContext::POINT;
                ret = TRUE;
                break;
            default:
                break;
        }

        if (pc->grab) {
            // Release grab now
            sp_canvas_item_ungrab(pc->grab, revent.time);
            pc->grab = NULL;
        }

        ret = TRUE;

        dc->green_closed = FALSE;
    }

    // TODO: can we be sure that the path was created correctly?
    // TODO: should we offer an option to collect the clicks in a list?
    if (pc->expecting_clicks_for_LPE == 0 && sp_pen_context_has_waiting_LPE(pc)) {
        sp_pen_context_set_polyline_mode(pc);

        SPEventContext *ec = SP_EVENT_CONTEXT(pc);
        Inkscape::Selection *selection = sp_desktop_selection (ec->desktop);

        if (pc->waiting_LPE) {
            // we have an already created LPE waiting for a path
            pc->waiting_LPE->acceptParamPath(SP_PATH(selection->singleItem()));
            selection->add(SP_OBJECT(pc->waiting_item));
            pc->waiting_LPE = NULL;
        } else {
            // the case that we need to create a new LPE and apply it to the just-drawn path is
            // handled in spdc_check_for_and_apply_waiting_LPE() in draw-context.cpp
        }
    }

    return ret;
}

static gint pen_handle_2button_press(SPPenContext *const pc, GdkEventButton const &bevent)
{
    gint ret = FALSE;
    // only end on LMB double click. Otherwise horizontal scrolling causes ending of the path
    if (pc->npoints != 0 && bevent.button == 1) {
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }
    return ret;
}

static void pen_redraw_all (SPPenContext *const pc)
{
    // green
    if (pc->green_bpaths) {
        // remove old piecewise green canvasitems
        while (pc->green_bpaths) {
            sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
            pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
        }
        // one canvas bpath for all of green_curve
        SPCanvasItem *cshape = sp_canvas_bpath_new(sp_desktop_sketch(pc->desktop), pc->green_curve);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cshape), 0, SP_WIND_RULE_NONZERO);

        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);
    }

    if (pc->green_anchor)
        SP_CTRL(pc->green_anchor->ctrl)->moveto(pc->green_anchor->dp);

    pc->red_curve->reset();
    pc->red_curve->moveto(pc->p[0]);
    pc->red_curve->curveto(pc->p[1], pc->p[2], pc->p[3]);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);

    // handles
    if (pc->p[0] != pc->p[1]) {
        SP_CTRL(pc->c1)->moveto(pc->p[1]);
        pc->cl1->setCoords(pc->p[0], pc->p[1]);
        sp_canvas_item_show(pc->c1);
        sp_canvas_item_show(pc->cl1);
    } else {
        sp_canvas_item_hide(pc->c1);
        sp_canvas_item_hide(pc->cl1);
    }

    Geom::Curve const * last_seg = pc->green_curve->last_segment();
    if (last_seg) {
        Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>( last_seg );
        if ( cubic &&
             (*cubic)[2] != pc->p[0] )
        {
            Geom::Point p2 = (*cubic)[2];
            SP_CTRL(pc->c0)->moveto(p2);
            pc->cl0->setCoords(p2, pc->p[0]);
            sp_canvas_item_show(pc->c0);
            sp_canvas_item_show(pc->cl0);
        } else {
            sp_canvas_item_hide(pc->c0);
            sp_canvas_item_hide(pc->cl0);
        }
    }
}

static void pen_lastpoint_move (SPPenContext *const pc, gdouble x, gdouble y)
{
    if (pc->npoints != 5)
        return;

    // green
    if (!pc->green_curve->is_empty()) {
        pc->green_curve->last_point_additive_move( Geom::Point(x,y) );
    } else {
        // start anchor too
        if (pc->green_anchor) {
            pc->green_anchor->dp += Geom::Point(x, y);
        }
    }

    // red
    pc->p[0] += Geom::Point(x, y);
    pc->p[1] += Geom::Point(x, y);
    pen_redraw_all(pc);
}

static void pen_lastpoint_move_screen (SPPenContext *const pc, gdouble x, gdouble y)
{
    pen_lastpoint_move (pc, x / pc->desktop->current_zoom(), y / pc->desktop->current_zoom());
}

static void pen_lastpoint_tocurve (SPPenContext *const pc)
{
    if (pc->npoints != 5)
        return;

    Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>( pc->green_curve->last_segment() );
    if ( cubic ) {
        pc->p[1] = pc->p[0] + (Geom::Point)( (*cubic)[3] - (*cubic)[2] );
    } else {
        pc->p[1] = pc->p[0] + (1./3)*(pc->p[3] - pc->p[0]);
    }

    pen_redraw_all(pc);
}

static void pen_lastpoint_toline (SPPenContext *const pc)
{
    if (pc->npoints != 5)
        return;

    pc->p[1] = pc->p[0];

    pen_redraw_all(pc);
}


static gint pen_handle_key_press(SPPenContext *const pc, GdkEvent *event)
{

    gint ret = FALSE;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gdouble const nudge = prefs->getDoubleLimited("/options/nudgedistance/value", 2, 0, 1000, "px"); // in px

    switch (get_group0_keyval (&event->key)) {

        case GDK_KEY_Left: // move last point left
        case GDK_KEY_KP_Left:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, -10, 0); // shift
                    else pen_lastpoint_move_screen(pc, -1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, -10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, -nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Up: // move last point up
        case GDK_KEY_KP_Up:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 0, 10); // shift
                    else pen_lastpoint_move_screen(pc, 0, 1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 0, 10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, nudge); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Right: // move last point right
        case GDK_KEY_KP_Right:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 10, 0); // shift
                    else pen_lastpoint_move_screen(pc, 1, 0); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 10*nudge, 0); // shift
                    else pen_lastpoint_move(pc, nudge, 0); // no shift
                }
                ret = TRUE;
            }
            break;
        case GDK_KEY_Down: // move last point down
        case GDK_KEY_KP_Down:
            if (!MOD__CTRL(event)) { // not ctrl
                if (MOD__ALT(event)) { // alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move_screen(pc, 0, -10); // shift
                    else pen_lastpoint_move_screen(pc, 0, -1); // no shift
                }
                else { // no alt
                    if (MOD__SHIFT(event)) pen_lastpoint_move(pc, 0, -10*nudge); // shift
                    else pen_lastpoint_move(pc, 0, -nudge); // no shift
                }
                ret = TRUE;
            }
            break;

/*TODO: this is not yet enabled?? looks like some traces of the Geometry tool
        case GDK_KEY_P:
        case GDK_KEY_p:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PARALLEL, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_C:
        case GDK_KEY_c:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::CIRCLE_3PTS, 3);
                ret = TRUE;
            }
            break;

        case GDK_KEY_B:
        case GDK_KEY_b:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::PERP_BISECTOR, 2);
                ret = TRUE;
            }
            break;

        case GDK_KEY_A:
        case GDK_KEY_a:
            if (MOD__SHIFT_ONLY(event)) {
                sp_pen_context_wait_for_LPE_mouse_clicks(pc, Inkscape::LivePathEffect::ANGLE_BISECTOR, 3);
                ret = TRUE;
            }
            break;
*/

        case GDK_KEY_U:
        case GDK_KEY_u:
            if (MOD__SHIFT_ONLY(event)) {
                pen_lastpoint_tocurve(pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_L:
        case GDK_KEY_l:
            if (MOD__SHIFT_ONLY(event)) {
                pen_lastpoint_toline(pc);
                ret = TRUE;
            }
            break;

        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            if (pc->npoints != 0) {
                spdc_pen_finish(pc, FALSE);
                ret = TRUE;
            }
            break;
        case GDK_KEY_Escape:
            if (pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for deselecting
                pen_cancel (pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (MOD__CTRL_ONLY(event) && pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for undo
                pen_cancel (pc);
                ret = TRUE;
            }
            break;
        case GDK_KEY_g:
        case GDK_KEY_G:
            if (MOD__SHIFT_ONLY(event)) {
                sp_selection_to_guides(SP_EVENT_CONTEXT(pc)->desktop);
                ret = true;
            }
            break;
        case GDK_KEY_BackSpace:
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            if ( pc->green_curve->is_empty() || (pc->green_curve->last_segment() == NULL) ) {
                if (!pc->red_curve->is_empty()) {
                    pen_cancel (pc);
                    ret = TRUE;
                } else {
                    // do nothing; this event should be handled upstream
                }
            } else {
                // Reset red curve
                pc->red_curve->reset();
                // Destroy topmost green bpath
                if (pc->green_bpaths) {
                    if (pc->green_bpaths->data)
                        sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
                    pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
                }
                // Get last segment
                if ( pc->green_curve->is_empty() ) {
                    g_warning("pen_handle_key_press, case GDK_KP_Delete: Green curve is empty");
                    break;
                }
                // The code below assumes that pc->green_curve has only ONE path !
                Geom::Curve const * crv = pc->green_curve->last_segment();
                pc->p[0] = crv->initialPoint();
                if ( Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const *>(crv)) {
                    pc->p[1] = (*cubic)[1];
                } else {
                    pc->p[1] = pc->p[0];
                }
                Geom::Point const pt(( pc->npoints < 4
                                     ? (Geom::Point)(crv->finalPoint())
                                     : pc->p[3] ));
                pc->npoints = 2;
                pc->green_curve->backspace();
                sp_canvas_item_hide(pc->c0);
                sp_canvas_item_hide(pc->c1);
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                pc->state = SPPenContext::POINT;
                spdc_pen_set_subsequent_point(pc, pt, true);
                pen_last_paraxial_dir = !pen_last_paraxial_dir;
                ret = TRUE;
            }
            break;
        default:
            break;
    }
    return ret;
}

static void spdc_reset_colors(SPPenContext *pc)
{
    // Red
    pc->red_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);
    // Blue
    pc->blue_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), NULL);
    // Green
    while (pc->green_bpaths) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(pc->green_bpaths->data));
        pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
    }
    pc->green_curve->reset();
    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }
    pc->sa = NULL;
    pc->ea = NULL;
    pc->npoints = 0;
    pc->red_curve_is_valid = false;
}


static void spdc_pen_set_initial_point(SPPenContext *const pc, Geom::Point const p)
{
    g_assert( pc->npoints == 0 );

    pc->p[0] = p;
    pc->p[1] = p;
    pc->npoints = 2;
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);

    pc->desktop->canvas->forceFullRedrawAfterInterruptions(5);
}

/**
 * Show the status message for the current line/curve segment.
 * This type of message always shows angle/distance as the last
 * two parameters ("angle %3.2f&#176;, distance %s").
 */
static void spdc_pen_set_angle_distance_status_message(SPPenContext *const pc, Geom::Point const p, int pc_point_to_compare, gchar const *message)
{
    g_assert(pc != NULL);
    g_assert((pc_point_to_compare == 0) || (pc_point_to_compare == 3)); // exclude control handles
    g_assert(message != NULL);

    SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
    Geom::Point rel = p - pc->p[pc_point_to_compare];
    GString *dist = SP_PX_TO_METRIC_STRING(Geom::L2(rel), desktop->namedview->getDefaultMetric());
    double angle = atan2(rel[Geom::Y], rel[Geom::X]) * 180 / M_PI;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/compassangledisplay/value", 0) != 0)
        angle = angle_to_compass (angle);

    pc->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, message, angle, dist->str);
    g_string_free(dist, FALSE);
}

static void spdc_pen_set_subsequent_point(SPPenContext *const pc, Geom::Point const p, bool statusbar, guint status)
{
    g_assert( pc->npoints != 0 );
    // todo: Check callers to see whether 2 <= npoints is guaranteed.

    pc->p[2] = p;
    pc->p[3] = p;
    pc->p[4] = p;
    pc->npoints = 5;
    pc->red_curve->reset();
    bool is_curve;
    pc->red_curve->moveto(pc->p[0]);
    if (pc->polylines_paraxial && !statusbar) {
        // we are drawing horizontal/vertical lines and hit an anchor;
        Geom::Point const origin = pc->p[0];
        // if the previous point and the anchor are not aligned either horizontally or vertically...
        if ((abs(p[Geom::X] - origin[Geom::X]) > 1e-9) && (abs(p[Geom::Y] - origin[Geom::Y]) > 1e-9)) {
            // ...then we should draw an L-shaped path, consisting of two paraxial segments
            Geom::Point intermed = p;
            pen_set_to_nearest_horiz_vert(pc, intermed, status, false);
            pc->red_curve->lineto(intermed);
        }
        pc->red_curve->lineto(p);
        is_curve = false;
    } else {
        // one of the 'regular' modes
        if (pc->p[1] != pc->p[0]) {
            pc->red_curve->curveto(pc->p[1], p, p);
            is_curve = true;
        } else {
            pc->red_curve->lineto(p);
            is_curve = false;
        }
    }

    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);

    if (statusbar) {
        gchar *message = is_curve ?
            _("<b>Curve segment</b>: angle %3.2f&#176;, distance %s; with <b>Ctrl</b> to snap angle, <b>Enter</b> to finish the path" ):
            _("<b>Line segment</b>: angle %3.2f&#176;, distance %s; with <b>Ctrl</b> to snap angle, <b>Enter</b> to finish the path");
        spdc_pen_set_angle_distance_status_message(pc, p, 0, message);
    }
}

static void spdc_pen_set_ctrl(SPPenContext *const pc, Geom::Point const p, guint const state)
{
    sp_canvas_item_show(pc->c1);
    sp_canvas_item_show(pc->cl1);

    if ( pc->npoints == 2 ) {
        pc->p[1] = p;
        sp_canvas_item_hide(pc->c0);
        sp_canvas_item_hide(pc->cl0);
        SP_CTRL(pc->c1)->moveto(pc->p[1]);
        pc->cl1->setCoords(pc->p[0], pc->p[1]);

        spdc_pen_set_angle_distance_status_message(pc, p, 0, _("<b>Curve handle</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle"));
    } else if ( pc->npoints == 5 ) {
        pc->p[4] = p;
        sp_canvas_item_show(pc->c0);
        sp_canvas_item_show(pc->cl0);
        bool is_symm = false;
        if ( ( ( pc->mode == SPPenContext::MODE_CLICK ) && ( state & GDK_CONTROL_MASK ) ) ||
             ( ( pc->mode == SPPenContext::MODE_DRAG ) &&  !( state & GDK_SHIFT_MASK  ) ) ) {
            Geom::Point delta = p - pc->p[3];
            pc->p[2] = pc->p[3] - delta;
            is_symm = true;
            pc->red_curve->reset();
            pc->red_curve->moveto(pc->p[0]);
            pc->red_curve->curveto(pc->p[1], pc->p[2], pc->p[3]);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
        }
        SP_CTRL(pc->c0)->moveto(pc->p[2]);
        pc->cl0 ->setCoords(pc->p[3], pc->p[2]);
        SP_CTRL(pc->c1)->moveto(pc->p[4]);
        pc->cl1->setCoords(pc->p[3], pc->p[4]);

        gchar *message = is_symm ?
            _("<b>Curve handle, symmetric</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle, with <b>Shift</b> to move this handle only") :
            _("<b>Curve handle</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle, with <b>Shift</b> to move this handle only");
        spdc_pen_set_angle_distance_status_message(pc, p, 3, message);
    } else {
        g_warning("Something bad happened - npoints is %d", pc->npoints);
    }
}

static void spdc_pen_finish_segment(SPPenContext *const pc, Geom::Point const p, guint const state)
{
    if (pc->polylines_paraxial) {
        pen_last_paraxial_dir = pen_next_paraxial_direction(pc, p, pc->p[0], state);
    }

    ++pc->num_clicks;

    if (!pc->red_curve->is_empty()) {
        pc->green_curve->append_continuous(pc->red_curve, 0.0625);
        SPCurve *curve = pc->red_curve->copy();
        /// \todo fixme:
        SPCanvasItem *cshape = sp_canvas_bpath_new(sp_desktop_sketch(pc->desktop), curve);
        curve->unref();
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);

        pc->p[0] = pc->p[3];
        pc->p[1] = pc->p[4];
        pc->npoints = 2;

        pc->red_curve->reset();
    }
}

static void spdc_pen_finish(SPPenContext *const pc, gboolean const closed)
{
    if (pc->expecting_clicks_for_LPE > 1) {
        // don't let the path be finished before we have collected the required number of mouse clicks
        return;
    }

    pc->num_clicks = 0;

    pen_disable_events(pc);

    SPDesktop *const desktop = pc->desktop;
    pc->message_context->clear();
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Drawing finished"));

    pc->red_curve->reset();
    spdc_concat_colors_and_flush(pc, closed);
    pc->sa = NULL;
    pc->ea = NULL;

    pc->npoints = 0;
    pc->state = SPPenContext::POINT;

    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);

    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }


    pc->desktop->canvas->endForcedFullRedraws();

    pen_enable_events(pc);
}

static void pen_disable_events(SPPenContext *const pc) {
  pc->events_disabled++;
}

static void pen_enable_events(SPPenContext *const pc) {
  g_return_if_fail(pc->events_disabled != 0);

  pc->events_disabled--;
}

void sp_pen_context_wait_for_LPE_mouse_clicks(SPPenContext *pc, Inkscape::LivePathEffect::EffectType effect_type,
                                         unsigned int num_clicks, bool use_polylines)
{
    if (effect_type == Inkscape::LivePathEffect::INVALID_LPE)
        return;

    pc->waiting_LPE_type = effect_type;
    pc->expecting_clicks_for_LPE = num_clicks;
    pc->polylines_only = use_polylines;
    pc->polylines_paraxial = false; // TODO: think if this is correct for all cases
}

void sp_pen_context_cancel_waiting_for_LPE(SPPenContext *pc)
{
    pc->waiting_LPE_type = Inkscape::LivePathEffect::INVALID_LPE;
    pc->expecting_clicks_for_LPE = 0;
    sp_pen_context_set_polyline_mode(pc);
}

static int pen_next_paraxial_direction(const SPPenContext *const pc,
                                       Geom::Point const &pt, Geom::Point const &origin, guint state) {
    //
    // after the first mouse click we determine whether the mouse pointer is closest to a
    // horizontal or vertical segment; for all subsequent mouse clicks, we use the direction
    // orthogonal to the last one; pressing Shift toggles the direction
    //
    // num_clicks is not reliable because spdc_pen_finish_segment is sometimes called too early
    // (on first mouse release), in which case num_clicks immediately becomes 1.
    // if (pc->num_clicks == 0) {

    if (pc->green_curve->is_empty()) {
        // first mouse click
        double dist_h = fabs(pt[Geom::X] - origin[Geom::X]);
        double dist_v = fabs(pt[Geom::Y] - origin[Geom::Y]);
        int ret = (dist_h < dist_v) ? 1 : 0; // 0 = horizontal, 1 = vertical
        pen_last_paraxial_dir = (state & GDK_SHIFT_MASK) ? 1 - ret : ret;
        return pen_last_paraxial_dir;
    } else {
        // subsequent mouse click
        return (state & GDK_SHIFT_MASK) ? pen_last_paraxial_dir : 1 - pen_last_paraxial_dir;
    }
}

void pen_set_to_nearest_horiz_vert(const SPPenContext *const pc, Geom::Point &pt, guint const state, bool snap)
{
    Geom::Point const origin = pc->p[0];

    int next_dir = pen_next_paraxial_direction(pc, pt, origin, state);

    if (!snap) {
        if (next_dir == 0) {
            // line is forced to be horizontal
            pt[Geom::Y] = origin[Geom::Y];
        } else {
            // line is forced to be vertical
            pt[Geom::X] = origin[Geom::X];
        }
    } else {
        // Create a horizontal or vertical constraint line
        Inkscape::Snapper::SnapConstraint cl(origin, next_dir ? Geom::Point(0, 1) : Geom::Point(1, 0));

        // Snap along the constraint line; if we didn't snap then still the constraint will be applied
        SnapManager &m = pc->desktop->namedview->snap_manager;

        Inkscape::Selection *selection = sp_desktop_selection (pc->desktop);
        // selection->singleItem() is the item that is currently being drawn. This item will not be snapped to (to avoid self-snapping)
        // TODO: Allow snapping to the stationary parts of the item, and only ignore the last segment

        m.setup(pc->desktop, true, selection->singleItem());
        m.constrainedSnapReturnByRef(pt, Inkscape::SNAPSOURCE_NODE_HANDLE, cl);
        m.unSetup();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
