/** @file
 * @brief PowerStroke LPE effect, see lpe-powerstroke.cpp.
 */
/* Authors:
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) 2010-2011 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef INKSCAPE_LPE_POWERSTROKE_H
#define INKSCAPE_LPE_POWERSTROKE_H

#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/powerstrokepointarray.h"
#include "live_effects/parameter/enum.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerStroke : public Effect {
public:
    LPEPowerStroke(LivePathEffectObject *lpeobject);
    virtual ~LPEPowerStroke();

    virtual std::vector<Geom::Path> doEffect_path (std::vector<Geom::Path> const & path_in);

    virtual void doOnApply(SPLPEItem *lpeitem);

private:
    PowerStrokePointArrayParam offset_points;
    BoolParam sort_points;
    EnumParam<unsigned> interpolator_type;
    EnumParam<unsigned> start_linecap_type;
    EnumParam<unsigned> cusp_linecap_type;
    EnumParam<unsigned> end_linecap_type;

    LPEPowerStroke(const LPEPowerStroke&);
    LPEPowerStroke& operator=(const LPEPowerStroke&);
};

} //namespace LivePathEffect
} //namespace Inkscape

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :