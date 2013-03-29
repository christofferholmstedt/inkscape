/** \file
 * SVG <feColorMatrix> implementation.
 *
 */
/*
 * Authors:
 *   Felipe Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Felipe C. da S. Sanches
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include "attributes.h"
#include "svg/svg.h"
#include "colormatrix.h"
#include "xml/repr.h"
#include "helper-fns.h"

#include "display/nr-filter.h"
#include "display/nr-filter-colormatrix.h"

/* FeColorMatrix base class */

static void sp_feColorMatrix_class_init(SPFeColorMatrixClass *klass);
static void sp_feColorMatrix_init(SPFeColorMatrix *feColorMatrix);

static void sp_feColorMatrix_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_feColorMatrix_release(SPObject *object);
static void sp_feColorMatrix_set(SPObject *object, unsigned int key, gchar const *value);
static void sp_feColorMatrix_update(SPObject *object, SPCtx *ctx, guint flags);
static Inkscape::XML::Node *sp_feColorMatrix_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_feColorMatrix_build_renderer(SPFilterPrimitive *primitive, Inkscape::Filters::Filter *filter);

static SPFilterPrimitiveClass *feColorMatrix_parent_class;

GType
sp_feColorMatrix_get_type()
{
    static GType feColorMatrix_type = 0;

    if (!feColorMatrix_type) {
        GTypeInfo feColorMatrix_info = {
            sizeof(SPFeColorMatrixClass),
            NULL, NULL,
            (GClassInitFunc) sp_feColorMatrix_class_init,
            NULL, NULL,
            sizeof(SPFeColorMatrix),
            16,
            (GInstanceInitFunc) sp_feColorMatrix_init,
            NULL,    /* value_table */
        };
        feColorMatrix_type = g_type_register_static(SP_TYPE_FILTER_PRIMITIVE, "SPFeColorMatrix", &feColorMatrix_info, (GTypeFlags)0);
    }
    return feColorMatrix_type;
}

static void
sp_feColorMatrix_class_init(SPFeColorMatrixClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *)klass;
    SPFilterPrimitiveClass *sp_primitive_class = (SPFilterPrimitiveClass *)klass;

    feColorMatrix_parent_class = (SPFilterPrimitiveClass*)g_type_class_peek_parent(klass);

    //sp_object_class->build = sp_feColorMatrix_build;
    sp_object_class->release = sp_feColorMatrix_release;
    sp_object_class->write = sp_feColorMatrix_write;
    sp_object_class->set = sp_feColorMatrix_set;
    sp_object_class->update = sp_feColorMatrix_update;
    //sp_primitive_class->build_renderer = sp_feColorMatrix_build_renderer;
}

CFeColorMatrix::CFeColorMatrix(SPFeColorMatrix* matrix) : CFilterPrimitive(matrix) {
	this->spfecolormatrix = matrix;
}

CFeColorMatrix::~CFeColorMatrix() {
}

static void
sp_feColorMatrix_init(SPFeColorMatrix *feColorMatrix)
{
	feColorMatrix->cfecolormatrix = new CFeColorMatrix(feColorMatrix);
	feColorMatrix->cfilterprimitive = feColorMatrix->cfecolormatrix;
	feColorMatrix->cobject = feColorMatrix->cfecolormatrix;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeColorMatrix variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
//static void
//sp_feColorMatrix_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
//{
////    if (((SPObjectClass *) feColorMatrix_parent_class)->build) {
////        ((SPObjectClass *) feColorMatrix_parent_class)->build(object, document, repr);
////    }
//
//    /*LOAD ATTRIBUTES FROM REPR HERE*/
//    object->readAttr( "type" );
//    object->readAttr( "values" );
//}

void CFeColorMatrix::onBuild(SPDocument *document, Inkscape::XML::Node *repr) {
	//    if (((SPObjectClass *) feColorMatrix_parent_class)->build) {
	//        ((SPObjectClass *) feColorMatrix_parent_class)->build(object, document, repr);
	//    }
	CFilterPrimitive::onBuild(document, repr);

	SPFeColorMatrix* object = this->spfecolormatrix;

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "type" );
	object->readAttr( "values" );
}

/**
 * Drops any allocated memory.
 */
static void
sp_feColorMatrix_release(SPObject *object)
{
//    if (((SPObjectClass *) feColorMatrix_parent_class)->release)
//        ((SPObjectClass *) feColorMatrix_parent_class)->release(object);
	((SPFeColorMatrix*)object)->cfecolormatrix->onRelease();
}

void CFeColorMatrix::onRelease() {
	CFilterPrimitive::onRelease();
}

static Inkscape::Filters::FilterColorMatrixType sp_feColorMatrix_read_type(gchar const *value){
    if (!value) return Inkscape::Filters::COLORMATRIX_MATRIX; //matrix is default
    switch(value[0]){
        case 'm':
            if (strcmp(value, "matrix") == 0) return Inkscape::Filters::COLORMATRIX_MATRIX;
            break;
        case 's':
            if (strcmp(value, "saturate") == 0) return Inkscape::Filters::COLORMATRIX_SATURATE;
            break;
        case 'h':
            if (strcmp(value, "hueRotate") == 0) return Inkscape::Filters::COLORMATRIX_HUEROTATE;
            break;
        case 'l':
            if (strcmp(value, "luminanceToAlpha") == 0) return Inkscape::Filters::COLORMATRIX_LUMINANCETOALPHA;
            break;
    }
    return Inkscape::Filters::COLORMATRIX_MATRIX; //matrix is default
}

/**
 * Sets a specific value in the SPFeColorMatrix.
 */
