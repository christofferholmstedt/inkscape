/*
 * 3D box drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007      Maximilian Albert <Anhalter42@gmx.de>
 * Copyright (C) 2006      Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2000-2005 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"

#include <gdk/gdkkeysyms.h>

#include "macros.h"
#include "display/sp-canvas.h"
#include "document.h"
#include "document-undo.h"
#include "sp-namedview.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "desktop-handles.h"
#include "snap.h"
#include "display/curve.h"
#include "display/sp-canvas-item.h"
#include "desktop.h"
#include "message-context.h"
#include "pixmaps/cursor-3dbox.xpm"
#include "box3d.h"
#include "box3d-context.h"
#include "sp-metrics.h"
#include <glibmm/i18n.h>
#include "xml/repr.h"
#include "xml/node-event-vector.h"
#include "preferences.h"
#include "context-fns.h"
#include "desktop-style.h"
#include "transf_mat_3x4.h"
#include "perspective-line.h"
#include "persp3d.h"
#include "box3d-side.h"
#include "document-private.h"
#include "line-geometry.h"
#include "shape-editor.h"
#include "verbs.h"

using Inkscape::DocumentUndo;

static void sp_box3d_drag(Box3DContext &bc, guint state);
static void sp_box3d_finish(Box3DContext *bc);


#include "sp-factory.h"

namespace {
	SPEventContext* createBox3dContext() {
		return new Box3DContext();
	}

	bool box3dContextRegistered = ToolFactory::instance().registerObject("/tools/shapes/3dbox", createBox3dContext);
}

const std::string& Box3DContext::getPrefsPath() {
	return Box3DContext::prefsPath;
}

const std::string Box3DContext::prefsPath = "/tools/shapes/3dbox";

Box3DContext::Box3DContext() : SPEventContext() {
	Box3DContext* box3d_context = this;

	box3d_context->_message_context = 0;

    SPEventContext *event_context = SP_EVENT_CONTEXT(box3d_context);

    event_context->cursor_shape = cursor_3dbox_xpm;
    event_context->hot_x = 4;
    event_context->hot_y = 4;
    event_context->xp = 0;
    event_context->yp = 0;
    event_context->tolerance = 0;
    event_context->within_tolerance = false;
    event_context->item_to_select = NULL;

    box3d_context->item = NULL;

    box3d_context->ctrl_dragged = false;
    box3d_context->extruded = false;

    box3d_context->_vpdrag = NULL;

    //new (&box3d_context->sel_changed_connection) sigc::connection();
}

void Box3DContext::finish() {
	SPEventContext* ec = this;

    Box3DContext *bc = SP_BOX3D_CONTEXT(ec);
    SPDesktop *desktop = ec->desktop;

    sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), GDK_CURRENT_TIME);
    sp_box3d_finish(bc);
    bc->sel_changed_connection.disconnect();
//    sp_repr_remove_listener_by_data(cc->active_shape_repr, cc);

//    if (((SPEventContextClass *) sp_box3d_context_parent_class)->finish) {
//        ((SPEventContextClass *) sp_box3d_context_parent_class)->finish(ec);
//    }
    SPEventContext::finish();
}


Box3DContext::~Box3DContext() {
    Box3DContext *bc = SP_BOX3D_CONTEXT(this);
    SPEventContext *ec = SP_EVENT_CONTEXT(this);

    ec->enableGrDrag(false);

    delete (bc->_vpdrag);
    bc->_vpdrag = NULL;

    bc->sel_changed_connection.disconnect();
    //bc->sel_changed_connection.~connection();

    delete ec->shape_editor;
    ec->shape_editor = NULL;

    /* fixme: This is necessary because we do not grab */
    if (bc->item) {
        sp_box3d_finish(bc);
    }

    if (bc->_message_context) {
        delete bc->_message_context;
    }

    //G_OBJECT_CLASS(sp_box3d_context_parent_class)->dispose(object);
}

/**
 * Callback that processes the "changed" signal on the selection;
 * destroys old and creates new knotholder.
 */
