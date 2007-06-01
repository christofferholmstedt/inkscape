#define __SP_FLOOD_CONTEXT_C__

/*
* Flood fill drawing context
*
* Author:
*   Lauris Kaplinski <lauris@kaplinski.com>
*   bulia byak <buliabyak@users.sf.net>
*   John Bintz <jcoswell@coswellproductions.org>
*
* Copyright (C) 2006      Johan Engelen <johan@shouraizou.nl>
* Copyright (C) 2000-2005 authors
* Copyright (C) 2000-2001 Ximian, Inc.
*
* Released under GNU GPL, read the file 'COPYING' for more information
*/

#include "config.h"

#include <gdk/gdkkeysyms.h>
#include <queue>
#include <deque>

#include "macros.h"
#include "display/sp-canvas.h"
#include "document.h"
#include "sp-namedview.h"
#include "sp-object.h"
#include "sp-rect.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop.h"
#include "desktop-style.h"
#include "message-stack.h"
#include "message-context.h"
#include "pixmaps/cursor-paintbucket.xpm"
#include "flood-context.h"
#include "sp-metrics.h"
#include <glibmm/i18n.h>
#include "object-edit.h"
#include "xml/repr.h"
#include "xml/node-event-vector.h"
#include "prefs-utils.h"
#include "context-fns.h"
#include "rubberband.h"

#include "display/nr-arena-item.h"
#include "display/nr-arena.h"
#include "display/nr-arena-image.h"
#include "display/canvas-arena.h"
#include "libnr/nr-pixops.h"
#include "libnr/nr-matrix-translate-ops.h"
#include "libnr/nr-scale-ops.h"
#include "libnr/nr-scale-translate-ops.h"
#include "libnr/nr-translate-matrix-ops.h"
#include "libnr/nr-translate-scale-ops.h"
#include "libnr/nr-matrix-ops.h"
#include "sp-item.h"
#include "sp-root.h"
#include "sp-defs.h"
#include "sp-path.h"
#include "splivarot.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"
#include "libnr/n-art-bpath.h"
#include "svg/svg.h"
#include "color.h"

#include "trace/trace.h"
#include "trace/potrace/inkscape-potrace.h"

static void sp_flood_context_class_init(SPFloodContextClass *klass);
static void sp_flood_context_init(SPFloodContext *flood_context);
static void sp_flood_context_dispose(GObject *object);

static void sp_flood_context_setup(SPEventContext *ec);

static gint sp_flood_context_root_handler(SPEventContext *event_context, GdkEvent *event);
static gint sp_flood_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);

static void sp_flood_finish(SPFloodContext *rc);

static SPEventContextClass *parent_class;


GtkType sp_flood_context_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPFloodContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_flood_context_class_init,
            NULL, NULL,
            sizeof(SPFloodContext),
            4,
            (GInstanceInitFunc) sp_flood_context_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPFloodContext", &info, (GTypeFlags) 0);
    }
    return type;
}

static void sp_flood_context_class_init(SPFloodContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass *) g_type_class_peek_parent(klass);

    object_class->dispose = sp_flood_context_dispose;

    event_context_class->setup = sp_flood_context_setup;
    event_context_class->root_handler  = sp_flood_context_root_handler;
    event_context_class->item_handler  = sp_flood_context_item_handler;
}

static void sp_flood_context_init(SPFloodContext *flood_context)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(flood_context);

    event_context->cursor_shape = cursor_paintbucket_xpm;
    event_context->hot_x = 11;
    event_context->hot_y = 30;
    event_context->xp = 0;
    event_context->yp = 0;
    event_context->tolerance = 4;
    event_context->within_tolerance = false;
    event_context->item_to_select = NULL;

    event_context->shape_repr = NULL;
    event_context->shape_knot_holder = NULL;

    flood_context->item = NULL;

    new (&flood_context->sel_changed_connection) sigc::connection();
}

