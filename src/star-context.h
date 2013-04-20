#ifndef __SP_STAR_CONTEXT_H__
#define __SP_STAR_CONTEXT_H__

/*
 * Star drawing context
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Mitsuru Oka
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <stddef.h>
#include <sigc++/sigc++.h>
#include <2geom/point.h>
#include "event-context.h"

#include "sp-star.h"

#define SP_STAR_CONTEXT(obj) ((SPStarContext*)obj)
#define SP_IS_STAR_CONTEXT(obj) (dynamic_cast<const SPStarContext*>((const SPEventContext*)obj))

class SPStarContext : public SPEventContext {
public:
	SPStarContext();
	virtual ~SPStarContext();

	static const std::string prefsPath;

	virtual void setup();
	virtual void finish();
	virtual void set(Inkscape::Preferences::Entry* val);
	virtual gint root_handler(GdkEvent* event);

	virtual const std::string& getPrefsPath();

private:
	SPStar* star;

	Geom::Point center;

    /* Number of corners */
    gint magnitude;

    /* Outer/inner radius ratio */
    gdouble proportion;

    /* flat sides or not? */
    bool isflatsided;

    /* rounded corners ratio */
    gdouble rounded;

    // randomization
    gdouble randomized;

    sigc::connection sel_changed_connection;

    Inkscape::MessageContext *_message_context;

	void drag(Geom::Point p, guint state);
	void finishItem();
	void cancel();
	void selection_changed(Inkscape::Selection* selection);
};

#endif
