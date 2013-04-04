#ifndef __SP_POLYGON_H__
#define __SP_POLYGON_H__

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-shape.h"


#define SP_POLYGON(obj) ((SPPolygon*)obj)
#define SP_IS_POLYGON(obj) (dynamic_cast<const SPPolygon*>((SPObject*)obj))

class SPPolygon : public SPShape, public CShape {
public:
	SPPolygon();
	virtual ~SPPolygon();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual void set(unsigned int key, gchar const* value);
	virtual gchar* description();
};

// made 'public' so that SPCurve can set it as friend:
void sp_polygon_set(SPObject *object, unsigned int key, const gchar *value);

#endif
