#define __LINE_GEOMETRY_C__

/*
 * Routines for dealing with lines (intersections, etc.)
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "line-geometry.h"
#include "inkscape.h"
#include "desktop-style.h"
#include "desktop-handles.h"
#include "display/sp-canvas.h"
#include "display/sodipodi-ctrl.h"
//#include "display/curve.cpp"

namespace Box3D {

/** 
 * Draw a line beginning at 'start'. If is_endpoint is true, use 'vec' as the endpoint
 * of the segment. Otherwise interpret it as the direction of the line.
 * FIXME: Think of a better way to distinguish between the two constructors of lines.
 */
Line::Line(NR::Point const &start, NR::Point const &vec, bool is_endpoint) {
    pt = start;
    if (is_endpoint)
        v_dir = vec - start;
    else
    	v_dir = vec;
    normal = v_dir.ccw();
    d0 = NR::dot(normal, pt);
}

Line::Line(Line const &line) {
    pt = line.pt;
    v_dir = line.v_dir;
    normal = line.normal;
    d0 = line.d0;
}

Line &Line::operator=(Line const &line) {
    pt = line.pt;
    v_dir = line.v_dir;
    normal = line.normal;
    d0 = line.d0;

    return *this;
}

NR::Maybe<NR::Point> Line::intersect(Line const &line) {
    NR::Coord denom = NR::dot(v_dir, line.normal);
    NR::Maybe<NR::Point> no_point = NR::Nothing();
    g_return_val_if_fail(fabs(denom) > 1e-6, no_point );

    NR::Coord lambda = (line.d0 - NR::dot(pt, line.normal)) / denom;
    return pt + lambda * v_dir;
}

void Line::set_direction(NR::Point const &dir)
{
    v_dir = dir;
    normal = v_dir.ccw();
    d0 = NR::dot(normal, pt);
}

NR::Point Line::closest_to(NR::Point const &pt)
{
	/* return the intersection of this line with a perpendicular line passing through pt */ 
    NR::Maybe<NR::Point> result = this->intersect(Line(pt, (this->v_dir).ccw(), false));
    g_return_val_if_fail (result, NR::Point (0.0, 0.0));
    return *result;
}

double Line::lambda (NR::Point const pt)
{
    double sign = (NR::dot (pt - this->pt, this->v_dir) > 0) ? 1.0 : -1.0;
    double lambda = sign * NR::L2 (pt - this->pt);
    // FIXME: It may speed things up (but how much?) if we assume that
    //        pt lies on the line and thus skip the following test
    NR::Point test = point_from_lambda (lambda);
    if (!pts_coincide (pt, test)) {
        g_warning ("Point does not lie on line.\n");
        return 0;
    }
    return lambda;
}

inline static double determinant (NR::Point const &a, NR::Point const &b)
{
    return (a[NR::X] * b[NR::Y] - a[NR::Y] * b[NR::X]);
}

/* The coordinates of w with respect to the basis {v1, v2} */
std::pair<double, double> coordinates (NR::Point const &v1, NR::Point const &v2, NR::Point const &w)
{
    double det = determinant (v1, v2);;
    if (fabs (det) < epsilon) {
        // vectors are not linearly independent; we indicate this in the return value(s)
        return std::make_pair (HUGE_VAL, HUGE_VAL);
    }

    double lambda1 = determinant (w, v2) / det;
    double lambda2 = determinant (v1, w) / det;
    return std::make_pair (lambda1, lambda2);
}

/* whether w lies inside the sector spanned by v1 and v2 */
bool lies_in_sector (NR::Point const &v1, NR::Point const &v2, NR::Point const &w)
{
    std::pair<double, double> coords = coordinates (v1, v2, w);
    if (coords.first == HUGE_VAL) {
        // catch the case that the vectors are not linearly independent
        // FIXME: Can we assume that it's safe to return true if the vectors point in different directions?
        return (NR::dot (v1, v2) < 0);
    }
    return (coords.first >= 0 and coords.second >= 0);
}

bool lies_in_quadrangle (NR::Point const &A, NR::Point const &B, NR::Point const &C, NR::Point const &D, NR::Point const &pt)
{
    return (lies_in_sector (D - A, B - A, pt - A) && lies_in_sector (D - C, B - C, pt - C));
}

static double pos_angle (NR::Point v, NR::Point w)
{
    return fabs (NR::atan2 (v) - NR::atan2 (w));
}

/*
 * Returns the two corners of the quadrangle A, B, C, D spanning the edge that is hit by a semiline
 * starting at pt and going into direction dir.
 * If none of the sides is hit, it returns a pair containing two identical points.
 */
