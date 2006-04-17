/*
 * Gradient aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#include "macros.h"
#include "widgets/button.h"
#include "widgets/widget-sizes.h"
#include "widgets/spw-utilities.h"
#include "widgets/spinbutton-events.h"
#include "widgets/gradient-vector.h"
#include "widgets/gradient-image.h"
#include "style.h"

#include "prefs-utils.h"
#include "document-private.h"
#include "desktop.h"
#include "desktop-handles.h"
#include <glibmm/i18n.h>

#include "gradient-context.h"
#include "gradient-drag.h"
#include "sp-linear-gradient.h"
#include "sp-radial-gradient.h"
#include "gradient-chemistry.h"
#include "selection.h"

#include "toolbox.h"


//########################
//##       Gradient     ##
//########################

static void gr_toggle_type (GtkWidget *button, gpointer data) {
    GtkWidget *linear = (GtkWidget *) g_object_get_data (G_OBJECT(data), "linear");
    GtkWidget *radial = (GtkWidget *) g_object_get_data (G_OBJECT(data), "radial");
    if (button == linear && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (linear))) {
        prefs_set_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_LINEAR);
        if (radial) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radial), FALSE);
    } else if (button == radial && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radial))) {
        prefs_set_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_RADIAL);
        if (linear) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (linear), FALSE);
    }
}

static void gr_toggle_fillstroke (GtkWidget *button, gpointer data) {
    GtkWidget *fill = (GtkWidget *) g_object_get_data (G_OBJECT(data), "fill");
    GtkWidget *stroke = (GtkWidget *) g_object_get_data (G_OBJECT(data), "stroke");
    if (button == fill && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fill))) {
        prefs_set_int_attribute ("tools.gradient", "newfillorstroke", 1);
        if (stroke) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stroke), FALSE);
    } else if (button == stroke && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (stroke))) {
        prefs_set_int_attribute ("tools.gradient", "newfillorstroke", 0);
        if (fill) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fill), FALSE);
    }
}

void
gr_apply_gradient_to_item (SPItem *item, SPGradient *gr, SPGradientType new_type, guint new_fill, bool do_fill, bool do_stroke)
{
    SPStyle *style = SP_OBJECT_STYLE (item);

    if (do_fill) {
        if (style && (style->fill.type == SP_PAINT_TYPE_PAINTSERVER) && 
            SP_IS_GRADIENT (SP_OBJECT_STYLE_FILL_SERVER (item))) {
            SPObject *server = SP_OBJECT_STYLE_FILL_SERVER (item);
            if (SP_IS_LINEARGRADIENT (server)) {
                sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_LINEAR, true);
            } else if (SP_IS_RADIALGRADIENT (server)) {
                sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_RADIAL, true);
            } 
        } else if (new_fill) {
            sp_item_set_gradient(item, gr, new_type, true);
        }
    } 

    if (do_stroke) {
        if (style && (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) && 
            SP_IS_GRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (item))) {
            SPObject *server = SP_OBJECT_STYLE_STROKE_SERVER (item);
            if (SP_IS_LINEARGRADIENT (server)) {
                sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_LINEAR, false);
            } else if (SP_IS_RADIALGRADIENT (server)) {
                sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_RADIAL, false);
            } 
        } else if (!new_fill) {
            sp_item_set_gradient(item, gr, new_type, false);
        }
    }
}

/**
Applies gradient vector gr to the gradients attached to the selected dragger of drag, or if none,
to all objects in selection. If there was no previous gradient on an item, uses gradient type and
fill/stroke setting from preferences to create new default (linear: left/right; radial: centered)
gradient.
*/
void
gr_apply_gradient (Inkscape::Selection *selection, GrDrag *drag, SPGradient *gr)
{
    SPGradientType new_type = (SPGradientType) prefs_get_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_LINEAR);
    guint new_fill = prefs_get_int_attribute ("tools.gradient", "newfillorstroke", 1);

    // First try selected dragger
    if (drag && drag->selected) {
        GrDragger *dragger = drag->selected;
        for (GSList const* i = dragger->draggables; i != NULL; i = i->next) { // for all draggables of dragger
            GrDraggable *draggable = (GrDraggable *) i->data;
            gr_apply_gradient_to_item (draggable->item, gr, new_type, new_fill, draggable->fill_or_stroke, !draggable->fill_or_stroke);
        }
        return;
    }

   // If no drag or no dragger selected, act on selection
   for (GSList const* i = selection->itemList(); i != NULL; i = i->next) {
       gr_apply_gradient_to_item (SP_ITEM(i->data), gr, new_type, new_fill, true, true);
   }
}

