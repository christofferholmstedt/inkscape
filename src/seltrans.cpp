#define __SELTRANS_C__

/*
 * Helper object for transforming selected items
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libnr/nr-matrix-ops.h>
#include <libnr/nr-matrix-translate-ops.h>
#include <libnr/nr-rotate-ops.h>
#include <libnr/nr-scale-ops.h>
#include <libnr/nr-translate-matrix-ops.h>
#include <libnr/nr-translate-ops.h>
#include <gdk/gdkkeysyms.h>
#include "document.h"
#include "sp-namedview.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "knot.h"
#include "snap.h"
#include "selection.h"
#include "select-context.h"
#include "sp-item.h"
#include "sp-item-transform.h"
#include "seltrans-handles.h"
#include "seltrans.h"
#include "selection-chemistry.h"
#include "sp-metrics.h"
#include "verbs.h"
#include <glibmm/i18n.h>
#include "display/sp-ctrlline.h"
#include "prefs-utils.h"
#include "xml/repr.h"

#include "isnan.h" //temp fix.  make sure included last

static void sp_remove_handles(SPKnot *knot[], gint num);

static void sp_sel_trans_handle_grab(SPKnot *knot, guint state, gpointer data);
static void sp_sel_trans_handle_ungrab(SPKnot *knot, guint state, gpointer data);
static void sp_sel_trans_handle_click(SPKnot *knot, guint state, gpointer data);
static void sp_sel_trans_handle_new_event(SPKnot *knot, NR::Point *position, guint32 state, gpointer data);
static gboolean sp_sel_trans_handle_request(SPKnot *knot, NR::Point *p, guint state, gboolean *data);

extern GdkPixbuf *handles[];

static gboolean sp_seltrans_handle_event(SPKnot *knot, GdkEvent *event, gpointer)
{
    switch (event->type) {
        case GDK_MOTION_NOTIFY:
            break;
        case GDK_KEY_PRESS:
            if (get_group0_keyval (&event->key) == GDK_space) {
                /* stamping mode: both mode(show content and outline) operation with knot */
                if (!SP_KNOT_IS_GRABBED(knot)) {
                    return FALSE;
                }
                SPDesktop *desktop = knot->desktop;
                Inkscape::SelTrans *seltrans = SP_SELECT_CONTEXT(desktop->event_context)->_seltrans;
                seltrans->stamp();
                return TRUE;
            }
            break;
        default:
            break;
    }

    return FALSE;
}

Inkscape::SelTrans::SelTrans(SPDesktop *desktop) :
    _desktop(desktop),
    _selcue(desktop),
    _state(STATE_SCALE),
    _show(SHOW_CONTENT),
    _grabbed(false),
    _show_handles(true),
    _bbox(NR::Nothing()),
    _approximate_bbox(NR::Nothing()),
    _chandle(NULL),
    _stamp_cache(NULL),
    _message_context(desktop->messageStack())
{
    gchar const *prefs_bbox = prefs_get_string_attribute("tools.select", "bounding_box");
    _snap_bbox_type = (prefs_bbox != NULL && strcmp(prefs_bbox, "geometric")==0)? SPItem::GEOMETRIC_BBOX : SPItem::APPROXIMATE_BBOX;
    
    g_return_if_fail(desktop != NULL);

    for (int i = 0; i < 8; i++) {
        _shandle[i] = NULL;
        _rhandle[i] = NULL;
    }

    _updateVolatileState();
    _current.set_identity();

    _center_is_set = false; // reread _center from items, or set to bbox midpoint

    _updateHandles();

    _selection = sp_desktop_selection(desktop);

    _norm = sp_canvas_item_new(sp_desktop_controls(desktop),
                               SP_TYPE_CTRL,
                               "anchor", GTK_ANCHOR_CENTER,
                               "mode", SP_CTRL_MODE_COLOR,
                               "shape", SP_CTRL_SHAPE_BITMAP,
                               "size", 13.0,
                               "filled", TRUE,
                               "fill_color", 0x00000000,
                               "stroked", TRUE,
                               "stroke_color", 0x000000a0,
                               "pixbuf", handles[12],
                               NULL);

    _grip = sp_canvas_item_new(sp_desktop_controls(desktop),
                               SP_TYPE_CTRL,
                               "anchor", GTK_ANCHOR_CENTER,
                               "mode", SP_CTRL_MODE_XOR,
                               "shape", SP_CTRL_SHAPE_CROSS,
                               "size", 7.0,
                               "filled", TRUE,
                               "fill_color", 0xffffff7f,
                               "stroked", TRUE,
                               "stroke_color", 0xffffffff,
                               "pixbuf", handles[12],
                               NULL);

    sp_canvas_item_hide(_grip);
    sp_canvas_item_hide(_norm);

    for (int i = 0; i < 4; i++) {
        _l[i] = sp_canvas_item_new(sp_desktop_controls(desktop), SP_TYPE_CTRLLINE, NULL);
        sp_canvas_item_hide(_l[i]);
    }

    _sel_changed_connection = _selection->connectChanged(
        sigc::mem_fun(*this, &Inkscape::SelTrans::_selChanged)
        );

    _sel_modified_connection = _selection->connectModified(
        sigc::mem_fun(*this, &Inkscape::SelTrans::_selModified)
        );
}

Inkscape::SelTrans::~SelTrans()
{
    _sel_changed_connection.disconnect();
    _sel_modified_connection.disconnect();

    for (unsigned int i = 0; i < 8; i++) {
        if (_shandle[i]) {
            g_object_unref(G_OBJECT(_shandle[i]));
            _shandle[i] = NULL;
        }
        if (_rhandle[i]) {
            g_object_unref(G_OBJECT(_rhandle[i]));
            _rhandle[i] = NULL;
        }
    }
    if (_chandle) {
        g_object_unref(G_OBJECT(_chandle));
        _chandle = NULL;
    }

    if (_norm) {
        gtk_object_destroy(GTK_OBJECT(_norm));
        _norm = NULL;
    }
    if (_grip) {
        gtk_object_destroy(GTK_OBJECT(_grip));
        _grip = NULL;
    }
    for (int i = 0; i < 4; i++) {
        if (_l[i]) {
            gtk_object_destroy(GTK_OBJECT(_l[i]));
            _l[i] = NULL;
        }
    }

    for (unsigned i = 0; i < _items.size(); i++) {
        sp_object_unref(SP_OBJECT(_items[i].first), NULL);
    }

    _items.clear();
    _items_centers.clear();
}

void Inkscape::SelTrans::resetState()
{
    _state = STATE_SCALE;
}

void Inkscape::SelTrans::increaseState()
{
    if (_state == STATE_SCALE) {
        _state = STATE_ROTATE;
    } else {
        _state = STATE_SCALE;
    }

    _center_is_set = true; // no need to reread center

    _updateHandles();
}

void Inkscape::SelTrans::setCenter(NR::Point const &p)
{
    _center = p;
    _center_is_set = true;

    // Write the new center position into all selected items
    for (GSList const *l = _desktop->selection->itemList(); l; l = l->next) {
        SPItem *it = (SPItem*)SP_OBJECT(l->data);
        it->setCenter(p);
        // only set the value; updating repr and document_done will be done once, on ungrab
    }

    _updateHandles();
}

