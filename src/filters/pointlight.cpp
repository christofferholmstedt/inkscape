/** \file
 * SVG <fepointlight> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include "attributes.h"
#include "document.h"
#include "filters/pointlight.h"
#include "filters/diffuselighting.h"
#include "filters/specularlighting.h"
#include "xml/repr.h"

#define SP_MACROS_SILENT
#include "macros.h"

/* FePointLight class */

static void sp_fepointlight_class_init(SPFePointLightClass *klass);
static void sp_fepointlight_init(SPFePointLight *fepointlight);

static void sp_fepointlight_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_fepointlight_release(SPObject *object);
static void sp_fepointlight_set(SPObject *object, unsigned int key, gchar const *value);
static void sp_fepointlight_update(SPObject *object, SPCtx *ctx, guint flags);
static Inkscape::XML::Node *sp_fepointlight_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPObjectClass *fePointLight_parent_class;

GType
sp_fepointlight_get_type()
{
    static GType fepointlight_type = 0;

    if (!fepointlight_type) {
        GTypeInfo fepointlight_info = {
            sizeof(SPFePointLightClass),
            NULL, NULL,
            (GClassInitFunc) sp_fepointlight_class_init,
            NULL, NULL,
            sizeof(SPFePointLight),
            16,
            (GInstanceInitFunc) sp_fepointlight_init,
            NULL,    /* value_table */
        };
        fepointlight_type = g_type_register_static(SP_TYPE_OBJECT, "SPFePointLight", &fepointlight_info, (GTypeFlags)0);
    }
    return fepointlight_type;
}

static void
sp_fepointlight_class_init(SPFePointLightClass *klass)
{

    SPObjectClass *sp_object_class = (SPObjectClass *)klass;

    fePointLight_parent_class = (SPObjectClass*)g_type_class_peek_parent(klass);

    //sp_object_class->build = sp_fepointlight_build;
//    sp_object_class->release = sp_fepointlight_release;
//    sp_object_class->write = sp_fepointlight_write;
//    sp_object_class->set = sp_fepointlight_set;
//    sp_object_class->update = sp_fepointlight_update;
}

CFePointLight::CFePointLight(SPFePointLight* pointlight) : CObject(pointlight) {
	this->spfepointlight = pointlight;
}

CFePointLight::~CFePointLight() {
}

static void
sp_fepointlight_init(SPFePointLight *fepointlight)
{
	fepointlight->cfepointlight = new CFePointLight(fepointlight);
	fepointlight->cobject = fepointlight->cfepointlight;

    fepointlight->x = 0;
    fepointlight->y = 0;
    fepointlight->z = 0;

    fepointlight->x_set = FALSE;
    fepointlight->y_set = FALSE;
    fepointlight->z_set = FALSE;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPPointLight variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
//static void
//sp_fepointlight_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
//{
//    if (((SPObjectClass *) fePointLight_parent_class)->build) {
//        ((SPObjectClass *) fePointLight_parent_class)->build(object, document, repr);
//    }
//    //Read values of key attributes from XML nodes into object.
//    object->readAttr( "x" );
//    object->readAttr( "y" );
//    object->readAttr( "z" );
//
////is this necessary?
//    document->addResource("fepointlight", object);
//	((SPFePointLight*)object)->cfepointlight->onBuild(document, repr);
//}

void CFePointLight::onBuild(SPDocument *document, Inkscape::XML::Node *repr) {
	CObject::onBuild(document, repr);

	SPFePointLight* object = this->spfepointlight;

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "z" );

//is this necessary?
    document->addResource("fepointlight", object);
}

/**
 * Drops any allocated memory.
 */
static void
sp_fepointlight_release(SPObject *object)
{
//    //SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);
//
//    if ( object->document ) {
//        // Unregister ourselves
//        object->document->removeResource("fepointlight", object);
//    }
//
////TODO: release resources here
	((SPFePointLight*)object)->cfepointlight->onRelease();
}

void CFePointLight::onRelease() {
	SPFePointLight* object = this->spfepointlight;
    //SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);

    if ( object->document ) {
        // Unregister ourselves
        object->document->removeResource("fepointlight", object);
    }

//TODO: release resources here
}

/**
 * Sets a specific value in the SPFePointLight.
 */
static void
sp_fepointlight_set(SPObject *object, unsigned int key, gchar const *value)
{
//    SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);
//    gchar *end_ptr;
//    switch (key) {
//    case SP_ATTR_X:
//        end_ptr = NULL;
//        if (value) {
//            fepointlight->x = g_ascii_strtod(value, &end_ptr);
//            if (end_ptr) {
//                fepointlight->x_set = TRUE;
//            }
//        }
//        if (!value || !end_ptr) {
//            fepointlight->x = 0;
//            fepointlight->x_set = FALSE;
//        }
//        if (object->parent &&
//                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
//                 SP_IS_FESPECULARLIGHTING(object->parent))) {
//            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
//        }
//        break;
//    case SP_ATTR_Y:
//        end_ptr = NULL;
//        if (value) {
//            fepointlight->y = g_ascii_strtod(value, &end_ptr);
//            if (end_ptr) {
//                fepointlight->y_set = TRUE;
//            }
//        }
//        if (!value || !end_ptr) {
//            fepointlight->y = 0;
//            fepointlight->y_set = FALSE;
//        }
//        if (object->parent &&
//                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
//                 SP_IS_FESPECULARLIGHTING(object->parent))) {
//            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
//        }
//        break;
//    case SP_ATTR_Z:
//        end_ptr = NULL;
//        if (value) {
//            fepointlight->z = g_ascii_strtod(value, &end_ptr);
//            if (end_ptr) {
//                fepointlight->z_set = TRUE;
//            }
//        }
//        if (!value || !end_ptr) {
//            fepointlight->z = 0;
//            fepointlight->z_set = FALSE;
//        }
//        if (object->parent &&
//                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
//                 SP_IS_FESPECULARLIGHTING(object->parent))) {
//            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
//        }
//        break;
//    default:
//        // See if any parents need this value.
//        if (((SPObjectClass *) fePointLight_parent_class)->set) {
//            ((SPObjectClass *) fePointLight_parent_class)->set(object, key, value);
//        }
//        break;
//    }
	((SPFePointLight*)object)->cfepointlight->onSet(key, value);
}

