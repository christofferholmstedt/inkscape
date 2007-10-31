/*
 * feDiffuseLighting renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib/gmessages.h>

#include "display/nr-3dutils.h"
#include "display/nr-arena-item.h"
#include "display/nr-filter-diffuselighting.h"
#include "display/nr-filter-getalpha.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"
#include "display/nr-filter-utils.h"
#include "display/nr-light.h"
#include "libnr/nr-blit.h"
#include "libnr/nr-pixblock.h"
#include "libnr/nr-matrix.h"
#include "libnr/nr-rect-l.h"
#include "color.h"

namespace NR {

FilterDiffuseLighting::FilterDiffuseLighting() 
{
    light_type = NO_LIGHT;
    diffuseConstant = 1;
    surfaceScale = 1;
    lighting_color = 0xffffffff;
}

FilterPrimitive * FilterDiffuseLighting::create() {
    return new FilterDiffuseLighting();
}

FilterDiffuseLighting::~FilterDiffuseLighting()
{}

#define COMPUTE_INTER(inter, N, L, kd) \
do {\
    (inter) = (kd) * scalar_product((N), (L)); \
    if ((inter) < 0) (inter) = 0; \
}while(0)


int FilterDiffuseLighting::render(FilterSlot &slot, FilterUnits const &units) {
    NRPixBlock *in = filter_get_alpha(slot.get(_input));
    NRPixBlock *out = new NRPixBlock;

    int w = in->area.x1 - in->area.x0;
    int h = in->area.y1 - in->area.y0;
    int x0 = in->area.x0;
    int y0 = in->area.y0;
    int i, j;
    //As long as FilterRes and kernel unit is not supported we hardcode the
    //default value
    int dx = 1; //TODO setup
    int dy = 1; //TODO setup
    //surface scale
    Matrix trans = units.get_matrix_primitiveunits2pb();
    gdouble ss = surfaceScale * trans[0];
    gdouble kd = diffuseConstant; //diffuse lighting constant

    Fvector L, N, LC;
    gdouble inter;

    nr_pixblock_setup_fast(out, in->mode,
            in->area.x0, in->area.y0, in->area.x1, in->area.y1,
            true);
    unsigned char *data_i = NR_PIXBLOCK_PX (in);
    unsigned char *data_o = NR_PIXBLOCK_PX (out);
    //No light, nothing to do
    switch (light_type) {
        case DISTANT_LIGHT:  
            //the light vector is constant
            {
            DistantLight *dl = new DistantLight(light.distant, lighting_color);
            dl->light_vector(L);
            dl->light_components(LC);
            //finish the work
            for (i = 0, j = 0; i < w*h; i++) {
                compute_surface_normal(N, ss, in, i / w, i % w, dx, dy);
                COMPUTE_INTER(inter, N, L, kd);

                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_RED]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_GREEN]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_BLUE]);
                data_o[j++] = 255;
            }
            out->empty = FALSE;
            delete dl;
            }
            break;
        case POINT_LIGHT:
            {
            PointLight *pl = new PointLight(light.point, lighting_color, trans);
            pl->light_components(LC);
        //TODO we need a reference to the filter to determine primitiveUnits
        //if objectBoundingBox is used, use a different matrix for light_vector
        // UPDATE: trans is now correct matrix from primitiveUnits to
        // pixblock coordinates
            //finish the work
            for (i = 0, j = 0; i < w*h; i++) {
                compute_surface_normal(N, ss, in, i / w, i % w, dx, dy);
                pl->light_vector(L,
                        i % w + x0,
                        i / w + y0,
                        ss * (double) data_i[4*i+3]/ 255);
                COMPUTE_INTER(inter, N, L, kd);

                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_RED]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_GREEN]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_BLUE]);
                data_o[j++] = 255;
            }
            out->empty = FALSE;
            delete pl;
            }
            break;
        case SPOT_LIGHT:
            {
            SpotLight *sl = new SpotLight(light.spot, lighting_color, trans);
        //TODO we need a reference to the filter to determine primitiveUnits
        //if objectBoundingBox is used, use a different matrix for light_vector
        // UPDATE: trans is now correct matrix from primitiveUnits to
        // pixblock coordinates
            //finish the work
            for (i = 0, j = 0; i < w*h; i++) {
                compute_surface_normal(N, ss, in, i / w, i % w, dx, dy);
                sl->light_vector(L,
                    i % w + x0,
                    i / w + y0,
                    ss * (double) data_i[4*i+3]/ 255);
                sl->light_components(LC, L);
                COMPUTE_INTER(inter, N, L, kd);
                
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_RED]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_GREEN]);
                data_o[j++] = CLAMP_D_TO_U8(inter * LC[LIGHT_BLUE]);
                data_o[j++] = 255;
            }
            out->empty = FALSE;
            delete sl;
            }
            break;
        //else unknown light source, doing nothing
        case NO_LIGHT:
        default:
            {
            if (light_type != NO_LIGHT)
                g_warning("unknown light source %d", light_type);
            for (i = 0; i < w*h; i++) {
                data_o[4*i+3] = 255;
            }
            out->empty = false;
            }
    }
        
    //finishing
    slot.set(_output, out);
    nr_pixblock_release(in);
    delete in;
    return 0;
}

FilterTraits FilterDiffuseLighting::get_input_traits() {
    return TRAIT_PARALLER;
}

} /* namespace NR */

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
