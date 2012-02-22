/*
 * Port of GnomeCanvas for Inkscape needs
 *
 * Authors:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@gimp.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   fred
 *   bbyak
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2002-2006 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "helper/sp-marshal.h"
#include <2geom/rect.h>
#include <2geom/affine.h>
#include "display/sp-canvas.h"
#include "display/sp-canvas-group.h"
#include "preferences.h"
#include "inkscape.h"
#include "sodipodi-ctrlrect.h"
#include "cms-system.h"
#include "display/rendermode.h"
#include "display/cairo-utils.h"
#include "debug/gdk-event-latency-tracker.h"
#include "desktop.h"
#include "sp-namedview.h"

using Inkscape::Debug::GdkEventLatencyTracker;

// gtk_check_version returns non-NULL on failure
static bool const HAS_BROKEN_MOTION_HINTS =
  true || gtk_check_version(2, 12, 0) != NULL;

// Define this to visualize the regions to be redrawn
//#define DEBUG_REDRAW 1;

// Tiles are a way to minimize the number of redraws, eliminating too small redraws.
// The canvas stores a 2D array of ints, each representing a TILE_SIZExTILE_SIZE pixels tile.
// If any part of it is dirtied, the entire tile is dirtied (its int is nonzero) and repainted.
#define TILE_SIZE 16

/**
 * The SPCanvasGroup vtable.
 */
struct SPCanvasGroupClass {
    SPCanvasItemClass parent_class;
};

/**
 * A group of Items.
 */
struct SPCanvasGroup {
    static GType getType();

    /**
     * Adds an item to a canvas group.
     */
    void add(SPCanvasItem *item);

    /**
     * Removes an item from a canvas group.
     */
    void remove(SPCanvasItem *item);

    /**
     * Class initialization function for SPCanvasGroupClass.
     */
    static void classInit(SPCanvasGroupClass *klass);

    /**
     * Callback. Empty.
     */
    static void init(SPCanvasGroup *group);

    /**
     * Callback that destroys all items in group and calls group's virtual
     * destroy() function.
     */
    static void destroy(GtkObject *object);

    /**
     * Update handler for canvas groups.
     */
    static void update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);

    /**
     * Point handler for canvas groups.
     */
    static double point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);

    /**
     * Renders all visible canvas group items in buf rectangle.
     */
    static void render(SPCanvasItem *item, SPCanvasBuf *buf);

    static void viewboxChanged(SPCanvasItem *item, Geom::IntRect const &new_area);


    // Data members: ----------------------------------------------------------

    SPCanvasItem item;

    GList *items;
    GList *last;

    static SPCanvasItemClass *parentClass;
};

SPCanvasItemClass *SPCanvasGroup::parentClass;

GType sp_canvas_group_get_type()
{
    return SPCanvasGroup::getType();
}

/**
 * The SPCanvas vtable.
 */
struct SPCanvasClass {
    GtkWidgetClass parent_class;
};

namespace {

gint const UPDATE_PRIORITY = G_PRIORITY_HIGH_IDLE;

GdkWindow *getWindow(SPCanvas *canvas)
{
    return gtk_widget_get_window(reinterpret_cast<GtkWidget *>(canvas));
}

enum {
    SP_CANVAS_ITEM_VISIBLE = 1 << 7,
    SP_CANVAS_ITEM_NEED_UPDATE = 1 << 8,
    SP_CANVAS_ITEM_NEED_AFFINE = 1 << 9
};

// SPCanvasItem

enum {
    ITEM_EVENT,
    ITEM_LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VISIBLE
};


void trackLatency(GdkEvent const *event);

/**
 * Initializes the SPCanvasItem vtable and the "event" signal.
 */
void sp_canvas_item_class_init(SPCanvasItemClass *klass);

/**
 * Callback for initialization of SPCanvasItem.
 */
void sp_canvas_item_init(SPCanvasItem *item);

/**
 * Callback that removes item from all referers and destroys it.
 */
void sp_canvas_item_dispose(GObject *object);

/**
 * Sets up the newly created SPCanvasItem.
 *
 * We make it static for encapsulation reasons since it was nowhere used.
 */
void sp_canvas_item_construct(SPCanvasItem *item, SPCanvasGroup *parent, gchar const *first_arg_name, va_list args);

/**
 * Convenience function to reorder items in a group's child list.
 *
 * This puts the specified link after the "before" link.
 */
void put_item_after(GList *link, GList *before);

/**
 * Helper that returns true iff item is descendant of parent.
 */
bool is_descendant(SPCanvasItem const *item, SPCanvasItem const *parent);

guint item_signals[ITEM_LAST_SIGNAL] = { 0 };

struct PaintRectSetup;

} // namespace

class SPCanvasImpl
{
public:

    /**
     * Helper that emits an event for an item in the canvas, be it the current
     * item, grabbed item, or focused item, as appropriate.
     */
    static int emitEvent(SPCanvas *canvas, GdkEvent *event);

    /**
     * Helper that re-picks the current item in the canvas, based on the event's
     * coordinates and emits enter/leave events for items as appropriate.
     */
    static int pickCurrentItem(SPCanvas *canvas, GdkEvent *event);

    /**
     * Class initialization function for SPCanvasClass.
     */
    static void classInit(SPCanvasClass *klass);

    /**
     * Callback: object initialization for SPCanvas.
     */
    static void init(SPCanvas *canvas);

    /**
     * Destroy handler for SPCanvas.
     */
    static void destroy(GtkObject *object);

    /**
     * The canvas widget's realize callback.
     */
    static void realize(GtkWidget *widget);

    /**
     * The canvas widget's unrealize callback.
     */
    static void unrealize(GtkWidget *widget);

    /**
     * The canvas widget's size request callback.
     */
    static void sizeRequest(GtkWidget *widget, GtkRequisition *req);

    /**
     * The canvas widget's size allocate callback.
     */
    static void sizeAllocate(GtkWidget *widget, GtkAllocation *allocation);

    /**
     * Button event handler for the canvas.
     */
    static gint button(GtkWidget *widget, GdkEventButton *event);

    /**
     * Scroll event handler for the canvas.
     *
     * @todo FIXME: generate motion events to re-select items.
     */
    static gint handleScroll(GtkWidget *widget, GdkEventScroll *event);

    /**
     * Motion event handler for the canvas.
     */
    static gint handleMotion(GtkWidget *widget, GdkEventMotion *event);

    /**
     * The canvas widget's expose callback.
     *
     * @todo FIXME: function allways retruns false.
     */
    static gint handleExpose(GtkWidget *widget, GdkEventExpose *event);

    /**
     * The canvas widget's keypress callback.
     */
    static gint handleKeyEvent(GtkWidget *widget, GdkEventKey *event);

    /**
     * Crossing event handler for the canvas.
     */
    static gint handleCrossing(GtkWidget *widget, GdkEventCrossing *event);

    /**
     * Focus in handler for the canvas.
     */
    static gint handleFocusIn(GtkWidget *widget, GdkEventFocus *event);

    /**
     * Focus out handler for the canvas.
     */
    static gint handleFocusOut(GtkWidget *widget, GdkEventFocus *event);

    /**
     * Helper that allocates a new tile array for the canvas, copying overlapping tiles from the old array
     */
    static void sp_canvas_resize_tiles(SPCanvas* canvas, int nl, int nt, int nr, int nb);

    /**
     * Helper that queues a canvas rectangle for redraw
     */
    static void sp_canvas_dirty_rect(SPCanvas* canvas, Geom::IntRect const &area);

    /**
     * Helper that marks specific canvas rectangle as clean (val == 0) or dirty (otherwise)
     */
    static void sp_canvas_mark_rect(SPCanvas* canvas, Geom::IntRect const &area, uint8_t val);

    /**
     * Helper that invokes update, paint, and repick on canvas.
     */
    static int do_update(SPCanvas *canvas);

    static void sp_canvas_paint_single_buffer(SPCanvas *canvas, Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect, int sw);

    /**
     * Paint the given rect, recursively subdividing the region until it is the size of a single
     * buffer.
     *
     * @return true if the drawing completes
     */
    static int sp_canvas_paint_rect_internal(PaintRectSetup const *setup, Geom::IntRect const &this_rect);

    /**
     * Helper that draws a specific rectangular part of the canvas.
     *
     * @return true if the rectangle painting succeeds.
     */
    static bool sp_canvas_paint_rect(SPCanvas *canvas, int xx0, int yy0, int xx1, int yy1);

    /**
     * Helper that repaints the areas in the canvas that need it.
     *
     * @return true if all the dirty parts have been redrawn
     */
    static int paint(SPCanvas *canvas);

    /**
     * Idle handler for the canvas that deals with pending updates and redraws.
     */
    static gint idle_handler(gpointer data);

    /**
     * Convenience function to add an idle handler to a canvas.
     */
    static void add_idle(SPCanvas *canvas);

    /**
     * Convenience function to remove the idle handler of a canvas.
     */
    static void remove_idle(SPCanvas *canvas);

    /**
     * Removes the transient state of the canvas (idle handler, grabs).
     */
    static void shutdown_transients(SPCanvas *canvas);

    /**
     * Update callback for canvas widget.
     */
    static void requestCanvasUpdate(SPCanvas *canvas);

    static GtkWidgetClass *parentClass;
};

GtkWidgetClass *SPCanvasImpl::parentClass = 0;

