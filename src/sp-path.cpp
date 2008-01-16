#define __SP_PATH_C__

/*
 * SVG <path> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   David Turner <novalis@gnu.org>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glibmm/i18n.h>

#include <display/curve.h>
#include <libnr/n-art-bpath.h>
#include <libnr/nr-path.h>
#include <libnr/nr-matrix-fns.h>

#include "svg/svg.h"
#include "xml/repr.h"
#include "attributes.h"

#include "sp-path.h"
#include "sp-guide.h"

#include "document.h"

#define noPATH_VERBOSE

static void sp_path_class_init(SPPathClass *klass);
static void sp_path_init(SPPath *path);
static void sp_path_finalize(GObject *obj);
static void sp_path_release(SPObject *object);

static void sp_path_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_path_set(SPObject *object, unsigned key, gchar const *value);

static Inkscape::XML::Node *sp_path_write(SPObject *object, Inkscape::XML::Node *repr, guint flags);
static NR::Matrix sp_path_set_transform(SPItem *item, NR::Matrix const &xform);
static gchar * sp_path_description(SPItem *item);
static void sp_path_convert_to_guides(SPItem *item);

static void sp_path_update(SPObject *object, SPCtx *ctx, guint flags);
static void sp_path_update_patheffect(SPShape *shape, bool write);

static SPShapeClass *parent_class;

/**
 * Gets the GType object for SPPathClass
 */
GType
sp_path_get_type(void)
{
    static GType type = 0;

    if (!type) {
        GTypeInfo info = {
            sizeof(SPPathClass),
            NULL, NULL,
            (GClassInitFunc) sp_path_class_init,
            NULL, NULL,
            sizeof(SPPath),
            16,
            (GInstanceInitFunc) sp_path_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_SHAPE, "SPPath", &info, (GTypeFlags)0);
    }
    return type;
}

/**
 *  Does the object-oriented work of initializing the class structure
 *  including parent class, and registers function pointers for
 *  the functions build, set, write, and set_transform.
 */
static void
sp_path_class_init(SPPathClass * klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    SPObjectClass *sp_object_class = (SPObjectClass *) klass;
    SPItemClass *item_class = (SPItemClass *) klass;
    SPShapeClass *shape_class = (SPShapeClass *) klass;

    parent_class = (SPShapeClass *)g_type_class_peek_parent(klass);

    gobject_class->finalize = sp_path_finalize;

    sp_object_class->build = sp_path_build;
    sp_object_class->release = sp_path_release;
    sp_object_class->set = sp_path_set;
    sp_object_class->write = sp_path_write;
    sp_object_class->update = sp_path_update;

    item_class->description = sp_path_description;
    item_class->set_transform = sp_path_set_transform;
    item_class->convert_to_guides = sp_path_convert_to_guides;

    shape_class->update_patheffect = sp_path_update_patheffect;
}


gint
sp_nodes_in_path(SPPath *path)
{
    SPCurve *curve = SP_SHAPE(path)->curve;
    if (!curve) return 0;
    gint r = curve->end;
    gint i = curve->length - 1;
    if (i > r) i = r; // sometimes after switching from node editor length is wrong, e.g. f6 - draw - f2 - tab - f1, this fixes it
    for (; i >= 0; i --)
        if (SP_CURVE_BPATH(curve)[i].code == NR_MOVETO)
            r --;
    return r;
}

static gchar *
sp_path_description(SPItem * item)
{
    int count = sp_nodes_in_path(SP_PATH(item));
    if (SP_SHAPE(item)->path_effect_href) {
        return g_strdup_printf(ngettext("<b>Path</b> (%i node, path effect)",
                                        "<b>Path</b> (%i nodes, path effect)",count), count);
    } else {
        return g_strdup_printf(ngettext("<b>Path</b> (%i node)",
                                        "<b>Path</b> (%i nodes)",count), count);
    }
}

static void
sp_path_convert_to_guides(SPItem *item)
{
    SPPath *path = SP_PATH(item);

    SPDocument *doc = SP_OBJECT_DOCUMENT(path);
    std::list<std::pair<Geom::Point, Geom::Point> > pts;

    NR::Matrix const i2d (sp_item_i2d_affine(SP_ITEM(path)));

    SPCurve *curve = SP_SHAPE(path)->curve;
    if (!curve) return;
    NArtBpath *bpath = SP_CURVE_BPATH(curve);

    NR::Point last_pt;
    NR::Point pt;
    for (int i = 0; bpath[i].code != NR_END; i++){
        if (bpath[i].code == NR_LINETO) {
            /* we only convert straight line segments (converting curve segments would be unintuitive) */
            pt = bpath[i].c(3) * i2d;
            pts.push_back(std::make_pair(last_pt.to_2geom(), pt.to_2geom()));
        }

        /* remember current point for potential reuse in the next step
           (e.g., in case this was an NR_MOVETO or NR_MOVETO_OPEN) */
        last_pt = bpath[i].c(3) * i2d;
    }

    sp_guide_pt_pairs_to_guides(doc, pts);

    SP_OBJECT(path)->deleteObject(true);
}

