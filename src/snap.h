#ifndef SEEN_SNAP_H
#define SEEN_SNAP_H

/**
 * \file snap.h
 * \brief SnapManager class.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 2006-2007 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2000-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <vector>

#include <libnr/nr-coord.h>
#include <libnr/nr-dim2.h>
#include <libnr/nr-forward.h>
#include <libnr/nr-scale.h>

#include "guide-snapper.h"
#include "object-snapper.h"

class SPNamedView;

/// Class to coordinate snapping operations

/**
 *  Each SPNamedView has one of these.  It offers methods to snap points to whatever
 *  snappers are defined (e.g. grid, guides etc.).  It also allows callers to snap
 *  points which have undergone some transformation (e.g. translation, scaling etc.)
 */

class SnapManager
{
public:
    SnapManager(SPNamedView const *v);

    typedef std::list<const Inkscape::Snapper*> SnapperList;

    bool SomeSnapperMightSnap() const;

    Inkscape::SnappedPoint freeSnap(Inkscape::Snapper::PointType t,
                                    NR::Point const &p,
                                    SPItem const *it) const;

    Inkscape::SnappedPoint freeSnap( Inkscape::Snapper::PointType t,
                                      NR::Point const &p,
                                      bool const &first_point,
                                      std::vector<NR::Point> &points_to_snap,
                                      std::list<SPItem const *> const &it) const;

    Inkscape::SnappedPoint freeSnapAlways( Inkscape::Snapper::PointType t,
                                           NR::Point const &p,
                                           SPItem const *it,
                                           SnapperList &snappers );

    Inkscape::SnappedPoint freeSnapAlways( Inkscape::Snapper::PointType t,
                                           NR::Point const &p,
                                           std::list<SPItem const *> const &it,
                                           SnapperList &snappers );

    Inkscape::SnappedPoint constrainedSnap(Inkscape::Snapper::PointType t,
                                           NR::Point const &p,
                                           Inkscape::Snapper::ConstraintLine const &c,
                                           SPItem const *it) const;
    
    Inkscape::SnappedPoint constrainedSnap(Inkscape::Snapper::PointType t,
                                           NR::Point const &p,
                                           bool const &first_point,
                                           std::vector<NR::Point> &points_to_snap,
                                           Inkscape::Snapper::ConstraintLine const &c,
                                           std::list<SPItem const *> const &it) const;
                                           
    Inkscape::SnappedPoint guideSnap(NR::Point const &p,
                                     NR::Point const &guide_normal) const;

    std::pair<NR::Point, bool> freeSnapTranslation(Inkscape::Snapper::PointType t,
                                                   std::vector<NR::Point> const &p,
                                                   std::list<SPItem const *> const &it,
                                                   NR::Point const &tr) const;

    std::pair<NR::Point, bool> constrainedSnapTranslation(Inkscape::Snapper::PointType t,
                                                          std::vector<NR::Point> const &p,
                                                          std::list<SPItem const *> const &it,
                                                          Inkscape::Snapper::ConstraintLine const &c,
                                                          NR::Point const &tr) const;

    std::pair<NR::scale, bool> freeSnapScale(Inkscape::Snapper::PointType t,
                                             std::vector<NR::Point> const &p,
                                             std::list<SPItem const *> const &it,
                                             NR::scale const &s,
                                             NR::Point const &o) const;

    std::pair<NR::scale, bool> constrainedSnapScale(Inkscape::Snapper::PointType t,
                                                    std::vector<NR::Point> const &p,
                                                    std::list<SPItem const *> const &it,
                                                    Inkscape::Snapper::ConstraintLine const &c,
                                                    NR::scale const &s,
                                                    NR::Point const &o) const;

    std::pair<NR::Coord, bool> freeSnapStretch(Inkscape::Snapper::PointType t,
                                               std::vector<NR::Point> const &p,
                                               std::list<SPItem const *> const &it,
                                               NR::Coord const &s,
                                               NR::Point const &o,
                                               NR::Dim2 d,
                                               bool uniform) const;

    std::pair<NR::Coord, bool> freeSnapSkew(Inkscape::Snapper::PointType t,
                                            std::vector<NR::Point> const &p,
                                            std::list<SPItem const *> const &it,
                                            NR::Coord const &s,
                                            NR::Point const &o,
                                            NR::Dim2 d) const;

    Inkscape::GuideSnapper guide;      ///< guide snapper
    Inkscape::ObjectSnapper object;    ///< snapper to other objects

    SnapperList getSnappers() const;
    SnapperList getGridSnappers() const;
    
    void setSnapModeBBox(bool enabled);
    void setSnapModeNode(bool enabled);
    void setSnapModeGuide(bool enabled);
    bool getSnapModeBBox() const;
    bool getSnapModeNode() const;
    bool getSnapModeGuide() const;

    void setIncludeItemCenter(bool enabled)    {
        _include_item_center = enabled;
        object.setIncludeItemCenter(enabled);     //store a local copy in the object-snapper
                                                //instead of passing it through many functions 
    }
    
    bool getIncludeItemCenter() const {
        return _include_item_center;
    }
        
protected:
    SPNamedView const *_named_view;

private:

    enum Transformation {
        TRANSLATION,
        SCALE,
        STRETCH,
        SKEW
    };
    
    bool _include_item_center; //If true, snapping nodes will also snap the item's center
    
    std::pair<NR::Point, bool> _snapTransformed(Inkscape::Snapper::PointType type,
                                                std::vector<NR::Point> const &points,
                                                std::list<SPItem const *> const &ignore,
                                                bool constrained,
                                                Inkscape::Snapper::ConstraintLine const &constraint,
                                                Transformation transformation_type,
                                                NR::Point const &transformation,
                                                NR::Point const &origin,
                                                NR::Dim2 dim,
                                                bool uniform) const;
                                                
    Inkscape::SnappedPoint findBestSnap(NR::Point const &p, SnappedConstraints &sc) const;
};

#endif /* !SEEN_SNAP_H */

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
