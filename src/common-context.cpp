
#include "common-context.h"

#include <gtk/gtk.h>

#include "config.h"

#include "message-context.h"
#include "streq.h"
#include "preferences.h"
#include "display/sp-canvas-item.h"
#include "desktop.h"

#define MIN_PRESSURE      0.0
#define MAX_PRESSURE      1.0
#define DEFAULT_PRESSURE  1.0

#define DRAG_MIN 0.0
#define DRAG_DEFAULT 1.0
#define DRAG_MAX 1.0

SPCommonContext::SPCommonContext() : SPEventContext() {
	this->_message_context = 0;
	this->tremor = 0;
	this->usetilt = 0;
	this->is_drawing = false;
	this->xtilt = 0;
	this->ytilt = 0;
	this->usepressure = 0;

//     ctx->cursor_shape = cursor_eraser_xpm;
//     ctx->hot_x = 4;
//     ctx->hot_y = 4;

    this->accumulated = 0;
    this->segments = 0;
    this->currentcurve = 0;
    this->currentshape = 0;
    this->npoints = 0;
    this->cal1 = 0;
    this->cal2 = 0;
    this->repr = 0;

    /* Common values */
    this->cur = Geom::Point(0,0);
    this->last = Geom::Point(0,0);
    this->vel = Geom::Point(0,0);
    this->vel_max = 0;
    this->acc = Geom::Point(0,0);
    this->ang = Geom::Point(0,0);
    this->del = Geom::Point(0,0);

    /* attributes */
    this->dragging = FALSE;

    this->mass = 0.3;
    this->drag = DRAG_DEFAULT;
    this->angle = 30.0;
    this->width = 0.2;
    this->pressure = DEFAULT_PRESSURE;

    this->vel_thin = 0.1;
    this->flatness = 0.9;
    this->cap_rounding = 0.0;

    this->abs_width = false;
}

SPCommonContext::~SPCommonContext() {
    if (this->accumulated) {
        this->accumulated = this->accumulated->unref();
        this->accumulated = 0;
    }

    while (this->segments) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(this->segments->data));
        this->segments = g_slist_remove(this->segments, this->segments->data);
    }

    if (this->currentcurve) {
        this->currentcurve = this->currentcurve->unref();
        this->currentcurve = 0;
    }

    if (this->cal1) {
        this->cal1 = this->cal1->unref();
        this->cal1 = 0;
    }

    if (this->cal2) {
        this->cal2 = this->cal2->unref();
        this->cal2 = 0;
    }

    if (this->currentshape) {
        sp_canvas_item_destroy(this->currentshape);
        this->currentshape = 0;
    }

    if (this->_message_context) {
        delete this->_message_context;
        this->_message_context = 0;
    }

    //G_OBJECT_CLASS(sp_common_context_parent_class)->dispose(object);
}

void SPCommonContext::set(const Inkscape::Preferences::Entry& value) {
    Glib::ustring path = value.getEntryName();
    
    // ignore preset modifications
    static Glib::ustring const presets_path = this->pref_observer->observed_path + "/preset";
    Glib::ustring const &full_path = value.getPath();

    if (full_path.compare(0, presets_path.size(), presets_path) == 0) {
    	return;
    }

    if (path == "mass") {
        this->mass = 0.01 * CLAMP(value.getInt(10), 0, 100);
    } else if (path == "wiggle") {
        this->drag = CLAMP((1 - 0.01 * value.getInt()), DRAG_MIN, DRAG_MAX); // drag is inverse to wiggle
    } else if (path == "angle") {
        this->angle = CLAMP(value.getDouble(), -90, 90);
    } else if (path == "width") {
        this->width = 0.01 * CLAMP(value.getInt(10), 1, 100);
    } else if (path == "thinning") {
        this->vel_thin = 0.01 * CLAMP(value.getInt(10), -100, 100);
    } else if (path == "tremor") {
        this->tremor = 0.01 * CLAMP(value.getInt(), 0, 100);
    } else if (path == "flatness") {
        this->flatness = 0.01 * CLAMP(value.getInt(), 0, 100);
    } else if (path == "usepressure") {
        this->usepressure = value.getBool();
    } else if (path == "usetilt") {
        this->usetilt = value.getBool();
    } else if (path == "abs_width") {
        this->abs_width = value.getBool();
    } else if (path == "cap_rounding") {
        this->cap_rounding = value.getDouble();
    }
}

/* Get normalized point */
Geom::Point SPCommonContext::getNormalizedPoint(Geom::Point v) const {
    Geom::Rect drect = this->desktop->get_display_area();

    double const max = MAX ( drect.dimensions()[Geom::X], drect.dimensions()[Geom::Y] );

    return Geom::Point(( v[Geom::X] - drect.min()[Geom::X] ) / max,  ( v[Geom::Y] - drect.min()[Geom::Y] ) / max);
}

/* Get view point */
Geom::Point SPCommonContext::getViewPoint(Geom::Point n) const {
    Geom::Rect drect = this->desktop->get_display_area();

    double const max = MAX ( drect.dimensions()[Geom::X], drect.dimensions()[Geom::Y] );

    return Geom::Point(n[Geom::X] * max + drect.min()[Geom::X], n[Geom::Y] * max + drect.min()[Geom::Y]);
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
