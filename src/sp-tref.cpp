/** \file
 * SVG <tref> implementation - All character data within the referenced
 * element, including character data enclosed within additional markup,
 * will be rendered.
 *
 * This file was created based on skeleton.cpp
 */
/*
 * Authors:
 *   Gail Banaszkiewicz <Gail.Banaszkiewicz@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Gail Banaszkiewicz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glibmm/i18n.h>

#include "attributes.h"
#include "document.h"
#include "sp-object-repr.h"
#include "sp-text.h"
#include "sp-tspan.h"
#include "sp-tref.h"
#include "style.h"
#include "text-editing.h"
#include "uri.h"

#include "xml/node.h"
#include "xml/repr.h"


//#define DEBUG_TREF
#ifdef DEBUG_TREF
# define debug(f, a...) { g_message("%s(%d) %s:", \
                                  __FILE__,__LINE__,__FUNCTION__); \
                          g_message(f, ## a); \
                          g_message("\n"); \
                        }
#else
# define debug(f, a...) /**/
#endif


static void build_string_from_root(Inkscape::XML::Node *root, Glib::ustring *retString);

/* TRef base class */

static void sp_tref_finalize(GObject *obj);

static void sp_tref_href_changed(SPObject *old_ref, SPObject *ref, SPTRef *tref);
static void sp_tref_delete_self(SPObject *deleted, SPTRef *self);

G_DEFINE_TYPE(SPTRef, sp_tref, G_TYPE_OBJECT);

static void
sp_tref_class_init(SPTRefClass *tref_class)
{
    GObjectClass *gobject_class = (GObjectClass *) tref_class;
    gobject_class->finalize = sp_tref_finalize;
}

CTRef::CTRef(SPTRef* tref) : CItem(tref) {
	this->sptref = tref;
}

CTRef::~CTRef() {
}

SPTRef::SPTRef() : SPItem() {
	SPTRef* tref = this;

	tref->ctref = new CTRef(tref);
	tref->typeHierarchy.insert(typeid(SPTRef));

	delete tref->citem;
	tref->citem = tref->ctref;
	tref->cobject = tref->ctref;

	tref->stringChild = NULL;

    new (&tref->attributes) TextTagAttributes;

    tref->href = NULL;
    tref->uriOriginalRef = new SPTRefReference(tref);
    new (&tref->_delete_connection) sigc::connection();
    new (&tref->_changed_connection) sigc::connection();

    tref->_changed_connection =
        tref->uriOriginalRef->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_tref_href_changed), tref));
}

static void
sp_tref_init(SPTRef *tref)
{
	new (tref) SPTRef();
}

static void
sp_tref_finalize(GObject *obj)
{
    SPTRef *tref = (SPTRef *) obj;

    delete tref->uriOriginalRef;

    tref->_delete_connection.~connection();
    tref->_changed_connection.~connection();
}

void CTRef::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPTRef* object = this->sptref;

    CItem::build(document, repr);

    object->readAttr( "xlink:href" );
    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "dx" );
    object->readAttr( "dy" );
    object->readAttr( "rotate" );
}

void CTRef::release() {
	SPTRef* object = this->sptref;

    SPTRef *tref = SP_TREF(object);

    tref->attributes.~TextTagAttributes();

    tref->_delete_connection.disconnect();
    tref->_changed_connection.disconnect();

    g_free(tref->href);
    tref->href = NULL;

    tref->uriOriginalRef->detach();

    CItem::release();
}

void CTRef::set(unsigned int key, const gchar* value) {
	SPTRef* object = this->sptref;

    debug("0x%p %s(%u): '%s'",object,
            sp_attribute_name(key),key,value ? value : "<no value>");

    SPTRef *tref = SP_TREF(object);

    if (tref->attributes.readSingleAttribute(key, value)) { // x, y, dx, dy, rotate
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    } else if (key == SP_ATTR_XLINK_HREF) { // xlink:href
        if ( !value ) {
            // No value
            g_free(tref->href);
            tref->href = NULL;
            tref->uriOriginalRef->detach();
        } else if ((tref->href && strcmp(value, tref->href) != 0) || (!tref->href)) {

            // Value has changed

            if ( tref->href ) {
                g_free(tref->href);
                tref->href = NULL;
            }

            tref->href = g_strdup(value);

            try {
                tref->uriOriginalRef->attach(Inkscape::URI(value));
                tref->uriOriginalRef->updateObserver();
            } catch ( Inkscape::BadURIException &e ) {
                g_warning("%s", e.what());
                tref->uriOriginalRef->detach();
            }

            // No matter what happened, an update should be in order
            tref->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }

    } else { // default
        CItem::set(key, value);
    }
}