static void sp_flood_context_dispose(GObject *object)
{
    SPFloodContext *rc = SP_FLOOD_CONTEXT(object);
    SPEventContext *ec = SP_EVENT_CONTEXT(object);

    rc->sel_changed_connection.disconnect();
    rc->sel_changed_connection.~connection();

    /* fixme: This is necessary because we do not grab */
    if (rc->item) {
        sp_flood_finish(rc);
    }

    if (ec->shape_repr) { // remove old listener
        sp_repr_remove_listener_by_data(ec->shape_repr, ec);
        Inkscape::GC::release(ec->shape_repr);
        ec->shape_repr = 0;
    }

    if (rc->_message_context) {
        delete rc->_message_context;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static Inkscape::XML::NodeEventVector ec_shape_repr_events = {
    NULL, /* child_added */
    NULL, /* child_removed */
    ec_shape_event_attr_changed,
    NULL, /* content_changed */
    NULL  /* order_changed */
};

/**
\brief  Callback that processes the "changed" signal on the selection;
destroys old and creates new knotholder
*/
void sp_flood_context_selection_changed(Inkscape::Selection *selection, gpointer data)
{
    SPFloodContext *rc = SP_FLOOD_CONTEXT(data);
    SPEventContext *ec = SP_EVENT_CONTEXT(rc);

    if (ec->shape_repr) { // remove old listener
        sp_repr_remove_listener_by_data(ec->shape_repr, ec);
        Inkscape::GC::release(ec->shape_repr);
        ec->shape_repr = 0;
    }

    SPItem *item = selection->singleItem();
    if (item) {
        Inkscape::XML::Node *shape_repr = SP_OBJECT_REPR(item);
        if (shape_repr) {
            ec->shape_repr = shape_repr;
            Inkscape::GC::anchor(shape_repr);
            sp_repr_add_listener(shape_repr, &ec_shape_repr_events, ec);
        }
    }
}

static void sp_flood_context_setup(SPEventContext *ec)
{
    SPFloodContext *rc = SP_FLOOD_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup) {
        ((SPEventContextClass *) parent_class)->setup(ec);
    }

    SPItem *item = sp_desktop_selection(ec->desktop)->singleItem();
    if (item) {
        Inkscape::XML::Node *shape_repr = SP_OBJECT_REPR(item);
        if (shape_repr) {
            ec->shape_repr = shape_repr;
            Inkscape::GC::anchor(shape_repr);
            sp_repr_add_listener(shape_repr, &ec_shape_repr_events, ec);
        }
    }

    rc->sel_changed_connection.disconnect();
    rc->sel_changed_connection = sp_desktop_selection(ec->desktop)->connectChanged(
        sigc::bind(sigc::ptr_fun(&sp_flood_context_selection_changed), (gpointer)rc)
    );

    rc->_message_context = new Inkscape::MessageContext((ec->desktop)->messageStack());
}

static void
merge_pixel_with_background (unsigned char *orig, unsigned char *bg,
           unsigned char *base)
{
    for (int i = 0; i < 3; i++) {
        base[i] = (255 * (255 - bg[3])) / 255 + (bg[i] * bg[3]) / 255;
        base[i] = (base[i] * (255 - orig[3])) / 255 + (orig[i] * orig[3]) / 255;
    }
    base[3] = 255;
}

inline unsigned char * get_pixel(guchar *px, int x, int y, int width) {
    return px + (x + y * width) * 4;
}

GList * flood_channels_dropdown_items_list() {
    GList *glist = NULL;

    glist = g_list_append (glist, _("Visible Colors"));
    glist = g_list_append (glist, _("Red"));
    glist = g_list_append (glist, _("Green"));
    glist = g_list_append (glist, _("Blue"));
    glist = g_list_append (glist, _("Hue"));
    glist = g_list_append (glist, _("Saturation"));
    glist = g_list_append (glist, _("Lightness"));
    glist = g_list_append (glist, _("Alpha"));

    return glist;
}

GList * flood_autogap_dropdown_items_list() {
    GList *glist = NULL;

    glist = g_list_append (glist, _("None"));
    glist = g_list_append (glist, _("Small"));
    glist = g_list_append (glist, _("Medium"));
    glist = g_list_append (glist, _("Large"));

    return glist;
}

static bool compare_pixels(unsigned char *check, unsigned char *orig, unsigned char *dtc, int threshold, PaintBucketChannels method) {
    int diff = 0;
    float hsl_check[3], hsl_orig[3];
    
    if ((method == FLOOD_CHANNELS_H) ||
        (method == FLOOD_CHANNELS_S) ||
        (method == FLOOD_CHANNELS_L)) {
        sp_color_rgb_to_hsl_floatv(hsl_check, check[0] / 255.0, check[1] / 255.0, check[2] / 255.0);
        sp_color_rgb_to_hsl_floatv(hsl_orig, orig[0] / 255.0, orig[1] / 255.0, orig[2] / 255.0);
    }
    
    switch (method) {
        case FLOOD_CHANNELS_ALPHA:
            return ((int)abs(check[3] - orig[3]) <= threshold);
        case FLOOD_CHANNELS_R:
            return ((int)abs(check[0] - orig[0]) <= threshold);
        case FLOOD_CHANNELS_G:
            return ((int)abs(check[1] - orig[1]) <= threshold);
        case FLOOD_CHANNELS_B:
            return ((int)abs(check[2] - orig[2]) <= threshold);
        case FLOOD_CHANNELS_RGB:
            unsigned char merged_orig[4];
            unsigned char merged_check[4];
            
            merge_pixel_with_background(orig, dtc, merged_orig);
            merge_pixel_with_background(check, dtc, merged_check);
            
            for (int i = 0; i < 3; i++) {
              diff += (int)abs(merged_check[i] - merged_orig[i]);
            }
            return ((diff / 3) <= ((threshold * 3) / 4));
        
        case FLOOD_CHANNELS_H:
            return ((int)(fabs(hsl_check[0] - hsl_orig[0]) * 100.0) <= threshold);
        case FLOOD_CHANNELS_S:
            return ((int)(fabs(hsl_check[1] - hsl_orig[1]) * 100.0) <= threshold);
        case FLOOD_CHANNELS_L:
            return ((int)(fabs(hsl_check[2] - hsl_orig[2]) * 100.0) <= threshold);
    }
    
    return false;
}

static bool is_pixel_checked(unsigned char *t) { return t[0] == 1; }
static bool is_pixel_queued(unsigned char *t) { return t[1] == 1; }
static bool is_pixel_paintability_checked(unsigned char *t) { return t[2] != 0; }
static bool is_pixel_paintable(unsigned char *t) { return t[2] == 1; }
static bool is_pixel_colored(unsigned char *t) { return t[3] == 255; }

static void mark_pixel_checked(unsigned char *t) { t[0] = 1; }
static void mark_pixel_queued(unsigned char *t) { t[1] = 1; }
static void mark_pixel_paintable(unsigned char *t) { t[2] = 1; }
static void mark_pixel_not_paintable(unsigned char *t) { t[2] = 2; }
static void mark_pixel_colored(unsigned char *t) { t[3] = 255; }

static void clear_pixel_paintability(unsigned char *t) { t[2] = 0; }

struct bitmap_coords_info {
    bool is_left;
    int x;
    int y;
    int y_limit;
    int width;
    int height;
    int threshold;
    int radius;
    PaintBucketChannels method;
    unsigned char *dtc;
    NR::Rect bbox;
    NR::Rect screen;
    unsigned int max_queue_size;
    unsigned int max_vertical_check;
};

static bool check_if_pixel_is_paintable(guchar *px, guchar *trace_px, int x, int y, unsigned char *orig_color, bitmap_coords_info bci) {
    unsigned char *trace_t = get_pixel(trace_px, x, y, bci.width);
    if (is_pixel_paintability_checked(trace_t)) {
        return is_pixel_paintable(trace_t);
    } else {
        unsigned char *t = get_pixel(px, x, y, bci.width);
        if (compare_pixels(t, orig_color, bci.dtc, bci.threshold, bci.method)) {
            mark_pixel_paintable(trace_t);
            return true;
        } else {
            mark_pixel_not_paintable(trace_t);
            return false;
        }
    }
}

static bool try_add_to_queue(std::deque<NR::Point> *fill_queue, guchar *px, guchar *trace_px, unsigned char *orig, int x, int y, int width, bitmap_coords_info bci, unsigned int max_queue_size) {
    unsigned char *trace_t = get_pixel(trace_px, x, y, width);
    if (!(is_pixel_checked(trace_t) || is_pixel_queued(trace_t))) {
        if (check_if_pixel_is_paintable(px, trace_px, x, y, orig, bci)) {
            if ((fill_queue->size() < max_queue_size)) {
                fill_queue->push_back(NR::Point(x, y));
                mark_pixel_queued(trace_t);
            }
        }
    }
    return true;
}

static void do_trace(GdkPixbuf *px, SPDesktop *desktop, NR::Matrix transform, bool union_with_selection) {
    SPDocument *document = sp_desktop_document(desktop);
    
    Inkscape::Trace::Potrace::PotraceTracingEngine pte;
        
    pte.setTraceType(Inkscape::Trace::Potrace::TRACE_BRIGHTNESS);
    pte.setInvert(false);

    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(px, true);
    
    std::vector<Inkscape::Trace::TracingEngineResult> results = pte.trace(pixbuf);
    
    Inkscape::XML::Node *layer_repr = SP_GROUP(desktop->currentLayer())->repr;
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(desktop->doc());

    long totalNodeCount = 0L;

    double offset = prefs_get_double_attribute("tools.paintbucket", "offset", 0.0);

    for (unsigned int i=0 ; i<results.size() ; i++) {
        Inkscape::Trace::TracingEngineResult result = results[i];
        totalNodeCount += result.getNodeCount();

        Inkscape::XML::Node *pathRepr = xml_doc->createElement("svg:path");
        /* Set style */
        sp_desktop_apply_style_tool (desktop, pathRepr, "tools.paintbucket", false);

        NArtBpath *bpath = sp_svg_read_path(result.getPathData().c_str());
        Path *path = bpath_to_Path(bpath);
        g_free(bpath);

        if (offset != 0) {
        
            Shape *path_shape = new Shape();
        
            path->ConvertWithBackData(0.03);
            path->Fill(path_shape, 0);
            delete path;
        
            Shape *expanded_path_shape = new Shape();
        
            expanded_path_shape->ConvertToShape(path_shape, fill_nonZero);
            path_shape->MakeOffset(expanded_path_shape, offset * desktop->current_zoom(), join_round, 4);
            expanded_path_shape->ConvertToShape(path_shape, fill_positive);

            Path *expanded_path = new Path();
        
            expanded_path->Reset();
            expanded_path_shape->ConvertToForme(expanded_path);
            expanded_path->ConvertEvenLines(1.0);
            expanded_path->Simplify(1.0);
        
            delete path_shape;
            delete expanded_path_shape;
        
            gchar *str = expanded_path->svg_dump_path();
            if (str && *str) {
                pathRepr->setAttribute("d", str);
                g_free(str);
            } else {
                desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("<b>Too much inset</b>, the result is empty."));
                Inkscape::GC::release(pathRepr);
                g_free(str);
                return;
            }

            delete expanded_path;

        } else {
            gchar *str = path->svg_dump_path();
            delete path;
            pathRepr->setAttribute("d", str);
            g_free(str);
        }

        layer_repr->addChild(pathRepr, NULL);

        SPObject *reprobj = document->getObjectByRepr(pathRepr);
        if (reprobj) {
            sp_item_write_transform(SP_ITEM(reprobj), pathRepr, transform, NULL);
            
            // premultiply the item transform by the accumulated parent transform in the paste layer
            NR::Matrix local = sp_item_i2doc_affine(SP_GROUP(desktop->currentLayer()));
            if (!local.test_identity()) {
                gchar const *t_str = pathRepr->attribute("transform");
                NR::Matrix item_t (NR::identity());
                if (t_str)
                    sp_svg_transform_read(t_str, &item_t);
                item_t *= local.inverse();
                // (we're dealing with unattached repr, so we write to its attr instead of using sp_item_set_transform)
                gchar *affinestr=sp_svg_transform_write(item_t);
                pathRepr->setAttribute("transform", affinestr);
                g_free(affinestr);
            }

            Inkscape::Selection *selection = sp_desktop_selection(desktop);

            pathRepr->setPosition(-1);

            if (union_with_selection) {
                desktop->messageStack()->flashF(Inkscape::WARNING_MESSAGE, _("Area filled, path with <b>%d</b> nodes created and unioned with selection."), sp_nodes_in_path(SP_PATH(reprobj)));
                selection->add(reprobj);
                sp_selected_path_union_skip_undo();
            } else {
                desktop->messageStack()->flashF(Inkscape::WARNING_MESSAGE, _("Area filled, path with <b>%d</b> nodes created."), sp_nodes_in_path(SP_PATH(reprobj)));
                selection->set(reprobj);
            }

        }

        Inkscape::GC::release(pathRepr);

    }
}

