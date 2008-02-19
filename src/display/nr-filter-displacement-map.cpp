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

#include "display/nr-filter-displacement-map.h"
#include "display/nr-filter-types.h"
#include "display/nr-filter-units.h"
#include "libnr/nr-pixops.h"

namespace NR {

FilterDisplacementMap::FilterDisplacementMap()
{}

FilterPrimitive * FilterDisplacementMap::create() {
    return new FilterDisplacementMap();
}

FilterDisplacementMap::~FilterDisplacementMap()
{}

int FilterDisplacementMap::render(FilterSlot &slot, FilterUnits const &units) {
    NRPixBlock *texture = slot.get(_input);
    NRPixBlock *map = slot.get(_input2);

    // Bail out if either one of source images is missing
    if (!map || !texture) {
        g_warning("Missing source image for feDisplacementMap (map=%d texture=%d)", _input, _input2);
        return 1;
    }

    //TODO: check whether do we really need this check:
    if (map->area.x1 <= map->area.x0 || map->area.y1 <=  map->area.y0) return 0; //nothing to do!

    NRPixBlock *out = new NRPixBlock;

    //are these checks really necessary?
    if (out_w > map->area.x1 - out_x0) out_w = map->area.x1 - out_x0;
    if (out_h > map->area.y1 - out_y0) out_h = map->area.y1 - out_y0;
    if (out_x0 < map->area.x0){
        out_x0 = map->area.x0;
        out_w -= (map->area.x0 - out_x0);
    }
    if (out_y0 < map->area.y0){
        out_y0 = map->area.y0;
        out_h -= (map->area.y0 - out_y0);
    }

    out->area.x0 = out_x0;
    out->area.y0 = out_y0;
    out->area.x1 = out_x0 + out_w;
    out->area.y1 = out_y0 + out_h;

    nr_pixblock_setup_fast(out, map->mode, out->area.x0, out->area.y0, out->area.x1, out->area.y1, true);

    unsigned char *map_data = NR_PIXBLOCK_PX(map);
    unsigned char *texture_data = NR_PIXBLOCK_PX(texture);
    unsigned char *out_data = NR_PIXBLOCK_PX(out);
    int x, y;
    int in_w = map->area.x1 - map->area.x0;
    int in_h = map->area.y1 - map->area.y0;
    double coordx, coordy;
    
    Matrix trans = units.get_matrix_primitiveunits2pb();
    double scalex = scale*trans.expansionX();
    double scaley = scale*trans.expansionY();
    
    for (x=0; x < out_w; x++){
        for (y=0; y < out_h; y++){
            if (x+out_x0-map->area.x0 >= 0 &&
                x+out_x0-map->area.x0 < in_w &&
                y+out_y0-map->area.y0 >= 0 &&
                y+out_y0-map->area.y0 < in_h){

                    coordx = out_x0 - map->area.x0 + x + scalex * ( double(map_data[4*((x+out_x0-map->area.x0) + in_w*(y+out_y0-map->area.y0)) + Xchannel])/255 - 0.5);
                    coordy = out_y0 - map->area.y0 + y + scaley * ( double(map_data[4*((x+out_x0-map->area.x0) + in_w*(y+out_y0-map->area.y0)) + Ychannel])/255 - 0.5);

                    if (coordx>=0 && coordx<in_w && coordy>=0 && coordy<in_h){
                            out_data[4*(x + out_w*y)] = texture_data[4*(int(coordx) + int(coordy)*in_w)];
                            out_data[4*(x + out_w*y) + 1] = texture_data[4*(int(coordx) + int(coordy)*in_w) + 1];
                            out_data[4*(x + out_w*y) + 2] = texture_data[4*(int(coordx) + int(coordy)*in_w) + 2];
                            out_data[4*(x + out_w*y) + 3] = texture_data[4*(int(coordx) + int(coordy)*in_w) + 3];
                    } else {
                            out_data[4*(x + out_w*y)] = 255;
                            out_data[4*(x + out_w*y) + 1] = 255;
                            out_data[4*(x + out_w*y) + 2] = 255;
                            out_data[4*(x + out_w*y) + 3] = 0;
                    }
            }
        }
    }

    out->empty = FALSE;
    slot.set(_output, out);
            return 0;
}

void FilterDisplacementMap::set_input(int slot) {
    _input = slot;
}

void FilterDisplacementMap::set_scale(double s) {
    scale = s;
}

void FilterDisplacementMap::set_input(int input, int slot) {
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void FilterDisplacementMap::set_channel_selector(int s, FilterDisplacementMapChannelSelector channel) {
    if (channel > DISPLACEMENTMAP_CHANNEL_ALPHA || channel < DISPLACEMENTMAP_CHANNEL_RED) {
        g_warning("Selected an invalid channel value. (%d)", channel);
        return;
    }

    if (s == 0) Xchannel = channel;
    if (s == 1) Ychannel = channel;
}

void FilterDisplacementMap::area_enlarge(NRRectL &area, Matrix const &trans)
{
    out_x0 = area.x0;
    out_y0 = area.y0;
    out_w = area.x1 - area.x0;
    out_h = area.y1 - area.y0;
    
    double scalex = scale*trans.expansionX();
    double scaley = scale*trans.expansionY();

    area.x0 -= (int)(scalex/2);
    area.x1 += (int)(scalex/2);
    area.y0 -= (int)(scaley/2);
    area.y1 += (int)(scaley/2);
}

FilterTraits FilterDisplacementMap::get_input_traits() {
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
