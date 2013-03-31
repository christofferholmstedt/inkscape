/*
 * SVG <rect> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "display/curve.h"
#include <2geom/rect.h>

#include "inkscape.h"
#include "document.h"
#include "attributes.h"
#include "style.h"
#include "sp-rect.h"
#include <glibmm/i18n.h>
#include "xml/repr.h"
#include "sp-guide.h"
#include "preferences.h"

#define noRECT_VERBOSE

G_DEFINE_TYPE(SPRect, sp_rect, SP_TYPE_SHAPE);

static void
sp_rect_class_init(SPRectClass *klass)
{
}

CRect::CRect(SPRect* rect) : CShape(rect) {
	this->sprect = rect;
}

CRect::~CRect() {
}

static void
sp_rect_init(SPRect *rect)
{
	rect->crect = new CRect(rect);
	rect->typeHierarchy.insert(typeid(SPRect));

	delete rect->cshape;
	rect->cshape = rect->crect;
	rect->clpeitem = rect->crect;
	rect->citem = rect->crect;
	rect->cobject = rect->crect;

    /* Initializing to zero is automatic */
    /* sp_svg_length_unset(&rect->x, SP_SVG_UNIT_NONE, 0.0, 0.0); */
    /* sp_svg_length_unset(&rect->y, SP_SVG_UNIT_NONE, 0.0, 0.0); */
    /* sp_svg_length_unset(&rect->width, SP_SVG_UNIT_NONE, 0.0, 0.0); */
    /* sp_svg_length_unset(&rect->height, SP_SVG_UNIT_NONE, 0.0, 0.0); */
    /* sp_svg_length_unset(&rect->rx, SP_SVG_UNIT_NONE, 0.0, 0.0); */
    /* sp_svg_length_unset(&rect->ry, SP_SVG_UNIT_NONE, 0.0, 0.0); */
}

void CRect::build(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPRect* object = this->sprect;

    CShape::build(doc, repr);

    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "width" );
    object->readAttr( "height" );
    object->readAttr( "rx" );
    object->readAttr( "ry" );
}

