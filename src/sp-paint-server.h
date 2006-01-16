#ifndef __SP_PAINT_SERVER_H__
#define __SP_PAINT_SERVER_H__

/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-pixblock.h>
#include "sp-object.h"



class SPPainter;

#define SP_TYPE_PAINT_SERVER (sp_paint_server_get_type ())
#define SP_PAINT_SERVER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_PAINT_SERVER, SPPaintServer))
#define SP_PAINT_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_PAINT_SERVER, SPPaintServerClass))
#define SP_IS_PAINT_SERVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_PAINT_SERVER))
#define SP_IS_PAINT_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_PAINT_SERVER))

typedef enum {
	SP_PAINTER_IND,
	SP_PAINTER_DEP
} SPPainterType;

typedef void (* SPPainterFillFunc) (SPPainter *painter, NRPixBlock *pb);

/* fixme: I do not like that class thingie (Lauris) */
struct SPPainter {
	SPPainter *next;
	SPPaintServer *server;
	GType server_type;
	SPPainterType type;
	SPPainterFillFunc fill;
};

struct SPPaintServer : public SPObject {
	/* List of paints */
	SPPainter *painters;
};

struct SPPaintServerClass {
	SPObjectClass sp_object_class;
	/* Get SPPaint instance */
	SPPainter * (* painter_new) (SPPaintServer *ps, NR::Matrix const &full_transform, NR::Matrix const &parent_transform, const NRRect *bbox);
	/* Free SPPaint instance */
	void (* painter_free) (SPPaintServer *ps, SPPainter *painter);
};

GType sp_paint_server_get_type (void);

SPPainter *sp_paint_server_painter_new (SPPaintServer *ps, NR::Matrix const &full_transform, NR::Matrix const &parent_transform, const NRRect *bbox);

SPPainter *sp_painter_free (SPPainter *painter);



#endif
