#ifndef SEEN_SP_GRADIENT_FNS_H
#define SEEN_SP_GRADIENT_FNS_H

/** \file
 * Macros and fn declarations related to gradients.
 */

#include <glib/gtypes.h>
#include <glib-object.h>
#include <2geom/forward.h>
#include "sp-gradient-spread.h"
#include "sp-gradient-units.h"

class SPGradient;

#define SP_GRADIENT_STATE_IS_SET(g) (SP_GRADIENT(g)->state != SP_GRADIENT_STATE_UNKNOWN)
#define SP_GRADIENT_IS_VECTOR(g) (SP_GRADIENT(g)->state == SP_GRADIENT_STATE_VECTOR)
#define SP_GRADIENT_IS_PRIVATE(g) (SP_GRADIENT(g)->state == SP_GRADIENT_STATE_PRIVATE)
#define SP_GRADIENT_HAS_STOPS(g) (SP_GRADIENT(g)->has_stops)
#define SP_GRADIENT_SPREAD(g) (SP_GRADIENT(g)->spread)
#define SP_GRADIENT_UNITS(g) (SP_GRADIENT(g)->units)

/** Forces vector to be built, if not present (i.e. changed) */
void sp_gradient_ensure_vector(SPGradient *gradient);

void sp_gradient_set_units(SPGradient *gr, SPGradientUnits units);
void sp_gradient_set_spread(SPGradient *gr, SPGradientSpread spread);

SPGradientSpread sp_gradient_get_spread (SPGradient *gradient);

/* Gradient repr methods */
void sp_gradient_repr_write_vector(SPGradient *gr);
void sp_gradient_repr_clear_vector(SPGradient *gr);

cairo_pattern_t *sp_gradient_create_preview_pattern(SPGradient *gradient, double width);

/** Transforms to/from gradient position space in given environment */
Geom::Matrix sp_gradient_get_g2d_matrix(SPGradient const *gr, Geom::Matrix const &ctm,
                                      Geom::Rect const &bbox);
Geom::Matrix sp_gradient_get_gs2d_matrix(SPGradient const *gr, Geom::Matrix const &ctm,
                                       Geom::Rect const &bbox);
void sp_gradient_set_gs2d_matrix(SPGradient *gr, Geom::Matrix const &ctm, Geom::Rect const &bbox,
                                 Geom::Matrix const &gs2d);


#endif /* !SEEN_SP_GRADIENT_FNS_H */

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
