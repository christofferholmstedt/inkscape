#define __SP_DYNA_DRAW_CONTEXT_C__

/*
 * Handwriting-like drawing mode
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   MenTaLguY <mental@rydia.net>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2005-2007 bulia byak
 * Copyright (C) 2006 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noDYNA_DRAW_VERBOSE

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glibmm/i18n.h>

#include <numeric>

#include "svg/svg.h"
#include "display/canvas-bpath.h"
#include "display/bezier-utils.h"

#include <glib/gmem.h>
#include "macros.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-style.h"
#include "message-context.h"
#include "pixmaps/cursor-calligraphy.xpm"
#include "pixmaps/cursor-thin.xpm"
#include "pixmaps/cursor-thicken.xpm"
#include "libnr/n-art-bpath.h"
#include "libnr/nr-path.h"
#include "libnr/nr-matrix-ops.h"
#include "libnr/nr-scale-translate-ops.h"
#include "xml/repr.h"
#include "context-fns.h"
#include "sp-item.h"
#include "inkscape.h"
#include "color.h"
#include "splivarot.h"
#include "sp-shape.h"
#include "sp-path.h"
#include "sp-text.h"
#include "display/canvas-bpath.h"
#include "display/canvas-arena.h"
#include "livarot/Shape.h"
#include "isnan.h"

#include "dyna-draw-context.h"

#define DDC_RED_RGBA 0xff0000ff

#define TOLERANCE_CALLIGRAPHIC 0.1

#define DYNA_EPSILON 0.5e-6
#define DYNA_EPSILON_START 0.5e-2
#define DYNA_VEL_START 1e-5

#define DYNA_MIN_WIDTH 1.0e-6

#define DRAG_MIN 0.0
#define DRAG_DEFAULT 1.0
#define DRAG_MAX 1.0

// FIXME: move it to some shared file to be reused by both calligraphy and dropper
#define C1 0.552
static NArtBpath const hatch_area_circle[] = {
    { NR_MOVETO, 0, 0, 0, 0, -1, 0 },
    { NR_CURVETO, -1, C1, -C1, 1, 0, 1 },
    { NR_CURVETO, C1, 1, 1, C1, 1, 0 },
    { NR_CURVETO, 1, -C1, C1, -1, 0, -1 },
    { NR_CURVETO, -C1, -1, -1, -C1, -1, 0 },
    { NR_END, 0, 0, 0, 0, 0, 0 }
};
#undef C1


static void sp_dyna_draw_context_class_init(SPDynaDrawContextClass *klass);
static void sp_dyna_draw_context_init(SPDynaDrawContext *ddc);
static void sp_dyna_draw_context_dispose(GObject *object);

static void sp_dyna_draw_context_setup(SPEventContext *ec);
static void sp_dyna_draw_context_set(SPEventContext *ec, gchar const *key, gchar const *val);
static gint sp_dyna_draw_context_root_handler(SPEventContext *ec, GdkEvent *event);

static void clear_current(SPDynaDrawContext *dc);
static void set_to_accumulated(SPDynaDrawContext *dc, bool unionize);
static void add_cap(SPCurve *curve, NR::Point const &pre, NR::Point const &from, NR::Point const &to, NR::Point const &post, double rounding);
static void accumulate_calligraphic(SPDynaDrawContext *dc);

static void fit_and_split(SPDynaDrawContext *ddc, gboolean release);

static void sp_dyna_draw_reset(SPDynaDrawContext *ddc, NR::Point p);
static NR::Point sp_dyna_draw_get_npoint(SPDynaDrawContext const *ddc, NR::Point v);
static NR::Point sp_dyna_draw_get_vpoint(SPDynaDrawContext const *ddc, NR::Point n);
static void draw_temporary_box(SPDynaDrawContext *dc);


static SPEventContextClass *parent_class;

GtkType
sp_dyna_draw_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPDynaDrawContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_dyna_draw_context_class_init,
            NULL, NULL,
            sizeof(SPDynaDrawContext),
            4,
            (GInstanceInitFunc) sp_dyna_draw_context_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPDynaDrawContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_dyna_draw_context_class_init(SPDynaDrawContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_dyna_draw_context_dispose;

    event_context_class->setup = sp_dyna_draw_context_setup;
    event_context_class->set = sp_dyna_draw_context_set;
    event_context_class->root_handler = sp_dyna_draw_context_root_handler;
}

static void
sp_dyna_draw_context_init(SPDynaDrawContext *ddc)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(ddc);

    event_context->cursor_shape = cursor_calligraphy_xpm;
    event_context->hot_x = 4;
    event_context->hot_y = 4;

    ddc->accumulated = NULL;
    ddc->segments = NULL;
    ddc->currentcurve = NULL;
    ddc->currentshape = NULL;
    ddc->npoints = 0;
    ddc->cal1 = NULL;
    ddc->cal2 = NULL;
    ddc->repr = NULL;

    /* DynaDraw values */
    ddc->cur = NR::Point(0,0);
    ddc->last = NR::Point(0,0);
    ddc->vel = NR::Point(0,0);
    ddc->vel_max = 0;
    ddc->acc = NR::Point(0,0);
    ddc->ang = NR::Point(0,0);
    ddc->del = NR::Point(0,0);

    /* attributes */
    ddc->dragging = FALSE;

    ddc->mass = 0.3;
    ddc->drag = DRAG_DEFAULT;
    ddc->angle = 30.0;
    ddc->width = 0.2;
    ddc->pressure = DDC_DEFAULT_PRESSURE;

    ddc->vel_thin = 0.1;
    ddc->flatness = 0.9;
    ddc->cap_rounding = 0.0;

    ddc->abs_width = false;
    ddc->keep_selected = true;

    ddc->hatch_spacing = 0;
    new (&ddc->hatch_pointer_past) std::list<double>();
    new (&ddc->hatch_nearest_past) std::list<double>();
    ddc->hatch_last_nearest = NR::Point(0,0);
    ddc->hatch_last_pointer = NR::Point(0,0);
    ddc->hatch_vector_accumulated = NR::Point(0,0);
    ddc->hatch_escaped = false;
    ddc->hatch_area = NULL;
    ddc->hatch_item = NULL;
    ddc->hatch_livarot_path = NULL;

    ddc->trace_bg = false;

    ddc->is_dilating = false;
}

static void
sp_dyna_draw_context_dispose(GObject *object)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(object);

    if (ddc->hatch_area) {
        gtk_object_destroy(GTK_OBJECT(ddc->hatch_area));
        ddc->hatch_area = NULL;
    }

    if (ddc->dilate_area) {
        gtk_object_destroy(GTK_OBJECT(ddc->dilate_area));
        ddc->dilate_area = NULL;
    }

    if (ddc->accumulated) {
        ddc->accumulated = sp_curve_unref(ddc->accumulated);
    }

    while (ddc->segments) {
        gtk_object_destroy(GTK_OBJECT(ddc->segments->data));
        ddc->segments = g_slist_remove(ddc->segments, ddc->segments->data);
    }

    if (ddc->currentcurve) ddc->currentcurve = sp_curve_unref(ddc->currentcurve);
    if (ddc->cal1) ddc->cal1 = sp_curve_unref(ddc->cal1);
    if (ddc->cal2) ddc->cal2 = sp_curve_unref(ddc->cal2);

    if (ddc->currentshape) {
        gtk_object_destroy(GTK_OBJECT(ddc->currentshape));
        ddc->currentshape = NULL;
    }

    if (ddc->_message_context) {
        delete ddc->_message_context;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);

    ddc->hatch_pointer_past.~list();
    ddc->hatch_nearest_past.~list();
}

static void
sp_dyna_draw_context_setup(SPEventContext *ec)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup)
        ((SPEventContextClass *) parent_class)->setup(ec);

    ddc->accumulated = sp_curve_new_sized(32);
    ddc->currentcurve = sp_curve_new_sized(4);

    ddc->cal1 = sp_curve_new_sized(32);
    ddc->cal2 = sp_curve_new_sized(32);

    ddc->currentshape = sp_canvas_item_new(sp_desktop_sketch(ec->desktop), SP_TYPE_CANVAS_BPATH, NULL);
    sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(ddc->currentshape), DDC_RED_RGBA, SP_WIND_RULE_EVENODD);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(ddc->currentshape), 0x00000000, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
    /* fixme: Cannot we cascade it to root more clearly? */
    g_signal_connect(G_OBJECT(ddc->currentshape), "event", G_CALLBACK(sp_desktop_root_handler), ec->desktop);

    {
        SPCurve *c = sp_curve_new_from_foreign_bpath(hatch_area_circle);
        ddc->hatch_area = sp_canvas_bpath_new(sp_desktop_controls(ec->desktop), c);
        sp_curve_unref(c);
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(ddc->hatch_area), 0x00000000,(SPWindRule)0);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(ddc->hatch_area), 0x0000007f, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_hide(ddc->hatch_area);
    }

    {
        SPCurve *c = sp_curve_new_from_foreign_bpath(hatch_area_circle);
        ddc->dilate_area = sp_canvas_bpath_new(sp_desktop_controls(ec->desktop), c);
        sp_curve_unref(c);
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(ddc->dilate_area), 0x00000000,(SPWindRule)0);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(ddc->dilate_area), 0xff9900ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_hide(ddc->dilate_area);
    }

    sp_event_context_read(ec, "mass");
    sp_event_context_read(ec, "wiggle");
    sp_event_context_read(ec, "angle");
    sp_event_context_read(ec, "width");
    sp_event_context_read(ec, "thinning");
    sp_event_context_read(ec, "tremor");
    sp_event_context_read(ec, "flatness");
    sp_event_context_read(ec, "tracebackground");
    sp_event_context_read(ec, "usepressure");
    sp_event_context_read(ec, "usetilt");
    sp_event_context_read(ec, "abs_width");
    sp_event_context_read(ec, "keep_selected");
    sp_event_context_read(ec, "cap_rounding");

    ddc->is_drawing = false;

    ddc->_message_context = new Inkscape::MessageContext((ec->desktop)->messageStack());
}

static void
sp_dyna_draw_context_set(SPEventContext *ec, gchar const *key, gchar const *val)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(ec);

    if (!strcmp(key, "mass")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.2 );
        ddc->mass = CLAMP(dval, -1000.0, 1000.0);
    } else if (!strcmp(key, "wiggle")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : (1 - DRAG_DEFAULT));
        ddc->drag = CLAMP((1 - dval), DRAG_MIN, DRAG_MAX); // drag is inverse to wiggle
    } else if (!strcmp(key, "angle")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.0);
        ddc->angle = CLAMP (dval, -90, 90);
    } else if (!strcmp(key, "width")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.1 );
        ddc->width = CLAMP(dval, -1000.0, 1000.0);
    } else if (!strcmp(key, "thinning")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.1 );
        ddc->vel_thin = CLAMP(dval, -1.0, 1.0);
    } else if (!strcmp(key, "tremor")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.0 );
        ddc->tremor = CLAMP(dval, 0.0, 1.0);
    } else if (!strcmp(key, "flatness")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 1.0 );
        ddc->flatness = CLAMP(dval, 0, 1.0);
    } else if (!strcmp(key, "tracebackground")) {
        ddc->trace_bg = (val && strcmp(val, "0"));
    } else if (!strcmp(key, "usepressure")) {
        ddc->usepressure = (val && strcmp(val, "0"));
    } else if (!strcmp(key, "usetilt")) {
        ddc->usetilt = (val && strcmp(val, "0"));
    } else if (!strcmp(key, "abs_width")) {
        ddc->abs_width = (val && strcmp(val, "0"));
    } else if (!strcmp(key, "keep_selected")) {
        ddc->keep_selected = (val && strcmp(val, "0"));
    } else if (!strcmp(key, "cap_rounding")) {
        ddc->cap_rounding = ( val ? g_ascii_strtod (val, NULL) : 0.0 );
    }

    //g_print("DDC: %g %g %g %g\n", ddc->mass, ddc->drag, ddc->angle, ddc->width);
}

static double
flerp(double f0, double f1, double p)
{
    return f0 + ( f1 - f0 ) * p;
}

/* Get normalized point */
static NR::Point
sp_dyna_draw_get_npoint(SPDynaDrawContext const *dc, NR::Point v)
{
    NR::Rect drect = SP_EVENT_CONTEXT(dc)->desktop->get_display_area();
    double const max = MAX ( drect.dimensions()[NR::X], drect.dimensions()[NR::Y] );
    return NR::Point(( v[NR::X] - drect.min()[NR::X] ) / max,  ( v[NR::Y] - drect.min()[NR::Y] ) / max);
}

/* Get view point */
static NR::Point
sp_dyna_draw_get_vpoint(SPDynaDrawContext const *dc, NR::Point n)
{
    NR::Rect drect = SP_EVENT_CONTEXT(dc)->desktop->get_display_area();
    double const max = MAX ( drect.dimensions()[NR::X], drect.dimensions()[NR::Y] );
    return NR::Point(n[NR::X] * max + drect.min()[NR::X], n[NR::Y] * max + drect.min()[NR::Y]);
}

static void
sp_dyna_draw_reset(SPDynaDrawContext *dc, NR::Point p)
{
    dc->last = dc->cur = sp_dyna_draw_get_npoint(dc, p);
    dc->vel = NR::Point(0,0);
    dc->vel_max = 0;
    dc->acc = NR::Point(0,0);
    dc->ang = NR::Point(0,0);
    dc->del = NR::Point(0,0);
}

static void
sp_dyna_draw_extinput(SPDynaDrawContext *dc, GdkEvent *event)
{
    if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &dc->pressure))
        dc->pressure = CLAMP (dc->pressure, DDC_MIN_PRESSURE, DDC_MAX_PRESSURE);
    else
        dc->pressure = DDC_DEFAULT_PRESSURE;

    if (gdk_event_get_axis (event, GDK_AXIS_XTILT, &dc->xtilt))
        dc->xtilt = CLAMP (dc->xtilt, DDC_MIN_TILT, DDC_MAX_TILT);
    else
        dc->xtilt = DDC_DEFAULT_TILT;

    if (gdk_event_get_axis (event, GDK_AXIS_YTILT, &dc->ytilt))
        dc->ytilt = CLAMP (dc->ytilt, DDC_MIN_TILT, DDC_MAX_TILT);
    else
        dc->ytilt = DDC_DEFAULT_TILT;
}


