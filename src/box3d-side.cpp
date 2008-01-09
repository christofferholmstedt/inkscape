#define __BOX3D_SIDE_C__

/*
 * 3D box face implementation
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007  Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "box3d-side.h"
#include "document.h"
#include "xml/document.h"
#include "xml/repr.h"
#include "display/curve.h"
#include "svg/svg.h"
#include "attributes.h"
#include "inkscape.h"
#include "persp3d.h"
#include "box3d-context.h"
#include "prefs-utils.h"
#include "desktop-style.h"
#include "box3d.h"

static void box3d_side_class_init (Box3DSideClass *klass);
static void box3d_side_init (Box3DSide *side);

static void box3d_side_build (SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static Inkscape::XML::Node *box3d_side_write (SPObject *object, Inkscape::XML::Node *repr, guint flags);
static void box3d_side_set (SPObject *object, unsigned int key, const gchar *value);
static void box3d_side_update (SPObject *object, SPCtx *ctx, guint flags);

//static gchar * box3d_side_description (SPItem * item);
//static void box3d_side_snappoints(SPItem const *item, SnapPointsIter p);

//static void box3d_side_set_shape (SPShape *shape);
//static void box3d_side_update_patheffect (SPShape *shape, bool write);

static Proj::Pt3 box3d_side_corner (Box3DSide *side, guint index);
static std::vector<Proj::Pt3> box3d_side_corners (Box3DSide *side);
// static gint box3d_side_descr_to_id (gchar const *descr);

static SPShapeClass *parent_class;

GType
box3d_side_get_type (void)
{
    static GType type = 0;

    if (!type) {
        GTypeInfo info = {
            sizeof (Box3DSideClass),
            NULL, NULL,
            (GClassInitFunc) box3d_side_class_init,
            NULL, NULL,
            sizeof (Box3DSide),
            16,
            (GInstanceInitFunc) box3d_side_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static (SP_TYPE_SHAPE, "Box3DSide", &info, (GTypeFlags)0);
    }
    return type;
}

static void
box3d_side_class_init (Box3DSideClass *klass)
{
    GObjectClass * gobject_class;
    SPObjectClass * sp_object_class;
    SPItemClass * item_class;
    SPPathClass * path_class;
    SPShapeClass * shape_class;

    gobject_class = (GObjectClass *) klass;
    sp_object_class = (SPObjectClass *) klass;
    item_class = (SPItemClass *) klass;
    path_class = (SPPathClass *) klass;
    shape_class = (SPShapeClass *) klass;

    parent_class = (SPShapeClass *)g_type_class_ref (SP_TYPE_SHAPE);

    sp_object_class->build = box3d_side_build;
    sp_object_class->write = box3d_side_write;
    sp_object_class->set = box3d_side_set;
    sp_object_class->update = box3d_side_update;

    //item_class->description = box3d_side_description;
    //item_class->snappoints = box3d_side_snappoints;

    shape_class->set_shape = box3d_side_set_shape;
    //shape_class->update_patheffect = box3d_side_update_patheffect;
}

static void
box3d_side_init (Box3DSide * side)
{
    side->dir1 = Box3D::NONE;
    side->dir2 = Box3D::NONE;
    side->front_or_rear = Box3D::FRONT;
}

static void
box3d_side_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr)
{
    if (((SPObjectClass *) parent_class)->build)
        ((SPObjectClass *) parent_class)->build (object, document, repr);

    sp_object_read_attr(object, "inkscape:box3dsidetype");
}

static Inkscape::XML::Node *
box3d_side_write (SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
    Box3DSide *side = SP_BOX3D_SIDE (object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        // this is where we end up when saving as plain SVG (also in other circumstances?)
        // thus we don' set "sodipodi:type" so that the box is only saved as an ordinary svg:path
        Inkscape::XML::Document *xml_doc = sp_document_repr_doc(SP_OBJECT_DOCUMENT(object));
        repr = xml_doc->createElement("svg:path");
    }

    if (flags & SP_OBJECT_WRITE_EXT) {
        sp_repr_set_int(repr, "inkscape:box3dsidetype", side->dir1 ^ side->dir2 ^ side->front_or_rear);
    }

    sp_shape_set_shape ((SPShape *) object);

    /* Duplicate the path */
    SPCurve *curve = ((SPShape *) object)->curve;
    //Nulls might be possible if this called iteratively
    if ( !curve ) {
        return NULL;
    }
    NArtBpath *bpath = SP_CURVE_BPATH(curve);
    if ( !bpath ) {
        return NULL;
    }
    char *d = sp_svg_write_path ( bpath );
    repr->setAttribute("d", d);
    g_free (d);

    if (((SPObjectClass *) (parent_class))->write)
        ((SPObjectClass *) (parent_class))->write (object, repr, flags);

    return repr;
}

