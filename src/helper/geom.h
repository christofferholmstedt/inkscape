#ifndef INKSCAPE_HELPER_GEOM_H
#define INKSCAPE_HELPER_GEOM_H

/**
 * @file
 * Specific geometry functions for Inkscape, not provided my lib2geom.
 */
/*
 * Author:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL
 */

#include <2geom/forward.h>
#include <2geom/rect.h>
#include <2geom/affine.h>

Geom::OptRect bounds_fast_transformed(Geom::PathVector const & pv, Geom::Affine const & t);
Geom::OptRect bounds_exact_transformed(Geom::PathVector const & pv, Geom::Affine const & t);

void pathv_matrix_point_bbox_wind_distance ( Geom::PathVector const & pathv, Geom::Affine const &m, Geom::Point const &pt,
                                             Geom::Rect *bbox, int *wind, Geom::Coord *dist,
                                             Geom::Coord tolerance, Geom::Rect const *viewbox);

Geom::PathVector pathv_to_linear_and_cubic_beziers( Geom::PathVector const &pathv );

void round_rectangle_outwards(Geom::Rect & rect);

namespace Geom{
bool transform_equalp(Geom::Affine const &m0, Geom::Affine const &m1, Geom::Coord const epsilon);
bool translate_equalp(Geom::Affine const &m0, Geom::Affine const &m1, Geom::Coord const epsilon);
bool matrix_equalp(Geom::Affine const &m0, Geom::Affine const &m1, Geom::Coord const epsilon);
}
#endif  // INKSCAPE_HELPER_GEOM_H

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