void CFePointLight::onSet(unsigned int key, gchar const *value) {
	SPFePointLight* object = this->spfepointlight;

    SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);
    gchar *end_ptr;
    switch (key) {
    case SP_ATTR_X:
        end_ptr = NULL;
        if (value) {
            fepointlight->x = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                fepointlight->x_set = TRUE;
            }
        }
        if (!value || !end_ptr) {
            fepointlight->x = 0;
            fepointlight->x_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_Y:
        end_ptr = NULL;
        if (value) {
            fepointlight->y = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                fepointlight->y_set = TRUE;
            }
        }
        if (!value || !end_ptr) {
            fepointlight->y = 0;
            fepointlight->y_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    case SP_ATTR_Z:
        end_ptr = NULL;
        if (value) {
            fepointlight->z = g_ascii_strtod(value, &end_ptr);
            if (end_ptr) {
                fepointlight->z_set = TRUE;
            }
        }
        if (!value || !end_ptr) {
            fepointlight->z = 0;
            fepointlight->z_set = FALSE;
        }
        if (object->parent &&
                (SP_IS_FEDIFFUSELIGHTING(object->parent) ||
                 SP_IS_FESPECULARLIGHTING(object->parent))) {
            object->parent->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
        }
        break;
    default:
        // See if any parents need this value.
//        if (((SPObjectClass *) fePointLight_parent_class)->set) {
//            ((SPObjectClass *) fePointLight_parent_class)->set(object, key, value);
//        }
    	CObject::onSet(key, value);
        break;
    }
}

/**
 *  * Receives update notifications.
 *   */
static void
sp_fepointlight_update(SPObject *object, SPCtx *ctx, guint flags)
{
//    SPFePointLight *fePointLight = SP_FEPOINTLIGHT(object);
//    (void)fePointLight;
//
//    if (flags & SP_OBJECT_MODIFIED_FLAG) {
//        /* do something to trigger redisplay, updates? */
//        object->readAttr( "x" );
//        object->readAttr( "y" );
//        object->readAttr( "z" );
//    }
//
//    if (((SPObjectClass *) fePointLight_parent_class)->update) {
//        ((SPObjectClass *) fePointLight_parent_class)->update(object, ctx, flags);
//    }
	((SPFePointLight*)object)->cfepointlight->onUpdate(ctx, flags);
}

void CFePointLight::onUpdate(SPCtx *ctx, guint flags) {
	SPFePointLight* object = this->spfepointlight;

    SPFePointLight *fePointLight = SP_FEPOINTLIGHT(object);
    (void)fePointLight;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        /* do something to trigger redisplay, updates? */
        object->readAttr( "x" );
        object->readAttr( "y" );
        object->readAttr( "z" );
    }

//    if (((SPObjectClass *) fePointLight_parent_class)->update) {
//        ((SPObjectClass *) fePointLight_parent_class)->update(object, ctx, flags);
//    }
    CObject::onUpdate(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *
sp_fepointlight_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
//    SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);
//
//    if (!repr) {
//        repr = object->getRepr()->duplicate(doc);
//    }
//
//    if (fepointlight->x_set)
//        sp_repr_set_css_double(repr, "x", fepointlight->x);
//    if (fepointlight->y_set)
//        sp_repr_set_css_double(repr, "y", fepointlight->y);
//    if (fepointlight->z_set)
//        sp_repr_set_css_double(repr, "z", fepointlight->z);
//
//    if (((SPObjectClass *) fePointLight_parent_class)->write) {
//        ((SPObjectClass *) fePointLight_parent_class)->write(object, doc, repr, flags);
//    }
//
//    return repr;
	return ((SPFePointLight*)object)->cfepointlight->onWrite(doc, repr, flags);
}

Inkscape::XML::Node* CFePointLight::onWrite(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFePointLight* object = this->spfepointlight;
    SPFePointLight *fepointlight = SP_FEPOINTLIGHT(object);

    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    if (fepointlight->x_set)
        sp_repr_set_css_double(repr, "x", fepointlight->x);
    if (fepointlight->y_set)
        sp_repr_set_css_double(repr, "y", fepointlight->y);
    if (fepointlight->z_set)
        sp_repr_set_css_double(repr, "z", fepointlight->z);

//    if (((SPObjectClass *) fePointLight_parent_class)->write) {
//        ((SPObjectClass *) fePointLight_parent_class)->write(object, doc, repr, flags);
//    }
    CObject::onWrite(doc, repr, flags);

    return repr;
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