enum ScanlineCheckResult {
    SCANLINE_CHECK_OK,
    SCANLINE_CHECK_ABORTED,
    SCANLINE_CHECK_BOUNDARY
};

static bool coords_in_range(int x, int y, bitmap_coords_info bci) {
    return (x >= 0) 
          && (x < bci.width)
          && (y >= 0)
          && (y < bci.height);
}

#define PAINT_DIRECTION_LEFT 1
#define PAINT_DIRECTION_RIGHT 2
#define PAINT_DIRECTION_UP 4
#define PAINT_DIRECTION_DOWN 8
#define PAINT_DIRECTION_ALL 15

static unsigned int paint_pixel(guchar *px, guchar *trace_px, unsigned char *orig_color, bitmap_coords_info bci) {
    unsigned char *trace_t;
  
    if (bci.radius == 0) {
        trace_t = get_pixel(trace_px, bci.x, bci.y, bci.width);
        mark_pixel_colored(trace_t); 
        return PAINT_DIRECTION_ALL;
    } else {
        bool can_paint_up = true;
        bool can_paint_down = true;
        bool can_paint_left = true;
        bool can_paint_right = true;
      
        for (int y = -bci.radius; y <= bci.radius; y++) {
            int ty = bci.y + y;
            for (int x = -bci.radius; x <= bci.radius; x++) {
                int tx = bci.x + x;
                
                if (coords_in_range(tx, ty, bci)) {
                    trace_t = get_pixel(trace_px, tx, ty, bci.width);
                    if (!is_pixel_colored(trace_t)) {
                        if (check_if_pixel_is_paintable(px, trace_px, tx, ty, orig_color, bci)) {
                            mark_pixel_colored(trace_t); 
                        } else {
                            if (x < 0) { can_paint_left = false; }
                            if (x > 0) { can_paint_right = false; }
                            if (y < 0) { can_paint_up = false; }
                            if (y > 0) { can_paint_down = false; }
                        }
                    }
                }
            }
        }
    
        unsigned int paint_directions = 0;
        if (can_paint_left) { paint_directions += PAINT_DIRECTION_LEFT; }
        if (can_paint_right) { paint_directions += PAINT_DIRECTION_RIGHT; }
        if (can_paint_up) { paint_directions += PAINT_DIRECTION_UP; }
        if (can_paint_down) { paint_directions += PAINT_DIRECTION_DOWN; }
        
        return paint_directions;
    }
}

