#define INKSCAPE_LPE_DOEFFECT_STACK_CPP

/*
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-test-doEffect-stack.h"

#include <2geom/piecewise.h>
#include <vector>
#include <libnr/n-art-bpath.h>

namespace Inkscape {
namespace LivePathEffect {


LPEdoEffectStackTest::LPEdoEffectStackTest(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    step(_("Stack step"), (""), "step", &wr, this)
{
    registerParameter( dynamic_cast<Parameter *>(&step) );
}

LPEdoEffectStackTest::~LPEdoEffectStackTest()
{

}

void
LPEdoEffectStackTest::doEffect (SPCurve * curve)
{
    if (step >= 1) {
        Effect::doEffect(curve);
    } else {
        // return here
        return;
    }
}

NArtBpath *
LPEdoEffectStackTest::doEffect_nartbpath (NArtBpath * path_in)
{
    if (step >= 2) {
        return Effect::doEffect_nartbpath(path_in);
    } else {
        // return here
        NArtBpath *path_out;

        unsigned ret = 0;
        while ( path_in[ret].code != NR_END ) {
            ++ret;
        }
        unsigned len = ++ret;

        path_out = g_new(NArtBpath, len);
        memcpy(path_out, path_in, len * sizeof(NArtBpath));
        return path_out;
    }
}

std::vector<Geom::Path>
LPEdoEffectStackTest::doEffect_path (std::vector<Geom::Path> & path_in)
{
    if (step >= 3) {
        return Effect::doEffect_path(path_in);
    } else {
        // return here
        std::vector<Geom::Path> path_out = path_in;
        return path_out;
    }
}

Geom::Piecewise<Geom::D2<Geom::SBasis> > 
LPEdoEffectStackTest::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > & pwd2_in)
{
    Geom::Piecewise<Geom::D2<Geom::SBasis> > output = pwd2_in;

    return output;
}


} // namespace LivePathEffect
} /* namespace Inkscape */

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