static void sp_box3d_context_selection_changed(Inkscape::Selection *selection, gpointer data)
{
    Box3DContext *bc = SP_BOX3D_CONTEXT(data);
    SPEventContext *ec = SP_EVENT_CONTEXT(bc);

    ec->shape_editor->unset_item(SH_KNOTHOLDER);
    SPItem *item = selection->singleItem();
    ec->shape_editor->set_item(item, SH_KNOTHOLDER);

    if (selection->perspList().size() == 1) {
        // selecting a single box changes the current perspective
        ec->desktop->doc()->setCurrentPersp3D(selection->perspList().front());
    }
}

/* Create a default perspective in document defs if none is present (which can happen, among other
 * circumstances, after 'vacuum defs' or when a pre-0.46 file is opened).
 */
static void sp_box3d_context_ensure_persp_in_defs(SPDocument *document) {
    SPDefs *defs = document->getDefs();

    bool has_persp = false;
    for ( SPObject *child = defs->firstChild(); child; child = child->getNext() ) {
        if (SP_IS_PERSP3D(child)) {
            has_persp = true;
            break;
        }
    }

    if (!has_persp) {
        document->setCurrentPersp3D(persp3d_create_xml_element (document));
    }
}

void Box3DContext::setup() {
	SPEventContext* ec = this;

    Box3DContext *bc = SP_BOX3D_CONTEXT(ec);

//    if (((SPEventContextClass *) sp_box3d_context_parent_class)->setup) {
//        ((SPEventContextClass *) sp_box3d_context_parent_class)->setup(ec);
//    }
    SPEventContext::setup();

    ec->shape_editor = new ShapeEditor(ec->desktop);

    SPItem *item = sp_desktop_selection(ec->desktop)->singleItem();
    if (item) {
        ec->shape_editor->set_item(item, SH_KNOTHOLDER);
    }

    bc->sel_changed_connection.disconnect();
    bc->sel_changed_connection = sp_desktop_selection(ec->desktop)->connectChanged(
        sigc::bind(sigc::ptr_fun(&sp_box3d_context_selection_changed), (gpointer)bc)
    );

    bc->_vpdrag = new Box3D::VPDrag(sp_desktop_document (ec->desktop));
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/tools/shapes/selcue")) {
        ec->enableSelectionCue();
    }

    if (prefs->getBool("/tools/shapes/gradientdrag")) {
        ec->enableGrDrag();
    }

    bc->_message_context = new Inkscape::MessageContext((ec->desktop)->messageStack());
}

gint Box3DContext::item_handler(SPItem* item, GdkEvent* event) {
	SPEventContext* event_context = this;

    SPDesktop *desktop = event_context->desktop;

    gint ret = FALSE;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1 && !event_context->space_panning) {
            Inkscape::setup_for_drag_start(desktop, event_context, event);
            ret = TRUE;
        }
        break;
        // motion and release are always on root (why?)
    default:
        break;
    }

//    if (((SPEventContextClass *) sp_box3d_context_parent_class)->item_handler) {
//        ret = ((SPEventContextClass *) sp_box3d_context_parent_class)->item_handler(event_context, item, event);
//    }
    // CPPIFY: ret is always overwritten...
    ret = SPEventContext::item_handler(item, event);

    return ret;
}

