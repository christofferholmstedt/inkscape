/*
 * Our nice measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include <gdk/gdkkeysyms.h>
#include <boost/none_t.hpp>
#include "util/units.h"
#include "display/canvas-text.h"
#include "display/curve.h"
#include "display/sodipodi-ctrl.h"
#include "display/sp-ctrlline.h"
#include "display/sp-canvas.h"
#include "display/sp-canvas-item.h"
#include "display/sp-canvas-util.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "ui/tools/measure-tool.h"
#include "ui/tools/freehand-base.h"
#include "ui/control-manager.h"
#include <2geom/line.h>
#include <2geom/path-intersection.h>
#include <2geom/pathvector.h>
#include <2geom/crossing.h>
#include <2geom/angle.h>
#include <2geom/transforms.h>
#include "sp-namedview.h"
#include "sp-shape.h"
#include "sp-text.h"
#include "sp-flowtext.h"
#include "sp-defs.h"
#include "sp-item.h"
#include "macros.h"
#include "rubberband.h"
#include "path-chemistry.h"
#include "desktop.h"
#include "document.h"
#include "document-undo.h"
#include "viewbox.h"
#include "snap.h"
#include "text-editing.h"
#include "pixmaps/cursor-measure.xpm"
#include "preferences.h"
#include "inkscape.h"
#include "knot.h"
#include "enums.h"
#include "knot-enums.h"
#include "desktop-style.h"
#include "verbs.h"
#include <glibmm/i18n.h>

using Inkscape::ControlManager;
using Inkscape::CTLINE_SECONDARY;
using Inkscape::Util::unit_table;
using Inkscape::DocumentUndo;

#define MT_KNOT_COLOR_NORMAL 0xffffff00
#define MT_KNOT_COLOR_MOUSEOVER 0xff000000


namespace Inkscape {
namespace UI {
namespace Tools {

std::vector<Inkscape::Display::TemporaryItem*> measure_tmp_items;

const std::string& MeasureTool::getPrefsPath()
{
    return MeasureTool::prefsPath;
}

const std::string MeasureTool::prefsPath = "/tools/measure";

namespace {

gint dimension_offset = 35;

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
 * @param measure_rpr the container of the curve if converted to items.
 */
void createAngleDisplayCurve(SPDesktop *desktop, Geom::Point const &center, Geom::Point const &end, Geom::Point const &anchor, double angle, Inkscape::XML::Node *measure_repr = NULL)
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
        if(measure_repr) {
            Geom::PathVector c;
            Geom::Path p;
            p.start(desktop->doc2dt(p1));
            p.appendNew<Geom::CubicBezier>(desktop->doc2dt(p2),desktop->doc2dt(p3),desktop->doc2dt(p4));
            c.push_back(p);
            c *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
            SPDocument *doc = desktop->getDocument();
            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            if (c.size() == 1) {
                Inkscape::XML::Node *repr;
                repr = xml_doc->createElement("svg:path");
                gchar const *str = sp_svg_write_path(c);
                Geom::Point strokewidth = Geom::Point(1,1) * SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
                SPCSSAttr *css = sp_repr_css_attr_new();
                std::stringstream stroke_width;
                stroke_width.imbue(std::locale::classic());
                stroke_width <<  strokewidth[Geom::X] / desktop->current_zoom();
                sp_repr_css_set_property (css, "stroke-width", stroke_width.str().c_str());
                sp_repr_css_set_property (css, "fill", "none");
                guint32 line_color_secondary = 0xff00007f;
                gchar c[64];
                sp_svg_write_color (c, sizeof(c), line_color_secondary);
                sp_repr_css_set_property (css, "stroke", c);
                sp_repr_css_set_property (css, "stroke-linecap", "butt");
                sp_repr_css_set_property (css, "stroke-linejoin", "miter");
                sp_repr_css_set_property (css, "stroke-miterlimit", "4");
                sp_repr_css_set_property (css, "stroke-dasharray", "none");
                sp_repr_css_set_property (css, "stroke-opacity", "0.5");
                Glib::ustring css_str;
                sp_repr_css_write_string(css,css_str);
                repr->setAttribute("style", css_str.c_str());
                sp_repr_css_attr_unref (css);
                g_assert( str != NULL );
                repr->setAttribute("d", str);
                measure_repr->addChild(repr, NULL);
                Inkscape::GC::release(repr);
            }
        }
    }
}
} // namespace

Geom::Point const MAGIC_POINT = Geom::Point(-0.0003432532004303,-0.006745034004304);
static Geom::Point start_p = MAGIC_POINT;
static Geom::Point end_p = MAGIC_POINT;

