#ifndef SEEN_PEN_CONTEXT_H
#define SEEN_PEN_CONTEXT_H

/** \file 
 * PenTool: a context for pen tool events.
 */

#include "ui/tools/freehand-base.h"
#include "live_effects/effect.h"

#define SP_PEN_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::PenTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_PEN_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::PenTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

struct SPCtrlLine;

namespace Inkscape {
namespace UI {
namespace Tools {

/**
 * PenTool: a context for pen tool events.
 */
class PenTool : public FreehandBase {
public:
	PenTool();
	PenTool(gchar const *const *cursor_shape, gint hot_x, gint hot_y);
	virtual ~PenTool();

	enum Mode {
	    MODE_CLICK,
	    MODE_DRAG
	};

	enum State {
	    POINT,
	    CONTROL,
	    CLOSE,
	    STOP
	};

    Geom::Point p[5];

    /** \invar npoints in {0, 2, 5}. */
    // npoints somehow determines the type of the node (what does it mean, exactly? the number of Bezier handles?)
    gint npoints;

    Mode mode;
    State state;

    bool polylines_only;
    bool polylines_paraxial;
    //spanish: propiedad que guarda si el modo Spiro está activo o no
    bool spiro;
    bool bspline;
    int num_clicks;

    unsigned int expecting_clicks_for_LPE; // if positive, finish the path after this many clicks
    Inkscape::LivePathEffect::Effect *waiting_LPE; // if NULL, waiting_LPE_type in SPDrawContext is taken into account
    SPLPEItem *waiting_item;

    SPCanvasItem *c0;
    SPCanvasItem *c1;

    SPCtrlLine *cl0;
    SPCtrlLine *cl1;
    
    bool events_disabled;

	static const std::string prefsPath;

	virtual const std::string& getPrefsPath();

protected:
	virtual void setup();
	virtual void finish();
	virtual void set(const Inkscape::Preferences::Entry& val);
	virtual bool root_handler(GdkEvent* event);
	virtual bool item_handler(SPItem* item, GdkEvent* event);
};

inline bool sp_pen_context_has_waiting_LPE(PenTool *pc) {
    // note: waiting_LPE_type is defined in SPDrawContext
    return (pc->waiting_LPE != NULL ||
            pc->waiting_LPE_type != Inkscape::LivePathEffect::INVALID_LPE);
}

void sp_pen_context_set_polyline_mode(PenTool *const pc);
void sp_pen_context_wait_for_LPE_mouse_clicks(PenTool *pc, Inkscape::LivePathEffect::EffectType effect_type,
                                              unsigned int num_clicks, bool use_polylines = true);
void sp_pen_context_cancel_waiting_for_LPE(PenTool *pc);
void sp_pen_context_put_into_waiting_mode(SPDesktop *desktop, Inkscape::LivePathEffect::EffectType effect_type,
                                          unsigned int num_clicks, bool use_polylines = true);

}
}
}

#endif /* !SEEN_PEN_CONTEXT_H */

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
