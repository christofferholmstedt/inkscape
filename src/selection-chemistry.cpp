#define __SP_SELECTION_CHEMISTRY_C__

/*
 * Miscellanous operations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Andrius R. <knutux@gmail.com>
 *
 * Copyright (C) 1999-2006 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtkmm/clipboard.h>

#include "svg/svg.h"
#include "inkscape.h"
#include "desktop.h"
#include "desktop-style.h"
#include "selection.h"
#include "tools-switch.h"
#include "desktop-handles.h"
#include "message-stack.h"
#include "sp-item-transform.h"
#include "marker.h"
#include "sp-use.h"
#include "sp-textpath.h"
#include "sp-tspan.h"
#include "sp-tref.h"
#include "sp-flowtext.h"
#include "sp-flowregion.h"
#include "text-editing.h"
#include "text-context.h"
#include "connector-context.h"
#include "sp-path.h"
#include "sp-conn-end.h"
#include "dropper-context.h"
#include <glibmm/i18n.h>
#include "libnr/nr-matrix-rotate-ops.h"
#include "libnr/nr-matrix-translate-ops.h"
#include "libnr/nr-rotate-fns.h"
#include "libnr/nr-scale-ops.h"
#include "libnr/nr-scale-translate-ops.h"
#include "libnr/nr-translate-matrix-ops.h"
#include "libnr/nr-translate-scale-ops.h"
#include "xml/repr.h"
#include "style.h"
#include "document-private.h"
#include "sp-gradient.h"
#include "sp-gradient-reference.h"
#include "sp-linear-gradient-fns.h"
#include "sp-pattern.h"
#include "sp-radial-gradient-fns.h"
#include "sp-namedview.h"
#include "prefs-utils.h"
#include "sp-offset.h"
#include "sp-clippath.h"
#include "sp-mask.h"
#include "file.h"
#include "helper/png-write.h"
#include "layer-fns.h"
#include "context-fns.h"
#include <map>
#include "helper/units.h"
#include "sp-item.h"
#include "unit-constants.h"
#include "xml/simple-document.h"
#include "sp-filter-reference.h"

using NR::X;
using NR::Y;

#include "selection-chemistry.h"

/* fixme: find a better place */
Inkscape::XML::Document *clipboard_document = NULL;
GSList *clipboard = NULL;
GSList *defs_clipboard = NULL;
SPCSSAttr *style_clipboard = NULL;
NR::Maybe<NR::Rect> size_clipboard;

static void sp_copy_stuff_used_by_item(GSList **defs_clip, SPItem *item, const GSList *items, Inkscape::XML::Document* xml_doc);

/**
 * Copies repr and its inherited css style elements, along with the accumulated transform 'full_t',
 * then prepends the copy to 'clip'.
 */
void sp_selection_copy_one (Inkscape::XML::Node *repr, NR::Matrix full_t, GSList **clip, Inkscape::XML::Document* xml_doc)
{
    Inkscape::XML::Node *copy = repr->duplicate(xml_doc);

    // copy complete inherited style
    SPCSSAttr *css = sp_repr_css_attr_inherited(repr, "style");
    sp_repr_css_set(copy, css, "style");
    sp_repr_css_attr_unref(css);

    // write the complete accumulated transform passed to us
    // (we're dealing with unattached repr, so we write to its attr 
    // instead of using sp_item_set_transform)
    gchar *affinestr=sp_svg_transform_write(full_t);
    copy->setAttribute("transform", affinestr);
    g_free(affinestr);

    *clip = g_slist_prepend(*clip, copy);
}

void sp_selection_copy_impl (const GSList *items, GSList **clip, GSList **defs_clip, SPCSSAttr **style_clip, Inkscape::XML::Document* xml_doc)
{

    // Copy stuff referenced by all items to defs_clip:
    if (defs_clip) {
        for (GSList *i = (GSList *) items; i != NULL; i = i->next) {
            sp_copy_stuff_used_by_item (defs_clip, SP_ITEM (i->data), items, xml_doc);
        }
        *defs_clip = g_slist_reverse(*defs_clip);
    }

    // Store style:
    if (style_clip) {
        SPItem *item = SP_ITEM (items->data); // take from the first selected item
        *style_clip = take_style_from_item (item);
    }

    if (clip) {
        // Sort items:
        GSList *sorted_items = g_slist_copy ((GSList *) items);
        sorted_items = g_slist_sort((GSList *) sorted_items, (GCompareFunc) sp_object_compare_position);

        // Copy item reprs:
        for (GSList *i = (GSList *) sorted_items; i != NULL; i = i->next) {
            sp_selection_copy_one (SP_OBJECT_REPR (i->data), sp_item_i2doc_affine(SP_ITEM (i->data)), clip, xml_doc);
        }

        *clip = g_slist_reverse(*clip);
        g_slist_free ((GSList *) sorted_items);
    }
}

/**
 * Add gradients/patterns/markers referenced by copied objects to defs.
 * Iterates through 'defs_clip', and for each item it adds the data
 * repr into the global defs.
 */
void
paste_defs (GSList **defs_clip, SPDocument *doc)
{
    if (!defs_clip)
        return;

    for (GSList *gl = *defs_clip; gl != NULL; gl = gl->next) {
        SPDefs *defs= (SPDefs *) SP_DOCUMENT_DEFS(doc);
        Inkscape::XML::Node *repr = (Inkscape::XML::Node *) gl->data;
        gchar const *id = repr->attribute("id");
        if (!id || !doc->getObjectById(id)) {
            Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
            Inkscape::XML::Node *copy = repr->duplicate(xml_doc);
            SP_OBJECT_REPR(defs)->addChild(copy, NULL);
            Inkscape::GC::release(copy);
        }
    }
}

GSList *sp_selection_paste_impl (SPDocument *doc, SPObject *parent, GSList **clip, GSList **defs_clip)
{
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
    paste_defs (defs_clip, doc);

    GSList *copied = NULL;
    // add objects to document
    for (GSList *l = *clip; l != NULL; l = l->next) {
        Inkscape::XML::Node *repr = (Inkscape::XML::Node *) l->data;
        Inkscape::XML::Node *copy = repr->duplicate(xml_doc);

        // premultiply the item transform by the accumulated parent transform in the paste layer
        NR::Matrix local = sp_item_i2doc_affine(SP_ITEM(parent));
        if (!local.test_identity()) {
            gchar const *t_str = copy->attribute("transform");
            NR::Matrix item_t (NR::identity());
            if (t_str)
                sp_svg_transform_read(t_str, &item_t);
            item_t *= local.inverse();
            // (we're dealing with unattached repr, so we write to its attr instead of using sp_item_set_transform)
            gchar *affinestr=sp_svg_transform_write(item_t);
            copy->setAttribute("transform", affinestr);
            g_free(affinestr);
        }

        parent->appendChildRepr(copy);
        copied = g_slist_prepend(copied, copy);
        Inkscape::GC::release(copy);
    }
    return copied;
}

void sp_selection_delete_impl(const GSList *items)
{
    for (const GSList *i = items ; i ; i = i->next ) {
        sp_object_ref((SPObject *)i->data, NULL);
    }
    for (const GSList *i = items; i != NULL; i = i->next) {
        SPItem *item = (SPItem *) i->data;
        SP_OBJECT(item)->deleteObject();
        sp_object_unref((SPObject *)item, NULL);
    }
}


void sp_selection_delete()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL) {
        return;
    }

    if (tools_isactive (desktop, TOOLS_TEXT))
        if (sp_text_delete_selection(desktop->event_context)) {
            sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_TEXT,
                             _("Delete text"));
            return;
        }

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("<b>Nothing</b> was deleted."));
        return;
    }

    const GSList *selected = g_slist_copy(const_cast<GSList *>(selection->itemList()));
    selection->clear();
    sp_selection_delete_impl (selected);
    g_slist_free ((GSList *) selected);

    /* a tool may have set up private information in it's selection context
     * that depends on desktop items.  I think the only sane way to deal with
     * this currently is to reset the current tool, which will reset it's
     * associated selection context.  For example: deleting an object
     * while moving it around the canvas.
     */
    tools_switch ( desktop, tools_active ( desktop ) );

    sp_document_done(sp_desktop_document(desktop), SP_VERB_EDIT_DELETE, 
                     _("Delete"));
}

/* fixme: sequencing */
void sp_selection_duplicate()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::XML::Document* xml_doc = sp_document_repr_doc(desktop->doc());
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to duplicate."));
        return;
    }

    GSList *reprs = g_slist_copy((GSList *) selection->reprList());

    selection->clear();

    // sorting items from different parents sorts each parent's subset without possibly mixing them, just what we need
    reprs = g_slist_sort(reprs, (GCompareFunc) sp_repr_compare_position);

    GSList *newsel = NULL;

    while (reprs) {
        Inkscape::XML::Node *parent = ((Inkscape::XML::Node *) reprs->data)->parent();
        Inkscape::XML::Node *copy = ((Inkscape::XML::Node *) reprs->data)->duplicate(xml_doc);

        parent->appendChild(copy);

        newsel = g_slist_prepend(newsel, copy);
        reprs = g_slist_remove(reprs, reprs->data);
        Inkscape::GC::release(copy);
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_EDIT_DUPLICATE, 
                     _("Duplicate"));

    selection->setReprList(newsel);

    g_slist_free(newsel);
}

void sp_edit_clear_all()
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    if (!dt)
        return;

    SPDocument *doc = sp_desktop_document(dt);
    sp_desktop_selection(dt)->clear();

    g_return_if_fail(SP_IS_GROUP(dt->currentLayer()));
    GSList *items = sp_item_group_item_list(SP_GROUP(dt->currentLayer()));

    while (items) {
        SP_OBJECT (items->data)->deleteObject();
        items = g_slist_remove(items, items->data);
    }

    sp_document_done(doc, SP_VERB_EDIT_CLEAR_ALL,
                     _("Delete all"));
}

GSList *
get_all_items (GSList *list, SPObject *from, SPDesktop *desktop, bool onlyvisible, bool onlysensitive, const GSList *exclude)
{
    for (SPObject *child = sp_object_first_child(SP_OBJECT(from)) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
        if (SP_IS_ITEM(child) &&
            !desktop->isLayer(SP_ITEM(child)) &&
            (!onlysensitive || !SP_ITEM(child)->isLocked()) &&
            (!onlyvisible || !desktop->itemIsHidden(SP_ITEM(child))) &&
            (!exclude || !g_slist_find ((GSList *) exclude, child))
            )
        {
            list = g_slist_prepend (list, SP_ITEM(child));
        }

        if (SP_IS_ITEM(child) && desktop->isLayer(SP_ITEM(child))) {
            list = get_all_items (list, child, desktop, onlyvisible, onlysensitive, exclude);
        }
    }

    return list;
}

void sp_edit_select_all_full (bool force_all_layers, bool invert)
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    if (!dt)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(dt);

    g_return_if_fail(SP_IS_GROUP(dt->currentLayer()));

    PrefsSelectionContext inlayer = (PrefsSelectionContext)prefs_get_int_attribute ("options.kbselection", "inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs_get_int_attribute ("options.kbselection", "onlyvisible", 1);
    bool onlysensitive = prefs_get_int_attribute ("options.kbselection", "onlysensitive", 1);

    GSList *items = NULL;

    const GSList *exclude = NULL;
    if (invert) {
        exclude = selection->itemList();
    }

    if (force_all_layers)
        inlayer = PREFS_SELECTION_ALL;

    switch (inlayer) {
        case PREFS_SELECTION_LAYER: {
        if ( (onlysensitive && SP_ITEM(dt->currentLayer())->isLocked()) ||
             (onlyvisible && dt->itemIsHidden(SP_ITEM(dt->currentLayer()))) )
        return;

        GSList *all_items = sp_item_group_item_list(SP_GROUP(dt->currentLayer()));

        for (GSList *i = all_items; i; i = i->next) {
            SPItem *item = SP_ITEM (i->data);

            if (item && (!onlysensitive || !item->isLocked())) {
                if (!onlyvisible || !dt->itemIsHidden(item)) {
                    if (!dt->isLayer(item)) {
                        if (!invert || !g_slist_find ((GSList *) exclude, item)) {
                            items = g_slist_prepend (items, item); // leave it in the list
                        }
                    }
                }
            }
        }

        g_slist_free (all_items);
            break;
        }
        case PREFS_SELECTION_LAYER_RECURSIVE: {
            items = get_all_items (NULL, dt->currentLayer(), dt, onlyvisible, onlysensitive, exclude);
            break;
        }
        default: {
        items = get_all_items (NULL, dt->currentRoot(), dt, onlyvisible, onlysensitive, exclude);
            break;
    }
    }

    selection->setList (items);

    if (items) {
        g_slist_free (items);
    }
}

