#define __SP_GROUP_C__

/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(WIN32) || defined(__APPLE__)
# include <glibmm/i18n.h>
#endif

#include "display/nr-arena-group.h"
#include "libnr/nr-matrix-ops.h"
#include "libnr/nr-matrix-fns.h"
#include "xml/repr.h"
#include "svg/svg.h"
#include "document.h"
#include "style.h"
#include "attributes.h"

#include "sp-root.h"
#include "sp-use.h"
#include "prefs-utils.h"

static void sp_group_class_init (SPGroupClass *klass);
static void sp_group_init (SPGroup *group);
static void sp_group_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_group_release(SPObject *object);
static void sp_group_dispose(GObject *object);

static void sp_group_child_added (SPObject * object, Inkscape::XML::Node * child, Inkscape::XML::Node * ref);
static void sp_group_remove_child (SPObject * object, Inkscape::XML::Node * child);
static void sp_group_order_changed (SPObject * object, Inkscape::XML::Node * child, Inkscape::XML::Node * old_ref, Inkscape::XML::Node * new_ref);
static void sp_group_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_group_modified (SPObject *object, guint flags);
static Inkscape::XML::Node *sp_group_write (SPObject *object, Inkscape::XML::Node *repr, guint flags);
static void sp_group_set(SPObject *object, unsigned key, char const *value);

static void sp_group_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags);
static void sp_group_print (SPItem * item, SPPrintContext *ctx);
static gchar * sp_group_description (SPItem * item);
static NRArenaItem *sp_group_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_group_hide (SPItem * item, unsigned int key);
static void sp_group_snappoints (SPItem const *item, SnapPointsIter p);

static SPItemClass * parent_class;

GType
sp_group_get_type (void)
{
	static GType group_type = 0;
	if (!group_type) {
		GTypeInfo group_info = {
			sizeof (SPGroupClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_group_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPGroup),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_group_init,
			NULL,	/* value_table */
		};
		group_type = g_type_register_static (SP_TYPE_ITEM, "SPGroup", &group_info, (GTypeFlags)0);
	}
	return group_type;
}

static void
sp_group_class_init (SPGroupClass *klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = (SPItemClass *)g_type_class_ref (SP_TYPE_ITEM);

	object_class->dispose = sp_group_dispose;

	sp_object_class->child_added = sp_group_child_added;
	sp_object_class->remove_child = sp_group_remove_child;
	sp_object_class->order_changed = sp_group_order_changed;
	sp_object_class->update = sp_group_update;
	sp_object_class->modified = sp_group_modified;
	sp_object_class->set = sp_group_set;
	sp_object_class->write = sp_group_write;
	sp_object_class->release = sp_group_release;
	sp_object_class->build = sp_group_build;

	item_class->bbox = sp_group_bbox;
	item_class->print = sp_group_print;
	item_class->description = sp_group_description;
	item_class->show = sp_group_show;
	item_class->hide = sp_group_hide;
	item_class->snappoints = sp_group_snappoints;
}

static void
sp_group_init (SPGroup *group)
{
	group->_layer_mode = SPGroup::GROUP;
    group->group = new CGroup(group);
	new (&group->_display_modes) std::map<unsigned int, SPGroup::LayerMode>();
}

static void sp_group_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	sp_object_read_attr(object, "inkscape:groupmode");

	if (((SPObjectClass *)parent_class)->build) {
		((SPObjectClass *)parent_class)->build(object, document, repr);
	}
}

static void sp_group_release(SPObject *object) {
	if ( SP_GROUP(object)->_layer_mode == SPGroup::LAYER ) {
		sp_document_remove_resource(SP_OBJECT_DOCUMENT(object), "layer", object);
	}
	if (((SPObjectClass *)parent_class)->release) {
		((SPObjectClass *)parent_class)->release(object);
	}
}

static void
sp_group_dispose(GObject *object)
{
	SP_GROUP(object)->_display_modes.~map();
    delete SP_GROUP(object)->group;
}