std::pair<NR::Point, NR::Point>
side_of_intersection (NR::Point const &A, NR::Point const &B, NR::Point const &C, NR::Point const &D,
                      NR::Point const &pt, NR::Point const &dir)
{
    NR::Point dir_A (A - pt);
    NR::Point dir_B (B - pt);
    NR::Point dir_C (C - pt);
    NR::Point dir_D (D - pt);

    std::pair<NR::Point, NR::Point> result;
    double angle = -1;
    double tmp_angle;

    if (lies_in_sector (dir_A, dir_B, dir)) {
        result = std::make_pair (A, B);
        angle = pos_angle (dir_A, dir_B);
    }
    if (lies_in_sector (dir_B, dir_C, dir)) {
        tmp_angle = pos_angle (dir_B, dir_C);
        if (tmp_angle > angle) {
            angle = tmp_angle;
            result = std::make_pair (B, C);
        }
    }
    if (lies_in_sector (dir_C, dir_D, dir)) {
        tmp_angle = pos_angle (dir_C, dir_D);
        if (tmp_angle > angle) {
            angle = tmp_angle;
            result = std::make_pair (C, D);
        }
    }
    if (lies_in_sector (dir_D, dir_A, dir)) {
        tmp_angle = pos_angle (dir_D, dir_A);
        if (tmp_angle > angle) {
            angle = tmp_angle;
            result = std::make_pair (D, A);
        }
    }
    if (angle == -1) {
        // no intersection found; return a pair containing two identical points
        return std::make_pair (A, A);
    } else {
        return result;
    }
}

double cross_ratio (NR::Point const &A, NR::Point const &B, NR::Point const &C, NR::Point const &D)
{
    Line line (A, D);
    double lambda_A = line.lambda (A);
    double lambda_B = line.lambda (B);
    double lambda_C = line.lambda (C);
    double lambda_D = line.lambda (D);

    if (fabs (lambda_D - lambda_A) < epsilon || fabs (lambda_C - lambda_B) < epsilon) {
        // FIXME: What should we return if the cross ratio can't be computed?
        return 0;
        //return NR_HUGE;
    }
    return (((lambda_C - lambda_A) / (lambda_D - lambda_A)) * ((lambda_D - lambda_B) / (lambda_C - lambda_B)));
}

double cross_ratio (VanishingPoint const &V, NR::Point const &B, NR::Point const &C, NR::Point const &D)
{
    if (V.is_finite()) {
        return cross_ratio (V.get_pos(), B, C, D);
    } else {
        Line line (B, D);
        double lambda_B = line.lambda (B);
        double lambda_C = line.lambda (C);
        double lambda_D = line.lambda (D);

        if (fabs (lambda_C - lambda_B) < epsilon) {
            // FIXME: What should we return if the cross ratio can't be computed?
            return 0;
            //return NR_HUGE;
        }
        return (lambda_D - lambda_B) / (lambda_C - lambda_B);
    }
}

NR::Point fourth_pt_with_given_cross_ratio (NR::Point const &A, NR::Point const &C, NR::Point const &D, double gamma)
{
    Line line (A, D);
    double lambda_A = line.lambda (A);
    double lambda_C = line.lambda (C);
    double lambda_D = line.lambda (D);

    double beta = (lambda_C - lambda_A) / (lambda_D - lambda_A);
    if (fabs (beta - gamma) < epsilon) {
        // FIXME: How to handle the case when the point can't be computed?
        // g_warning ("Cannot compute point with given cross ratio.\n");
        return NR::Point (0.0, 0.0);
    }
    return line.point_from_lambda ((beta * lambda_D - gamma * lambda_C) / (beta - gamma));
}

void create_canvas_point(NR::Point const &pos, double size, guint32 rgba)
{
    SPDesktop *desktop = inkscape_active_desktop();
    SPCanvasItem * canvas_pt = sp_canvas_item_new(sp_desktop_controls(desktop), SP_TYPE_CTRL,
                          "size", size,
                          "filled", 1,
                          "fill_color", rgba,
                          "stroked", 1,
                          "stroke_color", 0x000000ff,
                          NULL);
    SP_CTRL(canvas_pt)->moveto(pos);
}

void create_canvas_line(NR::Point const &p1, NR::Point const &p2, guint32 rgba)
{
    SPDesktop *desktop = inkscape_active_desktop();
    SPCanvasItem *line = sp_canvas_item_new(sp_desktop_controls(desktop),
                                                            SP_TYPE_CTRLLINE, NULL);
    sp_ctrlline_set_coords(SP_CTRLLINE(line), p1, p2);
    sp_ctrlline_set_rgba32 (SP_CTRLLINE(line), rgba);
    sp_canvas_item_show (line);
}

} // namespace Box3D 
 
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
