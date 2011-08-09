/**
 * @file
 * @brief Cairo surface that remembers its origin
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

//#include <iostream>
#include "display/drawing-surface.h"
#include "display/drawing-context.h"
#include "display/cairo-utils.h"

namespace Inkscape {

using Geom::X;
using Geom::Y;


/** @class DrawingSurface
 * @brief Drawing surface that remembers its origin.
 *
 * This is a very minimalistic wrapper over cairo_surface_t. The main
 * extra functionality provided by this class is that it automates
 * the mapping from "logical space" (coordinates in the rendering)
 * and the "physical space" (surface pixels). For example, patterns
 * have to be rendered on tiles which have possibly non-integer
 * widths and heights.
 *
 * This class has delayed allocation functionality - it creates
 * the Cairo surface it wraps on the first call to createRawContext()
 * of when a DrawingContext is constructed.
 */

/** @brief Creates a surface with the given physical extents.
 * When a drawing context is created for this surface, its pixels
 * will cover the area under the given rectangle. */
DrawingSurface::DrawingSurface(Geom::IntRect const &area)
    : _surface(NULL)
    , _origin(area.min())
    , _scale(1, 1)
    , _pixels(area.dimensions())
{}

/** @brief Creates a surface with the given logical and physical extents.
 * When a drawing context is created for this surface, its pixels
 * will cover the area under the given rectangle. IT will contain
 * the number of pixels specified by the second argument.
 * @param logbox Logical extents of the surface
 * @param pixdims Pixel dimensions of the surface. */
DrawingSurface::DrawingSurface(Geom::Rect const &logbox, Geom::IntPoint const &pixdims)
    : _surface(NULL)
    , _origin(logbox.min())
    , _scale(pixdims[X] / logbox.width(), pixdims[Y] / logbox.height())
    , _pixels(pixdims)
{}

/** @brief Wrap a cairo_surface_t.
 * This constructor will take an extra reference on @a surface, which will
 * be released on destruction. */
DrawingSurface::DrawingSurface(cairo_surface_t *surface, Geom::Point const &origin)
    : _surface(surface)
    , _origin(origin)
    , _scale(1, 1)
{
    cairo_surface_reference(surface);
    _pixels[X] = cairo_image_surface_get_width(surface);
    _pixels[Y] = cairo_image_surface_get_height(surface);
}

DrawingSurface::~DrawingSurface()
{
    if (_surface)
        cairo_surface_destroy(_surface);
}

/// Get the logical extents of the surface.
Geom::Rect
DrawingSurface::area() const
{
    Geom::Rect r = Geom::Rect::from_xywh(_origin, dimensions());
    return r;
}

/// Get the pixel dimensions of the surface
Geom::IntPoint
DrawingSurface::pixels() const
{
    return _pixels;
}

/// Get the logical width and weight of the surface as a point.
Geom::Point
DrawingSurface::dimensions() const
{
    Geom::Point logical_dims(_pixels[X] / _scale[X], _pixels[Y] / _scale[Y]);
    return logical_dims;
}

Geom::Point
DrawingSurface::origin() const
{
    return _origin;
}

Geom::Scale
DrawingSurface::scale() const
{
    return _scale;
}

/// Get the transformation applied to the drawing context on construction.
Geom::Affine
DrawingSurface::drawingTransform() const
{
    Geom::Affine ret = _scale * Geom::Translate(-_origin);
    return ret;
}

cairo_surface_type_t
DrawingSurface::type() const
{
    // currently hardcoded
    return CAIRO_SURFACE_TYPE_IMAGE;
}

/// Drop contents of the surface and release the underlying Cairo object.
void
DrawingSurface::dropContents()
{
    if (_surface) {
        cairo_surface_destroy(_surface);
        _surface = NULL;
    }
}

/** @brief Create a drawing context for this surface.
 * It's better to use the surface constructor of DrawingContext. */
cairo_t *
DrawingSurface::createRawContext()
{
    // deferred allocation
    if (!_surface) {
        _surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, _pixels[X], _pixels[Y]);
    }
    cairo_t *ct = cairo_create(_surface);
    if (_scale != Geom::Scale::identity()) {
        cairo_scale(ct, _scale[X], _scale[Y]);
    }
    cairo_translate(ct, -_origin[X], -_origin[Y]);
    return ct;
}

Geom::IntRect
DrawingSurface::pixelArea() const
{
    Geom::IntRect ret = Geom::IntRect::from_xywh(_origin.round(), _pixels);
    return ret;
}