void
gr_item_activate (GtkMenuItem *menuitem, gpointer data)
{
    SPGradient *gr = (SPGradient *) g_object_get_data (G_OBJECT (menuitem), "gradient");
    gr = sp_gradient_ensure_vector_normalized(gr);

    SPDesktop *desktop = (SPDesktop *) data;
    Inkscape::Selection *selection = sp_desktop_selection (desktop);
    SPEventContext *ev = sp_desktop_event_context (desktop);

    gr_apply_gradient (selection, ev? ev->get_drag() : NULL, gr);

    sp_document_done (sp_desktop_document (desktop));
}

gchar *
gr_prepare_label (SPObject *obj)
{
    const gchar *id = obj->defaultLabel();
    if (strlen(id) > 15 && (!strncmp (id, "#linearGradient", 15) || !strncmp (id, "#radialGradient", 15))) 
        return g_strdup_printf ("<small>#%s</small>", id+15);
    return g_strdup_printf ("<small>%s</small>", id);
}

GtkWidget *
gr_vector_list (SPDesktop *desktop, bool selection_empty, SPGradient *gr_selected, bool gr_multi)
{
    SPDocument *document = sp_desktop_document (desktop);

    GtkWidget *om = gtk_option_menu_new ();
    GtkWidget *m = gtk_menu_new ();

    GSList *gl = NULL;
    const GSList *gradients = sp_document_get_resource_list (document, "gradient");
		for (const GSList *i = gradients; i != NULL; i = i->next) {
        if (SP_GRADIENT_HAS_STOPS (i->data)) {
            gl = g_slist_prepend (gl, i->data);
        }
		}
    gl = g_slist_reverse (gl);

    guint pos = 0;
    guint idx = 0;

    if (!gl) {
        GtkWidget *l = gtk_label_new("");
        gtk_label_set_markup (GTK_LABEL(l), _("<small>No gradients</small>"));
        GtkWidget *i = gtk_menu_item_new ();
        gtk_container_add (GTK_CONTAINER (i), l);

        gtk_widget_show (i);
        gtk_menu_append (GTK_MENU (m), i);
        gtk_widget_set_sensitive (om, FALSE);
    } else if (selection_empty) {
        GtkWidget *l = gtk_label_new("");
        gtk_label_set_markup (GTK_LABEL(l), _("<small>Nothing selected</small>"));
        GtkWidget *i = gtk_menu_item_new ();
        gtk_container_add (GTK_CONTAINER (i), l);

        gtk_widget_show (i);
        gtk_menu_append (GTK_MENU (m), i);
        gtk_widget_set_sensitive (om, FALSE);
    } else {

        if (gr_selected == NULL) {
            GtkWidget *l = gtk_label_new("");
            gtk_label_set_markup (GTK_LABEL(l), _("<small>No gradients in selection</small>"));
            GtkWidget *i = gtk_menu_item_new ();
            gtk_container_add (GTK_CONTAINER (i), l);

            gtk_widget_show (i);
            gtk_menu_append (GTK_MENU (m), i);
        }

        if (gr_multi) {
            GtkWidget *l = gtk_label_new("");
            gtk_label_set_markup (GTK_LABEL(l), _("<small>Multiple gradients</small>"));
            GtkWidget *i = gtk_menu_item_new ();
            gtk_container_add (GTK_CONTAINER (i), l);

            gtk_widget_show (i);
            gtk_menu_append (GTK_MENU (m), i);
        }

        while (gl) {
            SPGradient *gradient = SP_GRADIENT (gl->data);
            gl = g_slist_remove (gl, gradient);

            GtkWidget *i = gtk_menu_item_new ();
            g_object_set_data (G_OBJECT (i), "gradient", gradient);
            g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (gr_item_activate), desktop);

            GtkWidget *image = sp_gradient_image_new (gradient);

            GtkWidget *hb = gtk_hbox_new (FALSE, 4);
            GtkWidget *l = gtk_label_new ("");
            gchar *label = gr_prepare_label (SP_OBJECT(gradient));
            gtk_label_set_markup (GTK_LABEL(l), label);
            g_free (label);
            gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
            gtk_box_pack_start (GTK_BOX (hb), l, TRUE, TRUE, 0);
            gtk_box_pack_start (GTK_BOX (hb), image, FALSE, FALSE, 0);

            gtk_widget_show_all (i);

            gtk_container_add (GTK_CONTAINER (i), hb);

            gtk_menu_append (GTK_MENU (m), i);

            if (gradient == gr_selected) {
                pos = idx;
            }
            idx ++;
        }
        gtk_widget_set_sensitive (om, TRUE);
    }

    gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);
    /* Select the current gradient, or the Multi/Nothing line */
    if (gr_multi || gr_selected == NULL)
        gtk_option_menu_set_history (GTK_OPTION_MENU (om), 0);
    else 
        gtk_option_menu_set_history (GTK_OPTION_MENU (om), pos);

    return om;
}


