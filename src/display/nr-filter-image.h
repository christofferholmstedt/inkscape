#ifndef __NR_FILTER_IMAGE_H__
#define __NR_FILTER_IMAGE_H__

/*
 * feImage filter primitive renderer
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
#include "display/nr-filter-units.h"
#include <gtkmm.h>
#include "sp-item.h"

namespace Inkscape {
namespace Filters {

class FilterImage : public FilterPrimitive {
public:
    FilterImage();
    static FilterPrimitive *create();
    virtual ~FilterImage();

    virtual int render(FilterSlot &slot, FilterUnits const &units);
    virtual FilterTraits get_input_traits();
    void set_document( SPDocument *document );
    void set_href(const gchar *href);
    void set_align( unsigned int align );
    void set_clip( unsigned int clip );
    bool from_element;
    SPItem* SVGElem;

private:
    SPDocument *document;
    gchar *feImageHref;
    guint8* image_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> image;
    int width, height, rowstride;
    unsigned int aspect_align : 4;
    unsigned int aspect_clip : 1;
    bool has_alpha;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_IMAGE_H__ */
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