GType SPCanvasItem::getType()
{
    static GType type = 0;
    if (!type) {
        static GTypeInfo const info = {
            sizeof(SPCanvasItemClass),
            NULL, NULL,
            reinterpret_cast<GClassInitFunc>(sp_canvas_item_class_init),
            NULL, NULL,
            sizeof(SPCanvasItem),
            0,
            reinterpret_cast<GInstanceInitFunc>(sp_canvas_item_init),
            NULL
        };
        type = g_type_register_static(GTK_TYPE_OBJECT, "SPCanvasItem", &info, GTypeFlags(0));
    }

    return type;
}

namespace {

void sp_canvas_item_class_init(SPCanvasItemClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    item_signals[ITEM_EVENT] = g_signal_new ("event",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_LAST,
                                             ((glong)((guint8*)&(klass->event) - (guint8*)klass)),
                                             NULL, NULL,
                                             sp_marshal_BOOLEAN__POINTER,
                                             G_TYPE_BOOLEAN, 1,
                                             GDK_TYPE_EVENT);

    object_class->dispose = sp_canvas_item_dispose;
}

void sp_canvas_item_init(SPCanvasItem *item)
{
    // TODO items should not be visible on creation - this causes kludges with items
    // that should be initially invisible; examples of such items: node handles, the CtrlRect
    // used for rubberbanding, path outline, etc.
    item->flags |= SP_CANVAS_ITEM_VISIBLE;
    item->xform = Geom::Affine(Geom::identity());
}

} // namespace

/**
 * Constructs new SPCanvasItem on SPCanvasGroup.
 */
SPCanvasItem *sp_canvas_item_new(SPCanvasGroup *parent, GType type, gchar const *first_arg_name, ...)
{
    va_list args;

    g_return_val_if_fail(parent != NULL, NULL);
    g_return_val_if_fail(SP_IS_CANVAS_GROUP (parent), NULL);
    g_return_val_if_fail(g_type_is_a(type, SPCanvasItem::getType()), NULL);

    SPCanvasItem *item = SP_CANVAS_ITEM (g_object_new (type, NULL));

    va_start (args, first_arg_name);
    sp_canvas_item_construct (item, parent, first_arg_name, args);
    va_end (args);

    return item;
}

namespace {

void sp_canvas_item_construct(SPCanvasItem *item, SPCanvasGroup *parent, gchar const *first_arg_name, va_list args)
{
    g_return_if_fail (SP_IS_CANVAS_GROUP (parent));
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    item->parent = SP_CANVAS_ITEM (parent);
    item->canvas = item->parent->canvas;

    g_object_set_valist (G_OBJECT (item), first_arg_name, args);

    SP_CANVAS_GROUP(item->parent)->add(item);

    sp_canvas_item_request_update (item);
}

} // namespace

/**
 * Helper function that requests redraw only if item's visible flag is set.
 */
static void redraw_if_visible(SPCanvasItem *item)
{
    if (item->flags & SP_CANVAS_ITEM_VISIBLE) {
        int x0 = (int)(item->x1);
        int x1 = (int)(item->x2);
        int y0 = (int)(item->y1);
        int y1 = (int)(item->y2);

        if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
            item->canvas->requestRedraw((int)(item->x1), (int)(item->y1), (int)(item->x2 + 1), (int)(item->y2 + 1));
        }
    }
}

namespace {

void sp_canvas_item_dispose(GObject *object)
{
    SPCanvasItem *item = SP_CANVAS_ITEM (object);

    // Hack: if this is a ctrlrect, move it to 0,0;
    // this redraws only the stroke of the rect to be deleted,
    // avoiding redraw of the entire area
    if (SP_IS_CTRLRECT(item)) {
        SP_CTRLRECT(object)->setRectangle(Geom::Rect(Geom::Point(0,0),Geom::Point(0,0)));
        SP_CTRLRECT(object)->update(item->xform, 0);
    } else {
        redraw_if_visible (item);
    }
    item->flags &= ~SP_CANVAS_ITEM_VISIBLE;

    if (item == item->canvas->current_item) {
        item->canvas->current_item = NULL;
        item->canvas->need_repick = TRUE;
    }

    if (item == item->canvas->new_current_item) {
        item->canvas->new_current_item = NULL;
        item->canvas->need_repick = TRUE;
    }

    if (item == item->canvas->grabbed_item) {
        item->canvas->grabbed_item = NULL;
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
    }

    if (item == item->canvas->focused_item) {
        item->canvas->focused_item = NULL;
    }

    if (item->parent) {
        SP_CANVAS_GROUP(item->parent)->remove(item);
    }

    G_OBJECT_CLASS(g_type_class_peek(g_type_parent(SPCanvasItem::getType())))->dispose(object);
}

} // namespace

/**
 * Helper function to update item and its children.
 *
 * NB! affine is parent2canvas.
 */
static void sp_canvas_item_invoke_update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    // Apply the child item's transform
    Geom::Affine child_affine = item->xform * affine;

    // apply object flags to child flags
    int child_flags = flags & ~SP_CANVAS_UPDATE_REQUESTED;

    if (item->flags & SP_CANVAS_ITEM_NEED_UPDATE) {
        child_flags |= SP_CANVAS_UPDATE_REQUESTED;
    }

    if (item->flags & SP_CANVAS_ITEM_NEED_AFFINE) {
        child_flags |= SP_CANVAS_UPDATE_AFFINE;
    }

    if (child_flags & (SP_CANVAS_UPDATE_REQUESTED | SP_CANVAS_UPDATE_AFFINE)) {
        if (SP_CANVAS_ITEM_GET_CLASS (item)->update) {
            SP_CANVAS_ITEM_GET_CLASS (item)->update(item, child_affine, child_flags);
        }
    }

    GTK_OBJECT_UNSET_FLAGS (item, SP_CANVAS_ITEM_NEED_UPDATE);
    GTK_OBJECT_UNSET_FLAGS (item, SP_CANVAS_ITEM_NEED_AFFINE);
}

/**
 * Helper function to invoke the point method of the item.
 *
 * The argument x, y should be in the parent's item-relative coordinate
 * system.  This routine applies the inverse of the item's transform,
 * maintaining the affine invariant.
 */
static double sp_canvas_item_invoke_point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    if (SP_CANVAS_ITEM_GET_CLASS(item)->point) {
        return SP_CANVAS_ITEM_GET_CLASS (item)->point (item, p, actual_item);
    }

    return Geom::infinity();
}

/**
 * Makes the item's affine transformation matrix be equal to the specified
 * matrix.
 *
 * @item: A canvas item.
 * @affine: An affine transformation matrix.
 */
void sp_canvas_item_affine_absolute(SPCanvasItem *item, Geom::Affine const &affine)
{
    item->xform = affine;

    if (!(item->flags & SP_CANVAS_ITEM_NEED_AFFINE)) {
        item->flags |= SP_CANVAS_ITEM_NEED_AFFINE;
        if (item->parent != NULL) {
            sp_canvas_item_request_update (item->parent);
        } else {
            SPCanvasImpl::requestCanvasUpdate(item->canvas);
        }
    }

    item->canvas->need_repick = TRUE;
}

namespace {

void put_item_after(GList *link, GList *before)
{
    if (link == before) {
        return;
    }

    SPCanvasGroup *parent = SP_CANVAS_GROUP (SP_CANVAS_ITEM (link->data)->parent);

    if (before == NULL) {
        if (link == parent->items) {
            return;
        }

        link->prev->next = link->next;

        if (link->next) {
            link->next->prev = link->prev;
        } else {
            parent->last = link->prev;
        }

        link->prev = before;
        link->next = parent->items;
        link->next->prev = link;
        parent->items = link;
    } else {
        if ((link == parent->last) && (before == parent->last->prev)) {
            return;
        }

        if (link->next) {
            link->next->prev = link->prev;
        }

        if (link->prev) {
            link->prev->next = link->next;
        } else {
            parent->items = link->next;
            parent->items->prev = NULL;
        }

        link->prev = before;
        link->next = before->next;

        link->prev->next = link;

        if (link->next) {
            link->next->prev = link;
        } else {
            parent->last = link;
        }
    }
}

} // namespace

/**
 * Raises the item in its parent's stack by the specified number of positions.
 *
 * @param item A canvas item.
 * @param positions Number of steps to raise the item.
 *
 * If the number of positions is greater than the distance to the top of the
 * stack, then the item is put at the top.
 */
void sp_canvas_item_raise(SPCanvasItem *item, int positions)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    g_return_if_fail (positions >= 0);

    if (!item->parent || positions == 0) {
        return;
    }

    SPCanvasGroup *parent = SP_CANVAS_GROUP (item->parent);
    GList *link = g_list_find (parent->items, item);
    g_assert (link != NULL);

    GList *before;
    for (before = link; positions && before; positions--)
        before = before->next;

    if (!before) {
        before = parent->last;
    }

    put_item_after (link, before);

    redraw_if_visible (item);
    item->canvas->need_repick = TRUE;
}


/**
 * Lowers the item in its parent's stack by the specified number of positions.
 *
 * @param item A canvas item.
 * @param positions Number of steps to lower the item.
 *
 * If the number of positions is greater than the distance to the bottom of the
 * stack, then the item is put at the bottom.
 */
void sp_canvas_item_lower(SPCanvasItem *item, int positions)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));
    g_return_if_fail (positions >= 1);

    if (!item->parent || positions == 0) {
        return;
    }

    SPCanvasGroup *parent = SP_CANVAS_GROUP (item->parent);
    GList *link = g_list_find (parent->items, item);
    g_assert (link != NULL);

    GList *before;
    if (link->prev) {
        for (before = link->prev; positions && before; positions--) {
            before = before->prev;
        }
    } else {
        before = NULL;
    }

    put_item_after (link, before);

    redraw_if_visible (item);
    item->canvas->need_repick = TRUE;
}