void Inkscape::SelTrans::grab(NR::Point const &p, gdouble x, gdouble y, bool show_handles)
{
    Inkscape::Selection *selection = sp_desktop_selection(_desktop);

    g_return_if_fail(!_grabbed);

    _grabbed = true;
    _show_handles = show_handles;
    _updateVolatileState();
    _current.set_identity();

    _changed = false;

    if (_empty) {
        return;
    }

    for (GSList const *l = selection->itemList(); l; l = l->next) {
        SPItem *it = (SPItem*)sp_object_ref(SP_OBJECT(l->data), NULL);
        _items.push_back(std::pair<SPItem *, NR::Matrix>(it, sp_item_i2d_affine(it)));
        _items_centers.push_back(std::pair<SPItem *, NR::Point>(it, it->getCenter())); // for content-dragging, we need to remember original centers
    }

    _current.set_identity();

    _point = p;
    
    // The selector tool should snap the bbox and the special snappoints, but not path nodes
    // (The special points are the handles, center, rotation axis, font baseline, ends of spiral, etc.)

    // First, determine the bounding box for snapping ...
    _bbox = selection->bounds(_snap_bbox_type);    
    _approximate_bbox = selection->bounds(SPItem::APPROXIMATE_BBOX); // Used for correctly scaling the strokewidth


    // Next, get all special points for snapping
    SnapManager const &m = _desktop->namedview->snap_manager;
    _snap_points = selection->getSnapPoints(m.getIncludeItemCenter()); // Excludes path nodes
    std::vector<NR::Point> snap_points_hull = selection->getSnapPointsConvexHull(); // Includes path nodes
    if (_snap_points.size() > 100) {
        /* Snapping a huge number of nodes will take way too long, so limit the number of snappable nodes
        An average user would rarely ever try to snap such a large number of nodes anyway, because 
        (s)he could hardly discern which node would be snapping */
        _snap_points = snap_points_hull;
        // Unfortunately, by now we will have lost the font-baseline snappoints :-(
    }
    
    // Find bbox hulling all special points, which excludes stroke width. Here we need to include the
    // path nodes, for example because a rectangle which has been converted to a path doesn't have 
    // any other special points
    NR::Rect snap_points_bbox;
    if ( snap_points_hull.empty() == false ) {
        std::vector<NR::Point>::iterator i = snap_points_hull.begin();
        snap_points_bbox = NR::Rect(*i, *i);
        i++;
        while (i != snap_points_hull.end()) {
            snap_points_bbox.expandTo(*i);
            i++;
        }
    }
    
    _bbox_points.clear();
    if (_bbox) {
        // ... and add the bbox corners to _bbox_points
        for ( unsigned i = 0 ; i < 4 ; i++ ) {
            _bbox_points.push_back(_bbox->corner(i));
        }
        // There are two separate "opposites" (i.e. opposite w.r.t. the handle being dragged):
        //  - one for snapping the boundingbox, which can be either visual or geometric
        //  - one for snapping the special points
        // The "opposite" in case of a geometric boundingbox always coincides with the "opposite" for the special points
        // These distinct "opposites" are needed in the snapmanager to avoid bugs such as #1540195 (in which 
        // a box is caught between to guides)
        _opposite_for_bboxpoints = _bbox->min() + _bbox->dimensions() * NR::scale(1-x, 1-y);
        _opposite_for_specpoints = (snap_points_bbox.min() + (snap_points_bbox.dimensions() * NR::scale(1-x, 1-y) ) );
        // Only a single "opposite" can be used in calculating transformations.
        _opposite = _opposite_for_bboxpoints;
    }
    
    // The lines below are usefull for debugging any snapping issues, as they'll spit out all points that are considered for snapping
    
    /*std::cout << "Number of snap points:  " << _snap_points.size() << std::endl;
    for (std::vector<NR::Point>::const_iterator i = _snap_points.begin(); i != _snap_points.end(); i++)
    {
        std::cout << "    " << *i << std::endl;
    }
    
    std::cout << "Number of bbox points:  " << _bbox_points.size() << std::endl;
    for (std::vector<NR::Point>::const_iterator i = _bbox_points.begin(); i != _bbox_points.end(); i++)
    {
        std::cout << "    " << *i << std::endl;
    }*/
    
    if ((x != -1) && (y != -1)) {
        sp_canvas_item_show(_norm);
        sp_canvas_item_show(_grip);
    }

    if (_show == SHOW_OUTLINE) {
        for (int i = 0; i < 4; i++)
            sp_canvas_item_show(_l[i]);
    }

    _updateHandles();
    g_return_if_fail(_stamp_cache == NULL);
}

void Inkscape::SelTrans::transform(NR::Matrix const &rel_affine, NR::Point const &norm)
{
    g_return_if_fail(_grabbed);
    g_return_if_fail(!_empty);

    NR::Matrix const affine( NR::translate(-norm) * rel_affine * NR::translate(norm) );

    if (_show == SHOW_CONTENT) {
        // update the content
        for (unsigned i = 0; i < _items.size(); i++) {
            SPItem &item = *_items[i].first;
            NR::Matrix const &prev_transform = _items[i].second;
            sp_item_set_i2d_affine(&item, prev_transform * affine);
        }
    } else {
        if (_bbox) {
            NR::Point p[4];
            /* update the outline */
            for (unsigned i = 0 ; i < 4 ; i++) {
                p[i] = _bbox->corner(i) * affine;
            }
            for (unsigned i = 0 ; i < 4 ; i++) {
                sp_ctrlline_set_coords(SP_CTRLLINE(_l[i]), p[i], p[(i+1)%4]);
            }
        }
    }

    _current = affine;
    _changed = true;
    _updateHandles();
}

void Inkscape::SelTrans::ungrab()
{
    g_return_if_fail(_grabbed);
    _grabbed = false;
    _show_handles = true;

    Inkscape::Selection *selection = sp_desktop_selection(_desktop);
    _updateVolatileState();

    for (unsigned i = 0; i < _items.size(); i++) {
        sp_object_unref(SP_OBJECT(_items[i].first), NULL);
    }

    sp_canvas_item_hide(_norm);
    sp_canvas_item_hide(_grip);

    if (_show == SHOW_OUTLINE) {
        for (int i = 0; i < 4; i++)
            sp_canvas_item_hide(_l[i]);
    }

    if (_stamp_cache) {
        g_slist_free(_stamp_cache);
        _stamp_cache = NULL;
    }

    _message_context.clear();

    if (!_empty && _changed) {
        sp_selection_apply_affine(selection, _current, (_show == SHOW_OUTLINE)? true : false);
        if (_center) {
            *_center *= _current;
            _center_is_set = true;
        }

// If dragging showed content live, sp_selection_apply_affine cannot change the centers
// appropriately - it does not know the original positions of the centers (all objects already have
// the new bboxes). So we need to reset the centers from our saved array.
        if (_show != SHOW_OUTLINE && !_current.is_translation()) {
            for (unsigned i = 0; i < _items_centers.size(); i++) {
                SPItem *currentItem = _items_centers[i].first;
                if (currentItem->isCenterSet()) { // only if it's already set
                    currentItem->setCenter (_items_centers[i].second * _current);
                    SP_OBJECT(currentItem)->updateRepr();
                }
            }
        }

        _items.clear();
        _items_centers.clear();

        if (_current.is_translation()) {
            sp_document_done(sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                             _("Move"));
        } else if (_current.is_scale()) {
            sp_document_done(sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                             _("Scale"));
        } else if (_current.is_rotation()) {
            sp_document_done(sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                             _("Rotate"));
        } else {
            sp_document_done(sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                             _("Skew"));
        }

    } else {

        if (_center_is_set) {
            // we were dragging center; update reprs and commit undoable action
            for (GSList const *l = _desktop->selection->itemList(); l; l = l->next) {
                SPItem *it = (SPItem*)SP_OBJECT(l->data);
                SP_OBJECT(it)->updateRepr();
            }
            sp_document_done (sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                            _("Set center"));
        }

        _items.clear();
        _items_centers.clear();
        _updateHandles();
    }
}