void
gr_read_selection (Inkscape::Selection *selection, GrDrag *drag, SPGradient **gr_selected, bool *gr_multi, SPGradientSpread *spr_selected, bool *spr_multi) 
{
    if (drag && drag->selected) {
        GrDragger *dragger = drag->selected;
        for (GSList const* i = dragger->draggables; i != NULL; i = i->next) { // for all draggables of dragger
            GrDraggable *draggable = (GrDraggable *) i->data;
            SPGradient *gradient = sp_item_gradient_get_vector (draggable->item, draggable->fill_or_stroke);
            SPGradientSpread spread = sp_item_gradient_get_spread (draggable->item, draggable->fill_or_stroke);

            if (gradient != *gr_selected) {
                if (*gr_selected != NULL) {
                    *gr_multi = true;
                } else {
                    *gr_selected = gradient;
                }
            }
            if (spread != *spr_selected) {
                if (*spr_selected != INT_MAX) {
                    *spr_multi = true;
                } else {
                    *spr_selected = spread;
                }
            }
         }
        return;
    } 

   // If no selected dragger, read desktop selection
   for (GSList const* i = selection->itemList(); i != NULL; i = i->next) {
        SPItem *item = SP_ITEM(i->data);
        SPStyle *style = SP_OBJECT_STYLE (item);

        if (style && (style->fill.type == SP_PAINT_TYPE_PAINTSERVER)) { 
            SPObject *server = SP_OBJECT_STYLE_FILL_SERVER (item);
            if (SP_IS_GRADIENT (server)) {
                SPGradient *gradient = sp_gradient_get_vector (SP_GRADIENT (server), false);
                SPGradientSpread spread = sp_gradient_get_spread (SP_GRADIENT (server));
                if (gradient != *gr_selected) {
                    if (*gr_selected != NULL) {
                        *gr_multi = true;
                    } else {
                        *gr_selected = gradient;
                    }
                }
                if (spread != *spr_selected) {
                    if (*spr_selected != INT_MAX) {
                        *spr_multi = true;
                    } else {
                        *spr_selected = spread;
                    }
                }
            }
        }
        if (style && (style->stroke.type == SP_PAINT_TYPE_PAINTSERVER)) { 
            SPObject *server = SP_OBJECT_STYLE_STROKE_SERVER (item);
            if (SP_IS_GRADIENT (server)) {
                SPGradient *gradient = sp_gradient_get_vector (SP_GRADIENT (server), false);
                SPGradientSpread spread = sp_gradient_get_spread (SP_GRADIENT (server));
                if (gradient != *gr_selected) {
                    if (*gr_selected != NULL) {
                        *gr_multi = true;
                    } else {
                        *gr_selected = gradient;
                    }
                }
                if (spread != *spr_selected) {
                    if (*spr_selected != INT_MAX) {
                        *spr_multi = true;
                    } else {
                        *spr_selected = spread;
                    }
                }
            }
        }
    }
 }