bool sp_canvas_item_is_visible(SPCanvasItem *item)
{
    return item->flags & SP_CANVAS_ITEM_VISIBLE;
}


/**
 * Sets visible flag on item and requests a redraw.
 */
void sp_canvas_item_show(SPCanvasItem *item)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (item->flags & SP_CANVAS_ITEM_VISIBLE) {
        return;
    }

    item->flags |= SP_CANVAS_ITEM_VISIBLE;

    int x0 = (int)(item->x1);
    int x1 = (int)(item->x2);
    int y0 = (int)(item->y1);
    int y1 = (int)(item->y2);

    if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
        item->canvas->requestRedraw((int)(item->x1), (int)(item->y1), (int)(item->x2 + 1), (int)(item->y2 + 1));
        item->canvas->need_repick = TRUE;
    }
}

/**
 * Clears visible flag on item and requests a redraw.
 */
void sp_canvas_item_hide(SPCanvasItem *item)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (!(item->flags & SP_CANVAS_ITEM_VISIBLE)) {
        return;
    }

    item->flags &= ~SP_CANVAS_ITEM_VISIBLE;

    int x0 = (int)(item->x1);
    int x1 = (int)(item->x2);
    int y0 = (int)(item->y1);
    int y1 = (int)(item->y2);

    if (x0 !=0 || x1 !=0 || y0 !=0 || y1 !=0) {
        item->canvas->requestRedraw((int)item->x1, (int)item->y1, (int)(item->x2 + 1), (int)(item->y2 + 1));
        item->canvas->need_repick = TRUE;
    }
}

/**
 * Grab item under cursor.
 *
 * \pre !canvas->grabbed_item && item->flags & SP_CANVAS_ITEM_VISIBLE
 */
int sp_canvas_item_grab(SPCanvasItem *item, guint event_mask, GdkCursor *cursor, guint32 etime)
{
    g_return_val_if_fail (item != NULL, -1);
    g_return_val_if_fail (SP_IS_CANVAS_ITEM (item), -1);
    g_return_val_if_fail (gtk_widget_get_mapped (GTK_WIDGET (item->canvas)), -1);

    if (item->canvas->grabbed_item) {
        return -1;
    }

    // This test disallows grabbing events by an invisible item, which may be useful
    // sometimes. An example is the hidden control point used for the selector component,
    // where it is used for object selection and rubberbanding. There seems to be nothing
    // preventing this except this test, so I removed it.
    // -- Krzysztof Kosiński, 2009.08.12
    //if (!(item->flags & SP_CANVAS_ITEM_VISIBLE))
    //    return -1;

    if (HAS_BROKEN_MOTION_HINTS) {
        event_mask &= ~GDK_POINTER_MOTION_HINT_MASK;
    }

    // fixme: Top hack (Lauris)
    // fixme: If we add key masks to event mask, Gdk will abort (Lauris)
    // fixme: But Canvas actualle does get key events, so all we need is routing these here
    gdk_pointer_grab( getWindow(item->canvas), FALSE,
                      (GdkEventMask)(event_mask & (~(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK))),
                      NULL, cursor, etime);

    item->canvas->grabbed_item = item;
    item->canvas->grabbed_event_mask = event_mask;
    item->canvas->current_item = item; // So that events go to the grabbed item

    return 0;
}

/**
 * Ungrabs the item, which must have been grabbed in the canvas, and ungrabs the
 * mouse.
 *
 * @param item A canvas item that holds a grab.
 * @param etime The timestamp for ungrabbing the mouse.
 */
void sp_canvas_item_ungrab(SPCanvasItem *item, guint32 etime)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (SP_IS_CANVAS_ITEM (item));

    if (item->canvas->grabbed_item != item) {
        return;
    }

    item->canvas->grabbed_item = NULL;

    gdk_pointer_ungrab (etime);
}

/**
 * Returns the product of all transformation matrices from the root item down
 * to the item.
 */
Geom::Affine sp_canvas_item_i2w_affine(SPCanvasItem const *item)
{
    g_assert (SP_IS_CANVAS_ITEM (item)); // should we get this?

    Geom::Affine affine = Geom::identity();

    while (item) {
        affine *= item->xform;
        item = item->parent;
    }
    return affine;
}

namespace {

bool is_descendant(SPCanvasItem const *item, SPCanvasItem const *parent)
{
    while (item) {
        if (item == parent) {
            return true;
        }
        item = item->parent;
    }

    return false;
}

} // namespace

/**
 * Requests that the canvas queue an update for the specified item.
 *
 * To be used only by item implementations.
 */
void sp_canvas_item_request_update(SPCanvasItem *item)
{
    if (item->flags & SP_CANVAS_ITEM_NEED_UPDATE) {
        return;
    }

    item->flags |= SP_CANVAS_ITEM_NEED_UPDATE;

    if (item->parent != NULL) {
        // Recurse up the tree
        sp_canvas_item_request_update (item->parent);
    } else {
        // Have reached the top of the tree, make sure the update call gets scheduled.
        SPCanvasImpl::requestCanvasUpdate(item->canvas);
    }
}

/**
 * Returns position of item in group.
 */
gint sp_canvas_item_order (SPCanvasItem * item)
{
    return g_list_index (SP_CANVAS_GROUP (item->parent)->items, item);
}

// SPCanvasGroup

/**
 * Registers SPCanvasGroup class with Gtk and returns its type number.
 */
GType SPCanvasGroup::getType(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPCanvasGroupClass),
            0, // base_init
            0, // base_finalize
            reinterpret_cast<GClassInitFunc>(SPCanvasGroup::classInit),
            0, // class_finalize
            0, // class_data
            sizeof(SPCanvasGroup),
            0, // n_preallocs
            reinterpret_cast<GInstanceInitFunc>(SPCanvasGroup::init),
            0 // value_table
        };
        type = g_type_register_static(SPCanvasItem::getType(), "SPCanvasGroup", &info, static_cast<GTypeFlags>(0));
    }
    return type;
}

void SPCanvasGroup::classInit(SPCanvasGroupClass *klass)
{
    GtkObjectClass *object_class = reinterpret_cast<GtkObjectClass *>(klass);
    SPCanvasItemClass *item_class = reinterpret_cast<SPCanvasItemClass *>(klass);

    parentClass = reinterpret_cast<SPCanvasItemClass*>(g_type_class_peek_parent(klass));

    object_class->destroy = SPCanvasGroup::destroy;

    item_class->update = SPCanvasGroup::update;
    item_class->render = SPCanvasGroup::render;
    item_class->point = SPCanvasGroup::point;
    item_class->viewbox_changed = SPCanvasGroup::viewboxChanged;
}

void SPCanvasGroup::init(SPCanvasGroup * /*group*/)
{
    // Nothing here
}

void SPCanvasGroup::destroy(GtkObject *object)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(SP_IS_CANVAS_GROUP(object));

    SPCanvasGroup const *group = SP_CANVAS_GROUP(object);

    GList *list = group->items;
    while (list) {
        SPCanvasItem *child = reinterpret_cast<SPCanvasItem *>(list->data);
        list = list->next;

        gtk_object_destroy(GTK_OBJECT(child));
    }

    if (GTK_OBJECT_CLASS(parentClass)->destroy) {
        (* GTK_OBJECT_CLASS(parentClass)->destroy)(object);
    }
}

void SPCanvasGroup::update(SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    SPCanvasGroup const *group = SP_CANVAS_GROUP(item);
    Geom::OptRect bounds;

    for (GList *list = group->items; list; list = list->next) {
        SPCanvasItem *i = (SPCanvasItem *)list->data;

        sp_canvas_item_invoke_update (i, affine, flags);

        if ( (i->x2 > i->x1) && (i->y2 > i->y1) ) {
            bounds.expandTo(Geom::Point(i->x1, i->y1));
            bounds.expandTo(Geom::Point(i->x2, i->y2));
        }
    }

    if (bounds) {
        item->x1 = bounds->min()[Geom::X];
        item->y1 = bounds->min()[Geom::Y];
        item->x2 = bounds->max()[Geom::X];
        item->y2 = bounds->max()[Geom::Y];
    } else {
        // FIXME ?
        item->x1 = item->x2 = item->y1 = item->y2 = 0;
    }
}

double SPCanvasGroup::point(SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    SPCanvasGroup const *group = SP_CANVAS_GROUP(item);
    double const x = p[Geom::X];
    double const y = p[Geom::Y];
    int x1 = (int)(x - item->canvas->close_enough);
    int y1 = (int)(y - item->canvas->close_enough);
    int x2 = (int)(x + item->canvas->close_enough);
    int y2 = (int)(y + item->canvas->close_enough);

    double best = 0.0;
    *actual_item = NULL;

    double dist = 0.0;

    for (GList *list = group->items; list; list = list->next) {
        SPCanvasItem *child = (SPCanvasItem *)list->data;

        if ((child->x1 <= x2) && (child->y1 <= y2) && (child->x2 >= x1) && (child->y2 >= y1)) {
            SPCanvasItem *point_item = NULL; // cater for incomplete item implementations

            int has_point;
            if ((child->flags & SP_CANVAS_ITEM_VISIBLE) && SP_CANVAS_ITEM_GET_CLASS (child)->point) {
                dist = sp_canvas_item_invoke_point (child, p, &point_item);
                has_point = TRUE;
            } else {
                has_point = FALSE;
            }

            // This metric should be improved, because in case of (partly) overlapping items we will now
            // always select the last one that has been added to the group. We could instead select the one
            // of which the center is the closest, for example. One can then move to the center
            // of the item to be focused, and have that one selected. Of course this will only work if the
            // centers are not coincident, but at least it's better than what we have now.
            // See the extensive comment in Inkscape::SelTrans::_updateHandles()
            if (has_point && point_item && ((int) (dist + 0.5) <= item->canvas->close_enough)) {
                best = dist;
                *actual_item = point_item;
            }
        }
    }

    return best;
}