MeasureTool::MeasureTool()
    : ToolBase(cursor_measure_xpm, 4, 4)
    , grabbed(NULL)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    // create the knots
    this->knot_start = new SPKnot(desktop, N_("Measure start"));
    this->knot_start->setMode(SP_KNOT_MODE_XOR);
    this->knot_start->setFill(MT_KNOT_COLOR_NORMAL, MT_KNOT_COLOR_MOUSEOVER, MT_KNOT_COLOR_MOUSEOVER);
    this->knot_start->setStroke(0x0000007f, 0x0000007f, 0x0000007f);
    this->knot_start->setShape(SP_KNOT_SHAPE_CIRCLE);
    this->knot_start->updateCtrl();
    this->knot_end = new SPKnot(desktop, N_("Measure end"));
    this->knot_end->setMode(SP_KNOT_MODE_XOR);
    this->knot_end->setFill(MT_KNOT_COLOR_NORMAL, MT_KNOT_COLOR_MOUSEOVER, MT_KNOT_COLOR_MOUSEOVER);
    this->knot_end->setStroke(0x0000007f, 0x0000007f, 0x0000007f);
    this->knot_end->setShape(SP_KNOT_SHAPE_CIRCLE);
    this->knot_end->updateCtrl();
    Geom::Rect display_area = desktop->get_display_area () ;
    if(display_area.interiorContains(start_p) && display_area.interiorContains(end_p) && end_p != MAGIC_POINT) {
        this->knot_start->moveto(start_p);
        this->knot_start->show();
        this->knot_end->moveto(end_p);
        this->knot_end->show();
        showCanvasItems();
    }
    this->_knot_start_moved_connection = this->knot_start->moved_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotStartMovedHandler));
    this->_knot_start_ungrabbed_connection = this->knot_start->ungrabbed_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotUngrabbedHandler));
    this->_knot_end_moved_connection = this->knot_end->moved_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotEndMovedHandler));
    this->_knot_end_ungrabbed_connection = this->knot_end->ungrabbed_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotUngrabbedHandler));

}

MeasureTool::~MeasureTool()
{
    this->_knot_start_moved_connection.disconnect();
    this->_knot_start_ungrabbed_connection.disconnect();
    this->_knot_end_moved_connection.disconnect();
    this->_knot_end_ungrabbed_connection.disconnect();

    /* unref should call destroy */
    knot_unref(this->knot_start);
    knot_unref(this->knot_end);
    for (size_t idx = 0; idx < measure_tmp_items.size(); ++idx) {
        desktop->remove_temporary_canvasitem(measure_tmp_items[idx]);
    }
    measure_tmp_items.clear();
}

void MeasureTool::reverseKnots()
{
    Geom::Point start = start_p;
    Geom::Point end = end_p;
    this->knot_start->moveto(end);
    this->knot_start->show();
    this->knot_end->moveto(start);
    this->knot_end->show();
    start_p = end;
    end_p = start;
    this->showCanvasItems();
}

