#define __SP_DRAW_CONTEXT_C__

/*
 * Generic drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define DRAW_VERBOSE

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <gdk/gdkkeysyms.h>

#include "display/canvas-bpath.h"
#include "xml/repr.h"
#include "svg/svg.h"
#include <glibmm/i18n.h>
#include "libnr/n-art-bpath.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "document.h"
#include "draw-anchor.h"
#include "macros.h"
#include "message-stack.h"
#include "pen-context.h"
#include "prefs-utils.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "snap.h"
#include "sp-path.h"
#include "sp-namedview.h"

static void sp_draw_context_class_init(SPDrawContextClass *klass);
static void sp_draw_context_init(SPDrawContext *dc);
static void sp_draw_context_dispose(GObject *object);

static void sp_draw_context_setup(SPEventContext *ec);
static void sp_draw_context_set(SPEventContext *ec, gchar const *key, gchar const *value);
static void sp_draw_context_finish(SPEventContext *ec);

static gint sp_draw_context_root_handler(SPEventContext *event_context, GdkEvent *event);

static void spdc_selection_changed(Inkscape::Selection *sel, SPDrawContext *dc);
static void spdc_selection_modified(Inkscape::Selection *sel, guint flags, SPDrawContext *dc);

static void spdc_attach_selection(SPDrawContext *dc, Inkscape::Selection *sel);

static void spdc_flush_white(SPDrawContext *dc, SPCurve *gc);

static void spdc_reset_white(SPDrawContext *dc);
static void spdc_free_colors(SPDrawContext *dc);


static SPEventContextClass *draw_parent_class;


GType
sp_draw_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPDrawContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_draw_context_class_init,
            NULL, NULL,
            sizeof(SPDrawContext),
            4,
            (GInstanceInitFunc) sp_draw_context_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPDrawContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_draw_context_class_init(SPDrawContextClass *klass)
{
    GObjectClass *object_class;
    SPEventContextClass *ec_class;

    object_class = (GObjectClass *)klass;
    ec_class = (SPEventContextClass *) klass;

    draw_parent_class = (SPEventContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_draw_context_dispose;

    ec_class->setup = sp_draw_context_setup;
    ec_class->set = sp_draw_context_set;
    ec_class->finish = sp_draw_context_finish;
    ec_class->root_handler = sp_draw_context_root_handler;
}

static void
sp_draw_context_init(SPDrawContext *dc)
{
    dc->attach = FALSE;

    dc->red_color = 0xff00007f;
    dc->blue_color = 0x0000ff7f;
    dc->green_color = 0x00ff007f;
    dc->red_curve_is_valid = false;

    new (&dc->sel_changed_connection) sigc::connection();
    new (&dc->sel_modified_connection) sigc::connection();
}

static void
sp_draw_context_dispose(GObject *object)
{
    SPDrawContext *dc = SP_DRAW_CONTEXT(object);

    dc->sel_changed_connection.~connection();
    dc->sel_modified_connection.~connection();

    if (dc->grab) {
        sp_canvas_item_ungrab(dc->grab, GDK_CURRENT_TIME);
        dc->grab = NULL;
    }

    if (dc->selection) {
        dc->selection = NULL;
    }

    spdc_free_colors(dc);

    G_OBJECT_CLASS(draw_parent_class)->dispose(object);
}

static void
sp_draw_context_setup(SPEventContext *ec)
{
    SPDrawContext *dc = SP_DRAW_CONTEXT(ec);
    SPDesktop *dt = ec->desktop;

    if (((SPEventContextClass *) draw_parent_class)->setup) {
        ((SPEventContextClass *) draw_parent_class)->setup(ec);
    }

    dc->selection = sp_desktop_selection(dt);

    /* Connect signals to track selection changes */
    dc->sel_changed_connection = dc->selection->connectChanged(
        sigc::bind(sigc::ptr_fun(&spdc_selection_changed), dc)
    );
    dc->sel_modified_connection = dc->selection->connectModified(
        sigc::bind(sigc::ptr_fun(&spdc_selection_modified), dc)
    );

    /* Create red bpath */
    dc->red_bpath = sp_canvas_bpath_new(sp_desktop_sketch(ec->desktop), NULL);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->red_bpath), dc->red_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
    /* Create red curve */
    dc->red_curve = sp_curve_new_sized(4);

    /* Create blue bpath */
    dc->blue_bpath = sp_canvas_bpath_new(sp_desktop_sketch(ec->desktop), NULL);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->blue_bpath), dc->blue_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
    /* Create blue curve */
    dc->blue_curve = sp_curve_new_sized(8);

    /* Create green curve */
    dc->green_curve = sp_curve_new_sized(64);
    /* No green anchor by default */
    dc->green_anchor = NULL;
    dc->green_closed = FALSE;

    dc->attach = TRUE;
    spdc_attach_selection(dc, dc->selection);
}

