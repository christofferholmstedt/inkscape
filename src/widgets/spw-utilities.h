#ifndef __SPW_UTILITIES_H__
#define __SPW_UTILITIES_H__

/*
 * Inkscape Widget Utilities
 *
 * Author:
 *   Bryce W. Harrington <brycehar@bryceharrington.com>
 * 
 * Copyright (C) 2003 Bryce Harrington
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/* The following are helper routines for making Inkscape dialog widgets.
   All are prefixed with spw_, short for inkscape_widget.  This is not to
   be confused with SPWidget, an existing datatype associated with Inkscape::XML::Node/
   SPObject, that reacts to modification.
*/

#include <glib/gtypes.h>
#include <gtk/gtkstyle.h>      /* GtkWidget */

GtkWidget *
spw_label(GtkWidget *table, gchar const *label_text, int col, int row);

GtkWidget *
spw_hbox(GtkWidget *table, int width, int col, int row);

GtkWidget *
spw_vbox_checkbutton(GtkWidget *dialog, GtkWidget *table,
		     const gchar *label, const gchar *tip, gchar *key, GCallback cb);

GtkWidget *
spw_checkbutton(GtkWidget *dialog, GtkWidget *table,
		gchar const *label, gchar *key, int col, int row,
		int sensitive, GCallback cb);

GtkWidget *
spw_dropdown(GtkWidget *dialog, GtkWidget *table,
	     gchar const *label, gchar *key, int row,
	     GtkWidget *selector
	     );

GtkWidget *
spw_unit_selector(GtkWidget *dialog, GtkWidget *table,
		  gchar const *label, gchar *key, int row,
		GtkWidget *us, GCallback cb, bool can_be_negative = false);

void sp_set_font_size (GtkWidget *w, guint font);
void sp_set_font_size_smaller (GtkWidget *w);

gpointer sp_search_by_data_recursive(GtkWidget *w, gpointer data);
GtkWidget *sp_search_by_value_recursive(GtkWidget *w, gchar *key, gchar *value);

#endif
