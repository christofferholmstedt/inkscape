/**
 *  \file guide-snapper.cpp
 *  \brief Snapping things to guides.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "libnr/nr-values.h"
#include "libnr/nr-point-fns.h"
#include "sp-namedview.h"
#include "sp-guide.h"

Inkscape::GuideSnapper::GuideSnapper(SPNamedView const *nv, NR::Coord const d) : LineSnapper(nv, d)
{

}

Inkscape::GuideSnapper::LineList Inkscape::GuideSnapper::_getSnapLines(NR::Point const &/*p*/) const
{
    LineList s;

    if ( NULL == _named_view || ThisSnapperMightSnap() == false) {
        return s;
    }

    for (GSList const *l = _named_view->guides; l != NULL; l = l->next) {
        SPGuide const *g = SP_GUIDE(l->data);
        NR::Point point_on_line = (g->normal == component_vectors[NR::X]) ? NR::Point(g->position, 0) : NR::Point(0, g->position);
        s.push_back(std::make_pair(g->normal, point_on_line)); 
    }

    return s;
}

/**
 *  \return true if this Snapper will snap at least one kind of point.
 */
bool Inkscape::GuideSnapper::ThisSnapperMightSnap() const
{
    return _named_view == NULL ? false : (_enabled && _snap_from != 0 && _named_view->showguides);
}

void Inkscape::GuideSnapper::_addSnappedLine(SnappedConstraints &sc, NR::Point const snapped_point, NR::Coord const snapped_distance, NR::Point const normal_to_line, NR::Point const point_on_line) const
{
    SnappedLine dummy = SnappedLine(snapped_point, snapped_distance, normal_to_line, point_on_line);
    sc.guide_lines.push_back(dummy);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
