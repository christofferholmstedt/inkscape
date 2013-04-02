/** \file
 * SVG <feConvolveMatrix> implementation.
 *
 */
/*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <vector>
#include "attributes.h"
#include "svg/svg.h"
#include "filters/convolvematrix.h"
#include "helper-fns.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-convolve-matrix.h"

#include "sp-factory.h"

namespace {
	SPObject* createConvolveMatrix() {
		return new SPFeConvolveMatrix();
	}

	bool convolveMatrixRegistered = SPFactory::instance().registerObject("svg:feConvolveMatrix", createConvolveMatrix);
}

/* FeConvolveMatrix base class */
G_DEFINE_TYPE(SPFeConvolveMatrix, sp_feConvolveMatrix, G_TYPE_OBJECT);

static void
sp_feConvolveMatrix_class_init(SPFeConvolveMatrixClass *klass)
{
}

CFeConvolveMatrix::CFeConvolveMatrix(SPFeConvolveMatrix* matrix) : CFilterPrimitive(matrix) {
	this->spfeconvolvematrix = matrix;
}

CFeConvolveMatrix::~CFeConvolveMatrix() {
}

SPFeConvolveMatrix::SPFeConvolveMatrix() : SPFilterPrimitive() {
	SPFeConvolveMatrix* feConvolveMatrix = this;

	feConvolveMatrix->cfeconvolvematrix = new CFeConvolveMatrix(feConvolveMatrix);
	feConvolveMatrix->typeHierarchy.insert(typeid(SPFeConvolveMatrix));

	delete feConvolveMatrix->cfilterprimitive;
	feConvolveMatrix->cfilterprimitive = feConvolveMatrix->cfeconvolvematrix;
	feConvolveMatrix->cobject = feConvolveMatrix->cfeconvolvematrix;

	feConvolveMatrix->bias = 0;
	feConvolveMatrix->divisorIsSet = 0;
	feConvolveMatrix->divisor = 0;

    //Setting default values:
    feConvolveMatrix->order.set("3 3");
    feConvolveMatrix->targetX = 1;
    feConvolveMatrix->targetY = 1;
    feConvolveMatrix->edgeMode = Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE;
    feConvolveMatrix->preserveAlpha = false;

    //some helper variables:
    feConvolveMatrix->targetXIsSet = false;
    feConvolveMatrix->targetYIsSet = false;
    feConvolveMatrix->kernelMatrixIsSet = false;
}

static void
sp_feConvolveMatrix_init(SPFeConvolveMatrix *feConvolveMatrix)
{
	new (feConvolveMatrix) SPFeConvolveMatrix();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeConvolveMatrix variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeConvolveMatrix::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);

	SPFeConvolveMatrix* object = this->spfeconvolvematrix;

	/*LOAD ATTRIBUTES FROM REPR HERE*/
	object->readAttr( "order" );
	object->readAttr( "kernelMatrix" );
	object->readAttr( "divisor" );
	object->readAttr( "bias" );
	object->readAttr( "targetX" );
	object->readAttr( "targetY" );
	object->readAttr( "edgeMode" );
	object->readAttr( "kernelUnitLength" );
	object->readAttr( "preserveAlpha" );
}

/**
 * Drops any allocated memory.
 */
void CFeConvolveMatrix::release() {
	CFilterPrimitive::release();
}

static Inkscape::Filters::FilterConvolveMatrixEdgeMode sp_feConvolveMatrix_read_edgeMode(gchar const *value){
    if (!value) return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE; //duplicate is default
    switch(value[0]){
        case 'd':
            if (strncmp(value, "duplicate", 9) == 0) return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE;
            break;
        case 'w':
            if (strncmp(value, "wrap", 4) == 0) return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_WRAP;
            break;
        case 'n':
            if (strncmp(value, "none", 4) == 0) return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_NONE;
            break;
    }
    return Inkscape::Filters::CONVOLVEMATRIX_EDGEMODE_DUPLICATE; //duplicate is default
}

/**
 * Sets a specific value in the SPFeConvolveMatrix.
 */
