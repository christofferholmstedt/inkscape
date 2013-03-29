/*
 * SVG <clipPath> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2001-2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <cstring>
#include <string>

#include "display/drawing.h"
#include "display/drawing-group.h"
#include "xml/repr.h"

#include "enums.h"
#include "attributes.h"
#include "document.h"
#include "document-private.h"
#include "sp-item.h"
#include "style.h"

#include <2geom/transforms.h>

#include "sp-clippath.h"

struct SPClipPathView {
    SPClipPathView *next;
    unsigned int key;
    Inkscape::DrawingItem *arenaitem;
    Geom::OptRect bbox;
};

static SPClipPathView*      sp_clippath_view_new_prepend(SPClipPathView *list, unsigned int key, Inkscape::DrawingItem *arenaitem);
static SPClipPathView*      sp_clippath_view_list_remove(SPClipPathView *list, SPClipPathView *view);

G_DEFINE_TYPE(SPClipPath, sp_clippath, SP_TYPE_OBJECTGROUP);

static void
sp_clippath_class_init(SPClipPathClass *klass)
{
}

CClipPath::CClipPath(SPClipPath* clippath) : CObjectGroup(clippath) {
	this->spclippath = clippath;
}

CClipPath::~CClipPath() {
}

static void 
sp_clippath_init(SPClipPath *cp)
{
	cp->cclippath = new CClipPath(cp);

	delete cp->cobjectgroup;
	cp->cobjectgroup = cp->cclippath;
	cp->cobject = cp->cclippath;

    cp->clipPathUnits_set = FALSE;
    cp->clipPathUnits = SP_CONTENT_UNITS_USERSPACEONUSE;

    cp->display = NULL;
}

void CClipPath::onBuild(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPClipPath* object = this->spclippath;

    CObjectGroup::onBuild(doc, repr);

    object->readAttr( "style" );
    object->readAttr( "clipPathUnits" );

    /* Register ourselves */
    doc->addResource("clipPath", object);
}

void CClipPath::onRelease() {
	SPClipPath* object = this->spclippath;

    if (object->document) {
        // Unregister ourselves
        object->document->removeResource("clipPath", object);
    }

    SPClipPath *cp = SP_CLIPPATH(object);
    while (cp->display) {
        /* We simply unref and let item manage this in handler */
        cp->display = sp_clippath_view_list_remove(cp->display, cp->display);
    }

    CObjectGroup::onRelease();
}

void CClipPath::onSet(unsigned int key, const gchar* value) {
	SPClipPath* object = this->spclippath;

    SPClipPath *cp = SP_CLIPPATH(object);

    switch (key) {
        case SP_ATTR_CLIPPATHUNITS:
            cp->clipPathUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
            cp->clipPathUnits_set = FALSE;
            if (value) {
                if (!strcmp(value, "userSpaceOnUse")) {
                    cp->clipPathUnits_set = TRUE;
                } else if (!strcmp(value, "objectBoundingBox")) {
                    cp->clipPathUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;
                    cp->clipPathUnits_set = TRUE;
                }
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            if (SP_ATTRIBUTE_IS_CSS(key)) {
                sp_style_read_from_object(object->style, object);
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            } else {
                CObjectGroup::onSet(key, value);
            }
            break;
    }
}

void CClipPath::onChildAdded(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) {
	SPClipPath* object = this->spclippath;

    /* Invoke SPObjectGroup implementation */
	CObjectGroup::onChildAdded(child, ref);

    /* Show new object */
    SPObject *ochild = object->document->getObjectByRepr(child);
    if (SP_IS_ITEM(ochild)) {
        SPClipPath *cp = SP_CLIPPATH(object);
        for (SPClipPathView *v = cp->display; v != NULL; v = v->next) {
            Inkscape::DrawingItem *ac = SP_ITEM(ochild)->invoke_show(                                                  v->arenaitem->drawing(),
                                                  v->key,
                                                  SP_ITEM_REFERENCE_FLAGS);
            if (ac) {
                v->arenaitem->prependChild(ac);
            }
        }
    }
}

void CClipPath::onUpdate(SPCtx* ctx, unsigned int flags) {
	SPClipPath* object = this->spclippath;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObjectGroup *og = SP_OBJECTGROUP(object);
    GSList *l = NULL;
    for ( SPObject *child = og->firstChild(); child; child = child->getNext()) {
        g_object_ref(G_OBJECT(child));
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse(l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, flags);
        }
        g_object_unref(G_OBJECT(child));
    }

    SPClipPath *cp = SP_CLIPPATH(object);
    for (SPClipPathView *v = cp->display; v != NULL; v = v->next) {
        Inkscape::DrawingGroup *g = dynamic_cast<Inkscape::DrawingGroup *>(v->arenaitem);
        if (cp->clipPathUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && v->bbox) {
            Geom::Affine t = Geom::Scale(v->bbox->dimensions());
            t.setTranslation(v->bbox->min());
            g->setChildTransform(t);
        } else {
            g->setChildTransform(Geom::identity());
        }
    }
}