void MeasureTool::knotStartMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state)
{
    Geom::Point point = this->knot_start->position();
    if (state & GDK_CONTROL_MASK) {
        spdc_endpoint_snap_rotation(this, point, end_p, state);
    } else if (!(state & GDK_SHIFT_MASK)) {
        SnapManager &m = desktop->namedview->snap_manager;
        m.setup(desktop);
        Inkscape::SnapCandidatePoint scp(point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
        scp.addOrigin(this->knot_end->position());
        Inkscape::SnappedPoint sp = m.freeSnap(scp);
        point = sp.getPoint();
        m.unSetup();
    }
    if(start_p != point) {
        start_p = point;
        this->knot_start->moveto(start_p);
    }
    showCanvasItems();
}

void MeasureTool::knotEndMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state)
{
    Geom::Point point = this->knot_end->position();
    if (state & GDK_CONTROL_MASK) {
        spdc_endpoint_snap_rotation(this, point, start_p, state);
    } else if (!(state & GDK_SHIFT_MASK)) {
        SnapManager &m = desktop->namedview->snap_manager;
        m.setup(desktop);
        Inkscape::SnapCandidatePoint scp(point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
        scp.addOrigin(this->knot_start->position());
        Inkscape::SnappedPoint sp = m.freeSnap(scp);
        point = sp.getPoint();
        m.unSetup();
    }
    if(end_p != point) {
        end_p = point;
        this->knot_end->moveto(end_p);
    }
    showCanvasItems();
}

void MeasureTool::knotUngrabbedHandler(SPKnot */*knot*/,  unsigned int state)
{
    this->knot_start->moveto(start_p);
    this->knot_end->moveto(end_p);
    showCanvasItems();
}



void MeasureTool::finish()
{
    this->enableGrDrag(false);

    if (this->grabbed) {
        sp_canvas_item_ungrab(this->grabbed, GDK_CURRENT_TIME);
        this->grabbed = NULL;
    }

    ToolBase::finish();
}

//void MeasureTool::setup() {
//    ToolBase* ec = this;
//
////    if (SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->setup) {
////        SP_EVENT_CONTEXT_CLASS(sp_measure_context_parent_class)->setup(ec);
////    }
//    ToolBase::setup();
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

bool MeasureTool::root_handler(GdkEvent* event)
{
    gint ret = FALSE;

    switch (event->type) {
    case GDK_BUTTON_PRESS: {
        this->knot_start->hide();
        this->knot_end->hide();
        Geom::Point const button_w(event->button.x, event->button.y);
        explicitBase = boost::none;
        last_end = boost::none;
        start_p = desktop->w2d(button_w);

        if (event->button.button == 1 && !this->space_panning) {
            // save drag origin
            start_p = desktop->w2d(Geom::Point(event->button.x, event->button.y));
            within_tolerance = true;

            ret = TRUE;
        }

        SnapManager &m = desktop->namedview->snap_manager;
        m.setup(desktop);
        m.freeSnapReturnByRef(start_p, Inkscape::SNAPSOURCE_OTHER_HANDLE);
        m.unSetup();

        sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                            GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK,
                            NULL, event->button.time);
        this->grabbed = SP_CANVAS_ITEM(desktop->acetate);
        break;
    }
    case GDK_KEY_PRESS: {
        if ((event->key.keyval == GDK_KEY_Shift_L) || (event->key.keyval == GDK_KEY_Shift_R)) {
            explicitBase = end_p;
        }
        break;
    }
    case GDK_MOTION_NOTIFY: {
        if (!(event->motion.state & GDK_BUTTON1_MASK)) {
            if(!(event->motion.state & GDK_SHIFT_MASK)) {
                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point const motion_dt(desktop->w2d(motion_w));

                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);

                Inkscape::SnapCandidatePoint scp(motion_dt, Inkscape::SNAPSOURCE_OTHER_HANDLE);
                scp.addOrigin(start_p);

                m.preSnap(scp);
                m.unSetup();
            }
        } else {
            ret = TRUE;
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
            Geom::Point const motion_w(event->motion.x, event->motion.y);
            if ( within_tolerance) {
                if ( Geom::LInfty( motion_w - start_p ) < tolerance) {
                    return FALSE;   // Do not drag if we're within tolerance from origin.
                }
            }
            // Once the user has moved farther than tolerance from the original location
            // (indicating they intend to move the object, not click), then always process the
            // motion notify coordinates as given (no snapping back to origin)
            within_tolerance = false;
            if(event->motion.time == 0 || !last_end  || Geom::LInfty( motion_w - *last_end ) > (tolerance/4.0)) {
                Geom::Point const motion_w(event->motion.x, event->motion.y);
                Geom::Point const motion_dt(desktop->w2d(motion_w));
                end_p = motion_dt;

                if (event->motion.state & GDK_CONTROL_MASK) {
                    spdc_endpoint_snap_rotation(this, end_p, start_p, event->motion.state);
                } else if (!(event->motion.state & GDK_SHIFT_MASK)) {
                    SnapManager &m = desktop->namedview->snap_manager;
                    m.setup(desktop);
                    Inkscape::SnapCandidatePoint scp(end_p, Inkscape::SNAPSOURCE_OTHER_HANDLE);
                    scp.addOrigin(start_p);
                    Inkscape::SnappedPoint sp = m.freeSnap(scp);
                    end_p = sp.getPoint();
                    m.unSetup();
                }
                showCanvasItems();
                last_end = motion_w ;
            }
            gobble_motion_events(GDK_BUTTON1_MASK);
        }
        break;
    }
    case GDK_BUTTON_RELEASE: {
        this->knot_start->moveto(start_p);
        this->knot_start->show();
        end_p = end_p;
        if(last_end) {
            end_p = desktop->w2d(*last_end);
            if (event->button.state & GDK_CONTROL_MASK) {
                spdc_endpoint_snap_rotation(this, end_p, start_p, event->motion.state);
            } else if (!(event->button.state & GDK_SHIFT_MASK)) {
                SnapManager &m = desktop->namedview->snap_manager;
                m.setup(desktop);
                Inkscape::SnapCandidatePoint scp(end_p, Inkscape::SNAPSOURCE_OTHER_HANDLE);
                scp.addOrigin(start_p);
                Inkscape::SnappedPoint sp = m.freeSnap(scp);
                end_p = sp.getPoint();
                m.unSetup();
            }
        }
        this->knot_end->moveto(end_p);
        this->knot_end->show();
        showCanvasItems();
        if (this->grabbed) {
            sp_canvas_item_ungrab(this->grabbed, event->button.time);
            this->grabbed = NULL;
        }
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

void MeasureTool::setMarkers()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    SPObject *arrowStart = doc->getObjectById("Arrow2Sstart");
    SPObject *arrowEnd = doc->getObjectById("Arrow2Send");
    if (!arrowStart) {
        setMarker(true);
    }
    if(!arrowEnd) {
        setMarker(false);
    }
}
void MeasureTool::setMarker(bool isStart)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    SPDefs *defs = doc->getDefs();
    Inkscape::XML::Node *rmarker;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    rmarker = xml_doc->createElement("svg:marker");
    if(isStart) {
        rmarker->setAttribute("id", "Arrow2Sstart");
    } else {
        rmarker->setAttribute("id", "Arrow2Send");
    }
    rmarker->setAttribute("inkscape:isstock", "true");
    if(isStart) {
        rmarker->setAttribute("inkscape:stockid", "Arrow2Sstart");
    } else {
        rmarker->setAttribute("inkscape:stockid", "Arrow2Send");
    }
    rmarker->setAttribute("orient", "auto");
    rmarker->setAttribute("refX", "0.0");
    rmarker->setAttribute("refY", "0.0");
    rmarker->setAttribute("style", "overflow:visible;");
    SPItem *marker = SP_ITEM(defs->appendChildRepr(rmarker));
    Inkscape::GC::release(rmarker);
    marker->updateRepr();
    Inkscape::XML::Node *rpath;
    rpath = xml_doc->createElement("svg:path");
    rpath->setAttribute("d", "M 8.72,4.03 L -2.21,0.02 L 8.72,-4.00 C 6.97,-1.63 6.98,1.62 8.72,4.03 z");
    if(isStart) {
        rpath->setAttribute("id", "Arrow2SstartPath");
    } else {
        rpath->setAttribute("id", "Arrow2SendPath");
    }
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property (css, "stroke", "none");
    sp_repr_css_set_property (css, "fill", "#000000");
    sp_repr_css_set_property (css, "fill-opacity", "1");
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rpath->setAttribute("style", css_str.c_str());
    sp_repr_css_attr_unref (css);
    if(isStart) {
        rpath->setAttribute("transform", "scale(0.3) translate(-2.3,0)");
    } else {
        rpath->setAttribute("transform", "scale(0.3) rotate(180) translate(-2.3,0)");
    }
    SPItem *path = SP_ITEM(marker->appendChildRepr(rpath));
    Inkscape::GC::release(rpath);
    path->updateRepr();
}