static unsigned int paint_leading_edge(guchar *px, guchar *trace_px, unsigned char *orig_color, bitmap_coords_info bci, unsigned int direction) {
    unsigned char *trace_t;
  
    if (bci.radius > 0) {
        bool can_paint_up = true;
        bool can_paint_down = true;
        bool can_paint_left = true;
        bool can_paint_right = true;
      
        int tx = bci.x;
      
        switch (direction) {
          case PAINT_DIRECTION_LEFT:
            tx = bci.x - bci.radius;
            break;
          case PAINT_DIRECTION_RIGHT:
            tx = bci.x + bci.radius;
            break;
        }
      
        for (int y = -bci.radius; y <= bci.radius; y++) {
            int ty = bci.y + y;
            if (coords_in_range(tx, ty, bci)) {
                trace_t = get_pixel(trace_px, tx, ty, bci.width);
                if (!is_pixel_colored(trace_t)) {
                    if (check_if_pixel_is_paintable(px, trace_px, tx, ty, orig_color, bci)) {
                        mark_pixel_colored(trace_t); 
                    } else {
                        if (direction == PAINT_DIRECTION_LEFT) { can_paint_left = false; }
                        if (direction == PAINT_DIRECTION_RIGHT) { can_paint_right = false; }
                        if (y < 0) { can_paint_up = false; }
                        if (y > 0) { can_paint_down = false; }
                    }
                }
            }
        }
    
        unsigned int paint_directions = 0;
        if (can_paint_left) { paint_directions += PAINT_DIRECTION_LEFT; }
        if (can_paint_right) { paint_directions += PAINT_DIRECTION_RIGHT; }
        if (can_paint_up) { paint_directions += PAINT_DIRECTION_UP; }
        if (can_paint_down) { paint_directions += PAINT_DIRECTION_DOWN; }
        
        return paint_directions;
    } else {
        return 0;
    }
}