//////////////////////////////////////////////////////////////////////////////

DrawingCache::DrawingCache(Geom::IntRect const &area)
    : DrawingSurface(area)
    , _clean_region(cairo_region_create())
    , _pending_area(area)
{}

DrawingCache::~DrawingCache()
{
    cairo_region_destroy(_clean_region);
}

void
DrawingCache::markDirty(Geom::IntRect const &area)
{
    cairo_rectangle_int_t dirty = _convertRect(area);
    cairo_region_subtract_rectangle(_clean_region, &dirty);
}
void
DrawingCache::markClean(Geom::IntRect const &area)
{
    Geom::OptIntRect r = Geom::intersect(area, pixelArea());
    if (!r) return;
    cairo_rectangle_int_t clean = _convertRect(*r);
    cairo_region_union_rectangle(_clean_region, &clean);
}
bool
DrawingCache::isClean(Geom::IntRect const &area) const
{
    cairo_rectangle_int_t test = _convertRect(area);
    if (cairo_region_contains_rectangle(_clean_region, &test) == CAIRO_REGION_OVERLAP_IN) {
        return true;
    } else {
        return false;
    }
}

/// Call this during the update phase to schedule a transformation of the cache.
void
DrawingCache::scheduleTransform(Geom::IntRect const &new_area, Geom::Affine const &trans)
{
    if (new_area.hasZeroArea() && trans.isIdentity()) return;
    _pending_area = new_area;
    _pending_transform *= trans;
}

/// Transforms the cache according to the transform specified during the update phase.
/// Call this during render phase, before painting.
void
DrawingCache::prepare()
{
    Geom::IntRect old_area = pixelArea();
    bool is_identity = _pending_transform.isIdentity();
    if (is_identity) {
        if (_pending_area == old_area) return;
    }

    bool is_integer_translation = false;
    if (!is_identity && _pending_transform.isTranslation()) {
        Geom::IntPoint t = _pending_transform.translation().round();
        if (Geom::are_near(Geom::Point(t), _pending_transform.translation())) {
            // integer translation or identity with change of area
            is_integer_translation = true;
            cairo_region_translate(_clean_region, t[X], t[Y]);
            if (old_area + t == _pending_area) {
                // if the areas match, the only thing to do
                // is to ensure that the clean area is not too large
                cairo_rectangle_int_t limit = _convertRect(_pending_area);
                cairo_region_intersect_rectangle(_clean_region, &limit);
                _origin += t;
                _pending_transform.setIdentity();
                return;
            }
        }
    }
    // otherwise, we need to transform the cache
    Geom::IntPoint old_origin = old_area.min();
    cairo_surface_t *old_surface = _surface;
    _surface = NULL;
    _pixels = _pending_area.dimensions();
    _origin = _pending_area.min();

    cairo_t *ct = createRawContext();
    if (!is_identity) {
        ink_cairo_transform(ct, _pending_transform);
    }
    cairo_set_source_surface(ct, old_surface, old_origin[X], old_origin[Y]);
    cairo_set_operator(ct, CAIRO_OPERATOR_SOURCE);
    cairo_paint(ct);

    cairo_surface_destroy(old_surface);
    cairo_destroy(ct);

    if (!is_identity && !is_integer_translation) {
        // dirty everything
        cairo_region_destroy(_clean_region);
        _clean_region = cairo_region_create();
    } else {
        cairo_rectangle_int_t limit = _convertRect(_pending_area);
        cairo_region_intersect_rectangle(_clean_region, &limit);
    }
    //std::cout << _pending_transform << old_area << _pending_area << std::endl;
    _pending_transform.setIdentity();
}

/** @brief Paints the clean area from cache and returns the remaining part */
bool
DrawingCache::paintFromCache(DrawingContext &ct, Geom::IntRect const &area)
{
    if (!isClean(area))
        return false;

    Inkscape::DrawingContext::Save save(ct);
    ct.rectangle(area);
    ct.clip();
    ct.setSource(this);
    ct.paint();
    
    return true;
}

cairo_rectangle_int_t
DrawingCache::_convertRect(Geom::IntRect const &area)
{
    cairo_rectangle_int_t ret;
    ret.x = area.left();
    ret.y = area.top();
    ret.width = area.width();
    ret.height = area.height();
    return ret;
}

} // end namespace Inkscape

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
