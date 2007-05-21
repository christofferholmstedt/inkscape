#define __SP_FEBLEND_CPP__

/** \file
 * SVG <feBlend> implementation.
 *
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include "attributes.h"
#include "svg/svg.h"
#include "sp-feblend.h"
#include "xml/repr.h"

#include "display/nr-filter.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-blend.h"
#include "display/nr-filter-types.h"

/* FeBlend base class */

static void sp_feBlend_class_init(SPFeBlendClass *klass);
static void sp_feBlend_init(SPFeBlend *feBlend);

static void sp_feBlend_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_feBlend_release(SPObject *object);
static void sp_feBlend_set(SPObject *object, unsigned int key, gchar const *value);
static void sp_feBlend_update(SPObject *object, SPCtx *ctx, guint flags);
static Inkscape::XML::Node *sp_feBlend_write(SPObject *object, Inkscape::XML::Node *repr, guint flags);
static void sp_feBlend_build_renderer(SPFilterPrimitive *sp_prim, NR::Filter *filter);

static SPFilterPrimitiveClass *feBlend_parent_class;
static int renderer;

GType
sp_feBlend_get_type()
{
    static GType feBlend_type = 0;

    if (!feBlend_type) {
        GTypeInfo feBlend_info = {
            sizeof(SPFeBlendClass),
            NULL, NULL,
            (GClassInitFunc) sp_feBlend_class_init,
            NULL, NULL,
            sizeof(SPFeBlend),
            16,
            (GInstanceInitFunc) sp_feBlend_init,
            NULL,    /* value_table */
        };
        feBlend_type = g_type_register_static(SP_TYPE_FILTER_PRIMITIVE, "SPFeBlend", &feBlend_info, (GTypeFlags)0);
    }
    return feBlend_type;
}

static void
sp_feBlend_class_init(SPFeBlendClass *klass)
{
    SPObjectClass *sp_object_class = (SPObjectClass *)klass;
    SPFilterPrimitiveClass *sp_primitive_class = (SPFilterPrimitiveClass *)klass;

    feBlend_parent_class = (SPFilterPrimitiveClass*)g_type_class_peek_parent(klass);

    sp_object_class->build = sp_feBlend_build;
    sp_object_class->release = sp_feBlend_release;
    sp_object_class->write = sp_feBlend_write;
    sp_object_class->set = sp_feBlend_set;
    sp_object_class->update = sp_feBlend_update;

    sp_primitive_class->build_renderer = sp_feBlend_build_renderer;
}

static void
sp_feBlend_init(SPFeBlend *feBlend)
{
    renderer = -1;
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeBlend variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
static void
sp_feBlend_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    if (((SPObjectClass *) feBlend_parent_class)->build) {
        ((SPObjectClass *) feBlend_parent_class)->build(object, document, repr);
    }

    /*LOAD ATTRIBUTES FROM REPR HERE*/
    sp_object_read_attr(object, "mode");
}

/**
 * Drops any allocated memory.
 */
static void
sp_feBlend_release(SPObject *object)
{
    if (((SPObjectClass *) feBlend_parent_class)->release)
        ((SPObjectClass *) feBlend_parent_class)->release(object);
}

static NR::FilterBlendMode sp_feBlend_readmode(gchar const *value)
{
    if (!value) return NR::BLEND_NORMAL;
    switch (value[0]) {
        case 'n':
            if (strncmp(value, "normal", 6) == 0)
                return NR::BLEND_NORMAL;
            break;
        case 'm':
            if (strncmp(value, "multiply", 8) == 0)
                return NR::BLEND_MULTIPLY;
            break;
        case 's':
            if (strncmp(value, "screen", 6) == 0)
                return NR::BLEND_SCREEN;
            break;
        case 'd':
            if (strncmp(value, "darken", 6) == 0)
                return NR::BLEND_DARKEN;
            break;
        case 'l':
            if (strncmp(value, "lighten", 7) == 0)
                return NR::BLEND_LIGHTEN;
            break;
        default:
            // do nothing by default
            break;
    }
    return NR::BLEND_NORMAL;
}

/**
 * Sets a specific value in the SPFeBlend.
 */
static void
sp_feBlend_set(SPObject *object, unsigned int key, gchar const *value)
{
    SPFeBlend *feBlend = SP_FEBLEND(object);
    (void)feBlend;

    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        case SP_ATTR_MODE:
            feBlend->blend_mode = sp_feBlend_readmode(value);
/*
            if (renderer >= 0) {
                NR::Filter *filter = SP_FILTER(object->parent)->_renderer;
                NR::FilterBlend *blend = dynamic_cast<NR::FilterBlend*>(filter->get_primitive(renderer));
                blend->set_mode(feBlend->blend_mode);
            }
*/
            object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            if (((SPObjectClass *) feBlend_parent_class)->set)
                ((SPObjectClass *) feBlend_parent_class)->set(object, key, value);
            break;
    }

}

/**
 * Receives update notifications.
 */
static void
sp_feBlend_update(SPObject *object, SPCtx *ctx, guint flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG)) {
        sp_object_read_attr(object, "mode");
    }

    if (((SPObjectClass *) feBlend_parent_class)->update) {
        ((SPObjectClass *) feBlend_parent_class)->update(object, ctx, flags);
    }
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *
sp_feBlend_write(SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
    // Inkscape-only object, not copied during an "plain SVG" dump:
    if (flags & SP_OBJECT_WRITE_EXT) {
        if (repr) {
            // is this sane?
            // repr->mergeFrom(SP_OBJECT_REPR(object), "id");
        } else {
            repr = SP_OBJECT_REPR(object)->duplicate(NULL); // FIXME
        }
    }

    if (((SPObjectClass *) feBlend_parent_class)->write) {
        ((SPObjectClass *) feBlend_parent_class)->write(object, repr, flags);
    }

    return repr;
}

static void sp_feBlend_build_renderer(SPFilterPrimitive *primitive, NR::Filter *filter) {
    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeBlend *sp_blend = SP_FEBLEND(primitive);

    int primitive_n = filter->add_primitive(NR::NR_FILTER_BLEND);
    NR::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    NR::FilterBlend *nr_blend = dynamic_cast<NR::FilterBlend*>(nr_primitive);
    g_assert(nr_blend != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    nr_blend->set_mode(sp_blend->blend_mode);

    renderer = primitive_n;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