/* fixme: This is really bad, as we compare positions for each stamp (Lauris) */
/* fixme: IMHO the best way to keep sort cache would be to implement timestamping at last */

void Inkscape::SelTrans::stamp()
{
    Inkscape::Selection *selection = sp_desktop_selection(_desktop);

    bool fixup = !_grabbed;
    if ( fixup && _stamp_cache ) {
        // TODO - give a proper fix. Simple temproary work-around for the grab() issue
        g_slist_free(_stamp_cache);
        _stamp_cache = NULL;
    }

    /* stamping mode */
    if (!_empty) {
        GSList *l;
        if (_stamp_cache) {
            l = _stamp_cache;
        } else {
            /* Build cache */
            l  = g_slist_copy((GSList *) selection->itemList());
            l  = g_slist_sort(l, (GCompareFunc) sp_object_compare_position);
            _stamp_cache = l;
        }

        while (l) {
            SPItem *original_item = SP_ITEM(l->data);
            Inkscape::XML::Node *original_repr = SP_OBJECT_REPR(original_item);

            // remember the position of the item
            gint pos = original_repr->position();
            // remember parent
            Inkscape::XML::Node *parent = sp_repr_parent(original_repr);

            Inkscape::XML::Node *copy_repr = original_repr->duplicate(parent->document());

            // add the new repr to the parent
            parent->appendChild(copy_repr);
            // move to the saved position
            copy_repr->setPosition(pos > 0 ? pos : 0);

            SPItem *copy_item = (SPItem *) sp_desktop_document(_desktop)->getObjectByRepr(copy_repr);

            NR::Matrix const *new_affine;
            if (_show == SHOW_OUTLINE) {
                NR::Matrix const i2d(sp_item_i2d_affine(original_item));
                NR::Matrix const i2dnew( i2d * _current );
                sp_item_set_i2d_affine(copy_item, i2dnew);
                new_affine = &copy_item->transform;
            } else {
                new_affine = &original_item->transform;
            }

            sp_item_write_transform(copy_item, copy_repr, *new_affine);

            if ( copy_item->isCenterSet() && _center ) {
                copy_item->setCenter(*_center * _current);
            }

            Inkscape::GC::release(copy_repr);
            l = l->next;
        }
        sp_document_done(sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT,
                         _("Stamp"));
    }

    if ( fixup && _stamp_cache ) {
        // TODO - give a proper fix. Simple temproary work-around for the grab() issue
        g_slist_free(_stamp_cache);
        _stamp_cache = NULL;
    }
}

void Inkscape::SelTrans::_updateHandles()
{
    if ( !_show_handles || _empty )
    {
        sp_remove_handles(_shandle, 8);
        sp_remove_handles(_rhandle, 8);
        sp_remove_handles(&_chandle, 1);
        return;
    }

    // center handle
    if ( _chandle == NULL ) {
        _chandle = sp_knot_new(_desktop, _("<b>Center</b> of rotation and skewing: drag to reposition; scaling with Shift also uses this center"));

        _chandle->setShape (SP_CTRL_SHAPE_BITMAP);
        _chandle->setSize (13);
        _chandle->setAnchor (handle_center.anchor);
        _chandle->setMode (SP_CTRL_MODE_XOR);
        _chandle->setFill(0x00000000, 0x00000000, 0x00000000);
        _chandle->setStroke(0x000000ff, 0xff0000b0, 0xff0000b0);
        _chandle->setPixbuf(handles[handle_center.control]);
        sp_knot_update_ctrl(_chandle);

        g_signal_connect(G_OBJECT(_chandle), "request",
                         G_CALLBACK(sp_sel_trans_handle_request), (gpointer) &handle_center);
        g_signal_connect(G_OBJECT(_chandle), "moved",
                         G_CALLBACK(sp_sel_trans_handle_new_event), (gpointer) &handle_center);
        g_signal_connect(G_OBJECT(_chandle), "grabbed",
                         G_CALLBACK(sp_sel_trans_handle_grab), (gpointer) &handle_center);
        g_signal_connect(G_OBJECT(_chandle), "ungrabbed",
                         G_CALLBACK(sp_sel_trans_handle_ungrab), (gpointer) &handle_center);
        g_signal_connect(G_OBJECT(_chandle), "clicked",
                         G_CALLBACK(sp_sel_trans_handle_click), (gpointer) &handle_center);
    }

    sp_remove_handles(&_chandle, 1);
    if ( _state == STATE_SCALE ) {
        sp_remove_handles(_rhandle, 8);
        _showHandles(_shandle, handles_scale, 8,
                    _("<b>Squeeze or stretch</b> selection; with <b>Ctrl</b> to scale uniformly; with <b>Shift</b> to scale around rotation center"),
                    _("<b>Scale</b> selection; with <b>Ctrl</b> to scale uniformly; with <b>Shift</b> to scale around rotation center"));
    } else {
        sp_remove_handles(_shandle, 8);
        _showHandles(_rhandle, handles_rotate, 8,
                    _("<b>Skew</b> selection; with <b>Ctrl</b> to snap angle; with <b>Shift</b> to skew around the opposite side"),
                    _("<b>Rotate</b> selection; with <b>Ctrl</b> to snap angle; with <b>Shift</b> to rotate around the opposite corner"));
    }

    if (!_center_is_set) {
        _center = _desktop->selection->center();
        _center_is_set = true;
    }

    if ( _state == STATE_SCALE || !_center ) {
        sp_knot_hide(_chandle);
    } else {
        sp_knot_show(_chandle);
        sp_knot_moveto(_chandle, &*_center);
    }
}

void Inkscape::SelTrans::_updateVolatileState()
{
    Inkscape::Selection *selection = sp_desktop_selection(_desktop);
    _empty = selection->isEmpty();

    if (_empty) {
        return;
    }

    //Update the bboxes
    _bbox = selection->bounds(_snap_bbox_type);
    _approximate_bbox = selection->bounds(SPItem::APPROXIMATE_BBOX);
    
    if (!_bbox) {
        _empty = true;
        return;
    }

    _strokewidth = stroke_average_width (selection->itemList());
}

