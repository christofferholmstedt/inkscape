#define __SP_NAMEDVIEW_C__

/*
 * <sodipodi:namedview> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2006      Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"

#include "display/canvas-grid.h"
#include "helper/units.h"
#include "svg/svg-color.h"
#include "xml/repr.h"
#include "attributes.h"
#include "document.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "event-log.h"
#include "sp-guide.h"
#include "sp-item-group.h"
#include "sp-namedview.h"
#include "prefs-utils.h"
#include "desktop.h"
#include "conn-avoid-ref.h" // for defaultConnSpacing.

#include "isnan.h" //temp fix for isnan().  include last

#define DEFAULTTOLERANCE 0.4
#define DEFAULTGRIDCOLOR 0x3f3fff25
#define DEFAULTGRIDEMPCOLOR 0x3f3fff60
#define DEFAULTGRIDEMPSPACING 5
#define DEFAULTGUIDECOLOR 0x0000ff7f
#define DEFAULTGUIDEHICOLOR 0xff00007f
#define DEFAULTBORDERCOLOR 0x000000ff
#define DEFAULTPAGECOLOR 0xffffff00

static void sp_namedview_class_init(SPNamedViewClass *klass);
static void sp_namedview_init(SPNamedView *namedview);

static void sp_namedview_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_namedview_release(SPObject *object);
static void sp_namedview_set(SPObject *object, unsigned int key, const gchar *value);
static void sp_namedview_child_added(SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref);
static void sp_namedview_remove_child(SPObject *object, Inkscape::XML::Node *child);
static Inkscape::XML::Node *sp_namedview_write(SPObject *object, Inkscape::XML::Node *repr, guint flags);

static void sp_namedview_setup_guides(SPNamedView * nv);

static gboolean sp_str_to_bool(const gchar *str);
static gboolean sp_nv_read_length(const gchar *str, guint base, gdouble *val, const SPUnit **unit);
static gboolean sp_nv_read_opacity(const gchar *str, guint32 *color);

static SPObjectGroupClass * parent_class;

GType
sp_namedview_get_type()
{
    static GType namedview_type = 0;
    if (!namedview_type) {
        GTypeInfo namedview_info = {
            sizeof(SPNamedViewClass),
            NULL,	/* base_init */
            NULL,	/* base_finalize */
            (GClassInitFunc) sp_namedview_class_init,
            NULL,	/* class_finalize */
            NULL,	/* class_data */
            sizeof(SPNamedView),
            16,	/* n_preallocs */
            (GInstanceInitFunc) sp_namedview_init,
            NULL,	/* value_table */
        };
        namedview_type = g_type_register_static(SP_TYPE_OBJECTGROUP, "SPNamedView", &namedview_info, (GTypeFlags)0);
    }
    return namedview_type;
}

static void sp_namedview_class_init(SPNamedViewClass * klass)
{
    GObjectClass * gobject_class;
    SPObjectClass * sp_object_class;

    gobject_class = (GObjectClass *) klass;
    sp_object_class = (SPObjectClass *) klass;

    parent_class = (SPObjectGroupClass*) g_type_class_ref(SP_TYPE_OBJECTGROUP);

    sp_object_class->build = sp_namedview_build;
    sp_object_class->release = sp_namedview_release;
    sp_object_class->set = sp_namedview_set;
    sp_object_class->child_added = sp_namedview_child_added;
    sp_object_class->remove_child = sp_namedview_remove_child;
    sp_object_class->write = sp_namedview_write;
}

static void sp_namedview_init(SPNamedView *nv)
{
    nv->editable = TRUE;
    nv->showguides = TRUE;
    nv->showborder = TRUE;
    nv->showpageshadow = TRUE;

    nv->guides = NULL;
    nv->viewcount = 0;
    nv->grids = NULL;

    nv->default_layer_id = 0;

    nv->connector_spacing = defaultConnSpacing;

    new (&nv->snap_manager) SnapManager(nv);
}

