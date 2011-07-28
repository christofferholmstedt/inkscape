/*
 * RGBA display list system for inkscape
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtk/gtk.h>

#include "display/display-forward.h"
#include "display/sp-canvas-util.h"
#include "helper/sp-marshal.h"
#include "display/nr-arena.h"
#include "display/nr-arena-group.h"
#include "display/canvas-arena.h"
#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing-surface.h"

enum {
    ARENA_EVENT,
    LAST_SIGNAL
};

static void sp_canvas_arena_class_init(SPCanvasArenaClass *klass);
static void sp_canvas_arena_init(SPCanvasArena *group);
static void sp_canvas_arena_destroy(GtkObject *object);

static void sp_canvas_arena_update (SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);
static void sp_canvas_arena_render (SPCanvasItem *item, SPCanvasBuf *buf);
static double sp_canvas_arena_point (SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);
static void sp_canvas_arena_viewbox_changed (SPCanvasItem *item, Geom::IntRect const &new_area);
static gint sp_canvas_arena_event (SPCanvasItem *item, GdkEvent *event);

static gint sp_canvas_arena_send_event (SPCanvasArena *arena, GdkEvent *event);

static void sp_canvas_arena_request_update (NRArena *arena, NRArenaItem *item, void *data);
static void sp_canvas_arena_request_render (NRArena *arena, NRRectL *area, void *data);

NRArenaEventVector carenaev = {
    {NULL},
    sp_canvas_arena_request_update,
    sp_canvas_arena_request_render
};

static SPCanvasItemClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GType
sp_canvas_arena_get_type (void)
{
    static GType type = 0;
    if (!type) {
	GTypeInfo info = {
            sizeof (SPCanvasArenaClass),
	    NULL, NULL,
            (GClassInitFunc) sp_canvas_arena_class_init,
	    NULL, NULL,
            sizeof (SPCanvasArena),
	    0,
            (GInstanceInitFunc) sp_canvas_arena_init,
	    NULL
	};
        type = g_type_register_static (SP_TYPE_CANVAS_ITEM, "SPCanvasArena", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_canvas_arena_class_init (SPCanvasArenaClass *klass)
{
    GtkObjectClass *object_class;
    SPCanvasItemClass *item_class;

    object_class = (GtkObjectClass *) klass;
    item_class = (SPCanvasItemClass *) klass;

    parent_class = (SPCanvasItemClass*)g_type_class_peek_parent (klass);

    signals[ARENA_EVENT] = g_signal_new ("arena_event",
                                           G_TYPE_FROM_CLASS(object_class),
                                           G_SIGNAL_RUN_LAST,
                                           ((glong)((guint8*)&(klass->arena_event) - (guint8*)klass)),
					   NULL, NULL,
                                           sp_marshal_INT__POINTER_POINTER,
                                           GTK_TYPE_INT, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

    object_class->destroy = sp_canvas_arena_destroy;

    item_class->update = sp_canvas_arena_update;
    item_class->render = sp_canvas_arena_render;
    item_class->point = sp_canvas_arena_point;
    item_class->event = sp_canvas_arena_event;
    item_class->viewbox_changed = sp_canvas_arena_viewbox_changed;
}

static void
sp_canvas_arena_init (SPCanvasArena *arena)
{
    arena->sticky = FALSE;

    arena->arena = NRArena::create();
    arena->arena->canvasarena = arena;
    arena->root = NRArenaGroup::create(arena->arena);
    nr_arena_group_set_transparent (NR_ARENA_GROUP (arena->root), TRUE);
    nr_arena_item_set_cache(arena->root, true);

    arena->active = NULL;

    nr_active_object_add_listener ((NRActiveObject *) arena->arena, (NRObjectEventVector *) &carenaev, sizeof (carenaev), arena);
}

static void
sp_canvas_arena_destroy (GtkObject *object)
{
    SPCanvasArena *arena = SP_CANVAS_ARENA (object);

    if (arena->active) {
        nr_object_unref ((NRObject *) arena->active);
        arena->active = NULL;
    }

    if (arena->root) {
        nr_arena_item_unref (arena->root);
        arena->root = NULL;
    }

    if (arena->arena) {
        nr_active_object_remove_listener_by_data ((NRActiveObject *) arena->arena, arena);

        nr_object_unref ((NRObject *) arena->arena);
        arena->arena = NULL;
    }

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_arena_update (SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags)
{
    SPCanvasArena *arena = SP_CANVAS_ARENA (item);

    if (((SPCanvasItemClass *) parent_class)->update)
        (* ((SPCanvasItemClass *) parent_class)->update) (item, affine, flags);

    arena->gc.transform = affine;

    guint reset;
    reset = (flags & SP_CANVAS_UPDATE_AFFINE)? NR_ARENA_ITEM_STATE_ALL : NR_ARENA_ITEM_STATE_NONE;

    nr_arena_item_invoke_update (arena->root, Geom::IntRect::infinite(), &arena->gc, NR_ARENA_ITEM_STATE_ALL, reset);

    Geom::OptIntRect b = arena->root->bbox;
    if (b) {
        item->x1 = b->left() - 1;
        item->y1 = b->top() - 1;
        item->x2 = b->right() + 1;
        item->y2 = b->bottom() + 1;
    }

    if (arena->cursor) {
        /* Mess with enter/leave notifiers */
        NRArenaItem *new_arena = nr_arena_item_invoke_pick (arena->root, arena->c, arena->arena->delta, arena->sticky);
        if (new_arena != arena->active) {
            GdkEventCrossing ec;
            ec.window = GTK_WIDGET (item->canvas)->window;
            ec.send_event = TRUE;
            ec.subwindow = ec.window;
            ec.time = GDK_CURRENT_TIME;
            ec.x = arena->c[Geom::X];
            ec.y = arena->c[Geom::Y];
            /* fixme: */
            if (arena->active) {
                ec.type = GDK_LEAVE_NOTIFY;
                sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
            }
            /* fixme: This is not optimal - better track ::destroy (Lauris) */
            if (arena->active) nr_object_unref ((NRObject *) arena->active);
            arena->active = new_arena;
            if (arena->active) nr_object_ref ((NRObject *) arena->active);
            if (arena->active) {
                ec.type = GDK_ENTER_NOTIFY;
                sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
            }
        }
    }
}

