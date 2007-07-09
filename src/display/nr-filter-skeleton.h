#ifndef __NR_FILTER_SKELETON_H__
#define __NR_FILTER_SKELETON_H__

/*
 * Filter primitive renderer skeleton class
 * You can create your new filter primitive renderer by replacing
 * all occurences of Skeleton, skeleton and SKELETON with your filter
 * type, like gaussian or blend in respective case.
 *
 * This can be accomplished with the following sed command:
 * sed -e "s/Skeleton/Name/g" -e "s/skeleton/name/" -e "s/SKELETON/NAME/"
 * nr-filter-skeleton.h >nr-filter-name.h
 *
 * (on one line, replace occurences of 'name' with your filter name)
 *
 * Remember to convert the .cpp file too. The sed command is same for both
 * files.
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "libnr/nr-matrix.h"

namespace NR {

class FilterSkeleton : public FilterPrimitive {
public:
    FilterSkeleton();
    static FilterPrimitive *create();
    virtual ~FilterSkeleton();

    virtual int render(FilterSlot &slot, Matrix const &trans);

private:

};

} /* namespace NR */

#endif /* __NR_FILTER_SKELETON_H__ */
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