static void sp_namedview_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    SPNamedView *nv = (SPNamedView *) object;
    SPObjectGroup *og = (SPObjectGroup *) object;

    if (((SPObjectClass *) (parent_class))->build) {
        (* ((SPObjectClass *) (parent_class))->build)(object, document, repr);
    }

    sp_object_read_attr(object, "inkscape:document-units");
    sp_object_read_attr(object, "viewonly");
    sp_object_read_attr(object, "showguides");
    sp_object_read_attr(object, "showgrid");
    sp_object_read_attr(object, "gridtolerance");
    sp_object_read_attr(object, "guidetolerance");
    sp_object_read_attr(object, "objecttolerance");
    sp_object_read_attr(object, "guidecolor");
    sp_object_read_attr(object, "guideopacity");
    sp_object_read_attr(object, "guidehicolor");
    sp_object_read_attr(object, "guidehiopacity");
    sp_object_read_attr(object, "showborder");
    sp_object_read_attr(object, "inkscape:showpageshadow");
    sp_object_read_attr(object, "borderlayer");
    sp_object_read_attr(object, "bordercolor");
    sp_object_read_attr(object, "borderopacity");
    sp_object_read_attr(object, "pagecolor");
    sp_object_read_attr(object, "inkscape:pageopacity");
    sp_object_read_attr(object, "inkscape:pageshadow");
    sp_object_read_attr(object, "inkscape:zoom");
    sp_object_read_attr(object, "inkscape:cx");
    sp_object_read_attr(object, "inkscape:cy");
    sp_object_read_attr(object, "inkscape:window-width");
    sp_object_read_attr(object, "inkscape:window-height");
    sp_object_read_attr(object, "inkscape:window-x");
    sp_object_read_attr(object, "inkscape:window-y");
    sp_object_read_attr(object, "inkscape:snap-bbox");
    sp_object_read_attr(object, "inkscape:snap-nodes");
    sp_object_read_attr(object, "inkscape:snap-guide");
    sp_object_read_attr(object, "inkscape:snap-center");
    sp_object_read_attr(object, "inkscape:object-paths");
    sp_object_read_attr(object, "inkscape:object-nodes");
    sp_object_read_attr(object, "inkscape:bbox-paths");
    sp_object_read_attr(object, "inkscape:bbox-nodes");    
    sp_object_read_attr(object, "inkscape:current-layer");
    sp_object_read_attr(object, "inkscape:connector-spacing");

    /* Construct guideline list */

    for (SPObject *o = sp_object_first_child(SP_OBJECT(og)) ; o != NULL; o = SP_OBJECT_NEXT(o) ) {
        if (SP_IS_GUIDE(o)) {
            SPGuide * g = SP_GUIDE(o);
            nv->guides = g_slist_prepend(nv->guides, g);
            g_object_set(G_OBJECT(g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
        }
    }
}

static void sp_namedview_release(SPObject *object)
{
    SPNamedView *namedview = (SPNamedView *) object;

    if (namedview->guides) {
        g_slist_free(namedview->guides);
        namedview->guides = NULL;
    }

    // delete grids:
    while ( namedview->grids ) {
        Inkscape::CanvasGrid *gr = (Inkscape::CanvasGrid *)namedview->grids->data;
        delete gr;
        namedview->grids = g_slist_remove_link(namedview->grids, namedview->grids);
    }

    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release(object);
    }

    namedview->snap_manager.~SnapManager();
}

