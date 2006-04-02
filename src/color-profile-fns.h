#ifndef SEEN_COLOR_PROFILE_FNS_H
#define SEEN_COLOR_PROFILE_FNS_H

/** \file 
 * Macros and fn declarations related to linear gradients.
 */

#include <glib-object.h>
#include <glib/gtypes.h>

namespace Inkscape {
namespace XML {
class Node;
} // namespace XML

class ColorProfile;

GType colorprofile_get_type();

} // namespace Inkscape

#define COLORPROFILE_TYPE (Inkscape::colorprofile_get_type())
#define COLORPROFILE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), COLORPROFILE_TYPE, Inkscape::ColorProfile))
#define COLORPROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), COLORPROFILE_TYPE, Inkscape::ColorProfileClass))
#define IS_COLORPROFILE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), COLORPROFILE_TYPE))
#define IS_COLORPROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), COLORPROFILE_TYPE))


#endif // !SEEN_COLOR_PROFILE_FNS_H

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