static gboolean
sp_dyna_draw_apply(SPDynaDrawContext *dc, NR::Point p)
{
    NR::Point n = sp_dyna_draw_get_npoint(dc, p);

    /* Calculate mass and drag */
    double const mass = flerp(1.0, 160.0, dc->mass);
    double const drag = flerp(0.0, 0.5, dc->drag * dc->drag);

    /* Calculate force and acceleration */
    NR::Point force = n - dc->cur;

    // If force is below the absolute threshold DYNA_EPSILON,
    // or we haven't yet reached DYNA_VEL_START (i.e. at the beginning of stroke)
    // _and_ the force is below the (higher) DYNA_EPSILON_START threshold,
    // discard this move. 
    // This prevents flips, blobs, and jerks caused by microscopic tremor of the tablet pen,
    // especially bothersome at the start of the stroke where we don't yet have the inertia to
    // smooth them out.
    if ( NR::L2(force) < DYNA_EPSILON || (dc->vel_max < DYNA_VEL_START && NR::L2(force) < DYNA_EPSILON_START)) {
        return FALSE;
    }

    dc->acc = force / mass;

    /* Calculate new velocity */
    dc->vel += dc->acc;

    if (NR::L2(dc->vel) > dc->vel_max)
        dc->vel_max = NR::L2(dc->vel);

    /* Calculate angle of drawing tool */

    double a1;
    if (dc->usetilt) {
        // 1a. calculate nib angle from input device tilt:
        gdouble length = std::sqrt(dc->xtilt*dc->xtilt + dc->ytilt*dc->ytilt);;

        if (length > 0) {
            NR::Point ang1 = NR::Point(dc->ytilt/length, dc->xtilt/length);
            a1 = atan2(ang1);
        }
        else
            a1 = 0.0;
    }
    else {
        // 1b. fixed dc->angle (absolutely flat nib):
        double const radians = ( (dc->angle - 90) / 180.0 ) * M_PI;
        NR::Point ang1 = NR::Point(-sin(radians),  cos(radians));
        a1 = atan2(ang1);
    }

    // 2. perpendicular to dc->vel (absolutely non-flat nib):
    gdouble const mag_vel = NR::L2(dc->vel);
    if ( mag_vel < DYNA_EPSILON ) {
        return FALSE;
    }
    NR::Point ang2 = NR::rot90(dc->vel) / mag_vel;

    // 3. Average them using flatness parameter:
    // calculate angles
    double a2 = atan2(ang2);
    // flip a2 to force it to be in the same half-circle as a1
    bool flipped = false;
    if (fabs (a2-a1) > 0.5*M_PI) {
        a2 += M_PI;
        flipped = true;
    }
    // normalize a2
    if (a2 > M_PI)
        a2 -= 2*M_PI;
    if (a2 < -M_PI)
        a2 += 2*M_PI;
    // find the flatness-weighted bisector angle, unflip if a2 was flipped
    // FIXME: when dc->vel is oscillating around the fixed angle, the new_ang flips back and forth. How to avoid this?
    double new_ang = a1 + (1 - dc->flatness) * (a2 - a1) - (flipped? M_PI : 0);

    // Try to detect a sudden flip when the new angle differs too much from the previous for the
    // current velocity; in that case discard this move
    double angle_delta = NR::L2(NR::Point (cos (new_ang), sin (new_ang)) - dc->ang);
    if ( angle_delta / NR::L2(dc->vel) > 4000 ) {
        return FALSE;
    }

    // convert to point
    dc->ang = NR::Point (cos (new_ang), sin (new_ang));

//    g_print ("force %g  acc %g  vel_max %g  vel %g  a1 %g  a2 %g  new_ang %g\n", NR::L2(force), NR::L2(dc->acc), dc->vel_max, NR::L2(dc->vel), a1, a2, new_ang);

    /* Apply drag */
    dc->vel *= 1.0 - drag;

    /* Update position */
    dc->last = dc->cur;
    dc->cur += dc->vel;

    return TRUE;
}

static void
sp_dyna_draw_brush(SPDynaDrawContext *dc)
{
    g_assert( dc->npoints >= 0 && dc->npoints < SAMPLING_SIZE );

    // How much velocity thins strokestyle
    double vel_thin = flerp (0, 160, dc->vel_thin);

    // Influence of pressure on thickness
    double pressure_thick = (dc->usepressure ? dc->pressure : 1.0);

    // get the real brush point, not the same as pointer (affected by hatch tracking and/or mass
    // drag)
    NR::Point brush = sp_dyna_draw_get_vpoint(dc, dc->cur);
    NR::Point brush_w = SP_EVENT_CONTEXT(dc)->desktop->d2w(brush); 

    double trace_thick = 1;
    if (dc->trace_bg) {
        // pick single pixel
        NRPixBlock pb;
        int x = (int) floor(brush_w[NR::X]);
        int y = (int) floor(brush_w[NR::Y]);
        nr_pixblock_setup_fast(&pb, NR_PIXBLOCK_MODE_R8G8B8A8P, x, y, x+1, y+1, TRUE);
        sp_canvas_arena_render_pixblock(SP_CANVAS_ARENA(sp_desktop_drawing(SP_EVENT_CONTEXT(dc)->desktop)), &pb);
        const unsigned char *s = NR_PIXBLOCK_PX(&pb);
        double R = s[0] / 255.0;
        double G = s[1] / 255.0;
        double B = s[2] / 255.0;
        double A = s[3] / 255.0;
        double max = MAX (MAX (R, G), B);
        double min = MIN (MIN (R, G), B);
        double L = A * (max + min)/2 + (1 - A); // blend with white bg
        trace_thick = 1 - L;
        //g_print ("L %g thick %g\n", L, trace_thick);
    }

    double width = (pressure_thick * trace_thick - vel_thin * NR::L2(dc->vel)) * dc->width;

    double tremble_left = 0, tremble_right = 0;
    if (dc->tremor > 0) {
        // obtain two normally distributed random variables, using polar Box-Muller transform
        double x1, x2, w, y1, y2;
        do {
            x1 = 2.0 * g_random_double_range(0,1) - 1.0;
            x2 = 2.0 * g_random_double_range(0,1) - 1.0;
            w = x1 * x1 + x2 * x2;
        } while ( w >= 1.0 );
        w = sqrt( (-2.0 * log( w ) ) / w );
        y1 = x1 * w;
        y2 = x2 * w;

        // deflect both left and right edges randomly and independently, so that:
        // (1) dc->tremor=1 corresponds to sigma=1, decreasing dc->tremor narrows the bell curve;
        // (2) deflection depends on width, but is upped for small widths for better visual uniformity across widths;
        // (3) deflection somewhat depends on speed, to prevent fast strokes looking
        // comparatively smooth and slow ones excessively jittery
        tremble_left  = (y1)*dc->tremor * (0.15 + 0.8*width) * (0.35 + 14*NR::L2(dc->vel));
        tremble_right = (y2)*dc->tremor * (0.15 + 0.8*width) * (0.35 + 14*NR::L2(dc->vel));
    }

    if ( width < 0.02 * dc->width ) {
        width = 0.02 * dc->width;
    }

    double dezoomify_factor = 0.05 * 1000;
    if (!dc->abs_width) {
        dezoomify_factor /= SP_EVENT_CONTEXT(dc)->desktop->current_zoom();
    }

    NR::Point del_left = dezoomify_factor * (width + tremble_left) * dc->ang;
    NR::Point del_right = dezoomify_factor * (width + tremble_right) * dc->ang;

    dc->point1[dc->npoints] = brush + del_left;
    dc->point2[dc->npoints] = brush - del_right;

    dc->del = 0.5*(del_left + del_right);

    dc->npoints++;
}

