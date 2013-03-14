#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_SVG_FONTS

/*
 * SVG <missing-glyph> element implementation
 *
 * Author:
 *   Felipe C. da S. Sanches <juca@members.fsf.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008, Felipe C. da S. Sanches
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "xml/repr.h"
#include "attributes.h"
#include "sp-missing-glyph.h"
#include "document.h"

static void sp_missing_glyph_class_init(SPMissingGlyphClass *gc);
static void sp_missing_glyph_init(SPMissingGlyph *glyph);

static void sp_missing_glyph_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_missing_glyph_release(SPObject *object);
static void sp_missing_glyph_set(SPObject *object, unsigned int key, const gchar *value);
static Inkscape::XML::Node *sp_missing_glyph_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPObjectClass *parent_class;

GType sp_missing_glyph_get_type(void)
{
    static GType type = 0;

    if (!type) {
        GTypeInfo info = {
            sizeof(SPMissingGlyphClass),
            NULL,	/* base_init */
            NULL,	/* base_finalize */
            (GClassInitFunc) sp_missing_glyph_class_init,
            NULL,	/* class_finalize */
            NULL,	/* class_data */
            sizeof(SPMissingGlyph),
            16,	/* n_preallocs */
            (GInstanceInitFunc) sp_missing_glyph_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static(SP_TYPE_OBJECT, "SPMissingGlyph", &info, (GTypeFlags) 0);
    }

    return type;
}

static void sp_missing_glyph_class_init(SPMissingGlyphClass *gc)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) gc;

    parent_class = (SPObjectClass*)g_type_class_peek_parent(gc);

    //sp_object_class->build = sp_missing_glyph_build;
    sp_object_class->release = sp_missing_glyph_release;
    sp_object_class->set = sp_missing_glyph_set;
    sp_object_class->write = sp_missing_glyph_write;
}

CMissingGlyph::CMissingGlyph(SPMissingGlyph* mg) : CObject(mg) {
	this->spmissingglyph = mg;
}

CMissingGlyph::~CMissingGlyph() {
}

static void sp_missing_glyph_init(SPMissingGlyph *glyph)
{
	glyph->cmissingglyph = new CMissingGlyph(glyph);
	glyph->cobject = glyph->cmissingglyph;

//TODO: correct these values:
    glyph->d = NULL;
    glyph->horiz_adv_x = 0;
    glyph->vert_origin_x = 0;
    glyph->vert_origin_y = 0;
    glyph->vert_adv_y = 0;
}

void CMissingGlyph::onBuild(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPMissingGlyph* object = this->spmissingglyph;

    CObject::onBuild(doc, repr);

    object->readAttr( "d" );
    object->readAttr( "horiz-adv-x" );
    object->readAttr( "vert-origin-x" );
    object->readAttr( "vert-origin-y" );
    object->readAttr( "vert-adv-y" );
}

static void sp_missing_glyph_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	((SPMissingGlyph*)object)->cmissingglyph->onBuild(document, repr);
}

void CMissingGlyph::onRelease() {
	CObject::onRelease();
}

static void sp_missing_glyph_release(SPObject *object)
{
	((SPMissingGlyph*)object)->cmissingglyph->onRelease();
}

void CMissingGlyph::onSet(unsigned int key, const gchar* value) {
	SPMissingGlyph* object = this->spmissingglyph;

    SPMissingGlyph *glyph = SP_MISSING_GLYPH(object);

    switch (key) {
        case SP_ATTR_D:
        {
            if (glyph->d) {
                g_free(glyph->d);
            }
            glyph->d = g_strdup(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_HORIZ_ADV_X:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->horiz_adv_x){
                glyph->horiz_adv_x = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ORIGIN_X:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_origin_x){
                glyph->vert_origin_x = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ORIGIN_Y:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_origin_y){
                glyph->vert_origin_y = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        case SP_ATTR_VERT_ADV_Y:
        {
            double number = value ? g_ascii_strtod(value, 0) : 0;
            if (number != glyph->vert_adv_y){
                glyph->vert_adv_y = number;
                object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default:
        {
            CObject::onSet(key, value);
            break;
        }
    }
}

static void sp_missing_glyph_set(SPObject *object, unsigned int key, const gchar *value)
{
	((SPMissingGlyph*)object)->cmissingglyph->onSet(key, value);
}

#define COPY_ATTR(rd,rs,key) (rd)->setAttribute((key), rs->attribute(key));

Inkscape::XML::Node* CMissingGlyph::onWrite(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
	SPMissingGlyph* object = this->spmissingglyph;

	//    SPMissingGlyph *glyph = SP_MISSING_GLYPH(object);

	    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
	        repr = xml_doc->createElement("svg:glyph");
	    }

	/* I am commenting out this part because I am not certain how does it work. I will have to study it later. Juca
	    repr->setAttribute("d", glyph->d);
	    sp_repr_set_svg_double(repr, "horiz-adv-x", glyph->horiz_adv_x);
	    sp_repr_set_svg_double(repr, "vert-origin-x", glyph->vert_origin_x);
	    sp_repr_set_svg_double(repr, "vert-origin-y", glyph->vert_origin_y);
	    sp_repr_set_svg_double(repr, "vert-adv-y", glyph->vert_adv_y);
	*/
	    if (repr != object->getRepr()) {

	        // All the COPY_ATTR functions below use
	        //  XML Tree directly while they shouldn't.
	        COPY_ATTR(repr, object->getRepr(), "d");
	        COPY_ATTR(repr, object->getRepr(), "horiz-adv-x");
	        COPY_ATTR(repr, object->getRepr(), "vert-origin-x");
	        COPY_ATTR(repr, object->getRepr(), "vert-origin-y");
	        COPY_ATTR(repr, object->getRepr(), "vert-adv-y");
	    }

	    CObject::onWrite(xml_doc, repr, flags);

	    return repr;
}

static Inkscape::XML::Node *sp_missing_glyph_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPMissingGlyph*)object)->cmissingglyph->onWrite(xml_doc, repr, flags);
}
#endif //#ifdef ENABLE_SVG_FONTS
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
