#ifndef __NR_FILTER_TYPES_H__
#define __NR_FILTER_TYPES_H__

namespace NR {

enum FilterPrimitiveType {
    NR_FILTER_BLEND,
    NR_FILTER_COLORMATRIX,
    NR_FILTER_COMPONENTTRANSFER,
    NR_FILTER_COMPOSITE,
    NR_FILTER_CONVOLVEMATRIX,
    NR_FILTER_DIFFUSELIGHTING,
    NR_FILTER_DISPLACEMENTMAP,
    NR_FILTER_FLOOD,
    NR_FILTER_GAUSSIANBLUR,
    NR_FILTER_IMAGE,
    NR_FILTER_MERGE,
    NR_FILTER_MORPHOLOGY,
    NR_FILTER_OFFSET,
    NR_FILTER_SPECULARLIGHTING,
    NR_FILTER_TILE,
    NR_FILTER_TURBULENCE,
    NR_FILTER_ENDPRIMITIVETYPE // This must be last
};
//const int Filter::_filter_primitive_type_count = 16;

enum FilterSlotType {
    NR_FILTER_SLOT_NOT_SET = -1,
    NR_FILTER_SOURCEGRAPHIC = -2,
    NR_FILTER_SOURCEALPHA = -3,
    NR_FILTER_BACKGROUNDIMAGE = -4,
    NR_FILTER_BACKGROUNDALPHA = -5,
    NR_FILTER_FILLPAINT = -6,
    NR_FILTER_STROKEPAINT = -7
};

} /* namespace NR */

#endif // __NR_FILTER_TYPES_H__
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