void CClipPath::onModified(unsigned int flags) {
	SPClipPath* object = this->spclippath;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    SPObjectGroup *og = SP_OBJECTGROUP(object);
    GSList *l = NULL;
    for (SPObject *child = og->firstChild(); child; child = child->getNext()) {
        g_object_ref(G_OBJECT(child));
        l = g_slist_prepend(l, child);
    }
    l = g_slist_reverse(l);
    while (l) {
        SPObject *child = SP_OBJECT(l->data);
        l = g_slist_remove(l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        g_object_unref(G_OBJECT(child));
    }
}

Inkscape::XML::Node* CClipPath::onWrite(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:clipPath");
    }

    CObjectGroup::onWrite(xml_doc, repr, flags);

    return repr;
}

Inkscape::DrawingItem *SPClipPath::show(Inkscape::Drawing &drawing, unsigned int key)
{
    Inkscape::DrawingGroup *ai = new Inkscape::DrawingGroup(drawing);
    display = sp_clippath_view_new_prepend(display, key, ai);

    for ( SPObject *child = firstChild() ; child ; child = child->getNext() ) {
        if (SP_IS_ITEM(child)) {
            Inkscape::DrawingItem *ac = SP_ITEM(child)->invoke_show(drawing, key, SP_ITEM_REFERENCE_FLAGS);
            if (ac) {
                /* The order is not important in clippath */
                ai->appendChild(ac);
            }
        }
    }

    if (clipPathUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && display->bbox) {
        Geom::Affine t = Geom::Scale(display->bbox->dimensions());
        t.setTranslation(display->bbox->min());
        ai->setChildTransform(t);
    }
    ai->setStyle(this->style);

    return ai;
}

void SPClipPath::hide(unsigned int key)
{
    for ( SPObject *child = firstChild() ; child; child = child->getNext() ) {
        if (SP_IS_ITEM(child)) {
            SP_ITEM(child)->invoke_hide(key);
        }
    }

    for (SPClipPathView *v = display; v != NULL; v = v->next) {
        if (v->key == key) {
            /* We simply unref and let item to manage this in handler */
            display = sp_clippath_view_list_remove(display, v);
            return;
        }
    }

    g_assert_not_reached();
}

void SPClipPath::setBBox(unsigned int key, Geom::OptRect const &bbox)
{
    for (SPClipPathView *v = display; v != NULL; v = v->next) {
        if (v->key == key) {
            v->bbox = bbox;
            break;
        }
    }
}

Geom::OptRect SPClipPath::geometricBounds(Geom::Affine const &transform)
{
    SPObject *i = 0;
    Geom::OptRect bbox;
    for (i = firstChild(); i; i = i->getNext()) {
        if (!SP_IS_ITEM(i)) continue;
        Geom::OptRect tmp = SP_ITEM(i)->geometricBounds(Geom::Affine(SP_ITEM(i)->transform) * transform);
        bbox.unionWith(tmp);
    }
    return bbox;
}

/* ClipPath views */

SPClipPathView *
sp_clippath_view_new_prepend(SPClipPathView *list, unsigned int key, Inkscape::DrawingItem *arenaitem)
{
    SPClipPathView *new_path_view = g_new(SPClipPathView, 1);

    new_path_view->next = list;
    new_path_view->key = key;
    new_path_view->arenaitem = arenaitem;
    new_path_view->bbox = Geom::OptRect();

    return new_path_view;
}

SPClipPathView *
sp_clippath_view_list_remove(SPClipPathView *list, SPClipPathView *view)
{
    if (view == list) {
        list = list->next;
    } else {
        SPClipPathView *prev;
        prev = list;
        while (prev->next != view) prev = prev->next;
        prev->next = view->next;
    }

    delete view->arenaitem;
    g_free(view);

    return list;
}

// Create a mask element (using passed elements), add it to <defs>
const gchar *SPClipPath::create (GSList *reprs, SPDocument *document, Geom::Affine const* applyTransform)
{
    Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:clipPath");
    repr->setAttribute("clipPathUnits", "userSpaceOnUse");

    defsrepr->appendChild(repr);
    const gchar *id = repr->attribute("id");
    SPObject *clip_path_object = document->getObjectById(id);

    for (GSList *it = reprs; it != NULL; it = it->next) {
        Inkscape::XML::Node *node = (Inkscape::XML::Node *)(it->data);
        SPItem *item = SP_ITEM(clip_path_object->appendChildRepr(node));

        if (NULL != applyTransform) {
            Geom::Affine transform (item->transform);
            transform *= (*applyTransform);
            item->doWriteTransform(item->getRepr(), transform);
        }
    }

    Inkscape::GC::release(repr);
    return id;
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