static void sp_namedview_set(SPObject *object, unsigned int key, const gchar *value)
{
    SPNamedView *nv = SP_NAMEDVIEW(object);
    SPUnit const &px = sp_unit_get_by_id(SP_UNIT_PX);

    switch (key) {
    case SP_ATTR_VIEWONLY:
            nv->editable = (!value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWGUIDES:
            if (!value) { // show guides if not specified, for backwards compatibility
                nv->showguides = TRUE;
            } else {
                nv->showguides = sp_str_to_bool(value);
            }
            sp_namedview_setup_guides(nv);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWGRIDS:
            if (!value) { // show grids if not specified, for backwards compatibility
                nv->grids_visible = false;
            } else {
                nv->grids_visible = sp_str_to_bool(value);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GRIDTOLERANCE:
            nv->gridtoleranceunit = &px;
            nv->gridtolerance = DEFAULTTOLERANCE;
            if (value) {
                sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &nv->gridtolerance, &nv->gridtoleranceunit);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDETOLERANCE:
            nv->guidetoleranceunit = &px;
            nv->guidetolerance = DEFAULTTOLERANCE;
            if (value) {
                sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &nv->guidetolerance, &nv->guidetoleranceunit);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_OBJECTTOLERANCE:
            nv->objecttoleranceunit = &px;
            nv->objecttolerance = DEFAULTTOLERANCE;
            if (value) {
                sp_nv_read_length(value, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &nv->objecttolerance, &nv->objecttoleranceunit);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDECOLOR:
            nv->guidecolor = (nv->guidecolor & 0xff) | (DEFAULTGUIDECOLOR & 0xffffff00);
            if (value) {
                nv->guidecolor = (nv->guidecolor & 0xff) | sp_svg_read_color(value, nv->guidecolor);
            }
            for (GSList *l = nv->guides; l != NULL; l = l->next) {
                g_object_set(G_OBJECT(l->data), "color", nv->guidecolor, NULL);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEOPACITY:
            nv->guidecolor = (nv->guidecolor & 0xffffff00) | (DEFAULTGUIDECOLOR & 0xff);
            sp_nv_read_opacity(value, &nv->guidecolor);
            for (GSList *l = nv->guides; l != NULL; l = l->next) {
                g_object_set(G_OBJECT(l->data), "color", nv->guidecolor, NULL);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEHICOLOR:
            nv->guidehicolor = (nv->guidehicolor & 0xff) | (DEFAULTGUIDEHICOLOR & 0xffffff00);
            if (value) {
                nv->guidehicolor = (nv->guidehicolor & 0xff) | sp_svg_read_color(value, nv->guidehicolor);
            }
            for (GSList *l = nv->guides; l != NULL; l = l->next) {
                g_object_set(G_OBJECT(l->data), "hicolor", nv->guidehicolor, NULL);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEHIOPACITY:
            nv->guidehicolor = (nv->guidehicolor & 0xffffff00) | (DEFAULTGUIDEHICOLOR & 0xff);
            sp_nv_read_opacity(value, &nv->guidehicolor);
            for (GSList *l = nv->guides; l != NULL; l = l->next) {
                g_object_set(G_OBJECT(l->data), "hicolor", nv->guidehicolor, NULL);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWBORDER:
            nv->showborder = (value) ? sp_str_to_bool (value) : TRUE;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_BORDERLAYER:
            nv->borderlayer = SP_BORDER_LAYER_BOTTOM;
            if (value && !strcasecmp(value, "true")) nv->borderlayer = SP_BORDER_LAYER_TOP;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_BORDERCOLOR:
            nv->bordercolor = (nv->bordercolor & 0xff) | (DEFAULTBORDERCOLOR & 0xffffff00);
            if (value) {
                nv->bordercolor = (nv->bordercolor & 0xff) | sp_svg_read_color (value, nv->bordercolor);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_BORDEROPACITY:
            nv->bordercolor = (nv->bordercolor & 0xffffff00) | (DEFAULTBORDERCOLOR & 0xff);
            sp_nv_read_opacity(value, &nv->bordercolor);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_PAGECOLOR:
            nv->pagecolor = (nv->pagecolor & 0xff) | (DEFAULTPAGECOLOR & 0xffffff00);
            if (value) {
                nv->pagecolor = (nv->pagecolor & 0xff) | sp_svg_read_color(value, nv->pagecolor);
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_PAGEOPACITY:
            nv->pagecolor = (nv->pagecolor & 0xffffff00) | (DEFAULTPAGECOLOR & 0xff);
            sp_nv_read_opacity(value, &nv->pagecolor);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_PAGESHADOW:
            nv->pageshadow = value? atoi(value) : 2; // 2 is the default
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWPAGESHADOW:
            nv->showpageshadow = (value) ? sp_str_to_bool(value) : TRUE;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_ZOOM:
            nv->zoom = value ? g_ascii_strtod(value, NULL) : 0; // zero means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CX:
            nv->cx = value ? g_ascii_strtod(value, NULL) : HUGE_VAL; // HUGE_VAL means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CY:
            nv->cy = value ? g_ascii_strtod(value, NULL) : HUGE_VAL; // HUGE_VAL means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_WIDTH:
            nv->window_width = value? atoi(value) : -1; // -1 means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_HEIGHT:
            nv->window_height = value ? atoi(value) : -1; // -1 means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_X:
            nv->window_x = value ? atoi(value) : -1; // -1 means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_Y:
            nv->window_y = value ? atoi(value) : -1; // -1 means not set
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX:
    		nv->snap_manager.setSnapModeBBox(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_NODES:
            nv->snap_manager.setSnapModeNode(value ? sp_str_to_bool(value) : TRUE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_CENTER:
            nv->snap_manager.setIncludeItemCenter(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_GUIDE:
            nv->snap_manager.setSnapModeGuide(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_OBJECT_PATHS:
            nv->snap_manager.object.setSnapToItemPath(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_OBJECT_NODES:
            nv->snap_manager.object.setSnapToItemNode(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_BBOX_PATHS:
            nv->snap_manager.object.setSnapToBBoxPath(value ? sp_str_to_bool(value) : FALSE);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_BBOX_NODES:
            nv->snap_manager.object.setSnapToBBoxNode(value ? sp_str_to_bool(value) : FALSE);            
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CURRENT_LAYER:
            nv->default_layer_id = value ? g_quark_from_string(value) : 0;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CONNECTOR_SPACING:
            nv->connector_spacing = value ? g_ascii_strtod(value, NULL) :
                    defaultConnSpacing;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_DOCUMENT_UNITS: {
            /* The default unit if the document doesn't override this: e.g. for files saved as
             * `plain SVG', or non-inkscape files, or files created by an inkscape 0.40 &
             * earlier.
             *
             * Here we choose `px': useful for screen-destined SVGs, and fewer bug reports
             * about "not the same numbers as what's in the SVG file" (at least for documents
             * without a viewBox attribute on the root <svg> element).  Similarly, it's also
             * the most reliable unit (i.e. least likely to be wrong in different viewing
             * conditions) for viewBox-less SVG files given that it's the unit that inkscape
             * uses for all coordinates.
             *
             * For documents that do have a viewBox attribute on the root <svg> element, it
             * might be better if we used either viewBox coordinates or if we used the unit of
             * say the width attribute of the root <svg> element.  However, these pose problems
             * in that they aren't in general absolute units as currently required by
             * doc_units.
             */
            SPUnit const *new_unit = &sp_unit_get_by_id(SP_UNIT_PX);

            if (value) {
                SPUnit const *const req_unit = sp_unit_get_by_abbreviation(value);
                if ( req_unit == NULL ) {
                    g_warning("Unrecognized unit `%s'", value);
                    /* fixme: Document errors should be reported in the status bar or
                     * the like (e.g. as per
                     * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing); g_log
                     * should be only for programmer errors. */
                } else if ( req_unit->base == SP_UNIT_ABSOLUTE ||
                            req_unit->base == SP_UNIT_DEVICE     ) {
                    new_unit = req_unit;
                } else {
                    g_warning("Document units must be absolute like `mm', `pt' or `px', but found `%s'",
                              value);
                    /* fixme: Don't use g_log (see above). */
                }
            }
            nv->doc_units = new_unit;
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    }
    default:
            if (((SPObjectClass *) (parent_class))->set) {
                ((SPObjectClass *) (parent_class))->set(object, key, value);
            }
            break;
    }
}

/**
* add a grid item from SVG-repr. Check if this namedview already has a gridobject for this one! If desktop=null, add grid-canvasitem to all desktops of this namedview,
* otherwise only add it to the specified desktop.
*/
static Inkscape::CanvasGrid*
sp_namedview_add_grid(SPNamedView *nv, Inkscape::XML::Node *repr, SPDesktop *desktop) {
    Inkscape::CanvasGrid* grid = NULL;
    //check if namedview already has an object for this grid
    for (GSList *l = nv->grids; l != NULL; l = l->next) {
        Inkscape::CanvasGrid* g = (Inkscape::CanvasGrid*) l->data;
        if (repr == g->repr) {
            grid = g;
            break;
        }
    }

    if (!grid) {
        //create grid object
        Inkscape::GridType gridtype = Inkscape::CanvasGrid::getGridTypeFromSVGName(repr->attribute("type"));
        SPDocument *doc = NULL;
        if (desktop)
            doc = sp_desktop_document(desktop);
        else
            doc = sp_desktop_document(static_cast<SPDesktop*>(nv->views->data));
        grid = Inkscape::CanvasGrid::NewGrid(nv, repr, doc, gridtype);
        nv->grids = g_slist_append(nv->grids, grid);
        //Initialize the snapping parameters for the new grid
        bool enabled_node = nv->snap_manager.getSnapModeNode();
        bool enabled_bbox = nv->snap_manager.getSnapModeBBox();
        grid->snapper->setSnapFrom(Inkscape::Snapper::SNAPPOINT_NODE, enabled_node);
        grid->snapper->setSnapFrom(Inkscape::Snapper::SNAPPOINT_BBOX, enabled_bbox);
    }

    if (!desktop) {
        //add canvasitem to all desktops
        for (GSList *l = nv->views; l != NULL; l = l->next) {
            SPDesktop *dt = static_cast<SPDesktop*>(l->data);
            grid->createCanvasItem(dt);
        }
    } else {
        //add canvasitem only for specified desktop
        grid->createCanvasItem(desktop);
    }

    return grid;
}

static void sp_namedview_child_added(SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPNamedView *nv = (SPNamedView *) object;

    if (((SPObjectClass *) (parent_class))->child_added) {
        (* ((SPObjectClass *) (parent_class))->child_added)(object, child, ref);
    }

    const gchar *id = child->attribute("id");
    if (!strcmp(child->name(), "inkscape:grid")) {
        sp_namedview_add_grid(nv, child, NULL);
    } else {
        SPObject *no = object->document->getObjectById(id);
        g_assert(SP_IS_OBJECT(no));

        if (SP_IS_GUIDE(no)) {
            SPGuide *g = (SPGuide *) no;
            nv->guides = g_slist_prepend(nv->guides, g);
            g_object_set(G_OBJECT(g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
            if (nv->editable) {
                for (GSList *l = nv->views; l != NULL; l = l->next) {
                    sp_guide_show(g, static_cast<SPDesktop*>(l->data)->guides, (GCallback) sp_dt_guide_event);
                    if (static_cast<SPDesktop*>(l->data)->guides_active)
                        sp_guide_sensitize(g,
                                           sp_desktop_canvas(static_cast<SPDesktop*> (l->data)),
                                           TRUE);
                    if (nv->showguides) {
                        for (GSList *v = SP_GUIDE(g)->views; v != NULL; v = v->next) {
                            sp_canvas_item_show(SP_CANVAS_ITEM(v->data));
                        }
                    } else {
                        for (GSList *v = SP_GUIDE(g)->views; v != NULL; v = v->next) {
                            sp_canvas_item_hide(SP_CANVAS_ITEM(v->data));
                        }
                    }
                }
            }
        }
    }
}

static void sp_namedview_remove_child(SPObject *object, Inkscape::XML::Node *child)
{
    SPNamedView *nv = (SPNamedView *) object;

    if (!strcmp(child->name(), "inkscape:grid")) {
        for ( GSList *iter = nv->grids ; iter ; iter = iter->next ) {
            Inkscape::CanvasGrid *gr = (Inkscape::CanvasGrid *)iter->data;
            if ( gr->repr == child ) {
                delete gr;
                nv->grids = g_slist_remove_link(nv->grids, iter);
                break;
            }
        }
    } else {
        GSList **ref = &nv->guides;
        for ( GSList *iter = nv->guides ; iter ; iter = iter->next ) {
            if ( SP_OBJECT_REPR((SPObject *)iter->data) == child ) {
                *ref = iter->next;
                iter->next = NULL;
                g_slist_free_1(iter);
                break;
            }
            ref = &iter->next;
        }
    }

    if (((SPObjectClass *) (parent_class))->remove_child) {
        (* ((SPObjectClass *) (parent_class))->remove_child)(object, child);
    }
}

static Inkscape::XML::Node *sp_namedview_write(SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
    if ( ( flags & SP_OBJECT_WRITE_EXT ) &&
         repr != SP_OBJECT_REPR(object) )
    {
        if (repr) {
            repr->mergeFrom(SP_OBJECT_REPR(object), "id");
        } else {
             /// \todo FIXME:  Plumb an appropriate XML::Document into this
             repr = SP_OBJECT_REPR(object)->duplicate(NULL);
        }
    }

    return repr;
}

void SPNamedView::show(SPDesktop *desktop)
{
    for (GSList *l = guides; l != NULL; l = l->next) {
        sp_guide_show(SP_GUIDE(l->data), desktop->guides, (GCallback) sp_dt_guide_event);
        if (desktop->guides_active) {
            sp_guide_sensitize(SP_GUIDE(l->data), sp_desktop_canvas(desktop), TRUE);
        }
        if (showguides) {
            for (GSList *v = SP_GUIDE(l->data)->views; v != NULL; v = v->next) {
                sp_canvas_item_show(SP_CANVAS_ITEM(v->data));
            }
        } else {
            for (GSList *v = SP_GUIDE(l->data)->views; v != NULL; v = v->next) {
                sp_canvas_item_hide(SP_CANVAS_ITEM(v->data));
            }
        }
    }

    views = g_slist_prepend(views, desktop);

    // generate grids specified in SVG:
    Inkscape::XML::Node *repr = SP_OBJECT_REPR(this);
    if (repr) {
        for (Inkscape::XML::Node * child = repr->firstChild() ; child != NULL; child = child->next() ) {
            if (!strcmp(child->name(), "inkscape:grid")) {
                sp_namedview_add_grid(this, child, desktop);
            }
        }
    }

    desktop->showGrids(grids_visible, false);
}

#define MIN_ONSCREEN_DISTANCE 50

/*
 * Restores window geometry from the document settings or defaults in prefs
 */
void sp_namedview_window_from_document(SPDesktop *desktop)
{
    SPNamedView *nv = desktop->namedview;
    gint geometry_from_file =
        (1==prefs_get_int_attribute("options.savewindowgeometry", "value", 0));

    // restore window size and position stored with the document
    if (geometry_from_file) {
        gint w = MIN(gdk_screen_width(), nv->window_width);
        gint h = MIN(gdk_screen_height(), nv->window_height);
        gint x = MIN(gdk_screen_width() - MIN_ONSCREEN_DISTANCE, nv->window_x);
        gint y = MIN(gdk_screen_height() - MIN_ONSCREEN_DISTANCE, nv->window_y);
        if (w>0 && h>0 && x>0 && y>0) {
            x = MIN(gdk_screen_width() - w, x);
            y = MIN(gdk_screen_height() - h, y);
        }
        if (w>0 && h>0) {
            desktop->setWindowSize(w, h);
        }
        if (x>0 && y>0) {
            desktop->setWindowPosition(NR::Point(x, y));
        }
    }

    // restore zoom and view
    if (nv->zoom != 0 && nv->zoom != HUGE_VAL && !isNaN(nv->zoom)
        && nv->cx != HUGE_VAL && !isNaN(nv->cx)
        && nv->cy != HUGE_VAL && !isNaN(nv->cy)) {
        desktop->zoom_absolute(nv->cx, nv->cy, nv->zoom);
    } else if (sp_desktop_document(desktop)) { // document without saved zoom, zoom to its page
        desktop->zoom_page();
    }

    // cancel any history of zooms up to this point
    if (desktop->zooms_past) {
        g_list_free(desktop->zooms_past);
        desktop->zooms_past = NULL;
    }
}

void sp_namedview_update_layers_from_document (SPDesktop *desktop)
{
    SPObject *layer = NULL;
    SPDocument *document = desktop->doc();
    SPNamedView *nv = desktop->namedview;
    if ( nv->default_layer_id != 0 ) {
        layer = document->getObjectById(g_quark_to_string(nv->default_layer_id));
    }
    // don't use that object if it's not at least group
    if ( !layer || !SP_IS_GROUP(layer) ) {
        layer = NULL;
    }
    // if that didn't work out, look for the topmost layer
    if (!layer) {
        SPObject *iter = sp_object_first_child(SP_DOCUMENT_ROOT(document));
        for ( ; iter ; iter = SP_OBJECT_NEXT(iter) ) {
            if (desktop->isLayer(iter)) {
                layer = iter;
            }
        }
    }
    if (layer) {
        desktop->setCurrentLayer(layer);
    }

    // FIXME: find a better place to do this
    desktop->event_log->updateUndoVerbs();
}

void sp_namedview_document_from_window(SPDesktop *desktop)
{
    gint save_geometry_in_file =
        (1==prefs_get_int_attribute("options.savewindowgeometry", "value", 0));
    Inkscape::XML::Node *view = SP_OBJECT_REPR(desktop->namedview);
    NR::Rect const r = desktop->get_display_area();

    // saving window geometry is not undoable
    bool saved = sp_document_get_undo_sensitive(sp_desktop_document(desktop));
    sp_document_set_undo_sensitive(sp_desktop_document(desktop), false);

    sp_repr_set_svg_double(view, "inkscape:zoom", desktop->current_zoom());
    sp_repr_set_svg_double(view, "inkscape:cx", r.midpoint()[NR::X]);
    sp_repr_set_svg_double(view, "inkscape:cy", r.midpoint()[NR::Y]);

    if (save_geometry_in_file) {
        gint w, h, x, y;
        desktop->getWindowGeometry(x, y, w, h);
        sp_repr_set_int(view, "inkscape:window-width", w);
        sp_repr_set_int(view, "inkscape:window-height", h);
        sp_repr_set_int(view, "inkscape:window-x", x);
        sp_repr_set_int(view, "inkscape:window-y", y);
    }

    view->setAttribute("inkscape:current-layer", SP_OBJECT_ID(desktop->currentLayer()));

    // restore undoability
    sp_document_set_undo_sensitive(sp_desktop_document(desktop), saved);
}

void SPNamedView::hide(SPDesktop const *desktop)
{
    g_assert(desktop != NULL);
    g_assert(g_slist_find(views, desktop));

    for (GSList *l = guides; l != NULL; l = l->next) {
        sp_guide_hide(SP_GUIDE(l->data), sp_desktop_canvas(desktop));
    }

    views = g_slist_remove(views, desktop);

    // delete grids:
    while ( grids ) {
        Inkscape::CanvasGrid *gr = (Inkscape::CanvasGrid *)grids->data;
        delete gr;
        grids = g_slist_remove_link(grids, grids);
    }
}

void SPNamedView::activateGuides(gpointer desktop, gboolean active)
{
    g_assert(desktop != NULL);
    g_assert(g_slist_find(views, desktop));

    SPDesktop *dt = static_cast<SPDesktop*>(desktop);

    for (GSList *l = guides; l != NULL; l = l->next) {
        sp_guide_sensitize(SP_GUIDE(l->data), sp_desktop_canvas(dt), active);
    }
}

static void sp_namedview_setup_guides(SPNamedView *nv)
{
    for (GSList *l = nv->guides; l != NULL; l = l->next) {
        if (nv->showguides) {
            for (GSList *v = SP_GUIDE(l->data)->views; v != NULL; v = v->next) {
                sp_canvas_item_show(SP_CANVAS_ITEM(v->data));
            }
        } else {
            for (GSList *v = SP_GUIDE(l->data)->views; v != NULL; v = v->next) {
                sp_canvas_item_hide(SP_CANVAS_ITEM(v->data));
            }
        }
    }
}

void sp_namedview_toggle_guides(SPDocument *doc, Inkscape::XML::Node *repr)
{
    unsigned int v;
    unsigned int set = sp_repr_get_boolean(repr, "showguides", &v);
    if (!set) { // hide guides if not specified, for backwards compatibility
        v = FALSE;
    } else {
        v = !v;
    }

    bool saved = sp_document_get_undo_sensitive(doc);
    sp_document_set_undo_sensitive(doc, false);

    sp_repr_set_boolean(repr, "showguides", v);

    doc->rroot->setAttribute("sodipodi:modified", "true");
    sp_document_set_undo_sensitive(doc, saved);
}

void sp_namedview_show_grids(SPNamedView * namedview, bool show, bool dirty_document)
{
    namedview->grids_visible = show;

    SPDocument *doc = SP_OBJECT_DOCUMENT (namedview);
    Inkscape::XML::Node *repr = SP_OBJECT_REPR(namedview);

    bool saved = sp_document_get_undo_sensitive(doc);
    sp_document_set_undo_sensitive(doc, false);

    sp_repr_set_boolean(repr, "showgrid", namedview->grids_visible);

    /* we don't want the document to get dirty on startup; that's when
       we call this function with dirty_document = false */
    if (dirty_document) {
        doc->rroot->setAttribute("sodipodi:modified", "true");
    }
    sp_document_set_undo_sensitive(doc, saved);
}

gchar const *SPNamedView::getName() const
{
    SPException ex;
    SP_EXCEPTION_INIT(&ex);
    return sp_object_getAttribute(SP_OBJECT(this), "id", &ex);
}

guint SPNamedView::getViewCount()
{
    return ++viewcount;
}

GSList const *SPNamedView::getViewList() const
{
    return views;
}

/* This should be moved somewhere */

static gboolean sp_str_to_bool(const gchar *str)
{
    if (str) {
        if (!g_strcasecmp(str, "true") ||
            !g_strcasecmp(str, "yes") ||
            !g_strcasecmp(str, "y") ||
            (atoi(str) != 0)) {
            return TRUE;
        }
    }

    return FALSE;
}

/* fixme: Collect all these length parsing methods and think common sane API */

static gboolean sp_nv_read_length(const gchar *str, guint base, gdouble *val, const SPUnit **unit)
{
    if (!str) {
        return FALSE;
    }

    gchar *u;
    gdouble v = g_ascii_strtod(str, &u);
    if (!u) {
        return FALSE;
    }
    while (isspace(*u)) {
        u += 1;
    }

    if (!*u) {
        /* No unit specified - keep default */
        *val = v;
        return TRUE;
    }

    if (base & SP_UNIT_DEVICE) {
        if (u[0] && u[1] && !isalnum(u[2]) && !strncmp(u, "px", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_PX);
            *val = v;
            return TRUE;
        }
    }

    if (base & SP_UNIT_ABSOLUTE) {
        if (!strncmp(u, "pt", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_PT);
        } else if (!strncmp(u, "mm", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_MM);
        } else if (!strncmp(u, "cm", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_CM);
        } else if (!strncmp(u, "m", 1)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_M);
        } else if (!strncmp(u, "in", 2)) {
            *unit = &sp_unit_get_by_id(SP_UNIT_IN);
        } else {
            return FALSE;
        }
        *val = v;
        return TRUE;
    }

    return FALSE;
}

static gboolean sp_nv_read_opacity(const gchar *str, guint32 *color)
{
    if (!str) {
        return FALSE;
    }

    gchar *u;
    gdouble v = g_ascii_strtod(str, &u);
    if (!u) {
        return FALSE;
    }
    v = CLAMP(v, 0.0, 1.0);

    *color = (*color & 0xffffff00) | (guint32) floor(v * 255.9999);

    return TRUE;
}

SPNamedView *sp_document_namedview(SPDocument *document, const gchar *id)
{
    g_return_val_if_fail(document != NULL, NULL);

    SPObject *nv = sp_item_group_get_child_by_name((SPGroup *) document->root, NULL, "sodipodi:namedview");
    g_assert(nv != NULL);

    if (id == NULL) {
        return (SPNamedView *) nv;
    }

    while (nv && strcmp(nv->id, id)) {
        nv = sp_item_group_get_child_by_name((SPGroup *) document->root, nv, "sodipodi:namedview");
    }

    return (SPNamedView *) nv;
}

/**
 * Returns namedview's default metric.
 */
SPMetric SPNamedView::getDefaultMetric() const
{
    if (doc_units) {
        return sp_unit_get_metric(doc_units);
    } else {
        return SP_PT;
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
