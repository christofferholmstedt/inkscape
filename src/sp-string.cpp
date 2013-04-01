/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme:
 *
 * These subcomponents should not be items, or alternately
 * we have to invent set of flags to mark, whether standard
 * attributes are applicable to given item (I even like this
 * idea somewhat - Lauris)
 *
 */



#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "sp-string.h"
#include "xml/repr.h"


/*#####################################################
#  SPSTRING
#####################################################*/

G_DEFINE_TYPE(SPString, sp_string, G_TYPE_OBJECT);

static void
sp_string_class_init(SPStringClass *classname)
{
}

CString::CString(SPString* str) : CObject(str) {
	this->spstring = str;
}

CString::~CString() {
}

SPString::SPString() : SPObject() {
	SPString* string = this;

	string->cstring = new CString(string);
	string->typeHierarchy.insert(typeid(SPString));

	delete string->cobject;
	string->cobject = string->cstring;

    new (&string->string) Glib::ustring();
}

static void
sp_string_init(SPString *string)
{
	new (string) SPString();
}

void CString::build(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPString* object = this->spstring;
    object->cstring->read_content();

    CObject::build(doc, repr);
}

void CString::release() {
	SPString* object = this->spstring;
    SPString *string = SP_STRING(object);

    string->string.~ustring();

    CObject::release();
}


void CString::read_content() {
	SPString* object = this->spstring;

    SPString *string = SP_STRING(object);

    string->string.clear();

    //XML Tree being used directly here while it shouldn't be.
    gchar const *xml_string = string->getRepr()->content();
    // see algorithms described in svg 1.1 section 10.15
    if (object->xml_space.value == SP_XML_SPACE_PRESERVE) {
        for ( ; *xml_string ; xml_string = g_utf8_next_char(xml_string) ) {
            gunichar c = g_utf8_get_char(xml_string);
            if ((c == 0xa) || (c == 0xd) || (c == '\t')) {
                c = ' ';
            }
            string->string += c;
        }
    }
    else {
        bool whitespace = false;
        for ( ; *xml_string ; xml_string = g_utf8_next_char(xml_string) ) {
            gunichar c = g_utf8_get_char(xml_string);
            if ((c == 0xa) || (c == 0xd)) {
                continue;
            }
            if ((c == ' ') || (c == '\t')) {
                whitespace = true;
            } else {
                if (whitespace && (!string->string.empty() || (object->getPrev() != NULL))) {
                    string->string += ' ';
                }
                string->string += c;
                whitespace = false;
            }
        }
        if (whitespace && object->getRepr()->next() != NULL) {   // can't use SPObject::getNext() when the SPObject tree is still being built
            string->string += ' ';
        }
    }
    object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void CString::update(SPCtx *ctx, unsigned flags) {
//    CObject::onUpdate(ctx, flags);

    if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_MODIFIED_FLAG)) {
        /* Parent style or we ourselves changed, so recalculate */
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // won't be "just a transformation" anymore, we're going to recompute "x" and "y" attributes
    }
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
