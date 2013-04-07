/** \file
 * SVG <filter> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <map>
#include <string.h>
using std::map;
using std::pair;

#include <glibmm/stringutils.h>
#include "attributes.h"
#include "document.h"
#include "sp-filter.h"
#include "sp-filter-reference.h"
#include "sp-filter-primitive.h"
#include "uri.h"
#include "xml/repr.h"
#include <cstring>
#include <string>

#define SP_MACROS_SILENT
#include "macros.h"

#include "display/nr-filter.h"

static void filter_ref_changed(SPObject *old_ref, SPObject *ref, SPFilter *filter);
static void filter_ref_modified(SPObject *href, guint flags, SPFilter *filter);

#include "sp-factory.h"

namespace {
	SPObject* createFilter() {
		return new SPFilter();
	}

	bool filterRegistered = SPFactory::instance().registerObject("svg:filter", createFilter);
}

SPFilter::SPFilter() : SPObject() {
    this->href = new SPFilterReference(this);
    this->href->changedSignal().connect(sigc::bind(sigc::ptr_fun(filter_ref_changed), this));

    this->x = 0;
    this->y = 0;
    this->width = 0;
    this->height = 0;

    this->filterUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
    this->primitiveUnits = SP_FILTER_UNITS_USERSPACEONUSE;
    this->filterUnits_set = FALSE;
    this->primitiveUnits_set = FALSE;

    this->_renderer = NULL;

    this->_image_name = new std::map<gchar *, int, ltstr>;
    this->_image_name->clear();
    this->_image_number_next = 0;

    this->filterRes = NumberOptNumber();

    new (&this->modified_connection) sigc::connection();
}

SPFilter::~SPFilter() {
}


/**
 * Reads the Inkscape::XML::Node, and initializes SPFilter variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void SPFilter::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPFilter* object = this;

    //Read values of key attributes from XML nodes into object.
    object->readAttr( "style" ); // struct not derived from SPItem, we need to do this ourselves.
    object->readAttr( "filterUnits" );
    object->readAttr( "primitiveUnits" );
    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "width" );
    object->readAttr( "height" );
    object->readAttr( "filterRes" );
    object->readAttr( "xlink:href" );

	SPObject::build(document, repr);

//is this necessary?
    document->addResource("filter", object);
}

/**
 * Drops any allocated memory.
 */
void SPFilter::release() {
	SPFilter* object = this;
    SPFilter *filter = SP_FILTER(object);

    if (object->document) {
        // Unregister ourselves
        object->document->removeResource("filter", object);
    }

//TODO: release resources here

    //release href
    if (filter->href) {
        filter->modified_connection.disconnect();
        filter->href->detach();
        delete filter->href;
        filter->href = NULL;
    }

    filter->modified_connection.~connection();
    delete filter->_image_name;

    SPObject::release();
}

/**
 * Sets a specific value in the SPFilter.
 */
