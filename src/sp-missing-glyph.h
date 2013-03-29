#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef __SP_MISSING_GLYPH_H__
#define __SP_MISSING_GLYPH_H__

/*
 * SVG <missing-glyph> element implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_MISSING_GLYPH (sp_missing_glyph_get_type ())
#define SP_MISSING_GLYPH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_MISSING_GLYPH, SPMissingGlyph))
#define SP_MISSING_GLYPH_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_MISSING_GLYPH, SPMissingGlyphClass))
#define SP_IS_MISSING_GLYPH(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_MISSING_GLYPH))
#define SP_IS_MISSING_GLYPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_MISSING_GLYPH))

class CMissingGlyph;

class SPMissingGlyph : public SPObject {
public:
	CMissingGlyph* cmissingglyph;

    char* d;
    double horiz_adv_x;
    double vert_origin_x;
    double vert_origin_y;
    double vert_adv_y;
};

struct SPMissingGlyphClass {
	SPObjectClass parent_class;
};


class CMissingGlyph : public CObject {
public:
	CMissingGlyph(SPMissingGlyph* mg);
	virtual ~CMissingGlyph();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();
	virtual void onSet(unsigned int key, const gchar* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPMissingGlyph* spmissingglyph;
};


GType sp_missing_glyph_get_type (void);

#endif //#ifndef __SP_MISSING_GLYPH_H__