void SPCanvasGroup::render(SPCanvasItem *item, SPCanvasBuf *buf)
{
    SPCanvasGroup const *group = SP_CANVAS_GROUP(item);

    for (GList *list = group->items; list; list = list->next) {
        SPCanvasItem *child = (SPCanvasItem *)list->data;
        if (child->flags & SP_CANVAS_ITEM_VISIBLE) {
            if ((child->x1 < buf->rect.right()) &&
                (child->y1 < buf->rect.bottom()) &&
                (child->x2 > buf->rect.left()) &&
                (child->y2 > buf->rect.top())) {
                if (SP_CANVAS_ITEM_GET_CLASS(child)->render) {
                    SP_CANVAS_ITEM_GET_CLASS(child)->render(child, buf);
                }
            }
        }
    }
}

void SPCanvasGroup::viewboxChanged(SPCanvasItem *item, Geom::IntRect const &new_area)
{
    SPCanvasGroup *group = SP_CANVAS_GROUP(item);
    
    for (GList *list = group->items; list; list = list->next) {
        SPCanvasItem *child = (SPCanvasItem *)list->data;
        if (child->flags & SP_CANVAS_ITEM_VISIBLE) {
            if (SP_CANVAS_ITEM_GET_CLASS(child)->viewbox_changed) {
                SP_CANVAS_ITEM_GET_CLASS(child)->viewbox_changed(child, new_area);
            }
        }
    }
}

void SPCanvasGroup::add(SPCanvasItem *item)
{
    gtk_object_ref( GTK_OBJECT(item) );
    g_object_ref_sink(item);

    if (!items) {
        items = g_list_append(items, item);
        last = items;
    } else {
        last = g_list_append(last, item)->next;
    }

    sp_canvas_item_request_update(item);
}

void SPCanvasGroup::remove(SPCanvasItem *item)
{
    g_return_if_fail(item != NULL);

    for (GList *children = items; children; children = children->next) {
        if (children->data == item) {

            // Unparent the child
            item->parent = NULL;
            gtk_object_unref(GTK_OBJECT(item));

            // Remove it from the list
            if (children == last) {
                last = children->prev;
            }

            items = g_list_remove_link(items, children);
            g_list_free(children);
            break;
        }
    }
}

/**
 * Registers the SPCanvas class if necessary, and returns the type ID
 * associated to it.
 *
 * @return The type ID of the SPCanvas class.
 */
GType SPCanvas::getType(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPCanvasClass),
            0, // base_init
            0, // base_finalize
            (GClassInitFunc)SPCanvasImpl::classInit,
            0, // class_finalize
            0, // class_data
            sizeof(SPCanvas),
            0, // n_preallocs
            (GInstanceInitFunc)SPCanvasImpl::init,
            0 // value_table
        };
        type = g_type_register_static(GTK_TYPE_WIDGET, "SPCanvas", &info, static_cast<GTypeFlags>(0));
    }
    return type;
}

void SPCanvasImpl::classInit(SPCanvasClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    parentClass = reinterpret_cast<GtkWidgetClass *>(g_type_class_peek_parent(klass));

    object_class->destroy = SPCanvasImpl::destroy;

    widget_class->realize = SPCanvasImpl::realize;
    widget_class->unrealize = SPCanvasImpl::unrealize;
    widget_class->size_request = SPCanvasImpl::sizeRequest;
    widget_class->size_allocate = SPCanvasImpl::sizeAllocate;
    widget_class->button_press_event = SPCanvasImpl::button;
    widget_class->button_release_event = SPCanvasImpl::button;
    widget_class->motion_notify_event = SPCanvasImpl::handleMotion;
    widget_class->scroll_event = SPCanvasImpl::handleScroll;
    widget_class->expose_event = SPCanvasImpl::handleExpose;
    widget_class->key_press_event = SPCanvasImpl::handleKeyEvent;
    widget_class->key_release_event = SPCanvasImpl::handleKeyEvent;
    widget_class->enter_notify_event = SPCanvasImpl::handleCrossing;
    widget_class->leave_notify_event = SPCanvasImpl::handleCrossing;
    widget_class->focus_in_event = SPCanvasImpl::handleFocusIn;
    widget_class->focus_out_event = SPCanvasImpl::handleFocusOut;
}

void SPCanvasImpl::init(SPCanvas *canvas)
{
    gtk_widget_set_has_window (GTK_WIDGET (canvas), TRUE);
    gtk_widget_set_double_buffered (GTK_WIDGET (canvas), FALSE);
    gtk_widget_set_can_focus (GTK_WIDGET (canvas), TRUE);

    canvas->pick_event.type = GDK_LEAVE_NOTIFY;
    canvas->pick_event.crossing.x = 0;
    canvas->pick_event.crossing.y = 0;

    // Create the root item as a special case
    canvas->root = SP_CANVAS_ITEM(g_object_new(SPCanvasGroup::getType(), NULL));
    canvas->root->canvas = canvas;

    gtk_object_ref (GTK_OBJECT (canvas->root));
    g_object_ref_sink (canvas->root);

    canvas->need_repick = TRUE;

    // See comment at in sp-canvas.h.
    canvas->gen_all_enter_events = false;
    
    canvas->drawing_disabled = false;

    canvas->tiles=NULL;
    canvas->tLeft=canvas->tTop=canvas->tRight=canvas->tBottom=0;
    canvas->tileH=canvas->tileV=0;

    canvas->forced_redraw_count = 0;
    canvas->forced_redraw_limit = -1;

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    canvas->enable_cms_display_adj = false;
    new (&canvas->cms_key) Glib::ustring("");
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    canvas->is_scrolling = false;
}

void SPCanvasImpl::remove_idle(SPCanvas *canvas)
{
    if (canvas->idle_id) {
        g_source_remove (canvas->idle_id);
        canvas->idle_id = 0;
    }
}

void SPCanvasImpl::shutdown_transients(SPCanvas *canvas)
{
    // We turn off the need_redraw flag, since if the canvas is mapped again
    // it will request a redraw anyways.  We do not turn off the need_update
    // flag, though, because updates are not queued when the canvas remaps
    // itself.
    //
    if (canvas->need_redraw) {
        canvas->need_redraw = FALSE;
    }
    if ( canvas->tiles ) g_free(canvas->tiles);
    canvas->tiles=NULL;
    canvas->tLeft=canvas->tTop=canvas->tRight=canvas->tBottom=0;
    canvas->tileH=canvas->tileV=0;

    if (canvas->grabbed_item) {
        canvas->grabbed_item = NULL;
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
    }

    remove_idle(canvas);
}

void SPCanvasImpl::destroy(GtkObject *object)
{
    SPCanvas *canvas = SP_CANVAS(object);

    if (canvas->root) {
        gtk_object_unref (GTK_OBJECT (canvas->root));
        canvas->root = NULL;
    }

    shutdown_transients(canvas);
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    canvas->cms_key.~ustring();
#endif
    if (GTK_OBJECT_CLASS(parentClass)->destroy) {
        (* GTK_OBJECT_CLASS(parentClass)->destroy)(object);
    }
}

namespace {

void trackLatency(GdkEvent const *event)
{
    GdkEventLatencyTracker &tracker = GdkEventLatencyTracker::default_tracker();
    boost::optional<double> latency = tracker.process(event);
    if (latency && *latency > 2.0) {
        //g_warning("Event latency reached %f sec (%1.4f)", *latency, tracker.getSkew());
    }
}

} // namespace

GtkWidget *SPCanvas::createAA()
{
    SPCanvas *canvas = reinterpret_cast<SPCanvas *>(g_object_new(SPCanvas::getType(), NULL));
    return GTK_WIDGET(canvas);
}

void SPCanvasImpl::realize(GtkWidget *widget)
{
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    attributes.window_type = GDK_WINDOW_CHILD;
    gtk_widget_get_allocation (widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gdk_rgb_get_visual ();
    attributes.colormap = gdk_rgb_get_cmap ();
    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_EXPOSURE_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_POINTER_MOTION_MASK |
                             ( HAS_BROKEN_MOTION_HINTS ?
                               0 : GDK_POINTER_MOTION_HINT_MASK ) |
                             GDK_PROXIMITY_IN_MASK |
                             GDK_PROXIMITY_OUT_MASK |
                             GDK_KEY_PRESS_MASK |
                             GDK_KEY_RELEASE_MASK |
                             GDK_ENTER_NOTIFY_MASK |
                             GDK_LEAVE_NOTIFY_MASK |
                             GDK_FOCUS_CHANGE_MASK);
    gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    GdkWindow *window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gtk_widget_set_window (widget, window);
    gdk_window_set_user_data (window, widget);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if ( prefs->getBool("/options/useextinput/value", true) )
        gtk_widget_set_events(widget, attributes.event_mask);

    GtkStyle *style = gtk_widget_get_style (widget);

    gtk_widget_set_style (widget, gtk_style_attach (style, window));

    gtk_widget_set_realized (widget, TRUE);
}

