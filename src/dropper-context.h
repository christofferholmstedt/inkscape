#ifndef __SP_DROPPER_CONTEXT_H__
#define __SP_DROPPER_CONTEXT_H__

/*
 * Tool for picking colors from drawing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "event-context.h"

G_BEGIN_DECLS

#define SP_TYPE_DROPPER_CONTEXT (sp_dropper_context_get_type ())
#define SP_DROPPER_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_DROPPER_CONTEXT, SPDropperContext))
#define SP_IS_DROPPER_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_DROPPER_CONTEXT))

enum {
      SP_DROPPER_PICK_VISIBLE,
      SP_DROPPER_PICK_ACTUAL  
};

class CDropperContext;

class SPDropperContext : public SPEventContext {
public:
	SPDropperContext();
	CDropperContext* cdroppercontext;

    SPEventContext event_context;

	static const std::string prefsPath;
};

struct SPDropperContextClass {
    SPEventContextClass parent_class;
};

class CDropperContext : public CEventContext {
public:
	CDropperContext(SPDropperContext* droppercontext);

	virtual void setup();
	virtual void finish();
	virtual gint root_handler(GdkEvent* event);

private:
	SPDropperContext* spdroppercontext;
};

GType sp_dropper_context_get_type (void);

guint32 sp_dropper_context_get_color(SPEventContext *ec);

G_END_DECLS

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
