/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
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

#include <cstring>
#include <string>
#include <glibmm/i18n.h>

#include <livarot/Path.h>
#include "svg/stringstream.h"
#include "attributes.h"
#include "sp-use-reference.h"
#include "sp-tspan.h"
#include "sp-tref.h"
#include "sp-textpath.h"
#include "text-editing.h"
#include "style.h"
#include "xml/repr.h"
#include "document.h"


/*#####################################################
#  SPTSPAN
#####################################################*/

static void sp_tspan_class_init(SPTSpanClass *classname);
static void sp_tspan_init(SPTSpan *tspan);

static void sp_tspan_build(SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
static void sp_tspan_release(SPObject *object);
static void sp_tspan_set(SPObject *object, unsigned key, gchar const *value);
static void sp_tspan_update(SPObject *object, SPCtx *ctx, guint flags);
static void sp_tspan_modified(SPObject *object, unsigned flags);
static Geom::OptRect sp_tspan_bbox(SPItem const *item, Geom::Affine const &transform, SPItem::BBoxType type);
static Inkscape::XML::Node *sp_tspan_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static char *sp_tspan_description (SPItem *item);

static SPItemClass *tspan_parent_class;

/**
 *
 */
GType
sp_tspan_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPTSpanClass),
            NULL,    /* base_init */
            NULL,    /* base_finalize */
            (GClassInitFunc) sp_tspan_class_init,
            NULL,    /* class_finalize */
            NULL,    /* class_data */
            sizeof(SPTSpan),
            16,    /* n_preallocs */
            (GInstanceInitFunc) sp_tspan_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_ITEM, "SPTSpan", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_tspan_class_init(SPTSpanClass *classname)
{
    SPObjectClass * sp_object_class;
    SPItemClass * item_class;

    sp_object_class = (SPObjectClass *) classname;
    item_class = (SPItemClass *) classname;

    tspan_parent_class = (SPItemClass*)g_type_class_ref(SP_TYPE_ITEM);

    sp_object_class->build = sp_tspan_build;
    sp_object_class->release = sp_tspan_release;
    sp_object_class->set = sp_tspan_set;
    sp_object_class->update = sp_tspan_update;
    sp_object_class->modified = sp_tspan_modified;
    sp_object_class->write = sp_tspan_write;

    item_class->bbox = sp_tspan_bbox;
    item_class->description = sp_tspan_description;
}

CTSpan::CTSpan(SPTSpan* span) : CItem(span) {
	this->sptspan = span;
}

CTSpan::~CTSpan() {
}

static void
sp_tspan_init(SPTSpan *tspan)
{
	tspan->ctspan = new CTSpan(tspan);
	tspan->citem = tspan->ctspan;
	tspan->cobject = tspan->ctspan;

    tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
    new (&tspan->attributes) TextTagAttributes;
}

void CTSpan::onRelease() {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    tspan->attributes.~TextTagAttributes();

    CItem::onRelease();
}

static void
sp_tspan_release(SPObject *object)
{
	((SPTSpan*)object)->ctspan->onRelease();
}

void CTSpan::onBuild(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPTSpan* object = this->sptspan;

    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "dx" );
    object->readAttr( "dy" );
    object->readAttr( "rotate" );
    object->readAttr( "sodipodi:role" );

    CItem::onBuild(doc, repr);
}

static void
sp_tspan_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
	((SPTSpan*)object)->ctspan->onBuild(doc, repr);
}

void CTSpan::onSet(unsigned int key, const gchar* value) {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    if (tspan->attributes.readSingleAttribute(key, value)) {
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    } else {
        switch (key) {
            case SP_ATTR_SODIPODI_ROLE:
                if (value && (!strcmp(value, "line") || !strcmp(value, "paragraph"))) {
                    tspan->role = SP_TSPAN_ROLE_LINE;
                } else {
                    tspan->role = SP_TSPAN_ROLE_UNSPECIFIED;
                }
                break;
            default:
                CItem::onSet(key, value);
                break;
        }
    }
}

static void
sp_tspan_set(SPObject *object, unsigned key, gchar const *value)
{
	((SPTSpan*)object)->ctspan->onSet(key, value);
}

void CTSpan::onUpdate(SPCtx *ctx, guint flags) {
	SPTSpan* object = this->sptspan;

    CItem::onUpdate(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if ( flags || ( ochild->uflags & SP_OBJECT_MODIFIED_FLAG )) {
	    ochild->updateDisplay(ctx, flags);
        }
    }
}

static void sp_tspan_update(SPObject *object, SPCtx *ctx, guint flags)
{
	((SPTSpan*)object)->ctspan->onUpdate(ctx, flags);
}