static void 
gr_tb_selection_changed (Inkscape::Selection *, gpointer data)
{
    GtkWidget *widget = (GtkWidget *) data;

    SPDesktop *desktop = (SPDesktop *) g_object_get_data (G_OBJECT(widget), "desktop");
    if (!desktop)
        return;

    Inkscape::Selection *selection = sp_desktop_selection (desktop); // take from desktop, not from args
    if (!selection) 
        return;
    
    SPEventContext *ev = sp_desktop_event_context (desktop);

    GtkWidget *om = (GtkWidget *) g_object_get_data (G_OBJECT (widget), "menu");
    if (om) gtk_widget_destroy (om);

    SPGradient *gr_selected = NULL;
    bool gr_multi = false;

    SPGradientSpread spr_selected = (SPGradientSpread) INT_MAX; // meaning undefined
    bool spr_multi = false;

    gr_read_selection (selection, ev? ev->get_drag() : NULL, &gr_selected, &gr_multi, &spr_selected, &spr_multi);

    om = gr_vector_list (desktop, selection->isEmpty(), gr_selected, gr_multi);
    g_object_set_data (G_OBJECT (widget), "menu", om);

    GtkWidget *buttons = (GtkWidget *) g_object_get_data (G_OBJECT(widget), "buttons");
    gtk_widget_set_sensitive (buttons, (gr_selected && !gr_multi));
  
    gtk_box_pack_start (GTK_BOX (widget), om, TRUE, TRUE, 0);

    gtk_widget_show_all (widget);
}

static void
gr_tb_selection_modified (Inkscape::Selection *selection, guint flags, gpointer data)
{
    gr_tb_selection_changed (selection, data);
}

static void
gr_drag_selection_changed (gpointer dragger, gpointer data)
{
    gr_tb_selection_changed (NULL, data);
}

static void
gr_defs_release (SPObject *defs, GtkWidget *widget)
{
    gr_tb_selection_changed (NULL, (gpointer) widget);
}

static void
gr_defs_modified (SPObject *defs, guint flags, GtkWidget *widget)
{
    gr_tb_selection_changed (NULL, (gpointer) widget);
}

static void
gr_fork (GtkWidget *button, GtkWidget *widget)
{
    SPDesktop *desktop = (SPDesktop *) g_object_get_data (G_OBJECT(widget), "desktop");
    SPDocument *document = sp_desktop_document (desktop);
    Inkscape::Selection *selection = sp_desktop_selection (desktop);
    SPEventContext *ev = sp_desktop_event_context (desktop);
    GtkWidget *om = (GtkWidget *) g_object_get_data (G_OBJECT(widget), "menu");

    if (om && document) {
        GtkWidget *i = gtk_menu_get_active (GTK_MENU (gtk_option_menu_get_menu (GTK_OPTION_MENU (om))));
        SPGradient *gr = (SPGradient *) g_object_get_data (G_OBJECT(i), "gradient");

        if (gr) {
            SPGradient *gr_new = sp_gradient_fork_vector_if_necessary (gr);
            if (gr_new != gr) {
                gr_apply_gradient (selection, ev? ev->get_drag() : NULL, gr_new);
                sp_document_done (document);
            }
        }
    }

    spinbutton_defocus(GTK_OBJECT(widget));
}

static void gr_disconnect_sigc (GObject *obj, sigc::connection *connection) {
    connection->disconnect();
    delete connection;
}

static void gr_disconnect_gsignal (GObject *widget, gpointer defs) {
    if (defs && G_IS_OBJECT(defs))
        sp_signal_disconnect_by_data (defs, widget);
}


static void
gr_edit (GtkWidget *button, GtkWidget *widget)
{
    GtkWidget *om = (GtkWidget *) g_object_get_data (G_OBJECT(widget), "menu");

    spinbutton_defocus(GTK_OBJECT(widget));

    if (om) {
        GtkWidget *i = gtk_menu_get_active (GTK_MENU (gtk_option_menu_get_menu (GTK_OPTION_MENU (om))));
        SPGradient *gr = (SPGradient *) g_object_get_data (G_OBJECT(i), "gradient");

        if (gr) {
            GtkWidget *dialog = sp_gradient_vector_editor_new (gr);
            gtk_widget_show (dialog);
        }
    }
}