static void
sp_group_child_added (SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
	SPItem *item;

	item = SP_ITEM (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

    SP_GROUP(object)->group->onChildAdded(child);
}

/* fixme: hide (Lauris) */

static void
sp_group_remove_child (SPObject * object, Inkscape::XML::Node * child)
{
	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

    SP_GROUP(object)->group->onChildRemoved(child);
}

static void
sp_group_order_changed (SPObject *object, Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old_ref, new_ref);

    SP_GROUP(object)->group->onOrderChanged(child, old_ref, new_ref);
}

static void
sp_group_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
    if (((SPObjectClass *) (parent_class))->update)
        ((SPObjectClass *) (parent_class))->update (object, ctx, flags);

    SP_GROUP(object)->group->onUpdate(ctx, flags);
}

static void
sp_group_modified (SPObject *object, guint flags)
{
    SP_GROUP(object)->group->onModified(flags);
}

static Inkscape::XML::Node *
sp_group_write (SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
	SPGroup *group;
	SPObject *child;
	Inkscape::XML::Node *crepr;

	group = SP_GROUP (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("svg:g");
		l = NULL;
		for (child = sp_object_first_child(object); child != NULL; child = SP_OBJECT_NEXT(child) ) {
			crepr = child->updateRepr(NULL, flags);
			if (crepr) l = g_slist_prepend (l, crepr);
		}
		while (l) {
			repr->addChild((Inkscape::XML::Node *) l->data, NULL);
			Inkscape::GC::release((Inkscape::XML::Node *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = sp_object_first_child(object) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
			child->updateRepr(flags);
		}
	}

	if ( flags & SP_OBJECT_WRITE_EXT ) {
		const char *value;
		if ( group->_layer_mode == SPGroup::LAYER ) {
			value = "layer";
		} else if ( flags & SP_OBJECT_WRITE_ALL ) {
			value = "group";
		} else {
			value = NULL;
		}
		repr->setAttribute("inkscape:groupmode", value);
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_group_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags)
{
    SP_GROUP(item)->group->calculateBBox(bbox, transform, flags);
}

static void
sp_group_print (SPItem * item, SPPrintContext *ctx)
{
    SP_GROUP(item)->group->onPrint(ctx);
}

static gchar * sp_group_description (SPItem * item)
{
    return SP_GROUP(item)->group->getDescription();
}

static void sp_group_set(SPObject *object, unsigned key, char const *value) {
	SPGroup *group=SP_GROUP(object);

	switch (key) {
		case SP_ATTR_INKSCAPE_GROUPMODE: {
			if ( value && !strcmp(value, "layer") ) {
				group->setLayerMode(SPGroup::LAYER);
			} else {
				group->setLayerMode(SPGroup::GROUP);
			}
		} break;
		default: {
			if (((SPObjectClass *) (parent_class))->set) {
				(* ((SPObjectClass *) (parent_class))->set)(object, key, value);
			}
		}
	}
}

static NRArenaItem *
sp_group_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
    return SP_GROUP(item)->group->show(arena, key, flags);
}

static void
sp_group_hide (SPItem *item, unsigned int key)
{
    SP_GROUP(item)->group->hide(key);
}

static void sp_group_snappoints (SPItem const *item, SnapPointsIter p)
{
	for (SPObject const *o = sp_object_first_child(SP_OBJECT(item));
	     o != NULL;
	     o = SP_OBJECT_NEXT(o))
	{
		if (SP_IS_ITEM(o)) {
			sp_item_snappoints(SP_ITEM(o), p);
		}
	}
}


void
sp_item_group_ungroup (SPGroup *group, GSList **children, bool do_done)
{
	g_return_if_fail (group != NULL);
	g_return_if_fail (SP_IS_GROUP (group));

	SPDocument *doc = SP_OBJECT_DOCUMENT (group);
	SPObject *root = SP_DOCUMENT_ROOT (doc);
	SPObject *defs = SP_OBJECT (SP_ROOT (root)->defs);

	SPItem *gitem = SP_ITEM (group);
	Inkscape::XML::Node *grepr = SP_OBJECT_REPR (gitem);

	g_return_if_fail (!strcmp (grepr->name(), "svg:g") || !strcmp (grepr->name(), "svg:a") || !strcmp (grepr->name(), "svg:switch"));

      // this converts the gradient/pattern fill/stroke on the group, if any, to userSpaceOnUse
      sp_item_adjust_paint_recursive (gitem, NR::identity(), NR::identity(), false);

	SPItem *pitem = SP_ITEM (SP_OBJECT_PARENT (gitem));
	Inkscape::XML::Node *prepr = SP_OBJECT_REPR (pitem);

	/* Step 1 - generate lists of children objects */
	GSList *items = NULL;
	GSList *objects = NULL;
	for (SPObject *child = sp_object_first_child(SP_OBJECT(group)) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {

		if (SP_IS_ITEM (child)) {

			SPItem *citem = SP_ITEM (child);

			/* Merging of style */
			// this converts the gradient/pattern fill/stroke, if any, to userSpaceOnUse; we need to do
			// it here _before_ the new transform is set, so as to use the pre-transform bbox
			sp_item_adjust_paint_recursive (citem, NR::identity(), NR::identity(), false);

			sp_style_merge_from_dying_parent(SP_OBJECT_STYLE(child), SP_OBJECT_STYLE(gitem));
			/*
			 * fixme: We currently make no allowance for the case where child is cloned
			 * and the group has any style settings.
			 *
			 * (This should never occur with documents created solely with the current
			 * version of inkscape without using the XML editor: we usually apply group
			 * style changes to children rather than to the group itself.)
			 *
			 * If the group has no style settings, then
			 * sp_style_merge_from_dying_parent should be a no-op.  Otherwise (i.e. if
			 * we change the child's style to compensate for its parent going away)
			 * then those changes will typically be reflected in any clones of child,
			 * whereas we'd prefer for Ungroup not to affect the visual appearance.
			 *
			 * The only way of preserving styling appearance in general is for child to
			 * be put into a new group -- a somewhat surprising response to an Ungroup
			 * command.  We could add a new groupmode:transparent that would mostly
			 * hide the existence of such groups from the user (i.e. editing behaves as
			 * if the transparent group's children weren't in a group), though that's
			 * extra complication & maintenance burden and this case is rare.
			 */

			child->updateRepr();

			Inkscape::XML::Node *nrepr = SP_OBJECT_REPR (child)->duplicate();

			// Merging transform
			NR::Matrix ctrans;
			NR::Matrix const g(gitem->transform);
			if (SP_IS_USE(citem) && sp_use_get_original (SP_USE(citem)) &&
					SP_OBJECT_PARENT (sp_use_get_original (SP_USE(citem))) == SP_OBJECT(group)) {
				// make sure a clone's effective transform is the same as was under group
				ctrans = g.inverse() * citem->transform * g;
			} else {
				ctrans = citem->transform * g;
			}

			// FIXME: constructing a transform that would fully preserve the appearance of a
			// textpath if it is ungrouped with its path seems to be impossible in general
			// case. E.g. if the group was squeezed, to keep the ungrouped textpath squeezed
			// as well, we'll need to relink it to some "virtual" path which is inversely
			// stretched relative to the actual path, and then squeeze the textpath back so it
			// would both fit the actual path _and_ be squeezed as before. It's a bummer.

			// This is just a way to temporarily remember the transform in repr. When repr is
			// reattached outside of the group, the transform will be written more properly
			// (i.e. optimized into the object if the corresponding preference is set)
			gchar affinestr[80];
			if (sp_svg_transform_write(affinestr, 79, ctrans)) {
				nrepr->setAttribute("transform", affinestr);
			} else {
				nrepr->setAttribute("transform", NULL);
			}

			items = g_slist_prepend (items, nrepr);

		} else {
			Inkscape::XML::Node *nrepr = SP_OBJECT_REPR (child)->duplicate();
			objects = g_slist_prepend (objects, nrepr);
		}
	}

	/* Step 2 - clear group */
	// remember the position of the group
	gint pos = SP_OBJECT_REPR(group)->position();

	// the group is leaving forever, no heir, clones should take note; its children however are going to reemerge
	SP_OBJECT (group)->deleteObject(true, false);

	/* Step 3 - add nonitems */
	if (objects) {
	    Inkscape::XML::Node *last_def = SP_OBJECT_REPR(defs)->lastChild();
	    while (objects) {
		SP_OBJECT_REPR(defs)->addChild((Inkscape::XML::Node *) objects->data, last_def);
		Inkscape::GC::release((Inkscape::XML::Node *) objects->data);
		objects = g_slist_remove (objects, objects->data);
	    }
	}

	/* Step 4 - add items */
	gint const preserve = prefs_get_int_attribute("options.preservetransform", "value", 0);
	while (items) {
		Inkscape::XML::Node *repr = (Inkscape::XML::Node *) items->data;
		// add item
		prepr->appendChild(repr);
		// restore position; since the items list was prepended (i.e. reverse), we now add
		// all children at the same pos, which inverts the order once again
		repr->setPosition(pos > 0 ? pos : 0);

		// fill in the children list if non-null
		SPItem *nitem = (SPItem *) doc->getObjectByRepr(repr);

		/* Optimize the transform matrix if requested. */
		// No compensations are required because this is supposed to be a non-transformation visually.
		if (!preserve) {
			NR::Matrix (*set_transform)(SPItem *, NR::Matrix const &) = ((SPItemClass *) G_OBJECT_GET_CLASS(nitem))->set_transform;
			if (set_transform) {
				sp_item_set_item_transform(nitem, set_transform(nitem, nitem->transform));
				nitem->updateRepr();
			}
		}

		Inkscape::GC::release(repr);
		if (children && SP_IS_ITEM (nitem))
			*children = g_slist_prepend (*children, nitem);

		items = g_slist_remove (items, items->data);
	}

	if (do_done) sp_document_done (doc);
}

/*
 * some API for list aspect of SPGroup
 */

GSList *
sp_item_group_item_list (SPGroup * group)
{
        GSList *s;
	SPObject *o;

	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (SP_IS_GROUP (group), NULL);

	s = NULL;

	for ( o = sp_object_first_child(SP_OBJECT(group)) ; o != NULL ; o = SP_OBJECT_NEXT(o) ) {
		if (SP_IS_ITEM (o)) {
			s = g_slist_prepend (s, o);
		}
	}

	return g_slist_reverse (s);
}

SPObject *
sp_item_group_get_child_by_name (SPGroup *group, SPObject *ref, const gchar *name)
{
	SPObject *child;
	child = (ref) ? SP_OBJECT_NEXT(ref) : sp_object_first_child(SP_OBJECT(group));
	while ( child && strcmp (SP_OBJECT_REPR(child)->name(), name) ) {
		child = SP_OBJECT_NEXT(child);
	}
	return child;
}

void SPGroup::setLayerMode(LayerMode mode) {
	if ( _layer_mode != mode ) {
		if ( mode == LAYER ) {
			sp_document_add_resource(SP_OBJECT_DOCUMENT(this), "layer", this);
		} else {
			sp_document_remove_resource(SP_OBJECT_DOCUMENT(this), "layer", this);
		}
		_layer_mode = mode;
		_updateLayerMode();
	}
}

SPGroup::LayerMode SPGroup::layerDisplayMode(unsigned int dkey) const {
	std::map<unsigned int, LayerMode>::const_iterator iter;
	iter = _display_modes.find(dkey);
	if ( iter != _display_modes.end() ) {
		return (*iter).second;
	} else {
		return GROUP;
	}
}

void SPGroup::setLayerDisplayMode(unsigned int dkey, SPGroup::LayerMode mode) {
	if ( layerDisplayMode(dkey) != mode ) {
		_display_modes[dkey] = mode;
		_updateLayerMode(dkey);
	}
}

void SPGroup::_updateLayerMode(unsigned int display_key) {
	SPItemView *view;
	for ( view = this->display ; view ; view = view->next ) {
		if ( !display_key || view->key == display_key ) {
			NRArenaGroup *arena_group=NR_ARENA_GROUP(view->arenaitem);
			if (arena_group) {
				nr_arena_group_set_transparent(arena_group, effectiveLayerMode(view->key) == SPGroup::LAYER);
			}
		}
	}
}

CGroup::CGroup(SPGroup *group) {
    _group = group;
}

CGroup::~CGroup() {
}

void CGroup::onChildAdded(Inkscape::XML::Node *child) {
    SPObject *last_child = _group->lastChild();
    if (last_child && SP_OBJECT_REPR(last_child) == child) {
        // optimization for the common special case where the child is being added at the end
        SPObject *ochild = last_child;
        if ( SP_IS_ITEM(ochild) ) {
            /* TODO: this should be moved into SPItem somehow */
            SPItemView *v;
            NRArenaItem *ac;

            for (v = _group->display; v != NULL; v = v->next) {
                ac = sp_item_invoke_show (SP_ITEM (ochild), NR_ARENA_ITEM_ARENA (v->arenaitem), v->key, v->flags);

                if (ac) {
                    nr_arena_item_append_child (v->arenaitem, ac);
                    nr_arena_item_unref (ac);
                }
            }
        }
    } else {    // general case
        SPObject *ochild = sp_object_get_child_by_repr(_group, child);
        if ( ochild && SP_IS_ITEM(ochild) ) {
            /* TODO: this should be moved into SPItem somehow */
            SPItemView *v;
            NRArenaItem *ac;

            unsigned position = sp_item_pos_in_parent(SP_ITEM(ochild));

            for (v = _group->display; v != NULL; v = v->next) {
                ac = sp_item_invoke_show (SP_ITEM (ochild), NR_ARENA_ITEM_ARENA (v->arenaitem), v->key, v->flags);

                if (ac) {
                    nr_arena_item_add_child (v->arenaitem, ac, NULL);
                    nr_arena_item_set_order (ac, position);
                    nr_arena_item_unref (ac);
                }
            }
        }
    }

    _group->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void CGroup::onChildRemoved(Inkscape::XML::Node */*child*/) {
    _group->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void CGroup::onUpdate(SPCtx *ctx, unsigned int flags) {    
    SPObject *child;
    SPItemCtx *ictx, cctx;

    ictx = (SPItemCtx *) ctx;
    cctx = *ictx;

    if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = g_slist_reverse(_childList(true, ActionUpdate));
    while (l) {
        child = SP_OBJECT (l->data);
        l = g_slist_remove (l, child);
        if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            if (SP_IS_ITEM (child)) {
                SPItem const &chi = *SP_ITEM(child);
                cctx.i2doc = chi.transform * ictx->i2doc;
                cctx.i2vp = chi.transform * ictx->i2vp;
                child->updateDisplay((SPCtx *)&cctx, flags);
            } else {
                child->updateDisplay(ctx, flags);
            }
        }
        g_object_unref (G_OBJECT (child));
    }
}

GSList *CGroup::_childList(bool add_ref, Action) {
    GSList *l = NULL;
    for (SPObject *child = sp_object_first_child(_group) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
        if (add_ref)
            g_object_ref (G_OBJECT (child));

        l = g_slist_prepend (l, child);
    }
    return l;
}

void CGroup::onModified(guint flags) {
    SPObject *child;

    if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    flags &= SP_OBJECT_MODIFIED_CASCADE;

    GSList *l = g_slist_reverse(_childList(true));
    while (l) {
        child = SP_OBJECT (l->data);
        l = g_slist_remove (l, child);
        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }
        g_object_unref (G_OBJECT (child));
    }
}

void CGroup::calculateBBox(NRRect *bbox, NR::Matrix const &transform, unsigned const flags) {
    GSList *l = _childList(false, ActionBBox);
    while (l) {
        SPObject *o = SP_OBJECT (l->data);
        if (SP_IS_ITEM(o)) {
            SPItem *child = SP_ITEM(o);
            NR::Matrix const ct(child->transform * transform);
            sp_item_invoke_bbox_full(child, bbox, ct, flags, FALSE);
        }        
        l = g_slist_remove (l, o);
    }
}

void CGroup::onPrint(SPPrintContext *ctx) {
    GSList *l = g_slist_reverse(_childList(false));
    while (l) {
        SPObject *o = SP_OBJECT (l->data);
        if (SP_IS_ITEM(o)) {
            sp_item_invoke_print (SP_ITEM (o), ctx);
        }        
        l = g_slist_remove (l, o);
    }
}

gint CGroup::getItemCount() {
    gint len = 0;
    for (SPObject *o = sp_object_first_child(SP_OBJECT(_group)) ; o != NULL ; o = SP_OBJECT_NEXT(o) ) {
        if (SP_IS_ITEM(o)) {
            len++;
        }
    }
    
    return len;
}

gchar *CGroup::getDescription() {
    gint len = getItemCount();
    return g_strdup_printf(
            ngettext("<b>Group</b> of <b>%d</b> object",
                 "<b>Group</b> of <b>%d</b> objects",
                 len), len);
}

NRArenaItem *CGroup::show (NRArena *arena, unsigned int key, unsigned int flags) {
    NRArenaItem *ai;

    ai = NRArenaGroup::create(arena);
    nr_arena_group_set_transparent(NR_ARENA_GROUP (ai),
                                   _group->effectiveLayerMode(key) ==
                         SPGroup::LAYER);

    _showChildren(arena, ai, key, flags);
    return ai;
}

void CGroup::_showChildren (NRArena *arena, NRArenaItem *ai, unsigned int key, unsigned int flags) {
    NRArenaItem *ac = NULL;
    NRArenaItem *ar = NULL;
    SPItem * child = NULL;
    GSList *l = g_slist_reverse(_childList(false, ActionShow));
    while (l) {
        SPObject *o = SP_OBJECT (l->data);
        if (SP_IS_ITEM (o)) {
            child = SP_ITEM (o);
            ac = sp_item_invoke_show (child, arena, key, flags);
            if (ac) {
                nr_arena_item_add_child (ai, ac, ar);
                ar = ac;
                nr_arena_item_unref (ac);
            }
        }
        l = g_slist_remove (l, o);
    }
}

void CGroup::hide (unsigned int key) {
    SPItem * child;

    GSList *l = g_slist_reverse(_childList(false, ActionShow));
    while (l) {
        SPObject *o = SP_OBJECT (l->data);
        if (SP_IS_ITEM (o)) {
            child = SP_ITEM (o);
            sp_item_invoke_hide (child, key);
        }
        l = g_slist_remove (l, o);
    }

    if (((SPItemClass *) parent_class)->hide)
        ((SPItemClass *) parent_class)->hide (_group, key);
}

void CGroup::onOrderChanged (Inkscape::XML::Node *child, Inkscape::XML::Node *, Inkscape::XML::Node *)
{
    SPObject *ochild = sp_object_get_child_by_repr(_group, child);
    if ( ochild && SP_IS_ITEM(ochild) ) {
        /* TODO: this should be moved into SPItem somehow */
        SPItemView *v;
        unsigned position = sp_item_pos_in_parent(SP_ITEM(ochild));
        for ( v = SP_ITEM (ochild)->display ; v != NULL ; v = v->next ) {
            nr_arena_item_set_order (v->arenaitem, position);
        }
    }

    _group->requestModified(SP_OBJECT_MODIFIED_FLAG);
}
