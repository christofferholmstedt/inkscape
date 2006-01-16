#define __SP_COLOR_PREVIEW_C__

/*
 * A simple color preview widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "../display/nr-plain-stuff-gdk.h"
#include "sp-color-preview.h"

#define SPCP_DEFAULT_WIDTH 32
#define SPCP_DEFAULT_HEIGHT 11

static void sp_color_preview_class_init (SPColorPreviewClass *klass);
static void sp_color_preview_init (SPColorPreview *image);
static void sp_color_preview_destroy (GtkObject *object);

static void sp_color_preview_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_color_preview_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint sp_color_preview_expose (GtkWidget *widget, GdkEventExpose *event);

static void sp_color_preview_paint (SPColorPreview *cp, GdkRectangle *area);

static GtkWidgetClass *parent_class;

GtkType
sp_color_preview_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPColorPreview",
			sizeof (SPColorPreview),
			sizeof (SPColorPreviewClass),
			(GtkClassInitFunc) sp_color_preview_class_init,
			(GtkObjectInitFunc) sp_color_preview_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_color_preview_class_init (SPColorPreviewClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = (GtkWidgetClass*)gtk_type_class (GTK_TYPE_WIDGET);

	object_class->destroy = sp_color_preview_destroy;

	widget_class->size_request = sp_color_preview_size_request;
	widget_class->size_allocate = sp_color_preview_size_allocate;
	widget_class->expose_event = sp_color_preview_expose;
}

static void
sp_color_preview_init (SPColorPreview *image)
{
	GTK_WIDGET_SET_FLAGS (image, GTK_NO_WINDOW);

	image->rgba = 0xffffffff;
}

static void
sp_color_preview_destroy (GtkObject *object)
{
	SPColorPreview *image;

	image = SP_COLOR_PREVIEW (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_color_preview_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPColorPreview *slider;

	slider = SP_COLOR_PREVIEW (widget);

	requisition->width = SPCP_DEFAULT_WIDTH;
	requisition->height = SPCP_DEFAULT_HEIGHT;
}

static void
sp_color_preview_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPColorPreview *image;

	image = SP_COLOR_PREVIEW (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_DRAWABLE (image)) {
		gtk_widget_queue_draw (GTK_WIDGET (image));
	}
}

static gint
sp_color_preview_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPColorPreview *cp;

	cp = SP_COLOR_PREVIEW (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_color_preview_paint (cp, &event->area);
	}

	return TRUE;
}

GtkWidget *
sp_color_preview_new (guint32 rgba)
{
	SPColorPreview *image;

	image = (SPColorPreview*)gtk_type_new (SP_TYPE_COLOR_PREVIEW);

	sp_color_preview_set_rgba32 (image, rgba);

	return (GtkWidget *) image;
}

void
sp_color_preview_set_rgba32 (SPColorPreview *cp, guint32 rgba)
{
	cp->rgba = rgba;

	if (GTK_WIDGET_DRAWABLE (cp)) {
		gtk_widget_queue_draw (GTK_WIDGET (cp));
	}
}

static void
sp_color_preview_paint (SPColorPreview *cp, GdkRectangle *area)
{
	GtkWidget *widget;
	GdkRectangle warea, carea;
	GdkRectangle wpaint, cpaint;
	gint w2;

	widget = GTK_WIDGET (cp);

	warea.x = widget->allocation.x;
	warea.y = widget->allocation.y;
	warea.width = widget->allocation.width;
	warea.height = widget->allocation.height;

	if (!gdk_rectangle_intersect (area, &warea, &wpaint)) return;

	/* Transparent area */

	w2 = warea.width / 2;

	carea.x = warea.x;
	carea.y = warea.y;
	carea.width = w2;
	carea.height = warea.height;

	if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
		nr_gdk_draw_rgba32_solid (widget->window, widget->style->black_gc,
					  cpaint.x, cpaint.y,
					  cpaint.width, cpaint.height,
					  cp->rgba);
	}

	/* Solid area */

	carea.x = warea.x + w2;
	carea.y = warea.y;
	carea.width = warea.width - w2;
	carea.height = warea.height;

	if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
		nr_gdk_draw_rgba32_solid (widget->window, widget->style->black_gc,
					  cpaint.x, cpaint.y,
					  cpaint.width, cpaint.height,
					  cp->rgba | 0xff);
	}
}