void sp_edit_select_all ()
{
    sp_edit_select_all_full (false, false);
}

void sp_edit_select_all_in_all_layers ()
{
    sp_edit_select_all_full (true, false);
}

void sp_edit_invert ()
{
    sp_edit_select_all_full (false, true);
}

void sp_edit_invert_in_all_layers ()
{
    sp_edit_select_all_full (true, true);
}

void sp_selection_group()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *doc = sp_desktop_document (desktop);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // Check if something is selected.
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>some objects</b> to group."));
        return;
    }

    GSList const *l = (GSList *) selection->reprList();

    GSList *p = g_slist_copy((GSList *) l);

    selection->clear();

    p = g_slist_sort(p, (GCompareFunc) sp_repr_compare_position);

    // Remember the position and parent of the topmost object.
    gint topmost = ((Inkscape::XML::Node *) g_slist_last(p)->data)->position();
    Inkscape::XML::Node *topmost_parent = ((Inkscape::XML::Node *) g_slist_last(p)->data)->parent();

    Inkscape::XML::Node *group = xml_doc->createElement("svg:g");

    while (p) {
        Inkscape::XML::Node *current = (Inkscape::XML::Node *) p->data;

        if (current->parent() == topmost_parent) {
            Inkscape::XML::Node *spnew = current->duplicate(xml_doc);
            sp_repr_unparent(current);
            group->appendChild(spnew);
            Inkscape::GC::release(spnew);
            topmost --; // only reduce count for those items deleted from topmost_parent
        } else { // move it to topmost_parent first
                GSList *temp_clip = NULL;

                // At this point, current may already have no item, due to its being a clone whose original is already moved away
                // So we copy it artificially calculating the transform from its repr->attr("transform") and the parent transform
                gchar const *t_str = current->attribute("transform");
                NR::Matrix item_t (NR::identity());
                if (t_str)
                    sp_svg_transform_read(t_str, &item_t);
                item_t *= sp_item_i2doc_affine(SP_ITEM(doc->getObjectByRepr(current->parent())));
                //FIXME: when moving both clone and original from a transformed group (either by
                //grouping into another parent, or by cut/paste) the transform from the original's
                //parent becomes embedded into original itself, and this affects its clones. Fix
                //this by remembering the transform diffs we write to each item into an array and
                //then, if this is clone, looking up its original in that array and pre-multiplying
                //it by the inverse of that original's transform diff.

                sp_selection_copy_one (current, item_t, &temp_clip, xml_doc);
                sp_repr_unparent(current);

                // paste into topmost_parent (temporarily)
                GSList *copied = sp_selection_paste_impl (doc, doc->getObjectByRepr(topmost_parent), &temp_clip, NULL);
                if (temp_clip) g_slist_free (temp_clip);
                if (copied) { // if success,
                    // take pasted object (now in topmost_parent)
                    Inkscape::XML::Node *in_topmost = (Inkscape::XML::Node *) copied->data;
                    // make a copy
                    Inkscape::XML::Node *spnew = in_topmost->duplicate(xml_doc);
                    // remove pasted
                    sp_repr_unparent(in_topmost);
                    // put its copy into group
                    group->appendChild(spnew);
                    Inkscape::GC::release(spnew);
                    g_slist_free (copied);
                }
        }
        p = g_slist_remove(p, current);
    }

    // Add the new group to the topmost members' parent
    topmost_parent->appendChild(group);

    // Move to the position of the topmost, reduced by the number of items deleted from topmost_parent
    group->setPosition(topmost + 1);

    sp_document_done(sp_desktop_document(desktop), SP_VERB_SELECTION_GROUP, 
                     _("Group"));

    selection->set(group);
    Inkscape::GC::release(group);
}

void sp_selection_ungroup()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select a <b>group</b> to ungroup."));
        return;
    }

    GSList *items = g_slist_copy((GSList *) selection->itemList());
    selection->clear();

    // Get a copy of current selection.
    GSList *new_select = NULL;
    bool ungrouped = false;
    for (GSList *i = items;
         i != NULL;
         i = i->next)
    {
        SPItem *group = (SPItem *) i->data;

        // when ungrouping cloned groups with their originals, some objects that were selected may no more exist due to unlinking
        if (!SP_IS_OBJECT(group)) {
            continue;
        }

        /* We do not allow ungrouping <svg> etc. (lauris) */
        if (strcmp(SP_OBJECT_REPR(group)->name(), "svg:g") && strcmp(SP_OBJECT_REPR(group)->name(), "svg:switch")) {
            // keep the non-group item in the new selection
            selection->add(group);
            continue;
        }

        GSList *children = NULL;
        /* This is not strictly required, but is nicer to rely on group ::destroy (lauris) */
        sp_item_group_ungroup(SP_GROUP(group), &children, false);
        ungrouped = true;
        // Add ungrouped items to the new selection.
        new_select = g_slist_concat(new_select, children);
    }

    if (new_select) { // Set new selection.
        selection->addList(new_select);
        g_slist_free(new_select);
    }
    if (!ungrouped) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No groups</b> to ungroup in the selection."));
    }

    g_slist_free(items);

    sp_document_done(sp_desktop_document(desktop), SP_VERB_SELECTION_UNGROUP, 
                     _("Ungroup"));
}

static SPGroup *
sp_item_list_common_parent_group(const GSList *items)
{
    if (!items) {
        return NULL;
    }
    SPObject *parent = SP_OBJECT_PARENT(items->data);
    /* Strictly speaking this CAN happen, if user selects <svg> from Inkscape::XML editor */
    if (!SP_IS_GROUP(parent)) {
        return NULL;
    }
    for (items = items->next; items; items = items->next) {
        if (SP_OBJECT_PARENT(items->data) != parent) {
            return NULL;
        }
    }

    return SP_GROUP(parent);
}

/** Finds out the minimum common bbox of the selected items
 */
static NR::Maybe<NR::Rect>
enclose_items(const GSList *items)
{
    g_assert(items != NULL);

    NR::Maybe<NR::Rect> r = NR::Nothing();
    for (GSList const *i = items; i; i = i->next) {
        r = NR::union_bounds(r, sp_item_bbox_desktop((SPItem *) i->data));
    }
    return r;
}

SPObject *
prev_sibling(SPObject *child)
{
    SPObject *parent = SP_OBJECT_PARENT(child);
    if (!SP_IS_GROUP(parent)) {
        return NULL;
    }
    for ( SPObject *i = sp_object_first_child(parent) ; i; i = SP_OBJECT_NEXT(i) ) {
        if (i->next == child)
            return i;
    }
    return NULL;
}

void
sp_selection_raise()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    GSList const *items = (GSList *) selection->itemList();
    if (!items) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to raise."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items);
    if (!group) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    Inkscape::XML::Node *grepr = SP_OBJECT_REPR(group);

    /* construct reverse-ordered list of selected children */
    GSList *rev = g_slist_copy((GSList *) items);
    rev = g_slist_sort(rev, (GCompareFunc) sp_item_repr_compare_position);

    // find out the common bbox of the selected items
    NR::Maybe<NR::Rect> selected = enclose_items(items);

    // for all objects in the selection (starting from top)
    if (selected) {
        while (rev) {
            SPObject *child = SP_OBJECT(rev->data);
            // for each selected object, find the next sibling
            for (SPObject *newref = child->next; newref; newref = newref->next) {
                // if the sibling is an item AND overlaps our selection,
                if (SP_IS_ITEM(newref)) {
                    NR::Maybe<NR::Rect> newref_bbox = sp_item_bbox_desktop(SP_ITEM(newref));
                    if ( newref_bbox && selected->intersects(*newref_bbox) ) {
                        // AND if it's not one of our selected objects,
                        if (!g_slist_find((GSList *) items, newref)) {
                            // move the selected object after that sibling
                            grepr->changeOrder(SP_OBJECT_REPR(child), SP_OBJECT_REPR(newref));
                        }
                        break;
                    }
                }
            }
            rev = g_slist_remove(rev, child);
        }
    } else {
        g_slist_free(rev);
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_SELECTION_RAISE,
                     _("Raise"));
}

void sp_selection_raise_to_top()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *document = sp_desktop_document(desktop);
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to raise to top."));
        return;
    }

    GSList const *items = (GSList *) selection->itemList();

    SPGroup const *group = sp_item_list_common_parent_group(items);
    if (!group) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    GSList *rl = g_slist_copy((GSList *) selection->reprList());
    rl = g_slist_sort(rl, (GCompareFunc) sp_repr_compare_position);

    for (GSList *l = rl; l != NULL; l = l->next) {
        Inkscape::XML::Node *repr = (Inkscape::XML::Node *) l->data;
        repr->setPosition(-1);
    }

    g_slist_free(rl);

    sp_document_done(document, SP_VERB_SELECTION_TO_FRONT, 
                     _("Raise to top"));
}

void
sp_selection_lower()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    GSList const *items = (GSList *) selection->itemList();
    if (!items) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to lower."));
        return;
    }

    SPGroup const *group = sp_item_list_common_parent_group(items);
    if (!group) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    Inkscape::XML::Node *grepr = SP_OBJECT_REPR(group);

    // find out the common bbox of the selected items
    NR::Maybe<NR::Rect> selected = enclose_items(items);

    /* construct direct-ordered list of selected children */
    GSList *rev = g_slist_copy((GSList *) items);
    rev = g_slist_sort(rev, (GCompareFunc) sp_item_repr_compare_position);
    rev = g_slist_reverse(rev);

    // for all objects in the selection (starting from top)
    if (selected) {
        while (rev) {
            SPObject *child = SP_OBJECT(rev->data);
            // for each selected object, find the prev sibling
            for (SPObject *newref = prev_sibling(child); newref; newref = prev_sibling(newref)) {
                // if the sibling is an item AND overlaps our selection,
                if (SP_IS_ITEM(newref)) {
                    NR::Maybe<NR::Rect> ref_bbox = sp_item_bbox_desktop(SP_ITEM(newref));
                    if ( ref_bbox && selected->intersects(*ref_bbox) ) {
                        // AND if it's not one of our selected objects,
                        if (!g_slist_find((GSList *) items, newref)) {
                            // move the selected object before that sibling
                            SPObject *put_after = prev_sibling(newref);
                            if (put_after)
                                grepr->changeOrder(SP_OBJECT_REPR(child), SP_OBJECT_REPR(put_after));
                            else
                                SP_OBJECT_REPR(child)->setPosition(0);
                        }
                        break;
                    }
                }
            }
            rev = g_slist_remove(rev, child);
        }
    } else {
        g_slist_free(rev);
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_SELECTION_LOWER, 
                     _("Lower"));
}

void sp_selection_lower_to_bottom()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *document = sp_desktop_document(desktop);
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to lower to bottom."));
        return;
    }

    GSList const *items = (GSList *) selection->itemList();

    SPGroup const *group = sp_item_list_common_parent_group(items);
    if (!group) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You cannot raise/lower objects from <b>different groups</b> or <b>layers</b>."));
        return;
    }

    GSList *rl;
    rl = g_slist_copy((GSList *) selection->reprList());
    rl = g_slist_sort(rl, (GCompareFunc) sp_repr_compare_position);
    rl = g_slist_reverse(rl);

    for (GSList *l = rl; l != NULL; l = l->next) {
        gint minpos;
        SPObject *pp, *pc;
        Inkscape::XML::Node *repr = (Inkscape::XML::Node *) l->data;
        pp = document->getObjectByRepr(sp_repr_parent(repr));
        minpos = 0;
        g_assert(SP_IS_GROUP(pp));
        pc = sp_object_first_child(pp);
        while (!SP_IS_ITEM(pc)) {
            minpos += 1;
            pc = pc->next;
        }
        repr->setPosition(minpos);
    }

    g_slist_free(rl);

    sp_document_done(document, SP_VERB_SELECTION_TO_BACK, 
                     _("Lower to bottom"));
}

