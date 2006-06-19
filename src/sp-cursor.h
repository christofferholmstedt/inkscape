#ifndef SP_CURSOR_H
#define SP_CURSOR_H

#include <gdk/gdk.h>

GdkCursor *sp_cursor_new_from_xpm(gchar **xpm, gint hot_x, gint hot_y);
GdkCursor *sp_cursor_new_from_pixbuf(GdkPixbuf *pixbuf, gint hot_x, gint hot_y);
GdkCursor *sp_cursor_new(GdkDisplay *display, GdkPixbuf *pixbuf, gchar **xpm, gint hot_x, gint hot_y);

#endif

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
