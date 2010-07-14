#ifndef __NR_FILTER_FLOOD_H__
#define __NR_FILTER_FLOOD_H__

/*
 * feFlood filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-filter-primitive.h"

class SVGICCColor;

namespace Inkscape {
namespace Filters {

class FilterFlood : public FilterPrimitive {
public:
    FilterFlood();
    static FilterPrimitive *create();
    virtual ~FilterFlood();

    virtual void render_cairo(FilterSlot &slot);
    virtual bool can_handle_affine(Geom::Matrix const &);
    virtual void set_opacity(double o);
    virtual void set_color(guint32 c);
    virtual void set_icc(SVGICCColor *icc_color);
    virtual void area_enlarge(NRRectL &area, Geom::Matrix const &trans);
private:
    double opacity;
    guint32 color;
    SVGICCColor *icc;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_FLOOD_H__ */
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