static void
sp_draw_context_finish(SPEventContext *ec)
{
    SPDrawContext *dc = SP_DRAW_CONTEXT(ec);

    dc->sel_changed_connection.disconnect();
    dc->sel_modified_connection.disconnect();

    if (dc->grab) {
        sp_canvas_item_ungrab(dc->grab, GDK_CURRENT_TIME);
    }

    if (dc->selection) {
        dc->selection = NULL;
    }

    spdc_free_colors(dc);
}

static void
sp_draw_context_set(SPEventContext *ec, const gchar *key, const gchar *value)
{
}

gint
sp_draw_context_root_handler(SPEventContext *ec, GdkEvent *event)
{
    //SPDrawContext *dc = SP_DRAW_CONTEXT(ec);
    SPDesktop *desktop = ec->desktop;

    gint ret = FALSE;

    switch (event->type) {
        case GDK_KEY_PRESS:
            switch (get_group0_keyval (&event->key)) {
                case GDK_Escape:
                    sp_desktop_selection(desktop)->clear();
                    ret = TRUE;
                    break;
                case GDK_Tab: // Tab - cycle selection forward
                    if (!(MOD__CTRL_ONLY || (MOD__CTRL && MOD__SHIFT))) {
                        sp_selection_item_next();
                        ret = TRUE;
                    }
                    break;
                case GDK_ISO_Left_Tab: // Shift Tab - cycle selection backward
                    if (!(MOD__CTRL_ONLY || (MOD__CTRL && MOD__SHIFT))) {
                        sp_selection_item_prev();
                        ret = TRUE;
                    }
                    break;
                case GDK_Up:
                case GDK_Down:
                case GDK_KP_Up:
                case GDK_KP_Down:
                    // prevent the zoom field from activation
                    if (!MOD__CTRL_ONLY) {
                        ret = TRUE;
                    }
                    break;
                default:
            break;
        }
        break;
    default:
        break;
    }

    if (!ret) {
        if (((SPEventContextClass *) draw_parent_class)->root_handler) {
            ret = ((SPEventContextClass *) draw_parent_class)->root_handler(ec, event);
        }
    }

    return ret;
}


/*
 * Selection handlers
 */

static void
spdc_selection_changed(Inkscape::Selection *sel, SPDrawContext *dc)
{
    if (dc->attach) {
        spdc_attach_selection(dc, sel);
    }
}

/* fixme: We have to ensure this is not delayed (Lauris) */

static void
spdc_selection_modified(Inkscape::Selection *sel, guint flags, SPDrawContext *dc)
{
    if (dc->attach) {
        spdc_attach_selection(dc, sel);
    }
}

