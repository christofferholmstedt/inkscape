#ifndef __NR_FILTER_SPECULARLIGHTING_H__
#define __NR_FILTER_SPECULARLIGHTING_H__

/*
 * feSpecularLighting renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 * 
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdktypes.h>
#include "display/nr-light-types.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "libnr/nr-matrix.h"
#include "sp-fedistantlight.h"
#include "sp-fepointlight.h"
#include "sp-fespotlight.h"
#include "color.h"

namespace NR {
    
class FilterSpecularLighting : public FilterPrimitive {
public:
    union {
        SPFeDistantLight *distant;
        SPFePointLight *point;
        SPFeSpotLight *spot;
    } light;
    LightType light_type;
    gdouble surfaceScale;
    gdouble specularConstant;
    gdouble specularExponent;
    guint32 lighting_color;
    
    FilterSpecularLighting();
    static FilterPrimitive *create();
    virtual ~FilterSpecularLighting();
    virtual int render(FilterSlot &slot, FilterUnits const &units);
    virtual FilterTraits get_input_traits();

private:
};

} /* namespace NR */

#endif /* __NR_FILTER_SPECULARLIGHTING_H__ */
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