static ScanlineCheckResult perform_bitmap_scanline_check(std::deque<NR::Point> *fill_queue, guchar *px, guchar *trace_px, unsigned char *orig_color, bitmap_coords_info bci) {
    bool aborted = false;
    bool reached_screen_boundary = false;
    bool ok;
  
    bool keep_tracing;
    bool initial_paint = true;
    
    int vertical_check_count = 0;
    
    do {
        ok = false;
        if (bci.is_left) {
            keep_tracing = (bci.x >= 0);
        } else {
            keep_tracing = (bci.x < bci.width);
        }
        
        if (keep_tracing) {
            if (check_if_pixel_is_paintable(px, trace_px, bci.x, bci.y, orig_color, bci)) {
                unsigned int paint_directions;
                if ((bci.radius == 0) || ((bci.radius > 0) && initial_paint)) {
                    paint_directions = paint_pixel(px, trace_px, orig_color, bci);
                } else {
                    if (bci.is_left) {
                        paint_directions = paint_leading_edge(px, trace_px, orig_color, bci, PAINT_DIRECTION_LEFT);
                    } else {
                        paint_directions = paint_leading_edge(px, trace_px, orig_color, bci, PAINT_DIRECTION_RIGHT);
                    }
                }
                initial_paint = false;
                
                if (vertical_check_count == 0) {
                    if (paint_directions & PAINT_DIRECTION_UP) { 
                        try_add_to_queue(fill_queue, px, trace_px, orig_color, bci.x, bci.y - 1, bci.width, bci, bci.max_queue_size);
                    }
                    if (paint_directions & PAINT_DIRECTION_DOWN) { 
                        try_add_to_queue(fill_queue, px, trace_px, orig_color, bci.x, bci.y + 1, bci.width, bci, bci.max_queue_size);
                    }
                }
                
                if (bci.max_vertical_check > 0) {
                    vertical_check_count = (vertical_check_count + 1) % bci.max_vertical_check;
                }
                
                if (bci.is_left) {
                    if (paint_directions & PAINT_DIRECTION_LEFT) {
                        bci.x -= 1;
                        ok = true;
                    }
                } else {
                    if (paint_directions & PAINT_DIRECTION_RIGHT) {
                        bci.x += 1;
                        ok = true;
                    }
                }
            }
        } else {
            if (bci.bbox.min()[NR::X] > bci.screen.min()[NR::X]) {
                aborted = true; break;
            } else {
                reached_screen_boundary = true;
            }
        }
    } while (ok);
    
    if (aborted) { return SCANLINE_CHECK_ABORTED; }
    if (reached_screen_boundary) { return SCANLINE_CHECK_BOUNDARY; }
    return SCANLINE_CHECK_OK;
}