void SPFilter::set(unsigned int key, gchar const *value) {
	SPFilter* object = this;
    SPFilter *filter = SP_FILTER(object);

    switch (key) {
        case SP_ATTR_FILTERUNITS:
            if (value) {
                if (!strcmp(value, "userSpaceOnUse")) {
                    filter->filterUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                } else {
                    filter->filterUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                }
                filter->filterUnits_set = TRUE;
            } else {
                filter->filterUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                filter->filterUnits_set = FALSE;
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_PRIMITIVEUNITS:
            if (value) {
                if (!strcmp(value, "objectBoundingBox")) {
                    filter->primitiveUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                } else {
                    filter->primitiveUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                }
                filter->primitiveUnits_set = TRUE;
            } else {
                filter->primitiveUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                filter->primitiveUnits_set = FALSE;
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_X:
            filter->x.readOrUnset(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_Y:
            filter->y.readOrUnset(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_WIDTH:
            filter->width.readOrUnset(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_HEIGHT:
            filter->height.readOrUnset(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_FILTERRES:
            filter->filterRes.set(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_XLINK_HREF:
            if (value) {
                try {
                    filter->href->attach(Inkscape::URI(value));
                } catch (Inkscape::BadURIException &e) {
                    g_warning("%s", e.what());
                    filter->href->detach();
                }
            } else {
                filter->href->detach();
            }
            break;
        default:
            // See if any parents need this value.
        	SPObject::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void SPFilter::update(SPCtx *ctx, guint flags) {
    //SPFilter *filter = SP_FILTER(object);

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    SPObject::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPFilter::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFilter* object = this;
    SPFilter *filter = SP_FILTER(object);

    // Original from sp-item-group.cpp
    if (flags & SP_OBJECT_WRITE_BUILD) {
        if (!repr) {
            repr = doc->createElement("svg:filter");
        }
        GSList *l = NULL;
        for ( SPObject *child = object->firstChild(); child; child = child->getNext() ) {
            Inkscape::XML::Node *crepr = child->updateRepr(doc, NULL, flags);
            if (crepr) {
                l = g_slist_prepend (l, crepr);
            }
        }
        while (l) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove (l, l->data);
        }
    } else {
        for ( SPObject *child = object->firstChild() ; child; child = child->getNext() ) {
            child->updateRepr(flags);
        }
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || filter->filterUnits_set) {
        switch (filter->filterUnits) {
            case SP_FILTER_UNITS_USERSPACEONUSE:
                repr->setAttribute("filterUnits", "userSpaceOnUse");
                break;
            default:
                repr->setAttribute("filterUnits", "objectBoundingBox");
                break;
        }
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || filter->primitiveUnits_set) {
        switch (filter->primitiveUnits) {
            case SP_FILTER_UNITS_OBJECTBOUNDINGBOX:
                repr->setAttribute("primitiveUnits", "objectBoundingBox");
                break;
            default:
                repr->setAttribute("primitiveUnits", "userSpaceOnUse");
                break;
        }
    }

    if (filter->x._set) {
        sp_repr_set_svg_double(repr, "x", filter->x.computed);
    } else {
        repr->setAttribute("x", NULL);
    }

    if (filter->y._set) {
        sp_repr_set_svg_double(repr, "y", filter->y.computed);
    } else {
        repr->setAttribute("y", NULL);
    }

    if (filter->width._set) {
        sp_repr_set_svg_double(repr, "width", filter->width.computed);
    } else {
        repr->setAttribute("width", NULL);
    }

    if (filter->height._set) {
        sp_repr_set_svg_double(repr, "height", filter->height.computed);
    } else {
        repr->setAttribute("height", NULL);
    }

    if (filter->filterRes.getNumber()>=0) {
        gchar *tmp = filter->filterRes.getValueString();
        repr->setAttribute("filterRes", tmp);
        g_free(tmp);
    } else {
        repr->setAttribute("filterRes", NULL);
    }

    if (filter->href->getURI()) {
        gchar *uri_string = filter->href->getURI()->toString();
        repr->setAttribute("xlink:href", uri_string);
        g_free(uri_string);
    }

    SPObject::write(doc, repr, flags);

    return repr;
}


/**
 * Gets called when the filter is (re)attached to another filter.
 */
static void
filter_ref_changed(SPObject *old_ref, SPObject *ref, SPFilter *filter)
{
    if (old_ref) {
        filter->modified_connection.disconnect();
    }
    if ( SP_IS_FILTER(ref)
         && ref != filter )
    {
        filter->modified_connection =
            ref->connectModified(sigc::bind(sigc::ptr_fun(&filter_ref_modified), filter));
    }

    filter_ref_modified(ref, 0, filter);
}

static void filter_ref_modified(SPObject */*href*/, guint /*flags*/, SPFilter *filter)
{
    filter->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Callback for child_added event.
 */
void SPFilter::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPFilter* object = this;
    //SPFilter *f = SP_FILTER(object);

	SPObject::child_added(child, ref);

    object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Callback for remove_child event.
 */
void SPFilter::remove_child(Inkscape::XML::Node *child) {
	SPFilter* object = this;
	//    SPFilter *f = SP_FILTER(object);

	SPObject::remove_child(child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void sp_filter_build_renderer(SPFilter *sp_filter, Inkscape::Filters::Filter *nr_filter)
{
    g_assert(sp_filter != NULL);
    g_assert(nr_filter != NULL);

    sp_filter->_renderer = nr_filter;

    nr_filter->set_filter_units(sp_filter->filterUnits);
    nr_filter->set_primitive_units(sp_filter->primitiveUnits);
    nr_filter->set_x(sp_filter->x);
    nr_filter->set_y(sp_filter->y);
    nr_filter->set_width(sp_filter->width);
    nr_filter->set_height(sp_filter->height);

    if (sp_filter->filterRes.getNumber() >= 0) {
        if (sp_filter->filterRes.getOptNumber() >= 0) {
            nr_filter->set_resolution(sp_filter->filterRes.getNumber(),
                                      sp_filter->filterRes.getOptNumber());
        } else {
            nr_filter->set_resolution(sp_filter->filterRes.getNumber());
        }
    }

    nr_filter->clear_primitives();
    SPObject *primitive_obj = sp_filter->children;
    while (primitive_obj) {
        if (SP_IS_FILTER_PRIMITIVE(primitive_obj)) {
            SPFilterPrimitive *primitive = SP_FILTER_PRIMITIVE(primitive_obj);
            g_assert(primitive != NULL);

//            if (((SPFilterPrimitiveClass*) G_OBJECT_GET_CLASS(primitive))->build_renderer) {
//                ((SPFilterPrimitiveClass *) G_OBJECT_GET_CLASS(primitive))->build_renderer(primitive, nr_filter);
//            } else {
//                g_warning("Cannot build filter renderer: missing builder");
//            }  // CPPIFY: => FilterPrimitive should be abstract.
            primitive->build_renderer(nr_filter);
        }
        primitive_obj = primitive_obj->next;
    }
}

int sp_filter_primitive_count(SPFilter *filter) {
    g_assert(filter != NULL);
    int count = 0;

    SPObject *primitive_obj = filter->children;
    while (primitive_obj) {
        if (SP_IS_FILTER_PRIMITIVE(primitive_obj)) count++;
        primitive_obj = primitive_obj->next;
    }
    return count;
}

int sp_filter_get_image_name(SPFilter *filter, gchar const *name) {
    gchar *name_copy = strdup(name);
    map<gchar *, int, ltstr>::iterator result = filter->_image_name->find(name_copy);
    free(name_copy);
    if (result == filter->_image_name->end()) return -1;
    else return (*result).second;
}

int sp_filter_set_image_name(SPFilter *filter, gchar const *name) {
    int value = filter->_image_number_next;
    filter->_image_number_next++;
    gchar *name_copy = strdup(name);
    pair<gchar*,int> new_pair(name_copy, value);
    pair<map<gchar*,int,ltstr>::iterator,bool> ret = filter->_image_name->insert(new_pair);
    if (ret.second == false) {
        return (*ret.first).second;
    }
    return value;
}

gchar const *sp_filter_name_for_image(SPFilter const *filter, int const image) {
    switch (image) {
        case Inkscape::Filters::NR_FILTER_SOURCEGRAPHIC:
            return "SourceGraphic";
            break;
        case Inkscape::Filters::NR_FILTER_SOURCEALPHA:
            return "SourceAlpha";
            break;
        case Inkscape::Filters::NR_FILTER_BACKGROUNDIMAGE:
            return "BackgroundImage";
            break;
        case Inkscape::Filters::NR_FILTER_BACKGROUNDALPHA:
            return "BackgroundAlpha";
            break;
        case Inkscape::Filters::NR_FILTER_STROKEPAINT:
            return "StrokePaint";
            break;
        case Inkscape::Filters::NR_FILTER_FILLPAINT:
            return "FillPaint";
            break;
        case Inkscape::Filters::NR_FILTER_SLOT_NOT_SET:
        case Inkscape::Filters::NR_FILTER_UNNAMED_SLOT:
            return 0;
            break;
        default:
            for (map<gchar *, int, ltstr>::const_iterator i
                     = filter->_image_name->begin() ;
                 i != filter->_image_name->end() ; ++i) {
                if (i->second == image) {
                    return i->first;
                }
            }
    }
    return 0;
}

Glib::ustring sp_filter_get_new_result_name(SPFilter *filter) {
    g_assert(filter != NULL);
    int largest = 0;

    SPObject *primitive_obj = filter->children;
    while (primitive_obj) {
        if (SP_IS_FILTER_PRIMITIVE(primitive_obj)) {
            Inkscape::XML::Node *repr = primitive_obj->getRepr();
            char const *result = repr->attribute("result");
            int index;
            if (result)
            {
                if (sscanf(result, "result%5d", &index) == 1)
                {
                    if (index > largest)
                    {
                        largest = index;
                    }
                }
            }
        }
        primitive_obj = primitive_obj->next;
    }

    return "result" + Glib::Ascii::dtostr(largest + 1);
}

bool ltstr::operator()(const char* s1, const char* s2) const
{
    return strcmp(s1, s2) < 0;
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