void MeasureTool::toGuides()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    Geom::Point start = desktop->doc2dt(start_p);
    Geom::Point end = desktop->doc2dt(end_p);
    start *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    end *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    Geom::Ray ray(start,end);
    SPNamedView *namedview = desktop->namedview;
    if(!namedview){
        return;
    }
    //meassure angle
    Inkscape::XML::Node *measure_line;
    measure_line = xml_doc->createElement("sodipodi:guide");
    std::stringstream position;
    position.imbue(std::locale::classic());
    position <<  start[Geom::X] << "," << start[Geom::Y];
    measure_line->setAttribute("position", position.str().c_str() );
    Geom::Point unit_vector = Geom::rot90(start.polar(ray.angle()));
    std::stringstream angle;
    angle.imbue(std::locale::classic());
    angle << unit_vector[Geom::X] << "," << unit_vector[Geom::Y];
    measure_line->setAttribute("orientation", angle.str().c_str());
    namedview->appendChild(measure_line);
    Inkscape::GC::release(measure_line);
    //base angle
    if(explicitBase){
        explicitBase = *explicitBase * SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
        ray.setPoints(start, *explicitBase);
        if(ray.angle() != 0){
            Inkscape::XML::Node *base_line;
            base_line = xml_doc->createElement("sodipodi:guide");
            position.str("");
            position.imbue(std::locale::classic());
            position <<  start[Geom::X] << "," << start[Geom::Y];
            base_line->setAttribute("position", position.str().c_str() );
            Geom::Point unit_vector = Geom::rot90(start.polar(ray.angle()));
            std::stringstream angle;
            angle.imbue(std::locale::classic());
            angle << unit_vector[Geom::X] << "," << unit_vector[Geom::Y];
            base_line->setAttribute("orientation", angle.str().c_str());
            namedview->appendChild(base_line);
            Inkscape::GC::release(base_line);
        }
    }
    //start horizontal
    Inkscape::XML::Node *start_horizontal;
    start_horizontal = xml_doc->createElement("sodipodi:guide");
    position.str("");
    position.imbue(std::locale::classic());
    position <<  start[Geom::X] << "," << start[Geom::Y];
    start_horizontal->setAttribute("position", position.str().c_str() );
    start_horizontal->setAttribute("orientation", "0,1");
    namedview->appendChild(start_horizontal);
    Inkscape::GC::release(start_horizontal);
    //start vertical
    Inkscape::XML::Node *start_vertical;
    start_vertical = xml_doc->createElement("sodipodi:guide");
    position.str("");
    position.imbue(std::locale::classic());
    position <<  start[Geom::X] << "," << start[Geom::Y];
    start_vertical->setAttribute("position", position.str().c_str() );
    start_vertical->setAttribute("orientation", "1,0");
    namedview->appendChild(start_vertical);
    Inkscape::GC::release(start_vertical);
    //end horizontal
    Inkscape::XML::Node *end_horizontal;
    end_horizontal = xml_doc->createElement("sodipodi:guide");
    position.str("");
    position.imbue(std::locale::classic());
    position <<  end[Geom::X] << "," << end[Geom::Y];
    end_horizontal->setAttribute("position", position.str().c_str() );
    end_horizontal->setAttribute("orientation", "0,1");
    namedview->appendChild(end_horizontal);
    Inkscape::GC::release(end_horizontal);
    //start vertical
    Inkscape::XML::Node *end_vertical;
    end_vertical = xml_doc->createElement("sodipodi:guide");
    position.str("");
    position.imbue(std::locale::classic());
    position <<  end[Geom::X] << "," << end[Geom::Y];
    end_vertical->setAttribute("position", position.str().c_str() );
    end_vertical->setAttribute("orientation", "1,0");
    namedview->appendChild(end_vertical);
    Inkscape::GC::release(end_vertical);

    doc->ensureUpToDate();
    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_MEASURE,_("Add guides from measure tool"));
}

void MeasureTool::toItem()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    Geom::Ray ray(start_p,end_p);
    guint32 line_color_primary = 0x0000ff7f;
    Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
    Inkscape::XML::Node *rgroup = xml_doc->createElement("svg:g");
    showCanvasItems(true, rgroup);
    setLine(start_p,end_p, false, &line_color_primary, rgroup);
    SPItem *measure_item = SP_ITEM(desktop->currentLayer()->appendChildRepr(rgroup));
    Inkscape::GC::release(rgroup);
    measure_item->updateRepr();
    doc->ensureUpToDate();
    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_MEASURE,_("Convert measure to items"));
    reset();
}

void MeasureTool::toMarkDimension()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPDocument *doc = desktop->getDocument();
    setMarkers();
    Geom::Ray ray(start_p,end_p);
    Geom::Point start = start_p + Geom::Point::polar(ray.angle(), 5);
    start = start + Geom::Point::polar(ray.angle() + Geom::deg_to_rad(90), -(dimension_offset / 4.0));
    Geom::Point end = end_p + Geom::Point::polar(ray.angle(), -5);
    end = end+ Geom::Point::polar(ray.angle() + Geom::deg_to_rad(90), -(dimension_offset / 4.0));
    guint32 color = 0x000000ff;
    setLine(start, end, true, &color);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit");
    if (!unit_name.compare("")) {
        unit_name = "px";
    }
    double fontsize = prefs->getInt("/tools/measure/fontsize")  * (96/72);
    Geom::Point middle = Geom::middle_point(start, end);
    double totallengthval = (end_p - start_p).length();
    totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);
    gchar *totallength_str = g_strdup_printf("%.2f %s", totallengthval, unit_name.c_str());
    setLabelText(totallength_str, middle, fontsize, Geom::deg_to_rad(180) - ray.angle());
    doc->ensureUpToDate();
    DocumentUndo::done(desktop->getDocument(), SP_VERB_CONTEXT_MEASURE,_("Add global meassure line"));
}