static void
sp_canvas_arena_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
    // todo: handle NR_ARENA_ITEM_RENDER_NO_CACHE
    SPCanvasArena *arena = SP_CANVAS_ARENA (item);

    Geom::OptIntRect r = buf->rect;
    if (!r || r->hasZeroArea()) return;

    Inkscape::DrawingContext ct(buf->ct, r->min());

    nr_arena_item_invoke_update (arena->root, Geom::IntRect::infinite(), &arena->gc,
                                 NR_ARENA_ITEM_STATE_BBOX | NR_ARENA_ITEM_STATE_RENDER,
                                 NR_ARENA_ITEM_STATE_NONE);
    nr_arena_item_invoke_render (ct, arena->root, *r, 0);
}

static double
sp_canvas_arena_point (SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    SPCanvasArena *arena = SP_CANVAS_ARENA (item);

    nr_arena_item_invoke_update (arena->root, Geom::IntRect::infinite(), &arena->gc,
                                 NR_ARENA_ITEM_STATE_BBOX | NR_ARENA_ITEM_STATE_PICK,
                                 NR_ARENA_ITEM_STATE_NONE);

    NRArenaItem *picked = nr_arena_item_invoke_pick (arena->root, p, arena->arena->delta, arena->sticky);

    arena->picked = picked;

    if (picked) {
        *actual_item = item;
        return 0.0;
    }

    return 1e18;
}

static void
sp_canvas_arena_viewbox_changed (SPCanvasItem *item, Geom::IntRect const &new_area)
{
    SPCanvasArena *arena = SP_CANVAS_ARENA(item);
    // make the cache limit larger than screen to facilitate smooth scrolling
    Geom::IntRect expanded = new_area;
    Geom::IntPoint expansion(new_area.width()/2, new_area.height()/2);
    expanded.expandBy(expansion);
    nr_arena_set_cache_limit(arena->arena, expanded);
}

