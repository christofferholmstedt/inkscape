/*
 * SVG <desc> implementation
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

#include "sp-desc.h"
#include "xml/repr.h"

static void sp_desc_class_init(SPDescClass *klass);
static void sp_desc_init(SPDesc *rect);
static Inkscape::XML::Node *sp_desc_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPObjectClass *desc_parent_class;

GType sp_desc_get_type (void)
{
    static GType desc_type = 0;

    if (!desc_type) {
        GTypeInfo desc_info = {
            sizeof (SPDescClass),
            NULL, NULL,
            (GClassInitFunc) sp_desc_class_init,
            NULL, NULL,
            sizeof (SPDesc),
            16,
            (GInstanceInitFunc) sp_desc_init,
            NULL,    /* value_table */
        };
        desc_type = g_type_register_static (SP_TYPE_OBJECT, "SPDesc", &desc_info, (GTypeFlags)0);
    }
    return desc_type;
}

static void sp_desc_class_init(SPDescClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *)(klass);
    desc_parent_class = (SPObjectClass *)(g_type_class_ref(SP_TYPE_OBJECT));

//    sp_object_class->write = sp_desc_write;
}

CDesc::CDesc(SPDesc* desc) : CObject(desc) {
	this->spdesc = desc;
}

CDesc::~CDesc() {
}

static void sp_desc_init(SPDesc *desc)
{
	desc->cdesc = new CDesc(desc);
	desc->cobject = desc->cdesc;
}

Inkscape::XML::Node* CDesc::onWrite(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
	SPDesc* object = this->spdesc;

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CObject::onWrite(doc, repr, flags);

    return repr;
}

/**
 * Writes it's settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *sp_desc_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPDesc*)object)->cdesc->onWrite(doc, repr, flags);
}
