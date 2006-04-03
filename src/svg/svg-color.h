#ifndef SVG_SVG_COLOR_H_SEEN
#define SVG_SVG_COLOR_H_SEEN

#include <glib/gtypes.h>

guint32 sp_svg_read_color(gchar const *str, unsigned int dfl);
guint32 sp_svg_read_color(gchar const *str, gchar const **end_ptr, guint32 def);
void sp_svg_write_color(char *buf, unsigned int buflen, unsigned int rgba32);


#endif /* !SVG_SVG_COLOR_H_SEEN */
