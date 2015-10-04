/*
 * Our nice measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include <gdk/gdkkeysyms.h>
#include <boost/none_t.hpp>
#include "util/units.h"
#include "macros.h"
#include "rubberband.h"
#include "display/curve.h"
#include "sp-shape.h"
#include "sp-text.h"
#include "sp-flowtext.h"
#include "text-editing.h"
#include "display/sp-ctrlline.h"
#include "display/sodipodi-ctrl.h"
#include "display/sp-canvas-item.h"
#include "display/sp-canvas-util.h"
#include "desktop.h"
#include "document.h"
#include "pixmaps/cursor-measure.xpm"
#include "preferences.h"
#include "inkscape.h"

#include "ui/tools/measure-tool.h"
#include "ui/tools/freehand-base.h"
#include "display/canvas-text.h"
#include "path-chemistry.h"
#include "2geom/line.h"
#include <2geom/path-intersection.h>
#include <2geom/pathvector.h>
#include <2geom/crossing.h>
#include <2geom/angle.h>
#include "snap.h"
#include "sp-namedview.h"
#include "enums.h"
#include "ui/control-manager.h"
#include "knot-enums.h"

using Inkscape::ControlManager;
using Inkscape::CTLINE_SECONDARY;
using Inkscape::Util::unit_table;

namespace Inkscape {
namespace UI {
namespace Tools {

std::vector<Inkscape::Display::TemporaryItem*> measure_tmp_items;

const std::string& MeasureTool::getPrefsPath() {
	return MeasureTool::prefsPath;
}

const std::string MeasureTool::prefsPath = "/tools/measure";

namespace
{

gint const DIMENSION_OFFSET = 35;

/**
 * Simple class to use for removing label overlap.
 */
class LabelPlacement {
public:

    double lengthVal;
    double offset;
    Geom::Point start;
    Geom::Point end;
};

bool SortLabelPlacement(LabelPlacement const &first, LabelPlacement const &second)
{
    if (first.end[Geom::Y] == second.end[Geom::Y]) {
        return first.end[Geom::X] < second.end[Geom::X];
    } else {
        return first.end[Geom::Y] < second.end[Geom::Y];
    }
}

void repositionOverlappingLabels(std::vector<LabelPlacement> &placements, SPDesktop *desktop, Geom::Point const &normal, double fontsize)
{
    std::sort(placements.begin(), placements.end(), SortLabelPlacement);

    double border = 3;
    Geom::Rect box;
    {
        Geom::Point tmp(fontsize * 8 + (border * 2), fontsize + (border * 2));
        tmp = desktop->w2d(tmp);
        box = Geom::Rect(-tmp[Geom::X] / 2, -tmp[Geom::Y] / 2, tmp[Geom::X] / 2, tmp[Geom::Y] / 2);
    }

    // Using index since vector may be re-ordered as we go.
    // Starting at one, since the first item can't overlap itself
    for (size_t i = 1; i < placements.size(); i++) {
        LabelPlacement &place = placements[i];

        bool changed = false;
        do {
            Geom::Rect current(box + place.end);

            changed = false;
            bool overlaps = false;
            for (size_t j = i; (j > 0) && !overlaps; --j) {
                LabelPlacement &otherPlace = placements[j - 1];
                Geom::Rect target(box + otherPlace.end);
                if (current.intersects(target)) {
                    overlaps = true;
                }
            }
            if (overlaps) {
                place.offset += (fontsize + border);
                place.end = place.start - desktop->w2d(normal * place.offset);
                changed = true;
            }
        } while (changed);

        std::sort(placements.begin(), placements.begin() + i + 1, SortLabelPlacement);
    }
}

/**
 * Calculates where to place the anchor for the display text and arc.
 *
 * @param desktop the desktop that is being used.
 * @param angle the angle to be displaying.
 * @param baseAngle the angle of the initial baseline.
 * @param startPoint the point that is the vertex of the selected angle.
 * @param endPoint the point that is the end the user is manipulating for measurement.
 * @param fontsize the size to display the text label at.
 */