static void
spdc_attach_selection(SPDrawContext *dc, Inkscape::Selection *sel)
{
    /* We reset white and forget white/start/end anchors */
    spdc_reset_white(dc);
    dc->sa = NULL;
    dc->ea = NULL;

    SPItem *item = dc->selection ? dc->selection->singleItem() : NULL;

    if ( item && SP_IS_PATH(item) ) {
        /* Create new white data */
        /* Item */
        dc->white_item = item;
        /* Curve list */
        /* We keep it in desktop coordinates to eliminate calculation errors */
        SPCurve *norm = sp_shape_get_curve(SP_SHAPE(item));
        sp_curve_transform(norm, sp_item_i2d_affine(dc->white_item));
        g_return_if_fail( norm != NULL );
        dc->white_curves = g_slist_reverse(sp_curve_split(norm));
        sp_curve_unref(norm);
        /* Anchor list */
        for (GSList *l = dc->white_curves; l != NULL; l = l->next) {
            SPCurve *c;
            c = (SPCurve*)l->data;
            g_return_if_fail( c->end > 1 );
            if ( SP_CURVE_BPATH(c)->code == NR_MOVETO_OPEN ) {
                NArtBpath *s, *e;
                SPDrawAnchor *a;
                s = sp_curve_first_bpath(c);
                e = sp_curve_last_bpath(c);
                a = sp_draw_anchor_new(dc, c, TRUE, NR::Point(s->x3, s->y3));
                dc->white_anchors = g_slist_prepend(dc->white_anchors, a);
                a = sp_draw_anchor_new(dc, c, FALSE, NR::Point(e->x3, e->y3));
                dc->white_anchors = g_slist_prepend(dc->white_anchors, a);
            }
        }
        /* fixme: recalculate active anchor? */
    }
}


/**
 *  Snaps node or handle to PI/rotationsnapsperpi degree increments.
 *
 *  \param dc draw context
 *  \param p cursor point (to be changed by snapping)
 *  \param o origin point
 *  \param state  keyboard state to check if ctrl was pressed
*/

void spdc_endpoint_snap_rotation(SPEventContext const *const ec, NR::Point &p, NR::Point const o,
                                 guint state)
{
    /* Control must be down for this snap to work */
    if ((state & GDK_CONTROL_MASK) == 0) {
        return;
    }

    unsigned const snaps = abs(prefs_get_int_attribute("options.rotationsnapsperpi", "value", 12));
    /* 0 means no snapping. */

    /* mirrored by fabs, so this corresponds to 15 degrees */
    NR::Point best; /* best solution */
    double bn = NR_HUGE; /* best normal */
    double bdot = 0;
    NR::Point v = NR::Point(0, 1);
    double const r00 = cos(M_PI / snaps), r01 = sin(M_PI / snaps);
    double const r10 = -r01, r11 = r00;

    NR::Point delta = p - o;

    for (unsigned i = 0; i < snaps; i++) {
        double const ndot = fabs(dot(v,NR::rot90(delta)));
        NR::Point t(r00*v[NR::X] + r01*v[NR::Y],
                    r10*v[NR::X] + r11*v[NR::Y]);
        if (ndot < bn) {
            /* I think it is better numerically to use the normal, rather than the dot product
             * to assess solutions, but I haven't proven it. */
            bn = ndot;
            best = v;
            bdot = dot(v, delta);
        }
        v = t;
    }

    if (fabs(bdot) > 0) {
        p = o + bdot * best;

        /* Snap it along best vector */
        SnapManager const &m = SP_EVENT_CONTEXT_DESKTOP(ec)->namedview->snap_manager;
        p = m.constrainedSnap(Inkscape::Snapper::SNAP_POINT | Inkscape::Snapper::BBOX_POINT,
                              p, Inkscape::Snapper::ConstraintLine(best), NULL).getPoint();
    }
}


void spdc_endpoint_snap_free(SPEventContext const * const ec, NR::Point& p, guint const state)
{
    /* Shift disables this snap */
    if (state & GDK_SHIFT_MASK) {
        return;
    }

    /* FIXME: this should be doing bbox snap as well */
    SnapManager const &m = SP_EVENT_CONTEXT_DESKTOP(ec)->namedview->snap_manager;
    p = m.freeSnap(Inkscape::Snapper::BBOX_POINT | Inkscape::Snapper::SNAP_POINT, p, NULL).getPoint();
}

static SPCurve *
reverse_then_unref(SPCurve *orig)
{
    SPCurve *ret = sp_curve_reverse(orig);
    sp_curve_unref(orig);
    return ret;
}

/**
 * Concats red, blue and green.
 * If any anchors are defined, process these, optionally removing curves from white list
 * Invoke _flush_white to write result back to object.
 */
