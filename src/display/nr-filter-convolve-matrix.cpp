/*
 * feConvolveMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <felipe.sanches@gmail.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-filter-convolve-matrix.h"
#include "display/nr-filter-utils.h"
#include <vector>
namespace NR {

FilterConvolveMatrix::FilterConvolveMatrix()
: orderX(3),
  orderY(3),
  targetX((int)(orderX/2)),
  targetY((int)(orderY/2)),
  bias(0),
  edgeMode((NR::FilterConvolveMatrixEdgeMode) 0),
  preserveAlpha(false)
{}

FilterPrimitive * FilterConvolveMatrix::create() {
    return new FilterConvolveMatrix();
}

FilterConvolveMatrix::~FilterConvolveMatrix()
{}

static bool inside_area(int px, int py, int w, int h){
        if (px<0) return false;
        if (py<0) return false;
        if (px>w) return false;
        if (py>h) return false;
        return true;
}

int FilterConvolveMatrix::render(FilterSlot &slot, Matrix const &trans) {
    NRPixBlock *in = slot.get(_input);
    NRPixBlock *out = new NRPixBlock;

    nr_pixblock_setup_fast(out, in->mode,
                           in->area.x0, in->area.y0, in->area.x1, in->area.y1,
                           true);

    unsigned char *in_data = NR_PIXBLOCK_PX(in);
    unsigned char *out_data = NR_PIXBLOCK_PX(out);

    double result_R, result_G, result_B, result_A;
    int i, j, x, y;
    int width = in->area.x1 - in->area.x0;
    int height = in->area.y1 - in->area.y0;

    double div=0;

    if (divisor != 0){
        div = divisor;
    } else {
        for (i=0;i<orderX*orderY;i++){
            div += kernelMatrix[i];
        }
    }

    for (x=targetX; x < width - (orderX - targetX); x++){
        for (y=targetY; y < height - (orderY - targetY); y++){
            result_R = 0;
            result_G = 0;
            result_B = 0;
            result_A = 0;
            for (i=0; i < orderY; i++){
                for (j=0; j < orderX; j++){
                    if (inside_area(x - targetX + j, y - targetY + i, width, height)){
                        result_R += ( (double) in_data[4*( x - targetX + j + width*(y - targetY + i) )] * kernelMatrix[orderX-j-1 + orderX*(orderY-i-1)] );
                        result_G += ( (double) in_data[4*( x - targetX + j + width*(y - targetY + i) )+1] * kernelMatrix[orderX-j-1 + orderX*(orderY-i-1)] );
                        result_B += ( (double) in_data[4*( x - targetX + j + width*(y - targetY + i) )+2] * kernelMatrix[orderX-j-1 + orderX*(orderY-i-1)] );
                        result_A += ( (double) in_data[4*( x - targetX + j + width*(y - targetY + i) )+3] * kernelMatrix[orderX-j-1 + orderX*(orderY-i-1)] );
                    }
                }
            }

            out_data[4*( x + width*y )] = CLAMP_D_TO_U8(result_R / div + bias);
            out_data[4*( x + width*y )+1] = CLAMP_D_TO_U8(result_G / div + bias);
            out_data[4*( x + width*y )+2] = CLAMP_D_TO_U8(result_B / div + bias);
            out_data[4*( x + width*y )+3] = CLAMP_D_TO_U8(result_A / div + bias);
        }
    }

    out->empty = FALSE;
    slot.set(_output, out);
    return 0;
}

void FilterConvolveMatrix::set_targetX(int coord) {
    targetX = coord;
}

void FilterConvolveMatrix::set_targetY(int coord) {
    targetY = coord;
}

void FilterConvolveMatrix::set_orderX(int coord) {
    orderX = coord;
}

void FilterConvolveMatrix::set_orderY(int coord) {
    orderY = coord;
}

void FilterConvolveMatrix::set_divisor(double d) {
    divisor = d;
}

void FilterConvolveMatrix::set_bias(double b) {
    bias = b;
}

void FilterConvolveMatrix::set_kernelMatrix(std::vector<gdouble> &km) {
    kernelMatrix = km;
}

void FilterConvolveMatrix::set_edgeMode(FilterConvolveMatrixEdgeMode mode){
    edgeMode = mode;
}    

void FilterConvolveMatrix::set_preserveAlpha(bool pa){
    preserveAlpha = pa;
}

void FilterConvolveMatrix::area_enlarge(NRRectL &area, Matrix const &trans)
{
    //Seems to me that since this filter's operation is resolution dependent,
    // some spurious pixels may still appear at the borders when low zooming or rotating. Needs a better fix.
    area.x0 -= targetX;
    area.y0 -= targetY;
    area.x1 += orderX - targetX;
    area.y1 += orderY - targetY;
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