Geom::Point calcAngleDisplayAnchor(SPDesktop *desktop, double angle, double baseAngle,
                                   Geom::Point const &startPoint, Geom::Point const &endPoint,
                                   double fontsize)
{
    // Time for the trick work of figuring out where things should go, and how.
    double lengthVal = (endPoint - startPoint).length();
    double effective = baseAngle + (angle / 2);
    Geom::Point where(lengthVal, 0);
    where *= Geom::Affine(Geom::Rotate(effective)) * Geom::Affine(Geom::Translate(startPoint));

    // When the angle is tight, the label would end up under the cursor and/or lines. Bump it
    double scaledFontsize = std::abs(fontsize * desktop->w2d(Geom::Point(0, 1.0))[Geom::Y]);
    if (std::abs((where - endPoint).length()) < scaledFontsize) {
        where[Geom::Y] += scaledFontsize * 2;
    }

    // We now have the ideal position, but need to see if it will fit/work.

    Geom::Rect visibleArea = desktop->get_display_area();
    // Bring it in to "title safe" for the anchor point
    Geom::Point textBox = desktop->w2d(Geom::Point(fontsize * 3, fontsize / 2));
    textBox[Geom::Y] = std::abs(textBox[Geom::Y]);

    visibleArea = Geom::Rect(visibleArea.min()[Geom::X] + textBox[Geom::X],
                             visibleArea.min()[Geom::Y] + textBox[Geom::Y],
                             visibleArea.max()[Geom::X] - textBox[Geom::X],
                             visibleArea.max()[Geom::Y] - textBox[Geom::Y]);

    where[Geom::X] = std::min(where[Geom::X], visibleArea.max()[Geom::X]);
    where[Geom::X] = std::max(where[Geom::X], visibleArea.min()[Geom::X]);
    where[Geom::Y] = std::min(where[Geom::Y], visibleArea.max()[Geom::Y]);
    where[Geom::Y] = std::max(where[Geom::Y], visibleArea.min()[Geom::Y]);

    return where;
}

/**
 * Given an angle, the arc center and edge point, draw an arc segment centered around that edge point.
 *
 * @param desktop the desktop that is being used.
 * @param center the center point for the arc.
 * @param end the point that ends at the edge of the arc segment.
 * @param anchor the anchor point for displaying the text label.
 * @param angle the angle of the arc segment to draw.
 */
void createAngleDisplayCurve(SPDesktop *desktop, Geom::Point const &center, Geom::Point const &end, Geom::Point const &anchor, double angle)
{
    // Given that we have a point on the arc's edge and the angle of the arc, we need to get the two endpoints.

    double textLen = std::abs((anchor - center).length());
    double sideLen = std::abs((end - center).length());
    if (sideLen > 0.0) {
        double factor = std::min(1.0, textLen / sideLen);
    
        // arc start
        Geom::Point p1 = end * (Geom::Affine(Geom::Translate(-center))
                                * Geom::Affine(Geom::Scale(factor))
                                * Geom::Affine(Geom::Translate(center)));

        // arc end
        Geom::Point p4 = p1 * (Geom::Affine(Geom::Translate(-center))
                               * Geom::Affine(Geom::Rotate(-angle))
                               * Geom::Affine(Geom::Translate(center)));

        // from Riskus
        double xc = center[Geom::X];
        double yc = center[Geom::Y];
        double ax = p1[Geom::X] - xc;
        double ay = p1[Geom::Y] - yc;
        double bx = p4[Geom::X] - xc;
        double by = p4[Geom::Y] - yc;
        double q1 = (ax * ax) + (ay * ay);
        double q2 = q1 + (ax * bx) + (ay * by);

        double k2 = (4.0 / 3.0) * (std::sqrt(2 * q1 * q2) - q2) / ((ax * by) - (ay * bx));

        Geom::Point p2(xc + ax - (k2 * ay),
                       yc + ay  + (k2 * ax));
        Geom::Point p3(xc + bx + (k2 * by),
                       yc + by - (k2 * bx));
        SPCtrlCurve *curve = ControlManager::getManager().createControlCurve(desktop->getTempGroup(), p1, p2, p3, p4, CTLINE_SECONDARY);

        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(SP_CANVAS_ITEM(curve), 0, true));
    }
}

} // namespace


MeasureTool::MeasureTool()
    : ToolBase(cursor_measure_xpm, 4, 4)
    , grabbed(NULL)
{
}

