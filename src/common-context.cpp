
#include "common-context.h"

#include <gtk/gtk.h>

#include "config.h"

#include "message-context.h"
#include "streq.h"
#include "preferences.h"
#include "display/sp-canvas-item.h"

#define MIN_PRESSURE      0.0
#define MAX_PRESSURE      1.0
#define DEFAULT_PRESSURE  1.0

#define DRAG_MIN 0.0
#define DRAG_DEFAULT 1.0
#define DRAG_MAX 1.0


static void sp_common_context_dispose(GObject *object);

static void sp_common_context_setup(SPEventContext *ec);
static void sp_common_context_set(SPEventContext *ec, Inkscape::Preferences::Entry *val);

static gint sp_common_context_root_handler(SPEventContext *event_context, GdkEvent *event);

G_DEFINE_TYPE(SPCommonContext, sp_common_context, SP_TYPE_EVENT_CONTEXT);

static void sp_common_context_class_init(SPCommonContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = SP_EVENT_CONTEXT_CLASS(klass);

    object_class->dispose = sp_common_context_dispose;

//    event_context_class->setup = sp_common_context_setup;
//    event_context_class->set = sp_common_context_set;
//    event_context_class->root_handler = sp_common_context_root_handler;
}

CCommonContext::CCommonContext(SPCommonContext* commoncontext) : CEventContext(commoncontext) {
	this->spcommoncontext = commoncontext;
}

SPCommonContext::SPCommonContext() : SPEventContext() {
	SPCommonContext* ctx = this;

	//ctx->ccommoncontext = new CCommonContext(ctx);
	//delete ctx->ceventcontext;
	//ctx->ceventcontext = ctx->ccommoncontext;
	ctx->ccommoncontext = 0;
	types.insert(typeid(SPCommonContext));

	ctx->_message_context = 0;
	ctx->tremor = 0;
	ctx->usetilt = 0;
	ctx->is_drawing = false;
	ctx->xtilt = 0;
	ctx->ytilt = 0;
	ctx->usepressure = 0;

//     ctx->cursor_shape = cursor_eraser_xpm;
//     ctx->hot_x = 4;
//     ctx->hot_y = 4;

    ctx->accumulated = 0;
    ctx->segments = 0;
    ctx->currentcurve = 0;
    ctx->currentshape = 0;
    ctx->npoints = 0;
    ctx->cal1 = 0;
    ctx->cal2 = 0;
    ctx->repr = 0;

    /* Common values */
    ctx->cur = Geom::Point(0,0);
    ctx->last = Geom::Point(0,0);
    ctx->vel = Geom::Point(0,0);
    ctx->vel_max = 0;
    ctx->acc = Geom::Point(0,0);
    ctx->ang = Geom::Point(0,0);
    ctx->del = Geom::Point(0,0);

    /* attributes */
    ctx->dragging = FALSE;

    ctx->mass = 0.3;
    ctx->drag = DRAG_DEFAULT;
    ctx->angle = 30.0;
    ctx->width = 0.2;
    ctx->pressure = DEFAULT_PRESSURE;

    ctx->vel_thin = 0.1;
    ctx->flatness = 0.9;
    ctx->cap_rounding = 0.0;

    ctx->abs_width = false;
}

static void sp_common_context_init(SPCommonContext *ctx)
{
	new (ctx) SPCommonContext();
}

static void sp_common_context_dispose(GObject *object)
{
    SPCommonContext *ctx = SP_COMMON_CONTEXT(object);

    if (ctx->accumulated) {
        ctx->accumulated = ctx->accumulated->unref();
        ctx->accumulated = 0;
    }

    while (ctx->segments) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(ctx->segments->data));
        ctx->segments = g_slist_remove(ctx->segments, ctx->segments->data);
    }

    if (ctx->currentcurve) {
        ctx->currentcurve = ctx->currentcurve->unref();
        ctx->currentcurve = 0;
    }
    if (ctx->cal1) {
        ctx->cal1 = ctx->cal1->unref();
        ctx->cal1 = 0;
    }
    if (ctx->cal2) {
        ctx->cal2 = ctx->cal2->unref();
        ctx->cal2 = 0;
    }

    if (ctx->currentshape) {
        sp_canvas_item_destroy(ctx->currentshape);
        ctx->currentshape = 0;
    }

    if (ctx->_message_context) {
        delete ctx->_message_context;
        ctx->_message_context = 0;
    }

    G_OBJECT_CLASS(sp_common_context_parent_class)->dispose(object);
}


static void sp_common_context_setup(SPEventContext *ec)
{
	ec->ceventcontext->setup();
}

void CCommonContext::setup() {
//    if ( SP_EVENT_CONTEXT_CLASS(sp_common_context_parent_class)->setup ) {
//        SP_EVENT_CONTEXT_CLASS(sp_common_context_parent_class)->setup(ec);
//    }
	CEventContext::setup();
}

static void sp_common_context_set(SPEventContext *ec, Inkscape::Preferences::Entry *value)
{
	ec->ceventcontext->set(value);
}

void CCommonContext::set(Inkscape::Preferences::Entry* value) {
	SPEventContext* ec = this->speventcontext;

    SPCommonContext *ctx = SP_COMMON_CONTEXT(ec);
    Glib::ustring path = value->getEntryName();
    
    // ignore preset modifications
    static Glib::ustring const presets_path = ec->pref_observer->observed_path + "/preset";
    Glib::ustring const &full_path = value->getPath();
    if (full_path.compare(0, presets_path.size(), presets_path) == 0) return;

    if (path == "mass") {
        ctx->mass = 0.01 * CLAMP(value->getInt(10), 0, 100);
    } else if (path == "wiggle") {
        ctx->drag = CLAMP((1 - 0.01 * value->getInt()),
            DRAG_MIN, DRAG_MAX); // drag is inverse to wiggle
    } else if (path == "angle") {
        ctx->angle = CLAMP(value->getDouble(), -90, 90);
    } else if (path == "width") {
        ctx->width = 0.01 * CLAMP(value->getInt(10), 1, 100);
    } else if (path == "thinning") {
        ctx->vel_thin = 0.01 * CLAMP(value->getInt(10), -100, 100);
    } else if (path == "tremor") {
        ctx->tremor = 0.01 * CLAMP(value->getInt(), 0, 100);
    } else if (path == "flatness") {
        ctx->flatness = 0.01 * CLAMP(value->getInt(), 0, 100);
    } else if (path == "usepressure") {
        ctx->usepressure = value->getBool();
    } else if (path == "usetilt") {
        ctx->usetilt = value->getBool();
    } else if (path == "abs_width") {
        ctx->abs_width = value->getBool();
    } else if (path == "cap_rounding") {
        ctx->cap_rounding = value->getDouble();
    }
}

static gint sp_common_context_root_handler(SPEventContext *event_context, GdkEvent *event)
{
	return event_context->ceventcontext->root_handler(event);
}

gint CCommonContext::root_handler(GdkEvent* event) {
	SPEventContext* event_context = this->speventcontext;

    gint ret = FALSE;

    // TODO add common hanlding


    if ( !ret ) {
//        if ( SP_EVENT_CONTEXT_CLASS(sp_common_context_parent_class)->root_handler ) {
//            ret = SP_EVENT_CONTEXT_CLASS(sp_common_context_parent_class)->root_handler(event_context, event);
//        }
    	ret = CEventContext::root_handler(event);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