void MeasureTool::setLine(Geom::Point start_point,Geom::Point end_point, bool markers, guint32 *color, Inkscape::XML::Node *measure_repr)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(!desktop || !start_p.isFinite() || !end_p.isFinite()) {
        return;
    }
    Geom::PathVector c;
    Geom::Path p;
    p.start(desktop->doc2dt(start_point));
    p.appendNew<Geom::LineSegment>(desktop->doc2dt(end_point));
    c.push_back(p);
    c *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    SPDocument *doc = desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    if (c.size() == 1) {
        Inkscape::XML::Node *repr;
        repr = xml_doc->createElement("svg:path");
        gchar const *str = sp_svg_write_path(c);
        Geom::Point strokewidth = Geom::Point(1,1) * SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
        SPCSSAttr *css = sp_repr_css_attr_new();
        std::stringstream stroke_width;
        stroke_width.imbue(std::locale::classic());
        if(measure_repr) {
            stroke_width <<  strokewidth[Geom::X] / desktop->current_zoom();
        } else {
            stroke_width <<  strokewidth[Geom::X];
        }
        sp_repr_css_set_property (css, "stroke-width", stroke_width.str().c_str());
        sp_repr_css_set_property (css, "fill", "none");
        if(color) {
            gchar c[64];
            sp_svg_write_color (c, sizeof(c), *color);
            sp_repr_css_set_property (css, "stroke", c);
        } else {
            sp_repr_css_set_property (css, "stroke", "#000000");
        }
        sp_repr_css_set_property (css, "stroke-linecap", "square");
        sp_repr_css_set_property (css, "stroke-linejoin", "miter");
        sp_repr_css_set_property (css, "stroke-miterlimit", "4");
        sp_repr_css_set_property (css, "stroke-dasharray", "none");
        if(measure_repr) {
            sp_repr_css_set_property (css, "stroke-opacity", "0.5");
        } else {
            sp_repr_css_set_property (css, "stroke-opacity", "1");
        }
        if(markers) {
            sp_repr_css_set_property (css, "marker-start", "url(#Arrow2Sstart)");
            sp_repr_css_set_property (css, "marker-end", "url(#Arrow2Send)");
        }
        Glib::ustring css_str;
        sp_repr_css_write_string(css,css_str);
        repr->setAttribute("style", css_str.c_str());
        sp_repr_css_attr_unref (css);
        g_assert( str != NULL );
        repr->setAttribute("d", str);
        if(measure_repr) {
            measure_repr->addChild(repr, NULL);
            Inkscape::GC::release(repr);
        } else {
            SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
            Inkscape::GC::release(repr);
            item->updateRepr();
        }
    }
}

void MeasureTool::setPoint(Geom::Point origin, Inkscape::XML::Node *measure_repr)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(!desktop || !origin.isFinite()) {
        return;
    }
    char const * svgd;
    svgd = "m 0.707,0.707 6.586,6.586 m 0,-6.586 -6.586,6.586";
    Geom::PathVector c = sp_svg_read_pathv(svgd);
    Geom::Scale scale = Geom::Scale(desktop->current_zoom()).inverse();
    c *= Geom::Translate(Geom::Point(-3.5,-3.5));
    c *= scale;
    c *= Geom::Translate(Geom::Point() - (scale.vector() * 0.5));
    c *= Geom::Translate(desktop->doc2dt(origin));
    c *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    SPDocument *doc = desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    if (c.size() == 2) {
        Inkscape::XML::Node *repr;
        repr = xml_doc->createElement("svg:path");
        gchar const *str = sp_svg_write_path(c);
        SPCSSAttr *css = sp_repr_css_attr_new();
        Geom::Point strokewidth = (Geom::Point(1,1) * SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse())/ desktop->current_zoom();
        std::stringstream stroke_width;
        stroke_width.imbue(std::locale::classic());
        stroke_width <<  strokewidth[Geom::X];
        sp_repr_css_set_property (css, "stroke-width", stroke_width.str().c_str());
        sp_repr_css_set_property (css, "fill", "none");
        guint32 line_color_secondary = 0xff0000ff;
        gchar c[64];
        sp_svg_write_color (c, sizeof(c), line_color_secondary);
        sp_repr_css_set_property (css, "stroke", c);
        sp_repr_css_set_property (css, "stroke-linecap", "square");
        sp_repr_css_set_property (css, "stroke-linejoin", "miter");
        sp_repr_css_set_property (css, "stroke-miterlimit", "4");
        sp_repr_css_set_property (css, "stroke-dasharray", "none");
        sp_repr_css_set_property (css, "stroke-opacity", "0.5");
        Glib::ustring css_str;
        sp_repr_css_write_string(css,css_str);
        repr->setAttribute("style", css_str.c_str());
        sp_repr_css_attr_unref (css);
        g_assert( str != NULL );
        repr->setAttribute("d", str);
        measure_repr->addChild(repr, NULL);
        Inkscape::GC::release(repr);
    }
}

