/*
 * Vanishing point for 3D perspectives
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_VANISHING_POINT_H
#define SEEN_VANISHING_POINT_H

#include "libnr/nr-point.h"
#include "line-geometry.h"

namespace Box3D {

enum VPState {
    VP_FINITE = 0, // perspective lines meet in the VP
    VP_INFINITE    // perspective lines are parallel
};

// The X-/Y-/Z-axis corresponds to the first/second/third digit
// in binary representation, respectively.
enum Axis {
    X = 1,
    Y = 2,
    Z = 4,
    XY = 3,
    XZ = 5,
    YZ = 6,
    XYZ = 7,
    NONE = 0
};

// We use the fourth bit in binary representation
// to indicate whether a face is front or rear.
enum FrontOrRear { // find a better name
    FRONT = 0,
    REAR = 8
};

extern Axis axes[3];
extern Axis planes[3];
extern FrontOrRear face_positions [2];

// Given a bit sequence that unambiguously specifies the face of a 3D box,
// return a number between 0 and 5 corresponding to that particular face
// (which is normally used to index an array). Return -1 if the bit sequence
// does not specify a face. A face can either be given by its plane (e.g, XY)
// or by the axis that is orthogonal to it (e.g., Z).
inline gint face_to_int (guint face_id) {
    switch (face_id) {
      case 1:  return 0;
      case 2:  return 2;
      case 4:  return 4;
      case 3:  return 4;
      case 5:  return 2;
      case 6:  return 0;

      case 9:  return 1;
      case 10: return 3;
      case 12: return 5;
      case 11: return 5;
      case 13: return 3;
      case 14: return 1;

    default: return -1;
    }
}

inline bool is_single_axis_direction (Box3D::Axis dir) {
    // tests whether dir is nonzero and a power of 2
    return (!(dir & (dir - 1)) && dir);
}

/**
 * Given two axis directions out of {X, Y, Z} or the corresponding plane, return the remaining one
 * We don't check if 'plane' really specifies a plane (i.e., if it consists of precisely two directions).
 */
inline Box3D::Axis third_axis_direction (Box3D::Axis dir1, Box3D::Axis dir2) {
    return (Box3D::Axis) ((dir1 + dir2) ^ 0x7);
}
inline Box3D::Axis third_axis_direction (Box3D::Axis plane) {
    return (Box3D::Axis) (plane ^ 0x7);
}

/* returns the first/second axis direction occuring in the (possibly compound) expression 'dirs' */
inline Box3D::Axis extract_first_axis_direction (Box3D::Axis dirs) {
    if (dirs & Box3D::X) return Box3D::X;
    if (dirs & Box3D::Y) return Box3D::Y;
    if (dirs & Box3D::Z) return Box3D::Z;
    return Box3D::NONE;
}
inline Box3D::Axis extract_second_axis_direction (Box3D::Axis dirs) {
    return extract_first_axis_direction ((Box3D::Axis) (dirs ^ extract_first_axis_direction(dirs)));
}

inline Box3D::Axis orth_plane (Box3D::Axis axis) {
    return (Box3D::Axis) (Box3D::XYZ ^ axis);
}

/* returns an axis direction perpendicular to the ones occuring in the (possibly compound) expression 'dirs' */
inline Box3D::Axis get_perpendicular_axis_direction (Box3D::Axis dirs) {
    if (!(dirs & Box3D::X)) return Box3D::X;
    if (!(dirs & Box3D::Y)) return Box3D::Y;
    if (!(dirs & Box3D::Z)) return Box3D::Z;
    return Box3D::NONE;
}

inline gchar * string_from_axes (Box3D::Axis axes) {
    GString *pstring = g_string_new("");
    if (axes & Box3D::X) g_string_append_printf (pstring, "X");
    if (axes & Box3D::Y) g_string_append_printf (pstring, "Y");
    if (axes & Box3D::Z) g_string_append_printf (pstring, "Z");
    return pstring->str;
}

// FIXME: Store the Axis of the VP inside the class
class VanishingPoint : public NR::Point {
public:
    inline VanishingPoint() : NR::Point() {};
    /***
    inline VanishingPoint(NR::Point const &pt, NR::Point const &ref = NR::Point(0,0))
                         : NR::Point (pt),
                           ref_pt (ref),
                           v_dir (pt[NR::X] - ref[NR::X], pt[NR::Y] - ref[NR::Y]) {}
    inline VanishingPoint(NR::Coord x, NR::Coord y, NR::Point const &ref = NR::Point(0,0))
                         : NR::Point (x, y),
                           ref_pt (ref),
                           v_dir (x - ref[NR::X], y - ref[NR::Y]) {}
    ***/
    VanishingPoint(NR::Point const &pt, NR::Point const &inf_dir, VPState st);
    VanishingPoint(NR::Point const &pt);
    VanishingPoint(NR::Point const &dir, VPState const state);
    VanishingPoint(NR::Point const &pt, NR::Point const &direction);
    VanishingPoint(NR::Coord x, NR::Coord y);
    VanishingPoint(NR::Coord x, NR::Coord y, VPState const state);
    VanishingPoint(NR::Coord x, NR::Coord y, NR::Coord dir_x, NR::Coord dir_y);
    VanishingPoint(VanishingPoint const &rhs);

    bool is_finite();
    VPState toggle_parallel();
    void draw(Box3D::Axis const axis); // Draws a point on the canvas if state == VP_FINITE
    //inline VPState state() { return state; }
	
    VPState state;
    //NR::Point ref_pt; // point of reference to compute the direction of parallel lines
    NR::Point v_dir; // direction of perslective lines if the VP has state == VP_INFINITE

private:
};


} // namespace Box3D


/** A function to print out the VanishingPoint (prints the coordinates) **/
/***
inline std::ostream &operator<< (std::ostream &out_file, const VanishingPoint &vp) {
    out_file << vp;
    return out_file;
}
***/


#endif /* !SEEN_VANISHING_POINT_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
