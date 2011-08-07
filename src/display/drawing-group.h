/**
 * @file
 * @brief Group belonging to an SVG drawing element
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_GROUP_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_GROUP_H

#include "display/drawing-item.h"

class SPStyle;

namespace Inkscape {

class DrawingGroup
    : public DrawingItem
{
public:
    DrawingGroup(Drawing &drawing);
    ~DrawingGroup();

    bool pickChildren() { return _pick_children; }
    void setPickChildren(bool p);

    void setStyle(SPStyle *style);
    void setChildTransform(Geom::Affine const &new_trans);

protected:
    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx,
                                 unsigned flags, unsigned reset);
    virtual void _renderItem(DrawingContext &ct, Geom::IntRect const &area, unsigned flags);
    virtual void _clipItem(DrawingContext &ct, Geom::IntRect const &area);
    virtual DrawingItem *_pickItem(Geom::Point const &p, double delta, bool sticky);
    virtual bool _canClip();

    SPStyle *_style;
    Geom::Affine *_child_transform;
};

bool is_drawing_group(DrawingItem *item);

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
