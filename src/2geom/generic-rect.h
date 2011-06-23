/**
 * \file
 * \brief Axis-aligned rectangle
 *//*
 * Authors:
 *   Michael Sloan <mgsloan@gmail.com>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 * Copyright 2007-2011 Authors
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, output to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * Authors of original rect class:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Nathan Hurst <njh@mail.csse.monash.edu.au>
 *   bulia byak <buliabyak@users.sf.net>
 *   MenTaLguY <mental@rydia.net>
 */

#ifndef LIB2GEOM_SEEN_GENERIC_RECT_H
#define LIB2GEOM_SEEN_GENERIC_RECT_H

#include <boost/optional.hpp>

namespace Geom {

template <typename C>
class GenericOptRect;

/**
 * @brief Axis aligned, non-empty, generic rectangle.
 * @ingroup Primitives
 */
template <typename C>
class GenericRect
    : boost::additive< GenericRect<C>, typename CoordTraits<C>::PointType
    , boost::equality_comparable< GenericRect<C>
    , boost::orable< GenericRect<C>
    , boost::orable< GenericRect<C>, typename CoordTraits<C>::OptRectType
      > > > >
{
    typedef typename CoordTraits<C>::IntervalType CInterval;
    typedef typename CoordTraits<C>::PointType CPoint;
    typedef typename CoordTraits<C>::RectType CRect;
    typedef typename CoordTraits<C>::OptRectType OptCRect;
protected:
    CInterval f[2];
public:
    /// @name Create rectangles.
    /// @{
    /** @brief Create a rectangle that contains only the point at (0,0). */
    GenericRect() { f[X] = f[Y] = CInterval(); }
    /** @brief Create a rectangle from X and Y intervals. */
    GenericRect(CInterval const &a, CInterval const &b) {
        f[X] = a;
        f[Y] = b;
    }
    /** @brief Create a rectangle from two points. */
    GenericRect(CPoint const &a, CPoint const &b) {
        f[X] = Interval(a[X], b[X]);
        f[Y] = Interval(a[Y], b[Y]);
    }
    /** @brief Create a rectangle from a range of points.
     * The resulting rectangle will contain all ponts from the range.
     * The return type of iterators must be convertible to Point.
     * The range must not be empty. For possibly empty ranges, see OptRect.
     * @param start Beginning of the range
     * @param end   End of the range
     * @return Rectangle that contains all points from [start, end). */
    template <typename InputIterator>
    static GenericRect<C> from_range(InputIterator start, InputIterator end) {
        assert(start != end);
        CPoint p1 = *start++;
        GenericRect<C> result(p1, p1);
        for (; start != end; ++start) {
            result.expandTo(*start);
        }
        return result;
    }
    /** @brief Create a rectangle from a C-style array of points it should contain. */
    static GenericRect<C> from_array(CPoint const *c, unsigned n) {
        GenericRect<C> result = GenericRect<C>::from_range(c, c+n);
        return result;
    }
    /** @brief Create rectangle from origin and dimensions. */
    static GenericRect<C> from_xywh(C x, C y, C w, C h) {
        CPoint xy(x, y);
        CPoint wh(w, h);
        GenericRect<C> result(xy, xy + wh);
        return result;
    }
    /** @brief Create rectangle from origin and dimensions. */
    static GenericRect<C> from_xywh(CPoint const &xy, CPoint const &wh) {
        GenericRect<C> result(xy, xy + wh);
        return result;
    }
    /** @brief Create rectangle from two points. */
    static GenericRect<C> from_xyxy(C x0, C x1, C y0, C y1) {
        CPoint p0(x0, y0);
        CPoint p1(x1, y1);
        GenericRect<C> result(p0, p1);
        return result;
    }
    /// @}

    /// @name Inspect dimensions.
    /// @{
    CInterval &operator[](unsigned i)              { return f[i]; }
    CInterval const &operator[](unsigned i) const { return f[i]; }

    CPoint min() const { return CPoint(f[X].min(), f[Y].min()); }
    CPoint max() const { return CPoint(f[X].max(), f[Y].max()); }
    /** @brief Return the n-th corner of the rectangle.
     * If the Y axis grows upwards, this returns corners in clockwise order
     * starting from the lower left. If Y grows downwards, it returns the corners
     * in counter-clockwise order starting from the upper left. */
    CPoint corner(unsigned i) const {
        switch(i % 4) {
            case 0:  return CPoint(f[X].min(), f[Y].min());
            case 1:  return CPoint(f[X].max(), f[Y].min());
            case 2:  return CPoint(f[X].max(), f[Y].max());
            default: return CPoint(f[X].min(), f[Y].max());
        }
    }
        
    //We should probably remove these - they're coord sys gnostic
    /** @brief Return top coordinate of the rectangle (+Y is downwards). */
    C top() const { return f[Y].min(); }
    /** @brief Return bottom coordinate of the rectangle (+Y is downwards). */
    C bottom() const { return f[Y].max(); }
    /** @brief Return leftmost coordinate of the rectangle (+X is to the right). */
    C left() const { return f[X].min(); }
    /** @brief Return rightmost coordinate of the rectangle (+X is to the right). */
    C right() const { return f[X].max(); }

    /** @brief Get the horizontal extent of the rectangle. */
    C width() const { return f[X].extent(); }
    /** @brief Get the vertical extent of the rectangle. */
    C height() const { return f[Y].extent(); }

    /** @brief Get rectangle's width and height as a point.
     * @return Point with X coordinate corresponding to the width and the Y coordinate
     *         corresponding to the height of the rectangle. */
    CPoint dimensions() const { return CPoint(f[X].extent(), f[Y].extent()); }
    /** @brief Get the point in the geometric center of the rectangle. */
    CPoint midpoint() const { return CPoint(f[X].middle(), f[Y].middle()); }

    /** @brief Compute rectangle's area. */
    C area() const { return f[X].extent() * f[Y].extent(); }
    /** @brief Check whether the rectangle has zero area. */
    bool hasZeroArea() const { return (area() == 0); }

    /** @brief Get the larger extent (width or height) of the rectangle. */
    C maxExtent() const { return std::max(f[X].extent(), f[Y].extent()); }
    /** @brief Get the smaller extent (width or height) of the rectangle. */
    C minExtent() const { return std::min(f[X].extent(), f[Y].extent()); }
    /// @}

    /// @name Test other rectangles and points for inclusion.
    /// @{
    /** @brief Check whether the rectangles have any common points. */
    bool intersects(GenericRect<C> const &r) const { 
        return f[X].intersects(r[X]) && f[Y].intersects(r[Y]);
    }
    /** @brief Check whether the rectangle includes all points in the given rectangle. */
    bool contains(GenericRect<C> const &r) const { 
        return f[X].contains(r[X]) && f[Y].contains(r[Y]);
    }

    /** @brief Check whether the rectangles have any common points.
     * A non-empty rectangle will not intersect empty rectangles. */
    inline bool intersects(OptCRect const &r) const;
    /** @brief Check whether the rectangle includes all points in the given rectangle.
     * A non-empty rectangle will contain any empty rectangle. */
    inline bool contains(OptCRect const &r) const;

    /** @brief Check whether the given point is within the rectangle. */
    bool contains(CPoint const &p) const {
        return f[X].contains(p[X]) && f[Y].contains(p[Y]);
    }
    /// @}

    /// @name Modify the rectangle.
    /// @{
    /** @brief Set the upper left point of the rectangle. */
    void setMin(CPoint const &p) {
        f[X].setMin(p[X]);
        f[Y].setMin(p[Y]);
    }
    /** @brief Set the lower right point of the rectangle. */
    void setMax(CPoint const &p) {
        f[X].setMax(p[X]);
        f[Y].setMax(p[Y]);
    }
    /** @brief Enlarge the rectangle to contain the given point. */
    void expandTo(CPoint const &p)        { 
        f[X].expandTo(p[X]);  f[Y].expandTo(p[Y]);
    }
    /** @brief Enlarge the rectangle to contain the given rectangle. */
    void unionWith(GenericRect<C> const &b) { 
        f[X].unionWith(b[X]); f[Y].unionWith(b[Y]);
    }
    /** @brief Enlarge the rectangle to contain the given rectangle.
     * Unioning with an empty rectangle results in no changes. */
    void unionWith(OptCRect const &b);

    /** @brief Expand the rectangle in both directions by the specified amount.
     * Note that this is different from scaling. Negative values wil shrink the
     * rectangle. If <code>-amount</code> is larger than
     * half of the width, the X interval will contain only the X coordinate
     * of the midpoint; same for height. */
    void expandBy(C amount) {
        f[X].expandBy(amount);  f[Y].expandBy(amount);
    }
    /** @brief Expand the rectangle by the coordinates of the given point.
     * This will expand the width by the X coordinate of the point in both directions
     * and the height by Y coordinate of the point. Negative coordinate values will
     * shrink the rectangle. If <code>-p[X]</code> is larger than half of the width,
     * the X interval will contain only the X coordinate of the midpoint; same for height. */
    void expandBy(CPoint const &p) { 
        f[X].expandBy(p[X]);  f[Y].expandBy(p[Y]);
    }
    /// @}

    /// @name Operators
    /// @{
    /** @brief Offset the rectangle by a vector. */
    GenericRect<C> &operator+=(CPoint const &p) {
        f[X] += p[X];
        f[Y] += p[Y];
        return *this;
    }
    /** @brief Offset the rectangle by the negation of a vector. */
    GenericRect<C> &operator-=(CPoint const &p) {
        f[X] -= p[X];
        f[Y] -= p[Y];
        return *this;
    }
    /** @brief Union two rectangles. */
    GenericRect<C> &operator|=(GenericRect<C> const &o) {
        unionWith(o);
        return *this;
    }
    GenericRect<C> &operator|=(OptCRect const &o) {
        unionWith(o);
        return *this;
    }
    /** @brief Test for equality of rectangles. */
    bool operator==(GenericRect<C> const &o) const { return f[X] == o[X] && f[Y] == o[Y]; }
    /// @}
};

/**
 * @brief Axis-aligned generic rectangle that can be empty.
 * @ingroup Primitives
 */
template <typename C>
class GenericOptRect
    : public boost::optional<typename CoordTraits<C>::RectType>
    , boost::orable< GenericOptRect<C>
    , boost::andable< GenericOptRect<C>
    , boost::andable< GenericOptRect<C>, typename CoordTraits<C>::RectType
      > > >
{
    typedef typename CoordTraits<C>::IntervalType CInterval;
    typedef typename CoordTraits<C>::OptIntervalType OptCInterval;
    typedef typename CoordTraits<C>::PointType CPoint;
    typedef typename CoordTraits<C>::RectType CRect;
    typedef typename CoordTraits<C>::OptRectType OptCRect;
    typedef boost::optional<CRect> Base;
public:
    GenericOptRect() : Base() {}
    GenericOptRect(GenericRect<C> const &a) : Base(CRect(a)) {}
    GenericOptRect(CPoint const &a, CPoint const &b) : Base(CRect(a, b)) {}
    /**
     * Creates an empty OptRect when one of the argument intervals is empty.
     */
    GenericOptRect(OptCInterval const &x_int, OptCInterval const &y_int) {
        if (x_int && y_int) {
            *this = CRect(*x_int, *y_int);
        }
        // else, stay empty.
    }

    /** @brief Check for emptiness. */
    inline bool isEmpty() const { return !*this; };

    bool intersects(CRect const &r) const { return r.intersects(*this); }
    bool contains(CRect const &r) const { return *this && (*this)->contains(r); }

    bool intersects(OptCRect const &r) const { return *this && (*this)->intersects(r); }
    bool contains(OptCRect const &r) const { return *this && (*this)->contains(r); }

    bool contains(CPoint const &p) const { return *this && (*this)->contains(p); }

    void unionWith(CRect const &b) {
        if (*this) {
            (*this)->unionWith(b);
        } else {
            *this = b;
        }
    }
    void unionWith(OptCRect const &b) {
        if (b) unionWith(*b);
    }
    void intersectWith(CRect const &b) {
        if (!*this) return;
        OptCInterval x = (**this)[X] & b[X], y = (**this)[Y] & b[Y];
        if (x && y) {
            *this = CRect(*x, *y);
        } else {
            *(static_cast<Base*>(this)) = boost::none;
        }
    }
    void intersectWith(OptCRect const &b) {
        if (b) {
            intersectWith(*b);
        } else {
            *(static_cast<Base*>(this)) = boost::none;
        }
    }
    GenericOptRect<C> &operator|=(OptCRect const &b) {
        unionWith(b);
        return *this;
    }
    GenericOptRect<C> &operator&=(CRect const &b) {
        intersectWith(b);
        return *this;
    }
    GenericOptRect<C> &operator&=(OptCRect const &b) {
        intersectWith(b);
        return *this;
    }
};

template <typename C>
inline void GenericRect<C>::unionWith(OptCRect const &b) { 
    if (b) {
        unionWith(*b);
    }
}
template <typename C>
inline bool GenericRect<C>::intersects(OptCRect const &r) const {
    return r && intersects(*r);
}
template <typename C>
inline bool GenericRect<C>::contains(OptCRect const &r) const {
    return !r || contains(*r);
}

#ifdef _GLIBCXX_IOSTREAM
template <typename C>
inline std::ostream &operator<<(std::ostream &out, GenericRect<C> const &r) {
    out << "X: " << r[X] << "  Y: " << r[Y];
    return out;
}
#endif

} // end namespace Geom

#endif // LIB2GEOM_SEEN_RECT_H

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