/**
 * Initializes an SPPath.
 */
static void
sp_path_init(SPPath *path)
{
    new (&path->connEndPair) SPConnEndPair(path);

    path->original_curve = NULL;
}

static void
sp_path_finalize(GObject *obj)
{
    SPPath *path = (SPPath *) obj;

    path->connEndPair.~SPConnEndPair();
}

/**
 *  Given a repr, this sets the data items in the path object such as
 *  fill & style attributes, markers, and CSS properties.
 */
static void
sp_path_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    /* Are these calls actually necessary? */
    sp_object_read_attr(object, "marker");
    sp_object_read_attr(object, "marker-start");
    sp_object_read_attr(object, "marker-mid");
    sp_object_read_attr(object, "marker-end");

    sp_conn_end_pair_build(object);

    if (((SPObjectClass *) parent_class)->build) {
        ((SPObjectClass *) parent_class)->build(object, document, repr);
    }

    sp_object_read_attr(object, "inkscape:original-d");
    sp_object_read_attr(object, "d");

    /* d is a required attribute */
    gchar const *d = sp_object_getAttribute(object, "d", NULL);
    if (d == NULL) {
        sp_object_set(object, sp_attribute_lookup("d"), "");
    }
}

static void
sp_path_release(SPObject *object)
{
    SPPath *path = SP_PATH(object);

    path->connEndPair.release();

    if (path->original_curve) {
        path->original_curve = sp_curve_unref (path->original_curve);
    }

    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release(object);
    }
}

/**
 *  Sets a value in the path object given by 'key', to 'value'.  This is used
 *  for setting attributes and markers on a path object.
 */
static void
sp_path_set(SPObject *object, unsigned int key, gchar const *value)
{
    SPPath *path = (SPPath *) object;

    switch (key) {
        case SP_ATTR_INKSCAPE_ORIGINAL_D:
                if (value) {
                    NArtBpath *bpath = sp_svg_read_path(value);
                    SPCurve *curve = sp_curve_new_from_bpath(bpath);
                    if (curve) {
                        sp_path_set_original_curve(path, curve, TRUE, true);
                        sp_curve_unref(curve);
                    }
                } else {
                    sp_path_set_original_curve(path, NULL, TRUE, true);
                }
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
       case SP_ATTR_D:
            if (!((SPShape *) path)->path_effect_href) {
                if (value) {
                    NArtBpath *bpath = sp_svg_read_path(value);
                    SPCurve *curve = sp_curve_new_from_bpath(bpath);
                    if (curve) {
                        sp_shape_set_curve((SPShape *) path, curve, TRUE);
                        sp_curve_unref(curve);
                    }
                } else {
                    sp_shape_set_curve((SPShape *) path, NULL, TRUE);
                }
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_PROP_MARKER:
        case SP_PROP_MARKER_START:
        case SP_PROP_MARKER_MID:
        case SP_PROP_MARKER_END:
            sp_shape_set_marker(object, key, value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_CONNECTOR_TYPE:
        case SP_ATTR_CONNECTION_START:
        case SP_ATTR_CONNECTION_END:
            path->connEndPair.setAttr(key, value);
            break;
        default:
            if (((SPObjectClass *) parent_class)->set) {
                ((SPObjectClass *) parent_class)->set(object, key, value);
            }
            break;
    }
}

/**
 *
 * Writes the path object into a Inkscape::XML::Node
 */
static Inkscape::XML::Node *
sp_path_write(SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
    SPShape *shape = (SPShape *) object;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        Inkscape::XML::Document *xml_doc = sp_document_repr_doc(SP_OBJECT_DOCUMENT(object));
        repr = xml_doc->createElement("svg:path");
    }

    if ( shape->curve != NULL ) {
        NArtBpath *abp = sp_curve_first_bpath(shape->curve);
        if (abp) {
            gchar *str = sp_svg_write_path(abp);
            repr->setAttribute("d", str);
            g_free(str);
        } else {
            repr->setAttribute("d", "");
        }
    } else {
        repr->setAttribute("d", NULL);
    }

    SPPath *path = (SPPath *) object;
    if ( path->original_curve != NULL ) {
        NArtBpath *abp = sp_curve_first_bpath(path->original_curve);
        if (abp) {
            gchar *str = sp_svg_write_path(abp);
            repr->setAttribute("inkscape:original-d", str);
            g_free(str);
        } else {
            repr->setAttribute("inkscape:original-d", "");
        }
    } else {
        repr->setAttribute("inkscape:original-d", NULL);
    }

    SP_PATH(shape)->connEndPair.writeRepr(repr);

    if (((SPObjectClass *)(parent_class))->write) {
        ((SPObjectClass *)(parent_class))->write(object, repr, flags);
    }

    return repr;
}

static void
sp_path_update(SPObject *object, SPCtx *ctx, guint flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
    }

    if (((SPObjectClass *) parent_class)->update) {
        ((SPObjectClass *) parent_class)->update(object, ctx, flags);
    }

    SPPath *path = SP_PATH(object);
    path->connEndPair.update();
}