void MeasureTool::setLabelText(const char *value, Geom::Point pos, double fontsize, Geom::Coord angle, guint32 *background, Inkscape::XML::Node *measure_repr, CanvasTextAnchorPositionEnum text_anchor)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
    /* Create <text> */
    pos = desktop->doc2dt(pos);
    Inkscape::XML::Node *rtext = xml_doc->createElement("svg:text");
    rtext->setAttribute("xml:space", "preserve");


    /* Set style */
    sp_desktop_apply_style_tool(desktop, rtext, "/tools/text", true);
    if(measure_repr) {
        sp_repr_set_svg_double(rtext, "x", 2);
        sp_repr_set_svg_double(rtext, "y", 2);
    } else {
        sp_repr_set_svg_double(rtext, "x", 0);
        sp_repr_set_svg_double(rtext, "y", 0);
    }

    /* Create <tspan> */
    Inkscape::XML::Node *rtspan = xml_doc->createElement("svg:tspan");
    rtspan->setAttribute("sodipodi:role", "line"); // otherwise, why bother creating the tspan?
    SPCSSAttr *css = sp_repr_css_attr_new();
    std::stringstream font_size;
    font_size.imbue(std::locale::classic());
    font_size <<  fontsize ;
    sp_repr_css_set_property (css, "font-size", font_size.str().c_str());
    sp_repr_css_set_property (css, "font-style", "normal");
    sp_repr_css_set_property (css, "font-weight", "normal");
    sp_repr_css_set_property (css, "line-height", "125%");
    sp_repr_css_set_property (css, "letter-spacing", "0px");
    sp_repr_css_set_property (css, "word-spacing", "0px");
    if(measure_repr) {
        sp_repr_css_set_property (css, "fill", "#FFFFFF");
    } else {
        sp_repr_css_set_property (css, "fill", "#000000");
    }
    sp_repr_css_set_property (css, "fill-opacity", "1");
    sp_repr_css_set_property (css, "stroke", "none");
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rtspan->setAttribute("style", css_str.c_str());
    sp_repr_css_attr_unref (css);
    rtext->addChild(rtspan, NULL);
    Inkscape::GC::release(rtspan);
    /* Create TEXT */
    Inkscape::XML::Node *rstring = xml_doc->createTextNode(value);
    rtspan->addChild(rstring, NULL);
    Inkscape::GC::release(rstring);
    SPItem *text_item = SP_ITEM(desktop->currentLayer()->appendChildRepr(rtext));
    Inkscape::GC::release(rtext);
    text_item->updateRepr();
    Geom::OptRect bbox = text_item->geometricBounds();
    if (!measure_repr && bbox) {
        Geom::Point center = bbox->midpoint();
        text_item->transform *= Geom::Translate(center).inverse();
        pos = pos + Geom::Point::polar(angle+ Geom::deg_to_rad(90), -bbox->height());
    }
    if(measure_repr) {
        /* Create <group> */
        Inkscape::XML::Node *rgroup = xml_doc->createElement("svg:g");
        /* Create <rect> */
        Inkscape::XML::Node *rrect = xml_doc->createElement("svg:rect");
        SPCSSAttr *css = sp_repr_css_attr_new ();
        gchar c[64];
        sp_svg_write_color (c, sizeof(c), *background);
        sp_repr_css_set_property (css, "fill", c);
        sp_repr_css_set_property (css, "fill-opacity", "0.5");
        sp_repr_css_set_property (css, "stroke-width", "0");
        Glib::ustring css_str;
        sp_repr_css_write_string(css,css_str);
        rrect->setAttribute("style", css_str.c_str());
        sp_repr_css_attr_unref (css);
        sp_repr_set_svg_double(rgroup, "x", 0);
        sp_repr_set_svg_double(rgroup, "y", 0);
        sp_repr_set_svg_double(rrect, "x", 0);
        sp_repr_set_svg_double(rrect, "y", -bbox->height());
        sp_repr_set_svg_double(rrect, "width", (bbox->width()) + 6);
        sp_repr_set_svg_double(rrect, "height", (bbox->height()) + 6);
        Inkscape::XML::Node *rtextitem = text_item->getRepr();
        text_item->deleteObject();
        rgroup->addChild(rtextitem, NULL);
        Inkscape::GC::release(rtextitem);
        rgroup->addChild(rrect, NULL);
        Inkscape::GC::release(rrect);
        SPItem *text_item_box = SP_ITEM(desktop->currentLayer()->appendChildRepr(rgroup));
        Geom::Scale scale = Geom::Scale(desktop->current_zoom()).inverse();
        if(bbox && text_anchor == TEXT_ANCHOR_CENTER) {
            text_item_box->transform *= Geom::Translate(bbox->midpoint() - Geom::Point(1.0,1.0)).inverse();
        }
        text_item_box->transform *= scale;
        text_item_box->transform *= Geom::Translate(Geom::Point() - (scale.vector() * 0.5));
        text_item_box->transform *= Geom::Translate(pos);
        text_item_box->transform *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
        text_item_box->updateRepr();
        text_item_box->doWriteTransform(text_item_box->getRepr(), text_item_box->transform, NULL, true);
        Inkscape::XML::Node *rlabel = text_item_box->getRepr();
        text_item_box->deleteObject();
        measure_repr->addChild(rlabel, NULL);
        Inkscape::GC::release(rlabel);
    } else {
        text_item->transform *= Geom::Rotate(angle);
        text_item->transform *= Geom::Translate(pos);
        text_item->transform *= SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
        text_item->doWriteTransform(text_item->getRepr(), text_item->transform, NULL, true);
    }
}