gint Box3DContext::root_handler(GdkEvent* event) {
	SPEventContext* event_context = this;

    static bool dragging;

    SPDesktop *desktop = event_context->desktop;
    SPDocument *document = sp_desktop_document (desktop);
    Inkscape::Selection *selection = sp_desktop_selection (desktop);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int const snaps = prefs->getInt("/options/rotationsnapsperpi/value", 12);

    Box3DContext *bc = SP_BOX3D_CONTEXT(event_context);
    Persp3D *cur_persp = document->getCurrentPersp3D();

    event_context->tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

    gint ret = FALSE;
    switch (event->type) {
    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1  && !event_context->space_panning) {
            Geom::Point const button_w(event->button.x,
                                       event->button.y);
            Geom::Point button_dt(desktop->w2d(button_w));

            // save drag origin
            event_context->xp = (gint) button_w[Geom::X];
            event_context->yp = (gint) button_w[Geom::Y];
            event_context->within_tolerance = true;

            // remember clicked item, *not* disregarding groups (since a 3D box is a group), honoring Alt
            event_context->item_to_select = sp_event_context_find_item (desktop, button_w, event->button.state & GDK_MOD1_MASK, event->button.state & GDK_CONTROL_MASK);

            dragging = true;

            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop, true, bc->item);
            m.freeSnapReturnByRef(button_dt, Inkscape::SNAPSOURCE_NODE_HANDLE);
            m.unSetup();
            bc->center = button_dt;

            bc->drag_origin = button_dt;
            bc->drag_ptB = button_dt;
            bc->drag_ptC = button_dt;

            // This can happen after saving when the last remaining perspective was purged and must be recreated.
            if (!cur_persp) {
                sp_box3d_context_ensure_persp_in_defs(document);
                cur_persp = document->getCurrentPersp3D();
            }

            /* Projective preimages of clicked point under current perspective */
            bc->drag_origin_proj = cur_persp->perspective_impl->tmat.preimage (button_dt, 0, Proj::Z);
            bc->drag_ptB_proj = bc->drag_origin_proj;
            bc->drag_ptC_proj = bc->drag_origin_proj;
            bc->drag_ptC_proj.normalize();
            bc->drag_ptC_proj[Proj::Z] = 0.25;

            sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                ( GDK_KEY_PRESS_MASK |
                                  GDK_BUTTON_RELEASE_MASK       |
                                  GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK       |
                                  GDK_BUTTON_PRESS_MASK ),
                                NULL, event->button.time);
            ret = TRUE;
        }
        break;
    case GDK_MOTION_NOTIFY:
        if ( dragging
             && ( event->motion.state & GDK_BUTTON1_MASK )  && !event_context->space_panning)
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

            Geom::Point const motion_w(event->motion.x,
                                       event->motion.y);
            Geom::Point motion_dt(desktop->w2d(motion_w));

            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop, true, bc->item);
            m.freeSnapReturnByRef(motion_dt, Inkscape::SNAPSOURCE_NODE_HANDLE);
            bc->ctrl_dragged  = event->motion.state & GDK_CONTROL_MASK;

            if (event->motion.state & GDK_SHIFT_MASK && !bc->extruded && bc->item) {
                // once shift is pressed, set bc->extruded
                bc->extruded = true;
            }

            if (!bc->extruded) {
                bc->drag_ptB = motion_dt;
                bc->drag_ptC = motion_dt;

                bc->drag_ptB_proj = cur_persp->perspective_impl->tmat.preimage (motion_dt, 0, Proj::Z);
                bc->drag_ptC_proj = bc->drag_ptB_proj;
                bc->drag_ptC_proj.normalize();
                bc->drag_ptC_proj[Proj::Z] = 0.25;
            } else {
                // Without Ctrl, motion of the extruded corner is constrained to the
                // perspective line from drag_ptB to vanishing point Y.
                if (!bc->ctrl_dragged) {
                    /* snapping */
                    Box3D::PerspectiveLine pline (bc->drag_ptB, Proj::Z, document->getCurrentPersp3D());
                    bc->drag_ptC = pline.closest_to (motion_dt);

                    bc->drag_ptB_proj.normalize();
                    bc->drag_ptC_proj = cur_persp->perspective_impl->tmat.preimage (bc->drag_ptC, bc->drag_ptB_proj[Proj::X], Proj::X);
                } else {
                    bc->drag_ptC = motion_dt;

                    bc->drag_ptB_proj.normalize();
                    bc->drag_ptC_proj = cur_persp->perspective_impl->tmat.preimage (motion_dt, bc->drag_ptB_proj[Proj::X], Proj::X);
                }
                m.freeSnapReturnByRef(bc->drag_ptC, Inkscape::SNAPSOURCE_NODE_HANDLE);
            }
            m.unSetup();

            sp_box3d_drag(*bc, event->motion.state);

            ret = TRUE;
        } else if (!sp_event_context_knot_mouseover(bc)) {
            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop);

            Geom::Point const motion_w(event->motion.x, event->motion.y);
            Geom::Point motion_dt(desktop->w2d(motion_w));
            m.preSnap(Inkscape::SnapCandidatePoint(motion_dt, Inkscape::SNAPSOURCE_NODE_HANDLE));
            m.unSetup();
        }
        break;
    case GDK_BUTTON_RELEASE:
        event_context->xp = event_context->yp = 0;
        if ( event->button.button == 1  && !event_context->space_panning) {
            dragging = false;
            sp_event_context_discard_delayed_snap_event(event_context);

            if (!event_context->within_tolerance) {
                // we've been dragging, finish the box
                sp_box3d_finish(bc);
            } else if (event_context->item_to_select) {
                // no dragging, select clicked item if any
                if (event->button.state & GDK_SHIFT_MASK) {
                    selection->toggle(event_context->item_to_select);
                } else {
                    selection->set(event_context->item_to_select);
                }
            } else {
                // click in an empty space
                selection->clear();
            }

            event_context->item_to_select = NULL;
            ret = TRUE;
            sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate),
                                  event->button.time);
        }
        break;
    case GDK_KEY_PRESS:
        switch (get_group0_keyval (&event->key)) {
        case GDK_KEY_Up:
        case GDK_KEY_Down:
        case GDK_KEY_KP_Up:
        case GDK_KEY_KP_Down:
            // prevent the zoom field from activation
            if (!MOD__CTRL_ONLY)
                ret = TRUE;
            break;

        case GDK_KEY_bracketright:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::X, -180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        case GDK_KEY_bracketleft:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::X, 180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        case GDK_KEY_parenright:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::Y, -180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        case GDK_KEY_parenleft:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::Y, 180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        case GDK_KEY_braceright:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::Z, -180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        case GDK_KEY_braceleft:
            persp3d_rotate_VP (document->getCurrentPersp3D(), Proj::Z, 180/snaps, MOD__ALT);
            DocumentUndo::done(document, SP_VERB_CONTEXT_3DBOX,
                             _("Change perspective (angle of PLs)"));
            ret = true;
            break;

        /* TODO: what is this???
        case GDK_O:
            if (MOD__CTRL && MOD__SHIFT) {
                Box3D::create_canvas_point(persp3d_get_VP(document()->getCurrentPersp3D(), Proj::W).affine(),
                                           6, 0xff00ff00);
            }
            ret = true;
            break;
        */

        case GDK_KEY_g:
        case GDK_KEY_G:
            if (MOD__SHIFT_ONLY) {
                sp_selection_to_guides(desktop);
                ret = true;
            }
            break;

        case GDK_KEY_p:
        case GDK_KEY_P:
            if (MOD__SHIFT_ONLY) {
                if (document->getCurrentPersp3D()) {
                    persp3d_print_debugging_info (document->getCurrentPersp3D());
                }
                ret = true;
            }
            break;

        case GDK_KEY_x:
        case GDK_KEY_X:
            if (MOD__ALT_ONLY) {
                desktop->setToolboxFocusTo ("altx-box3d");
                ret = TRUE;
            }
            if (MOD__SHIFT_ONLY) {
                persp3d_toggle_VPs(selection->perspList(), Proj::X);
                bc->_vpdrag->updateLines(); // FIXME: Shouldn't this be done automatically?
                ret = true;
            }
            break;

        case GDK_KEY_y:
        case GDK_KEY_Y:
            if (MOD__SHIFT_ONLY) {
                persp3d_toggle_VPs(selection->perspList(), Proj::Y);
                bc->_vpdrag->updateLines(); // FIXME: Shouldn't this be done automatically?
                ret = true;
            }
            break;

        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (MOD__SHIFT_ONLY) {
                persp3d_toggle_VPs(selection->perspList(), Proj::Z);
                bc->_vpdrag->updateLines(); // FIXME: Shouldn't this be done automatically?
                ret = true;
            }
            break;

        case GDK_KEY_Escape:
            sp_desktop_selection(desktop)->clear();
            //TODO: make dragging escapable by Esc
            break;

        case GDK_KEY_space:
            if (dragging) {
                sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate),
                                      event->button.time);
                dragging = false;
                sp_event_context_discard_delayed_snap_event(event_context);
                if (!event_context->within_tolerance) {
                    // we've been dragging, finish the box
                    sp_box3d_finish(bc);
                }
                // do not return true, so that space would work switching to selector
            }
            break;
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
        case GDK_KEY_BackSpace:
            ret = event_context->deleteSelectedDrag(MOD__CTRL_ONLY);
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }

    if (!ret) {
//        if (((SPEventContextClass *) sp_box3d_context_parent_class)->root_handler) {
//            ret = ((SPEventContextClass *) sp_box3d_context_parent_class)->root_handler(event_context, event);
//        }
    	ret = SPEventContext::root_handler(event);
    }

    return ret;
}