static gint
sp_canvas_arena_event (SPCanvasItem *item, GdkEvent *event)
{
    NRArenaItem *new_arena;
    /* fixme: This sucks, we have to handle enter/leave notifiers */

    SPCanvasArena *arena = SP_CANVAS_ARENA (item);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_ENTER_NOTIFY:
            if (!arena->cursor) {
                if (arena->active) {
                    //g_warning ("Cursor entered to arena with already active item");
                    nr_object_unref ((NRObject *) arena->active);
                }
                arena->cursor = TRUE;

                /* TODO ... event -> arena transform? */
                arena->c = Geom::Point(event->crossing.x, event->crossing.y);

                /* fixme: Not sure abut this, but seems the right thing (Lauris) */
                nr_arena_item_invoke_update (arena->root, Geom::IntRect::infinite(), &arena->gc, NR_ARENA_ITEM_STATE_PICK, NR_ARENA_ITEM_STATE_NONE);
                arena->active = nr_arena_item_invoke_pick (arena->root, arena->c, arena->arena->delta, arena->sticky);
                if (arena->active) nr_object_ref ((NRObject *) arena->active);
                ret = sp_canvas_arena_send_event (arena, event);
            }
            break;

        case GDK_LEAVE_NOTIFY:
            if (arena->cursor) {
                ret = sp_canvas_arena_send_event (arena, event);
                if (arena->active) nr_object_unref ((NRObject *) arena->active);
                arena->active = NULL;
                arena->cursor = FALSE;
            }
            break;

        case GDK_MOTION_NOTIFY:
            /* TODO ... event -> arena transform? */
            arena->c = Geom::Point(event->motion.x, event->motion.y);

            /* fixme: Not sure abut this, but seems the right thing (Lauris) */
            nr_arena_item_invoke_update (arena->root, Geom::IntRect::infinite(), &arena->gc, NR_ARENA_ITEM_STATE_PICK, NR_ARENA_ITEM_STATE_NONE);
            new_arena = nr_arena_item_invoke_pick (arena->root, arena->c, arena->arena->delta, arena->sticky);
            if (new_arena != arena->active) {
                GdkEventCrossing ec;
                ec.window = event->motion.window;
                ec.send_event = event->motion.send_event;
                ec.subwindow = event->motion.window;
                ec.time = event->motion.time;
                ec.x = event->motion.x;
                ec.y = event->motion.y;
                /* fixme: */
                if (arena->active) {
                    ec.type = GDK_LEAVE_NOTIFY;
                    ret = sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
                }
                if (arena->active) nr_object_unref ((NRObject *) arena->active);
                arena->active = new_arena;
                if (arena->active) nr_object_ref ((NRObject *) arena->active);
                if (arena->active) {
                    ec.type = GDK_ENTER_NOTIFY;
                    ret = sp_canvas_arena_send_event (arena, (GdkEvent *) &ec);
                }
            }
            ret = sp_canvas_arena_send_event (arena, event);
            break;

        default:
            /* Just send event */
            ret = sp_canvas_arena_send_event (arena, event);
            break;
    }

    return ret;
}

static gint
sp_canvas_arena_send_event (SPCanvasArena *arena, GdkEvent *event)
{
    gint ret = FALSE;

    /* Send event to arena */
    g_signal_emit (G_OBJECT (arena), signals[ARENA_EVENT], 0, arena->active, event, &ret);

    return ret;
}

static void
sp_canvas_arena_request_update (NRArena */*arena*/, NRArenaItem */*item*/, void *data)
{
    sp_canvas_item_request_update (SP_CANVAS_ITEM (data));
}

static void
sp_canvas_arena_request_render (NRArena */*arena*/, NRRectL *area, void *data)
{
    if (!area) return;
    sp_canvas_request_redraw (SP_CANVAS_ITEM (data)->canvas, area->x0, area->y0, area->x1, area->y1);
}

void
sp_canvas_arena_set_pick_delta (SPCanvasArena *ca, gdouble delta)
{
    g_return_if_fail (ca != NULL);
    g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

    /* fixme: repick? */
    ca->delta = delta;
}

void
sp_canvas_arena_set_sticky (SPCanvasArena *ca, gboolean sticky)
{
    g_return_if_fail (ca != NULL);
    g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

    /* fixme: repick? */
    ca->sticky = sticky;
}

void
sp_canvas_arena_render_surface (SPCanvasArena *ca, cairo_surface_t *surface, NRRectL const &r)
{
    g_return_if_fail (ca != NULL);
    g_return_if_fail (SP_IS_CANVAS_ARENA (ca));

    Geom::OptIntRect area = r.upgrade_2geom();
    if (!area) return;
    Inkscape::DrawingContext ct(surface, area->min());
    nr_arena_item_invoke_render (ct, ca->root, *area, 0);
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