void CTSpan::onModified(unsigned int flags) {
	SPTSpan* object = this->sptspan;

	// CPPIFY: This doesn't make no sense.
	// CObject::onModified is pure and CItem doesn't override this method. What was the idea behind these lines?
//    if (((SPObjectClass *) tspan_parent_class)->modified) {
//        ((SPObjectClass *) tspan_parent_class)->modified(object, flags);
//    }
//    CItem::onModified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if (flags || (ochild->mflags & SP_OBJECT_MODIFIED_FLAG)) {
            ochild->emitModified(flags);
        }
    }
}

static void sp_tspan_modified(SPObject *object, unsigned flags)
{
	((SPTSpan*)object)->ctspan->onModified(flags);
}

Geom::OptRect CTSpan::onBbox(Geom::Affine const &transform, SPItem::BBoxType type) {
	SPTSpan* item = this->sptspan;

    Geom::OptRect bbox;
    // find out the ancestor text which holds our layout
    SPObject const *parent_text = item;
    while (parent_text && !SP_IS_TEXT(parent_text)) {
        parent_text = parent_text->parent;
    }
    if (parent_text == NULL) {
        return bbox;
    }

    // get the bbox of our portion of the layout
    bbox = SP_TEXT(parent_text)->layout.bounds(transform, sp_text_get_length_upto(parent_text, item), sp_text_get_length_upto(item, NULL) - 1);
    if (!bbox) return bbox;

    // Add stroke width
    // FIXME this code is incorrect
    if (type == SPItem::VISUAL_BBOX && !item->style->stroke.isNone()) {
        double scale = transform.descrim();
        bbox->expandBy(0.5 * item->style->stroke_width.computed * scale);
    }
    return bbox;
}

static Geom::OptRect
sp_tspan_bbox(SPItem const *item, Geom::Affine const &transform, SPItem::BBoxType type)
{
	return ((SPTSpan*)item)->ctspan->onBbox(transform, type);
}

Inkscape::XML::Node* CTSpan::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTSpan* object = this->sptspan;

    SPTSpan *tspan = SP_TSPAN(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:tspan");
    }

    tspan->attributes.writeTo(repr);

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        GSList *l = NULL;
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr=NULL;
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                c_repr = xml_doc->createTextNode(SP_STRING(child)->string.c_str());
            }
            if ( c_repr ) {
                l = g_slist_prepend(l, c_repr);
            }
        }
        while ( l ) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::onWrite(xml_doc, repr, flags);

    return repr;
}

static Inkscape::XML::Node *
sp_tspan_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPTSpan*)object)->ctspan->onWrite(xml_doc, repr, flags);
}

gchar* CTSpan::onDescription() {
	SPTSpan* item = this->sptspan;

	g_return_val_if_fail(SP_IS_TSPAN(item), NULL);

    return g_strdup(_("<b>Text span</b>"));
}

static char *
sp_tspan_description(SPItem *item)
{
    return ((SPTSpan*)item)->ctspan->onDescription();
}


/*#####################################################
#  SPTEXTPATH
#####################################################*/

static void sp_textpath_class_init(SPTextPathClass *classname);
static void sp_textpath_init(SPTextPath *textpath);
static void sp_textpath_finalize(GObject *obj);

static void sp_textpath_build(SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
static void sp_textpath_release(SPObject *object);
static void sp_textpath_set(SPObject *object, unsigned key, gchar const *value);
static void sp_textpath_update(SPObject *object, SPCtx *ctx, guint flags);
static void sp_textpath_modified(SPObject *object, unsigned flags);
static Inkscape::XML::Node *sp_textpath_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPItemClass *textpath_parent_class;

void   refresh_textpath_source(SPTextPath* offset);


/**
 *
 */
GType
sp_textpath_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPTextPathClass),
            NULL,    /* base_init */
            NULL,    /* base_finalize */
            (GClassInitFunc) sp_textpath_class_init,
            NULL,    /* class_finalize */
            NULL,    /* class_data */
            sizeof(SPTextPath),
            16,    /* n_preallocs */
            (GInstanceInitFunc) sp_textpath_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_ITEM, "SPTextPath", &info, (GTypeFlags)0);
    }
    return type;
}

static void sp_textpath_class_init(SPTextPathClass *classname)
{
    GObjectClass  *gobject_class = reinterpret_cast<GObjectClass *>(classname);
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(classname);

    textpath_parent_class = reinterpret_cast<SPItemClass*>(g_type_class_ref(SP_TYPE_ITEM));

    gobject_class->finalize = sp_textpath_finalize;

    sp_object_class->build = sp_textpath_build;
    sp_object_class->release = sp_textpath_release;
    sp_object_class->set = sp_textpath_set;
    sp_object_class->update = sp_textpath_update;
    sp_object_class->modified = sp_textpath_modified;
    sp_object_class->write = sp_textpath_write;
}


