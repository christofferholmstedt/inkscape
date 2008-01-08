#ifndef SEEN_OBJECT_SNAPPER_H
#define SEEN_OBJECT_SNAPPER_H

/**
 *  \file object-snapper.h
 *  \brief Snapping things to objects.
 *
 * Authors:
 *   Carl Hetherington <inkscape@carlh.net>
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *
 * Copyright (C) 2005 - 2007 Authors 
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "snapper.h"
#include "sp-path.h"
#include "splivarot.h"

struct SPNamedView;
struct SPItem;
struct SPObject;

namespace Inkscape
{

class ObjectSnapper : public Snapper
{

public:
  ObjectSnapper(SPNamedView const *nv, NR::Coord const d);
  ~ObjectSnapper();

  enum DimensionToSnap {
    GUIDE_TRANSL_SNAP_X, // For snapping a vertical guide (normal in the X-direction) to objects, 
    GUIDE_TRANSL_SNAP_Y, // For snapping a horizontal guide (normal in the Y-direction) to objects
    ANGLED_GUIDE_TRANSL_SNAP, // For snapping an angled guide, while translating it accross the desktop
    ANGLED_GUIDE_ROT_SNAP, // For snapping an angled guide, while rotating it around some pivot point
    TRANSL_SNAP_XY}; // All other cases; for snapping to objects, other than guides

  void setSnapToItemNode(bool s) {
    _snap_to_itemnode = s;
  }

  bool getSnapToItemNode() const {
    return _snap_to_itemnode;
  }

  void setSnapToItemPath(bool s) {
    _snap_to_itempath = s;
  }

  bool getSnapToItemPath() const {
    return _snap_to_itempath;
  }
  
  void setSnapToBBoxNode(bool s) {
    _snap_to_bboxnode = s;
  }

  bool getSnapToBBoxNode() const {
    return _snap_to_bboxnode;
  }

  void setSnapToBBoxPath(bool s) {
    _snap_to_bboxpath = s;
  }

  bool getSnapToBBoxPath() const {
    return _snap_to_bboxpath;
  }
  
  void setIncludeItemCenter(bool s) {
    _include_item_center = s;
  }

  bool getIncludeItemCenter() const {
    return _include_item_center;
  }
  
  void setStrictSnapping(bool enabled) {
      _strict_snapping = enabled;
  }
  
  void guideSnap(SnappedConstraints &sc,
  				 NR::Point const &p,
                 NR::Point const &guide_normal) const;
  
  bool ThisSnapperMightSnap() const;
  bool GuidesMightSnap() const;
  
private:
  //store some lists of candidates, points and paths, so we don't have to rebuild them for each point we want to snap
  std::vector<SPItem*> *_candidates; 
  std::vector<NR::Point> *_points_to_snap_to;
  std::vector<NArtBpath*> *_bpaths_to_snap_to;
  std::vector<Path*> *_paths_to_snap_to;
  void _doFreeSnap(SnappedConstraints &sc,
                      Inkscape::Snapper::PointType const &t,
                      NR::Point const &p,
                      bool const &first_point,
                      std::vector<NR::Point> &points_to_snap,
                      std::list<SPItem const *> const &it) const;

  void _doConstrainedSnap(SnappedConstraints &sc,
                      Inkscape::Snapper::PointType const &t,
                      NR::Point const &p,
                      bool const &first_point,                                                                   
                      std::vector<NR::Point> &points_to_snap,
                      ConstraintLine const &c,
                      std::list<SPItem const *> const &it) const;
                       
  void _findCandidates(SPObject* r,
                       std::list<SPItem const *> const &it,
                       bool const &first_point,
                       std::vector<NR::Point> &points_to_snap,
                       DimensionToSnap const snap_dim) const;
  
  void _snapNodes(SnappedConstraints &sc,
                      Inkscape::Snapper::PointType const &t,
                      NR::Point const &p, 
                      bool const &first_point) const;
                      
  void _snapTranslatingGuideToNodes(SnappedConstraints &sc,
                     Inkscape::Snapper::PointType const &t,
                     NR::Point const &p,
                     NR::Point const &guide_normal) const;
                     
  void _collectNodes(Inkscape::Snapper::PointType const &t,
                  bool const &first_point) const;
  
  void _snapPaths(SnappedConstraints &sc,
                      Inkscape::Snapper::PointType const &t, 
                      NR::Point const &p,
                      bool const &first_point) const;
                      
  void _snapPathsConstrained(SnappedConstraints &sc,
                 Inkscape::Snapper::PointType const &t,
                 NR::Point const &p,
                 bool const &first_point,
                 ConstraintLine const &c) const;
  
  void _collectPaths(Inkscape::Snapper::PointType const &t, 
                  bool const &first_point) const;
  void _clear_paths() const;
  
  bool _snap_to_itemnode;
  bool _snap_to_itempath;
  bool _snap_to_bboxnode;
  bool _snap_to_bboxpath;
  
  //If enabled, then bbox corners will only snap to bboxes, 
  //and nodes will only snap to nodes and paths. We will not
  //snap bbox corners to nodes, or nodes to bboxes.
  //(snapping to grids and guides is not affected by this)
  bool _strict_snapping; 
  bool _include_item_center;
};

}

#endif