double
get_dilate_radius (SPDynaDrawContext *dc)
{
    // 10 times the pen width:
    return 500 * dc->width/SP_EVENT_CONTEXT(dc)->desktop->current_zoom(); 
}

double
get_dilate_force (SPDynaDrawContext *dc)
{
    return 8 * dc->pressure/SP_EVENT_CONTEXT(dc)->desktop->current_zoom(); 
}

bool
sp_ddc_dilate (SPDynaDrawContext *dc, NR::Point p, bool expand)
{
    Inkscape::Selection *selection = sp_desktop_selection(SP_EVENT_CONTEXT(dc)->desktop);

    if (selection->isEmpty()) {
        return false;
    }

    bool did = false;
    double radius = get_dilate_radius(dc); 
    double offset = get_dilate_force(dc); 

    for (GSList *items = g_slist_copy((GSList *) selection->itemList());
         items != NULL;
         items = items->next) {

        SPItem *item = (SPItem *) items->data;

        // only paths
        if (!SP_IS_PATH(item))
            continue;

        SPCurve *curve = NULL;
        curve = sp_shape_get_curve(SP_SHAPE(item));
        if (curve == NULL)
            continue;

        // skip those paths whose bboxes are entirely out of reach with our radius
        NR::Maybe<NR::Rect> bbox = item->getBounds(sp_item_i2doc_affine(item));
        if (bbox) {
            bbox->growBy(radius);
            if (!bbox->contains(p)) {
                continue;
            }
        }

        Path *orig = Path_for_item(item, false);
        if (orig == NULL) {
            sp_curve_unref(curve);
            continue;
        }
        Path *res = new Path;
        res->SetBackData(false);

        Shape *theShape = new Shape;
        Shape *theRes = new Shape;

        orig->ConvertWithBackData(0.05);
        orig->Fill(theShape, 0);

        SPCSSAttr *css = sp_repr_css_attr(SP_OBJECT_REPR(item), "style");
        gchar const *val = sp_repr_css_property(css, "fill-rule", NULL);
        if (val && strcmp(val, "nonzero") == 0)
        {
            theRes->ConvertToShape(theShape, fill_nonZero);
        }
        else if (val && strcmp(val, "evenodd") == 0)
        {
            theRes->ConvertToShape(theShape, fill_oddEven);
        }
        else
        {
            theRes->ConvertToShape(theShape, fill_nonZero);
        }

        bool did_this = false;
        if (theShape->MakeOffset(theRes, 
                                  expand? offset : -offset,
                                  join_straight, butt_straight,
                                  true, p[NR::X], p[NR::Y], radius) == 0) // 0 means the shape was actually changed
            did_this = true;

        // the rest only makes sense if we actually changed the path
        if (did_this) {
            theRes->ConvertToShape(theShape, fill_positive);

            res->Reset();
            theRes->ConvertToForme(res);

            if (offset >= 0.5)
            {
                res->ConvertEvenLines(0.5);
                res->Simplify(0.5);
            }
            else
            {
                res->ConvertEvenLines(0.5*offset);
                res->Simplify(0.5 * offset);
            }

            sp_curve_unref(curve);
            if (res->descr_cmd.size() > 1) { 
                gchar *str = res->svg_dump_path();
                SP_OBJECT_REPR(item)->setAttribute("d", str);
                g_free(str);
            } else {
                // TODO: if there's 0 or 1 node left, delete this path altogether
            }
        }

        delete theShape;
        delete theRes;
        delete orig;
        delete res;

        if (did_this) 
            did = true;
    }

    return did;
}

void
sp_ddc_update_cursors (SPDynaDrawContext *dc)
{
    if (dc->is_dilating) {
        double radius = get_dilate_radius(dc);
        NR::Matrix const sm (NR::scale(radius, radius) * NR::translate(SP_EVENT_CONTEXT(dc)->desktop->point()));
        sp_canvas_item_affine_absolute(dc->dilate_area, sm);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0xff9900ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        sp_canvas_item_show(dc->dilate_area);
    }
}

void
sp_ddc_update_toolbox (SPDesktop *desktop, const gchar *id, double value)
{
    desktop->setToolboxAdjustmentValue (id, value);
}

static void
calligraphic_cancel(SPDynaDrawContext *dc)
{
    SPDesktop *desktop = SP_EVENT_CONTEXT(dc)->desktop;
    dc->dragging = FALSE;
    dc->is_drawing = false;
    sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), 0);
            /* Remove all temporary line segments */
            while (dc->segments) {
                gtk_object_destroy(GTK_OBJECT(dc->segments->data));
                dc->segments = g_slist_remove(dc->segments, dc->segments->data);
            }
            /* reset accumulated curve */
            sp_curve_reset(dc->accumulated);
            clear_current(dc);
            if (dc->repr) {
                dc->repr = NULL;
            }
}


