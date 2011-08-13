/**
 * @file
 * @brief Bitmap image belonging to an SVG drawing
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_IMAGE_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_IMAGE_H

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <2geom/transforms.h>

#include "display/drawing-item.h"

namespace Inkscape {

class DrawingImage
    : public DrawingItem
{
public:
    DrawingImage(Drawing &drawing);
    ~DrawingImage();

    void setARGB32Pixbuf(GdkPixbuf *pb);
    void setStyle(SPStyle *style);
    void setScale(double sx, double sy);
    void setOrigin(Geom::Point const &o);
    void setClipbox(Geom::Rect const &box);
    Geom::Rect bounds() const;

protected:
    virtual unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx,
                                 unsigned flags, unsigned reset);
    virtual void _renderItem(DrawingContext &ct, Geom::IntRect const &area, unsigned flags);
    virtual DrawingItem *_pickItem(Geom::Point const &p, double delta, unsigned flags);

    GdkPixbuf *_pixbuf;
    cairo_surface_t *_surface;
    SPStyle *_style;

    // TODO: the following three should probably be merged into a new Geom::Viewbox object
    Geom::Rect _clipbox; ///< for preserveAspectRatio
    Geom::Point _origin;
    Geom::Scale _scale;
};

} // end namespace Inkscape

#endif // !SEEN_INKSCAPE_DISPLAY_DRAWING_ITEM_H

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