static void sp_box3d_drag(Box3DContext &bc, guint /*state*/)
{
    SPDesktop *desktop = bc.desktop;

    if (!bc.item) {

        if (Inkscape::have_viable_layer(desktop, bc._message_context) == false) {
            return;
        }

        // Create object
        SPBox3D *box3d = 0;
        box3d = SPBox3D::createBox3D((SPItem *)desktop->currentLayer());

        // Set style
        desktop->applyCurrentOrToolStyle(box3d, "/tools/shapes/3dbox", false);
        
        bc.item = box3d;

        // TODO: Incorporate this in box3d-side.cpp!
        for (int i = 0; i < 6; ++i) {
            Box3DSide *side = Box3DSide::createBox3DSide(box3d);
            
            guint desc = Box3D::int_to_face(i);

            Box3D::Axis plane = (Box3D::Axis) (desc & 0x7);
            plane = (Box3D::is_plane(plane) ? plane : Box3D::orth_plane_or_axis(plane));
            side->dir1 = Box3D::extract_first_axis_direction(plane);
            side->dir2 = Box3D::extract_second_axis_direction(plane);
            side->front_or_rear = (Box3D::FrontOrRear) (desc & 0x8);

            // Set style
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();

            Glib::ustring descr = "/desktop/";
            descr += box3d_side_axes_string(side);
            descr += "/style";
            Glib::ustring cur_style = prefs->getString(descr);    
    
            bool use_current = prefs->getBool("/tools/shapes/3dbox/usecurrent", false);
            if (use_current && !cur_style.empty()) {
                // use last used style 
                side->setAttribute("style", cur_style.data());
				
            } else {
                // use default style 
                GString *pstring = g_string_new("");
                g_string_printf (pstring, "/tools/shapes/3dbox/%s", box3d_side_axes_string(side));
                desktop->applyCurrentOrToolStyle (side, pstring->str, false);
            }

            side->updateRepr(); // calls box3d_side_write() and updates, e.g., the axes string description
        }

        box3d_set_z_orders(SP_BOX3D(bc.item));
        bc.item->updateRepr();

        // TODO: It would be nice to show the VPs during dragging, but since there is no selection
        //       at this point (only after finishing the box), we must do this "manually"
        /* bc._vpdrag->updateDraggers(); */

        desktop->canvas->forceFullRedrawAfterInterruptions(5);
    }

    g_assert(bc.item);

    SPBox3D *box = SP_BOX3D(bc.item);

    box->orig_corner0 = bc.drag_origin_proj;
    box->orig_corner7 = bc.drag_ptC_proj;

    box3d_check_for_swapped_coords(box);

    /* we need to call this from here (instead of from box3d_position_set(), for example)
       because z-order setting must not interfere with display updates during undo/redo */
    box3d_set_z_orders (box);

    box3d_position_set(box);

    // status text
    bc._message_context->setF(Inkscape::NORMAL_MESSAGE, _("<b>3D Box</b>; with <b>Shift</b> to extrude along the Z axis"));
}