MeasureTool::~MeasureTool() {
}

void MeasureTool::finish() {
    this->enableGrDrag(false);

    if (this->grabbed) {
        sp_canvas_item_ungrab(this->grabbed, GDK_CURRENT_TIME);
        this->grabbed = NULL;
    }

    ToolBase::finish();
}

//void MeasureTool::setup() {
//	ToolBase* ec = this;
//
////    if (SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->setup) {
////        SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->setup(ec);
////    }
//	ToolBase::setup();
//}

//gint MeasureTool::item_handler(SPItem* item, GdkEvent* event) {
//    gint ret = FALSE;
//
////    if (SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->item_handler) {
////        ret = SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->item_handler(event_context, item, event);
////    }
//    ret = ToolBase::item_handler(item, event);
//
//    return ret;
//}

static void calculate_intersections(SPDesktop * /*desktop*/, SPItem* item, Geom::PathVector const &lineseg, SPCurve *curve, std::vector<double> &intersections)
{

    curve->transform(item->i2doc_affine());
    // Find all intersections of the control-line with this shape
    Geom::CrossingSet cs = Geom::crossings(lineseg, curve->get_pathvector());
    Geom::delete_duplicates(cs[0]);

    // Reconstruct and store the points of intersection
    for (Geom::Crossings::const_iterator m = cs[0].begin(); m != cs[0].end(); ++m) {
#if 0
//TODO: consider only visible intersections
        Geom::Point intersection = lineseg[0].pointAt((*m).ta);
        double eps = 0.0001;
        SPDocument* doc = desktop->getDocument();
        if (((*m).ta > eps &&
             item == doc->getItemAtPoint(desktop->dkey, lineseg[0].pointAt((*m).ta - eps), false, NULL)) ||
            ((*m).ta + eps < 1 &&
             item == doc->getItemAtPoint(desktop->dkey, lineseg[0].pointAt((*m).ta + eps), false, NULL)) ) {
            intersections.push_back((*m).ta);
        }
#else
        intersections.push_back((*m).ta);

#endif
    }
}

bool MeasureTool::root_handler(GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS: {
            Geom::Point const button_w(event->button.x, event->button.y);
            explicitBase = boost::none;
            last_end = boost::none;
            start_point = desktop->w2d(button_w);

            if (event->button.button == 1 && !this->space_panning) {
                // save drag origin
                start_point = desktop->w2d(Geom::Point(event->button.x, event->button.y));
                within_tolerance = true;

                ret = TRUE;
            }

            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop);
            m.freeSnapReturnByRef(start_point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
            m.unSetup();

            sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK,
                                NULL, event->button.time);
            this->grabbed = SP_CANVAS_ITEM(desktop->acetate);
            break;
        }
        case GDK_KEY_PRESS: {
            if ((event->key.keyval == GDK_KEY_Shift_L) || (event->key.keyval == GDK_KEY_Shift_R)) {
                if (last_end) {
                    explicitBase = last_end;
                }
            }
            break;
        }
        case GDK_MOTION_NOTIFY: {
            if (!(event->motion.state & GDK_BUTTON1_MASK) && !(event->motion.state & GDK_SHIFT_MASK)) {
                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point const motion_dt(desktop->w2d(motion_w));

                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);

                Inkscape::SnapCandidatePoint scp(motion_dt, Inkscape::SNAPSOURCE_OTHER_HANDLE);
                scp.addOrigin(start_point);

                m.preSnap(scp);
                m.unSetup();
            } else {
                ret = TRUE;
                Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
                Geom::Point const motion_w(event->motion.x, event->motion.y);
                if ( within_tolerance){
                    if ( Geom::LInfty( motion_w - start_point ) < tolerance) {
                        return false;   // Do not drag if we're within tolerance from origin.
                    }
                }
                // Once the user has moved farther than tolerance from the original location
                // (indicating they intend to move the object, not click), then always process the
                // motion notify coordinates as given (no snapping back to origin)
                within_tolerance = false;
                if(event->motion.time == 0 || !last_end  || Geom::LInfty( motion_w - *last_end ) > (tolerance/2)){
                    showCanvasItems(event->motion);
                    last_end = motion_w ;
                }
                gobble_motion_events(GDK_BUTTON1_MASK);
            }
            break;
        }
        case GDK_BUTTON_RELEASE: {
            sp_event_context_discard_delayed_snap_event(this);
            explicitBase = boost::none;
            last_end = boost::none;

            //clear all temporary canvas items related to the measurement tool.
            for (size_t idx = 0; idx < measure_tmp_items.size(); ++idx) {
                desktop->remove_temporary_canvasitem(measure_tmp_items[idx]);
            }

            measure_tmp_items.clear();

            if (this->grabbed) {
                sp_canvas_item_ungrab(this->grabbed, event->button.time);
                this->grabbed = NULL;
            }

            start_point = Geom::Point();
            break;
        }
        default:
            break;
    }
    if (!ret) {
    	ret = ToolBase::root_handler(event);
    }
    
    return ret;
}