void CTRef::update(SPCtx *ctx, guint flags) {
	SPTRef* object = this->sptref;

    debug("0x%p",object);

    SPTRef *tref = SP_TREF(object);

    CItem::update(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObject *child = tref->stringChild;
    if (child) {
        if ( flags || ( child->uflags & SP_OBJECT_MODIFIED_FLAG )) {
            child->updateDisplay(ctx, flags);
        }
    }
}

void CTRef::modified(unsigned int flags) {
	SPTRef* object = this->sptref;

    SPTRef *tref_obj = SP_TREF(object);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObject *child = tref_obj->stringChild;
    if (child) {
        sp_object_ref(child);
        if (flags || (child->mflags & SP_OBJECT_MODIFIED_FLAG)) {
            child->emitModified(flags);
        }
        sp_object_unref(child);
    }
}

Inkscape::XML::Node* CTRef::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTRef* object = this->sptref;

    debug("0x%p",object);

    SPTRef *tref = SP_TREF(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:tref");
    }

    tref->attributes.writeTo(repr);

    if (tref->uriOriginalRef->getURI()) {
        gchar *uri_string = tref->uriOriginalRef->getURI()->toString();
        debug("uri_string=%s", uri_string);
        repr->setAttribute("xlink:href", uri_string);
        g_free(uri_string);
    }

    CItem::write(xml_doc, repr, flags);

    return repr;
}

Geom::OptRect CTRef::bbox(Geom::Affine const &transform, SPItem::BBoxType type) {
	SPTRef* item = this->sptref;

    Geom::OptRect bbox;
    // find out the ancestor text which holds our layout
    SPObject const *parent_text = item;
    while ( parent_text && !SP_IS_TEXT(parent_text) ) {
        parent_text = parent_text->parent;
    }
    if (parent_text == NULL) {
        return bbox;
    }

    // get the bbox of our portion of the layout
    bbox = SP_TEXT(parent_text)->layout.bounds(transform,
        sp_text_get_length_upto(parent_text, item), sp_text_get_length_upto(item, NULL) - 1);

    // Add stroke width
    // FIXME this code is incorrect
    if (bbox && type == SPItem::VISUAL_BBOX && !item->style->stroke.isNone()) {
        double scale = transform.descrim();
        bbox->expandBy(0.5 * item->style->stroke_width.computed * scale);
    }
    return bbox;
}

gchar* CTRef::description() {
	SPTRef* item = this->sptref;

    SPTRef *tref = SP_TREF(item);
    
    if (tref)
    {
        SPObject *referred = tref->getObjectReferredTo();
        
        if (tref->getObjectReferredTo()) {
            char *child_desc;
            
            if (SP_IS_ITEM(referred)) {
                child_desc = SP_ITEM(referred)->description();
            } else {
                child_desc = g_strdup("");
            }
            
            char *ret = g_strdup_printf(
                    _("<b>Cloned character data</b>%s%s"),
                    (SP_IS_ITEM(referred) ? _(" from ") : ""),
                    child_desc);
            g_free(child_desc);
            return ret;
        }
    }
    return g_strdup(_("<b>Orphaned cloned character data</b>"));
}