static void sp_flood_do_flood_fill(SPEventContext *event_context, GdkEvent *event, bool union_with_selection, bool is_point_fill, bool is_touch_fill) {
    SPDesktop *desktop = event_context->desktop;
    SPDocument *document = sp_desktop_document(desktop);

    /* Create new arena */
    NRArena *arena = NRArena::create();
    unsigned dkey = sp_item_display_key_new(1);

    sp_document_ensure_up_to_date (document);
    
    SPItem *document_root = SP_ITEM(SP_DOCUMENT_ROOT(document));
    NR::Maybe<NR::Rect> bbox = document_root->getBounds(NR::identity());

    if (!bbox) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("<b>Area is not bounded</b>, cannot fill."));
        return;
    }
    
    double zoom_scale = desktop->current_zoom();
    double padding = 1.6;

    NR::Rect screen = desktop->get_display_area();

    int width = (int)ceil(screen.extent(NR::X) * zoom_scale * padding);
    int height = (int)ceil(screen.extent(NR::Y) * zoom_scale * padding);

    NR::Point origin(screen.min()[NR::X],
                     sp_document_height(document) - screen.extent(NR::Y) - screen.min()[NR::Y]);
                    
    origin[NR::X] = origin[NR::X] + (screen.extent(NR::X) * ((1 - padding) / 2));
    origin[NR::Y] = origin[NR::Y] + (screen.extent(NR::Y) * ((1 - padding) / 2));
    
    NR::scale scale(zoom_scale, zoom_scale);
    NR::Matrix affine = scale * NR::translate(-origin * scale);
    
    /* Create ArenaItems and set transform */
    NRArenaItem *root = sp_item_invoke_show(SP_ITEM(sp_document_root(document)), arena, dkey, SP_ITEM_SHOW_DISPLAY);
    nr_arena_item_set_transform(NR_ARENA_ITEM(root), affine);

    NRGC gc(NULL);
    nr_matrix_set_identity(&gc.transform);
    
    NRRectL final_bbox;
    final_bbox.x0 = 0;
    final_bbox.y0 = 0;//row;
    final_bbox.x1 = width;
    final_bbox.y1 = height;//row + num_rows;
    
    nr_arena_item_invoke_update(root, &final_bbox, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);

    guchar *px = g_new(guchar, 4 * width * height);
    
    NRPixBlock B;
    nr_pixblock_setup_extern( &B, NR_PIXBLOCK_MODE_R8G8B8A8N,
                              final_bbox.x0, final_bbox.y0, final_bbox.x1, final_bbox.y1,
                              px, 4 * width, FALSE, FALSE );
    
    SPNamedView *nv = sp_desktop_namedview(desktop);
    unsigned long bgcolor = nv->pagecolor;
    
    unsigned char dtc[4];
    dtc[0] = NR_RGBA32_R(bgcolor);
    dtc[1] = NR_RGBA32_G(bgcolor);
    dtc[2] = NR_RGBA32_B(bgcolor);
    dtc[3] = NR_RGBA32_A(bgcolor);
    
    for (int fy = 0; fy < height; fy++) {
        guchar *p = NR_PIXBLOCK_PX(&B) + fy * B.rs;
        for (int fx = 0; fx < width; fx++) {
            for (int i = 0; i < 4; i++) { 
                *p++ = dtc[i];
            }
        }
    }

    nr_arena_item_invoke_render(NULL, root, &final_bbox, &B, NR_ARENA_ITEM_RENDER_NO_CACHE );
    nr_pixblock_release(&B);
    
    // Hide items
    sp_item_invoke_hide(SP_ITEM(sp_document_root(document)), dkey);
    
    nr_arena_item_unref(root);
    nr_object_unref((NRObject *) arena);
    
    guchar *trace_px = g_new(guchar, 4 * width * height);
    memset(trace_px, 0x00, 4 * width * height);
    
    std::deque<NR::Point> fill_queue;
    std::queue<NR::Point> color_queue;
    
    std::vector<NR::Point> fill_points;
    
    if (is_point_fill) {
        fill_points.push_back(NR::Point(event->button.x, event->button.y));
    } else {
        Inkscape::Rubberband::Rubberband *r = Inkscape::Rubberband::get();
        fill_points = r->getPoints();
    }

    for (unsigned int i = 0; i < fill_points.size(); i++) {
        NR::Point pw = NR::Point(fill_points[i][NR::X] / zoom_scale, sp_document_height(document) + (fill_points[i][NR::Y] / zoom_scale)) * affine;
        
        pw[NR::X] = (int)MIN(width - 1, MAX(0, pw[NR::X]));
        pw[NR::Y] = (int)MIN(height - 1, MAX(0, pw[NR::Y]));
        
        if (is_touch_fill) {
            if (i == 0) {
                color_queue.push(pw);
            } else {
                fill_queue.push_back(pw);
            }
        } else {
            color_queue.push(pw);
        }
    }

    bool aborted = false;
    int y_limit = height - 1;

    PaintBucketChannels method = (PaintBucketChannels)prefs_get_int_attribute("tools.paintbucket", "channels", 0);
    int threshold = prefs_get_int_attribute_limited("tools.paintbucket", "threshold", 1, 0, 100);

    switch(method) {
        case FLOOD_CHANNELS_ALPHA:
        case FLOOD_CHANNELS_RGB:
        case FLOOD_CHANNELS_R:
        case FLOOD_CHANNELS_G:
        case FLOOD_CHANNELS_B:
            threshold = (255 * threshold) / 100;
            break;
        case FLOOD_CHANNELS_H:
        case FLOOD_CHANNELS_S:
        case FLOOD_CHANNELS_L:
            break;
      }

    bool reached_screen_boundary = false;

    bitmap_coords_info bci;
    
    bci.y_limit = y_limit;
    bci.width = width;
    bci.height = height;
    bci.threshold = threshold;
    bci.method = method;
    bci.bbox = *bbox;
    bci.screen = screen;
    bci.dtc = dtc;
    bci.radius = prefs_get_int_attribute_limited("tools.paintbucket", "autogap", 0, 0, 3);
    bci.max_queue_size = width * height;
    bci.max_vertical_check = prefs_get_int_attribute_limited("tools.paintbucket", "fillaccuracy", 0, 0, 5);

    bool first_run = true;

