#ifndef SEEN_SP_DESKTOP_WIDGET_H
#define SEEN_SP_DESKTOP_WIDGET_H

/** \file
 * SPDesktopWidget: handling Gtk events on a desktop.
 *
 * Authors:
 *      John Bintz <jcoswell@coswellproductions.org> (c) 2006
 *      Ralf Stephan <ralf@ark.in-berlin.de> (c) 2005, distrib. under GPL2
 *      ? -2004
 */

#include <gtk/gtktooltips.h>
#include <gtk/gtkwindow.h>

#include "display/display-forward.h"
#include "libnr/nr-point.h"
#include "forward.h"
#include "message.h"
#include "ui/view/view-widget.h"
#include "ui/view/edit-widget-interface.h"

#include <sigc++/connection.h>

#define SP_TYPE_DESKTOP_WIDGET (sp_desktop_widget_get_type ())
#define SP_DESKTOP_WIDGET(o) (GTK_CHECK_CAST ((o), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidget))
#define SP_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidgetClass))
#define SP_IS_DESKTOP_WIDGET(o) (GTK_CHECK_TYPE ((o), SP_TYPE_DESKTOP_WIDGET))
#define SP_IS_DESKTOP_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_DESKTOP_WIDGET))

GtkType sp_desktop_widget_get_type();

void sp_desktop_widget_destroy (SPDesktopWidget* dtw);

void sp_desktop_widget_show_decorations(SPDesktopWidget *dtw, gboolean show);
void sp_desktop_widget_iconify(SPDesktopWidget *dtw);
void sp_desktop_widget_maximize(SPDesktopWidget *dtw);
void sp_desktop_widget_fullscreen(SPDesktopWidget *dtw);
void sp_desktop_widget_layout(SPDesktopWidget *dtw);
void sp_desktop_widget_update_zoom(SPDesktopWidget *dtw);
void sp_desktop_widget_update_rulers (SPDesktopWidget *dtw);

/* Show/hide rulers & scrollbars */
void sp_desktop_widget_toggle_rulers (SPDesktopWidget *dtw);
void sp_desktop_widget_toggle_scrollbars (SPDesktopWidget *dtw);
void sp_desktop_widget_update_scrollbars (SPDesktopWidget *dtw, double scale);

void sp_dtw_desktop_activate (SPDesktopWidget *dtw);
void sp_dtw_desktop_deactivate (SPDesktopWidget *dtw);

namespace Inkscape { namespace Widgets { class LayerSelector; } }

namespace Inkscape { namespace UI { namespace Widget { class SelectedStyle; } } }

/// A GtkEventBox on an SPDesktop.
struct SPDesktopWidget {
    SPViewWidget viewwidget;

    unsigned int update : 1;

    sigc::connection modified_connection;

    GtkTooltips *tt;

    SPDesktop *desktop;

    Gtk::Window *window;
    
    // The root vbox of the window layout.
    GtkWidget *vbox;

    GtkWidget *menubar, *statusbar;

    GtkWidget *panels;

    GtkWidget *hscrollbar, *vscrollbar, *vscrollbar_box;

    GtkWidget *tool_toolbox, *aux_toolbox, *commands_toolbox;

    /* Rulers */
    GtkWidget *hruler, *vruler;
    GtkWidget *hruler_box, *vruler_box; // eventboxes for setting tooltips

    GtkWidget *sticky_zoom;
    GtkWidget *coord_status;
    GtkWidget *coord_status_x;
    GtkWidget *coord_status_y;
    GtkWidget *select_status;
    GtkWidget *select_status_eventbox;
    GtkWidget *zoom_status;
    gulong zoom_update;

    Inkscape::UI::Widget::SelectedStyle *selected_style;

    gint coord_status_id, select_status_id;
    
    unsigned int _interaction_disabled_counter;

    SPCanvas *canvas;
    NR::Point ruler_origin;
    double dt2r;

    GtkAdjustment *hadj, *vadj;

    Inkscape::Widgets::LayerSelector *layer_selector;

    struct WidgetStub : public Inkscape::UI::View::EditWidgetInterface {
        SPDesktopWidget *_dtw;
        SPDesktop       *_dt;
        WidgetStub (SPDesktopWidget* dtw) : _dtw(dtw) {}

