#ifndef SEEN_SP_CANVAS_ITEM_H
#define SEEN_SP_CANVAS_ITEM_H

/**
 * @file
 * SPCanvasItem.
 */
/*
 * Authors:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@gimp.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2010 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <2geom/rect.h>

G_BEGIN_DECLS

struct SPCanvas;
struct SPCanvasBuf;
struct SPCanvasGroup;

typedef struct _SPCanvasItemClass SPCanvasItemClass;

#define SP_TYPE_CANVAS_ITEM (SPCanvasItem::getType())
#define SP_CANVAS_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_CANVAS_ITEM, SPCanvasItem))
#define SP_IS_CANVAS_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_CANVAS_ITEM))
#define SP_CANVAS_ITEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), SP_TYPE_CANVAS_ITEM, SPCanvasItemClass))

/**
 * An SPCanvasItem refers to a SPCanvas and to its parent item; it has
 * four coordinates, a bounding rectangle, and a transformation matrix.
 */
struct SPCanvasItem : public GtkObject {
    static GType getType();

    SPCanvas *canvas;
    SPCanvasItem *parent;

    double x1, y1, x2, y2;
    Geom::Rect bounds;
    Geom::Affine xform;
};

/**
 * The vtable of an SPCanvasItem.
 */
struct _SPCanvasItemClass : public GtkObjectClass {
    void (* update) (SPCanvasItem *item, Geom::Affine const &affine, unsigned int flags);

    void (* render) (SPCanvasItem *item, SPCanvasBuf *buf);
    double (* point) (SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);

    int (* event) (SPCanvasItem *item, GdkEvent *event);
    void (* viewbox_changed) (SPCanvasItem *item, Geom::IntRect const &new_area);
};

SPCanvasItem *sp_canvas_item_new(SPCanvasGroup *parent, GType type, const gchar *first_arg_name, ...);

G_END_DECLS


#define sp_canvas_item_set g_object_set

void sp_canvas_item_affine_absolute(SPCanvasItem *item, Geom::Affine const &aff);

void sp_canvas_item_raise(SPCanvasItem *item, int positions);
void sp_canvas_item_lower(SPCanvasItem *item, int positions);
bool sp_canvas_item_is_visible(SPCanvasItem *item);
void sp_canvas_item_show(SPCanvasItem *item);
void sp_canvas_item_hide(SPCanvasItem *item);
int sp_canvas_item_grab(SPCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);
void sp_canvas_item_ungrab(SPCanvasItem *item, guint32 etime);

Geom::Affine sp_canvas_item_i2w_affine(SPCanvasItem const *item);

void sp_canvas_item_request_update(SPCanvasItem *item);

/* get item z-order in parent group */

gint sp_canvas_item_order(SPCanvasItem * item);



#endif // SEEN_SP_CANVAS_ITEM_H

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