gint
sp_dyna_draw_context_root_handler(SPEventContext *event_context,
                                  GdkEvent *event)
{
    SPDynaDrawContext *dc = SP_DYNA_DRAW_CONTEXT(event_context);
    SPDesktop *desktop = event_context->desktop;

    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            if ( event->button.button == 1 ) {

                SPDesktop *desktop = SP_EVENT_CONTEXT_DESKTOP(dc);

                if (Inkscape::have_viable_layer(desktop, dc->_message_context) == false) {
                    return TRUE;
                }

                NR::Point const button_w(event->button.x,
                                         event->button.y);
                NR::Point const button_dt(desktop->w2d(button_w));
                sp_dyna_draw_reset(dc, button_dt);
                sp_dyna_draw_extinput(dc, event);
                sp_dyna_draw_apply(dc, button_dt);
                sp_curve_reset(dc->accumulated);
                if (dc->repr) {
                    dc->repr = NULL;
                }

                /* initialize first point */
                dc->npoints = 0;

                sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                    ( GDK_KEY_PRESS_MASK |
                                      GDK_BUTTON_RELEASE_MASK |
                                      GDK_POINTER_MOTION_MASK |
                                      GDK_BUTTON_PRESS_MASK ),
                                    NULL,
                                    event->button.time);

                ret = TRUE;

                dc->is_drawing = true;
            }
            break;
        case GDK_MOTION_NOTIFY:
        {
            NR::Point const motion_w(event->motion.x,
                                     event->motion.y);
            NR::Point motion_dt(desktop->w2d(motion_w));

            // draw the dilating cursor
            if (event->motion.state & GDK_MOD1_MASK) {
                double radius = get_dilate_radius(dc);
                NR::Matrix const sm (NR::scale(radius, radius) * NR::translate(desktop->w2d(motion_w)));
                sp_canvas_item_affine_absolute(dc->dilate_area, sm);
                sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0xff9900ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
                sp_canvas_item_show(dc->dilate_area);

                guint num = 0;
                if (!desktop->selection->isEmpty()) {
                    num = g_slist_length((GSList *) desktop->selection->itemList());
                }
                if (num == 0) {
                    dc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Select paths</b> to thin or thicken"));
                } else {
                    dc->_message_context->setF(Inkscape::NORMAL_MESSAGE,
                                           event->motion.state & GDK_SHIFT_MASK?
                                           _("<b>Thickening %d</b> selected paths; without <b>Shift</b> to thin") :
                                           _("<b>Thinning %d</b> selected paths; with <b>Shift</b> to thicken"), num);
                }

            } else {
                dc->_message_context->clear();
                sp_canvas_item_hide(dc->dilate_area);
            }

            // dilating:
            if (dc->is_drawing && ( event->motion.state & GDK_BUTTON1_MASK ) && event->motion.state & GDK_MOD1_MASK) {  
                sp_ddc_dilate (dc, desktop->dt2doc(motion_dt), event->motion.state & GDK_SHIFT_MASK? true : false);
                dc->is_dilating = true;
                // it's slow, so prevent clogging up with events
                gobble_motion_events(GDK_BUTTON1_MASK);
                return TRUE;
            }


            // for hatching:
            double hatch_dist = 0;
            NR::Point hatch_unit_vector(0,0);
            NR::Point nearest(0,0);
            NR::Point pointer(0,0);
            NR::Matrix motion_to_curve(NR::identity());

            if (event->motion.state & GDK_CONTROL_MASK) { // hatching - sense the item

                SPItem *selected = sp_desktop_selection(desktop)->singleItem();
                if (selected && (SP_IS_SHAPE(selected) || SP_IS_TEXT(selected))) {
                    // One item selected, and it's a path;
                    // let's try to track it as a guide

                    if (selected != dc->hatch_item) {
                        dc->hatch_item = selected;
                        if (dc->hatch_livarot_path)
                            delete dc->hatch_livarot_path;
                        dc->hatch_livarot_path = Path_for_item (dc->hatch_item, true, true);
                        dc->hatch_livarot_path->ConvertWithBackData(0.01);
                    }

                    // calculate pointer point in the guide item's coords
                    motion_to_curve = sp_item_dt2i_affine(selected) * sp_item_i2doc_affine(selected);
                    pointer = motion_dt * motion_to_curve;

                    // calculate the nearest point on the guide path
                    NR::Maybe<Path::cut_position> position = get_nearest_position_on_Path(dc->hatch_livarot_path, pointer);
                    nearest = get_point_on_Path(dc->hatch_livarot_path, position->piece, position->t);


                    // distance from pointer to nearest
                    hatch_dist = NR::L2(pointer - nearest);
                    // unit-length vector
                    hatch_unit_vector = (pointer - nearest)/hatch_dist;

                    dc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Guide path selected</b>; start drawing along the guide with <b>Ctrl</b>"));
                } else {
                    dc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Select a guide path</b> to track with <b>Ctrl</b>"));
                }
            } 

            if ( dc->is_drawing && ( event->motion.state & GDK_BUTTON1_MASK ) ) {
                dc->dragging = TRUE;

                if (event->motion.state & GDK_CONTROL_MASK && dc->hatch_item) { // hatching

#define SPEED_ELEMENTS 12
#define SPEED_MIN 0.12
#define SPEED_NORMAL 0.65

                    // speed is the movement of the nearest point along the guide path, divided by
                    // the movement of the pointer at the same period; it is averaged for the last
                    // SPEED_ELEMENTS motion events.  Normally, as you track the guide path, speed
                    // is about 1, i.e. the nearest point on the path is moved by about the same
                    // distance as the pointer. If the speed starts to decrease, we are losing
                    // contact with the guide; if it drops below SPEED_MIN, we are on our own and
                    // not attracted to guide anymore. Most often this happens when you have
                    // tracked to the end of a guide calligraphic stroke and keep moving
                    // further. We try to handle this situation gracefully: not stick with the
                    // guide forever but let go of it smoothly and without sharp jerks (non-zero
                    // mass recommended; with zero mass, jerks are still quite noticeable).

                    double speed = 1;
                    if (NR::L2(dc->hatch_last_nearest) != 0) {
                        // the distance nearest moved since the last motion event
                        double nearest_moved = NR::L2(nearest - dc->hatch_last_nearest);
                        // the distance pointer moved since the last motion event
                        double pointer_moved = NR::L2(pointer - dc->hatch_last_pointer);
                        // store them in stacks limited to SPEED_ELEMENTS
                        dc->hatch_nearest_past.push_front(nearest_moved);
                        if (dc->hatch_nearest_past.size() > SPEED_ELEMENTS)
                            dc->hatch_nearest_past.pop_back();
                        dc->hatch_pointer_past.push_front(pointer_moved);
                        if (dc->hatch_pointer_past.size() > SPEED_ELEMENTS)
                            dc->hatch_pointer_past.pop_back();

                        // If the stacks are full,
                        if (dc->hatch_nearest_past.size() == SPEED_ELEMENTS) {
                            // calculate the sums of all stored movements
                            double nearest_sum = std::accumulate (dc->hatch_nearest_past.begin(), dc->hatch_nearest_past.end(), 0.0);
                            double pointer_sum = std::accumulate (dc->hatch_pointer_past.begin(), dc->hatch_pointer_past.end(), 0.0);
                            // and divide to get the speed
                            speed = nearest_sum/pointer_sum;
                            //g_print ("nearest sum %g  pointer_sum %g  speed %g\n", nearest_sum, pointer_sum, speed);
                        }
                    }

                    if (   dc->hatch_escaped  // already escaped, do not reattach
                        || (speed < SPEED_MIN) // stuck; most likely reached end of traced stroke
                        || (dc->hatch_spacing > 0 && hatch_dist > 50 * dc->hatch_spacing) // went too far from the guide
                        ) {
                        // We are NOT attracted to the guide!

                        //g_print ("\nlast_nearest %g %g   nearest %g %g  pointer %g %g  pos %d %g\n", dc->last_nearest[NR::X], dc->last_nearest[NR::Y], nearest[NR::X], nearest[NR::Y], pointer[NR::X], pointer[NR::Y], position->piece, position->t);
                        //g_print ("------nm %g  pm %g  ratio %g\n", nearest_moved, pointer_moved, ratio);
                        //g_print ("------dist %g  spacing %g\n", dist, dc->spacing);
                        //g_print ("acting up: %s dist %g \n", position_is_ok? (dc->hatch_escaped? "escaped" : "dist") : "livarot", dist/dc->hatch_spacing);

                        // Remember hatch_escaped so we don't get
                        // attracted again until the end of this stroke
                        dc->hatch_escaped = true;

                    } else {

                        // Calculate angle cosine of this vector-to-guide and all past vectors
                        // summed, to detect if we accidentally flipped to the other side of the
                        // guide
                        double dot = NR::dot (pointer - nearest, dc->hatch_vector_accumulated);
                        dot /= NR::L2(pointer - nearest) * NR::L2(dc->hatch_vector_accumulated);

                        if (dc->hatch_spacing != 0) { // spacing was already set
                            double target;
                            if (speed > SPEED_NORMAL) {
                                // all ok, strictly obey the spacing
                                target = dc->hatch_spacing;
                            } else {
                                // looks like we're starting to lose speed,
                                // so _gradually_ let go attraction to prevent jerks
                                target = (dc->hatch_spacing * speed + hatch_dist * (SPEED_NORMAL - speed))/SPEED_NORMAL;                            
                            }
                            if (!isNaN(dot) && dot < -0.5) {// flip
                                target = -target;
                                //g_print ("FLIP\n");
                            }

                            // This is the track pointer that we will use instead of the real one
                            NR::Point new_pointer = nearest + target * hatch_unit_vector;
                            //g_print ("NEW %g %g\n", new_point[NR::X], new_point[NR::Y]);

                            // some limited feedback: allow persistent pulling to slightly change
                            // the spacing
                            dc->hatch_spacing += (hatch_dist - dc->hatch_spacing)/3500;

                            // return it to the desktop coords
                            motion_dt = new_pointer * motion_to_curve.inverse();

                        } else {
                            // this is the first motion event, set the dist 
                            dc->hatch_spacing = hatch_dist;
                        }

                        // remember last points
                        dc->hatch_last_pointer = pointer;
                        dc->hatch_last_nearest = nearest;
                        dc->hatch_vector_accumulated += (pointer - nearest);
                    }

                    dc->_message_context->set(Inkscape::NORMAL_MESSAGE, dc->hatch_escaped? _("Tracking: <b>connection to guide path lost!</b>") : _("<b>Tracking</b> a guide path"));

                } else {
                    dc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Drawing</b> a calligraphic stroke"));
                }

                sp_dyna_draw_extinput(dc, event);
                if (!sp_dyna_draw_apply(dc, motion_dt)) {
                    ret = TRUE;
                    break;
                }

                if ( dc->cur != dc->last ) {
                    sp_dyna_draw_brush(dc);
                    g_assert( dc->npoints > 0 );
                    fit_and_split(dc, FALSE);
                }
                ret = TRUE;
            }

            // Draw the hatching circle if necessary
            if (event->motion.state & GDK_CONTROL_MASK) { 
                if (dc->hatch_spacing == 0 && hatch_dist != 0) { 
                    // Haven't set spacing yet: gray, center free, update radius live
                    NR::Point c = desktop->w2d(motion_w);
                    NR::Matrix const sm (NR::scale(hatch_dist, hatch_dist) * NR::translate(c));
                    sp_canvas_item_affine_absolute(dc->hatch_area, sm);
                    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0x7f7f7fff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
                    sp_canvas_item_show(dc->hatch_area);
                } else if (dc->dragging && !dc->hatch_escaped) {
                    // Tracking: green, center snapped, fixed radius
                    NR::Point c = motion_dt;
                    NR::Matrix const sm (NR::scale(dc->hatch_spacing, dc->hatch_spacing) * NR::translate(c));
                    sp_canvas_item_affine_absolute(dc->hatch_area, sm);
                    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0x00FF00ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
                    sp_canvas_item_show(dc->hatch_area);
                } else if (dc->dragging && dc->hatch_escaped) {
                    // Tracking escaped: red, center free, fixed radius
                    NR::Point c = desktop->w2d(motion_w);
                    NR::Matrix const sm (NR::scale(dc->hatch_spacing, dc->hatch_spacing) * NR::translate(c));

                    sp_canvas_item_affine_absolute(dc->hatch_area, sm);
                    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0xFF0000ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
                    sp_canvas_item_show(dc->hatch_area);
                } else {
                    // Not drawing but spacing set: gray, center snapped, fixed radius
                    NR::Point c = (nearest + dc->hatch_spacing * hatch_unit_vector) * motion_to_curve.inverse();
                    if (!isNaN(c[NR::X]) && !isNaN(c[NR::Y])) {
                        NR::Matrix const sm (NR::scale(dc->hatch_spacing, dc->hatch_spacing) * NR::translate(c));
                        sp_canvas_item_affine_absolute(dc->hatch_area, sm);
                        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(dc->hatch_area), 0x7f7f7fff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
                        sp_canvas_item_show(dc->hatch_area);
                    }
                }
            } else {
                sp_canvas_item_hide(dc->hatch_area);
            }
        }
        break;


    case GDK_BUTTON_RELEASE:
        sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), event->button.time);
        dc->is_drawing = false;

        if ( dc->is_dilating && event->button.button == 1 ) {
            dc->is_dilating = false;
            sp_document_done(sp_desktop_document(SP_EVENT_CONTEXT(dc)->desktop), 
                         SP_VERB_CONTEXT_CALLIGRAPHIC,
                         (event->button.state & GDK_SHIFT_MASK ? _("Thicken paths") : _("Thin paths")));
            ret = TRUE;
        } else if ( dc->dragging && event->button.button == 1 ) {
            dc->dragging = FALSE;

            NR::Point const motion_w(event->button.x, event->button.y);
            NR::Point const motion_dt(desktop->w2d(motion_w));
            sp_dyna_draw_apply(dc, motion_dt);

            /* Remove all temporary line segments */
            while (dc->segments) {
                gtk_object_destroy(GTK_OBJECT(dc->segments->data));
                dc->segments = g_slist_remove(dc->segments, dc->segments->data);
            }

            /* Create object */
            fit_and_split(dc, TRUE);
            accumulate_calligraphic(dc);
            set_to_accumulated(dc, event->button.state & GDK_SHIFT_MASK); // performs document_done

            /* reset accumulated curve */
            sp_curve_reset(dc->accumulated);

            clear_current(dc);
            if (dc->repr) {
                dc->repr = NULL;
            }

            if (!dc->hatch_pointer_past.empty()) dc->hatch_pointer_past.clear();
            if (!dc->hatch_nearest_past.empty()) dc->hatch_nearest_past.clear();
            dc->hatch_last_nearest = NR::Point(0,0);
            dc->hatch_last_pointer = NR::Point(0,0);
            dc->hatch_vector_accumulated = NR::Point(0,0);
            dc->hatch_escaped = false;
            dc->hatch_item = NULL;
            dc->hatch_livarot_path = NULL;

            dc->_message_context->clear();
            ret = TRUE;
        }
        break;

    case GDK_KEY_PRESS:
        switch (get_group0_keyval (&event->key)) {
        case GDK_Up:
        case GDK_KP_Up:
            if (!MOD__CTRL_ONLY) {
                dc->angle += 5.0;
                if (dc->angle > 90.0)
                    dc->angle = 90.0;
                sp_ddc_update_toolbox (desktop, "calligraphy-angle", dc->angle);
                ret = TRUE;
            }
            break;
        case GDK_Down:
        case GDK_KP_Down:
            if (!MOD__CTRL_ONLY) {
                dc->angle -= 5.0;
                if (dc->angle < -90.0)
                    dc->angle = -90.0;
                sp_ddc_update_toolbox (desktop, "calligraphy-angle", dc->angle);
                ret = TRUE;
            }
            break;
        case GDK_Right:
        case GDK_KP_Right:
            if (!MOD__CTRL_ONLY) {
                dc->width += 0.01;
                if (dc->width > 1.0)
                    dc->width = 1.0;
                sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width * 100); // the same spinbutton is for alt+x
                sp_ddc_update_cursors(dc);
                ret = TRUE;
            }
            break;
        case GDK_Left:
        case GDK_KP_Left:
            if (!MOD__CTRL_ONLY) {
                dc->width -= 0.01;
                if (dc->width < 0.01)
                    dc->width = 0.01;
                sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width * 100);
                sp_ddc_update_cursors(dc);
                ret = TRUE;
            }
            break;
        case GDK_Home:
        case GDK_KP_Home:
            dc->width = 0.01;
            sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width * 100);
            sp_ddc_update_cursors(dc);
            ret = TRUE;
            break;
        case GDK_End:
        case GDK_KP_End:
            dc->width = 1.0;
            sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width * 100);
            sp_ddc_update_cursors(dc);
            ret = TRUE;
            break;
        case GDK_x:
        case GDK_X:
            if (MOD__ALT_ONLY) {
                desktop->setToolboxFocusTo ("altx-calligraphy");
                ret = TRUE;
            }
            break;
        case GDK_Escape:
            if (dc->is_drawing) {
                // if drawing, cancel, otherwise pass it up for deselecting
                calligraphic_cancel (dc);
                ret = TRUE;
            }
            break;
        case GDK_z:
        case GDK_Z:
            if (MOD__CTRL_ONLY && dc->is_drawing) {
                // if drawing, cancel, otherwise pass it up for undo
                calligraphic_cancel (dc);
                ret = TRUE;
            }
            break;
        case GDK_Meta_L:
        case GDK_Meta_R:
            event_context->cursor_shape = cursor_thicken_xpm;
            sp_event_context_update_cursor(event_context);
            break;
        case GDK_Alt_L:
        case GDK_Alt_R:
            if (MOD__SHIFT) {
                event_context->cursor_shape = cursor_thicken_xpm;
                sp_event_context_update_cursor(event_context);
            } else {
                event_context->cursor_shape = cursor_thin_xpm;
                sp_event_context_update_cursor(event_context);
            }
            break;
        case GDK_Shift_L:
        case GDK_Shift_R:
            if (MOD__ALT) {
                event_context->cursor_shape = cursor_thicken_xpm;
                sp_event_context_update_cursor(event_context);
            }
            break;
        default:
            break;
        }
        break;

    case GDK_KEY_RELEASE:
        switch (get_group0_keyval(&event->key)) {
            case GDK_Control_L:
            case GDK_Control_R:
                dc->_message_context->clear();
                dc->hatch_spacing = 0;
                break;
            case GDK_Alt_L:
            case GDK_Alt_R:
                event_context->cursor_shape = cursor_calligraphy_xpm;
                sp_event_context_update_cursor(event_context);
                break;
            case GDK_Shift_L:
            case GDK_Shift_R:
                if (MOD__ALT) {
                    event_context->cursor_shape = cursor_thin_xpm;
                    sp_event_context_update_cursor(event_context);
                }
                break;
            case GDK_Meta_L:
            case GDK_Meta_R:
                event_context->cursor_shape = cursor_calligraphy_xpm;
                sp_event_context_update_cursor(event_context);
            break;
            default:
                break;
        }

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