static void sp_box3d_finish(Box3DContext *bc)
{
    bc->_message_context->clear();
    bc->ctrl_dragged = false;
    bc->extruded = false;

    if ( bc->item != NULL ) {
        SPDesktop * desktop = SP_EVENT_CONTEXT_DESKTOP(bc);
        SPDocument *doc = sp_desktop_document(desktop);
        if (!doc || !doc->getCurrentPersp3D())
            return;

        SPBox3D *box = SP_BOX3D(bc->item);

        box->orig_corner0 = bc->drag_origin_proj;
        box->orig_corner7 = bc->drag_ptC_proj;

        box->updateRepr();

        box3d_relabel_corners(box);

        desktop->canvas->endForcedFullRedraws();

        sp_desktop_selection(desktop)->set(bc->item);
        DocumentUndo::done(sp_desktop_document(desktop), SP_VERB_CONTEXT_3DBOX,
                         _("Create 3D box"));

        bc->item = NULL;
    }
}

void sp_box3d_context_update_lines(SPEventContext *ec) {
    /*  update perspective lines if we are in the 3D box tool (so that infinite ones are shown correctly) */
    if (SP_IS_BOX3D_CONTEXT (ec)) {
        Box3DContext *bc = SP_BOX3D_CONTEXT (ec);
        bc->_vpdrag->updateLines();
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