void SPCanvasImpl::unrealize(GtkWidget *widget)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    canvas->current_item = NULL;
    canvas->grabbed_item = NULL;
    canvas->focused_item = NULL;

    shutdown_transients(canvas);

    if (GTK_WIDGET_CLASS(parentClass)->unrealize)
        (* GTK_WIDGET_CLASS(parentClass)->unrealize)(widget);
}

void SPCanvasImpl::sizeRequest(GtkWidget *widget, GtkRequisition *req)
{
    static_cast<void>(SP_CANVAS (widget));

    req->width = 256;
    req->height = 256;
}

void SPCanvasImpl::sizeAllocate(GtkWidget *widget, GtkAllocation *allocation)
{
    SPCanvas *canvas = SP_CANVAS (widget);
    GtkAllocation widg_allocation;
   
    gtk_widget_get_allocation (widget, &widg_allocation);

    Geom::IntRect old_area = Geom::IntRect::from_xywh(canvas->x0, canvas->y0,
        widg_allocation.width, widg_allocation.height);
    Geom::IntRect new_area = Geom::IntRect::from_xywh(canvas->x0, canvas->y0,
        allocation->width, allocation->height);

    // Schedule redraw of new region
    sp_canvas_resize_tiles(canvas,canvas->x0,canvas->y0,canvas->x0+allocation->width,canvas->y0+allocation->height);
    if (SP_CANVAS_ITEM_GET_CLASS (canvas->root)->viewbox_changed)
        SP_CANVAS_ITEM_GET_CLASS (canvas->root)->viewbox_changed (canvas->root, new_area);
    
    if (allocation->width > widg_allocation.width) {
        canvas->requestRedraw(canvas->x0 + widg_allocation.width,
                              0,
                              canvas->x0 + allocation->width,
                              canvas->y0 + allocation->height);
    }
    if (allocation->height > widg_allocation.height) {
        canvas->requestRedraw(0,
                              canvas->y0 + widg_allocation.height,
                              canvas->x0 + allocation->width,
                              canvas->y0 + allocation->height);
    }

    gtk_widget_set_allocation (widget, allocation);

    if (gtk_widget_get_realized (widget)) {
        gdk_window_move_resize (gtk_widget_get_window (widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);
    }
}

int SPCanvasImpl::emitEvent(SPCanvas *canvas, GdkEvent *event)
{
    guint mask;

    if (canvas->grabbed_item) {
        switch (event->type) {
        case GDK_ENTER_NOTIFY:
            mask = GDK_ENTER_NOTIFY_MASK;
            break;
        case GDK_LEAVE_NOTIFY:
            mask = GDK_LEAVE_NOTIFY_MASK;
            break;
        case GDK_MOTION_NOTIFY:
            mask = GDK_POINTER_MOTION_MASK;
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
            mask = GDK_BUTTON_PRESS_MASK;
            break;
        case GDK_BUTTON_RELEASE:
            mask = GDK_BUTTON_RELEASE_MASK;
            break;
        case GDK_KEY_PRESS:
            mask = GDK_KEY_PRESS_MASK;
            break;
        case GDK_KEY_RELEASE:
            mask = GDK_KEY_RELEASE_MASK;
            break;
        case GDK_SCROLL:
            mask = GDK_SCROLL;
            break;
        default:
            mask = 0;
            break;
        }

        if (!(mask & canvas->grabbed_event_mask)) return FALSE;
    }

    // Convert to world coordinates -- we have two cases because of different
    // offsets of the fields in the event structures.
    //

    GdkEvent ev = *event;

    switch (ev.type) {
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
        ev.crossing.x += canvas->x0;
        ev.crossing.y += canvas->y0;
        break;
    case GDK_MOTION_NOTIFY:
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        ev.motion.x += canvas->x0;
        ev.motion.y += canvas->y0;
        break;
    default:
        break;
    }

    // Choose where we send the event

    // canvas->current_item becomes NULL in some cases under Win32
    // (e.g. if the pointer leaves the window).  So this is a hack that
    // Lauris applied to SP to get around the problem.
    //
    SPCanvasItem* item = NULL;
    if (canvas->grabbed_item && !is_descendant (canvas->current_item, canvas->grabbed_item)) {
        item = canvas->grabbed_item;
    } else {
        // Make sure that current_item is up-to-date. If a snap indicator was just deleted, then
        // sp_canvas_item_dispose has been called and there is no current_item specified. We need
        // that though because otherwise we don't know where to send this event to, leading to a
        // lost event. We can't wait for idle events to have current_item updated, we need it now!
        // Otherwise, scrolling when hovering above a pre-snap indicator won't work (for example)
        // See this bug report: https://bugs.launchpad.net/inkscape/+bug/522335/comments/8
        if (canvas->need_repick && !canvas->in_repick && event->type == GDK_SCROLL) {
            // To avoid side effects, we'll only do this for scroll events, because this is the
            // only thing we want to fix here. An example of a reported side effect is that
            // otherwise selection of nodes in the node editor by dragging a rectangle using a
            // tablet will break
            canvas->need_repick = FALSE;
            pickCurrentItem(canvas, reinterpret_cast<GdkEvent *>(event));
        }
        item = canvas->current_item;
    }

    if (canvas->focused_item &&
        ((event->type == GDK_KEY_PRESS) ||
         (event->type == GDK_KEY_RELEASE) ||
         (event->type == GDK_FOCUS_CHANGE))) {
        item = canvas->focused_item;
    }

    // The event is propagated up the hierarchy (for if someone connected to
    // a group instead of a leaf event), and emission is stopped if a
    // handler returns TRUE, just like for GtkWidget events.

    gint finished = FALSE;

    while (item && !finished) {
        gtk_object_ref (GTK_OBJECT (item));
        g_signal_emit (G_OBJECT (item), item_signals[ITEM_EVENT], 0, &ev, &finished);
        SPCanvasItem *parent = item->parent;
        gtk_object_unref (GTK_OBJECT (item));
        item = parent;
    }

    return finished;
}

int SPCanvasImpl::pickCurrentItem(SPCanvas *canvas, GdkEvent *event)
{
    int button_down = 0;
    double x, y;

    if (!canvas->root) // canvas may have already be destroyed by closing desktop durring interrupted display!
        return FALSE;

    int retval = FALSE;

    if (canvas->gen_all_enter_events == false) {
        // If a button is down, we'll perform enter and leave events on the
        // current item, but not enter on any other item.  This is more or
        // less like X pointer grabbing for canvas items.
        //
        button_down = canvas->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
                GDK_BUTTON3_MASK | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK);

        if (!button_down) canvas->left_grabbed_item = FALSE;
    }

    // Save the event in the canvas.  This is used to synthesize enter and
    // leave events in case the current item changes.  It is also used to
    // re-pick the current item if the current one gets deleted.  Also,
    // synthesize an enter event.

    if (event != &canvas->pick_event) {
        if ((event->type == GDK_MOTION_NOTIFY) || (event->type == GDK_BUTTON_RELEASE)) {
            // these fields have the same offsets in both types of events

            canvas->pick_event.crossing.type       = GDK_ENTER_NOTIFY;
            canvas->pick_event.crossing.window     = event->motion.window;
            canvas->pick_event.crossing.send_event = event->motion.send_event;
            canvas->pick_event.crossing.subwindow  = NULL;
            canvas->pick_event.crossing.x          = event->motion.x;
            canvas->pick_event.crossing.y          = event->motion.y;
            canvas->pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
            canvas->pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
            canvas->pick_event.crossing.focus      = FALSE;
            canvas->pick_event.crossing.state      = event->motion.state;

            // these fields don't have the same offsets in both types of events

            if (event->type == GDK_MOTION_NOTIFY) {
                canvas->pick_event.crossing.x_root = event->motion.x_root;
                canvas->pick_event.crossing.y_root = event->motion.y_root;
            } else {
                canvas->pick_event.crossing.x_root = event->button.x_root;
                canvas->pick_event.crossing.y_root = event->button.y_root;
            }
        } else {
            canvas->pick_event = *event;
        }
    }

    // Don't do anything else if this is a recursive call
    if (canvas->in_repick) {
        return retval;
    }

    // LeaveNotify means that there is no current item, so we don't look for one
    if (canvas->pick_event.type != GDK_LEAVE_NOTIFY) {
        // these fields don't have the same offsets in both types of events

        if (canvas->pick_event.type == GDK_ENTER_NOTIFY) {
            x = canvas->pick_event.crossing.x;
            y = canvas->pick_event.crossing.y;
        } else {
            x = canvas->pick_event.motion.x;
            y = canvas->pick_event.motion.y;
        }

        // world coords
        x += canvas->x0;
        y += canvas->y0;

        // find the closest item
        if (canvas->root->flags & SP_CANVAS_ITEM_VISIBLE) {
            sp_canvas_item_invoke_point (canvas->root, Geom::Point(x, y), &canvas->new_current_item);
        } else {
            canvas->new_current_item = NULL;
        }
    } else {
        canvas->new_current_item = NULL;
    }

    if ((canvas->new_current_item == canvas->current_item) && !canvas->left_grabbed_item) {
        return retval; // current item did not change
    }

    // Synthesize events for old and new current items

    if ((canvas->new_current_item != canvas->current_item)
        && (canvas->current_item != NULL)
        && !canvas->left_grabbed_item) {
        GdkEvent new_event;

        new_event = canvas->pick_event;
        new_event.type = GDK_LEAVE_NOTIFY;

        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = NULL;
        canvas->in_repick = TRUE;
        retval = emitEvent(canvas, &new_event);
        canvas->in_repick = FALSE;
    }

    if (canvas->gen_all_enter_events == false) {
        // new_current_item may have been set to NULL during the call to
        // emitEvent() above
        if ((canvas->new_current_item != canvas->current_item) && button_down) {
            canvas->left_grabbed_item = TRUE;
            return retval;
        }
    }

    // Handle the rest of cases

    canvas->left_grabbed_item = FALSE;
    canvas->current_item = canvas->new_current_item;

    if (canvas->current_item != NULL) {
        GdkEvent new_event;

        new_event = canvas->pick_event;
        new_event.type = GDK_ENTER_NOTIFY;
        new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
        new_event.crossing.subwindow = NULL;
        retval = emitEvent(canvas, &new_event);
    }

    return retval;
}