static void sp_remove_handles(SPKnot *knot[], gint num)
{
    for (int i = 0; i < num; i++) {
        if (knot[i] != NULL) {
            sp_knot_hide(knot[i]);
        }
    }
}

void Inkscape::SelTrans::_showHandles(SPKnot *knot[], SPSelTransHandle const handle[], gint num,
                             gchar const *even_tip, gchar const *odd_tip)
{
    g_return_if_fail( !_empty );

    for (int i = 0; i < num; i++) {
        if (knot[i] == NULL) {
            knot[i] = sp_knot_new(_desktop, i % 2 ? even_tip : odd_tip);

            knot[i]->setShape (SP_CTRL_SHAPE_BITMAP);
            knot[i]->setSize (13);
            knot[i]->setAnchor (handle[i].anchor);
            knot[i]->setMode (SP_CTRL_MODE_XOR);
            knot[i]->setFill(0x000000ff, 0x00ff6600, 0x00ff6600); // inversion, green, green
            knot[i]->setStroke(0x000000ff, 0x000000ff, 0x000000ff); // inversion
            knot[i]->setPixbuf(handles[handle[i].control]);
            sp_knot_update_ctrl(knot[i]);

            g_signal_connect(G_OBJECT(knot[i]), "request",
                             G_CALLBACK(sp_sel_trans_handle_request), (gpointer) &handle[i]);
            g_signal_connect(G_OBJECT(knot[i]), "moved",
                             G_CALLBACK(sp_sel_trans_handle_new_event), (gpointer) &handle[i]);
            g_signal_connect(G_OBJECT(knot[i]), "grabbed",
                             G_CALLBACK(sp_sel_trans_handle_grab), (gpointer) &handle[i]);
            g_signal_connect(G_OBJECT(knot[i]), "ungrabbed",
                             G_CALLBACK(sp_sel_trans_handle_ungrab), (gpointer) &handle[i]);
            g_signal_connect(G_OBJECT(knot[i]), "event", G_CALLBACK(sp_seltrans_handle_event), (gpointer) &handle[i]);
        }
        sp_knot_show(knot[i]);

        NR::Point const handle_pt(handle[i].x, handle[i].y);
        // shouldn't have nullary bbox, but knots
        g_assert(_bbox);
        NR::Point p( _bbox->min()
                     + ( _bbox->dimensions()
                         * NR::scale(handle_pt) ) );

        sp_knot_moveto(knot[i], &p);
    }
}

static void sp_sel_trans_handle_grab(SPKnot *knot, guint state, gpointer data)
{
    SP_SELECT_CONTEXT(knot->desktop->event_context)->_seltrans->handleGrab(
        knot, state, *(SPSelTransHandle const *) data
        );
}

static void sp_sel_trans_handle_ungrab(SPKnot *knot, guint state, gpointer data)
{
    SP_SELECT_CONTEXT(knot->desktop->event_context)->_seltrans->ungrab();
}

static void sp_sel_trans_handle_new_event(SPKnot *knot, NR::Point *position, guint state, gpointer data)
{
    SP_SELECT_CONTEXT(knot->desktop->event_context)->_seltrans->handleNewEvent(
        knot, position, state, *(SPSelTransHandle const *) data
        );
}

static gboolean sp_sel_trans_handle_request(SPKnot *knot, NR::Point *position, guint state, gboolean *data)
{
    return SP_SELECT_CONTEXT(knot->desktop->event_context)->_seltrans->handleRequest(
        knot, position, state, *(SPSelTransHandle const *) data
        );
}

static void sp_sel_trans_handle_click(SPKnot *knot, guint state, gpointer data)
{
    SP_SELECT_CONTEXT(knot->desktop->event_context)->_seltrans->handleClick(
        knot, state, *(SPSelTransHandle const *) data
        );
}

void Inkscape::SelTrans::handleClick(SPKnot *knot, guint state, SPSelTransHandle const &handle)
{
    switch (handle.anchor) {
        case GTK_ANCHOR_CENTER:
            if (state & GDK_SHIFT_MASK) {
                // Unset the  center position for all selected items
                for (GSList const *l = _desktop->selection->itemList(); l; l = l->next) {
                    SPItem *it = (SPItem*)(SP_OBJECT(l->data));
                    it->unsetCenter();
                    SP_OBJECT(it)->updateRepr();
                    _center_is_set = false;  // center has changed
                    _updateHandles();
                }
                sp_document_done (sp_desktop_document(_desktop), SP_VERB_CONTEXT_SELECT, 
                                        _("Reset center"));
            }
            break;
        default:
            break;
    }
}

void Inkscape::SelTrans::handleGrab(SPKnot *knot, guint state, SPSelTransHandle const &handle)
{
    switch (handle.anchor) {
        case GTK_ANCHOR_CENTER:
            g_object_set(G_OBJECT(_grip),
                         "shape", SP_CTRL_SHAPE_BITMAP,
                         "size", 13.0,
                         NULL);
            sp_canvas_item_show(_grip);
            break;
        default:
            g_object_set(G_OBJECT(_grip),
                         "shape", SP_CTRL_SHAPE_CROSS,
                         "size", 7.0,
                         NULL);
            sp_canvas_item_show(_norm);
            sp_canvas_item_show(_grip);

            break;
    }

    grab(sp_knot_position(knot), handle.x, handle.y, FALSE);
}


void Inkscape::SelTrans::handleNewEvent(SPKnot *knot, NR::Point *position, guint state, SPSelTransHandle const &handle)
{
    if (!SP_KNOT_IS_GRABBED(knot)) {
        return;
    }

    // in case items have been unhooked from the document, don't
    // try to continue processing events for them.
    for (unsigned int i = 0; i < _items.size(); i++) {
        if (!SP_OBJECT_DOCUMENT(SP_OBJECT(_items[i].first)) ) {
            return;
        }
    }

    handle.action(this, handle, *position, state);
}


gboolean Inkscape::SelTrans::handleRequest(SPKnot *knot, NR::Point *position, guint state, SPSelTransHandle const &handle)
{
    if (!SP_KNOT_IS_GRABBED(knot)) {
        return TRUE;
    }

    knot->desktop->setPosition(*position);

    if ((!(state & GDK_SHIFT_MASK) == !(_state == STATE_ROTATE)) && (&handle != &handle_center)) {
        _origin = _opposite;
        _origin_for_bboxpoints = _opposite_for_bboxpoints;
        _origin_for_specpoints = _opposite_for_specpoints;
    } else if (_center) {
        _origin = *_center;
        _origin_for_bboxpoints = *_center;
        _origin_for_specpoints = *_center;
    } else {
        // FIXME
        return TRUE;
    }
    if (handle.request(this, handle, *position, state)) {
        sp_knot_set_position(knot, position, state);
        SP_CTRL(_grip)->moveto(*position);
        SP_CTRL(_norm)->moveto(_origin);
    }

    return TRUE;
}


