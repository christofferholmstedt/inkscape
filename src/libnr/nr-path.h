#ifndef __NR_PATH_H__
#define __NR_PATH_H__

/*
 * NArtBpath: Curve component.  Adapted from libart.
 */

/*
 * libart_lgpl/art_bpath.h copyright information:
 *
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <libnr/nr-forward.h>
#include <libnr/nr-coord.h>

NArtBpath* nr_artpath_affine(NArtBpath *s, NR::Matrix const &transform);

//ArtBpath* nr_artpath_to_art_bpath(NArtBpath *s); // this lives in src/extension/internal/gnome.cpp to avoid requiring libart everywhere

struct NRBPath {
	NArtBpath *path;
};

NRBPath *nr_path_duplicate_transform(NRBPath *d, NRBPath *s, NRMatrix const *transform);

NRBPath *nr_path_duplicate_transform(NRBPath *d, NRBPath *s, NR::Matrix const transform);

void nr_path_matrix_point_bbox_wind_distance (NRBPath *bpath, NR::Matrix const &m, NR::Point &pt,
					      NRRect *bbox, int *wind, NR::Coord *dist,
					      NR::Coord tolerance);

void nr_path_matrix_bbox_union(NRBPath const *bpath, NR::Matrix const &m, NRRect *bbox);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