//     Time values to measure each buffer's paint time
//     GTimeVal tstart, tfinish;

//     g_get_current_time (&tstart);

    while (!color_queue.empty() && !aborted) {
        NR::Point color_point = color_queue.front();
        color_queue.pop();
        
        unsigned char *orig_px = get_pixel(px, (int)color_point[NR::X], (int)color_point[NR::Y], width);
        unsigned char orig_color[4];
        for (int i = 0; i < 4; i++) { orig_color[i] = orig_px[i]; }
        
        unsigned char merged_orig[4];
    
        merge_pixel_with_background(orig_color, dtc, merged_orig);
        
        unsigned char *trace_t = get_pixel(trace_px, (int)color_point[NR::X], (int)color_point[NR::Y], width);
        if (!is_pixel_checked(trace_t) && !is_pixel_colored(trace_t)) {
            fill_queue.push_front(color_point);
            
            if (!first_run) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        trace_t = get_pixel(trace_px, x, y, width);
                        clear_pixel_paintability(trace_t);
                    }
                }
            }
            first_run = false;
        }
        
        while (!fill_queue.empty() && !aborted) {
            NR::Point cp = fill_queue.front();
            fill_queue.pop_front();
            
            unsigned char *trace_t = get_pixel(trace_px, (int)cp[NR::X], (int)cp[NR::Y], width);
            if (!is_pixel_checked(trace_t)) {
                mark_pixel_checked(trace_t);
    
                int x = (int)cp[NR::X];
                int y = (int)cp[NR::Y];
                
                // same color at this point
                if (check_if_pixel_is_paintable(px, trace_px, x, y, orig_color, bci)) {
                    if (y == 0) {
                        if (bbox->min()[NR::Y] > screen.min()[NR::Y]) {
                            aborted = true; break;
                        } else {
                            reached_screen_boundary = true;
                        }
                    }
    
                    if (y == y_limit) {
                        if (bbox->max()[NR::Y] < screen.max()[NR::Y]) {
                            aborted = true; break;
                        } else {
                            reached_screen_boundary = true;
                        }
                    }
    
                    bci.is_left = true;
                    bci.x = x;
                    bci.y = y;
    
                    ScanlineCheckResult result = perform_bitmap_scanline_check(&fill_queue, px, trace_px, orig_color, bci);
    
                    switch (result) {
                        case SCANLINE_CHECK_ABORTED:
                            aborted = true;
                            break;
                        case SCANLINE_CHECK_BOUNDARY:
                            reached_screen_boundary = true;
                            break;
                        default:
                            break;
                    }
    
                    bci.is_left = false;
                    bci.x = x + 1;
                    bci.y = y;
    
                    result = perform_bitmap_scanline_check(&fill_queue, px, trace_px, orig_color, bci);
    
                    switch (result) {
                        case SCANLINE_CHECK_ABORTED:
                            aborted = true;
                            break;
                        case SCANLINE_CHECK_BOUNDARY:
                            reached_screen_boundary = true;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    
//     g_get_current_time (&tfinish);
    
//     Remember the slowest_buffer of this paint.
//     glong this_buffer = (tfinish.tv_sec - tstart.tv_sec) * 1000000 + (tfinish.tv_usec - tstart.tv_usec);
    
//     g_message("time: %ld", this_buffer);
    
    g_free(px);
    
    if (aborted) {
        g_free(trace_px);
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("<b>Area is not bounded</b>, cannot fill."));
        return;
    }
    
    if (reached_screen_boundary) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("<b>Only the visible part of the bounded area was filled.</b> If you want to fill all of the area, undo, zoom out, and fill again.")); 
    }
    
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(trace_px,
                                      GDK_COLORSPACE_RGB,
                                      TRUE,
                                      8, width, height, width * 4,
                                      (GdkPixbufDestroyNotify)g_free,
                                      NULL);

    NR::Matrix inverted_affine = NR::Matrix(affine).inverse();
    
    do_trace(pixbuf, desktop, inverted_affine, union_with_selection);

    g_free(trace_px);
    
    sp_document_done(document, SP_VERB_CONTEXT_PAINTBUCKET, _("Fill bounded area"));
}