void
sp_undo(SPDesktop *desktop, SPDocument *)
{
        if (!sp_document_undo(sp_desktop_document(desktop)))
            desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing to undo."));
}

void
sp_redo(SPDesktop *desktop, SPDocument *)
{
        if (!sp_document_redo(sp_desktop_document(desktop)))
            desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing to redo."));
}

void sp_selection_cut()
{
    sp_selection_copy();
    sp_selection_delete();
}

void sp_copy_gradient (GSList **defs_clip, SPGradient *gradient, Inkscape::XML::Document* xml_doc)
{
    SPGradient *ref = gradient;

    while (ref) {
        // climb up the refs, copying each one in the chain
        Inkscape::XML::Node *grad_repr = SP_OBJECT_REPR(ref)->duplicate(xml_doc);
        *defs_clip = g_slist_prepend (*defs_clip, grad_repr);

        ref = ref->ref->getObject();
    }
}

void sp_copy_pattern (GSList **defs_clip, SPPattern *pattern, Inkscape::XML::Document* xml_doc)
{
    SPPattern *ref = pattern;

    while (ref) {
        // climb up the refs, copying each one in the chain
        Inkscape::XML::Node *pattern_repr = SP_OBJECT_REPR(ref)->duplicate(xml_doc);
        *defs_clip = g_slist_prepend (*defs_clip, pattern_repr);

        // items in the pattern may also use gradients and other patterns, so we need to recurse here as well
        for (SPObject *child = sp_object_first_child(SP_OBJECT(ref)) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
            if (!SP_IS_ITEM (child))
                continue;
            sp_copy_stuff_used_by_item (defs_clip, (SPItem *) child, NULL, xml_doc);
        }

        ref = ref->ref->getObject();
    }
}

void sp_copy_single (GSList **defs_clip, SPObject *thing, Inkscape::XML::Document* xml_doc)
{
    Inkscape::XML::Node *duplicate_repr = SP_OBJECT_REPR(thing)->duplicate(xml_doc);
    *defs_clip = g_slist_prepend (*defs_clip, duplicate_repr);
}


void sp_copy_textpath_path (GSList **defs_clip, SPTextPath *tp, const GSList *items, Inkscape::XML::Document* xml_doc)
{
    SPItem *path = sp_textpath_get_path_item (tp);
    if (!path)
        return;
    if (items && g_slist_find ((GSList *) items, path)) // do not copy it to defs if it is already in the list of items copied
        return;
    Inkscape::XML::Node *repr = SP_OBJECT_REPR(path)->duplicate(xml_doc);
    *defs_clip = g_slist_prepend (*defs_clip, repr);
}

/**
 * Copies things like patterns, markers, gradients, etc.
 */
void sp_copy_stuff_used_by_item (GSList **defs_clip, SPItem *item, const GSList *items, Inkscape::XML::Document* xml_doc)
{
    SPStyle *style = SP_OBJECT_STYLE (item);

    if (style && (style->fill.type == SP_PAINT_TYPE_PAINTSERVER)) {
        SPObject *server = SP_OBJECT_STYLE_FILL_SERVER(item);
        if (SP_IS_LINEARGRADIENT (server) || SP_IS_RADIALGRADIENT (server))
            sp_copy_gradient (defs_clip, SP_GRADIENT(server), xml_doc);
        if (SP_IS_PATTERN (server))
            sp_copy_pattern (defs_clip, SP_PATTERN(server), xml_doc);
    }

    if (style && (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER)) {
        SPObject *server = SP_OBJECT_STYLE_STROKE_SERVER(item);
        if (SP_IS_LINEARGRADIENT (server) || SP_IS_RADIALGRADIENT (server))
            sp_copy_gradient (defs_clip, SP_GRADIENT(server), xml_doc);
        if (SP_IS_PATTERN (server))
            sp_copy_pattern (defs_clip, SP_PATTERN(server), xml_doc);
    }

    // For shapes, copy all of the shape's markers into defs_clip
    if (SP_IS_SHAPE (item)) {
        SPShape *shape = SP_SHAPE (item);
        for (int i = 0 ; i < SP_MARKER_LOC_QTY ; i++) {
            if (shape->marker[i]) {
                sp_copy_single (defs_clip, SP_OBJECT (shape->marker[i]), xml_doc);
            }
        }
    }

    if (SP_IS_TEXT_TEXTPATH (item)) {
        sp_copy_textpath_path (defs_clip, SP_TEXTPATH(sp_object_first_child(SP_OBJECT(item))), items, xml_doc);
    }

    if (item->clip_ref->getObject()) {
        sp_copy_single (defs_clip, item->clip_ref->getObject(), xml_doc);
    }

    if (item->mask_ref->getObject()) {
        SPObject *mask = item->mask_ref->getObject();
        sp_copy_single (defs_clip, mask, xml_doc);
        // recurse into the mask for its gradients etc.
        for (SPObject *o = SP_OBJECT(mask)->children; o != NULL; o = o->next) {
            if (SP_IS_ITEM(o))
                sp_copy_stuff_used_by_item (defs_clip, SP_ITEM (o), items, xml_doc);
        }
    }

    if (style->getFilter()) {
        SPObject *filter = style->getFilter();
        if (SP_IS_FILTER(filter)) {
            sp_copy_single (defs_clip, filter, xml_doc);
        }
    }

    // recurse
    for (SPObject *o = SP_OBJECT(item)->children; o != NULL; o = o->next) {
        if (SP_IS_ITEM(o))
            sp_copy_stuff_used_by_item (defs_clip, SP_ITEM (o), items, xml_doc);
    }
}

/**
 * \pre item != NULL
 */
SPCSSAttr *
take_style_from_item (SPItem *item)
{
    // write the complete cascaded style, context-free
    SPCSSAttr *css = sp_css_attr_from_object (SP_OBJECT(item), SP_STYLE_FLAG_ALWAYS);
    if (css == NULL)
        return NULL;

    if ((SP_IS_GROUP(item) && SP_OBJECT(item)->children) ||
        (SP_IS_TEXT (item) && SP_OBJECT(item)->children && SP_OBJECT(item)->children->next == NULL)) {
        // if this is a text with exactly one tspan child, merge the style of that tspan as well
        // If this is a group, merge the style of its topmost (last) child with style
        for (SPObject *last_element = item->lastChild(); last_element != NULL; last_element = SP_OBJECT_PREV (last_element)) {
            if (SP_OBJECT_STYLE (last_element) != NULL) {
                SPCSSAttr *temp = sp_css_attr_from_object (last_element, SP_STYLE_FLAG_IFSET);
                if (temp) {
                    sp_repr_css_merge (css, temp);
                    sp_repr_css_attr_unref (temp);
                }
                break;
            }
        }
    }
    if (!(SP_IS_TEXT (item) || SP_IS_TSPAN (item) || SP_IS_TREF(item) || SP_IS_STRING (item))) {
        // do not copy text properties from non-text objects, it's confusing
        css = sp_css_attr_unset_text (css);
    }

    // FIXME: also transform gradient/pattern fills, by forking? NO, this must be nondestructive
    double ex = NR::expansion(sp_item_i2doc_affine(item));
    if (ex != 1.0) {
        css = sp_css_attr_scale (css, ex);
    }

    return css;
}


void sp_selection_copy()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    if (!clipboard_document) {
        clipboard_document = new Inkscape::XML::SimpleDocument();
    }

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (tools_isactive (desktop, TOOLS_DROPPER)) {
        sp_dropper_context_copy(desktop->event_context);
        return; // copied color under cursor, nothing else to do
    }

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing was copied."));
        return;
    }

    const GSList *items = g_slist_copy ((GSList *) selection->itemList());

    // 0. Copy text to system clipboard
    // FIXME: for non-texts, put serialized Inkscape::XML as text to the clipboard;
    //for this sp_repr_write_stream needs to be rewritten with iostream instead of FILE
    Glib::ustring text;
    if (tools_isactive (desktop, TOOLS_TEXT)) {
        text = sp_text_get_selected_text(desktop->event_context);
    }

    if (text.empty()) {
        guint texts = 0;
        for (GSList *i = (GSList *) items; i; i = i->next) {
            SPItem *item = SP_ITEM (i->data);
            if (SP_IS_TEXT (item) || SP_IS_FLOWTEXT(item)) {
                if (texts > 0) // if more than one text object is copied, separate them by spaces
                    text += " ";
                gchar *this_text = sp_te_get_string_multiline (item);
                if (this_text) {
                    text += this_text;
                    g_free(this_text);
                }
                texts++;
            }
        }
    }
    if (!text.empty()) {
        Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
        refClipboard->set_text(text);
    }

    // clear old defs clipboard
    while (defs_clipboard) {
        Inkscape::GC::release((Inkscape::XML::Node *) defs_clipboard->data);
        defs_clipboard = g_slist_remove (defs_clipboard, defs_clipboard->data);
    }

    // clear style clipboard
    if (style_clipboard) {
        sp_repr_css_attr_unref (style_clipboard);
        style_clipboard = NULL;
    }

    //clear main clipboard
    while (clipboard) {
        Inkscape::GC::release((Inkscape::XML::Node *) clipboard->data);
        clipboard = g_slist_remove(clipboard, clipboard->data);
    }

    sp_selection_copy_impl (items, &clipboard, &defs_clipboard, &style_clipboard, clipboard_document);

    if (tools_isactive (desktop, TOOLS_TEXT)) { // take style from cursor/text selection, overwriting the style just set by copy_impl
        SPStyle *const query = sp_style_new(SP_ACTIVE_DOCUMENT);
        if (sp_desktop_query_style_all (desktop, query)) {
            SPCSSAttr *css = sp_css_attr_from_style (query, SP_STYLE_FLAG_ALWAYS);
            if (css != NULL) {
                // clear style clipboard
                if (style_clipboard) {
                    sp_repr_css_attr_unref (style_clipboard);
                    style_clipboard = NULL;
                }
                //sp_repr_css_print (css);
                style_clipboard = css;
            }
        }
        sp_style_unref(query);
    }

    size_clipboard = selection->bounds();

    g_slist_free ((GSList *) items);
}

void sp_selection_paste(bool in_place)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    if (desktop == NULL) {
        return;
    }

    SPDocument *document = sp_desktop_document(desktop);

    if (Inkscape::have_viable_layer(desktop, desktop->messageStack()) == false) {
        return;
    }

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (tools_isactive (desktop, TOOLS_TEXT)) {
        if (sp_text_paste_inline(desktop->event_context))
            return; // pasted from system clipboard into text, nothing else to do
    }

    // check if something is in the clipboard
    if (clipboard == NULL) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing on the clipboard."));
        return;
    }

    GSList *copied = sp_selection_paste_impl(document, desktop->currentLayer(), &clipboard, &defs_clipboard);
    // add pasted objects to selection
    selection->setReprList((GSList const *) copied);
    g_slist_free (copied);

    if (!in_place) {
        sp_document_ensure_up_to_date(document);

        NR::Maybe<NR::Rect> sel_bbox = selection->bounds();
        NR::Point m( desktop->point() );
        if (sel_bbox) {
            m -= sel_bbox->midpoint();
        }

        /* Snap the offset of the new item(s) to the grid */
        SnapManager &sm = desktop->namedview->snap_manager;
        SnapManager::SnapperList gs = sm.getGridSnappers();
        m = sm.freeSnapAlways(Inkscape::Snapper::SNAPPOINT_NODE, m, NULL, gs).getPoint();
        sp_selection_move_relative(selection, m);
    }

    sp_document_done(document, SP_VERB_EDIT_PASTE, 
                     _("Paste"));
}