/**
 * Writes the given transform into the repr for the given item.
 */
static NR::Matrix
sp_path_set_transform(SPItem *item, NR::Matrix const &xform)
{
    SPShape *shape = (SPShape *) item;
    SPPath *path = (SPPath *) item;

    if (!shape->curve) { // 0 nodes, nothing to transform
        return NR::identity();
    }

    if (path->original_curve) { /* Transform the original-d path */
        NRBPath dorigpath, sorigpath;
        sorigpath.path = SP_CURVE_BPATH(path->original_curve);
        nr_path_duplicate_transform(&dorigpath, &sorigpath, xform);
        SPCurve *origcurve = sp_curve_new_from_bpath(dorigpath.path);
        if (origcurve) {
            sp_path_set_original_curve(path, origcurve, TRUE, true);
            sp_curve_unref(origcurve);
        }
    } else {    /* Transform the path */
        NRBPath dpath, spath;
        spath.path = SP_CURVE_BPATH(shape->curve);
        nr_path_duplicate_transform(&dpath, &spath, xform);
        SPCurve *curve = sp_curve_new_from_bpath(dpath.path);
        if (curve) {
            sp_shape_set_curve(shape, curve, TRUE);
            sp_curve_unref(curve);
        }
    }

    // Adjust stroke
    sp_item_adjust_stroke(item, NR::expansion(xform));

    // Adjust pattern fill
    sp_item_adjust_pattern(item, xform);

    // Adjust gradient fill
    sp_item_adjust_gradient(item, xform);

    // Adjust LPE
    sp_item_adjust_livepatheffect(item, xform);

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);

    // nothing remains - we've written all of the transform, so return identity
    return NR::identity();
}

static void
sp_path_update_patheffect(SPShape *shape, bool write)
{
    SPPath *path = (SPPath *) shape;
    if (path->original_curve) {
        SPCurve *curve = sp_curve_copy (path->original_curve);
        sp_shape_perform_path_effect(curve, shape);
        sp_shape_set_curve(shape, curve, TRUE);
        sp_curve_unref(curve);

        if (write) {
            // could also do SP_OBJECT(shape)->updateRepr();  but only the d attribute needs updating.
            Inkscape::XML::Node *repr = SP_OBJECT_REPR(shape);
            if ( shape->curve != NULL ) {
                NArtBpath *abp = sp_curve_first_bpath(shape->curve);
                if (abp) {
                    gchar *str = sp_svg_write_path(abp);
                    repr->setAttribute("d", str);
                    g_free(str);
                } else {
                    repr->setAttribute("d", "");
                }
            } else {
                repr->setAttribute("d", NULL);
            }
        }
    } else {

    }
}


/**
 * Adds a original_curve to the path.  If owner is specified, a reference
 * will be made, otherwise the curve will be copied into the path.
 * Any existing curve in the path will be unreferenced first.
 * This routine triggers reapplication of the an effect is present
 * an also triggers a request to update the display. Does not write
* result to XML when write=false.
 */
void
sp_path_set_original_curve (SPPath *path, SPCurve *curve, unsigned int owner, bool write)
{
    if (path->original_curve) {
        path->original_curve = sp_curve_unref (path->original_curve);
    }
    if (curve) {
        if (owner) {
            path->original_curve = sp_curve_ref (curve);
        } else {
            path->original_curve = sp_curve_copy (curve);
        }
    }
    sp_path_update_patheffect(path, write);
    SP_OBJECT(path)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Return duplicate of original_curve (if any exists) or NULL if there is no curve
 */
SPCurve *
sp_path_get_original_curve (SPPath *path)
{
    if (path->original_curve) {
        return sp_curve_copy (path->original_curve);
    }
    return NULL;
}

/**
 * Return duplicate of edittable curve which is original_curve if it exists or
 * shape->curve if not.
 */
SPCurve*
sp_path_get_curve_for_edit (SPPath *path)
{
    if (path->original_curve) {
        return sp_path_get_original_curve(path);
    } else {
        return sp_shape_get_curve( (SPShape *) path );
    }
}

/**
 * Return a reference to original_curve if it exists or
 * shape->curve if not.
 */
const SPCurve*
sp_path_get_curve_reference (SPPath *path)
{
    if (path->original_curve) {
        return path->original_curve;
    } else {
        return path->curve;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