static void
clear_current(SPDynaDrawContext *dc)
{
    /* reset bpath */
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), NULL);
    /* reset curve */
    sp_curve_reset(dc->currentcurve);
    sp_curve_reset(dc->cal1);
    sp_curve_reset(dc->cal2);
    /* reset points */
    dc->npoints = 0;
}

static void
set_to_accumulated(SPDynaDrawContext *dc, bool unionize)
{
    SPDesktop *desktop = SP_EVENT_CONTEXT(dc)->desktop;

    if (!sp_curve_empty(dc->accumulated)) {
        NArtBpath *abp;
        gchar *str;

        if (!dc->repr) {
            /* Create object */
            Inkscape::XML::Document *xml_doc = sp_document_repr_doc(desktop->doc());
            Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

            /* Set style */
            sp_desktop_apply_style_tool (desktop, repr, "tools.calligraphic", false);

            dc->repr = repr;

            SPItem *item=SP_ITEM(desktop->currentLayer()->appendChildRepr(dc->repr));
            Inkscape::GC::release(dc->repr);
            item->transform = SP_ITEM(desktop->currentRoot())->getRelativeTransform(desktop->currentLayer());
            item->updateRepr();
        }
        abp = nr_artpath_affine(sp_curve_first_bpath(dc->accumulated), sp_desktop_dt2root_affine(desktop));
        str = sp_svg_write_path(abp);
        g_assert( str != NULL );
        g_free(abp);
        dc->repr->setAttribute("d", str);
        g_free(str);

        if (unionize) {
            sp_desktop_selection(desktop)->add(dc->repr);
            sp_selected_path_union_skip_undo();
        } else {
            if (dc->keep_selected) {
                sp_desktop_selection(desktop)->set(dc->repr);
            } else {
                sp_desktop_selection(desktop)->clear();
            }
        }

    } else {
        if (dc->repr) {
            sp_repr_unparent(dc->repr);
        }
        dc->repr = NULL;
    }

    sp_document_done(sp_desktop_document(desktop), SP_VERB_CONTEXT_CALLIGRAPHIC, 
                     _("Draw calligraphic stroke"));
}