void
spdc_concat_colors_and_flush(SPDrawContext *dc, bool forceclosed)
{
    /* Concat RBG */
    SPCurve *c = dc->green_curve;

    /* Green */
    dc->green_curve = sp_curve_new_sized(64);
    while (dc->green_bpaths) {
        gtk_object_destroy(GTK_OBJECT(dc->green_bpaths->data));
        dc->green_bpaths = g_slist_remove(dc->green_bpaths, dc->green_bpaths->data);
    }
    /* Blue */
    sp_curve_append_continuous(c, dc->blue_curve, 0.0625);
    sp_curve_reset(dc->blue_curve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->blue_bpath), NULL);
    /* Red */
    if (dc->red_curve_is_valid) {
        sp_curve_append_continuous(c, dc->red_curve, 0.0625);
    }
    sp_curve_reset(dc->red_curve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->red_bpath), NULL);

    if (sp_curve_empty(c)) {
        sp_curve_unref(c);
        return;
    }

    /* Step A - test, whether we ended on green anchor */
    if ( forceclosed || ( dc->green_anchor && dc->green_anchor->active ) ) {
        // We hit green anchor, closing Green-Blue-Red
        SP_EVENT_CONTEXT_DESKTOP(dc)->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Path is closed."));
        sp_curve_closepath_current(c);
        /* Closed path, just flush */
        spdc_flush_white(dc, c);
        sp_curve_unref(c);
        return;
    }

    /* Step B - both start and end anchored to same curve */
    if ( dc->sa && dc->ea
         && ( dc->sa->curve == dc->ea->curve )
         && ( ( dc->sa != dc->ea )
              || dc->sa->curve->closed ) )
    {
        // We hit bot start and end of single curve, closing paths
        SP_EVENT_CONTEXT_DESKTOP(dc)->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Closing path."));
        if (dc->sa->start && !(dc->sa->curve->closed) ) {
            c = reverse_then_unref(c);
        }
        sp_curve_append_continuous(dc->sa->curve, c, 0.0625);
        sp_curve_unref(c);
        sp_curve_closepath_current(dc->sa->curve);
        spdc_flush_white(dc, NULL);
        return;
    }

    /* Step C - test start */
    if (dc->sa) {
        SPCurve *s = dc->sa->curve;
        dc->white_curves = g_slist_remove(dc->white_curves, s);
        if (dc->sa->start) {
            s = reverse_then_unref(s);
        }
        sp_curve_append_continuous(s, c, 0.0625);
        sp_curve_unref(c);
        c = s;
    } else /* Step D - test end */ if (dc->ea) {
        SPCurve *e = dc->ea->curve;
        dc->white_curves = g_slist_remove(dc->white_curves, e);
        if (!dc->ea->start) {
            e = reverse_then_unref(e);
        }
        sp_curve_append_continuous(c, e, 0.0625);
        sp_curve_unref(e);
    }


    spdc_flush_white(dc, c);

    sp_curve_unref(c);
}

static char const *
tool_name(SPDrawContext *dc)
{
    return ( SP_IS_PEN_CONTEXT(dc)
             ? "tools.freehand.pen"
             : "tools.freehand.pencil" );
}

/*
 * Flushes white curve(s) and additional curve into object
 *
 * No cleaning of colored curves - this has to be done by caller
 * No rereading of white data, so if you cannot rely on ::modified, do it in caller
 *
 */