gint SPCanvasImpl::button(GtkWidget *widget, GdkEventButton *event)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    int retval = FALSE;

    // dispatch normally regardless of the event's window if an item
    // has a pointer grab in effect
    if (!canvas->grabbed_item &&
        event->window != getWindow(canvas))
        return retval;

    int mask;
    switch (event->button) {
    case 1:
        mask = GDK_BUTTON1_MASK;
        break;
    case 2:
        mask = GDK_BUTTON2_MASK;
        break;
    case 3:
        mask = GDK_BUTTON3_MASK;
        break;
    case 4:
        mask = GDK_BUTTON4_MASK;
        break;
    case 5:
        mask = GDK_BUTTON5_MASK;
        break;
    default:
        mask = 0;
    }

    switch (event->type) {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
        // Pick the current item as if the button were not pressed, and
        // then process the event.
        //
        canvas->state = event->state;
        pickCurrentItem(canvas, reinterpret_cast<GdkEvent *>(event));
        canvas->state ^= mask;
        retval = emitEvent(canvas, (GdkEvent *) event);
        break;

    case GDK_BUTTON_RELEASE:
        // Process the event as if the button were pressed, then repick
        // after the button has been released
        //
        canvas->state = event->state;
        retval = emitEvent(canvas, (GdkEvent *) event);
        event->state ^= mask;
        canvas->state = event->state;
        pickCurrentItem(canvas, reinterpret_cast<GdkEvent *>(event));
        event->state ^= mask;

        break;

    default:
        g_assert_not_reached ();
    }

    return retval;
}

gint SPCanvasImpl::handleScroll(GtkWidget *widget, GdkEventScroll *event)
{
    return emitEvent(SP_CANVAS(widget), reinterpret_cast<GdkEvent *>(event));
}

static inline void request_motions(GdkWindow *w, GdkEventMotion *event) {
    gdk_window_get_pointer(w, NULL, NULL, NULL);
    gdk_event_request_motions(event);
}

int SPCanvasImpl::handleMotion(GtkWidget *widget, GdkEventMotion *event)
{
    int status;
    SPCanvas *canvas = SP_CANVAS (widget);

    trackLatency((GdkEvent *)event);

    if (event->window != getWindow(canvas)) {
        return FALSE;
    }

    if (canvas->root == NULL) // canvas being deleted
        return FALSE;

    canvas->state = event->state;
    pickCurrentItem(canvas, reinterpret_cast<GdkEvent *>(event));
    status = emitEvent(canvas, reinterpret_cast<GdkEvent *>(event));
    if (event->is_hint) {
        request_motions(gtk_widget_get_window (widget), event);
    }

    return status;
}

void SPCanvasImpl::sp_canvas_paint_single_buffer(SPCanvas *canvas, Geom::IntRect const &paint_rect, Geom::IntRect const &canvas_rect, int /*sw*/)
{
    GtkWidget *widget = GTK_WIDGET (canvas);
    GtkStyle  *style;

    // Mark the region clean
    sp_canvas_mark_rect(canvas, paint_rect, 0);

    SPCanvasBuf buf;
    buf.buf = NULL;
    buf.buf_rowstride = 0;
    buf.rect = paint_rect;
    buf.visible_rect = canvas_rect;
    buf.is_empty = true;
    //buf.ct = gdk_cairo_create(widget->window);

    /*
    cairo_t *xctt = gdk_cairo_create(widget->window);
    cairo_translate(xctt, paint_rect.left() - canvas->x0, paint_rect.top() - canvas->y0);
    cairo_set_source_rgb(xctt, 1,0,0);
    cairo_rectangle(xctt, 0, 0, paint_rect.width(), paint_rect.height());
    cairo_fill(xctt);
    cairo_destroy(xctt);
    //
    */

    // create temporary surface
    cairo_surface_t *imgs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, paint_rect.width(), paint_rect.height());
    buf.ct = cairo_create(imgs);
    //cairo_translate(buf.ct, -x0, -y0);

    // fix coordinates, clip all drawing to the tile and clear the background
    //cairo_translate(buf.ct, paint_rect.left() - canvas->x0, paint_rect.top() - canvas->y0);
    //cairo_rectangle(buf.ct, 0, 0, paint_rect.width(), paint_rect.height());
    //cairo_set_line_width(buf.ct, 3);
    //cairo_set_source_rgba(buf.ct, 1.0, 0.0, 0.0, 0.1);
    //cairo_stroke_preserve(buf.ct);
    //cairo_clip(buf.ct);

    style = gtk_widget_get_style (widget);
    gdk_cairo_set_source_color(buf.ct, &style->bg[GTK_STATE_NORMAL]);
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_SOURCE);
    //cairo_rectangle(buf.ct, 0, 0, paint_rect.width(), paint_rec.height());
    cairo_paint(buf.ct);
    cairo_set_operator(buf.ct, CAIRO_OPERATOR_OVER);

    if (canvas->root->flags & SP_CANVAS_ITEM_VISIBLE) {
        SP_CANVAS_ITEM_GET_CLASS (canvas->root)->render (canvas->root, &buf);
    }

    // output to X
    cairo_destroy(buf.ct);

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    if (canvas->enable_cms_display_adj) {
        cmsHTRANSFORM transf = 0;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
        if ( fromDisplay ) {
            transf = Inkscape::CMSSystem::getDisplayPer( canvas->cms_key );
        } else {
            transf = Inkscape::CMSSystem::getDisplayTransform();
        }
        
        if (transf) {
            cairo_surface_flush(imgs);
            unsigned char *px = cairo_image_surface_get_data(imgs);
            int stride = cairo_image_surface_get_stride(imgs);
            for (int i=0; i<paint_rect.height(); ++i) {
                unsigned char *row = px + i*stride;
                Inkscape::CMSSystem::doTransform(transf, row, row, paint_rect.width());
            }
            cairo_surface_mark_dirty(imgs);
        }
    }
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    cairo_t *xct = gdk_cairo_create(gtk_widget_get_window (widget));
    cairo_translate(xct, paint_rect.left() - canvas->x0, paint_rect.top() - canvas->y0);
    cairo_rectangle(xct, 0, 0, paint_rect.width(), paint_rect.height());
    cairo_clip(xct);
    cairo_set_source_surface(xct, imgs, 0, 0);
    cairo_set_operator(xct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(xct);
    cairo_destroy(xct);
    cairo_surface_destroy(imgs);

    //cairo_surface_t *cst = cairo_get_target(buf.ct);
    //cairo_destroy (buf.ct);
    //cairo_surface_finish (cst);
    //cairo_surface_destroy (cst);
}

namespace {

struct PaintRectSetup {
    SPCanvas* canvas;
    Geom::IntRect big_rect;
    GTimeVal start_time;
    int max_pixels;
    Geom::Point mouse_loc;
};

}// namespace

