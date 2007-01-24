#ifndef __SP_GRADIENT_CHEMISTRY_H__
#define __SP_GRADIENT_CHEMISTRY_H__

/*
 * Various utility methods for gradients
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"
#include "sp-gradient.h"

/*
 * Either normalizes given gradient to vector, or returns fresh normalized
 * vector - in latter case, original gradient is flattened and stops cleared
 * No transrefing - i.e. caller shouldn't hold reference to original and
 * does not get one to new automatically (doc holds ref of every object anyways)
 */

SPGradient *sp_gradient_ensure_vector_normalized (SPGradient *gradient);


/*
 * Sets item fill/stroke to lineargradient with given vector, creating
 * new private gradient, if needed
 * gr has to be normalized vector
 */

SPGradient *sp_item_set_gradient (SPItem *item, SPGradient *gr, SPGradientType type, bool is_fill);

/*
 * Get default normalized gradient vector of document, create if there is none
 */

SPGradient *sp_document_default_gradient_vector (SPDocument *document, guint32 color = 0);
SPGradient *sp_gradient_vector_for_object (SPDocument *doc, SPDesktop *desktop, SPObject *o, bool is_fill);

void sp_object_ensure_fill_gradient_normalized (SPObject *object);
void sp_object_ensure_stroke_gradient_normalized (SPObject *object);

SPGradient *sp_gradient_convert_to_userspace (SPGradient *gr, SPItem *item, const gchar *property);
SPGradient *sp_gradient_reset_to_userspace (SPGradient *gr, SPItem *item);

SPGradient *sp_gradient_fork_vector_if_necessary (SPGradient *gr);

SPStop* sp_first_stop(SPGradient *gradient);
SPStop* sp_last_stop(SPGradient *gradient);
SPStop* sp_prev_stop(SPStop *stop, SPGradient *gradient);
SPStop* sp_next_stop(SPStop *stop);
SPStop* sp_get_stop_i(SPGradient *gradient, guint i);

void sp_gradient_transform_multiply (SPGradient *gradient, NR::Matrix postmul, bool set);

SPGradient * sp_item_gradient (SPItem *item, bool fill_or_stroke);
void sp_item_gradient_set_coords (SPItem *item, guint point_type, guint point_i, NR::Point p_desk, bool fill_or_stroke, bool write_repr, bool scale);
NR::Point sp_item_gradient_get_coords (SPItem *item, guint point_type, guint point_i, bool fill_or_stroke);
SPGradient *sp_item_gradient_get_vector (SPItem *item, bool fill_or_stroke);
SPGradientSpread sp_item_gradient_get_spread (SPItem *item, bool fill_or_stroke);

struct SPCSSAttr;
void sp_item_gradient_stop_set_style (SPItem *item, guint point_type, guint point_i, bool fill_or_stroke, SPCSSAttr *stop);
guint32 sp_item_gradient_stop_query_style (SPItem *item, guint point_type, guint point_i, bool fill_or_stroke);
void sp_item_gradient_edit_stop (SPItem *item, guint point_type, guint point_i, bool fill_or_stroke);
void sp_item_gradient_reverse_vector (SPItem *item, bool fill_or_stroke);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