        virtual void setTitle (gchar const *uri)
            { _dtw->updateTitle (uri); }
        virtual Gtk::Window* getWindow()
            { return _dtw->window; }
        virtual void layout() 
            { sp_desktop_widget_layout (_dtw); }
        virtual void present() 
            { _dtw->presentWindow(); }
        virtual void getGeometry (gint &x, gint &y, gint &w, gint &h)
            { _dtw->getWindowGeometry (x, y, w, h); }
        virtual void setSize (gint w, gint h)
            { _dtw->setWindowSize (w, h); }
        virtual void setPosition (NR::Point p)
            { _dtw->setWindowPosition (p); }
        virtual void setTransient (void* p, int transient_policy)
            { _dtw->setWindowTransient (p, transient_policy); }
        virtual NR::Point getPointer()
            { return _dtw->window_get_pointer(); }
        virtual void setIconified()
            { sp_desktop_widget_iconify (_dtw); }
        virtual void setMaximized()
            { sp_desktop_widget_maximize (_dtw); }
        virtual void setFullscreen()
            { sp_desktop_widget_fullscreen (_dtw); }
        virtual bool shutdown()
            { return _dtw->shutdown(); }
        virtual void destroy()
            {
				if(_dtw->window != NULL)
        			delete _dtw->window;
				_dtw->window = NULL;
			}
        
        virtual void requestCanvasUpdate()
            { _dtw->requestCanvasUpdate(); }
        virtual void requestCanvasUpdateAndWait()
            { _dtw->requestCanvasUpdateAndWait(); }
        virtual void enableInteraction()
            { _dtw->enableInteraction(); }
        virtual void disableInteraction()
            { _dtw->disableInteraction(); }
        virtual void activateDesktop()
            { sp_dtw_desktop_activate (_dtw); }
        virtual void deactivateDesktop()
            { sp_dtw_desktop_deactivate (_dtw); }
        virtual void viewSetPosition (NR::Point p)
            { _dtw->viewSetPosition (p); }
        virtual void updateRulers()
            { sp_desktop_widget_update_rulers (_dtw); }
        virtual void updateScrollbars (double scale)
            { sp_desktop_widget_update_scrollbars (_dtw, scale); }
        virtual void toggleRulers()
            { sp_desktop_widget_toggle_rulers (_dtw); }
        virtual void toggleScrollbars()
            { sp_desktop_widget_toggle_scrollbars (_dtw); }
        virtual void updateZoom()
            { sp_desktop_widget_update_zoom (_dtw); }
        virtual void letZoomGrabFocus()
            { _dtw->letZoomGrabFocus(); }
        virtual void setToolboxFocusTo (const gchar * id)
            { _dtw->setToolboxFocusTo (id); }
        virtual void setToolboxAdjustmentValue (const gchar *id, double val)
            { _dtw->setToolboxAdjustmentValue (id, val); }
        virtual bool isToolboxButtonActive (gchar const* id)
            { return _dtw->isToolboxButtonActive (id); }
        virtual void setCoordinateStatus (NR::Point p)
            { _dtw->setCoordinateStatus (p); }
        virtual void setMessage (Inkscape::MessageType type, gchar const* msg)
            { _dtw->setMessage (type, msg); }
        virtual bool warnDialog (gchar* text)
            { return _dtw->warnDialog (text); }
    };

    WidgetStub *stub;
    
    void setMessage(Inkscape::MessageType type, gchar const *message);
    NR::Point window_get_pointer();
    bool shutdown();
    void viewSetPosition (NR::Point p);
    void letZoomGrabFocus();
    void getWindowGeometry (gint &x, gint &y, gint &w, gint &h);
    void setWindowPosition (NR::Point p);
    void setWindowSize (gint w, gint h);
    void setWindowTransient (void *p, int transient_policy);
    void presentWindow();
    bool warnDialog (gchar *text);
    void setToolboxFocusTo (gchar const *);
    void setToolboxAdjustmentValue (gchar const * id, double value);
    bool isToolboxButtonActive (gchar const *id);
    void setCoordinateStatus(NR::Point p);
    void requestCanvasUpdate();
    void requestCanvasUpdateAndWait();
    void enableInteraction();
    void disableInteraction();
    void updateTitle(gchar const *uri);
	
	bool onFocusInEvent(GdkEventFocus*);
};

/// The SPDesktopWidget vtable
struct SPDesktopWidgetClass {
    SPViewWidgetClass parent_class;
};

#endif /* !SEEN_SP_DESKTOP_WIDGET_H */

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
