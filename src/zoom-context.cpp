#define __SP_ZOOM_CONTEXT_C__

/*
 * Handy zooming tool
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include <gdk/gdkkeysyms.h>

#include "macros.h"
#include "rubberband.h"
#include "desktop.h"
#include "pixmaps/cursor-zoom.xpm"
#include "pixmaps/cursor-zoom-out.xpm"
#include "pixmaps/cursor-zoom.pixbuf"
#include "pixmaps/cursor-zoom-out.pixbuf"
#include "prefs-utils.h"

#include "zoom-context.h"

static void sp_zoom_context_class_init(SPZoomContextClass *klass);
static void sp_zoom_context_init(SPZoomContext *zoom_context);
static void sp_zoom_context_setup(SPEventContext *ec);
static void sp_zoom_context_finish (SPEventContext *ec);

static gint sp_zoom_context_root_handler(SPEventContext *event_context, GdkEvent *event);
static gint sp_zoom_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);

static SPEventContextClass *parent_class;

static gint xp = 0, yp = 0; // where drag started
static gint tolerance = 0;
static bool within_tolerance = false;
static bool escaped;

GType sp_zoom_context_get_type(void)
{
    static GType type = 0;

    if (!type) {
        GTypeInfo info = {
            sizeof(SPZoomContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_zoom_context_class_init,
            NULL, NULL,
            sizeof(SPZoomContext),
            4,
            (GInstanceInitFunc) sp_zoom_context_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPZoomContext", &info, (GTypeFlags) 0);
    }

    return type;
}

static void sp_zoom_context_class_init(SPZoomContextClass *klass)
{
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass*) g_type_class_peek_parent(klass);

    event_context_class->setup = sp_zoom_context_setup;
    event_context_class->finish = sp_zoom_context_finish;

    event_context_class->root_handler = sp_zoom_context_root_handler;
    event_context_class->item_handler = sp_zoom_context_item_handler;
}

static void sp_zoom_context_init (SPZoomContext *zoom_context)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(zoom_context);

    event_context->cursor_shape = cursor_zoom_xpm;
    event_context->cursor_pixbuf = gdk_pixbuf_new_from_inline(
            -1,
            cursor_zoom_pixbuf,
            FALSE,
            NULL);    
    event_context->hot_x = 9;
    event_context->hot_y = 9;
}

static void
sp_zoom_context_finish (SPEventContext *ec)
{
	ec->enableGrDrag(false);
}

static void sp_zoom_context_setup(SPEventContext *ec)
{
    if (prefs_get_int_attribute("tools.zoom", "selcue", 0) != 0) {
        ec->enableSelectionCue();
    }
    if (prefs_get_int_attribute("tools.zoom", "gradientdrag", 0) != 0) {
        ec->enableGrDrag();
    }

    if (((SPEventContextClass *) parent_class)->setup) {
        ((SPEventContextClass *) parent_class)->setup(ec);
    }
}

static gint sp_zoom_context_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
    gint ret = FALSE;

    if (((SPEventContextClass *) parent_class)->item_handler) {
        ret = ((SPEventContextClass *) parent_class)->item_handler (event_context, item, event);
    }

    return ret;
}

static gint sp_zoom_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    SPDesktop *desktop = event_context->desktop;
    tolerance = prefs_get_int_attribute_limited ("options.dragtolerance", "value", 0, 0, 100);
    double const zoom_inc = prefs_get_double_attribute_limited("options.zoomincrement", "value", M_SQRT2, 1.01, 10);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if (event->button.button == 1) {
                // save drag origin
                xp = (gint) event->button.x;
                yp = (gint) event->button.y;
                within_tolerance = true;

                NR::Point const button_w(event->button.x, event->button.y);
                NR::Point const button_dt(desktop->w2d(button_w));
                Inkscape::Rubberband::get()->start(desktop, button_dt);

                escaped = false;

                ret = TRUE;
            }
            break;

	case GDK_MOTION_NOTIFY:
            if (event->motion.state & GDK_BUTTON1_MASK) {
                ret = TRUE;

                if ( within_tolerance
                     && ( abs( (gint) event->motion.x - xp ) < tolerance )
                     && ( abs( (gint) event->motion.y - yp ) < tolerance ) ) {
                    break; // do not drag if we're within tolerance from origin
                }
                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to move the object, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                within_tolerance = false;

                NR::Point const motion_w(event->motion.x, event->motion.y);
                NR::Point const motion_dt(desktop->w2d(motion_w));
                Inkscape::Rubberband::get()->move(motion_dt);
            }
            break;

	case GDK_BUTTON_RELEASE:
            if ( event->button.button == 1 ) {
                NR::Maybe<NR::Rect> const b = Inkscape::Rubberband::get()->getRectangle();
                if (b != NR::Nothing() && !within_tolerance) {
                    desktop->set_display_area(b.assume(), 10);
                } else if (!escaped) {
                    NR::Point const button_w(event->button.x, event->button.y);
                    NR::Point const button_dt(desktop->w2d(button_w));
                    double const zoom_rel( (event->button.state & GDK_SHIFT_MASK)
                                           ? 1 / zoom_inc
                                           : zoom_inc );
                    desktop->zoom_relative_keep_point(button_dt, zoom_rel);
                }
                ret = TRUE;
            }
            Inkscape::Rubberband::get()->stop();
            xp = yp = 0;
            escaped = false;
            break;

        case GDK_KEY_PRESS:
            switch (get_group0_keyval (&event->key)) {
                case GDK_Escape:
                    Inkscape::Rubberband::get()->stop();
                    xp = yp = 0;
                    escaped = true;
                    ret = TRUE;
                    break;
                case GDK_Up:
                case GDK_Down:
                case GDK_KP_Up:
                case GDK_KP_Down:
                    // prevent the zoom field from activation
                    if (!MOD__CTRL_ONLY)
                        ret = TRUE;
                    break;
                case GDK_Shift_L:
                case GDK_Shift_R:
                    event_context->cursor_shape = cursor_zoom_out_xpm;
                    event_context->cursor_pixbuf = gdk_pixbuf_new_from_inline(
                            -1,
                            cursor_zoom_out_pixbuf,
                            FALSE,
                            NULL);    
                    sp_event_context_update_cursor(event_context);
                    break;
                default:
                    break;
            }
            break;
        case GDK_KEY_RELEASE:
            switch (get_group0_keyval (&event->key)) {
                case GDK_Shift_L:
                case GDK_Shift_R:
                    event_context->cursor_shape = cursor_zoom_xpm;
                    event_context->cursor_pixbuf = gdk_pixbuf_new_from_inline(
                            -1,
                            cursor_zoom_pixbuf,
                            FALSE,
                            NULL);
                    sp_event_context_update_cursor(event_context);
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