void sp_selection_paste_style()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL) return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is in the clipboard
    if (clipboard == NULL) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing on the clipboard."));
        return;
    }

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to paste style to."));
        return;
    }

    paste_defs (&defs_clipboard, sp_desktop_document(desktop));

    sp_desktop_set_style (desktop, style_clipboard);

    sp_document_done(sp_desktop_document (desktop), SP_VERB_EDIT_PASTE_STYLE,
                     _("Paste style"));
}

void sp_selection_paste_size (bool apply_x, bool apply_y)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL) return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is in the clipboard
    if (!size_clipboard) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing on the clipboard."));
        return;
    }

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to paste size to."));
        return;
    }

    NR::Maybe<NR::Rect> current = selection->bounds();
    if ( !current || current->isEmpty() ) {
        return;
    }

    double scale_x = size_clipboard->extent(NR::X) / current->extent(NR::X);
    double scale_y = size_clipboard->extent(NR::Y) / current->extent(NR::Y);

    sp_selection_scale_relative (selection, current->midpoint(),
                                 NR::scale(
                                     apply_x? scale_x : (desktop->isToolboxButtonActive ("lock")? scale_y : 1.0),
                                     apply_y? scale_y : (desktop->isToolboxButtonActive ("lock")? scale_x : 1.0)));

    sp_document_done(sp_desktop_document (desktop), SP_VERB_EDIT_PASTE_SIZE,
                     _("Paste size"));
}

void sp_selection_paste_size_separately (bool apply_x, bool apply_y)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL) return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is in the clipboard
    if ( !size_clipboard ) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Nothing on the clipboard."));
        return;
    }

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to paste size to."));
        return;
    }

    for (GSList const *l = selection->itemList(); l != NULL; l = l->next) {
        SPItem *item = SP_ITEM(l->data);

        NR::Maybe<NR::Rect> current = sp_item_bbox_desktop(item);
        if ( !current || current->isEmpty() ) {
            continue;
        }

        double scale_x = size_clipboard->extent(NR::X) / current->extent(NR::X);
        double scale_y = size_clipboard->extent(NR::Y) / current->extent(NR::Y);

        sp_item_scale_rel (item,
                                 NR::scale(
                                     apply_x? scale_x : (desktop->isToolboxButtonActive ("lock")? scale_y : 1.0),
                                     apply_y? scale_y : (desktop->isToolboxButtonActive ("lock")? scale_x : 1.0)));

    }

    sp_document_done(sp_desktop_document (desktop), SP_VERB_EDIT_PASTE_SIZE_SEPARATELY,
                     _("Paste size separately"));
}

void sp_selection_to_next_layer ()
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = sp_desktop_selection(dt);

    // check if something is selected
    if (selection->isEmpty()) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to move to the layer above."));
        return;
    }

    const GSList *items = g_slist_copy ((GSList *) selection->itemList());

    bool no_more = false; // Set to true, if no more layers above
    SPObject *next=Inkscape::next_layer(dt->currentRoot(), dt->currentLayer());
    if (next) {
        GSList *temp_clip = NULL;
        sp_selection_copy_impl (items, &temp_clip, NULL, NULL, sp_document_repr_doc(dt->doc())); // we're in the same doc, so no need to copy defs
        sp_selection_delete_impl (items);
        next=Inkscape::next_layer(dt->currentRoot(), dt->currentLayer()); // Fixes bug 1482973: crash while moving layers
        GSList *copied;
        if(next) {
            copied = sp_selection_paste_impl (sp_desktop_document (dt), next, &temp_clip, NULL);
        } else {
            copied = sp_selection_paste_impl (sp_desktop_document (dt), dt->currentLayer(), &temp_clip, NULL);
            no_more = true;
        }
        selection->setReprList((GSList const *) copied);
        g_slist_free (copied);
        if (temp_clip) g_slist_free (temp_clip);
        if (next) dt->setCurrentLayer(next);
        sp_document_done(sp_desktop_document (dt), SP_VERB_LAYER_MOVE_TO_NEXT, 
                         _("Raise to next layer"));
    } else {
        no_more = true;
    }

    if (no_more) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No more layers above."));
    }

    g_slist_free ((GSList *) items);
}

void sp_selection_to_prev_layer ()
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = sp_desktop_selection(dt);

    // check if something is selected
    if (selection->isEmpty()) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to move to the layer below."));
        return;
    }

    const GSList *items = g_slist_copy ((GSList *) selection->itemList());

    bool no_more = false; // Set to true, if no more layers below
    SPObject *next=Inkscape::previous_layer(dt->currentRoot(), dt->currentLayer());
    if (next) {
        GSList *temp_clip = NULL;
        sp_selection_copy_impl (items, &temp_clip, NULL, NULL, sp_document_repr_doc(dt->doc())); // we're in the same doc, so no need to copy defs
        sp_selection_delete_impl (items);
        next=Inkscape::previous_layer(dt->currentRoot(), dt->currentLayer()); // Fixes bug 1482973: crash while moving layers
        GSList *copied;
        if(next) {
            copied = sp_selection_paste_impl (sp_desktop_document (dt), next, &temp_clip, NULL);
        } else {
            copied = sp_selection_paste_impl (sp_desktop_document (dt), dt->currentLayer(), &temp_clip, NULL);
            no_more = true;
        }
        selection->setReprList((GSList const *) copied);
        g_slist_free (copied);
        if (temp_clip) g_slist_free (temp_clip);
        if (next) dt->setCurrentLayer(next);
        sp_document_done(sp_desktop_document (dt), SP_VERB_LAYER_MOVE_TO_PREV,
                         _("Lower to previous layer"));
    } else {
        no_more = true;
    }

    if (no_more) {
        dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No more layers below."));
    }

    g_slist_free ((GSList *) items);
}

bool
selection_contains_original (SPItem *item, Inkscape::Selection *selection)
{
    bool contains_original = false;
    
    bool is_use = SP_IS_USE(item);
    SPItem *item_use = item;
    SPItem *item_use_first = item;
    while (is_use && item_use && !contains_original)
    {
        item_use = sp_use_get_original (SP_USE(item_use));
        contains_original |= selection->includes(item_use);
        if (item_use == item_use_first)
            break;
        is_use = SP_IS_USE(item_use);
    }
    
    // If it's a tref, check whether the object containing the character
    // data is part of the selection
    if (!contains_original && SP_IS_TREF(item)) {
        contains_original = selection->includes(SP_TREF(item)->getObjectReferredTo());
    }
       
    return contains_original;
}


bool
selection_contains_both_clone_and_original (Inkscape::Selection *selection)
{
    bool clone_with_original = false;
    for (GSList const *l = selection->itemList(); l != NULL; l = l->next) {
        SPItem *item = SP_ITEM(l->data);
        clone_with_original |= selection_contains_original(item, selection);
        if (clone_with_original) 
            break;
    }
    return clone_with_original;
}


/** Apply matrix to the selection.  \a set_i2d is normally true, which means objects are in the
original transform, synced with their reprs, and need to jump to the new transform in one go. A
value of set_i2d==false is only used by seltrans when it's dragging objects live (not outlines); in
that case, items are already in the new position, but the repr is in the old, and this function
then simply updates the repr from item->transform.
 */
void sp_selection_apply_affine(Inkscape::Selection *selection, NR::Matrix const &affine, bool set_i2d)
{
    if (selection->isEmpty())
        return;

    for (GSList const *l = selection->itemList(); l != NULL; l = l->next) {
        SPItem *item = SP_ITEM(l->data);

        NR::Point old_center(0,0);
        if (set_i2d && item->isCenterSet())
            old_center = item->getCenter();

#if 0 /* Re-enable this once persistent guides have a graphical indication.
	 At the time of writing, this is the only place to re-enable. */
        sp_item_update_cns(*item, selection->desktop());
#endif

        // we're moving both a clone and its original or any ancestor in clone chain?
        bool transform_clone_with_original = selection_contains_original(item, selection);
        // ...both a text-on-path and its path?
        bool transform_textpath_with_path = (SP_IS_TEXT_TEXTPATH(item) && selection->includes( sp_textpath_get_path_item (SP_TEXTPATH(sp_object_first_child(SP_OBJECT(item)))) ));
        // ...both a flowtext and its frame?
        bool transform_flowtext_with_frame = (SP_IS_FLOWTEXT(item) && selection->includes( SP_FLOWTEXT(item)->get_frame (NULL))); // (only the first frame is checked so far)
        // ...both an offset and its source?
        bool transform_offset_with_source = (SP_IS_OFFSET(item) && SP_OFFSET (item)->sourceHref) && selection->includes( sp_offset_get_source (SP_OFFSET(item)) );
       
        // If we're moving a connector, we want to detach it
        // from shapes that aren't part of the selection, but
        // leave it attached if they are
        if (cc_item_is_connector(item)) {
            SPItem *attItem[2];
            SP_PATH(item)->connEndPair.getAttachedItems(attItem);
            
            for (int n = 0; n < 2; ++n) {
                if (!selection->includes(attItem[n])) {
                    sp_conn_end_detach(item, n);
                }
            }
        }
        
        // "clones are unmoved when original is moved" preference
        int compensation = prefs_get_int_attribute("options.clonecompensation", "value", SP_CLONE_COMPENSATION_UNMOVED);
        bool prefs_unmoved = (compensation == SP_CLONE_COMPENSATION_UNMOVED);
        bool prefs_parallel = (compensation == SP_CLONE_COMPENSATION_PARALLEL);

	// If this is a clone and it's selected along with its original, do not move it; it will feel the
	// transform of its original and respond to it itself. Without this, a clone is doubly
	// transformed, very unintuitive.
      // Same for textpath if we are also doing ANY transform to its path: do not touch textpath,
      // letters cannot be squeezed or rotated anyway, they only refill the changed path.
      // Same for linked offset if we are also moving its source: do not move it.
        if (transform_textpath_with_path || transform_offset_with_source) {
		// restore item->transform field from the repr, in case it was changed by seltrans
            sp_object_read_attr (SP_OBJECT (item), "transform");

        } else if (transform_flowtext_with_frame) {
            // apply the inverse of the region's transform to the <use> so that the flow remains
            // the same (even though the output itself gets transformed)
            for (SPObject *region = item->firstChild() ; region ; region = SP_OBJECT_NEXT(region)) {
                if (!SP_IS_FLOWREGION(region) && !SP_IS_FLOWREGIONEXCLUDE(region))
                    continue;
                for (SPObject *use = region->firstChild() ; use ; use = SP_OBJECT_NEXT(use)) {
                    if (!SP_IS_USE(use)) continue;
                    sp_item_write_transform(SP_USE(use), SP_OBJECT_REPR(use), item->transform.inverse(), NULL);
                }
            }
        } else if (transform_clone_with_original) {
            // We are transforming a clone along with its original. The below matrix juggling is
            // necessary to ensure that they transform as a whole, i.e. the clone's induced
            // transform and its move compensation are both cancelled out.

            // restore item->transform field from the repr, in case it was changed by seltrans
            sp_object_read_attr (SP_OBJECT (item), "transform");

            // calculate the matrix we need to apply to the clone to cancel its induced transform from its original
            NR::Matrix parent_transform = sp_item_i2root_affine(SP_ITEM(SP_OBJECT_PARENT (item)));
            NR::Matrix t = parent_transform * matrix_to_desktop (matrix_from_desktop (affine, item), item) * parent_transform.inverse();
            NR::Matrix t_inv =parent_transform * matrix_to_desktop (matrix_from_desktop (affine.inverse(), item), item) * parent_transform.inverse();
            NR::Matrix result = t_inv * item->transform * t;

            if ((prefs_parallel || prefs_unmoved) && affine.is_translation()) {
                // we need to cancel out the move compensation, too

                // find out the clone move, same as in sp_use_move_compensate
                NR::Matrix parent = sp_use_get_parent_transform (SP_USE(item));
                NR::Matrix clone_move = parent.inverse() * t * parent;

                if (prefs_parallel) {
                    NR::Matrix move = result * clone_move * t_inv;
                    sp_item_write_transform(item, SP_OBJECT_REPR(item), move, &move);

                } else if (prefs_unmoved) {
                    //if (SP_IS_USE(sp_use_get_original(SP_USE(item))))
                    //    clone_move = NR::identity();
                    NR::Matrix move = result * clone_move;
                    sp_item_write_transform(item, SP_OBJECT_REPR(item), move, &t);
                }

            } else {
                // just apply the result
                sp_item_write_transform(item, SP_OBJECT_REPR(item), result, &t);
            }

        } else {
            if (set_i2d) {
                sp_item_set_i2d_affine(item, sp_item_i2d_affine(item) * affine);
            }
            sp_item_write_transform(item, SP_OBJECT_REPR(item), item->transform, NULL);
        }

        // if we're moving the actual object, not just updating the repr, we can transform the
        // center by the same matrix (only necessary for non-translations)
        if (set_i2d && item->isCenterSet() && !affine.is_translation()) {
            item->setCenter(old_center * affine);
            SP_OBJECT(item)->updateRepr();
        }
    }
}

