#ifndef __SP_FILTER_PRIMITIVE_H__
#define __SP_FILTER_PRIMITIVE_H__

/** \file
 * Document level base class for all SVG filter primitives.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"
#include "svg/svg-length.h"

#define SP_TYPE_FILTER_PRIMITIVE (sp_filter_primitive_get_type ())
#define SP_FILTER_PRIMITIVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FILTER_PRIMITIVE, SPFilterPrimitive))
#define SP_FILTER_PRIMITIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FILTER_PRIMITIVE, SPFilterPrimitiveClass))
#define SP_IS_FILTER_PRIMITIVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FILTER_PRIMITIVE))
#define SP_IS_FILTER_PRIMITIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FILTER_PRIMITIVE))

namespace Inkscape {
namespace Filters {
class Filter;
class FilterPrimitive;
} }

class CFilterPrimitive;

class SPFilterPrimitive : public SPObject {
public:
	CFilterPrimitive* cfilterprimitive;

    int image_in, image_out;

    /* filter primitive subregion */
    SVGLength x, y, height, width;
};

struct SPFilterPrimitiveClass {
    SPObjectClass sp_object_class;
};

class CFilterPrimitive : public CObject {
public:
	CFilterPrimitive(SPFilterPrimitive* fp);
	virtual ~CFilterPrimitive();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();

	virtual void onSet(unsigned int key, const gchar* value);

	virtual void onUpdate(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

	virtual void onBuildRenderer(Inkscape::Filters::Filter* filter);

private:
	SPFilterPrimitive* spfilterprimitive;
};


GType sp_filter_primitive_get_type (void);

/* Common initialization for filter primitives */
void sp_filter_primitive_renderer_common(SPFilterPrimitive *sp_prim, Inkscape::Filters::FilterPrimitive *nr_prim);

int sp_filter_primitive_name_previous_out(SPFilterPrimitive *prim);
int sp_filter_primitive_read_in(SPFilterPrimitive *prim, gchar const *name);
int sp_filter_primitive_read_result(SPFilterPrimitive *prim, gchar const *name);

#endif
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
