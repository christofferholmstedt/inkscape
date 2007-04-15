#ifndef __SP_DYNA_DRAW_CONTEXT_H__
#define __SP_DYNA_DRAW_CONTEXT_H__

/*
 * Handwriting-like drawing mode
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/curve.h"
#include "event-context.h"
#include <display/display-forward.h>
#include <libnr/nr-point.h>

#define SP_TYPE_DYNA_DRAW_CONTEXT (sp_dyna_draw_context_get_type())
#define SP_DYNA_DRAW_CONTEXT(o) (GTK_CHECK_CAST((o), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContext))
#define SP_DYNA_DRAW_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_CAST((k), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContextClass))
#define SP_IS_DYNA_DRAW_CONTEXT(o) (GTK_CHECK_TYPE((o), SP_TYPE_DYNA_DRAW_CONTEXT))
#define SP_IS_DYNA_DRAW_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_TYPE((k), SP_TYPE_DYNA_DRAW_CONTEXT))

class SPDynaDrawContext;
class SPDynaDrawContextClass;

#define SAMPLING_SIZE 8        /* fixme: ?? */

#define DDC_MIN_PRESSURE      0.0
#define DDC_MAX_PRESSURE      1.0
#define DDC_DEFAULT_PRESSURE  1.0

#define DDC_MIN_TILT         -1.0
#define DDC_MAX_TILT          1.0
#define DDC_DEFAULT_TILT      0.0

struct SPDynaDrawContext
{
    SPEventContext event_context;

    /** accumulated shape which ultimately goes in svg:path */
    SPCurve *accumulated;

    /** canvas items for "comitted" segments */
    GSList *segments;

    /** canvas item for red "leading" segment */
    SPCanvasItem *currentshape;
    /** shape of red "leading" segment */
    SPCurve *currentcurve;

    /** left edge of the stroke; combined to get accumulated */
    SPCurve *cal1;
    /** right edge of the stroke; combined to get accumulated */
    SPCurve *cal2;

    /** left edge points for this segment */
    NR::Point point1[SAMPLING_SIZE];
    /** right edge points for this segment */
    NR::Point point2[SAMPLING_SIZE];
    /** number of edge points for this segment */
    gint npoints;

    /* repr */
    Inkscape::XML::Node *repr;

    /* DynaDraw */
    NR::Point cur;
    NR::Point vel;
    double vel_max;
    NR::Point acc;
    NR::Point ang;
    NR::Point last;
    NR::Point del;
    /* extended input data */
    gdouble pressure;
    gdouble xtilt;
    gdouble ytilt;
    /* attributes */
    guint dragging : 1;           /* mouse state: mouse is dragging */
    guint usepressure : 1;
    guint usetilt : 1;
    double mass, drag;
    double angle;
    double width;

    double vel_thin;
    double flatness;
    double tremor;
    double cap_rounding;

    Inkscape::MessageContext *_message_context;

    bool is_drawing;

    /** uses absolute width independent of zoom */
    bool abs_width;

    /** newly created object remain selected */
    bool keep_selected;

    double hatch_spacing;
    double hatch_spacing_step;
    SPItem *hatch_item;
    Path *hatch_livarot_path;
    std::list<double> hatch_nearest_past;
    std::list<double> hatch_pointer_past;
    NR::Point hatch_last_nearest, hatch_last_pointer;
    NR::Point hatch_vector_accumulated;
    bool hatch_escaped;
    SPCanvasItem *hatch_area;

    bool trace_bg;

    bool is_dilating;
    SPCanvasItem *dilate_area;
};

struct SPDynaDrawContextClass
{
    SPEventContextClass parent_class;
};

GtkType sp_dyna_draw_context_get_type(void);

#endif

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