CTextPath::CTextPath(SPTextPath* textpath) : CItem(textpath) {
	this->sptextpath = textpath;
}

CTextPath::~CTextPath() {
}

static void
sp_textpath_init(SPTextPath *textpath)
{
	textpath->ctextpath = new CTextPath(textpath);
	textpath->citem = textpath->ctextpath;
	textpath->cobject = textpath->ctextpath;

    new (&textpath->attributes) TextTagAttributes;

    textpath->startOffset._set = false;
    textpath->originalPath = NULL;
    textpath->isUpdating=false;
    // set up the uri reference
    textpath->sourcePath = new SPUsePath(textpath);
    textpath->sourcePath->user_unlink = sp_textpath_to_text;
}

static void
sp_textpath_finalize(GObject *obj)
{
    SPTextPath *textpath = (SPTextPath *) obj;

    delete textpath->sourcePath;
}

void CTextPath::onRelease() {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    textpath->attributes.~TextTagAttributes();

    if (textpath->originalPath) delete textpath->originalPath;
    textpath->originalPath = NULL;

    CItem::onRelease();
}

static void
sp_textpath_release(SPObject *object)
{
	((SPTextPath*)object)->ctextpath->onRelease();
}

void CTextPath::onBuild(SPDocument *doc, Inkscape::XML::Node *repr) {
	SPTextPath* object = this->sptextpath;

    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "dx" );
    object->readAttr( "dy" );
    object->readAttr( "rotate" );
    object->readAttr( "startOffset" );
    object->readAttr( "xlink:href" );

    bool  no_content = true;
    for (Inkscape::XML::Node* rch = repr->firstChild() ; rch != NULL; rch = rch->next()) {
        if ( rch->type() == Inkscape::XML::TEXT_NODE )
        {
            no_content = false;
            break;
        }
    }

    if ( no_content ) {
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node* rch = xml_doc->createTextNode("");
        repr->addChild(rch, NULL);
    }

    CItem::onBuild(doc, repr);
}

static void sp_textpath_build(SPObject *object, SPDocument *doc, Inkscape::XML::Node *repr)
{
	((SPTextPath*)object)->ctextpath->onBuild(doc, repr);
}

void CTextPath::onSet(unsigned int key, const gchar* value) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    if (textpath->attributes.readSingleAttribute(key, value)) {
        object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    } else {
        switch (key) {
            case SP_ATTR_XLINK_HREF:
                textpath->sourcePath->link((char*)value);
                break;
            case SP_ATTR_STARTOFFSET:
                textpath->startOffset.readOrUnset(value);
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                break;
            default:
                CItem::onSet(key, value);
                break;
        }
    }
}

static void
sp_textpath_set(SPObject *object, unsigned key, gchar const *value)
{
	((SPTextPath*)object)->ctextpath->onSet(key, value);
}

void CTextPath::onUpdate(SPCtx *ctx, guint flags) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    textpath->isUpdating = true;
    if ( textpath->sourcePath->sourceDirty ) {
        refresh_textpath_source(textpath);
    }
    textpath->isUpdating = false;

    CItem::onUpdate(ctx, flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if ( flags || ( ochild->uflags & SP_OBJECT_MODIFIED_FLAG )) {
            ochild->updateDisplay(ctx, flags);
        }
    }
}

static void sp_textpath_update(SPObject *object, SPCtx *ctx, guint flags)
{
	((SPTextPath*)object)->ctextpath->onUpdate(ctx, flags);
}


void   refresh_textpath_source(SPTextPath* tp)
{
    if ( tp == NULL ) return;
    tp->sourcePath->refresh_source();
    tp->sourcePath->sourceDirty=false;

    // finalisons
    if ( tp->sourcePath->originalPath ) {
        if (tp->originalPath) {
            delete tp->originalPath;
        }
        tp->originalPath = NULL;

        tp->originalPath = new Path;
        tp->originalPath->Copy(tp->sourcePath->originalPath);
        tp->originalPath->ConvertWithBackData(0.01);

    }
}

