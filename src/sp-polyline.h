#ifndef SEEN_SP_POLYLINE_H
#define SEEN_SP_POLYLINE_H

#include "sp-shape.h"



#define SP_TYPE_POLYLINE            (sp_polyline_get_type ())
#define SP_POLYLINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYLINE, SPPolyLine))
#define SP_POLYLINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYLINE, SPPolyLineClass))
#define SP_IS_POLYLINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYLINE))
#define SP_IS_POLYLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYLINE))

class SPPolyLine;
class SPPolyLineClass;
class CPolyLine;

GType sp_polyline_get_type (void) G_GNUC_CONST;

class SPPolyLine : public SPShape {
public:
	CPolyLine* cpolyline;

private:
    friend class SPPolyLineClass;
};

class SPPolyLineClass {
public:
    SPShapeClass parent_class;

private:
    friend class SPPolyLine;	
};


class CPolyLine : public CShape {
public:
	CPolyLine(SPPolyLine* polyline);
	virtual ~CPolyLine();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

	virtual gchar* onDescription();

protected:
	SPPolyLine* sppolyline;
};


#endif // SEEN_SP_POLYLINE_H

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