void Inkscape::SelTrans::_selChanged(Inkscape::Selection *selection)
{
    if (!_grabbed) {
        // reread in case it changed on the fly:
        gchar const *prefs_bbox = prefs_get_string_attribute("tools.select", "bounding_box");
        _snap_bbox_type = (prefs_bbox != NULL && strcmp(prefs_bbox, "geometric")==0)? SPItem::GEOMETRIC_BBOX : SPItem::APPROXIMATE_BBOX;
        //SPItem::APPROXIMATE_BBOX will be replaced by SPItem::VISUAL_BBOX, as soon as the latter is implemented properly
    
        _updateVolatileState();
        _current.set_identity();
        _center_is_set = false; // center(s) may have changed
        _updateHandles();
    }
}

void Inkscape::SelTrans::_selModified(Inkscape::Selection *selection, guint flags)
{
    if (!_grabbed) {
        _updateVolatileState();
        _current.set_identity();

        // reset internal flag
        _changed = false;

        _center_is_set = false;  // center(s) may have changed

        _updateHandles();
    }
}

/*
 * handlers for handle move-request
 */

/** Returns -1 or 1 according to the sign of x.  Returns 1 for 0 and NaN. */
static double sign(double const x)
{
    return ( x < 0
             ? -1
             : 1 );
}

gboolean sp_sel_trans_scale_request(Inkscape::SelTrans *seltrans,
                                    SPSelTransHandle const &, NR::Point &pt, guint state)
{
    return seltrans->scaleRequest(pt, state);
}

gboolean sp_sel_trans_stretch_request(Inkscape::SelTrans *seltrans,
                                      SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    return seltrans->stretchRequest(handle, pt, state);
}

gboolean sp_sel_trans_skew_request(Inkscape::SelTrans *seltrans,
                                   SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    return seltrans->skewRequest(handle, pt, state);
}

gboolean sp_sel_trans_rotate_request(Inkscape::SelTrans *seltrans,
                                     SPSelTransHandle const &, NR::Point &pt, guint state)
{
    return seltrans->rotateRequest(pt, state);
}

gboolean sp_sel_trans_center_request(Inkscape::SelTrans *seltrans,
                                     SPSelTransHandle const &, NR::Point &pt, guint state)
{
    return seltrans->centerRequest(pt, state);
}

gboolean Inkscape::SelTrans::scaleRequest(NR::Point &pt, guint state)
{
    using NR::X;
    using NR::Y;

    NR::Point d = _point - _origin;
    NR::scale s(0, 0);

    /* Work out the new scale factors `s' */
    for ( unsigned int i = 0 ; i < 2 ; i++ ) {
        if ( fabs(d[i]) > 0.001 ) {
            s[i] = ( pt[i] - _origin[i] ) / d[i];
            if ( fabs(s[i]) < 1e-9 ) {
                s[i] = 1e-9;
            }
        }
    }

    if (state & GDK_MOD1_MASK) { // scale by an integer multiplier/divider
        for ( unsigned int i = 0 ; i < 2 ; i++ ) {
            if (fabs(s[i]) > 1) 
                s[i] = round(s[i]);
            else 
                s[i] = 1/round(1/(MIN(s[i], 10)));
        }
    }

    SnapManager const &m = _desktop->namedview->snap_manager;

    /* Get a STL list of the selected items.
    ** FIXME: this should probably be done by Inkscape::Selection.
    */
    std::list<SPItem const*> it;
    for (GSList const *i = _selection->itemList(); i != NULL; i = i->next) {
        it.push_back(reinterpret_cast<SPItem*>(i->data));
    }

    if ((state & GDK_CONTROL_MASK) || _desktop->isToolboxButtonActive ("lock")) {
        // Scale is locked to a 1:1 aspect ratio, so that s[X] must be made to equal s[Y].
        //
        // The aspect-ratio must be locked before snapping
        if (fabs(s[NR::X]) > fabs(s[NR::Y])) {
                s[NR::X] = fabs(s[NR::Y]) * sign(s[NR::X]);
            } else {
                s[NR::Y] = fabs(s[NR::X]) * sign(s[NR::Y]);
            }
            
        // Snap along a suitable constraint vector from the origin.

        // The inclination of the constraint vector is calculated from the aspect ratio
        NR::Point bbox_dim = _bbox->dimensions();
        double const aspect_ratio = bbox_dim[1] / bbox_dim[0]; // = height / width

        // Determine direction of the constraint vector
        NR::Point const cv = NR::Point(
            pt[NR::X] > _origin[NR::X] ? 1 : -1,
            pt[NR::Y] > _origin[NR::Y] ? aspect_ratio : -aspect_ratio
            );

        std::pair<NR::scale, bool> bb = m.constrainedSnapScale(Snapper::SNAPPOINT_BBOX,
                                                               _bbox_points,
                                                               it,
                                                               Snapper::ConstraintLine(_origin_for_bboxpoints, cv),
                                                               s,
                                                               _origin_for_bboxpoints);

        std::pair<NR::scale, bool> sn = m.constrainedSnapScale(Snapper::SNAPPOINT_NODE,
                                                               _snap_points,
                                                               it,
                                                               Snapper::ConstraintLine(_origin_for_specpoints, cv),
                                                               s,
                                                               _origin_for_specpoints);

        if (bb.second == false && sn.second == false) {
            /* We didn't snap, so just keep the locked aspect ratio */
        } else {
            /* Choose the smaller difference in scale.  Since s[X] == s[Y] we can
            ** just compare difference in s[X].
            */
            double const bd = bb.second ? fabs(bb.first[NR::X] - s[NR::X]) : NR_HUGE;
            double const sd = sn.second ? fabs(sn.first[NR::X] - s[NR::X]) : NR_HUGE;
            s = (bd < sd) ? bb.first : sn.first;
        }

    } else {
        /* Scale aspect ratio is unlocked */
        
        std::pair<NR::scale, bool> bb = m.freeSnapScale(Snapper::SNAPPOINT_BBOX,
                                                        _bbox_points,
                                                        it,
                                                        s,
                                                        _origin_for_bboxpoints);
        std::pair<NR::scale, bool> sn = m.freeSnapScale(Snapper::SNAPPOINT_NODE,
                                                        _snap_points,
                                                        it,
                                                        s,
                                                        _origin_for_specpoints);

	/* Pick the snap that puts us closest to the original scale */
        NR::Coord bd = bb.second ?
            fabs(NR::L2(NR::Point(bb.first[NR::X], bb.first[NR::Y])) -
                 NR::L2(NR::Point(s[NR::X], s[NR::Y])))
            : NR_HUGE;
        NR::Coord sd = sn.second ?
            fabs(NR::L2(NR::Point(sn.first[NR::X], sn.first[NR::Y])) -
                 NR::L2(NR::Point(s[NR::X], s[NR::Y])))
            : NR_HUGE;
        s = (bd < sd) ? bb.first : sn.first;
    }

    /* Update the knot position */
    pt = ( _point - _origin ) * s + _origin;

    /* Status text */
    _message_context.setF(Inkscape::NORMAL_MESSAGE,
                          _("<b>Scale</b>: %0.2f%% x %0.2f%%; with <b>Ctrl</b> to lock ratio"),
                          100 * s[NR::X], 100 * s[NR::Y]);

    return TRUE;
}