void sp_selection_remove_transform()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    GSList const *l = (GSList *) selection->reprList();
    while (l != NULL) {
        sp_repr_set_attr((Inkscape::XML::Node*)l->data, "transform", NULL);
        l = l->next;
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_OBJECT_FLATTEN, 
                     _("Remove transform"));
}

void
sp_selection_scale_absolute(Inkscape::Selection *selection,
                            double const x0, double const x1,
                            double const y0, double const y1)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Rect> const bbox(selection->bounds());
    if ( !bbox || bbox->isEmpty() ) {
        return;
    }

    NR::translate const p2o(-bbox->min());

    NR::scale const newSize(x1 - x0,
                            y1 - y0);
    NR::scale const scale( newSize / NR::scale(bbox->dimensions()) );
    NR::translate const o2n(x0, y0);
    NR::Matrix const final( p2o * scale * o2n );

    sp_selection_apply_affine(selection, final);
}


void sp_selection_scale_relative(Inkscape::Selection *selection, NR::Point const &align, NR::scale const &scale)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Rect> const bbox(selection->bounds());

    if ( !bbox || bbox->isEmpty() ) {
        return;
    }

    // FIXME: ARBITRARY LIMIT: don't try to scale above 1 Mpx, it won't display properly and will crash sooner or later anyway
    if ( bbox->extent(NR::X) * scale[NR::X] > 1e6  ||
         bbox->extent(NR::Y) * scale[NR::Y] > 1e6 )
    {
        return;
    }

    NR::translate const n2d(-align);
    NR::translate const d2n(align);
    NR::Matrix const final( n2d * scale * d2n );
    sp_selection_apply_affine(selection, final);
}

void
sp_selection_rotate_relative(Inkscape::Selection *selection, NR::Point const &center, gdouble const angle_degrees)
{
    NR::translate const d2n(center);
    NR::translate const n2d(-center);
    NR::rotate const rotate(rotate_degrees(angle_degrees));
    NR::Matrix const final( NR::Matrix(n2d) * rotate * d2n );
    sp_selection_apply_affine(selection, final);
}

void
sp_selection_skew_relative(Inkscape::Selection *selection, NR::Point const &align, double dx, double dy)
{
    NR::translate const d2n(align);
    NR::translate const n2d(-align);
    NR::Matrix const skew(1, dy,
                          dx, 1,
                          0, 0);
    NR::Matrix const final( n2d * skew * d2n );
    sp_selection_apply_affine(selection, final);
}

void sp_selection_move_relative(Inkscape::Selection *selection, NR::Point const &move)
{
    sp_selection_apply_affine(selection, NR::Matrix(NR::translate(move)));
}

void sp_selection_move_relative(Inkscape::Selection *selection, double dx, double dy)
{
    sp_selection_apply_affine(selection, NR::Matrix(NR::translate(dx, dy)));
}


/**
 * \brief sp_selection_rotate_90
 *
 * This function rotates selected objects 90 degrees clockwise.
 *
 */

void sp_selection_rotate_90_cw()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty())
        return;

    GSList const *l = selection->itemList();
    NR::rotate const rot_neg_90(NR::Point(0, -1));
    for (GSList const *l2 = l ; l2 != NULL ; l2 = l2->next) {
        SPItem *item = SP_ITEM(l2->data);
        sp_item_rotate_rel(item, rot_neg_90);
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_OBJECT_ROTATE_90_CCW, 
                     _("Rotate 90&#176; CW"));
}


/**
 * \brief sp_selection_rotate_90_ccw
 *
 * This function rotates selected objects 90 degrees counter-clockwise.
 *
 */

void sp_selection_rotate_90_ccw()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty())
        return;

    GSList const *l = selection->itemList();
    NR::rotate const rot_neg_90(NR::Point(0, 1));
    for (GSList const *l2 = l ; l2 != NULL ; l2 = l2->next) {
        SPItem *item = SP_ITEM(l2->data);
        sp_item_rotate_rel(item, rot_neg_90);
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_OBJECT_ROTATE_90_CW,
                     _("Rotate 90&#176; CCW"));
}

void
sp_selection_rotate(Inkscape::Selection *selection, gdouble const angle_degrees)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Point> center = selection->center();
    if (!center) {
        return;
    }

    sp_selection_rotate_relative(selection, *center, angle_degrees);

    sp_document_maybe_done(sp_desktop_document(selection->desktop()),
                           ( ( angle_degrees > 0 )
                             ? "selector:rotate:ccw"
                             : "selector:rotate:cw" ), 
                           SP_VERB_CONTEXT_SELECT, 
                           _("Rotate"));
}

/**
\param  angle   the angle in "angular pixels", i.e. how many visible pixels must move the outermost point of the rotated object
*/
void
sp_selection_rotate_screen(Inkscape::Selection *selection, gdouble angle)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Rect> const bbox(selection->bounds());
    NR::Maybe<NR::Point> center = selection->center();

    if ( !bbox || !center ) {
        return;
    }

    gdouble const zoom = selection->desktop()->current_zoom();
    gdouble const zmove = angle / zoom;
    gdouble const r = NR::L2(bbox->max() - *center);

    gdouble const zangle = 180 * atan2(zmove, r) / M_PI;

    sp_selection_rotate_relative(selection, *center, zangle);

    sp_document_maybe_done(sp_desktop_document(selection->desktop()),
                           ( (angle > 0)
                             ? "selector:rotate:ccw"
                             : "selector:rotate:cw" ),
                           SP_VERB_CONTEXT_SELECT, 
                           _("Rotate by pixels"));
}

void
sp_selection_scale(Inkscape::Selection *selection, gdouble grow)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Rect> const bbox(selection->bounds());
    if (!bbox) {
        return;
    }

    NR::Point const center(bbox->midpoint());

    // you can't scale "do nizhe pola" (below zero)
    double const max_len = bbox->maxExtent();
    if ( max_len + grow <= 1e-3 ) {
        return;
    }

    double const times = 1.0 + grow / max_len;
    sp_selection_scale_relative(selection, center, NR::scale(times, times));

    sp_document_maybe_done(sp_desktop_document(selection->desktop()),
                           ( (grow > 0)
                             ? "selector:scale:larger"
                             : "selector:scale:smaller" ),
                           SP_VERB_CONTEXT_SELECT,
                           _("Scale"));
}

void
sp_selection_scale_screen(Inkscape::Selection *selection, gdouble grow_pixels)
{
    sp_selection_scale(selection,
                       grow_pixels / selection->desktop()->current_zoom());
}

void
sp_selection_scale_times(Inkscape::Selection *selection, gdouble times)
{
    if (selection->isEmpty())
        return;

    NR::Maybe<NR::Rect> sel_bbox = selection->bounds();

    if (!sel_bbox) {
        return;
    }

    NR::Point const center(sel_bbox->midpoint());
    sp_selection_scale_relative(selection, center, NR::scale(times, times));
    sp_document_done(sp_desktop_document(selection->desktop()), SP_VERB_CONTEXT_SELECT, 
                     _("Scale by whole factor"));
}

void
sp_selection_move(gdouble dx, gdouble dy)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Inkscape::Selection *selection = sp_desktop_selection(desktop);
    if (selection->isEmpty()) {
        return;
    }

    sp_selection_move_relative(selection, dx, dy);

    if (dx == 0) {
        sp_document_maybe_done(sp_desktop_document(desktop), "selector:move:vertical", SP_VERB_CONTEXT_SELECT, 
                               _("Move vertically"));
    } else if (dy == 0) {
        sp_document_maybe_done(sp_desktop_document(desktop), "selector:move:horizontal", SP_VERB_CONTEXT_SELECT, 
                               _("Move horizontally"));
    } else {
        sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_SELECT, 
                         _("Move"));
    }
}

void
sp_selection_move_screen(gdouble dx, gdouble dy)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);
    if (selection->isEmpty()) {
        return;
    }

    // same as sp_selection_move but divide deltas by zoom factor
    gdouble const zoom = desktop->current_zoom();
    gdouble const zdx = dx / zoom;
    gdouble const zdy = dy / zoom;
    sp_selection_move_relative(selection, zdx, zdy);

    if (dx == 0) {
        sp_document_maybe_done(sp_desktop_document(desktop), "selector:move:vertical", SP_VERB_CONTEXT_SELECT, 
                               _("Move vertically by pixels"));
    } else if (dy == 0) {
        sp_document_maybe_done(sp_desktop_document(desktop), "selector:move:horizontal", SP_VERB_CONTEXT_SELECT, 
                               _("Move horizontally by pixels"));
    } else {
        sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_SELECT, 
                         _("Move"));
    }
}

namespace {

template <typename D>
SPItem *next_item(SPDesktop *desktop, GSList *path, SPObject *root,
                  bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive);

template <typename D>
SPItem *next_item_from_list(SPDesktop *desktop, GSList const *items, SPObject *root,
                  bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive);

struct Forward {
    typedef SPObject *Iterator;

    static Iterator children(SPObject *o) { return sp_object_first_child(o); }
    static Iterator siblings_after(SPObject *o) { return SP_OBJECT_NEXT(o); }
    static void dispose(Iterator i) {}

    static SPObject *object(Iterator i) { return i; }
    static Iterator next(Iterator i) { return SP_OBJECT_NEXT(i); }
};

struct Reverse {
    typedef GSList *Iterator;

    static Iterator children(SPObject *o) {
        return make_list(o->firstChild(), NULL);
    }
    static Iterator siblings_after(SPObject *o) {
        return make_list(SP_OBJECT_PARENT(o)->firstChild(), o);
    }
    static void dispose(Iterator i) {
        g_slist_free(i);
    }

    static SPObject *object(Iterator i) {
        return reinterpret_cast<SPObject *>(i->data);
    }
    static Iterator next(Iterator i) { return i->next; }

private:
    static GSList *make_list(SPObject *object, SPObject *limit) {
        GSList *list=NULL;
        while ( object != limit ) {
            list = g_slist_prepend(list, object);
            object = SP_OBJECT_NEXT(object);
        }
        return list;
    }
};

}

void
sp_selection_item_next(void)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    g_return_if_fail(desktop != NULL);
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    PrefsSelectionContext inlayer = (PrefsSelectionContext)prefs_get_int_attribute ("options.kbselection", "inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs_get_int_attribute ("options.kbselection", "onlyvisible", 1);
    bool onlysensitive = prefs_get_int_attribute ("options.kbselection", "onlysensitive", 1);

    SPObject *root;
    if (PREFS_SELECTION_ALL != inlayer) {
        root = selection->activeContext();
    } else {
        root = desktop->currentRoot();
    }

    SPItem *item=next_item_from_list<Forward>(desktop, selection->itemList(), root, SP_CYCLING == SP_CYCLE_VISIBLE, inlayer, onlyvisible, onlysensitive);

    if (item) {
        selection->set(item, PREFS_SELECTION_LAYER_RECURSIVE == inlayer);
        if ( SP_CYCLING == SP_CYCLE_FOCUS ) {
            scroll_to_show_item(desktop, item);
        }
    }
}