GtkWidget *
gr_change_widget (SPDesktop *desktop)
{
    Inkscape::Selection *selection = sp_desktop_selection (desktop);
    SPDocument *document = sp_desktop_document (desktop);
    SPEventContext *ev = sp_desktop_event_context (desktop);

    SPGradient *gr_selected = NULL;
    bool gr_multi = false;

    SPGradientSpread spr_selected = (SPGradientSpread) INT_MAX; // meaning undefined
    bool spr_multi = false;

    GtkTooltips *tt = gtk_tooltips_new();

    gr_read_selection (selection, ev? ev->get_drag() : NULL, &gr_selected, &gr_multi, &spr_selected, &spr_multi);
 
    GtkWidget *widget = gtk_hbox_new(FALSE, FALSE);
    gtk_object_set_data(GTK_OBJECT(widget), "dtw", desktop->canvas);
    g_object_set_data (G_OBJECT (widget), "desktop", desktop);

    GtkWidget *om = gr_vector_list (desktop, selection->isEmpty(), gr_selected, gr_multi);
    g_object_set_data (G_OBJECT (widget), "menu", om);
  
    gtk_box_pack_start (GTK_BOX (widget), om, TRUE, TRUE, 0);

    {
    GtkWidget *buttons = gtk_hbox_new(FALSE, 1);

    /* Fork */
    {
        GtkWidget *hb = gtk_hbox_new(FALSE, 1);
        GtkWidget *b = gtk_button_new_with_label(_("Duplicate"));
        gtk_tooltips_set_tip(tt, b, _("If the gradient is used by more than one object, create a copy of it for the selected object(s)"), NULL);
        gtk_widget_show(b);
        gtk_container_add(GTK_CONTAINER(hb), b);
        gtk_signal_connect(GTK_OBJECT(b), "clicked", GTK_SIGNAL_FUNC(gr_fork), widget);
        gtk_box_pack_start (GTK_BOX(buttons), hb, FALSE, FALSE, 0);
    }

    /* Edit... */
    {
        GtkWidget *hb = gtk_hbox_new(FALSE, 1);
        GtkWidget *b = gtk_button_new_with_label(_("Edit..."));
        gtk_tooltips_set_tip(tt, b, _("Edit the stops of the gradient"), NULL);
        gtk_widget_show(b);
        gtk_container_add(GTK_CONTAINER(hb), b);
        gtk_signal_connect(GTK_OBJECT(b), "clicked", GTK_SIGNAL_FUNC(gr_edit), widget);
        gtk_box_pack_start (GTK_BOX(buttons), hb, FALSE, FALSE, 0);
    }

    gtk_box_pack_end (GTK_BOX(widget), buttons, FALSE, FALSE, 0);
    g_object_set_data (G_OBJECT(widget), "buttons", buttons);
    gtk_widget_set_sensitive (buttons, (gr_selected && !gr_multi));
    }

    // connect to selection modified and changed signals
    sigc::connection *conn1 = new sigc::connection (selection->connectChanged(
        sigc::bind (
            sigc::ptr_fun(&gr_tb_selection_changed),
            (gpointer)widget )
    ));
    sigc::connection *conn2 = new sigc::connection (selection->connectModified(
        sigc::bind (
            sigc::ptr_fun(&gr_tb_selection_modified),
            (gpointer)widget )
    ));

    sigc::connection *conn3 = new sigc::connection (desktop->connectToolSubselectionChanged(
        sigc::bind (
            sigc::ptr_fun(&gr_drag_selection_changed),
            (gpointer)widget )
    ));

    // when widget is destroyed, disconnect
    g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gr_disconnect_sigc), conn1);
    g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gr_disconnect_sigc), conn2);
    g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gr_disconnect_sigc), conn3);

    // connect to release and modified signals of the defs (i.e. when someone changes gradient)
    g_signal_connect (G_OBJECT (SP_DOCUMENT_DEFS (document)), "release", G_CALLBACK (gr_defs_release), widget);
    g_signal_connect (G_OBJECT (SP_DOCUMENT_DEFS (document)), "modified", G_CALLBACK (gr_defs_modified), widget);

    // when widget is destroyed, disconnect
    g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gr_disconnect_gsignal), G_OBJECT (SP_DOCUMENT_DEFS (document)));
    g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gr_disconnect_gsignal), G_OBJECT (SP_DOCUMENT_DEFS (document)));

    gtk_widget_show_all (widget);
    return widget;
}

