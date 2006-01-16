#define __SP_DESKTOP_AFFINE_C__

/*
 * Editable view and widget implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "desktop.h"
#include "document.h"
#include "sp-root.h"
#include "libnr/nr-matrix-ops.h"

NR::Matrix const sp_desktop_root2dt_affine (SPDesktop const *dt)
{
	SPRoot const *root = SP_ROOT(SP_DOCUMENT_ROOT(dt->doc()));
	return root->c2p * dt->doc2dt();
}

NR::Matrix const sp_desktop_dt2root_affine (SPDesktop const *dt)
{
	return sp_desktop_root2dt_affine(dt).inverse();
}

NR::Point sp_desktop_root2dt_xy_point(SPDesktop const *dt, NR::Point const p)
{
	return p * sp_desktop_root2dt_affine(dt);
}

NR::Point sp_desktop_dt2root_xy_point(SPDesktop const *dt, NR::Point const p)
{
	return p * sp_desktop_dt2root_affine(dt);
}