void
sp_selection_item_prev(void)
{
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    g_return_if_fail(document != NULL);
    g_return_if_fail(desktop != NULL);
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    PrefsSelectionContext inlayer = (PrefsSelectionContext)prefs_get_int_attribute ("options.kbselection", "inlayer", PREFS_SELECTION_LAYER);
    bool onlyvisible = prefs_get_int_attribute ("options.kbselection", "onlyvisible", 1);
    bool onlysensitive = prefs_get_int_attribute ("options.kbselection", "onlysensitive", 1);

    SPObject *root;
    if (PREFS_SELECTION_ALL != inlayer) {
        root = selection->activeContext();
    } else {
        root = desktop->currentRoot();
    }

    SPItem *item=next_item_from_list<Reverse>(desktop, selection->itemList(), root, SP_CYCLING == SP_CYCLE_VISIBLE, inlayer, onlyvisible, onlysensitive);

    if (item) {
        selection->set(item, PREFS_SELECTION_LAYER_RECURSIVE == inlayer);
        if ( SP_CYCLING == SP_CYCLE_FOCUS ) {
            scroll_to_show_item(desktop, item);
        }
    }
}

namespace {

template <typename D>
SPItem *next_item_from_list(SPDesktop *desktop, GSList const *items,
                            SPObject *root, bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive)
{
    SPObject *current=root;
    while (items) {
        SPItem *item=SP_ITEM(items->data);
        if ( root->isAncestorOf(item) &&
             ( !only_in_viewport || desktop->isWithinViewport(item) ) )
        {
            current = item;
            break;
        }
        items = items->next;
    }

    GSList *path=NULL;
    while ( current != root ) {
        path = g_slist_prepend(path, current);
        current = SP_OBJECT_PARENT(current);
    }

    SPItem *next;
    // first, try from the current object
    next = next_item<D>(desktop, path, root, only_in_viewport, inlayer, onlyvisible, onlysensitive);
    g_slist_free(path);

    if (!next) { // if we ran out of objects, start over at the root
        next = next_item<D>(desktop, NULL, root, only_in_viewport, inlayer, onlyvisible, onlysensitive);
    }

    return next;
}

template <typename D>
SPItem *next_item(SPDesktop *desktop, GSList *path, SPObject *root,
                  bool only_in_viewport, PrefsSelectionContext inlayer, bool onlyvisible, bool onlysensitive)
{
    typename D::Iterator children;
    typename D::Iterator iter;

    SPItem *found=NULL;

    if (path) {
        SPObject *object=reinterpret_cast<SPObject *>(path->data);
        g_assert(SP_OBJECT_PARENT(object) == root);
        if (desktop->isLayer(object)) {
            found = next_item<D>(desktop, path->next, object, only_in_viewport, inlayer, onlyvisible, onlysensitive);
        }
        iter = children = D::siblings_after(object);
    } else {
        iter = children = D::children(root);
    }

    while ( iter && !found ) {
        SPObject *object=D::object(iter);
        if (desktop->isLayer(object)) {
            if (PREFS_SELECTION_LAYER != inlayer) { // recurse into sublayers
                found = next_item<D>(desktop, NULL, object, only_in_viewport, inlayer, onlyvisible, onlysensitive);
            }
        } else if ( SP_IS_ITEM(object) &&
                    ( !only_in_viewport || desktop->isWithinViewport(SP_ITEM(object)) ) &&
                    ( !onlyvisible || !desktop->itemIsHidden(SP_ITEM(object))) &&
                    ( !onlysensitive || !SP_ITEM(object)->isLocked()) &&
                    !desktop->isLayer(SP_ITEM(object)) )
        {
            found = SP_ITEM(object);
        }
        iter = D::next(iter);
    }

    D::dispose(children);

    return found;
}

}

/**
 * If \a item is not entirely visible then adjust visible area to centre on the centre on of
 * \a item.
 */
void scroll_to_show_item(SPDesktop *desktop, SPItem *item)
{
    NR::Rect dbox = desktop->get_display_area();
    NR::Maybe<NR::Rect> sbox = sp_item_bbox_desktop(item);

    if ( sbox && dbox.contains(*sbox) == false ) {
        NR::Point const s_dt = sbox->midpoint();
        NR::Point const s_w = desktop->d2w(s_dt);
        NR::Point const d_dt = dbox.midpoint();
        NR::Point const d_w = desktop->d2w(d_dt);
        NR::Point const moved_w( d_w - s_w );
        gint const dx = (gint) moved_w[X];
        gint const dy = (gint) moved_w[Y];
        desktop->scroll_world(dx, dy);
    }
}


void
sp_selection_clone()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(desktop->doc());

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select an <b>object</b> to clone."));
        return;
    }

    GSList *reprs = g_slist_copy((GSList *) selection->reprList());
  
    selection->clear();
  
    // sorting items from different parents sorts each parent's subset without possibly mixing them, just what we need
    reprs = g_slist_sort(reprs, (GCompareFunc) sp_repr_compare_position);

    GSList *newsel = NULL;
 
    while (reprs) {
        Inkscape::XML::Node *sel_repr = (Inkscape::XML::Node *) reprs->data;
        Inkscape::XML::Node *parent = sp_repr_parent(sel_repr);

        Inkscape::XML::Node *clone = xml_doc->createElement("svg:use");
        sp_repr_set_attr(clone, "x", "0");
        sp_repr_set_attr(clone, "y", "0");
        sp_repr_set_attr(clone, "xlink:href", g_strdup_printf("#%s", sel_repr->attribute("id")));

        sp_repr_set_attr(clone, "inkscape:transform-center-x", sel_repr->attribute("inkscape:transform-center-x"));
        sp_repr_set_attr(clone, "inkscape:transform-center-y", sel_repr->attribute("inkscape:transform-center-y"));
        
        // add the new clone to the top of the original's parent
        parent->appendChild(clone);

        newsel = g_slist_prepend(newsel, clone);
        reprs = g_slist_remove(reprs, sel_repr);
        Inkscape::GC::release(clone);
    }
    
    // TRANSLATORS: only translate "string" in "context|string".
    // For more details, see http://developer.gnome.org/doc/API/2.0/glib/glib-I18N.html#Q-:CAPS
    sp_document_done(sp_desktop_document(desktop), SP_VERB_EDIT_CLONE, 
                     Q_("action|Clone"));

    selection->setReprList(newsel);
 
    g_slist_free(newsel);
}

void
sp_selection_unlink()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select a <b>clone</b> to unlink."));
        return;
    }

    // Get a copy of current selection.
    GSList *new_select = NULL;
    bool unlinked = false;
    for (GSList *items = g_slist_copy((GSList *) selection->itemList());
         items != NULL;
         items = items->next)
    {
        SPItem *item = (SPItem *) items->data;

        if (SP_IS_TEXT(item)) {
            SPObject *tspan = sp_tref_convert_to_tspan(SP_OBJECT(item));
            
            if (tspan) {            
                SP_OBJECT(item)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
            
            // Set unlink to true, and fall into the next if which
            // will include this text item in the new selection
            unlinked = true;
        }

        if (!(SP_IS_USE(item) || SP_IS_TREF(item))) {
            // keep the non-use item in the new selection
            new_select = g_slist_prepend(new_select, item);
            continue;
        }

        SPItem *unlink;
        if (SP_IS_USE(item)) { 
            unlink = sp_use_unlink(SP_USE(item));
        } else /*if (SP_IS_TREF(use))*/ {
            unlink = SP_ITEM(sp_tref_convert_to_tspan(SP_OBJECT(item)));
        }
        
        unlinked = true;
        // Add ungrouped items to the new selection.
        new_select = g_slist_prepend(new_select, unlink);
    }

    if (new_select) { // set new selection
        selection->clear();
        selection->setList(new_select);
        g_slist_free(new_select);
    }
    if (!unlinked) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No clones to unlink</b> in the selection."));
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_EDIT_UNLINK_CLONE,
                     _("Unlink clone"));
}

void
sp_select_clone_original()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    SPItem *item = selection->singleItem();

    const gchar *error = _("Select a <b>clone</b> to go to its original. Select a <b>linked offset</b> to go to its source. Select a <b>text on path</b> to go to the path. Select a <b>flowed text</b> to go to its frame.");

    // Check if other than two objects are selected
    if (g_slist_length((GSList *) selection->itemList()) != 1 || !item) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, error);
        return;
    }

    SPItem *original = NULL;
    if (SP_IS_USE(item)) {
        original = sp_use_get_original (SP_USE(item));
    } else if (SP_IS_OFFSET(item) && SP_OFFSET (item)->sourceHref) {
        original = sp_offset_get_source (SP_OFFSET(item));
    } else if (SP_IS_TEXT_TEXTPATH(item)) {
        original = sp_textpath_get_path_item (SP_TEXTPATH(sp_object_first_child(SP_OBJECT(item))));
    } else if (SP_IS_FLOWTEXT(item)) {
        original = SP_FLOWTEXT(item)->get_frame (NULL); // first frame only
    } else { // it's an object that we don't know what to do with
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, error);
        return;
    }

    if (!original) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>Cannot find</b> the object to select (orphaned clone, offset, textpath, flowed text?)"));
        return;
    }

    for (SPObject *o = original; o && !SP_IS_ROOT(o); o = SP_OBJECT_PARENT (o)) {
        if (SP_IS_DEFS (o)) {
            desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("The object you're trying to select is <b>not visible</b> (it is in &lt;defs&gt;)"));
            return;
        }
    }

    if (original) {
        selection->clear();
        selection->set(original);
        if (SP_CYCLING == SP_CYCLE_FOCUS) {
            scroll_to_show_item(desktop, original);
        }
    }
}

void
sp_selection_tile(bool apply)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *doc = sp_desktop_document(desktop);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to convert to pattern."));
        return;
    }

    sp_document_ensure_up_to_date(doc);
    NR::Maybe<NR::Rect> r = selection->bounds();
    if ( !r || r->isEmpty() ) {
        return;
    }

    // calculate the transform to be applied to objects to move them to 0,0
    NR::Point move_p = NR::Point(0, sp_document_height(doc)) - (r->min() + NR::Point (0, r->extent(NR::Y)));
    move_p[NR::Y] = -move_p[NR::Y];
    NR::Matrix move = NR::Matrix (NR::translate (move_p));

    GSList *items = g_slist_copy((GSList *) selection->itemList());

    items = g_slist_sort (items, (GCompareFunc) sp_object_compare_position);

    // bottommost object, after sorting
    SPObject *parent = SP_OBJECT_PARENT (items->data);

    NR::Matrix parent_transform = sp_item_i2root_affine(SP_ITEM(parent));

    // remember the position of the first item
    gint pos = SP_OBJECT_REPR (items->data)->position();

    // create a list of duplicates
    GSList *repr_copies = NULL;
    for (GSList *i = items; i != NULL; i = i->next) {
        Inkscape::XML::Node *dup = (SP_OBJECT_REPR (i->data))->duplicate(xml_doc);
        repr_copies = g_slist_prepend (repr_copies, dup);
    }

    NR::Rect bounds(desktop->dt2doc(r->min()), desktop->dt2doc(r->max()));

    if (apply) {
        // delete objects so that their clones don't get alerted; this object will be restored shortly
        for (GSList *i = items; i != NULL; i = i->next) {
            SPObject *item = SP_OBJECT (i->data);
            item->deleteObject (false);
        }
    }

    // Hack: Temporarily set clone compensation to unmoved, so that we can move clone-originals
    // without disturbing clones.
    // See ActorAlign::on_button_click() in src/ui/dialog/align-and-distribute.cpp
    int saved_compensation = prefs_get_int_attribute("options.clonecompensation", "value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs_set_int_attribute("options.clonecompensation", "value", SP_CLONE_COMPENSATION_UNMOVED);

    const gchar *pat_id = pattern_tile (repr_copies, bounds, doc,
                                        NR::Matrix(NR::translate(desktop->dt2doc(NR::Point(r->min()[NR::X], r->max()[NR::Y])))) * parent_transform.inverse(), parent_transform * move);

    // restore compensation setting
    prefs_set_int_attribute("options.clonecompensation", "value", saved_compensation);

    if (apply) {
        Inkscape::XML::Node *rect = xml_doc->createElement("svg:rect");
        rect->setAttribute("style", g_strdup_printf("stroke:none;fill:url(#%s)", pat_id));

        NR::Point min = bounds.min() * parent_transform.inverse();
        NR::Point max = bounds.max() * parent_transform.inverse();

        sp_repr_set_svg_double(rect, "width", max[NR::X] - min[NR::X]);
        sp_repr_set_svg_double(rect, "height", max[NR::Y] - min[NR::Y]);
        sp_repr_set_svg_double(rect, "x", min[NR::X]);
        sp_repr_set_svg_double(rect, "y", min[NR::Y]);

        // restore parent and position
        SP_OBJECT_REPR (parent)->appendChild(rect);
        rect->setPosition(pos > 0 ? pos : 0);
        SPItem *rectangle = (SPItem *) sp_desktop_document (desktop)->getObjectByRepr(rect);

        Inkscape::GC::release(rect);

        selection->clear();
        selection->set(rectangle);
    }

    g_slist_free (items);

    sp_document_done (doc, SP_VERB_EDIT_TILE, 
                      _("Objects to pattern"));
}