GtkWidget *
sp_gradient_toolbox_new(SPDesktop *desktop)
{
    GtkWidget *tbl = gtk_hbox_new(FALSE, 0);

    gtk_object_set_data(GTK_OBJECT(tbl), "dtw", desktop->canvas);
    gtk_object_set_data(GTK_OBJECT(tbl), "desktop", desktop);

    GtkTooltips *tt = gtk_tooltips_new();

    sp_toolbox_add_label(tbl, _("<b>New:</b>"));

    aux_toolbox_space(tbl, AUX_SPACING);

    {
    GtkWidget *cvbox = gtk_vbox_new (FALSE, 0);
    GtkWidget *cbox = gtk_hbox_new (FALSE, 0);

    {
    GtkWidget *button = sp_button_new_from_data( Inkscape::ICON_SIZE_DECORATION,
                                              SP_BUTTON_TYPE_TOGGLE,
                                              NULL,
                                              "fill_gradient",
                                              _("Create linear gradient"),
                                              tt);
    g_signal_connect_after (G_OBJECT (button), "clicked", G_CALLBACK (gr_toggle_type), tbl);
    g_object_set_data(G_OBJECT(tbl), "linear", button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
              prefs_get_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_LINEAR) == SP_GRADIENT_TYPE_LINEAR);
    gtk_box_pack_start(GTK_BOX(cbox), button, FALSE, FALSE, 0);
    }

    {
    GtkWidget *button = sp_button_new_from_data( Inkscape::ICON_SIZE_DECORATION,
                                              SP_BUTTON_TYPE_TOGGLE,
                                              NULL,
                                              "fill_radial",
                                              _("Create radial (elliptic or circular) gradient"),
                                              tt);
    g_signal_connect_after (G_OBJECT (button), "clicked", G_CALLBACK (gr_toggle_type), tbl);
    g_object_set_data(G_OBJECT(tbl), "radial", button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
              prefs_get_int_attribute ("tools.gradient", "newgradient", SP_GRADIENT_TYPE_LINEAR) == SP_GRADIENT_TYPE_RADIAL);
    gtk_box_pack_start(GTK_BOX(cbox), button, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(cvbox), cbox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tbl), cvbox, FALSE, FALSE, 0);
    }

    aux_toolbox_space(tbl, AUX_SPACING);

    sp_toolbox_add_label(tbl, _("on"), false);

    aux_toolbox_space(tbl, AUX_SPACING);

    {
        GtkWidget *cvbox = gtk_vbox_new (FALSE, 0);
    GtkWidget *cbox = gtk_hbox_new (FALSE, 0);

    {
    GtkWidget *button = sp_button_new_from_data( Inkscape::ICON_SIZE_DECORATION,
                                              SP_BUTTON_TYPE_TOGGLE,
                                              NULL,
                                              "controls_fill",
                                              _("Create gradient in the fill"),
                                              tt);
    g_signal_connect_after (G_OBJECT (button), "clicked", G_CALLBACK (gr_toggle_fillstroke), tbl);
    g_object_set_data(G_OBJECT(tbl), "fill", button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  prefs_get_int_attribute ("tools.gradient", "newfillorstroke", 1) == 1);
    gtk_box_pack_start(GTK_BOX(cbox), button, FALSE, FALSE, 0);
    }

    {
    GtkWidget *button = sp_button_new_from_data( Inkscape::ICON_SIZE_DECORATION,
                                              SP_BUTTON_TYPE_TOGGLE,
                                              NULL,
                                              "controls_stroke",
                                              _("Create gradient in the stroke"),
                                              tt);
    g_signal_connect_after (G_OBJECT (button), "clicked", G_CALLBACK (gr_toggle_fillstroke), tbl);
    g_object_set_data(G_OBJECT(tbl), "stroke", button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  prefs_get_int_attribute ("tools.gradient", "newfillorstroke", 1) == 0);
    gtk_box_pack_start(GTK_BOX(cbox), button, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(cvbox), cbox, TRUE, TRUE, 3);
    gtk_box_pack_start(GTK_BOX(tbl), cvbox, FALSE, FALSE, 0);
    }


    sp_toolbox_add_label(tbl, _("<b>Change:</b>"));

    aux_toolbox_space(tbl, AUX_SPACING);

    {
        GtkWidget *vectors = gr_change_widget (desktop);
        gtk_box_pack_start (GTK_BOX (tbl), vectors, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(tbl);
    sp_set_font_size_smaller (tbl);

    return tbl;
}




/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
