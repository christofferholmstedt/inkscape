/*
 * SVG <title> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sp-title.h"
#include "xml/repr.h"

#include "sp-factory.h"

namespace {
	SPObject* createTitle() {
		return new SPTitle();
	}

	bool titleRegistered = SPFactory::instance().registerObject("svg:title", createTitle);
}

G_DEFINE_TYPE(SPTitle, sp_title, G_TYPE_OBJECT);

static void
sp_title_class_init(SPTitleClass *klass)
{
}

CTitle::CTitle(SPTitle* title) : CObject(title) {
	this->sptitle = title;
}

CTitle::~CTitle() {
}

SPTitle::SPTitle() : SPObject() {
	SPTitle* desc = this;

	desc->ctitle = new CTitle(desc);
	desc->typeHierarchy.insert(typeid(SPTitle));

	delete desc->cobject;
	desc->cobject = desc->ctitle;
}

static void
sp_title_init(SPTitle *desc)
{
	new (desc) SPTitle();
}

Inkscape::XML::Node* CTitle::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTitle* object = this->sptitle;

    if (!repr) {
        repr = object->getRepr()->duplicate(xml_doc);
    }

    CObject::write(xml_doc, repr, flags);

    return repr;
}

