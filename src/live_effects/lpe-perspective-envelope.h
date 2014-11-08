#ifndef INKSCAPE_LPE_PERSPECTIVE_ENVELOPE_H
#define INKSCAPE_LPE_PERSPECTIVE_ENVELOPE_H

/** \file
 * LPE <perspective-envelope> implementation , see lpe-perspective-envelope.cpp.
 
 */
/*
 * Authors:
 *   Jabiertxof Code migration from python extensions envelope and perspective
 *   Aaron Spike, aaron@ekips.org from envelope and perspective phyton code
 *   Dmitry Platonov, shadowjack@mail.ru, 2006 perspective approach & math
 *   Jose Hevia (freon) Transform algorithm from envelope
 *   
 * Copyright (C) 2007-2014 Authors 
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/pointreseteable.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPerspectiveEnvelope : public Effect, GroupBBoxEffect {
public:

    LPEPerspectiveEnvelope(LivePathEffectObject *lpeobject);

    virtual ~LPEPerspectiveEnvelope();

    virtual void doEffect(SPCurve *curve);

    virtual Geom::Point project_point(Geom::Point p);

    virtual Geom::Point project_point(Geom::Point p,  double m[][3]);

    virtual Geom::Point pointAtRatio(Geom::Coord ratio,Geom::Point A, Geom::Point B);

    virtual void resetDefaults(SPItem const* item);

    virtual void doBeforeEffect(SPLPEItem const* lpeitem);

    virtual Gtk::Widget * newWidget();

    virtual void setDefaults();

    virtual void resetGrid();

protected:
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec);
private:

    EnumParam<unsigned> deform_type;
    PointReseteableParam Up_Left_Point;
    PointReseteableParam Up_Right_Point;
    PointReseteableParam Down_Left_Point;
    PointReseteableParam Down_Right_Point;

    LPEPerspectiveEnvelope(const LPEPerspectiveEnvelope&);
    LPEPerspectiveEnvelope& operator=(const LPEPerspectiveEnvelope&);
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif