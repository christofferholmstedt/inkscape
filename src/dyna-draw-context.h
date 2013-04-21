#ifndef SP_DYNA_DRAW_CONTEXT_H_SEEN
#define SP_DYNA_DRAW_CONTEXT_H_SEEN

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

#include "common-context.h"
#include "splivarot.h"

#define SP_DYNA_DRAW_CONTEXT(obj) ((SPDynaDrawContext*)obj)
#define SP_IS_DYNA_DRAW_CONTEXT(obj) (dynamic_cast<const SPDynaDrawContext*>((const SPEventContext*)obj))


#define DDC_MIN_PRESSURE      0.0
#define DDC_MAX_PRESSURE      1.0
#define DDC_DEFAULT_PRESSURE  1.0

#define DDC_MIN_TILT         -1.0
#define DDC_MAX_TILT          1.0
#define DDC_DEFAULT_TILT      0.0

class SPDynaDrawContext : public SPCommonContext {
public:
	SPDynaDrawContext();
	virtual ~SPDynaDrawContext();

    /** newly created object remain selected */
    bool keep_selected;

    double hatch_spacing;
    double hatch_spacing_step;
    SPItem *hatch_item;
    Path *hatch_livarot_path;
    std::list<double> hatch_nearest_past;
    std::list<double> hatch_pointer_past;
    std::list<Geom::Point> inertia_vectors;
    Geom::Point hatch_last_nearest, hatch_last_pointer;
    std::list<Geom::Point> hatch_vectors;
    bool hatch_escaped;
    SPCanvasItem *hatch_area;
    bool just_started_drawing;
    bool trace_bg;

	static const std::string prefsPath;

	virtual void setup();
	virtual void set(const Inkscape::Preferences::Entry& val);
	virtual gint root_handler(GdkEvent* event);

	virtual const std::string& getPrefsPath();
};

#endif // SP_DYNA_DRAW_CONTEXT_H_SEEN

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