/* For the sigc::connection changes (i.e. when the object being refered to changes) */
static void
sp_tref_href_changed(SPObject */*old_ref*/, SPObject */*ref*/, SPTRef *tref)
{
    if (tref)
    {
        // Save a pointer to the original object being referred to
        SPObject *refRoot = tref->getObjectReferredTo();

        tref->_delete_connection.disconnect();

        if (tref->stringChild) {
            tref->detach(tref->stringChild);
            tref->stringChild = NULL;
        }

        // Ensure that we are referring to a legitimate object
        if (tref->href && refRoot && sp_tref_reference_allowed(tref, refRoot)) {

            // Update the text being referred to (will create a new string child)
            sp_tref_update_text(tref);

            // Restore the delete connection now that we're done messing with stuff
            tref->_delete_connection = refRoot->connectDelete(sigc::bind(sigc::ptr_fun(&sp_tref_delete_self), tref));
        }

    }
}


/**
 * Delete the tref object
 */
static void
sp_tref_delete_self(SPObject */*deleted*/, SPTRef *self)
{
    self->deleteObject();
}

/**
 * Return the object referred to via the URI reference
 */
SPObject * SPTRef::getObjectReferredTo(void)
{
    SPObject *referredObject = NULL;

    if (uriOriginalRef) {
        referredObject = uriOriginalRef->getObject();
    }

    return referredObject;
}


/**
 * Returns true when the given tref is allowed to refer to a particular object
 */
bool
sp_tref_reference_allowed(SPTRef *tref, SPObject *possible_ref)
{
    bool allowed = false;

    if (tref && possible_ref) {
        if (tref != possible_ref) {
            bool ancestor = false;
            for (SPObject *obj = tref; obj; obj = obj->parent) {
                if (possible_ref == obj) {
                    ancestor = true;
                    break;
                }
            }
            allowed = !ancestor;
        }
    }

    return allowed;
}


/**
 * Returns true if a tref is fully contained in the confines of the given
 * iterators and layout (or if there is no tref).
 */
bool
sp_tref_fully_contained(SPObject *start_item, Glib::ustring::iterator &start,
                             SPObject *end_item, Glib::ustring::iterator &end)
{
    bool fully_contained = false;

    if (start_item && end_item) {

        // If neither the beginning or the end is a tref then we return true (whether there
        // is a tref in the innards or not, because if there is one then it must be totally
        // contained)
        if (!(SP_IS_STRING(start_item) && SP_IS_TREF(start_item->parent))
                && !(SP_IS_STRING(end_item) && SP_IS_TREF(end_item->parent))) {
            fully_contained = true;
        }

        // Both the beginning and end are trefs; but in this case, the string iterators
        // must be at the right places
        else if ((SP_IS_STRING(start_item) && SP_IS_TREF(start_item->parent))
                && (SP_IS_STRING(end_item) && SP_IS_TREF(end_item->parent))) {
            if (start == SP_STRING(start_item)->string.begin()
                    && end == SP_STRING(start_item)->string.end()) {
                fully_contained = true;
            }
        }

        // If the beginning is a string that is a child of a tref, the iterator has to be
        // at the beginning of the item
        else if ((SP_IS_STRING(start_item) && SP_IS_TREF(start_item->parent))
                    && !(SP_IS_STRING(end_item) && SP_IS_TREF(end_item->parent))) {
            if (start == SP_STRING(start_item)->string.begin()) {
                fully_contained = true;
            }
        }

        // Same, but the for the end
        else if (!(SP_IS_STRING(start_item) && SP_IS_TREF(start_item->parent))
                    && (SP_IS_STRING(end_item) && SP_IS_TREF(end_item->parent))) {
            if (end == SP_STRING(start_item)->string.end()) {
                fully_contained = true;
            }
        }
    }

    return fully_contained;
}


void sp_tref_update_text(SPTRef *tref)
{
    if (tref) {
        // Get the character data that will be used with this tref
        Glib::ustring charData = "";
        build_string_from_root(tref->getObjectReferredTo()->getRepr(), &charData);

        if (tref->stringChild) {
            tref->detach(tref->stringChild);
            tref->stringChild = NULL;
        }

        // Create the node and SPString to be the tref's child
        Inkscape::XML::Document *xml_doc = tref->document->getReprDoc();

        Inkscape::XML::Node *newStringRepr = xml_doc->createTextNode(charData.c_str());
        tref->stringChild = SP_OBJECT(g_object_new(sp_repr_type_lookup(newStringRepr), NULL));

        // Add this SPString as a child of the tref
        tref->attach(tref->stringChild, tref->lastChild());
        sp_object_unref(tref->stringChild, NULL);
        (tref->stringChild)->invoke_build(tref->document, newStringRepr, TRUE);

        Inkscape::GC::release(newStringRepr);
    }
}