static void
add_cap(SPCurve *curve,
        NR::Point const &pre, NR::Point const &from,
        NR::Point const &to, NR::Point const &post,
        double rounding)
{
    NR::Point vel = rounding * NR::rot90( to - from ) / sqrt(2.0);
    double mag = NR::L2(vel);

    NR::Point v_in = from - pre;
    double mag_in = NR::L2(v_in);
    if ( mag_in > DYNA_EPSILON ) {
        v_in = mag * v_in / mag_in;
    } else {
        v_in = NR::Point(0, 0);
    }

    NR::Point v_out = to - post;
    double mag_out = NR::L2(v_out);
    if ( mag_out > DYNA_EPSILON ) {
        v_out = mag * v_out / mag_out;
    } else {
        v_out = NR::Point(0, 0);
    }

    if ( NR::L2(v_in) > DYNA_EPSILON || NR::L2(v_out) > DYNA_EPSILON ) {
        sp_curve_curveto(curve, from + v_in, to + v_out, to);
    }
}

static void
accumulate_calligraphic(SPDynaDrawContext *dc)
{
    if ( !sp_curve_empty(dc->cal1) && !sp_curve_empty(dc->cal2) ) {
        sp_curve_reset(dc->accumulated); /*  Is this required ?? */
        SPCurve *rev_cal2 = sp_curve_reverse(dc->cal2);

        g_assert(dc->cal1->end > 1);
        g_assert(rev_cal2->end > 1);
        g_assert(SP_CURVE_SEGMENT(dc->cal1, 0)->code == NR_MOVETO_OPEN);
        g_assert(SP_CURVE_SEGMENT(rev_cal2, 0)->code == NR_MOVETO_OPEN);
        g_assert(SP_CURVE_SEGMENT(dc->cal1, 1)->code == NR_CURVETO);
        g_assert(SP_CURVE_SEGMENT(rev_cal2, 1)->code == NR_CURVETO);
        g_assert(SP_CURVE_SEGMENT(dc->cal1, dc->cal1->end-1)->code == NR_CURVETO);
        g_assert(SP_CURVE_SEGMENT(rev_cal2, rev_cal2->end-1)->code == NR_CURVETO);

        sp_curve_append(dc->accumulated, dc->cal1, FALSE);

        add_cap(dc->accumulated, SP_CURVE_SEGMENT(dc->cal1, dc->cal1->end-1)->c(2), SP_CURVE_SEGMENT(dc->cal1, dc->cal1->end-1)->c(3), SP_CURVE_SEGMENT(rev_cal2, 0)->c(3), SP_CURVE_SEGMENT(rev_cal2, 1)->c(1), dc->cap_rounding);

        sp_curve_append(dc->accumulated, rev_cal2, TRUE);

        add_cap(dc->accumulated, SP_CURVE_SEGMENT(rev_cal2, rev_cal2->end-1)->c(2), SP_CURVE_SEGMENT(rev_cal2, rev_cal2->end-1)->c(3), SP_CURVE_SEGMENT(dc->cal1, 0)->c(3), SP_CURVE_SEGMENT(dc->cal1, 1)->c(1), dc->cap_rounding);

        sp_curve_closepath(dc->accumulated);

        sp_curve_unref(rev_cal2);

        sp_curve_reset(dc->cal1);
        sp_curve_reset(dc->cal2);
    }
}

static double square(double const x)
{
    return x * x;
}