void
sp_selection_untile()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *doc = sp_desktop_document(desktop);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select an <b>object with pattern fill</b> to extract objects from."));
        return;
    }

    GSList *new_select = NULL;

    bool did = false;

    for (GSList *items = g_slist_copy((GSList *) selection->itemList());
         items != NULL;
         items = items->next) {

        SPItem *item = (SPItem *) items->data;

        SPStyle *style = SP_OBJECT_STYLE (item);

        if (!style || style->fill.type != SP_PAINT_TYPE_PAINTSERVER)
            continue;

        SPObject *server = SP_OBJECT_STYLE_FILL_SERVER(item);

        if (!SP_IS_PATTERN(server))
            continue;

        did = true;

        SPPattern *pattern = pattern_getroot (SP_PATTERN (server));

        NR::Matrix pat_transform = pattern_patternTransform (SP_PATTERN (server));
        pat_transform *= item->transform;

        for (SPObject *child = sp_object_first_child(SP_OBJECT(pattern)) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
            Inkscape::XML::Node *copy = SP_OBJECT_REPR(child)->duplicate(xml_doc);
            SPItem *i = SP_ITEM (desktop->currentLayer()->appendChildRepr(copy));

           // FIXME: relink clones to the new canvas objects
           // use SPObject::setid when mental finishes it to steal ids of

            // this is needed to make sure the new item has curve (simply requestDisplayUpdate does not work)
            sp_document_ensure_up_to_date (doc);

            NR::Matrix transform( i->transform * pat_transform );
            sp_item_write_transform(i, SP_OBJECT_REPR(i), transform);

            new_select = g_slist_prepend(new_select, i);
        }

        SPCSSAttr *css = sp_repr_css_attr_new ();
        sp_repr_css_set_property (css, "fill", "none");
        sp_repr_css_change (SP_OBJECT_REPR (item), css, "style");
    }

    if (!did) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No pattern fills</b> in the selection."));
    } else {
        sp_document_done(sp_desktop_document(desktop), SP_VERB_EDIT_UNTILE, 
                         _("Pattern to objects"));
        selection->setList(new_select);
    }
}

void
sp_selection_get_export_hints (Inkscape::Selection *selection, const char **filename, float *xdpi, float *ydpi) 
{
    if (selection->isEmpty()) {
        return;
    }

    const GSList * reprlst = selection->reprList();
    bool filename_search = TRUE;
    bool xdpi_search = TRUE;
    bool ydpi_search = TRUE;

    for(; reprlst != NULL &&
            filename_search &&
            xdpi_search &&
            ydpi_search;
        reprlst = reprlst->next) {
        const gchar * dpi_string;
        Inkscape::XML::Node * repr = (Inkscape::XML::Node *)reprlst->data;

        if (filename_search) {
            *filename = repr->attribute("inkscape:export-filename");
            if (*filename != NULL)
                filename_search = FALSE;
        }

        if (xdpi_search) {
            dpi_string = NULL;
            dpi_string = repr->attribute("inkscape:export-xdpi");
            if (dpi_string != NULL) {
                *xdpi = atof(dpi_string);
                xdpi_search = FALSE;
            }
        }

        if (ydpi_search) {
            dpi_string = NULL;
            dpi_string = repr->attribute("inkscape:export-ydpi");
            if (dpi_string != NULL) {
                *ydpi = atof(dpi_string);
                ydpi_search = FALSE;
            }
        }
    }
}

void
sp_document_get_export_hints (SPDocument * doc, const char **filename, float *xdpi, float *ydpi) 
{
    Inkscape::XML::Node * repr = sp_document_repr_root(doc);
    const gchar * dpi_string;

    *filename = repr->attribute("inkscape:export-filename");

    dpi_string = NULL;
    dpi_string = repr->attribute("inkscape:export-xdpi");
    if (dpi_string != NULL) {
        *xdpi = atof(dpi_string);
    }

    dpi_string = NULL;
    dpi_string = repr->attribute("inkscape:export-ydpi");
    if (dpi_string != NULL) {
        *ydpi = atof(dpi_string);
    }
}

void
sp_selection_create_bitmap_copy ()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *document = sp_desktop_document(desktop);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(document);

    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to make a bitmap copy."));
        return;
    }

    // Get the bounding box of the selection
    NRRect bbox;
    sp_document_ensure_up_to_date (document);
    selection->bounds(&bbox);
    if (NR_RECT_DFLS_TEST_EMPTY(&bbox)) {
        return; // exceptional situation, so not bother with a translatable error message, just quit quietly
    }

    // List of the items to show; all others will be hidden
    GSList *items = g_slist_copy ((GSList *) selection->itemList());

    // Sort items so that the topmost comes last
    items = g_slist_sort(items, (GCompareFunc) sp_item_repr_compare_position);

    // Generate a random value from the current time (you may create bitmap from the same object(s)
    // multiple times, and this is done so that they don't clash)
    GTimeVal cu;
    g_get_current_time (&cu);
    guint current = (int) (cu.tv_sec * 1000000 + cu.tv_usec) % 1024;

    // Create the filename
    gchar *filename = g_strdup_printf ("%s-%s-%u.png", document->name, SP_OBJECT_REPR(items->data)->attribute("id"), current);
    // Imagemagick is known not to handle spaces in filenames, so we replace anything but letters,
    // digits, and a few other chars, with "_"
    filename = g_strcanon (filename, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.=+~$#@^&!?", '_');
    // Build the complete path by adding document->base if set
    gchar *filepath = g_build_filename (document->base?document->base:"", filename, NULL);

    //g_print ("%s\n", filepath);

    // Remember parent and z-order of the topmost one
    gint pos = SP_OBJECT_REPR(g_slist_last(items)->data)->position();
    SPObject *parent_object = SP_OBJECT_PARENT(g_slist_last(items)->data);
    Inkscape::XML::Node *parent = SP_OBJECT_REPR(parent_object);

    // Calculate resolution
    double res;
    int const prefs_res = prefs_get_int_attribute ("options.createbitmap", "resolution", 0);
    int const prefs_min = prefs_get_int_attribute ("options.createbitmap", "minsize", 0);
    if (0 < prefs_res) {
        // If it's given explicitly in prefs, take it
        res = prefs_res;
    } else if (0 < prefs_min) {
        // If minsize is given, look up minimum bitmap size (default 250 pixels) and calculate resolution from it
        res = PX_PER_IN * prefs_min / MIN ((bbox.x1 - bbox.x0), (bbox.y1 - bbox.y0));
    } else {
        float hint_xdpi = 0, hint_ydpi = 0;
        const char *hint_filename;
        // take resolution hint from the selected objects
        sp_selection_get_export_hints (selection, &hint_filename, &hint_xdpi, &hint_ydpi);
        if (hint_xdpi != 0) {
            res = hint_xdpi;
        } else {
            // take resolution hint from the document
            sp_document_get_export_hints (document, &hint_filename, &hint_xdpi, &hint_ydpi);
            if (hint_xdpi != 0) {
                res = hint_xdpi;
            } else {
                // if all else fails, take the default 90 dpi
                res = PX_PER_IN;
            }
        }
    }

    // The width and height of the bitmap in pixels
    unsigned width = (unsigned) floor ((bbox.x1 - bbox.x0) * res / PX_PER_IN);
    unsigned height =(unsigned) floor ((bbox.y1 - bbox.y0) * res / PX_PER_IN);

    // Find out if we have to run a filter
    const gchar *run = NULL;
    const gchar *filter = prefs_get_string_attribute ("options.createbitmap", "filter");
    if (filter) {
        // filter command is given;
        // see if we have a parameter to pass to it
        const gchar *param1 = prefs_get_string_attribute ("options.createbitmap", "filter_param1");
        if (param1) {
            if (param1[strlen(param1) - 1] == '%') {
                // if the param string ends with %, interpret it as a percentage of the image's max dimension
                gchar p1[256];
                g_ascii_dtostr (p1, 256, ceil (g_ascii_strtod (param1, NULL) * MAX(width, height) / 100));
                // the first param is always the image filename, the second is param1
                run = g_strdup_printf ("%s \"%s\" %s", filter, filepath, p1);
            } else {
                // otherwise pass the param1 unchanged
                run = g_strdup_printf ("%s \"%s\" %s", filter, filepath, param1);
            }
        } else {
            // run without extra parameter
            run = g_strdup_printf ("%s \"%s\"", filter, filepath);
        }
    }

    // Calculate the matrix that will be applied to the image so that it exactly overlaps the source objects
    NR::Matrix eek = sp_item_i2d_affine (SP_ITEM(parent_object));
    NR::Matrix t;

    double shift_x = bbox.x0;
    double shift_y = bbox.y1; 
    if (res == PX_PER_IN) { // for default 90 dpi, snap it to pixel grid
        shift_x = round (shift_x);
        shift_y = -round (-shift_y); // this gets correct rounding despite coordinate inversion, remove the negations when the inversion is gone
    }
    t = NR::scale(1, -1) * NR::translate (shift_x, shift_y) * eek.inverse();

    // Do the export
    sp_export_png_file(document, filepath,
                   bbox.x0, bbox.y0, bbox.x1, bbox.y1,
                   width, height, res, res,
                   (guint32) 0xffffff00,
                   NULL, NULL,
                   true,  /*bool force_overwrite,*/
                   items);

    g_slist_free (items);

    // Run filter, if any
    if (run) {
        g_print ("Running external filter: %s\n", run);
        system (run);
    }

    // Import the image back
    GdkPixbuf *pb = gdk_pixbuf_new_from_file (filepath, NULL);
    if (pb) {
        // Create the repr for the image
        Inkscape::XML::Node * repr = xml_doc->createElement("svg:image");
        repr->setAttribute("xlink:href", filename);
        repr->setAttribute("sodipodi:absref", filepath);
        if (res == PX_PER_IN) { // for default 90 dpi, snap it to pixel grid
            sp_repr_set_svg_double(repr, "width", width);
            sp_repr_set_svg_double(repr, "height", height);
        } else {
            sp_repr_set_svg_double(repr, "width", (bbox.x1 - bbox.x0));
            sp_repr_set_svg_double(repr, "height", (bbox.y1 - bbox.y0));
        }

        // Write transform
        gchar *c=sp_svg_transform_write(t);
        repr->setAttribute("transform", c);
        g_free(c);

        // add the new repr to the parent
        parent->appendChild(repr);

        // move to the saved position
        repr->setPosition(pos > 0 ? pos + 1 : 1);

        // Set selection to the new image
        selection->clear();
        selection->add(repr);

        // Clean up
        Inkscape::GC::release(repr);
        gdk_pixbuf_unref (pb);

        // Complete undoable transaction
        sp_document_done (document, SP_VERB_SELECTION_CREATE_BITMAP,
                          _("Create bitmap"));
    }

    g_free (filename);
    g_free (filepath);
}

