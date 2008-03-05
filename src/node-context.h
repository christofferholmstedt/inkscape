#ifndef __SP_NODE_CONTEXT_H__
#define __SP_NODE_CONTEXT_H__

/*
 * Node editing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * This code is in public domain
 */

#include <sigc++/sigc++.h>
#include "event-context.h"
#include "forward.h"
#include "display/display-forward.h"
#include "nodepath.h"
struct SPKnotHolder;
namespace Inkscape { class Selection; }

#define SP_TYPE_NODE_CONTEXT            (sp_node_context_get_type ())
#define SP_NODE_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_NODE_CONTEXT, SPNodeContext))
#define SP_NODE_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_NODE_CONTEXT, SPNodeContextClass))
#define SP_IS_NODE_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_NODE_CONTEXT))
#define SP_IS_NODE_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_NODE_CONTEXT))

enum { SP_NODE_CONTEXT_INACTIVE,
       SP_NODE_CONTEXT_NODE_DRAGGING,
       SP_NODE_CONTEXT_RUBBERBAND_DRAGGING };

class SPNodeContext;
class SPNodeContextClass;

struct SPNodeContext {
	SPEventContext event_context;

	guint drag : 1;

	ShapeEditor* shape_editor;

	gboolean leftalt;
	gboolean rightalt;
	gboolean leftctrl;
	gboolean rightctrl;

      /// If true, rubberband was cancelled by esc, so the next button release should not deselect.
	bool rb_escaped;

	sigc::connection sel_changed_connection;

	Inkscape::MessageContext *_node_message_context;

	bool cursor_drag;

    bool added_node;
  
    unsigned int current_state;
 
    SPItem * flashed_item;
    Inkscape::Display::TemporaryItem * flash_tempitem;
    int remove_flash_counter;
};

struct SPNodeContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_node_context_get_type (void);

void sp_node_context_selection_changed (Inkscape::Selection * selection, gpointer data);

#endif