gboolean Inkscape::SelTrans::stretchRequest(SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    using NR::X;
    using NR::Y;

    NR::Dim2 axis, perp;

    switch (handle.cursor) {
        case GDK_TOP_SIDE:
        case GDK_BOTTOM_SIDE:
           axis = NR::Y;
           perp = NR::X;
           break;
        case GDK_LEFT_SIDE:
        case GDK_RIGHT_SIDE:
           axis = NR::X;
           perp = NR::Y;
           break;
        default:
            g_assert_not_reached();
            return TRUE;
    };

    if ( fabs( _point[axis] - _origin[axis] ) < 1e-15 ) {
        return FALSE;
    }

    NR::scale s(1, 1);
    s[axis] = ( ( pt[axis] - _origin[axis] )
                / ( _point[axis] - _origin[axis] ) );
    if ( fabs(s[axis]) < 1e-15 ) {
        s[axis] = 1e-15;
    }

    if (state & GDK_MOD1_MASK) { // scale by an integer multiplier/divider
        if (fabs(s[axis]) > 1) 
            s[axis] = round(s[axis]);
        else 
            s[axis] = 1/round(1/(MIN(s[axis], 10)));
    }

    /* Get a STL list of the selected items.
    ** FIXME: this should probably be done by Inkscape::Selection.
    */
    std::list<SPItem const*> it;
    for (GSList const *i = _selection->itemList(); i != NULL; i = i->next) {
        it.push_back(reinterpret_cast<SPItem*>(i->data));
    }

    SnapManager const &m = _desktop->namedview->snap_manager;

    if ( state & GDK_CONTROL_MASK ) {
        // on ctrl, apply symmetrical scaling instead of stretching
        s[perp] = fabs(s[axis]);

        std::pair<NR::Coord, bool> const bb = m.freeSnapStretch(
            Snapper::SNAPPOINT_BBOX,
            _bbox_points,
            it,
            s[axis],
            _origin_for_bboxpoints,
            axis,
            true);

        std::pair<NR::Coord, bool> const sn = m.freeSnapStretch(
            Snapper::SNAPPOINT_NODE,
            _snap_points,
            it,
            s[axis],
            _origin_for_specpoints,
            axis,
            true);

        NR::Coord const bd = bb.second ? fabs(bb.first - s[axis]) : NR_HUGE;
        NR::Coord const sd = sn.second ? fabs(sn.first - s[axis]) : NR_HUGE;
        NR::Coord const ratio = (bd < sd) ? bb.first : sn.first;

        s[axis] = fabs(ratio) * sign(s[axis]);
        s[perp] = fabs(s[axis]);
    } else {
        
        std::pair<NR::Coord, bool> const bb = m.freeSnapStretch(
            Snapper::SNAPPOINT_BBOX,
            _bbox_points,
            it,
            s[axis],
            _origin_for_bboxpoints,
            axis,
            false);

        std::pair<NR::Coord, bool> const sn = m.freeSnapStretch(
            Snapper::SNAPPOINT_NODE,
            _snap_points,
            it,
            s[axis],
            _origin_for_specpoints,
            axis,
            false);

        /* Choose the smaller difference in scale */
        NR::Coord const bd = bb.second ? fabs(bb.first - s[axis]) : NR_HUGE;
        NR::Coord const sd = sn.second ? fabs(sn.first - s[axis]) : NR_HUGE;
        s[axis] = (bd < sd) ? bb.first : sn.first;
        s[perp] = 1;
    }

    pt = ( _point - _origin ) * NR::scale(s) + _origin;
    if (isNaN(pt[X] + pt[Y])) {
        g_warning("point=(%g, %g), norm=(%g, %g), s=(%g, %g)\n",
                  _point[X], _point[Y], _origin[X], _origin[Y], s[X], s[Y]);
    }

    // status text
    _message_context.setF(Inkscape::NORMAL_MESSAGE,
                          _("<b>Scale</b>: %0.2f%% x %0.2f%%; with <b>Ctrl</b> to lock ratio"),
                          100 * s[NR::X], 100 * s[NR::Y]);

    return TRUE;
}

gboolean Inkscape::SelTrans::skewRequest(SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    using NR::X;
    using NR::Y;

    if (handle.cursor != GDK_SB_V_DOUBLE_ARROW && handle.cursor != GDK_SB_H_DOUBLE_ARROW) {
        return FALSE;
    }

    NR::Dim2 dim_a;
    NR::Dim2 dim_b;
    if (handle.cursor == GDK_SB_V_DOUBLE_ARROW) {
        dim_a = X;
        dim_b = Y;
    } else {
        dim_a = Y;
        dim_b = X;
    }

    double skew[2];
    double s[2] = { 1.0, 1.0 };

    if (fabs(_point[dim_a] - _origin[dim_a]) < NR_EPSILON) {
        return FALSE;
    }

    skew[dim_a] = ( pt[dim_b] - _point[dim_b] ) / ( _point[dim_a] - _origin[dim_a] );

    s[dim_a] = ( pt[dim_a] - _origin[dim_a] ) / ( _point[dim_a] - _origin[dim_a] );

    if ( fabs(s[dim_a]) < 1 ) {
        s[dim_a] = sign(s[dim_a]);
    } else {
        s[dim_a] = floor( s[dim_a] + 0.5 );
    }

    double radians = atan(skew[dim_a] / s[dim_a]);

    if (state & GDK_CONTROL_MASK) {

        int snaps = prefs_get_int_attribute("options.rotationsnapsperpi", "value", 12);

        if (snaps) {
            double sections = floor( radians * snaps / M_PI + .5 );
            if (fabs(sections) >= snaps / 2) sections = sign(sections) * (snaps / 2 - 1);
            radians = ( M_PI / snaps ) * sections;
        }
        skew[dim_a] = tan(radians) * s[dim_a];
    } else {
        SnapManager const &m = _desktop->namedview->snap_manager;

        std::pair<NR::Coord, bool> bb = m.freeSnapSkew(Inkscape::Snapper::SNAPPOINT_BBOX,
                                                       _bbox_points,
                                                       std::list<SPItem const *>(),
                                                       skew[dim_a],
                                                       _origin_for_bboxpoints,
                                                       dim_b);

        std::pair<NR::Coord, bool> sn = m.freeSnapSkew(Inkscape::Snapper::SNAPPOINT_NODE,
                                                       _snap_points,
                                                       std::list<SPItem const *>(),
                                                       skew[dim_a],
                                                       _origin_for_specpoints,
                                                       dim_b);
        
        if (bb.second || sn.second) {
            /* We snapped something, so change the skew to reflect it */
            NR::Coord const bd = bb.second ? bb.first : NR_HUGE;
            NR::Coord const sd = sn.second ? sn.first : NR_HUGE;
            skew[dim_a] = std::min(bd, sd);
        }
    }

    pt[dim_b] = ( _point[dim_a] - _origin[dim_a] ) * skew[dim_a] + _point[dim_b];
    pt[dim_a] = ( _point[dim_a] - _origin[dim_a] ) * s[dim_a] + _origin[dim_a];

    /* status text */
    double degrees = 180 / M_PI * radians;
    if (degrees > 180) degrees -= 360;
    if (degrees < -180) degrees += 360;

    _message_context.setF(Inkscape::NORMAL_MESSAGE,
                          // TRANSLATORS: don't modify the first ";"
                          // (it will NOT be displayed as ";" - only the second one will be)
                          _("<b>Skew</b>: %0.2f&#176;; with <b>Ctrl</b> to snap angle"),
                          degrees);

    return TRUE;
}