static void
spdc_flush_white(SPDrawContext *dc, SPCurve *gc)
{
    SPCurve *c;

    if (dc->white_curves) {
        g_assert(dc->white_item);
        c = sp_curve_concat(dc->white_curves);
        g_slist_free(dc->white_curves);
        dc->white_curves = NULL;
        if (gc) {
            sp_curve_append(c, gc, FALSE);
        }
    } else if (gc) {
        c = gc;
        sp_curve_ref(c);
    } else {
        return;
    }

    /* Now we have to go back to item coordinates at last */
    sp_curve_transform(c, ( dc->white_item
                            ? sp_item_dt2i_affine(dc->white_item)
                            : sp_desktop_dt2root_affine(SP_EVENT_CONTEXT_DESKTOP(dc)) ));

    SPDesktop *desktop = SP_EVENT_CONTEXT_DESKTOP(dc);
    SPDocument *doc = sp_desktop_document(desktop);

    if ( c && !sp_curve_empty(c) ) {
        /* We actually have something to write */

        Inkscape::XML::Node *repr;
        if (dc->white_item) {
            repr = SP_OBJECT_REPR(dc->white_item);
        } else {
            repr = sp_repr_new("svg:path");
            /* Set style */
            sp_desktop_apply_style_tool(desktop, repr, tool_name(dc), false);
        }

        gchar *str = sp_svg_write_path(SP_CURVE_BPATH(c));
        g_assert( str != NULL );
        repr->setAttribute("d", str);
        g_free(str);

        if (!dc->white_item) {
            /* Attach repr */
            SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
            dc->selection->set(repr);
            Inkscape::GC::release(repr);
            item->transform = i2i_affine(desktop->currentRoot(), desktop->currentLayer());
            item->updateRepr();
        }

        sp_document_done(doc, SP_IS_PEN_CONTEXT(dc)? SP_VERB_CONTEXT_PEN : SP_VERB_CONTEXT_PENCIL, 
                         /* TODO: annotate */ "draw-context.cpp:561");
    }

    sp_curve_unref(c);

    /* Flush pending updates */
    sp_document_ensure_up_to_date(doc);
}

/**
 * Returns FIRST active anchor (the activated one).
 */
SPDrawAnchor *
spdc_test_inside(SPDrawContext *dc, NR::Point p)
{
    SPDrawAnchor *active = NULL;

    /* Test green anchor */
    if (dc->green_anchor) {
        active = sp_draw_anchor_test(dc->green_anchor, p, TRUE);
    }

    for (GSList *l = dc->white_anchors; l != NULL; l = l->next) {
        SPDrawAnchor *na = sp_draw_anchor_test((SPDrawAnchor *) l->data, p, !active);
        if ( !active && na ) {
            active = na;
        }
    }

    return active;
}

static void
spdc_reset_white(SPDrawContext *dc)
{
    if (dc->white_item) {
        /* We do not hold refcount */
        dc->white_item = NULL;
    }
    while (dc->white_curves) {
        sp_curve_unref((SPCurve *) dc->white_curves->data);
        dc->white_curves = g_slist_remove(dc->white_curves, dc->white_curves->data);
    }
    while (dc->white_anchors) {
        sp_draw_anchor_destroy((SPDrawAnchor *) dc->white_anchors->data);
        dc->white_anchors = g_slist_remove(dc->white_anchors, dc->white_anchors->data);
    }
}

static void
spdc_free_colors(SPDrawContext *dc)
{
    /* Red */
    if (dc->red_bpath) {
        gtk_object_destroy(GTK_OBJECT(dc->red_bpath));
        dc->red_bpath = NULL;
    }
    if (dc->red_curve) {
        dc->red_curve = sp_curve_unref(dc->red_curve);
    }
    /* Blue */
    if (dc->blue_bpath) {
        gtk_object_destroy(GTK_OBJECT(dc->blue_bpath));
        dc->blue_bpath = NULL;
    }
    if (dc->blue_curve) {
        dc->blue_curve = sp_curve_unref(dc->blue_curve);
    }
    /* Green */
    while (dc->green_bpaths) {
        gtk_object_destroy(GTK_OBJECT(dc->green_bpaths->data));
        dc->green_bpaths = g_slist_remove(dc->green_bpaths, dc->green_bpaths->data);
    }
    if (dc->green_curve) {
        dc->green_curve = sp_curve_unref(dc->green_curve);
    }
    if (dc->green_anchor) {
        dc->green_anchor = sp_draw_anchor_destroy(dc->green_anchor);
    }
    /* White */
    if (dc->white_item) {
        /* We do not hold refcount */
        dc->white_item = NULL;
    }
    while (dc->white_curves) {
        sp_curve_unref((SPCurve *) dc->white_curves->data);
        dc->white_curves = g_slist_remove(dc->white_curves, dc->white_curves->data);
    }
    while (dc->white_anchors) {
        sp_draw_anchor_destroy((SPDrawAnchor *) dc->white_anchors->data);
        dc->white_anchors = g_slist_remove(dc->white_anchors, dc->white_anchors->data);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