/**
 * \brief sp_selection_set_mask
 *
 * This function creates a mask or clipPath from selection
 * Two different modes:
 *  if applyToLayer, all selection is moved to DEFS as mask/clippath
 *       and is applied to current layer
 *  otherwise, topmost object is used as mask for other objects
 * If \a apply_clip_path parameter is true, clipPath is created, otherwise mask
 * 
 */
void
sp_selection_set_mask(bool apply_clip_path, bool apply_to_layer)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;

    SPDocument *doc = sp_desktop_document(desktop);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
    
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    bool is_empty = selection->isEmpty();
    if ( apply_to_layer && is_empty) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to create clippath or mask from."));
        return;
    } else if (!apply_to_layer && ( is_empty || NULL == selection->itemList()->next )) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select mask object and <b>object(s)</b> to apply clippath or mask to."));
        return;
    }

    // FIXME: temporary patch to prevent crash! 
    // Remove this when bboxes are fixed to not blow up on an item clipped/masked with its own clone
    bool clone_with_original = selection_contains_both_clone_and_original (selection);
    if (clone_with_original) {
        return; // in this version, you cannot clip/mask an object with its own clone
    }
    // /END FIXME
    
    sp_document_ensure_up_to_date(doc);

    GSList *items = g_slist_copy((GSList *) selection->itemList());
    
    items = g_slist_sort (items, (GCompareFunc) sp_object_compare_position);

    // create a list of duplicates
    GSList *mask_items = NULL;
    GSList *apply_to_items = NULL;
    GSList *items_to_delete = NULL;
    bool topmost = prefs_get_int_attribute ("options.maskobject", "topmost", 1);
    bool remove_original = prefs_get_int_attribute ("options.maskobject", "remove", 1);
    
    if (apply_to_layer) {
        // all selected items are used for mask, which is applied to a layer
        apply_to_items = g_slist_prepend (apply_to_items, desktop->currentLayer());

        for (GSList *i = items; i != NULL; i = i->next) {
            Inkscape::XML::Node *dup = (SP_OBJECT_REPR (i->data))->duplicate(xml_doc);
            mask_items = g_slist_prepend (mask_items, dup);

            if (remove_original) {
                SPObject *item = SP_OBJECT (i->data);
                items_to_delete = g_slist_prepend (items_to_delete, item);
            }
        }
    } else if (!topmost) {
        // topmost item is used as a mask, which is applied to other items in a selection
        GSList *i = items;
        Inkscape::XML::Node *dup = (SP_OBJECT_REPR (i->data))->duplicate(xml_doc);
        mask_items = g_slist_prepend (mask_items, dup);

        if (remove_original) {
            SPObject *item = SP_OBJECT (i->data);
            items_to_delete = g_slist_prepend (items_to_delete, item);
        }
        
        for (i = i->next; i != NULL; i = i->next) {
            apply_to_items = g_slist_prepend (apply_to_items, i->data);
        }
    } else {
        GSList *i = NULL;
        for (i = items; NULL != i->next; i = i->next) {
            apply_to_items = g_slist_prepend (apply_to_items, i->data);
        }

        Inkscape::XML::Node *dup = (SP_OBJECT_REPR (i->data))->duplicate(xml_doc);
        mask_items = g_slist_prepend (mask_items, dup);

        if (remove_original) {
            SPObject *item = SP_OBJECT (i->data);
            items_to_delete = g_slist_prepend (items_to_delete, item);
        }
    }
    
    g_slist_free (items);
    items = NULL;
            
    gchar const* attributeName = apply_clip_path ? "clip-path" : "mask";
    for (GSList *i = apply_to_items; NULL != i; i = i->next) {
        SPItem *item = reinterpret_cast<SPItem *>(i->data);
        // inverted object transform should be applied to a mask object,
        // as mask is calculated in user space (after applying transform)
        NR::Matrix maskTransform (item->transform.inverse());

        GSList *mask_items_dup = NULL;
        for (GSList *mask_item = mask_items; NULL != mask_item; mask_item = mask_item->next) {
            Inkscape::XML::Node *dup = reinterpret_cast<Inkscape::XML::Node *>(mask_item->data)->duplicate(xml_doc);
            mask_items_dup = g_slist_prepend (mask_items_dup, dup);
        }

        const gchar *mask_id = NULL;
        if (apply_clip_path) {
            mask_id = sp_clippath_create(mask_items_dup, doc, &maskTransform);
        } else {
            mask_id = sp_mask_create(mask_items_dup, doc, &maskTransform);
        }

        g_slist_free (mask_items_dup);
        mask_items_dup = NULL;

        SP_OBJECT_REPR(i->data)->setAttribute(attributeName, g_strdup_printf("url(#%s)", mask_id));
    }

    g_slist_free (mask_items);
    g_slist_free (apply_to_items);

    for (GSList *i = items_to_delete; NULL != i; i = i->next) {
        SPObject *item = SP_OBJECT (i->data);
        item->deleteObject (false);
    }
    g_slist_free (items_to_delete);

    if (apply_clip_path) 
        sp_document_done (doc, SP_VERB_OBJECT_SET_CLIPPATH, _("Set clipping path"));
    else 
        sp_document_done (doc, SP_VERB_OBJECT_SET_MASK, _("Set mask"));
}

void sp_selection_unset_mask(bool apply_clip_path) {
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == NULL)
        return;
    
    SPDocument *doc = sp_desktop_document(desktop);    
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
    Inkscape::Selection *selection = sp_desktop_selection(desktop);

    // check if something is selected
    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>object(s)</b> to remove clippath or mask from."));
        return;
    }
    
    bool remove_original = prefs_get_int_attribute ("options.maskobject", "remove", 1);
    sp_document_ensure_up_to_date(doc);

    gchar const* attributeName = apply_clip_path ? "clip-path" : "mask";
    std::map<SPObject*,SPItem*> referenced_objects;
    for (GSList const*i = selection->itemList(); NULL != i; i = i->next) {
        if (remove_original) {
            // remember referenced mask/clippath, so orphaned masks can be moved back to document
            SPItem *item = reinterpret_cast<SPItem *>(i->data);
            Inkscape::URIReference *uri_ref = NULL;
        
            if (apply_clip_path) {
                uri_ref = item->clip_ref;
            } else {
                uri_ref = item->mask_ref;
            }

            // collect distinct mask object (and associate with item to apply transform)
            if (NULL != uri_ref && NULL != uri_ref->getObject()) {
                referenced_objects[uri_ref->getObject()] = item;
            }
        }

        SP_OBJECT_REPR(i->data)->setAttribute(attributeName, "none");
    }

    // restore mask objects into a document
    for ( std::map<SPObject*,SPItem*>::iterator it = referenced_objects.begin() ; it != referenced_objects.end() ; ++it) {
        SPObject *obj = (*it).first;
        GSList *items_to_move = NULL;
        for (SPObject *child = sp_object_first_child(obj) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
            Inkscape::XML::Node *copy = SP_OBJECT_REPR(child)->duplicate(xml_doc);
            items_to_move = g_slist_prepend (items_to_move, copy);
        }

        if (!obj->isReferenced()) {
            // delete from defs if no other object references this mask
            obj->deleteObject(false);
        }

        // remember parent and position of the item to which the clippath/mask was applied
        Inkscape::XML::Node *parent = SP_OBJECT_REPR((*it).second)->parent();
        gint pos = SP_OBJECT_REPR((*it).second)->position();

        for (GSList *i = items_to_move; NULL != i; i = i->next) {
            Inkscape::XML::Node *repr = (Inkscape::XML::Node *)i->data;

            // insert into parent, restore pos
            parent->appendChild(repr);
            repr->setPosition((pos + 1) > 0 ? (pos + 1) : 0);

            SPItem *mask_item = (SPItem *) sp_desktop_document (desktop)->getObjectByRepr(repr);
            selection->add(repr);

            // transform mask, so it is moved the same spot where mask was applied
            NR::Matrix transform (mask_item->transform);
            transform *= (*it).second->transform;
            sp_item_write_transform(mask_item, SP_OBJECT_REPR(mask_item), transform);
        }

        g_slist_free (items_to_move);
    }

    if (apply_clip_path) 
        sp_document_done (doc, SP_VERB_OBJECT_UNSET_CLIPPATH, _("Release clipping path"));
    else 
        sp_document_done (doc, SP_VERB_OBJECT_UNSET_MASK, _("Release mask"));
}

void fit_canvas_to_selection(SPDesktop *desktop) {
    g_return_if_fail(desktop != NULL);
    SPDocument *doc = sp_desktop_document(desktop);

    g_return_if_fail(doc != NULL);
    g_return_if_fail(desktop->selection != NULL);
    g_return_if_fail(!desktop->selection->isEmpty());

    NR::Maybe<NR::Rect> const bbox(desktop->selection->bounds());
    if (bbox && !bbox->isEmpty()) {
        doc->fitToRect(*bbox);
    }
};

void fit_canvas_to_drawing(SPDocument *doc) {
    g_return_if_fail(doc != NULL);

    sp_document_ensure_up_to_date(doc);
    SPItem const *const root = SP_ITEM(doc->root);
    NR::Maybe<NR::Rect> const bbox(root->getBounds(sp_item_i2r_affine(root)));
    if (bbox && !bbox->isEmpty()) {
        doc->fitToRect(*bbox);
    }
};

void fit_canvas_to_selection_or_drawing(SPDesktop *desktop) {
    g_return_if_fail(desktop != NULL);
    SPDocument *doc = sp_desktop_document(desktop);

    g_return_if_fail(doc != NULL);
    g_return_if_fail(desktop->selection != NULL);

    if (desktop->selection->isEmpty()) {
        fit_canvas_to_drawing(doc);
    } else {
        fit_canvas_to_selection(desktop);
    }

    sp_document_done(doc, SP_VERB_FIT_CANVAS_TO_DRAWING, 
                     _("Fit page to selection"));
};

static void itemtree_map(void (*f)(SPItem *, SPDesktop *), SPObject *root, SPDesktop *desktop) {
    // don't operate on layers
    if (SP_IS_ITEM(root) && !desktop->isLayer(SP_ITEM(root))) {
        f(SP_ITEM(root), desktop);
    }
    for ( SPObject::SiblingIterator iter = root->firstChild() ; iter ; ++iter ) {
        //don't recurse into locked layers
        if (!(SP_IS_ITEM(&*iter) && desktop->isLayer(SP_ITEM(&*iter)) && SP_ITEM(&*iter)->isLocked())) {
            itemtree_map(f, iter, desktop);
        }
    }
}

static void unlock(SPItem *item, SPDesktop *desktop) {
    if (item->isLocked()) {
        item->setLocked(FALSE);
    }
}

static void unhide(SPItem *item, SPDesktop *desktop) {
    if (desktop->itemIsHidden(item)) {
        item->setExplicitlyHidden(FALSE);
    }
}

static void process_all(void (*f)(SPItem *, SPDesktop *), SPDesktop *dt, bool layer_only) {
    if (!dt) return;
        
    SPObject *root;
    if (layer_only) {
        root = dt->currentLayer();
    } else {
        root = dt->currentRoot();
    }
    
    itemtree_map(f, root, dt);
}

void unlock_all(SPDesktop *dt) {
    process_all(&unlock, dt, true);
}

void unlock_all_in_all_layers(SPDesktop *dt) {
    process_all(&unlock, dt, false);
}

void unhide_all(SPDesktop *dt) {
    process_all(&unhide, dt, true);
}

void unhide_all_in_all_layers(SPDesktop *dt) {
    process_all(&unhide, dt, false);
}


GSList * sp_selection_get_clipboard() {
    return clipboard;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
