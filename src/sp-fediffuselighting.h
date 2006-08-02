#ifndef SP_FEDIFFUSELIGHTING_FNS_H
#define SP_FEDIFFUSELIGHTING_FNS_H

/** \file 
 * Macros and fn declarations related to gaussian blur filter.
 */

#include <glib-object.h>
#include <glib/gtypes.h>

namespace Inkscape {
namespace XML {
class Node;
}
}

class SPFeDiffuseLighting;

#define SP_TYPE_FEDIFFUSELIGHTING (sp_feDiffuseLighting_get_type())
#define SP_FEDIFFUSELIGHTING(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_FEDIFFUSELIGHTING, SPFeDiffuseLighting))
#define SP_FEDIFFUSELIGHTING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_FEDIFFUSELIGHTING, SPFeDiffuseLightingClass))
#define SP_IS_FEDIFFUSELIGHTING(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_FEDIFFUSELIGHTING))
#define SP_IS_FEDIFFUSELIGHTING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_FEDIFFUSELIGHTING))

GType sp_feDiffuseLighting_get_type();

#endif /* !SP_FEDIFFUSELIGHTING_FNS_H */

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
#ifndef SP_FEDIFFUSELIGHTING_H_SEEN
#define SP_FEDIFFUSELIGHTING_H_SEEN

/** \file
 * SVG <feDiffuseLighting> implementation, see sp-feDiffuseLighting.cpp.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-filter.h"
#include "sp-fediffuselighting-fns.h"

/* FeDiffuseLighting base class */
class SPFeDiffuseLightingClass;

struct SPFeDiffuseLighting : public SPFilter {
    /** DIFFUSELIGHTING ATTRIBUTES HERE */
    
};

struct SPFeDiffuseLightingClass {
    SPFilterClass parent_class;
};

GType sp_feDiffuseLighting_get_type();


#endif /* !SP_FEDIFFUSELIGHTING_H_SEEN */

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
