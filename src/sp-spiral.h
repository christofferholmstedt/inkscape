#ifndef SEEN_SP_SPIRAL_H
#define SEEN_SP_SPIRAL_H
/*
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-shape.h"

#define noSPIRAL_VERBOSE

#define SP_EPSILON       1e-5
#define SP_EPSILON_2     (SP_EPSILON * SP_EPSILON)
#define SP_HUGE          1e5

#define SPIRAL_TOLERANCE 3.0
#define SAMPLE_STEP      (1.0/4.0) ///< step per 2PI 
#define SAMPLE_SIZE      8         ///< sample size per one bezier 

#define SP_TYPE_SPIRAL            (sp_spiral_get_type ())
#define SP_SPIRAL(obj) ((SPSpiral*)obj)
#define SP_IS_SPIRAL(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPSpiral)))

class CSpiral;
/**
 * A spiral Shape.
 *
 * The Spiral shape is defined as:
 * \verbatim
   x(t) = rad * t^exp cos(2 * Pi * revo*t + arg) + cx
   y(t) = rad * t^exp sin(2 * Pi * revo*t + arg) + cy    \endverbatim
 * where spiral curve is drawn for {t | t0 <= t <= 1}. The  rad and arg 
 * parameters can also be represented by transformation. 
 *
 * \todo Should I remove these attributes?
 */
class SPSpiral : public SPShape {
public:
	CSpiral* cspiral;

	float cx, cy;
	float exp;  ///< Spiral expansion factor
	float revo; ///< Spiral revolution factor
	float rad;  ///< Spiral radius
	float arg;  ///< Spiral argument
	float t0;

	/* Lowlevel interface */
	void setPosition(gdouble cx, gdouble cy, gdouble exp, gdouble revo, gdouble rad, gdouble arg, gdouble t0);

	Geom::Point getXY(gdouble t) const;

	void getPolar(gdouble t, gdouble* rad, gdouble* arg) const;

	bool isInvalid() const;

//private:
	Geom::Point getTangent(gdouble t) const;

	void fitAndDraw(SPCurve* c, double dstep, Geom::Point darray[], Geom::Point const& hat1, Geom::Point& hat2, double* t) const;
};

/// The SPSpiral vtable.
struct SPSpiralClass {
	SPShapeClass parent_class;
};


class CSpiral : public CShape {
public:
	CSpiral(SPSpiral* spiral);
	virtual ~CSpiral();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void update(SPCtx *ctx, guint flags);
	virtual void set(unsigned int key, gchar const* value);

	virtual void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
	virtual gchar* description();

	virtual void set_shape();
	virtual void update_patheffect(bool write);

protected:
	SPSpiral* spspiral;
};


/* Standard Gtk function */
GType sp_spiral_get_type  (void);

#endif // SEEN_SP_SPIRAL_H