void MeasureTool::showCanvasItems(GdkEventMotion const &mevent){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Geom::Point const motion_w(mevent.x, mevent.y);
    bool show_in_between = prefs->getBool("/tools/measure/show_in_between");
    bool all_layers = prefs->getBool("/tools/measure/all_layers");
    //clear previous temporary canvas items, we'll draw new ones
    for (size_t idx = 0; idx < measure_tmp_items.size(); ++idx) {
        desktop->remove_temporary_canvasitem(measure_tmp_items[idx]);
    }

    measure_tmp_items.clear();
    Geom::Point const motion_dt(desktop->w2d(motion_w));
    Geom::Point end_point = motion_dt;

    if (mevent.state & GDK_CONTROL_MASK) {
        spdc_endpoint_snap_rotation(this, end_point, start_point, mevent.state);
    } else {
        if (!(mevent.state & GDK_SHIFT_MASK)) {
            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop);
            Inkscape::SnapCandidatePoint scp(end_point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
            scp.addOrigin(start_point);
            Inkscape::SnappedPoint sp = m.freeSnap(scp);
            end_point = sp.getPoint();
            m.unSetup();
        }
    }

    Geom::PathVector lineseg;
    Geom::Path p;
    p.start(desktop->dt2doc(start_point));
    p.appendNew<Geom::LineSegment>(desktop->dt2doc(end_point));
    lineseg.push_back(p);

    double deltax = end_point[Geom::X] - start_point[Geom::X];
    double deltay = end_point[Geom::Y] - start_point[Geom::Y];
    double angle = atan2(deltay, deltax);
    double baseAngle = 0;

    if (explicitBase) {
        double deltax2 = explicitBase.get()[Geom::X] - start_point[Geom::X];
        double deltay2 = explicitBase.get()[Geom::Y] - start_point[Geom::Y];

        baseAngle = atan2(deltay2, deltax2);
        angle -= baseAngle;

        if (angle < -M_PI) {
            angle += 2 * M_PI;
        } else if (angle > M_PI) {
            angle -= 2 * M_PI;
        }
    }
    std::vector<SPItem*> items;
    Inkscape::Rubberband *r = Inkscape::Rubberband::get(desktop);
    r->setMode(RUBBERBAND_MODE_TOUCHPATH);
    if(!show_in_between){
        r->start(desktop,start_point);
        r->move(end_point);
        items = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers, 2);
        r->stop();
        r->setMode(RUBBERBAND_MODE_TOUCHPATH);
        r->start(desktop,end_point);
        r->move(start_point);
        std::vector<SPItem*> items_reverse = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers, 2);
        r->stop();
        if(items_reverse.size() == 2){
            items.push_back(items_reverse[1]);
        }
        if(items_reverse.size() >= 1){
            items.push_back(items_reverse[0]);
        }
    } else {
        r->start(desktop,start_point);
        r->move(end_point);
        items = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers);
        r->stop();
    }
    std::vector<double> intersection_times;
    for (std::vector<SPItem*>::const_iterator i=items.begin();i!=items.end();i++) {
        SPItem *item = *i;
        if (SP_IS_SHAPE(item)) {
           calculate_intersections(desktop, item, lineseg, SP_SHAPE(item)->getCurve(), intersection_times);
        } else {
            if (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item)) {
                Inkscape::Text::Layout::iterator iter = te_get_layout(item)->begin();
                do {
                    Inkscape::Text::Layout::iterator iter_next = iter;
                    iter_next.nextGlyph(); // iter_next is one glyph ahead from iter
                    if (iter == iter_next) {
                        break;
                    }

                    // get path from iter to iter_next:
                    SPCurve *curve = te_get_layout(item)->convertToCurves(iter, iter_next);
                    iter = iter_next; // shift to next glyph
                    if (!curve) {
                        continue; // error converting this glyph
                    }
                    if (curve->is_empty()) { // whitespace glyph?
                        curve->unref();
                        continue;
                    }

                    curve->transform(item->i2doc_affine());

                    calculate_intersections(desktop, item, lineseg, curve, intersection_times);
                    if (iter == te_get_layout(item)->end()) {
                        break;
                    }
                } while (true);
            }
        }
    }
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit");
    if (!unit_name.compare("")) {
        unit_name = "px";
    }

    double fontsize = prefs->getInt("/tools/measure/fontsize");

    // Normal will be used for lines and text
    Geom::Point windowNormal = Geom::unit_vector(Geom::rot90(desktop->d2w(end_point - start_point)));
    Geom::Point normal = desktop->w2d(windowNormal);

    std::vector<Geom::Point> intersections;
    std::sort(intersection_times.begin(), intersection_times.end());
    for (std::vector<double>::iterator iter_t = intersection_times.begin(); iter_t != intersection_times.end(); iter_t++) {
        if(show_in_between){
            intersections.push_back(lineseg[0].pointAt(*iter_t));
        }
    }
    if(!show_in_between && intersection_times.size() > 1){
        intersections.push_back(lineseg[0].pointAt(intersection_times[0]));
        intersections.push_back(lineseg[0].pointAt(intersection_times[intersection_times.size()-1]));
    }
    if (!prefs->getBool("/tools/measure/ignore_1st_and_last", true)) {
        intersections.insert(intersections.begin(),lineseg[0].pointAt(0));
        intersections.push_back(lineseg[0].pointAt(1));
    }
    std::vector<LabelPlacement> placements;
    for (size_t idx = 1; idx < intersections.size(); ++idx) {
        LabelPlacement placement;
        placement.lengthVal = (intersections[idx] - intersections[idx - 1]).length();
        placement.lengthVal = Inkscape::Util::Quantity::convert(placement.lengthVal, "px", unit_name);
        placement.offset = DIMENSION_OFFSET;
        placement.start = desktop->doc2dt( (intersections[idx - 1] + intersections[idx]) / 2 );
        placement.end = placement.start - (normal * placement.offset);

        placements.push_back(placement);
    }

    // Adjust positions
    repositionOverlappingLabels(placements, desktop, windowNormal, fontsize);
    for (std::vector<LabelPlacement>::iterator it = placements.begin(); it != placements.end(); ++it)
    {
        LabelPlacement &place = *it;

        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *measure_str = g_strdup_printf("%.2f %s", place.lengthVal, unit_name.c_str());
        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                                         desktop,
                                                         place.end,
                                                         measure_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x0000007f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_CENTER;

        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        g_free(measure_str);
    }

    Geom::Point angleDisplayPt = calcAngleDisplayAnchor(desktop, angle, baseAngle,
                                                        start_point, end_point,
                                                        fontsize);

    {
        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *angle_str = g_strdup_printf("%.2f °", angle * 180/M_PI);

        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                                         desktop,
                                                         angleDisplayPt,
                                                         angle_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x337f337f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_CENTER;

        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        g_free(angle_str);
    }

    {
        double totallengthval = (end_point - start_point).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);

        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *totallength_str = g_strdup_printf("%.2f %s", totallengthval, unit_name.c_str());
        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                                         desktop,
                                                         end_point + desktop->w2d(Geom::Point(3*fontsize, -fontsize)),
                                                         totallength_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x3333337f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_LEFT;

        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        g_free(totallength_str);
    }

    if (intersections.size() > 2) {
        double totallengthval = (intersections[intersections.size()-1] - intersections[0]).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);

        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *total_str = g_strdup_printf("%.2f %s", totallengthval, unit_name.c_str());
        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                                         desktop,
                                                         desktop->doc2dt((intersections[0] + intersections[intersections.size()-1])/2) + normal * 60,
                                                         total_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x33337f7f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_CENTER;

        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        g_free(total_str);
    }

    // Now that text has been added, we can add lines and controls so that they go underneath
    for (size_t idx = 0; idx < intersections.size(); ++idx) {
        // Display the intersection indicator (i.e. the cross)
        SPCanvasItem * canvasitem = sp_canvas_item_new(desktop->getTempGroup(),
                                                       SP_TYPE_CTRL,
                                                       "anchor", SP_ANCHOR_CENTER,
                                                       "size", 8.0,
                                                       "stroked", TRUE,
                                                       "stroke_color", 0xff0000ff,
                                                       "mode", SP_KNOT_MODE_XOR,
                                                       "shape", SP_KNOT_SHAPE_CROSS,
                                                       NULL );

        SP_CTRL(canvasitem)->moveto(desktop->doc2dt(intersections[idx]));
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvasitem, 0));
    }

    // Since adding goes to the bottom, do all lines last.

    // draw main control line
    {
        SPCtrlLine *control_line = ControlManager::getManager().createControlLine(desktop->getTempGroup(),
                                                                                  start_point,
                                                                                  end_point);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));

        if ((end_point[Geom::X] != start_point[Geom::X]) && (end_point[Geom::Y] != start_point[Geom::Y])) {
            double length = std::abs((end_point - start_point).length());
            Geom::Point anchorEnd = start_point;
            anchorEnd[Geom::X] += length;
            if (explicitBase) {
                anchorEnd *= (Geom::Affine(Geom::Translate(-start_point))
                              * Geom::Affine(Geom::Rotate(baseAngle))
                              * Geom::Affine(Geom::Translate(start_point)));
            }

            SPCtrlLine *control_line = ControlManager::getManager().createControlLine(desktop->getTempGroup(),
                                                                                      start_point,
                                                                                      anchorEnd,
                                                                                      CTLINE_SECONDARY);
            measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));

            createAngleDisplayCurve(desktop, start_point, end_point, angleDisplayPt, angle);
        }
    }

    if (intersections.size() > 2) {
        ControlManager &mgr = ControlManager::getManager();
        SPCtrlLine *control_line = 0;
        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[0]) + normal * 60,
                                             desktop->doc2dt(intersections[intersections.size() - 1]) + normal * 60);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));

        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[0]),
                                             desktop->doc2dt(intersections[0]) + normal * 65);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));

        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[intersections.size() - 1]),
                                             desktop->doc2dt(intersections[intersections.size() - 1]) + normal * 65);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
    }

    // call-out lines
    for (std::vector<LabelPlacement>::iterator it = placements.begin(); it != placements.end(); ++it)
    {
        LabelPlacement &place = *it;

        ControlManager &mgr = ControlManager::getManager();
        SPCtrlLine *control_line = mgr.createControlLine(desktop->getTempGroup(),
                                                         place.start,
                                                         place.end,
                                                         CTLINE_SECONDARY);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
    }

    {
        for (size_t idx = 1; idx < intersections.size(); ++idx) {
            Geom::Point measure_text_pos = (intersections[idx - 1] + intersections[idx]) / 2;

            ControlManager &mgr = ControlManager::getManager();
            SPCtrlLine *control_line = mgr.createControlLine(desktop->getTempGroup(),
                                                             desktop->doc2dt(measure_text_pos),
                                                             desktop->doc2dt(measure_text_pos) - (normal * DIMENSION_OFFSET),
                                                             CTLINE_SECONDARY);
            measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
        }
    }

    // Initial point
    {
        SPCanvasItem * canvasitem = sp_canvas_item_new(desktop->getTempGroup(),
                                                       SP_TYPE_CTRL,
                                                       "anchor", SP_ANCHOR_CENTER,
                                                       "size", 8.0,
                                                       "stroked", TRUE,
                                                       "stroke_color", 0xff0000ff,
                                                       "mode", SP_KNOT_MODE_XOR,
                                                       "shape", SP_KNOT_SHAPE_CROSS,
                                                       NULL );

        SP_CTRL(canvasitem)->moveto(start_point);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvasitem, 0));
    }
}
}
}
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