static gint sp_flood_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
    gint ret = FALSE;

    SPDesktop *desktop = event_context->desktop;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        if (event->button.state & GDK_CONTROL_MASK) {
            NR::Point const button_w(event->button.x,
                                    event->button.y);
            
            SPItem *item = sp_event_context_find_item (desktop, button_w, TRUE, TRUE);
            
            Inkscape::XML::Node *pathRepr = SP_OBJECT_REPR(item);
            /* Set style */
            sp_desktop_apply_style_tool (desktop, pathRepr, "tools.paintbucket", false);
            sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_PAINTBUCKET, _("Set style on object"));
            ret = TRUE;
        }
        break;
    default:
        break;
    }

    if (((SPEventContextClass *) parent_class)->item_handler) {
        ret = ((SPEventContextClass *) parent_class)->item_handler(event_context, item, event);
    }

    return ret;
}

static gint sp_flood_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    static bool dragging;
    
    gint ret = FALSE;
    SPDesktop *desktop = event_context->desktop;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1 ) {
            if (!(event->button.state & GDK_CONTROL_MASK)) {
                NR::Point const button_w(event->button.x,
                                        event->button.y);
    
                if (Inkscape::have_viable_layer(desktop, event_context->defaultMessageContext())) {
                    // save drag origin
                    event_context->xp = (gint) button_w[NR::X];
                    event_context->yp = (gint) button_w[NR::Y];
                    event_context->within_tolerance = true;
                    
                    dragging = true;
                    
                    NR::Point const p(desktop->w2d(button_w));
                    Inkscape::Rubberband::get()->setMode(RUBBERBAND_MODE_TOUCHPATH);
                    Inkscape::Rubberband::get()->start(desktop, p);
                }
            }
        }
    case GDK_MOTION_NOTIFY:
        if ( dragging
             && ( event->motion.state & GDK_BUTTON1_MASK ) )
        {
            if ( event_context->within_tolerance
                 && ( abs( (gint) event->motion.x - event_context->xp ) < event_context->tolerance )
                 && ( abs( (gint) event->motion.y - event_context->yp ) < event_context->tolerance ) ) {
                break; // do not drag if we're within tolerance from origin
            }
            
            event_context->within_tolerance = false;
            
            NR::Point const motion_pt(event->motion.x, event->motion.y);
            NR::Point const p(desktop->w2d(motion_pt));
            if (Inkscape::Rubberband::get()->is_started()) {
                Inkscape::Rubberband::get()->move(p);
                event_context->defaultMessageContext()->set(Inkscape::NORMAL_MESSAGE, _("<b>Draw over</b> areas to add to fill, hold <b>Alt</b> for touch fill"));
                gobble_motion_events(GDK_BUTTON1_MASK);
            }
        }
        break;

    case GDK_BUTTON_RELEASE:
        if ( event->button.button == 1 ) {
            Inkscape::Rubberband::Rubberband *r = Inkscape::Rubberband::get();
            if (r->is_started()) {
                // set "busy" cursor
                desktop->setWaitingCursor();

                if (SP_IS_EVENT_CONTEXT(event_context)) { 
                    // Since setWaitingCursor runs main loop iterations, we may have already left this tool!
                    // So check if the tool is valid before doing anything
                    dragging = false;

                    bool is_point_fill = event_context->within_tolerance;
                    bool is_touch_fill = event->button.state & GDK_MOD1_MASK;
                    
                    sp_flood_do_flood_fill(event_context, event, event->button.state & GDK_SHIFT_MASK, is_point_fill, is_touch_fill);
                    
                    desktop->clearWaitingCursor();
                    // restore cursor when done; note that it may already be different if e.g. user 
                    // switched to another tool during interruptible tracing or drawing, in which case do nothing

                    ret = TRUE;
                }

                r->stop();
                event_context->defaultMessageContext()->clear();
            }
        }
        break;
    case GDK_KEY_PRESS:
        switch (get_group0_keyval (&event->key)) {
        case GDK_Up:
        case GDK_Down:
        case GDK_KP_Up:
        case GDK_KP_Down:
            // prevent the zoom field from activation
            if (!MOD__CTRL_ONLY)
                ret = TRUE;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (!ret) {
        if (((SPEventContextClass *) parent_class)->root_handler) {
            ret = ((SPEventContextClass *) parent_class)->root_handler(event_context, event);
        }
    }

    return ret;
}


static void sp_flood_finish(SPFloodContext *rc)
{
    rc->_message_context->clear();

    if ( rc->item != NULL ) {
        SPDesktop * desktop;

        desktop = SP_EVENT_CONTEXT_DESKTOP(rc);

        SP_OBJECT(rc->item)->updateRepr();

        sp_canvas_end_forced_full_redraws(desktop->canvas);

        sp_desktop_selection(desktop)->set(rc->item);
        sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_PAINTBUCKET,
                        _("Fill bounded area"));

        rc->item = NULL;
    }
}

void flood_channels_set_channels( gint channels )
{
    prefs_set_int_attribute("tools.paintbucket", "channels", channels);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