gboolean Inkscape::SelTrans::rotateRequest(NR::Point &pt, guint state)
{
    int snaps = prefs_get_int_attribute("options.rotationsnapsperpi", "value", 12);

    // rotate affine in rotate
    NR::Point const d1 = _point - _origin;
    NR::Point const d2 = pt     - _origin;

    NR::Coord const h1 = NR::L2(d1);
    if (h1 < 1e-15) return FALSE;
    NR::Point q1 = d1 / h1;
    NR::Coord const h2 = NR::L2(d2);
    if (fabs(h2) < 1e-15) return FALSE;
    NR::Point q2 = d2 / h2;

    double radians;
    if (state & GDK_CONTROL_MASK) {
        /* Have to restrict movement. */
        double cos_t = NR::dot(q1, q2);
        double sin_t = NR::dot(NR::rot90(q1), q2);
        radians = atan2(sin_t, cos_t);
        if (snaps) {
            radians = ( M_PI / snaps ) * floor( radians * snaps / M_PI + .5 );
        }
        q1 = NR::Point(1, 0);
        q2 = NR::Point(cos(radians), sin(radians));
    } else {
        radians = atan2(NR::dot(NR::rot90(d1), d2),
                        NR::dot(d1, d2));
    }

    NR::rotate const r1(q1);
    NR::rotate const r2(q2);
    pt = _point * NR::translate(-_origin) * ( r2 / r1 ) * NR::translate(_origin);

    /* status text */
    double degrees = 180 / M_PI * radians;
    if (degrees > 180) degrees -= 360;
    if (degrees < -180) degrees += 360;

    _message_context.setF(Inkscape::NORMAL_MESSAGE,
                          // TRANSLATORS: don't modify the first ";"
                          // (it will NOT be displayed as ";" - only the second one will be)
                          _("<b>Rotate</b>: %0.2f&#176;; with <b>Ctrl</b> to snap angle"), degrees);

    return TRUE;
}

gboolean Inkscape::SelTrans::centerRequest(NR::Point &pt, guint state)
{
    using NR::X;
    using NR::Y;

    SnapManager const &m = _desktop->namedview->snap_manager;
    pt = m.freeSnap(Snapper::SNAPPOINT_NODE, pt, NULL).getPoint();

    if (state & GDK_CONTROL_MASK) {
        if ( fabs(_point[X] - pt[X]) > fabs(_point[Y] - pt[Y]) ) {
            pt[Y] = _point[Y];
        } else {
            pt[X] = _point[X];
        }
    }

    if ( !(state & GDK_SHIFT_MASK) && _bbox ) {
        // screen pixels to snap center to bbox
#define SNAP_DIST 5
        // FIXME: take from prefs
        double snap_dist = SNAP_DIST / _desktop->current_zoom();

        for (int i = 0; i < 2; i++) {
            if (fabs(pt[i] - _bbox->min()[i]) < snap_dist) {
                pt[i] = _bbox->min()[i];
            }
            if (fabs(pt[i] - _bbox->midpoint()[i]) < snap_dist) {
                pt[i] = _bbox->midpoint()[i];
            }
            if (fabs(pt[i] - _bbox->max()[i]) < snap_dist) {
                pt[i] = _bbox->max()[i];
            }
        }
    }

    // status text
    GString *xs = SP_PX_TO_METRIC_STRING(pt[X], _desktop->namedview->getDefaultMetric());
    GString *ys = SP_PX_TO_METRIC_STRING(pt[Y], _desktop->namedview->getDefaultMetric());
    _message_context.setF(Inkscape::NORMAL_MESSAGE, _("Move <b>center</b> to %s, %s"), xs->str, ys->str);
    g_string_free(xs, FALSE);
    g_string_free(ys, FALSE);

    return TRUE;
}

/*
 * handlers for handle movement
 *
 */

void sp_sel_trans_stretch(Inkscape::SelTrans *seltrans, SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    seltrans->stretch(handle, pt, state);
}

void sp_sel_trans_scale(Inkscape::SelTrans *seltrans, SPSelTransHandle const &, NR::Point &pt, guint state)
{
    seltrans->scale(pt, state);
}

void sp_sel_trans_skew(Inkscape::SelTrans *seltrans, SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    seltrans->skew(handle, pt, state);
}

void sp_sel_trans_rotate(Inkscape::SelTrans *seltrans, SPSelTransHandle const &, NR::Point &pt, guint state)
{
    seltrans->rotate(pt, state);
}

void Inkscape::SelTrans::stretch(SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    using NR::X;
    using NR::Y;

    NR::Dim2 dim;
    switch (handle.cursor) {
        case GDK_LEFT_SIDE:
        case GDK_RIGHT_SIDE:
            dim = X;
            break;
        case GDK_TOP_SIDE:
        case GDK_BOTTOM_SIDE:
            dim = Y;
            break;
        default:
            g_assert_not_reached();
            abort();
            break;
    }

    NR::Point const scale_origin(_origin);
    double const offset = _point[dim] - scale_origin[dim];
    if (!( fabs(offset) >= 1e-15 )) {
        return;
    }
    NR::scale s(1, 1);
    s[dim] = ( pt[dim] - scale_origin[dim] ) / offset;
    if (isNaN(s[dim])) {
        g_warning("s[dim]=%g, pt[dim]=%g, scale_origin[dim]=%g, point[dim]=%g\n",
                  s[dim], pt[dim], scale_origin[dim], _point[dim]);
    }
    if (!( fabs(s[dim]) >= 1e-15 )) {
        s[dim] = 1e-15;
    }
    if (state & GDK_CONTROL_MASK) {
        /* Preserve aspect ratio, but never flip in the dimension not being edited. */
        s[!dim] = fabs(s[dim]);
    }

    if (!_bbox) {
        return;
    }

    NR::Point new_bbox_min = _approximate_bbox->min() * (NR::translate(-scale_origin) * NR::Matrix(s) * NR::translate(scale_origin));
    NR::Point new_bbox_max = _approximate_bbox->max() * (NR::translate(-scale_origin) * NR::Matrix(s) * NR::translate(scale_origin));

    int transform_stroke = false;
    gdouble strokewidth = 0;

    if ( _snap_bbox_type != SPItem::GEOMETRIC_BBOX) {
        transform_stroke = prefs_get_int_attribute ("options.transform", "stroke", 1);
        strokewidth = _strokewidth;
    }

    NR::Matrix scaler = get_scale_transform_with_stroke (*_approximate_bbox, strokewidth, transform_stroke,
                    new_bbox_min[NR::X], new_bbox_min[NR::Y], new_bbox_max[NR::X], new_bbox_max[NR::Y]);

    transform(scaler, NR::Point(0, 0)); // we have already accounted for origin, so pass 0,0
}