static void
box3d_side_set (SPObject *object, unsigned int key, const gchar *value)
{
    Box3DSide *side = SP_BOX3D_SIDE (object);

    // TODO: In case the box was recreated (by undo, e.g.) we need to recreate the path
    //       (along with other info?) from the parent box.

    /* fixme: we should really collect updates */
    switch (key) {
        case SP_ATTR_INKSCAPE_BOX3D_SIDE_TYPE:
            if (value) {
                guint desc = atoi (value);

                if (!Box3D::is_face_id(desc)) {
                    g_print ("desc is not a face id: =%s=\n", value);
                }
                g_return_if_fail (Box3D::is_face_id (desc));
                Box3D::Axis plane = (Box3D::Axis) (desc & 0x7);
                plane = (Box3D::is_plane(plane) ? plane : Box3D::orth_plane_or_axis(plane));
                side->dir1 = Box3D::extract_first_axis_direction(plane);
                side->dir2 = Box3D::extract_second_axis_direction(plane);
                side->front_or_rear = (Box3D::FrontOrRear) (desc & 0x8);

                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
    default:
        if (((SPObjectClass *) parent_class)->set)
            ((SPObjectClass *) parent_class)->set (object, key, value);
        break;
    }
}

static void
box3d_side_update (SPObject *object, SPCtx *ctx, guint flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
    }

    //g_print ("box3d_side_update\n");
    if (flags & (SP_OBJECT_MODIFIED_FLAG |
                 SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        sp_shape_set_shape ((SPShape *) object);
    }

    if (((SPObjectClass *) parent_class)->update)
        ((SPObjectClass *) parent_class)->update (object, ctx, flags);
}

/***
static void
box3d_side_update_patheffect(SPShape *shape, bool write)
{
    box3d_side_set_shape(shape);

    if (write) {
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

    ((SPObject *)shape)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}
***/

/***
static gchar *
box3d_side_description (SPItem *item)
{
    Box3DSide *side = SP_BOX3D_SIDE (item);

    // while there will never be less than 3 vertices, we still need to
    // make calls to ngettext because the pluralization may be different
    // for various numbers >=3.  The singular form is used as the index.
    if (side->flatsided == false )
	return g_strdup_printf (ngettext("<b>Star</b> with %d vertex",
				         "<b>Star</b> with %d vertices",
					 star->sides), star->sides);
    else
        return g_strdup_printf (ngettext("<b>Polygon</b> with %d vertex",
				         "<b>Polygon</b> with %d vertices",
					 star->sides), star->sides);
}
***/

void
box3d_side_position_set (Box3DSide *side) {
    box3d_side_set_shape (SP_SHAPE (side));

    /* This call is responsible for live update of the sides during the initial drag */
    SP_OBJECT(side)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
box3d_side_set_shape (SPShape *shape)
{
    //g_print ("box3d_side_set_shape\n");
    Box3DSide *side = SP_BOX3D_SIDE (shape);
    if (!SP_OBJECT_DOCUMENT(side)->root) {
        // avoid a warning caused by sp_document_height() (which is called from sp_item_i2d_affine() below)
        // when reading a file containing 3D boxes
        return;
    }

    if (!SP_IS_BOX3D(SP_OBJECT(side)->parent)) {
        g_warning ("Parent of 3D box side is not a 3D box.\n");
        /**
        g_print ("Removing the inkscape:box3dside attribute and returning from box3d_side_set_shape().\n");
        SP_OBJECT_REPR (shape)->setAttribute("sodipodi:type", NULL);
        SP_OBJECT_REPR (shape)->setAttribute("inkscape:box3dside", NULL);
        **/
        return;
    }

    Persp3D *persp = box3d_side_perspective(side);
    //g_return_if_fail (persp != NULL);
    if (!persp) {
        //g_warning ("persp != NULL in box3d_side_set_shape failed!\n");
        //persp = SP_OBJECT_DOCUMENT(side)->current_persp3d;
        return;
    }

    SPCurve *c = sp_curve_new ();
    // TODO: Draw the correct quadrangle here
    //       To do this, determine the perspective of the box, the orientation of the side (e.g., XY-FRONT)
    //       compute the coordinates of the corners in P^3, project them onto the canvas, and draw the
    //       resulting path.

    std::vector<Proj::Pt3> corners = box3d_side_corners (side);

    NR::Matrix const i2d (sp_item_i2d_affine (SP_ITEM(shape)));

    // FIXME: This can better be implemented by using box3d_get_corner
    sp_curve_moveto (c, persp->tmat.image(corners[0]).affine() * i2d);
    sp_curve_lineto (c, persp->tmat.image(corners[1]).affine() * i2d);
    sp_curve_lineto (c, persp->tmat.image(corners[2]).affine() * i2d);
    sp_curve_lineto (c, persp->tmat.image(corners[3]).affine() * i2d);

    sp_curve_closepath (c);
    //sp_shape_perform_path_effect(c, SP_SHAPE (side));
    sp_shape_set_curve_insync (SP_SHAPE (side), c, TRUE);
    sp_curve_unref (c);
}

void
box3d_side_apply_style (Box3DSide *side) {
    Inkscape::XML::Node *repr_face = SP_OBJECT_REPR(SP_OBJECT(side));

    gchar *descr = g_strconcat ("desktop.", box3d_side_axes_string (side), NULL);
    const gchar * cur_style = prefs_get_string_attribute(descr, "style");
    g_free (descr);    
    
    SPDesktop *desktop = inkscape_active_desktop();
    bool use_current = prefs_get_int_attribute("tools.shapes.3dbox", "usecurrent", 0);
    if (use_current && cur_style !=NULL) {
        /* use last used style */
        repr_face->setAttribute("style", cur_style);
    } else {
        /* use default style */
        GString *pstring = g_string_new("");
        g_string_printf (pstring, "tools.shapes.3dbox.%s", box3d_side_axes_string(side));
        sp_desktop_apply_style_tool (desktop, repr_face, pstring->str, false);
    }
}

gchar *
box3d_side_axes_string(Box3DSide *side)
{
    GString *pstring = g_string_new("");
    g_string_printf (pstring, "%s", Box3D::string_from_axes ((Box3D::Axis) (side->dir1 ^ side->dir2)));
    switch ((Box3D::Axis) (side->dir1 ^ side->dir2)) {
        case Box3D::XY:
            g_string_append_printf (pstring, (side->front_or_rear == Box3D::FRONT) ? "front" : "rear");
            break;
        case Box3D::XZ:
            g_string_append_printf (pstring, (side->front_or_rear == Box3D::FRONT) ? "top" : "bottom");
            break;
        case Box3D::YZ:
            g_string_append_printf (pstring, (side->front_or_rear == Box3D::FRONT) ? "right" : "left");
            break;
        default:
            break;
    }
    return pstring->str;
}

static Proj::Pt3
box3d_side_corner (Box3DSide *side, guint index) {
    SPBox3D *box = SP_BOX3D(SP_OBJECT_PARENT(side));
    return Proj::Pt3 ((index & 0x1) ? box->orig_corner7[Proj::X] : box->orig_corner0[Proj::X],
                      (index & 0x2) ? box->orig_corner7[Proj::Y] : box->orig_corner0[Proj::Y],
                      (index & 0x4) ? box->orig_corner7[Proj::Z] : box->orig_corner0[Proj::Z],
                      1.0);
}

static std::vector<Proj::Pt3>
box3d_side_corners (Box3DSide *side) {
    std::vector<Proj::Pt3> corners;
    Box3D::Axis orth = Box3D::third_axis_direction (side->dir1, side->dir2);
    unsigned int i0 = (side->front_or_rear ? orth : 0);
    unsigned int i1 = i0 ^ side->dir1;
    unsigned int i2 = i0 ^ side->dir1 ^ side->dir2;
    unsigned int i3 = i0 ^ side->dir2;

    corners.push_back (box3d_side_corner (side, i0));
    corners.push_back (box3d_side_corner (side, i1));
    corners.push_back (box3d_side_corner (side, i2));
    corners.push_back (box3d_side_corner (side, i3));
    return corners;
}

/*
static gint
box3d_side_descr_to_id (gchar const *descr)
{
    if (!strcmp (descr, "XYrear")) { return 5; }
    if (!strcmp (descr, "XYfront")) { return 2; }
    if (!strcmp (descr, "XZbottom")) { return 1; }
    if (!strcmp (descr, "XZtop")) { return 4; }
    if (!strcmp (descr, "YZleft")) { return 3; }
    if (!strcmp (descr, "YZright")) { return 0; }

    g_warning ("Invalid description of 3D box face.\n");
    g_print ("         (description is: %s)\n", descr);
    return -1;
}
*/

Persp3D *
box3d_side_perspective(Box3DSide *side) {
    return SP_BOX3D(SP_OBJECT(side)->parent)->persp_ref->getObject();
}

Inkscape::XML::Node *
box3d_side_convert_to_path(Box3DSide *side) {
    // TODO: Copy over all important attributes (see sp_selected_item_to_curved_repr() for an example)
    SPDocument *doc = SP_OBJECT_DOCUMENT(side);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);

    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
    repr->setAttribute("d", SP_OBJECT_REPR(side)->attribute("d"));
    repr->setAttribute("style", SP_OBJECT_REPR(side)->attribute("style"));

    return repr;
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
