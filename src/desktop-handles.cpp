/*
 * Frontends
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/sp-canvas.h"
#include "display/sp-canvas-item.h"
#include "desktop.h"
#include "desktop-handles.h"

SPDocument *
sp_desktop_document (SPDesktop const * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);

	return desktop->doc();
}

SPCanvasGroup *
sp_desktop_guides (SPDesktop const * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);

	return desktop->guides;
}

SPCanvasGroup *
sp_desktop_controls (SPDesktop const * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);

	return desktop->controls;
}

Inkscape::MessageStack *
sp_desktop_message_stack (SPDesktop const * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);

	return desktop->messageStack();
}