void CRect::set(unsigned key, gchar const *value) {
	SPRect* rect = this->sprect;
	SPRect* object = rect;

    /* fixme: We need real error processing some time */

    switch (key) {
        case SP_ATTR_X:
            rect->x.readOrUnset(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_Y:
            rect->y.readOrUnset(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_WIDTH:
            if (!rect->width.read(value) || rect->width.value < 0.0) {
                rect->width.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_HEIGHT:
            if (!rect->height.read(value) || rect->height.value < 0.0) {
                rect->height.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_RX:
            if (!rect->rx.read(value) || rect->rx.value <= 0.0) {
                rect->rx.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_RY:
            if (!rect->ry.read(value) || rect->ry.value <= 0.0) {
                rect->ry.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        default:
            CShape::set(key, value);
            break;
    }
}

void CRect::update(SPCtx* ctx, unsigned int flags) {
	SPRect* object = this->sprect;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        SPRect *rect = (SPRect *) object;
        SPStyle *style = object->style;
        SPItemCtx const *ictx = (SPItemCtx const *) ctx;
        double const w = ictx->viewport.width();
        double const h = ictx->viewport.height();
        double const em = style->font_size.computed;
        double const ex = 0.5 * em;  // fixme: get x height from pango or libnrtype.
        rect->x.update(em, ex, w);
        rect->y.update(em, ex, h);
        rect->width.update(em, ex, w);
        rect->height.update(em, ex, h);
        rect->rx.update(em, ex, w);
        rect->ry.update(em, ex, h);
        ((SPShape *) object)->setShape();
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
    }

    CShape::update(ctx, flags);
}

Inkscape::XML::Node * CRect::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPRect* rect = this->sprect;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:rect");
    }

    sp_repr_set_svg_double(repr, "width", rect->width.computed);
    sp_repr_set_svg_double(repr, "height", rect->height.computed);
    if (rect->rx._set) sp_repr_set_svg_double(repr, "rx", rect->rx.computed);
    if (rect->ry._set) sp_repr_set_svg_double(repr, "ry", rect->ry.computed);
    sp_repr_set_svg_double(repr, "x", rect->x.computed);
    sp_repr_set_svg_double(repr, "y", rect->y.computed);

    this->set_shape(); // evaluate SPCurve
    CShape::write(xml_doc, repr, flags);

    return repr;
}

gchar* CRect::description() {
	g_return_val_if_fail(SP_IS_RECT(this->sprect), NULL);
	return g_strdup(_("<b>Rectangle</b>"));
}

#define C1 0.554

void CRect::set_shape() {
    SPRect *rect = this->sprect;

    if ((rect->height.computed < 1e-18) || (rect->width.computed < 1e-18)) {
        SP_SHAPE(rect)->setCurveInsync( NULL, TRUE);
        SP_SHAPE(rect)->setCurveBeforeLPE( NULL );
        return;
    }

    SPCurve *c = new SPCurve();

    double const x = rect->x.computed;
    double const y = rect->y.computed;
    double const w = rect->width.computed;
    double const h = rect->height.computed;
    double const w2 = w / 2;
    double const h2 = h / 2;
    double const rx = std::min(( rect->rx._set
                                 ? rect->rx.computed
                                 : ( rect->ry._set
                                     ? rect->ry.computed
                                     : 0.0 ) ),
                               .5 * rect->width.computed);
    double const ry = std::min(( rect->ry._set
                                 ? rect->ry.computed
                                 : ( rect->rx._set
                                     ? rect->rx.computed
                                     : 0.0 ) ),
                               .5 * rect->height.computed);
    /* TODO: Handle negative rx or ry as per
     * http://www.w3.org/TR/SVG11/shapes.html#RectElementRXAttribute once Inkscape has proper error
     * handling (see http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing).
     */

    /* We don't use proper circular/elliptical arcs, but bezier curves can approximate a 90-degree
     * arc fairly well.
     */
    if ((rx > 1e-18) && (ry > 1e-18)) {
        c->moveto(x + rx, y);
        if (rx < w2) c->lineto(x + w - rx, y);
        c->curveto(x + w - rx * (1 - C1), y,     x + w, y + ry * (1 - C1),       x + w, y + ry);
        if (ry < h2) c->lineto(x + w, y + h - ry);
        c->curveto(x + w, y + h - ry * (1 - C1),     x + w - rx * (1 - C1), y + h,       x + w - rx, y + h);
        if (rx < w2) c->lineto(x + rx, y + h);
        c->curveto(x + rx * (1 - C1), y + h,     x, y + h - ry * (1 - C1),       x, y + h - ry);
        if (ry < h2) c->lineto(x, y + ry);
        c->curveto(x, y + ry * (1 - C1),     x + rx * (1 - C1), y,       x + rx, y);
    } else {
        c->moveto(x + 0.0, y + 0.0);
        c->lineto(x + w, y + 0.0);
        c->lineto(x + w, y + h);
        c->lineto(x + 0.0, y + h);
    }

    c->closepath();
    SP_SHAPE(rect)->setCurveInsync( c, TRUE);
    SP_SHAPE(rect)->setCurveBeforeLPE( c );

    // LPE is not applied because result can generally not be represented as SPRect

    c->unref();
}

/* fixme: Think (Lauris) */

void SPRect::setPosition(gdouble x, gdouble y, gdouble width, gdouble height) {
    this->x.computed = x;
    this->y.computed = y;
    this->width.computed = width;
    this->height.computed = height;

    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPRect::setRx(bool set, gdouble value) {
	this->rx._set = set;

    if (set) {
    	this->rx.computed = value;
    }

    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPRect::setRy(bool set, gdouble value) {
	this->ry._set = set;

    if (set) {
    	this->ry.computed = value;
    }

    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Affine CRect::set_transform(Geom::Affine const& xform) {
    SPRect *rect = this->sprect;
    SPRect* item = rect;

    /* Calculate rect start in parent coords. */
    Geom::Point pos( Geom::Point(rect->x.computed, rect->y.computed) * xform );

    /* This function takes care of translation and scaling, we return whatever parts we can't
       handle. */
    Geom::Affine ret(Geom::Affine(xform).withoutTranslation());
    gdouble const sw = hypot(ret[0], ret[1]);
    gdouble const sh = hypot(ret[2], ret[3]);
    if (sw > 1e-9) {
        ret[0] /= sw;
        ret[1] /= sw;
    } else {
        ret[0] = 1.0;
        ret[1] = 0.0;
    }
    if (sh > 1e-9) {
        ret[2] /= sh;
        ret[3] /= sh;
    } else {
        ret[2] = 0.0;
        ret[3] = 1.0;
    }

    /* fixme: Would be nice to preserve units here */
    rect->width = rect->width.computed * sw;
    rect->height = rect->height.computed * sh;
    if (rect->rx._set) {
        rect->rx = rect->rx.computed * sw;
    }
    if (rect->ry._set) {
        rect->ry = rect->ry.computed * sh;
    }

    /* Find start in item coords */
    pos = pos * ret.inverse();
    rect->x = pos[Geom::X];
    rect->y = pos[Geom::Y];

    this->set_shape();

    // Adjust stroke width
    item->adjust_stroke(sqrt(fabs(sw * sh)));

    // Adjust pattern fill
    item->adjust_pattern(xform * ret.inverse());

    // Adjust gradient fill
    item->adjust_gradient(xform * ret.inverse());

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);

    return ret;
}


/**
Returns the ratio in which the vector from p0 to p1 is stretched by transform
 */
gdouble SPRect::vectorStretch(Geom::Point p0, Geom::Point p1, Geom::Affine xform) {
    if (p0 == p1) {
        return 0;
    }

    return (Geom::distance(p0 * xform, p1 * xform) / Geom::distance(p0, p1));
}

void SPRect::setVisibleRx(gdouble rx) {
    if (rx == 0) {
        this->rx.computed = 0;
        this->rx._set = false;
    } else {
    	this->rx.computed = rx / SPRect::vectorStretch(
            Geom::Point(this->x.computed + 1, this->y.computed),
            Geom::Point(this->x.computed, this->y.computed),
            this->transform);
    	this->rx._set = true;
    }

    this->updateRepr();
}

void SPRect::setVisibleRy(gdouble ry) {
    if (ry == 0) {
    	this->ry.computed = 0;
    	this->ry._set = false;
    } else {
    	this->ry.computed = ry / SPRect::vectorStretch(
            Geom::Point(this->x.computed, this->y.computed + 1),
            Geom::Point(this->x.computed, this->y.computed),
            this->transform);
    	this->ry._set = true;
    }
    this->updateRepr();
}

gdouble SPRect::getVisibleRx() const {
    if (!this->rx._set) {
        return 0;
    }

    return this->rx.computed * SPRect::vectorStretch(
        Geom::Point(this->x.computed + 1, this->y.computed),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
}

gdouble SPRect::getVisibleRy() const {
    if (!this->ry._set) {
        return 0;
    }

    return this->ry.computed * SPRect::vectorStretch(
        Geom::Point(this->x.computed, this->y.computed + 1),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
}

Geom::Rect SPRect::getRect() const {
    Geom::Point p0 = Geom::Point(this->x.computed, this->y.computed);
    Geom::Point p2 = Geom::Point(this->x.computed + this->width.computed, this->y.computed + this->height.computed);

    return Geom::Rect(p0, p2);
}

void SPRect::compensateRxRy(Geom::Affine xform) {
    if (this->rx.computed == 0 && this->ry.computed == 0) {
        return; // nothing to compensate
    }

    // test unit vectors to find out compensation:
    Geom::Point c(this->x.computed, this->y.computed);
    Geom::Point cx = c + Geom::Point(1, 0);
    Geom::Point cy = c + Geom::Point(0, 1);

    // apply previous transform if any
    c *= this->transform;
    cx *= this->transform;
    cy *= this->transform;

    // find out stretches that we need to compensate
    gdouble eX = SPRect::vectorStretch(cx, c, xform);
    gdouble eY = SPRect::vectorStretch(cy, c, xform);

    // If only one of the radii is set, set both radii so they have the same visible length
    // This is needed because if we just set them the same length in SVG, they might end up unequal because of transform
    if ((this->rx._set && !this->ry._set) || (this->ry._set && !this->rx._set)) {
        gdouble r = MAX(this->rx.computed, this->ry.computed);
        this->rx.computed = r / eX;
        this->ry.computed = r / eY;
    } else {
    	this->rx.computed = this->rx.computed / eX;
    	this->ry.computed = this->ry.computed / eY;
    }

    // Note that a radius may end up larger than half-side if the rect is scaled down;
    // that's ok because this preserves the intended radii in case the rect is enlarged again,
    // and set_shape will take care of trimming too large radii when generating d=

    this->rx._set = this->ry._set = true;
}

void SPRect::setVisibleWidth(gdouble width) {
	this->width.computed = width / SPRect::vectorStretch(
        Geom::Point(this->x.computed + 1, this->y.computed),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
	this->width._set = true;
	this->updateRepr();
}

void SPRect::setVisibleHeight(gdouble height) {
	this->height.computed = height / SPRect::vectorStretch(
        Geom::Point(this->x.computed, this->y.computed + 1),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
	this->height._set = true;
	this->updateRepr();
}

gdouble SPRect::getVisibleWidth() const {
    if (!this->width._set) {
        return 0;
    }

    return this->width.computed * SPRect::vectorStretch(
        Geom::Point(this->x.computed + 1, this->y.computed),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
}

gdouble SPRect::getVisibleHeight() const {
    if (!this->height._set) {
        return 0;
    }

    return this->height.computed * SPRect::vectorStretch(
        Geom::Point(this->x.computed, this->y.computed + 1),
        Geom::Point(this->x.computed, this->y.computed),
        this->transform);
}

void CRect::snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) {
    SPRect *rect = this->sprect;
    SPRect* item = rect;

    /* This method overrides sp_shape_snappoints, which is the default for any shape. The default method
    returns all eight points along the path of a rounded rectangle, but not the real corners. Snapping
    the startpoint and endpoint of each rounded corner is not very useful and really confusing. Instead
    we could snap either the real corners, or not snap at all. Bulia Byak opted to snap the real corners,
    but it should be noted that this might be confusing in some cases with relatively large radii. With
    small radii though the user will easily understand which point is snapping. */

    g_assert(item != NULL);
    g_assert(SP_IS_RECT(item));

    Geom::Affine const i2dt (item->i2dt_affine ());

    Geom::Point p0 = Geom::Point(rect->x.computed, rect->y.computed) * i2dt;
    Geom::Point p1 = Geom::Point(rect->x.computed, rect->y.computed + rect->height.computed) * i2dt;
    Geom::Point p2 = Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed) * i2dt;
    Geom::Point p3 = Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed) * i2dt;

    if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_RECT_CORNER)) {
        p.push_back(Inkscape::SnapCandidatePoint(p0, Inkscape::SNAPSOURCE_RECT_CORNER, Inkscape::SNAPTARGET_RECT_CORNER));
        p.push_back(Inkscape::SnapCandidatePoint(p1, Inkscape::SNAPSOURCE_RECT_CORNER, Inkscape::SNAPTARGET_RECT_CORNER));
        p.push_back(Inkscape::SnapCandidatePoint(p2, Inkscape::SNAPSOURCE_RECT_CORNER, Inkscape::SNAPTARGET_RECT_CORNER));
        p.push_back(Inkscape::SnapCandidatePoint(p3, Inkscape::SNAPSOURCE_RECT_CORNER, Inkscape::SNAPTARGET_RECT_CORNER));
    }

    if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_LINE_MIDPOINT)) {
        p.push_back(Inkscape::SnapCandidatePoint((p0 + p1)/2, Inkscape::SNAPSOURCE_LINE_MIDPOINT, Inkscape::SNAPTARGET_LINE_MIDPOINT));
        p.push_back(Inkscape::SnapCandidatePoint((p1 + p2)/2, Inkscape::SNAPSOURCE_LINE_MIDPOINT, Inkscape::SNAPTARGET_LINE_MIDPOINT));
        p.push_back(Inkscape::SnapCandidatePoint((p2 + p3)/2, Inkscape::SNAPSOURCE_LINE_MIDPOINT, Inkscape::SNAPTARGET_LINE_MIDPOINT));
        p.push_back(Inkscape::SnapCandidatePoint((p3 + p0)/2, Inkscape::SNAPSOURCE_LINE_MIDPOINT, Inkscape::SNAPTARGET_LINE_MIDPOINT));
    }

    if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_OBJECT_MIDPOINT)) {
        p.push_back(Inkscape::SnapCandidatePoint((p0 + p2)/2, Inkscape::SNAPSOURCE_OBJECT_MIDPOINT, Inkscape::SNAPTARGET_OBJECT_MIDPOINT));
    }
}

void CRect::convert_to_guides() {
	SPRect* rect = this->sprect;
	SPRect* item = rect;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (!prefs->getBool("/tools/shapes/rect/convertguides", true)) {
        rect->convert_to_guides();
        return;
    }

    std::list<std::pair<Geom::Point, Geom::Point> > pts;

    Geom::Affine const i2dt(rect->i2dt_affine());

    Geom::Point A1(Geom::Point(rect->x.computed, rect->y.computed) * i2dt);
    Geom::Point A2(Geom::Point(rect->x.computed, rect->y.computed + rect->height.computed) * i2dt);
    Geom::Point A3(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed) * i2dt);
    Geom::Point A4(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed) * i2dt);

    pts.push_back(std::make_pair(A1, A2));
    pts.push_back(std::make_pair(A2, A3));
    pts.push_back(std::make_pair(A3, A4));
    pts.push_back(std::make_pair(A4, A1));

    sp_guide_pt_pairs_to_guides(item->document, pts);
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