void Inkscape::SelTrans::scale(NR::Point &pt, guint state)
{
    if (!_bbox) {
        return;
    }

    NR::Point const offset = _point - _origin;

    NR::scale s (1, 1);
    for (int i = NR::X; i <= NR::Y; i++) {
        if (fabs(offset[i]) > 1e-9)
            s[i] = (pt[i] - _origin[i]) / offset[i];
        if (fabs(s[i]) < 1e-9)
            s[i] = 1e-9;
    }
    
    NR::Point new_bbox_min = _approximate_bbox->min() * (NR::translate(-_origin) * NR::Matrix(s) * NR::translate(_origin));
    NR::Point new_bbox_max = _approximate_bbox->max() * (NR::translate(-_origin) * NR::Matrix(s) * NR::translate(_origin));
    
    int transform_stroke = false;
    gdouble strokewidth = 0;

    if ( _snap_bbox_type != SPItem::GEOMETRIC_BBOX) {
        transform_stroke = prefs_get_int_attribute ("options.transform", "stroke", 1);
        strokewidth = _strokewidth;
    }
        
    NR::Matrix scaler = get_scale_transform_with_stroke (*_approximate_bbox, strokewidth, transform_stroke,
                    new_bbox_min[NR::X], new_bbox_min[NR::Y], new_bbox_max[NR::X], new_bbox_max[NR::Y]);
    
    transform(scaler, NR::Point(0, 0)); // we have already accounted for origin, so pass 0,0
}

void Inkscape::SelTrans::skew(SPSelTransHandle const &handle, NR::Point &pt, guint state)
{
    NR::Point const offset = _point - _origin;

    unsigned dim;
    switch (handle.cursor) {
        case GDK_SB_H_DOUBLE_ARROW:
            dim = NR::Y;
            break;
        case GDK_SB_V_DOUBLE_ARROW:
            dim = NR::X;
            break;
        default:
            g_assert_not_reached();
            abort();
            break;
    }
    if (fabs(offset[dim]) < 1e-15) {
        return;
    }
    NR::Matrix skew = NR::identity();
    skew[2*dim + dim] = (pt[dim] - _origin[dim]) / offset[dim];
    skew[2*dim + (1-dim)] = (pt[1-dim] - _point[1-dim]) / offset[dim];
    skew[2*(1-dim) + (dim)] = 0;
    skew[2*(1-dim) + (1-dim)] = 1;

    for (int i = 0; i < 2; i++) {
        if (fabs(skew[3*i]) < 1e-15) {
            skew[3*i] = 1e-15;
        }
    }
    transform(skew, _origin);
}

void Inkscape::SelTrans::rotate(NR::Point &pt, guint state)
{
    NR::Point const offset = _point - _origin;

    NR::Coord const h1 = NR::L2(offset);
    if (h1 < 1e-15) {
        return;
    }
    NR::Point const q1 = offset / h1;
    NR::Coord const h2 = NR::L2( pt - _origin );
    if (h2 < 1e-15) {
        return;
    }
    NR::Point const q2 = (pt - _origin) / h2;
    NR::rotate const r1(q1);
    NR::rotate const r2(q2);

    NR::Matrix rotate( r2 / r1 );
    transform(rotate, _origin);
}

void sp_sel_trans_center(Inkscape::SelTrans *seltrans, SPSelTransHandle const &, NR::Point &pt, guint state)
{
    seltrans->setCenter(pt);
}


void Inkscape::SelTrans::moveTo(NR::Point const &xy, guint state)
{
    SnapManager const &m = _desktop->namedview->snap_manager;

    /* The amount that we've moved by during this drag */
    NR::Point dxy = xy - _point;

    /* Get a STL list of the selected items.
    ** FIXME: this should probably be done by Inkscape::Selection.
    */
    std::list<SPItem const*> it;
    for (GSList const *i = _selection->itemList(); i != NULL; i = i->next) {
        it.push_back(reinterpret_cast<SPItem*>(i->data));
    }

    bool const alt = (state & GDK_MOD1_MASK);
    bool const control = (state & GDK_CONTROL_MASK);
    bool const shift = (state & GDK_SHIFT_MASK);

    if (alt) {

        /* Alt pressed means keep offset: snap the moved distance to the grid.
        ** FIXME: this will snap to more than just the grid, nowadays.
        */

        dxy = m.freeSnap(Snapper::SNAPPOINT_NODE, dxy, NULL).getPoint();

    } else if (!shift) {

        /* We're snapping to things, possibly with a constraint to horizontal or
        ** vertical movement.  Obtain a list of possible translations and then
        ** pick the smallest.
        */

        /* This will be our list of possible translations */
        std::list<std::pair<NR::Point, bool> > s;

        if (control) {

            /* Snap to things, and also constrain to horizontal or vertical movement */

            for (unsigned int dim = 0; dim < 2; dim++) {
                s.push_back(m.constrainedSnapTranslation(Inkscape::Snapper::SNAPPOINT_BBOX,
                                                         _bbox_points,
                                                         it,
                                                         Inkscape::Snapper::ConstraintLine(component_vectors[dim]),
                                                         dxy));
                            
                s.push_back(m.constrainedSnapTranslation(Inkscape::Snapper::SNAPPOINT_NODE,
                                                         _snap_points,
                                                         it,
                                                         Inkscape::Snapper::ConstraintLine(component_vectors[dim]),
                                                         dxy));
            }

        } else {

            /* Snap to things with no constraint */

            s.push_back(m.freeSnapTranslation(Inkscape::Snapper::SNAPPOINT_BBOX,
                                              _bbox_points, it, dxy));
            s.push_back(m.freeSnapTranslation(Inkscape::Snapper::SNAPPOINT_NODE,
                                              _snap_points, it, dxy));
        }

        /* Pick one */
        NR::Coord best = NR_HUGE;
        for (std::list<std::pair<NR::Point, bool> >::const_iterator i = s.begin(); i != s.end(); i++) {
            if (i->second) {
                NR::Coord const m = NR::L2(i->first);
                if (m < best) {
                    best = m;
                    dxy = i->first;
                }
            }
        }
    }

    if (control) {
        /* Ensure that the horizontal and vertical constraint has been applied */
        if (fabs(dxy[NR::X]) > fabs(dxy[NR::Y])) {
            dxy[NR::Y] = 0;
        } else {
            dxy[NR::X] = 0;
        }
    }

    NR::Matrix const move((NR::translate(dxy)));
    NR::Point const norm(0, 0);
    transform(move, norm);

    // status text
    GString *xs = SP_PX_TO_METRIC_STRING(dxy[NR::X], _desktop->namedview->getDefaultMetric());
    GString *ys = SP_PX_TO_METRIC_STRING(dxy[NR::Y], _desktop->namedview->getDefaultMetric());
    _message_context.setF(Inkscape::NORMAL_MESSAGE, _("<b>Move</b> by %s, %s; with <b>Ctrl</b> to restrict to horizontal/vertical; with <b>Shift</b> to disable snapping"), xs->str, ys->str);
    g_string_free(xs, TRUE);
    g_string_free(ys, TRUE);
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
