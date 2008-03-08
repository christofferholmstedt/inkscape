#ifndef SEEN_LINE_SNAPPER_H
#define SEEN_LINE_SNAPPER_H

/**
 *    \file src/line-snapper.h
 *    \brief Superclass for snappers to horizontal and vertical lines.
 *
 *    Authors:
 *      Carl Hetherington <inkscape@carlh.net>
 *
 *    Released under GNU GPL, read the file 'COPYING' for more information.
 */

#include "snapper.h"

namespace Inkscape
{

class LineSnapper : public Snapper
{
public:
  LineSnapper(SPNamedView const *nv, NR::Coord const d);

protected:
  typedef std::list<std::pair<NR::Point, NR::Point> > LineList; 
  //first point is a vector normal to the line
  //second point is a point on the line

private:
  void _doFreeSnap(SnappedConstraints &sc,
                   Inkscape::Snapper::PointType const &t,
                   NR::Point const &p,
                   bool const &first_point,
                   std::vector<NR::Point> &points_to_snap,
                   std::list<SPItem const *> const &it,
                   std::vector<NR::Point> *unselected_nodes) const;
  
  void _doConstrainedSnap(SnappedConstraints &sc,
                          Inkscape::Snapper::PointType const &t,
                          NR::Point const &p,
                          bool const &first_point,
                          std::vector<NR::Point> &points_to_snap,
                          ConstraintLine const &c,
                          std::list<SPItem const *> const &it) const;
  
  /**
   *  \param p Point that we are trying to snap.
   *  \return List of lines that we should try snapping to.
   */
  virtual LineList _getSnapLines(NR::Point const &p) const = 0;
  
  virtual void _addSnappedLine(SnappedConstraints &sc, NR::Point const snapped_point, NR::Coord const snapped_distance, NR::Point const normal_to_line, NR::Point const point_on_line) const = 0;
};

}

#endif /* !SEEN_LINE_SNAPPER_H */
