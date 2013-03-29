#ifndef SEEN_CONNECTOR_CONTEXT_H
#define SEEN_CONNECTOR_CONTEXT_H

/*
 * Connector creation tool
 *
 * Authors:
 *   Michael Wybrow <mjwybrow@users.sourceforge.net>
 *
 * Copyright (C) 2005 Michael Wybrow
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <stddef.h>
#include <sigc++/sigc++.h>
#include <sigc++/connection.h>
#include "event-context.h"
#include <2geom/point.h>
#include "libavoid/connector.h"
#include <glibmm/i18n.h>

#define SP_TYPE_CONNECTOR_CONTEXT (sp_connector_context_get_type())
#define SP_CONNECTOR_CONTEXT(o) (G_TYPE_CHECK_INSTANCE_CAST((o), SP_TYPE_CONNECTOR_CONTEXT, SPConnectorContext))
#define SP_CONNECTOR_CONTEXT_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), SP_TYPE_CONNECTOR_CONTEXT, SPConnectorContextClass))
#define SP_IS_CONNECTOR_CONTEXT(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), SP_TYPE_CONNECTOR_CONTEXT))
#define SP_IS_CONNECTOR_CONTEXT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), SP_TYPE_CONNECTOR_CONTEXT))

struct SPKnot;
class SPCurve;

namespace Inkscape
{
  class Selection;
}

enum {
    SP_CONNECTOR_CONTEXT_IDLE,
    SP_CONNECTOR_CONTEXT_DRAGGING,
    SP_CONNECTOR_CONTEXT_CLOSE,
    SP_CONNECTOR_CONTEXT_STOP,
    SP_CONNECTOR_CONTEXT_REROUTING,
    SP_CONNECTOR_CONTEXT_NEWCONNPOINT
};

typedef std::map<SPKnot *, int>  SPKnotList;

struct SPConnectorContext : public SPEventContext {
    Inkscape::Selection *selection;
    Geom::Point p[5];

    /** \invar npoints in {0, 2}. */
    gint npoints;
    unsigned int state : 4;

    // Red curve
    SPCanvasItem *red_bpath;
    SPCurve *red_curve;
    guint32 red_color;

    // Green curve
    SPCurve *green_curve;

    // The new connector
    SPItem *newconn;
    Avoid::ConnRef *newConnRef;
    gdouble curvature;
    bool isOrthogonal;

    // The active shape
    SPItem *active_shape;
    Inkscape::XML::Node *active_shape_repr;
    Inkscape::XML::Node *active_shape_layer_repr;

    // Same as above, but for the active connector
    SPItem *active_conn;
    Inkscape::XML::Node *active_conn_repr;
    sigc::connection sel_changed_connection;

    // The activehandle
    SPKnot *active_handle;

    // The selected handle, used in editing mode
    SPKnot *selected_handle;

    SPItem *clickeditem;
    SPKnot *clickedhandle;

    SPKnotList knots;
    SPKnot *endpt_handle[2];
    guint  endpt_handler_id[2];
    gchar *shref;
    gchar *ehref;
    SPCanvasItem *c0, *c1, *cl0, *cl1;
};

struct SPConnectorContextClass : public SPEventContextClass { };

GType sp_connector_context_get_type();

void cc_selection_set_avoid(bool const set_ignore);
void cc_create_connection_point(SPConnectorContext* cc);
void cc_remove_connection_point(SPConnectorContext* cc);
bool cc_item_is_connector(SPItem *item);


#endif /* !SEEN_CONNECTOR_CONTEXT_H */

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