static void
sp_feColorMatrix_set(SPObject *object, unsigned int key, gchar const *str)
{
//    SPFeColorMatrix *feColorMatrix = SP_FECOLORMATRIX(object);
//    (void)feColorMatrix;
//
//    Inkscape::Filters::FilterColorMatrixType read_type;
//	/*DEAL WITH SETTING ATTRIBUTES HERE*/
//    switch(key) {
//        case SP_ATTR_TYPE:
//            read_type = sp_feColorMatrix_read_type(str);
//            if (feColorMatrix->type != read_type){
//                feColorMatrix->type = read_type;
//                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
//            }
//            break;
//        case SP_ATTR_VALUES:
//            if (str){
//                feColorMatrix->values = helperfns_read_vector(str);
//                feColorMatrix->value = helperfns_read_number(str, HELPERFNS_NO_WARNING);
//                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
//            }
//            break;
//        default:
//            if (((SPObjectClass *) feColorMatrix_parent_class)->set)
//                ((SPObjectClass *) feColorMatrix_parent_class)->set(object, key, str);
//            break;
//    }
	((SPFeColorMatrix*)object)->cfecolormatrix->onSet(key, str);
}

void CFeColorMatrix::onSet(unsigned int key, gchar const *str) {
	SPFeColorMatrix* object = this->spfecolormatrix;

    SPFeColorMatrix *feColorMatrix = SP_FECOLORMATRIX(object);
    (void)feColorMatrix;

    Inkscape::Filters::FilterColorMatrixType read_type;
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
    switch(key) {
        case SP_ATTR_TYPE:
            read_type = sp_feColorMatrix_read_type(str);
            if (feColorMatrix->type != read_type){
                feColorMatrix->type = read_type;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_VALUES:
            if (str){
                feColorMatrix->values = helperfns_read_vector(str);
                feColorMatrix->value = helperfns_read_number(str, HELPERFNS_NO_WARNING);
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        default:
//            if (((SPObjectClass *) feColorMatrix_parent_class)->set)
//                ((SPObjectClass *) feColorMatrix_parent_class)->set(object, key, str);
        	CFilterPrimitive::onSet(key, str);
            break;
    }
}

/**
 * Receives update notifications.
 */
static void
sp_feColorMatrix_update(SPObject *object, SPCtx *ctx, guint flags)
{
//    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
//                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
//
//        /* do something to trigger redisplay, updates? */
//
//    }
//
//    if (((SPObjectClass *) feColorMatrix_parent_class)->update) {
//        ((SPObjectClass *) feColorMatrix_parent_class)->update(object, ctx, flags);
//    }
	((SPFeColorMatrix*)object)->cfecolormatrix->onUpdate(ctx, flags);
}

void CFeColorMatrix::onUpdate(SPCtx *ctx, guint flags) {
	SPFeColorMatrix* object = this->spfecolormatrix;

	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

//    if (((SPObjectClass *) feColorMatrix_parent_class)->update) {
//        ((SPObjectClass *) feColorMatrix_parent_class)->update(object, ctx, flags);
//    }
	CFilterPrimitive::onUpdate(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *
sp_feColorMatrix_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
//    /* TODO: Don't just clone, but create a new repr node and write all
//     * relevant values into it */
//    if (!repr) {
//        repr = object->getRepr()->duplicate(doc);
//    }
//
//    if (((SPObjectClass *) feColorMatrix_parent_class)->write) {
//        ((SPObjectClass *) feColorMatrix_parent_class)->write(object, doc, repr, flags);
//    }
//
//    return repr;
	return ((SPFeColorMatrix*)object)->cfecolormatrix->onWrite(doc, repr, flags);
}

Inkscape::XML::Node* CFeColorMatrix::onWrite(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeColorMatrix* object = this->spfecolormatrix;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

//    if (((SPObjectClass *) feColorMatrix_parent_class)->write) {
//        ((SPObjectClass *) feColorMatrix_parent_class)->write(object, doc, repr, flags);
//    }
    CFilterPrimitive::onWrite(doc, repr, flags);

    return repr;
}

static void sp_feColorMatrix_build_renderer(SPFilterPrimitive *primitive, Inkscape::Filters::Filter *filter) {
//    g_assert(primitive != NULL);
//    g_assert(filter != NULL);
//
//    SPFeColorMatrix *sp_colormatrix = SP_FECOLORMATRIX(primitive);
//
//    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_COLORMATRIX);
//    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
//    Inkscape::Filters::FilterColorMatrix *nr_colormatrix = dynamic_cast<Inkscape::Filters::FilterColorMatrix*>(nr_primitive);
//    g_assert(nr_colormatrix != NULL);
//
//    sp_filter_primitive_renderer_common(primitive, nr_primitive);
//    nr_colormatrix->set_type(sp_colormatrix->type);
//    nr_colormatrix->set_value(sp_colormatrix->value);
//    nr_colormatrix->set_values(sp_colormatrix->values);
	((SPFeColorMatrix*)primitive)->cfecolormatrix->onBuildRenderer(filter);
}

void CFeColorMatrix::onBuildRenderer(Inkscape::Filters::Filter* filter) {
	SPFeColorMatrix* primitive = this->spfecolormatrix;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeColorMatrix *sp_colormatrix = SP_FECOLORMATRIX(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_COLORMATRIX);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterColorMatrix *nr_colormatrix = dynamic_cast<Inkscape::Filters::FilterColorMatrix*>(nr_primitive);
    g_assert(nr_colormatrix != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);
    nr_colormatrix->set_type(sp_colormatrix->type);
    nr_colormatrix->set_value(sp_colormatrix->value);
    nr_colormatrix->set_values(sp_colormatrix->values);
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