void CTextPath::onModified(unsigned int flags) {
	SPTextPath* object = this->sptextpath;

	// CPPIFY: This doesn't make no sense.
	// CObject::onModified is pure and CItem doesn't override this method. What was the idea behind these lines?
//    if (((SPObjectClass *) textpath_parent_class)->modified) {
//        ((SPObjectClass *) textpath_parent_class)->modified(object, flags);
//    }
//    CItem::onModified(flags);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    for ( SPObject *ochild = object->firstChild() ; ochild ; ochild = ochild->getNext() ) {
        if (flags || (ochild->mflags & SP_OBJECT_MODIFIED_FLAG)) {
            ochild->emitModified(flags);
        }
    }
}

static void sp_textpath_modified(SPObject *object, unsigned flags)
{
	((SPTextPath*)object)->ctextpath->onModified(flags);
}

Inkscape::XML::Node* CTextPath::onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTextPath* object = this->sptextpath;

    SPTextPath *textpath = SP_TEXTPATH(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:textPath");
    }

    textpath->attributes.writeTo(repr);
    if (textpath->startOffset._set) {
        if (textpath->startOffset.unit == SVGLength::PERCENT) {
	        Inkscape::SVGOStringStream os;
            os << (textpath->startOffset.computed * 100.0) << "%";
            textpath->getRepr()->setAttribute("startOffset", os.str().c_str());
        } else {
            /* FIXME: This logic looks rather undesirable if e.g. startOffset is to be
               in ems. */
            sp_repr_set_svg_double(repr, "startOffset", textpath->startOffset.computed);
        }
    }

    if ( textpath->sourcePath->sourceHref ) repr->setAttribute("xlink:href", textpath->sourcePath->sourceHref);

    if ( flags&SP_OBJECT_WRITE_BUILD ) {
        GSList *l = NULL;
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            Inkscape::XML::Node* c_repr=NULL;
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                c_repr = child->updateRepr(xml_doc, NULL, flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                c_repr = xml_doc->createTextNode(SP_STRING(child)->string.c_str());
            }
            if ( c_repr ) {
                l = g_slist_prepend(l, c_repr);
            }
        }
        while ( l ) {
            repr->addChild((Inkscape::XML::Node *) l->data, NULL);
            Inkscape::GC::release((Inkscape::XML::Node *) l->data);
            l = g_slist_remove(l, l->data);
        }
    } else {
        for (SPObject* child = object->firstChild() ; child ; child = child->getNext() ) {
            if ( SP_IS_TSPAN(child) || SP_IS_TREF(child) ) {
                child->updateRepr(flags);
            } else if ( SP_IS_TEXTPATH(child) ) {
                //c_repr = child->updateRepr(xml_doc, NULL, flags); // shouldn't happen
            } else if ( SP_IS_STRING(child) ) {
                child->getRepr()->setContent(SP_STRING(child)->string.c_str());
            }
        }
    }

    CItem::onWrite(xml_doc, repr, flags);

    return repr;
}

static Inkscape::XML::Node *
sp_textpath_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
	return ((SPTextPath*)object)->ctextpath->onWrite(xml_doc, repr, flags);
}


SPItem *
sp_textpath_get_path_item(SPTextPath *tp)
{
    if (tp && tp->sourcePath) {
        SPItem *refobj = tp->sourcePath->getObject();
        if (SP_IS_ITEM(refobj))
            return (SPItem *) refobj;
    }
    return NULL;
}

void
sp_textpath_to_text(SPObject *tp)
{
    SPObject *text = tp->parent;

    Geom::OptRect bbox = SP_ITEM(text)->geometricBounds(SP_ITEM(text)->i2doc_affine());
    if (!bbox) return;
    Geom::Point xy = bbox->min();

    // make a list of textpath children
    GSList *tp_reprs = NULL;
    for (SPObject *o = tp->firstChild() ; o != NULL; o = o->next) {
        tp_reprs = g_slist_prepend(tp_reprs, o->getRepr());
    }

    for ( GSList *i = tp_reprs ; i ; i = i->next ) {
        // make a copy of each textpath child
        Inkscape::XML::Node *copy = ((Inkscape::XML::Node *) i->data)->duplicate(text->getRepr()->document());
        // remove the old repr from under textpath
        tp->getRepr()->removeChild((Inkscape::XML::Node *) i->data);
        // put its copy under text
        text->getRepr()->addChild(copy, NULL); // fixme: copy id
    }

    //remove textpath
    tp->deleteObject();
    g_slist_free(tp_reprs);

    // set x/y on text
    /* fixme: Yuck, is this really the right test? */
    if (xy[Geom::X] != 1e18 && xy[Geom::Y] != 1e18) {
        sp_repr_set_svg_double(text->getRepr(), "x", xy[Geom::X]);
        sp_repr_set_svg_double(text->getRepr(), "y", xy[Geom::Y]);
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