void MeasureTool::reset()
{
    this->knot_start->hide();
    this->knot_end->hide();
    for (size_t idx = 0; idx < measure_tmp_items.size(); ++idx) {
        desktop->remove_temporary_canvasitem(measure_tmp_items[idx]);
    }
    measure_tmp_items.clear();
}

void MeasureTool::showCanvasItems(bool to_item, Inkscape::XML::Node *measure_repr)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(!desktop || !start_p.isFinite() || !end_p.isFinite() || end_p == MAGIC_POINT) {
        return;
    }
    guint32 line_color_primary = 0x0000ff7f;
    guint32 line_color_secondary = 0xff00007f;
    //clear previous temporary canvas items, we'll draw new ones
    for (size_t idx = 0; idx < measure_tmp_items.size(); ++idx) {
        desktop->remove_temporary_canvasitem(measure_tmp_items[idx]);
    }
    measure_tmp_items.clear();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool show_in_between = prefs->getBool("/tools/measure/show_in_between");
    bool all_layers = prefs->getBool("/tools/measure/all_layers");
    dimension_offset = prefs->getDouble("/tools/measure/offset");
    Geom::PathVector lineseg;
    Geom::Path p;
    p.start(desktop->dt2doc(start_p));
    p.appendNew<Geom::LineSegment>(desktop->dt2doc(end_p));
    lineseg.push_back(p);

    double deltax = end_p[Geom::X] - start_p[Geom::X];
    double deltay = end_p[Geom::Y] - start_p[Geom::Y];
    double angle = atan2(deltay, deltax);
    double baseAngle = 0;

    if (explicitBase) {
        double deltax2 = explicitBase.get()[Geom::X] - start_p[Geom::X];
        double deltay2 = explicitBase.get()[Geom::Y] - start_p[Geom::Y];

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
    if(!show_in_between) {
        r->start(desktop,start_p);
        r->move(end_p);
        items = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers, 2);
        r->stop();
        r->setMode(RUBBERBAND_MODE_TOUCHPATH);
        r->start(desktop,end_p);
        r->move(start_p);
        std::vector<SPItem*> items_reverse = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers, 2);
        r->stop();
        if(items_reverse.size() == 2 && items_reverse[1] != items[0] && items_reverse[1] != items[1]) {
            items.push_back(items_reverse[1]);
        }
        if(items_reverse.size() >= 1 && items_reverse[0] != items[1]) {
            items.push_back(items_reverse[0]);
        }
    } else {
        r->start(desktop,start_p);
        r->move(end_p);
        items = desktop->getDocument()->getItemsAtPoints(desktop->dkey, r->getPoints(), all_layers);
        r->stop();
    }
    std::vector<double> intersection_times;
    for (std::vector<SPItem*>::const_iterator i=items.begin(); i!=items.end(); i++) {
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

    double fontsize = prefs->getInt("/tools/measure/fontsize") * (96/72);
    // Normal will be used for lines and text
    Geom::Point windowNormal = Geom::unit_vector(Geom::rot90(desktop->d2w(end_p - start_p)));
    Geom::Point normal = desktop->w2d(windowNormal);

    std::vector<Geom::Point> intersections;
    std::sort(intersection_times.begin(), intersection_times.end());
    for (std::vector<double>::iterator iter_t = intersection_times.begin(); iter_t != intersection_times.end(); iter_t++) {
        if(show_in_between) {
            intersections.push_back(lineseg[0].pointAt(*iter_t));
        }
    }
    if(!show_in_between && intersection_times.size() > 1) {
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
        placement.offset = dimension_offset;
        placement.start = desktop->doc2dt( (intersections[idx - 1] + intersections[idx]) / 2 );
        placement.end = placement.start - (normal * placement.offset);

        placements.push_back(placement);
    }

    // Adjust positions
    repositionOverlappingLabels(placements, desktop, windowNormal, fontsize);
    for (std::vector<LabelPlacement>::iterator it = placements.begin(); it != placements.end(); ++it) {
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
        if(to_item) {
            guint32 background = canvas_tooltip->rgba_background;
            setLabelText(measure_str, place.end, fontsize, 0, &background, measure_repr);
        }
        g_free(measure_str);
    }

    Geom::Point angleDisplayPt = calcAngleDisplayAnchor(desktop, angle, baseAngle,
                                 start_p, end_p,
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
        if(to_item) {
            guint32 background = canvas_tooltip->rgba_background;
            setLabelText(angle_str, angleDisplayPt, fontsize, 0, &background, measure_repr);
        }
        g_free(angle_str);
    }

    {
        double totallengthval = (end_p - start_p).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);

        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *totallength_str = g_strdup_printf("%.2f %s", totallengthval, unit_name.c_str());
        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                       desktop,
                                       end_p + desktop->w2d(Geom::Point(3*fontsize, -fontsize)),
                                       totallength_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x3333337f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_LEFT;
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        if(to_item) {
            guint32 background = canvas_tooltip->rgba_background;
            setLabelText(totallength_str, end_p + desktop->w2d(Geom::Point(3*fontsize, -fontsize)), fontsize, 0, &background, measure_repr, TEXT_ANCHOR_LEFT);
        }
        g_free(totallength_str);
    }

    if (intersections.size() > 2) {
        double totallengthval = (intersections[intersections.size()-1] - intersections[0]).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);

        // TODO cleanup memory, Glib::ustring, etc.:
        gchar *total_str = g_strdup_printf("%.2f %s", totallengthval, unit_name.c_str());
        SPCanvasText *canvas_tooltip = sp_canvastext_new(desktop->getTempGroup(),
                                       desktop,
                                       desktop->doc2dt((intersections[0] + intersections[intersections.size()-1])/2) + normal * (dimension_offset * 2),
                                       total_str);
        sp_canvastext_set_fontsize(canvas_tooltip, fontsize);
        canvas_tooltip->rgba = 0xffffffff;
        canvas_tooltip->rgba_background = 0x33337f7f;
        canvas_tooltip->outline = false;
        canvas_tooltip->background = true;
        canvas_tooltip->anchor_position = TEXT_ANCHOR_CENTER;
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvas_tooltip, 0));
        if(to_item) {
            guint32 background = canvas_tooltip->rgba_background;
            setLabelText(total_str, desktop->doc2dt((intersections[0] + intersections[intersections.size()-1])/2) + normal * (dimension_offset * 2), fontsize, 0, &background, measure_repr);
        }
        g_free(total_str);
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

        SP_CTRL(canvasitem)->moveto(start_p);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(canvasitem, 0));
        if(to_item) {
            setPoint(start_p, measure_repr);
        }
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
        if(to_item) {
            setPoint(desktop->doc2dt(intersections[idx]), measure_repr);
        }
    }
    // Since adding goes to the bottom, do all lines last.

    // draw main control line
    {
        SPCtrlLine *control_line = ControlManager::getManager().createControlLine(desktop->getTempGroup(),
                                   start_p,
                                   end_p);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));

        if ((end_p[Geom::X] != start_p[Geom::X]) && (end_p[Geom::Y] != start_p[Geom::Y])) {
            double length = std::abs((end_p - start_p).length());
            Geom::Point anchorEnd = start_p;
            anchorEnd[Geom::X] += length;
            if (explicitBase) {
                anchorEnd *= (Geom::Affine(Geom::Translate(-start_p))
                              * Geom::Affine(Geom::Rotate(baseAngle))
                              * Geom::Affine(Geom::Translate(start_p)));
            }

            SPCtrlLine *control_line = ControlManager::getManager().createControlLine(desktop->getTempGroup(),
                                       start_p,
                                       anchorEnd,
                                       CTLINE_SECONDARY);
            measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
            if(to_item) {
                setLine(start_p,
                        anchorEnd,
                        false,
                        &line_color_secondary,
                        measure_repr);
            }
            createAngleDisplayCurve(desktop, start_p, end_p, angleDisplayPt, angle, measure_repr);
        }
    }

    if (intersections.size() > 2) {
        ControlManager &mgr = ControlManager::getManager();
        SPCtrlLine *control_line = 0;
        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[0]) + normal * (dimension_offset * 2),
                                             desktop->doc2dt(intersections[intersections.size() - 1]) + normal * (dimension_offset * 2));
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
        if(to_item) {
            setLine(desktop->doc2dt(intersections[0]) + normal * (dimension_offset * 2),
                    desktop->doc2dt(intersections[intersections.size() - 1]) + normal * (dimension_offset * 2),
                    false,
                    &line_color_primary,
                    measure_repr);
        }
        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[0]),
                                             desktop->doc2dt(intersections[0]) + normal * (dimension_offset * 2));
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
        if(to_item) {
            setLine(desktop->doc2dt(intersections[0]),
                    desktop->doc2dt(intersections[0]) + normal * (dimension_offset * 2),
                    false,
                    &line_color_primary,
                    measure_repr);
        }
        control_line = mgr.createControlLine(desktop->getTempGroup(),
                                             desktop->doc2dt(intersections[intersections.size() - 1]),
                                             desktop->doc2dt(intersections[intersections.size() - 1]) + normal * (dimension_offset * 2));
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
        if(to_item) {
            setLine(desktop->doc2dt(intersections[intersections.size() - 1]),
                    desktop->doc2dt(intersections[intersections.size() - 1]) + normal * (dimension_offset * 2),
                    false,
                    &line_color_primary,
                    measure_repr);
        }
    }

    // call-out lines
    for (std::vector<LabelPlacement>::iterator it = placements.begin(); it != placements.end(); ++it) {
        LabelPlacement &place = *it;

        ControlManager &mgr = ControlManager::getManager();
        SPCtrlLine *control_line = mgr.createControlLine(desktop->getTempGroup(),
                                   place.start,
                                   place.end,
                                   CTLINE_SECONDARY);
        measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
        if(to_item) {
            setLine(place.start,place.end, false, &line_color_secondary, measure_repr);
        }
    }

    {
        for (size_t idx = 1; idx < intersections.size(); ++idx) {
            Geom::Point measure_text_pos = (intersections[idx - 1] + intersections[idx]) / 2;

            ControlManager &mgr = ControlManager::getManager();
            SPCtrlLine *control_line = mgr.createControlLine(desktop->getTempGroup(),
                                       desktop->doc2dt(measure_text_pos),
                                       desktop->doc2dt(measure_text_pos) - (normal * dimension_offset),
                                       CTLINE_SECONDARY);
            measure_tmp_items.push_back(desktop->add_temporary_canvasitem(control_line, 0));
            if(to_item) {
                setLine(desktop->doc2dt(measure_text_pos),
                        desktop->doc2dt(measure_text_pos) - (normal * dimension_offset),
                        false,
                        &line_color_secondary,
                        measure_repr);
            }
        }
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