int SPCanvasImpl::sp_canvas_paint_rect_internal(PaintRectSetup const *setup, Geom::IntRect const &this_rect)
{
    GTimeVal now;
    g_get_current_time (&now);

    glong elapsed = (now.tv_sec - setup->start_time.tv_sec) * 1000000
        + (now.tv_usec - setup->start_time.tv_usec);

    // Allow only very fast buffers to be run together;
    // as soon as the total redraw time exceeds 1ms, cancel;
    // this returns control to the idle loop and allows Inkscape to process user input
    // (potentially interrupting the redraw); as soon as Inkscape has some more idle time,
    // it will get back and finish painting what remains to paint.
    if (elapsed > 1000) {

        // Interrupting redraw isn't always good.
        // For example, when you drag one node of a big path, only the buffer containing
        // the mouse cursor will be redrawn again and again, and the rest of the path
        // will remain stale because Inkscape never has enough idle time to redraw all
        // of the screen. To work around this, such operations set a forced_redraw_limit > 0.
        // If this limit is set, and if we have aborted redraw more times than is allowed,
        // interrupting is blocked and we're forced to redraw full screen once
        // (after which we can again interrupt forced_redraw_limit times).
        if (setup->canvas->forced_redraw_limit < 0 ||
            setup->canvas->forced_redraw_count < setup->canvas->forced_redraw_limit) {

            if (setup->canvas->forced_redraw_limit != -1) {
                setup->canvas->forced_redraw_count++;
            }

            return false;
        }
    }

    // Find the optimal buffer dimensions
    int bw = this_rect.width();
    int bh = this_rect.height();
    if ((bw < 1) || (bh < 1))
        return 0;

    if (bw * bh < setup->max_pixels) {
        // We are small enough
        /*
        GdkRectangle r;
        r.x = this_rect.x0 - setup->canvas->x0;
        r.y = this_rect.y0 - setup->canvas->y0;
        r.width = this_rect.x1 - this_rect.x0;
        r.height = this_rect.y1 - this_rect.y0;

        GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(setup->canvas));
        gdk_window_begin_paint_rect(window, &r);
        */

        sp_canvas_paint_single_buffer (setup->canvas,
                                       this_rect, setup->big_rect, bw);
        //gdk_window_end_paint(window);
        return 1;
    }

    Geom::IntRect lo, hi;

/*
This test determines the redraw strategy:

bw < bh (strips mode) splits across the smaller dimension of the rect and therefore (on
horizontally-stretched windows) results in redrawing in horizontal strips (from cursor point, in
both directions if the cursor is in the middle). This is traditional for Inkscape since old days,
and seems to be faster for drawings with many smaller objects at zoom-out.

bw > bh (chunks mode) splits across the larger dimension of the rect and therefore paints in
almost-square chunks, again from the cursor point. It's sometimes faster for drawings with few slow
(e.g. blurred) objects crossing the entire screen. It also appears to be somewhat psychologically
faster.

The default for now is the strips mode.
*/
    if (bw < bh || bh < 2 * TILE_SIZE) {
        int mid = this_rect[Geom::X].middle();
        // Make sure that mid lies on a tile boundary
        mid = (mid / TILE_SIZE) * TILE_SIZE;

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), mid, this_rect.bottom());
        hi = Geom::IntRect(mid, this_rect.top(), this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::X] < mid) {
            // Always paint towards the mouse first
            return sp_canvas_paint_rect_internal(setup, lo)
                && sp_canvas_paint_rect_internal(setup, hi);
        } else {
            return sp_canvas_paint_rect_internal(setup, hi)
                && sp_canvas_paint_rect_internal(setup, lo);
        }
    } else {
        int mid = this_rect[Geom::Y].middle();
        // Make sure that mid lies on a tile boundary
        mid = (mid / TILE_SIZE) * TILE_SIZE;

        lo = Geom::IntRect(this_rect.left(), this_rect.top(), this_rect.right(), mid);
        hi = Geom::IntRect(this_rect.left(), mid, this_rect.right(), this_rect.bottom());

        if (setup->mouse_loc[Geom::Y] < mid) {
            // Always paint towards the mouse first
            return sp_canvas_paint_rect_internal(setup, lo)
                && sp_canvas_paint_rect_internal(setup, hi);
        } else {
            return sp_canvas_paint_rect_internal(setup, hi)
                && sp_canvas_paint_rect_internal(setup, lo);
        }
    }
}


bool SPCanvasImpl::sp_canvas_paint_rect(SPCanvas *canvas, int xx0, int yy0, int xx1, int yy1)
{
    GtkAllocation allocation;
    g_return_val_if_fail (!canvas->need_update, false);

    gtk_widget_get_allocation (GTK_WIDGET (canvas), &allocation);

    Geom::IntRect canvas_rect = Geom::IntRect::from_xywh(canvas->x0, canvas->y0,
        allocation.width, allocation.height);
    Geom::IntRect paint_rect(xx0, yy0, xx1, yy1);

    Geom::OptIntRect area = paint_rect & canvas_rect;
    if (!area || area->hasZeroArea()) return 0;

    paint_rect = *area;

    PaintRectSetup setup;

    setup.canvas = canvas;
    setup.big_rect = paint_rect;

    // Save the mouse location
    gint x, y;
    gdk_window_get_pointer (gtk_widget_get_window (GTK_WIDGET(canvas)), &x, &y, NULL);
    setup.mouse_loc = sp_canvas_window_to_world (canvas, Geom::Point(x,y));

    if (canvas->rendermode != Inkscape::RENDERMODE_OUTLINE) {
        // use 256K as a compromise to not slow down gradients
        // 256K is the cached buffer and we need 4 channels
        setup.max_pixels = 65536; // 256K/4
    } else {
        // paths only, so 1M works faster
        // 1M is the cached buffer and we need 4 channels
        setup.max_pixels = 262144;
    }

    // Start the clock
    g_get_current_time(&(setup.start_time));

    // Go
    return sp_canvas_paint_rect_internal(&setup, paint_rect);
}

void SPCanvas::forceFullRedrawAfterInterruptions(unsigned int count)
{
    forced_redraw_limit = count;
    forced_redraw_count = 0;
}

void SPCanvas::endForcedFullRedraws()
{
    forced_redraw_limit = -1;
}

gint SPCanvasImpl::handleExpose(GtkWidget *widget, GdkEventExpose *event)
{
    SPCanvas *canvas = SP_CANVAS(widget);

    if (!gtk_widget_is_drawable (widget) ||
        (event->window != getWindow(canvas))) {
        return FALSE;
    }

    int n_rects = 0;
    GdkRectangle *rects = NULL;
    gdk_region_get_rectangles(event->region, &rects, &n_rects);
    
    if ((rects == NULL) || (n_rects == 0))
    {
        return FALSE;
    }
    else
    {
        for (int i = 0; i < n_rects; i++) {
            Geom::IntRect r = Geom::IntRect::from_xywh(
                rects[i].x + canvas->x0, rects[i].y + canvas->y0,
                rects[i].width, rects[i].height);
            
            canvas->requestRedraw(r.left(), r.top(), r.right(), r.bottom());
        }
        
        g_free (rects);
        return FALSE;
    }
}

gint SPCanvasImpl::handleKeyEvent(GtkWidget *widget, GdkEventKey *event)
{
    return emitEvent(SP_CANVAS(widget), reinterpret_cast<GdkEvent *>(event));
}

gint SPCanvasImpl::handleCrossing(GtkWidget *widget, GdkEventCrossing *event)
{
    SPCanvas *canvas = SP_CANVAS (widget);

    if (event->window != getWindow(canvas)) {
        return FALSE;
    }

    canvas->state = event->state;
    return pickCurrentItem(canvas, reinterpret_cast<GdkEvent *>(event));
}

gint SPCanvasImpl::handleFocusIn(GtkWidget *widget, GdkEventFocus *event)
{
    gtk_widget_grab_focus (widget);

    SPCanvas *canvas = SP_CANVAS (widget);

    if (canvas->focused_item) {
        return emitEvent(canvas, reinterpret_cast<GdkEvent *>(event));
    } else {
        return FALSE;
    }
}

gint SPCanvasImpl::handleFocusOut(GtkWidget *widget, GdkEventFocus *event)
{
    SPCanvas *canvas = SP_CANVAS(widget);

    if (canvas->focused_item) {
        return emitEvent(canvas, reinterpret_cast<GdkEvent *>(event));
    } else {
        return FALSE;
    }
}

int SPCanvasImpl::paint(SPCanvas *canvas)
{
    if (canvas->need_update) {
        sp_canvas_item_invoke_update (canvas->root, Geom::identity(), 0);
        canvas->need_update = FALSE;
    }

    if (!canvas->need_redraw) {
        return TRUE;
    }

    Gdk::Region to_paint;

    for (int j=canvas->tTop; j<canvas->tBottom; j++) {
        for (int i=canvas->tLeft; i<canvas->tRight; i++) {
            int tile_index = (i - canvas->tLeft) + (j - canvas->tTop)*canvas->tileH;

            if ( canvas->tiles[tile_index] ) { // if this tile is dirtied (nonzero)
                to_paint.union_with_rect(Gdk::Rectangle(i*TILE_SIZE, j*TILE_SIZE,
                                   TILE_SIZE, TILE_SIZE));
            }

        }
    }

    if (!to_paint.empty()) {
        Glib::ArrayHandle<Gdk::Rectangle> rect = to_paint.get_rectangles();
        typedef Glib::ArrayHandle<Gdk::Rectangle>::const_iterator Iter;
        for (Iter i=rect.begin(); i != rect.end(); ++i) {
            int x0 = (*i).get_x();
            int y0 = (*i).get_y();
            int x1 = x0 + (*i).get_width();
            int y1 = y0 + (*i).get_height();
            if (!sp_canvas_paint_rect(canvas, x0, y0, x1, y1)) {
                // Aborted
                return FALSE;
            };
        }
    }

    canvas->need_redraw = FALSE;

    // we've had a full unaborted redraw, reset the full redraw counter
    if (canvas->forced_redraw_limit != -1) {
        canvas->forced_redraw_count = 0;
    }

    return TRUE;
}

int SPCanvasImpl::do_update(SPCanvas *canvas)
{
    if (!canvas->root) { // canvas may have already be destroyed by closing desktop during interrupted display!
        return TRUE;
    }

    if (canvas->drawing_disabled) {
        return TRUE;
    }

    // Cause the update if necessary
    if (canvas->need_update) {
        sp_canvas_item_invoke_update(canvas->root, Geom::identity(), 0);
        canvas->need_update = FALSE;
    }

    // Paint if able to
    if (gtk_widget_is_drawable( GTK_WIDGET(canvas) )) {
        return paint(canvas);
    }

    // Pick new current item
    while (canvas->need_repick) {
        canvas->need_repick = FALSE;
        pickCurrentItem(canvas, &canvas->pick_event);
    }

    return TRUE;
}

