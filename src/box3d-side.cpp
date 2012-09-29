/*
 * 3D box face implementation
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *   Abhishek Sharma
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
#include "preferences.h"
#include "desktop-style.h"
#include "box3d.h"

struct SPPathClass;

static void box3d_side_class_init (Box3DSideClass *klass);
static void box3d_side_init (Box3DSide *side);

static void box3d_side_build (SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static Inkscape::XML::Node *box3d_side_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void box3d_side_set (SPObject *object, unsigned int key, const gchar *value);
static void box3d_side_update (SPObject *object, SPCtx *ctx, guint flags);

static void box3d_side_set_shape (SPShape *shape);

static void box3d_side_compute_corner_ids(Box3DSide *side, unsigned int corners[4]);

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

static void box3d_side_class_init(Box3DSideClass *klass)
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);
    SPShapeClass *shape_class = reinterpret_cast<SPShapeClass *>(klass);

    parent_class = (SPShapeClass *)g_type_class_ref (SP_TYPE_SHAPE);

    sp_object_class->build = box3d_side_build;
    sp_object_class->write = box3d_side_write;
    sp_object_class->set = box3d_side_set;
    sp_object_class->update = box3d_side_update;

    //shape_class->set_shape = box3d_side_set_shape;
}

CBox3DSide::CBox3DSide(Box3DSide* box3dside) : CPolygon(box3dside) {
	this->spbox3dside = box3dside;
}

CBox3DSide::~CBox3DSide() {
}

static void
box3d_side_init (Box3DSide * side)
{
	side->cbox3dside = new CBox3DSide(side);
	side->cpolygon = side->cbox3dside;
	side->cshape = side->cbox3dside;
	side->clpeitem = side->cbox3dside;
	side->citem = side->cbox3dside;
	side->cobject = side->cbox3dside;

    side->dir1 = Box3D::NONE;
    side->dir2 = Box3D::NONE;
    side->front_or_rear = Box3D::FRONT;
}

void CBox3DSide::onBuild(SPDocument * document, Inkscape::XML::Node * repr) {
	Box3DSide* object = this->spbox3dside;

    CPolygon::onBuild(document, repr);

    object->readAttr( "inkscape:box3dsidetype" );
}

// CPPIFY: remove
static void box3d_side_build(SPObject * object, SPDocument * document, Inkscape::XML::Node * repr)
{
	((Box3DSide*)object)->cbox3dside->onBuild(document, repr);
}

Inkscape::XML::Node* CBox3DSide::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	Box3DSide* object = this->spbox3dside;
    Box3DSide *side = object;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        // this is where we end up when saving as plain SVG (also in other circumstances?)
        // thus we don' set "sodipodi:type" so that the box is only saved as an ordinary svg:path
        repr = xml_doc->createElement("svg:path");
    }

    if (flags & SP_OBJECT_WRITE_EXT) {
        sp_repr_set_int(repr, "inkscape:box3dsidetype", side->dir1 ^ side->dir2 ^ side->front_or_rear);
    }

    static_cast<SPShape *>(object)->setShape();

    /* Duplicate the path */
    SPCurve const *curve = ((SPShape *) object)->_curve;
    //Nulls might be possible if this called iteratively
    if ( !curve ) {
        return NULL;
    }
    char *d = sp_svg_write_path ( curve->get_pathvector() );
    repr->setAttribute("d", d);
    g_free (d);

    CPolygon::onWrite(xml_doc, repr, flags);

    return repr;
}

// CPPIFY: remove
static Inkscape::XML::Node *
box3d_side_write (SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((Box3DSide*)object)->cbox3dside->onWrite(xml_doc, repr, flags);
}

void CBox3DSide::onSet(unsigned int key, const gchar* value) {
	Box3DSide* object = this->spbox3dside;
    Box3DSide *side = object;

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
        CPolygon::onSet(key, value);
        break;
    }
}

// CPPIFY: remove
static void
box3d_side_set (SPObject *object, unsigned int key, const gchar *value)
{
	((Box3DSide*)object)->cbox3dside->onSet(key, value);
}

void CBox3DSide::onUpdate(SPCtx* ctx, guint flags) {
	Box3DSide* object = this->spbox3dside;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
    }

    if (flags & (SP_OBJECT_MODIFIED_FLAG |
                 SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        static_cast<SPShape *>(object)->setShape ();
    }

    CPolygon::onUpdate(ctx, flags);
}

// CPPIFY: remove
static void
box3d_side_update (SPObject *object, SPCtx *ctx, guint flags)
{
	((Box3DSide*)object)->cbox3dside->onUpdate(ctx, flags);
}

