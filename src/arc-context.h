#ifndef SEEN_ARC_CONTEXT_H
#define SEEN_ARC_CONTEXT_H

/*
 * Ellipse drawing context
 *
 * Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Mitsuru Oka
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <stddef.h>
#include <sigc++/connection.h>

#include <2geom/point.h>
#include "event-context.h"

#define SP_TYPE_ARC_CONTEXT            (sp_arc_context_get_type())
//#define SP_ARC_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_ARC_CONTEXT, SPArcContext))
#define SP_ARC_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_ARC_CONTEXT, SPArcContextClass))
//#define SP_IS_ARC_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_ARC_CONTEXT))
#define SP_IS_ARC_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_ARC_CONTEXT))
#define SP_ARC_CONTEXT(obj) ((SPArcContext*)obj)
#define SP_IS_ARC_CONTEXT(obj) (((SPEventContext*)obj)->types.count(typeid(SPArcContext)))

class CArcContext;

class SPArcContext : public SPEventContext {
public:
	SPArcContext();
	CArcContext* carccontext;

    SPItem *item;
    Geom::Point center;

    sigc::connection sel_changed_connection;

    Inkscape::MessageContext *_message_context;

	static const std::string prefsPath;
};

struct SPArcContextClass {
    SPEventContextClass parent_class;
};

class CArcContext : public CEventContext {
public:
	CArcContext(SPArcContext* arccontext);

	virtual void setup();
	virtual void finish();
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
private:
	SPArcContext* sparccontext;
};

/* Standard Gtk function */

GType sp_arc_context_get_type(void);


#endif /* !SEEN_ARC_CONTEXT_H */

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