gint SPCanvasImpl::idle_handler(gpointer data)
{
    GDK_THREADS_ENTER ();

    SPCanvas *canvas = SP_CANVAS (data);

    int const ret = do_update (canvas);

    if (ret) {
        // Reset idle id
        canvas->idle_id = 0;
    }

    GDK_THREADS_LEAVE ();

    return !ret;
}

void SPCanvasImpl::add_idle(SPCanvas *canvas)
{
    if (canvas->idle_id == 0) {
        canvas->idle_id = g_idle_add_full(UPDATE_PRIORITY, idle_handler, canvas, NULL);
    }
}

SPCanvasGroup *SPCanvas::getRoot()
{
    return SP_CANVAS_GROUP(root);
}

void SPCanvas::scrollTo(double cx, double cy, unsigned int clear, bool is_scrolling)
{
    GtkAllocation allocation;

    int ix = (int) round(cx); // ix and iy are the new canvas coordinates (integer screen pixels)
    int iy = (int) round(cy); // cx might be negative, so (int)(cx + 0.5) will not do!
    int dx = ix - x0; // dx and dy specify the displacement (scroll) of the
    int dy = iy - y0; // canvas w.r.t its previous position

    Geom::IntRect old_area = getViewboxIntegers();
    Geom::IntRect new_area = old_area + Geom::IntPoint(dx, dy);
    
    dx0 = cx; // here the 'd' stands for double, not delta!
    dy0 = cy;
    x0 = ix;
    y0 = iy;

    gtk_widget_get_allocation(&widget, &allocation);

    SPCanvasImpl::sp_canvas_resize_tiles(this, x0, y0, x0 + allocation.width, y0 + allocation.height);
    if (SP_CANVAS_ITEM_GET_CLASS(root)->viewbox_changed) {
        SP_CANVAS_ITEM_GET_CLASS(root)->viewbox_changed(root, new_area);
    }

    if (!clear) {
        // scrolling without zoom; redraw only the newly exposed areas
        if ((dx != 0) || (dy != 0)) {
            this->is_scrolling = is_scrolling;
            if (gtk_widget_get_realized(GTK_WIDGET(this))) {
                gdk_window_scroll(getWindow(this), -dx, -dy);
            }
        }
    } else {
        // scrolling as part of zoom; do nothing here - the next do_update will perform full redraw
    }
}

void SPCanvas::updateNow()
{
    if (need_update || need_redraw) {
        SPCanvasImpl::do_update(this);
    }
}

void SPCanvasImpl::requestCanvasUpdate(SPCanvas *canvas)
{
    canvas->need_update = TRUE;
    add_idle(canvas);
}

void SPCanvas::requestRedraw(int x0, int y0, int x1, int y1)
{
    GtkAllocation allocation;

    if (!gtk_widget_is_drawable( GTK_WIDGET(this) )) {
        return;
    }
    if ((x0 >= x1) || (y0 >= y1)) {
        return;
    }

    Geom::IntRect bbox(x0, y0, x1, y1);
    gtk_widget_get_allocation(GTK_WIDGET(this), &allocation);

    Geom::IntRect canvas_rect = Geom::IntRect::from_xywh(this->x0, this->y0,
                                                         allocation.width, allocation.height);
    
    Geom::OptIntRect clip = bbox & canvas_rect;
    if (clip) {
        SPCanvasImpl::sp_canvas_dirty_rect(this, *clip);
        SPCanvasImpl::add_idle(this);
    }
}

/**
 * Sets world coordinates from win and canvas.
 */
void sp_canvas_window_to_world(SPCanvas const *canvas, double winx, double winy, double *worldx, double *worldy)
{
    g_return_if_fail (canvas != NULL);
    g_return_if_fail (SP_IS_CANVAS (canvas));

    if (worldx) *worldx = canvas->x0 + winx;
    if (worldy) *worldy = canvas->y0 + winy;
}

/**
 * Sets win coordinates from world and canvas.
 */
void sp_canvas_world_to_window(SPCanvas const *canvas, double worldx, double worldy, double *winx, double *winy)
{
    g_return_if_fail (canvas != NULL);
    g_return_if_fail (SP_IS_CANVAS (canvas));

    if (winx) *winx = worldx - canvas->x0;
    if (winy) *winy = worldy - canvas->y0;
}

/**
 * Converts point from win to world coordinates.
 */
Geom::Point sp_canvas_window_to_world(SPCanvas const *canvas, Geom::Point const win)
{
    g_assert (canvas != NULL);
    g_assert (SP_IS_CANVAS (canvas));

    return Geom::Point(canvas->x0 + win[0], canvas->y0 + win[1]);
}

/**
 * Converts point from world to win coordinates.
 */
Geom::Point sp_canvas_world_to_window(SPCanvas const *canvas, Geom::Point const world)
{
    g_assert (canvas != NULL);
    g_assert (SP_IS_CANVAS (canvas));

    return Geom::Point(world[0] - canvas->x0, world[1] - canvas->y0);
}

/**
 * Returns true if point given in world coordinates is inside window.
 */
bool sp_canvas_world_pt_inside_window(SPCanvas const *canvas, Geom::Point const &world)
{
    GtkAllocation allocation;
    
    g_assert( canvas != NULL );
    g_assert(SP_IS_CANVAS(canvas));

    GtkWidget *w = GTK_WIDGET(canvas);
    gtk_widget_get_allocation (w, &allocation);

    return ( ( canvas->x0 <= world[Geom::X] )  &&
             ( canvas->y0 <= world[Geom::Y] )  &&
             ( world[Geom::X] < canvas->x0 + allocation.width )  &&
             ( world[Geom::Y] < canvas->y0 + allocation.height ) );
}

/**
 * Return canvas window coordinates as Geom::Rect.
 */
Geom::Rect SPCanvas::getViewbox() const
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (GTK_WIDGET (this), &allocation);
    return Geom::Rect(Geom::Point(dx0, dy0),
                      Geom::Point(dx0 + allocation.width, dy0 + allocation.height));
}

/**
 * Return canvas window coordinates as integer rectangle.
 */
Geom::IntRect SPCanvas::getViewboxIntegers() const
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (GTK_WIDGET(this), &allocation);
    Geom::IntRect ret;
    ret.setMin(Geom::IntPoint(x0, y0));
    ret.setMax(Geom::IntPoint(x0 + allocation.width, y0 + allocation.height));
    return ret;
}

inline int sp_canvas_tile_floor(int x)
{
    return (x & (~(TILE_SIZE - 1))) / TILE_SIZE;
}

inline int sp_canvas_tile_ceil(int x)
{
    return ((x + (TILE_SIZE - 1)) & (~(TILE_SIZE - 1))) / TILE_SIZE;
}

void SPCanvasImpl::sp_canvas_resize_tiles(SPCanvas* canvas, int nl, int nt, int nr, int nb)
{
    if ( nl >= nr || nt >= nb ) {
        if ( canvas->tiles ) g_free(canvas->tiles);
        canvas->tLeft=canvas->tTop=canvas->tRight=canvas->tBottom=0;
        canvas->tileH=canvas->tileV=0;
        canvas->tiles=NULL;
        return;
    }
    int tl=sp_canvas_tile_floor(nl);
    int tt=sp_canvas_tile_floor(nt);
    int tr=sp_canvas_tile_ceil(nr);
    int tb=sp_canvas_tile_ceil(nb);

    int nh = tr-tl, nv = tb-tt;
    uint8_t* ntiles = (uint8_t*)g_malloc(nh*nv*sizeof(uint8_t));
    for (int i=tl; i<tr; i++) {
        for (int j=tt; j<tb; j++) {
            int ind = (i-tl) + (j-tt)*nh;
            if ( i >= canvas->tLeft && i < canvas->tRight && j >= canvas->tTop && j < canvas->tBottom ) {
                ntiles[ind]=canvas->tiles[(i-canvas->tLeft)+(j-canvas->tTop)*canvas->tileH]; // copy from the old tile
            } else {
                ntiles[ind]=0; // newly exposed areas get 0
            }
        }
    }
    if ( canvas->tiles ) g_free(canvas->tiles);
    canvas->tiles=ntiles;
    canvas->tLeft=tl;
    canvas->tTop=tt;
    canvas->tRight=tr;
    canvas->tBottom=tb;
    canvas->tileH=nh;
    canvas->tileV=nv;
}

void SPCanvasImpl::sp_canvas_dirty_rect(SPCanvas* canvas, Geom::IntRect const &area) {
    canvas->need_redraw = TRUE;

    sp_canvas_mark_rect(canvas, area, 1);
}

void SPCanvasImpl::sp_canvas_mark_rect(SPCanvas* canvas, Geom::IntRect const &area, uint8_t val)
{
    int tl=sp_canvas_tile_floor(area.left());
    int tt=sp_canvas_tile_floor(area.top());
    int tr=sp_canvas_tile_ceil(area.right());
    int tb=sp_canvas_tile_ceil(area.bottom());
    if ( tl >= canvas->tRight || tr <= canvas->tLeft || tt >= canvas->tBottom || tb <= canvas->tTop ) return;
    if ( tl < canvas->tLeft ) tl=canvas->tLeft;
    if ( tr > canvas->tRight ) tr=canvas->tRight;
    if ( tt < canvas->tTop ) tt=canvas->tTop;
    if ( tb > canvas->tBottom ) tb=canvas->tBottom;

    for (int i=tl; i<tr; i++) {
        for (int j=tt; j<tb; j++) {
            canvas->tiles[(i-canvas->tLeft)+(j-canvas->tTop)*canvas->tileH] = val;
        }
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