/* Create a new Box3DSide and append it to the parent box */
Box3DSide * Box3DSide::createBox3DSide(SPBox3D *box)
{
	Box3DSide *box3d_side = 0;
	Inkscape::XML::Document *xml_doc = box->document->rdoc;
	Inkscape::XML::Node *repr_side = xml_doc->createElement("svg:path");
	repr_side->setAttribute("sodipodi:type", "inkscape:box3dside");
	box3d_side = static_cast<Box3DSide *>(box->appendChildRepr(repr_side));
	return box3d_side;
}

/*
 * Function which return the type attribute for Box3D. 
 * Acts as a replacement for directly accessing the XML Tree directly.
 */
int Box3DSide::getFaceId()
{
	    return this->getIntAttribute("inkscape:box3dsidetype", -1);
}

void
box3d_side_position_set (Box3DSide *side) {
    box3d_side_set_shape (SP_SHAPE (side));

    // This call is responsible for live update of the sides during the initial drag
    side->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void CBox3DSide::onSetShape() {
	Box3DSide* shape = this->spbox3dside;
    Box3DSide *side = shape;

    if (!side->document->getRoot()) {
        // avoid a warning caused by sp_document_height() (which is called from sp_item_i2d_affine() below)
        // when reading a file containing 3D boxes
        return;
    }

    SPObject *parent = side->parent;
    if (!SP_IS_BOX3D(parent)) {
        g_warning ("Parent of 3D box side is not a 3D box.\n");
        return;
    }
    SPBox3D *box = SP_BOX3D(parent);

    Persp3D *persp = box3d_side_perspective(side);
    if (!persp) {
        return;
    }

    // TODO: Draw the correct quadrangle here
    //       To do this, determine the perspective of the box, the orientation of the side (e.g., XY-FRONT)
    //       compute the coordinates of the corners in P^3, project them onto the canvas, and draw the
    //       resulting path.

    unsigned int corners[4];
    box3d_side_compute_corner_ids(side, corners);

    SPCurve *c = new SPCurve();

    if (!box3d_get_corner_screen(box, corners[0]).isFinite() ||
        !box3d_get_corner_screen(box, corners[1]).isFinite() ||
        !box3d_get_corner_screen(box, corners[2]).isFinite() ||
        !box3d_get_corner_screen(box, corners[3]).isFinite() )
    {
        g_warning ("Trying to draw a 3D box side with invalid coordinates.\n");
        return;
    }

    c->moveto(box3d_get_corner_screen(box, corners[0]));
    c->lineto(box3d_get_corner_screen(box, corners[1]));
    c->lineto(box3d_get_corner_screen(box, corners[2]));
    c->lineto(box3d_get_corner_screen(box, corners[3]));
    c->closepath();

    /* Reset the shape'scurve to the "original_curve"
     * This is very important for LPEs to work properly! (the bbox might be recalculated depending on the curve in shape)*/
    shape->setCurveInsync( c, TRUE);
    if (sp_lpe_item_has_path_effect(SP_LPE_ITEM(shape)) && sp_lpe_item_path_effects_enabled(SP_LPE_ITEM(shape))) {
        SPCurve *c_lpe = c->copy();
        bool success = sp_lpe_item_perform_path_effect(SP_LPE_ITEM (shape), c_lpe);
        if (success) {
            shape->setCurveInsync( c_lpe, TRUE);
        }
        c_lpe->unref();
    }
    c->unref();
}

// CPPIFY: remove
void
box3d_side_set_shape (SPShape *shape)
{
	((Box3DSide*)shape)->cbox3dside->onSetShape();
}

gchar *box3d_side_axes_string(Box3DSide *side)
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

static void
box3d_side_compute_corner_ids(Box3DSide *side, unsigned int corners[4]) {
    Box3D::Axis orth = Box3D::third_axis_direction (side->dir1, side->dir2);

    corners[0] = (side->front_or_rear ? orth : 0);
    corners[1] = corners[0] ^ side->dir1;
    corners[2] = corners[0] ^ side->dir1 ^ side->dir2;
    corners[3] = corners[0] ^ side->dir2;
}

Persp3D *
box3d_side_perspective(Box3DSide *side) {
    return SP_BOX3D(side->parent)->persp_ref->getObject();
}

Inkscape::XML::Node *box3d_side_convert_to_path(Box3DSide *side) {
    // TODO: Copy over all important attributes (see sp_selected_item_to_curved_repr() for an example)
    SPDocument *doc = side->document;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
    repr->setAttribute("d", side->getAttribute("d"));
    repr->setAttribute("style", side->getAttribute("style"));

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
