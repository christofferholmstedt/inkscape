/** \file
 * SVG <feMerge> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "xml/repr.h"

#include "merge.h"
#include "mergenode.h"
#include "display/nr-filter.h"
#include "display/nr-filter-merge.h"

#include "sp-factory.h"

namespace {
	SPObject* createMerge() {
		return new SPFeMerge();
	}

	bool mergeRegistered = SPFactory::instance().registerObject("svg:feMerge", createMerge);
}

SPFeMerge::SPFeMerge() : SPFilterPrimitive() {
	this->cobject = this;
}

SPFeMerge::~SPFeMerge() {
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeMerge variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFeMerge::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFilterPrimitive::build(document, repr);
}

/**
 * Drops any allocated memory.
 */
void SPFeMerge::release() {
	SPFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeMerge.
 */
void SPFeMerge::set(unsigned int key, gchar const *value) {
	SPFeMerge* object = this;

    SPFeMerge *feMerge = SP_FEMERGE(object);
    (void)feMerge;

    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        default:
        	SPFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFeMerge::update(SPCtx *ctx, guint flags) {
	SPFeMerge* object = this;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        object->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }

    SPFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFeMerge::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeMerge* object = this;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it. And child nodes, too! */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }


    SPFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void SPFeMerge::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeMerge* primitive = this;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeMerge *sp_merge = SP_FEMERGE(primitive);
    (void)sp_merge;

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_MERGE);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterMerge *nr_merge = dynamic_cast<Inkscape::Filters::FilterMerge*>(nr_primitive);
    g_assert(nr_merge != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);

    SPObject *input = primitive->children;
    int in_nr = 0;
    while (input) {
        if (SP_IS_FEMERGENODE(input)) {
            SPFeMergeNode *node = SP_FEMERGENODE(input);
            nr_merge->set_input(in_nr, node->input);
            in_nr++;
        }
        input = input->next;
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
