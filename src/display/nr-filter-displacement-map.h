#ifndef __NR_FILTER_DISPLACEMENT_MAP_H__
#define __NR_FILTER_DISPLACEMENT_MAP_H__

/*
 * feDisplacementMap filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <felipe.sanches@gmail.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "libnr/nr-matrix.h"
#include "libnr/nr-rect-l.h"

namespace NR {

class FilterDisplacementMap : public FilterPrimitive {
public:
    FilterDisplacementMap();
    static FilterPrimitive *create();
    virtual ~FilterDisplacementMap();

    virtual void set_input(int slot);
    virtual void set_input(int input, int slot);
    virtual void set_scale(double s);
    virtual void set_channel_selector(int channel, int s);
    virtual int render(FilterSlot &slot, FilterUnits const &units);
    virtual void area_enlarge(NRRectL &area, Matrix const &trans);
    virtual FilterTraits get_input_traits();

private:
    double scale;
    int _input2;
    int Xchannel;
    int Ychannel;
    int out_x0, out_y0, out_w, out_h;
};

} /* namespace NR */

#endif /* __NR_FILTER_DISPLACEMENT_MAP_H__ */
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
