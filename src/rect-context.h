#ifndef __SP_RECT_CONTEXT_H__
#define __SP_RECT_CONTEXT_H__

/*
 * Rectangle drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <stddef.h>
#include <sigc++/sigc++.h>
#include <2geom/point.h>
#include "event-context.h"

#define SP_TYPE_RECT_CONTEXT            (sp_rect_context_get_type ())
#define SP_RECT_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_RECT_CONTEXT, SPRectContext))
#define SP_RECT_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_RECT_CONTEXT, SPRectContextClass))
#define SP_IS_RECT_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_RECT_CONTEXT))
#define SP_IS_RECT_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_RECT_CONTEXT))

class CRectContext;

class SPRectContext : public SPEventContext {
public:
	CRectContext* crectcontext;

	SPItem *item;
	Geom::Point center;

  	gdouble rx;	/* roundness radius (x direction) */
  	gdouble ry;	/* roundness radius (y direction) */

	sigc::connection sel_changed_connection;

	Inkscape::MessageContext *_message_context;
};

struct SPRectContextClass {
	SPEventContextClass parent_class;
};

class CRectContext : public CEventContext {
public:
	CRectContext(SPRectContext* rectcontext);

	virtual void setup();
	virtual void finish();
	virtual void set(Inkscape::Preferences::Entry* val);
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

protected:
	SPRectContext* sprectcontext;
};


/* Standard Gtk function */

GType sp_rect_context_get_type (void);

#endif