/**
 * Using depth-first search, build up a string by concatenating all SPStrings
 * found in the tree starting at the root
 */
static void
build_string_from_root(Inkscape::XML::Node *root, Glib::ustring *retString)
{
    if (root && retString) {

        // Stop and concatenate when a SPString is found
        if (root->type() == Inkscape::XML::TEXT_NODE) {
            *retString += (root->content());

            debug("%s", retString->c_str());

        // Otherwise, continue searching down the tree (with the assumption that no children nodes
        // of a SPString are actually legal)
        } else {
            Inkscape::XML::Node *childNode;
            for (childNode = root->firstChild(); childNode; childNode = childNode->next()) {
                build_string_from_root(childNode, retString);
            }
        }
    }
}

/**
 * This function will create a new tspan element with the same attributes as
 * the tref had and add the same text as a child.  The tref is replaced in the
 * tree with the new tspan.
 * The code is based partially on sp_use_unlink
 */
SPObject *
sp_tref_convert_to_tspan(SPObject *obj)
{
    SPObject * new_tspan = NULL;

    ////////////////////
    // BASE CASE
    ////////////////////
    if (SP_IS_TREF(obj)) {

        SPTRef *tref = SP_TREF(obj);

        if (tref && tref->stringChild) {
            Inkscape::XML::Node *tref_repr = tref->getRepr();
            Inkscape::XML::Node *tref_parent = tref_repr->parent();

            SPDocument *document = tref->document;
            Inkscape::XML::Document *xml_doc = document->getReprDoc();

            Inkscape::XML::Node *new_tspan_repr = xml_doc->createElement("svg:tspan");

            // Add the new tspan element just after the current tref
            tref_parent->addChild(new_tspan_repr, tref_repr);
            Inkscape::GC::release(new_tspan_repr);

            new_tspan = document->getObjectByRepr(new_tspan_repr);

            // Create a new string child for the tspan
            Inkscape::XML::Node *new_string_repr = tref->stringChild->getRepr()->duplicate(xml_doc);
            new_tspan_repr->addChild(new_string_repr, NULL);

            //SPObject * new_string_child = document->getObjectByRepr(new_string_repr);

            // Merge style from the tref
            SPStyle *new_tspan_sty = new_tspan->style;
            SPStyle const *tref_sty = tref->style;
            sp_style_merge_from_dying_parent(new_tspan_sty, tref_sty);
            sp_style_merge_from_parent(new_tspan_sty, new_tspan->parent->style);


            new_tspan->updateRepr();

            // Hold onto our SPObject and repr for now.
            sp_object_ref(tref, NULL);
            Inkscape::GC::anchor(tref_repr);

            // Remove ourselves, not propagating delete events to avoid a
            // chain-reaction with other elements that might reference us.
            tref->deleteObject(false);

            // Give the copy our old id and let go of our old repr.
            new_tspan_repr->setAttribute("id", tref_repr->attribute("id"));
            Inkscape::GC::release(tref_repr);

            // Establish the succession and let go of our object.
            tref->setSuccessor(new_tspan);
            sp_object_unref(tref, NULL);
        }
    }
    ////////////////////
    // RECURSIVE CASE
    ////////////////////
    else {
        GSList *l = NULL;
        for (SPObject *child = obj->firstChild() ; child != NULL ; child = child->getNext() ) {
            sp_object_ref(child, obj);
            l = g_slist_prepend (l, child);
        }
        l = g_slist_reverse (l);
        while (l) {
            SPObject *child = reinterpret_cast<SPObject *>(l->data); // We just built this list, so cast is safe.
            l = g_slist_remove (l, child);

            // Note that there may be more than one conversion happening here, so if it's not a
            // tref being passed into this function, the returned value can't be specifically known
            new_tspan = sp_tref_convert_to_tspan(child);

            sp_object_unref(child, obj);
        }
    }

    return new_tspan;
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