static void
fit_and_split(SPDynaDrawContext *dc, gboolean release)
{
    double const tolerance_sq = square( NR::expansion(SP_EVENT_CONTEXT(dc)->desktop->w2d()) * TOLERANCE_CALLIGRAPHIC );

#ifdef DYNA_DRAW_VERBOSE
    g_print("[F&S:R=%c]", release?'T':'F');
#endif

    if (!( dc->npoints > 0 && dc->npoints < SAMPLING_SIZE ))
        return; // just clicked

    if ( dc->npoints == SAMPLING_SIZE - 1 || release ) {
#define BEZIER_SIZE       4
#define BEZIER_MAX_BEZIERS  8
#define BEZIER_MAX_LENGTH ( BEZIER_SIZE * BEZIER_MAX_BEZIERS )

#ifdef DYNA_DRAW_VERBOSE
        g_print("[F&S:#] dc->npoints:%d, release:%s\n",
                dc->npoints, release ? "TRUE" : "FALSE");
#endif

        /* Current calligraphic */
        if ( dc->cal1->end == 0 || dc->cal2->end == 0 ) {
            /* dc->npoints > 0 */
            /* g_print("calligraphics(1|2) reset\n"); */
            sp_curve_reset(dc->cal1);
            sp_curve_reset(dc->cal2);

            sp_curve_moveto(dc->cal1, dc->point1[0]);
            sp_curve_moveto(dc->cal2, dc->point2[0]);
        }

        NR::Point b1[BEZIER_MAX_LENGTH];
        gint const nb1 = sp_bezier_fit_cubic_r(b1, dc->point1, dc->npoints,
                                               tolerance_sq, BEZIER_MAX_BEZIERS);
        g_assert( nb1 * BEZIER_SIZE <= gint(G_N_ELEMENTS(b1)) );

        NR::Point b2[BEZIER_MAX_LENGTH];
        gint const nb2 = sp_bezier_fit_cubic_r(b2, dc->point2, dc->npoints,
                                               tolerance_sq, BEZIER_MAX_BEZIERS);
        g_assert( nb2 * BEZIER_SIZE <= gint(G_N_ELEMENTS(b2)) );

        if ( nb1 != -1 && nb2 != -1 ) {
            /* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
            g_print("nb1:%d nb2:%d\n", nb1, nb2);
#endif
            /* CanvasShape */
            if (! release) {
                sp_curve_reset(dc->currentcurve);
                sp_curve_moveto(dc->currentcurve, b1[0]);
                for (NR::Point *bp1 = b1; bp1 < b1 + BEZIER_SIZE * nb1; bp1 += BEZIER_SIZE) {
                    sp_curve_curveto(dc->currentcurve, bp1[1],
                                     bp1[2], bp1[3]);
                }
                sp_curve_lineto(dc->currentcurve,
                                b2[BEZIER_SIZE*(nb2-1) + 3]);
                for (NR::Point *bp2 = b2 + BEZIER_SIZE * ( nb2 - 1 ); bp2 >= b2; bp2 -= BEZIER_SIZE) {
                    sp_curve_curveto(dc->currentcurve, bp2[2], bp2[1], bp2[0]);
                }
                // FIXME: dc->segments is always NULL at this point??
                if (!dc->segments) { // first segment
                    add_cap(dc->currentcurve, b2[1], b2[0], b1[0], b1[1], dc->cap_rounding);
                }
                sp_curve_closepath(dc->currentcurve);
                sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), dc->currentcurve);
            }

            /* Current calligraphic */
            for (NR::Point *bp1 = b1; bp1 < b1 + BEZIER_SIZE * nb1; bp1 += BEZIER_SIZE) {
                sp_curve_curveto(dc->cal1, bp1[1], bp1[2], bp1[3]);
            }
            for (NR::Point *bp2 = b2; bp2 < b2 + BEZIER_SIZE * nb2; bp2 += BEZIER_SIZE) {
                sp_curve_curveto(dc->cal2, bp2[1], bp2[2], bp2[3]);
            }
        } else {
            /* fixme: ??? */
#ifdef DYNA_DRAW_VERBOSE
            g_print("[fit_and_split] failed to fit-cubic.\n");
#endif
            draw_temporary_box(dc);

            for (gint i = 1; i < dc->npoints; i++) {
                sp_curve_lineto(dc->cal1, dc->point1[i]);
            }
            for (gint i = 1; i < dc->npoints; i++) {
                sp_curve_lineto(dc->cal2, dc->point2[i]);
            }
        }

        /* Fit and draw and copy last point */
#ifdef DYNA_DRAW_VERBOSE
        g_print("[%d]Yup\n", dc->npoints);
#endif
        if (!release) {
            g_assert(!sp_curve_empty(dc->currentcurve));

            SPCanvasItem *cbp = sp_canvas_item_new(sp_desktop_sketch(SP_EVENT_CONTEXT(dc)->desktop),
                                                   SP_TYPE_CANVAS_BPATH,
                                                   NULL);
            SPCurve *curve = sp_curve_copy(dc->currentcurve);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH (cbp), curve);
            sp_curve_unref(curve);

            guint32 fillColor = sp_desktop_get_color_tool (SP_ACTIVE_DESKTOP, "tools.calligraphic", true);
            //guint32 strokeColor = sp_desktop_get_color_tool (SP_ACTIVE_DESKTOP, "tools.calligraphic", false);
            double opacity = sp_desktop_get_master_opacity_tool (SP_ACTIVE_DESKTOP, "tools.calligraphic");
            double fillOpacity = sp_desktop_get_opacity_tool (SP_ACTIVE_DESKTOP, "tools.calligraphic", true);
            //double strokeOpacity = sp_desktop_get_opacity_tool (SP_ACTIVE_DESKTOP, "tools.calligraphic", false);
            sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cbp), ((fillColor & 0xffffff00) | SP_COLOR_F_TO_U(opacity*fillOpacity)), SP_WIND_RULE_EVENODD);
            //on second thougtht don't do stroke yet because we don't have stoke-width yet and because stoke appears between segments while drawing
            //sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cbp), ((strokeColor & 0xffffff00) | SP_COLOR_F_TO_U(opacity*strokeOpacity)), 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
            sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cbp), 0x00000000, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
            /* fixme: Cannot we cascade it to root more clearly? */
            g_signal_connect(G_OBJECT(cbp), "event", G_CALLBACK(sp_desktop_root_handler), SP_EVENT_CONTEXT(dc)->desktop);

            dc->segments = g_slist_prepend(dc->segments, cbp);
        }

        dc->point1[0] = dc->point1[dc->npoints - 1];
        dc->point2[0] = dc->point2[dc->npoints - 1];
        dc->npoints = 1;
    } else {
        draw_temporary_box(dc);
    }
}

static void
draw_temporary_box(SPDynaDrawContext *dc)
{
    sp_curve_reset(dc->currentcurve);

    sp_curve_moveto(dc->currentcurve, dc->point1[dc->npoints-1]);
    for (gint i = dc->npoints-2; i >= 0; i--) {
        sp_curve_lineto(dc->currentcurve, dc->point1[i]);
    }
    for (gint i = 0; i < dc->npoints; i++) {
        sp_curve_lineto(dc->currentcurve, dc->point2[i]);
    }
    if (dc->npoints >= 2) {
        add_cap(dc->currentcurve, dc->point2[dc->npoints-2], dc->point2[dc->npoints-1], dc->point1[dc->npoints-1], dc->point1[dc->npoints-2], dc->cap_rounding);
    }

    sp_curve_closepath(dc->currentcurve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), dc->currentcurve);
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