void CFeConvolveMatrix::set(unsigned int key, gchar const *value) {
	SPFeConvolveMatrix* object = this->spfeconvolvematrix;

    SPFeConvolveMatrix *feConvolveMatrix = SP_FECONVOLVEMATRIX(object);
    (void)feConvolveMatrix;
    double read_num;
    int read_int;
    bool read_bool;
    Inkscape::Filters::FilterConvolveMatrixEdgeMode read_mode;
   
    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_ATTR_ORDER:
            feConvolveMatrix->order.set(value);
            //From SVG spec: If <orderY> is not provided, it defaults to <orderX>.
            if (feConvolveMatrix->order.optNumIsSet() == false)
                feConvolveMatrix->order.setOptNumber(feConvolveMatrix->order.getNumber());
            if (feConvolveMatrix->targetXIsSet == false) feConvolveMatrix->targetX = (int) floor(feConvolveMatrix->order.getNumber()/2);
            if (feConvolveMatrix->targetYIsSet == false) feConvolveMatrix->targetY = (int) floor(feConvolveMatrix->order.getOptNumber()/2);
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_KERNELMATRIX:
            if (value){
                feConvolveMatrix->kernelMatrixIsSet = true;
                feConvolveMatrix->kernelMatrix = helperfns_read_vector(value);
                if (! feConvolveMatrix->divisorIsSet) {
                    feConvolveMatrix->divisor = 0;
                    for (unsigned int i = 0; i< feConvolveMatrix->kernelMatrix.size(); i++)
                        feConvolveMatrix->divisor += feConvolveMatrix->kernelMatrix[i];
                    if (feConvolveMatrix->divisor == 0) feConvolveMatrix->divisor = 1;
                }
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            } else {
                g_warning("For feConvolveMatrix you MUST pass a kernelMatrix parameter!");
            }
            break;
        case SP_ATTR_DIVISOR:
            if (value) { 
                read_num = helperfns_read_number(value);
                if (read_num == 0) {
                    // This should actually be an error, but given our UI it is more useful to simply set divisor to the default.
                    if (feConvolveMatrix->kernelMatrixIsSet) {
                        for (unsigned int i = 0; i< feConvolveMatrix->kernelMatrix.size(); i++)
                            read_num += feConvolveMatrix->kernelMatrix[i];
                    }
                    if (read_num == 0) read_num = 1;
                    if (feConvolveMatrix->divisorIsSet || feConvolveMatrix->divisor!=read_num) {
                        feConvolveMatrix->divisorIsSet = false;
                        feConvolveMatrix->divisor = read_num;
                        object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
                    }
                } else if (!feConvolveMatrix->divisorIsSet || feConvolveMatrix->divisor!=read_num) {
                    feConvolveMatrix->divisorIsSet = true;
                    feConvolveMatrix->divisor = read_num;
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        case SP_ATTR_BIAS:
            read_num = 0;
            if (value) read_num = helperfns_read_number(value);
            if (read_num != feConvolveMatrix->bias){
                feConvolveMatrix->bias = read_num;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_TARGETX:
            if (value) {
                read_int = (int) helperfns_read_number(value);
                if (read_int < 0 || read_int > feConvolveMatrix->order.getNumber()){
                    g_warning("targetX must be a value between 0 and orderX! Assuming floor(orderX/2) as default value.");
                    read_int = (int) floor(feConvolveMatrix->order.getNumber()/2.0);
                }
                feConvolveMatrix->targetXIsSet = true;
                if (read_int != feConvolveMatrix->targetX){
                    feConvolveMatrix->targetX = read_int;
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        case SP_ATTR_TARGETY:
            if (value) {
                read_int = (int) helperfns_read_number(value);
                if (read_int < 0 || read_int > feConvolveMatrix->order.getOptNumber()){
                    g_warning("targetY must be a value between 0 and orderY! Assuming floor(orderY/2) as default value.");
                    read_int = (int) floor(feConvolveMatrix->order.getOptNumber()/2.0);
                }
                feConvolveMatrix->targetYIsSet = true;
                if (read_int != feConvolveMatrix->targetY){
                    feConvolveMatrix->targetY = read_int;
                    object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
                }
            }
            break;
        case SP_ATTR_EDGEMODE:
            read_mode = sp_feConvolveMatrix_read_edgeMode(value);
            if (read_mode != feConvolveMatrix->edgeMode){
                feConvolveMatrix->edgeMode = read_mode;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        case SP_ATTR_KERNELUNITLENGTH:
            feConvolveMatrix->kernelUnitLength.set(value);
            //From SVG spec: If the <dy> value is not specified, it defaults to the same value as <dx>.
            if (feConvolveMatrix->kernelUnitLength.optNumIsSet() == false)
                feConvolveMatrix->kernelUnitLength.setOptNumber(feConvolveMatrix->kernelUnitLength.getNumber());
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_PRESERVEALPHA:
            read_bool = helperfns_read_bool(value, false);
            if (read_bool != feConvolveMatrix->preserveAlpha){
                feConvolveMatrix->preserveAlpha = read_bool;
                object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        default:
        	CFilterPrimitive::set(key, value);
            break;
    }

}

/**
 * Receives update notifications.
 */
void CFeConvolveMatrix::update(SPCtx *ctx, guint flags) {
	SPFeConvolveMatrix* object = this->spfeconvolvematrix;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeConvolveMatrix::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeConvolveMatrix* object = this->spfeconvolvematrix;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }


    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeConvolveMatrix::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeConvolveMatrix* primitive = this->spfeconvolvematrix;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeConvolveMatrix *sp_convolve = SP_FECONVOLVEMATRIX(primitive);

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_CONVOLVEMATRIX);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterConvolveMatrix *nr_convolve = dynamic_cast<Inkscape::Filters::FilterConvolveMatrix*>(nr_primitive);
    g_assert(nr_convolve != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_convolve->set_targetX(sp_convolve->targetX);
    nr_convolve->set_targetY(sp_convolve->targetY);
    nr_convolve->set_orderX( (int)sp_convolve->order.getNumber() );
    nr_convolve->set_orderY( (int)sp_convolve->order.getOptNumber() );
    nr_convolve->set_kernelMatrix(sp_convolve->kernelMatrix);
    nr_convolve->set_divisor(sp_convolve->divisor);
    nr_convolve->set_bias(sp_convolve->bias);
    nr_convolve->set_preserveAlpha(sp_convolve->preserveAlpha);
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
