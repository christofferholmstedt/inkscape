#define __SP_DESKTOP_STYLE_C__

/** \file
 * Desktop style management
 *
 * Authors:
 *   bulia byak
 *   verbalshadow
 *
 * Copyright (C) 2004, 2006 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "desktop.h"
#include "color-rgba.h"
#include "svg/css-ostringstream.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "selection.h"
#include "inkscape.h"
#include "style.h"
#include "prefs-utils.h"
#include "sp-use.h"
#include "sp-feblend.h"
#include "sp-filter.h"
#include "sp-filter-reference.h"
#include "sp-gaussian-blur.h"
#include "sp-flowtext.h"
#include "sp-flowregion.h"
#include "sp-flowdiv.h"
#include "sp-linear-gradient.h"
#include "sp-pattern.h"
#include "sp-radial-gradient.h"
#include "sp-textpath.h"
#include "sp-tref.h"
#include "sp-tspan.h"
#include "xml/repr.h"
#include "libnrtype/font-style-to-pos.h"

#include "desktop-style.h"

/**
 * Set color on selection on desktop.
 */
void
sp_desktop_set_color(SPDesktop *desktop, ColorRGBA const &color, bool is_relative, bool fill)
{
    /// \todo relative color setting
    if (is_relative) {
        g_warning("FIXME: relative color setting not yet implemented");
        return;
    }

    guint32 rgba = SP_RGBA32_F_COMPOSE(color[0], color[1], color[2], color[3]);
    gchar b[64];
    sp_svg_write_color(b, 64, rgba);
    SPCSSAttr *css = sp_repr_css_attr_new();
    if (fill) {
        sp_repr_css_set_property(css, "fill", b);
        Inkscape::CSSOStringStream osalpha;
        osalpha << color[3];
        sp_repr_css_set_property(css, "fill-opacity", osalpha.str().c_str());
    } else {
        sp_repr_css_set_property(css, "stroke", b);
        Inkscape::CSSOStringStream osalpha;
        osalpha << color[3];
        sp_repr_css_set_property(css, "stroke-opacity", osalpha.str().c_str());
    }

    sp_desktop_set_style(desktop, css);

    sp_repr_css_attr_unref(css);
}

/**
 * Apply style on object and children, recursively.
 */
void
sp_desktop_apply_css_recursive(SPObject *o, SPCSSAttr *css, bool skip_lines)
{
    // non-items should not have style
    if (!SP_IS_ITEM(o))
        return;

    // 1. tspans with role=line are not regular objects in that they are not supposed to have style of their own,
    // but must always inherit from the parent text. Same for textPath.
    // However, if the line tspan or textPath contains some style (old file?), we reluctantly set our style to it too.

    // 2. Generally we allow setting style on clones, but when it's inside flowRegion, do not touch
    // it, be it clone or not; it's just styleless shape (because that's how Inkscape does
    // flowtext).

    if (!(skip_lines
          && ((SP_IS_TSPAN(o) && SP_TSPAN(o)->role == SP_TSPAN_ROLE_LINE)
              || SP_IS_FLOWDIV(o)
              || SP_IS_FLOWPARA(o)
              || SP_IS_TEXTPATH(o))
          && !SP_OBJECT_REPR(o)->attribute("style"))
        &&
        !(SP_IS_FLOWREGION(o) ||
          SP_IS_FLOWREGIONEXCLUDE(o) ||
          (SP_IS_USE(o) &&
           SP_OBJECT_PARENT(o) &&
           (SP_IS_FLOWREGION(SP_OBJECT_PARENT(o)) ||
            SP_IS_FLOWREGIONEXCLUDE(SP_OBJECT_PARENT(o))
           )
          )
         )
        ) {

        SPCSSAttr *css_set = sp_repr_css_attr_new();
        sp_repr_css_merge(css_set, css);

        // Scale the style by the inverse of the accumulated parent transform in the paste context.
        {
            NR::Matrix const local(sp_item_i2doc_affine(SP_ITEM(o)));
            double const ex(NR::expansion(local));
            if ( ( ex != 0. )
                 && ( ex != 1. ) ) {
                sp_css_attr_scale(css_set, 1/ex);
            }
        }

        sp_repr_css_change(SP_OBJECT_REPR(o), css_set, "style");

        sp_repr_css_attr_unref(css_set);
    }

    // setting style on child of clone spills into the clone original (via shared repr), don't do it!
    if (SP_IS_USE(o))
        return;

    for (SPObject *child = sp_object_first_child(SP_OBJECT(o)) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
        if (sp_repr_css_property(css, "opacity", NULL) != NULL) {
            // Unset properties which are accumulating and thus should not be set recursively.
            // For example, setting opacity 0.5 on a group recursively would result in the visible opacity of 0.25 for an item in the group.
            SPCSSAttr *css_recurse = sp_repr_css_attr_new();
            sp_repr_css_merge(css_recurse, css);
            sp_repr_css_set_property(css_recurse, "opacity", NULL);
            sp_desktop_apply_css_recursive(child, css_recurse, skip_lines);
            sp_repr_css_attr_unref(css_recurse);
        } else {
            sp_desktop_apply_css_recursive(child, css, skip_lines);
        }
    }
}

/**
 * Apply style on selection on desktop.
 */
void
sp_desktop_set_style(SPDesktop *desktop, SPCSSAttr *css, bool change, bool write_current)
{
    if (write_current) {
// 1. Set internal value
        sp_repr_css_merge(desktop->current, css);

// 1a. Write to prefs; make a copy and unset any URIs first
        SPCSSAttr *css_write = sp_repr_css_attr_new();
        sp_repr_css_merge(css_write, css);
        sp_css_attr_unset_uris(css_write);
        sp_repr_css_change(inkscape_get_repr(INKSCAPE, "desktop"), css_write, "style");
        sp_repr_css_attr_unref(css_write);
    }

    if (!change)
        return;

// 2. Emit signal
    bool intercepted = desktop->_set_style_signal.emit(css);

/** \todo
 * FIXME: in set_style, compensate pattern and gradient fills, stroke width,
 * rect corners, font size for the object's own transform so that pasting
 * fills does not depend on preserve/optimize.
 */

// 3. If nobody has intercepted the signal, apply the style to the selection
    if (!intercepted) {
        for (GSList const *i = desktop->selection->itemList(); i != NULL; i = i->next) {
            /// \todo if the style is text-only, apply only to texts?
            sp_desktop_apply_css_recursive(SP_OBJECT(i->data), css, true);
        }
    }
}

/**
 * Return the desktop's current style.
 */
SPCSSAttr *
sp_desktop_get_style(SPDesktop *desktop, bool with_text)
{
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_merge(css, desktop->current);
    if (!css->attributeList()) {
        sp_repr_css_attr_unref(css);
        return NULL;
    } else {
        if (!with_text) {
            css = sp_css_attr_unset_text(css);
        }
        return css;
    }
}

/**
 * Return the desktop's current color.
 */
guint32
sp_desktop_get_color(SPDesktop *desktop, bool is_fill)
{
    guint32 r = 0; // if there's no color, return black
    gchar const *property = sp_repr_css_property(desktop->current,
                                                 is_fill ? "fill" : "stroke",
                                                 "#000");

    if (desktop->current && property) { // if there is style and the property in it,
        if (strncmp(property, "url", 3)) { // and if it's not url,
            // read it
            r = sp_svg_read_color(property, r);
        }
    }

    return r;
}

double
sp_desktop_get_master_opacity_tool(SPDesktop *desktop, char const *tool)
{
    SPCSSAttr *css = NULL;
    gfloat value = 1.0; // default if nothing else found
    if (prefs_get_double_attribute(tool, "usecurrent", 0) != 0) {
        css = sp_desktop_get_style(desktop, true);
    } else { 
        Inkscape::XML::Node *tool_repr = inkscape_get_repr(INKSCAPE, tool);
        if (tool_repr) {
            css = sp_repr_css_attr_inherited(tool_repr, "style");
        }
    }
   
    if (css) {
        gchar const *property = css ? sp_repr_css_property(css, "opacity", "1.000") : 0;
           
        if (desktop->current && property) { // if there is style and the property in it,
            if ( !sp_svg_number_read_f(property, &value) ) {
                value = 1.0; // things failed. set back to the default
            }
        }

        sp_repr_css_attr_unref(css);
    }

    return value;
}
double
sp_desktop_get_opacity_tool(SPDesktop *desktop, char const *tool, bool is_fill)
{
    SPCSSAttr *css = NULL;
    gfloat value = 1.0; // default if nothing else found
    if (prefs_get_double_attribute(tool, "usecurrent", 0) != 0) {
        css = sp_desktop_get_style(desktop, true);
    } else { 
        Inkscape::XML::Node *tool_repr = inkscape_get_repr(INKSCAPE, tool);
        if (tool_repr) {
            css = sp_repr_css_attr_inherited(tool_repr, "style");
        }
    }
   
    if (css) {
        gchar const *property = css ? sp_repr_css_property(css, is_fill ? "fill-opacity": "stroke-opacity", "1.000") : 0;
           
        if (desktop->current && property) { // if there is style and the property in it,
            if ( !sp_svg_number_read_f(property, &value) ) {
                value = 1.0; // things failed. set back to the default
            }
        }

        sp_repr_css_attr_unref(css);
    }

    return value;
}
guint32
sp_desktop_get_color_tool(SPDesktop *desktop, char const *tool, bool is_fill)
{
    SPCSSAttr *css = NULL;
    guint32 r = 0; // if there's no color, return black
    if (prefs_get_int_attribute(tool, "usecurrent", 0) != 0) {
        css = sp_desktop_get_style(desktop, true);
    } else {
        Inkscape::XML::Node *tool_repr = inkscape_get_repr(INKSCAPE, tool);
        if (tool_repr) {
            css = sp_repr_css_attr_inherited(tool_repr, "style");
        }
    }
   
    if (css) {
        gchar const *property = sp_repr_css_property(css, is_fill ? "fill" : "stroke", "#000");
           
        if (desktop->current && property) { // if there is style and the property in it,
            if (strncmp(property, "url", 3)) { // and if it's not url,
                // read it
                r = sp_svg_read_color(property, r);
            }
        }

        sp_repr_css_attr_unref(css);
    }

    return r | 0xff;
}
/**
 * Apply the desktop's current style or the tool style to repr.
 */
void
sp_desktop_apply_style_tool(SPDesktop *desktop, Inkscape::XML::Node *repr, char const *tool, bool with_text)
{
    SPCSSAttr *css_current = sp_desktop_get_style(desktop, with_text);
    if ((prefs_get_int_attribute(tool, "usecurrent", 0) != 0) && css_current) {
        sp_repr_css_set(repr, css_current, "style");
    } else {
        Inkscape::XML::Node *tool_repr = inkscape_get_repr(INKSCAPE, tool);
        if (tool_repr) {
            SPCSSAttr *css = sp_repr_css_attr_inherited(tool_repr, "style");
            sp_repr_css_set(repr, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
    if (css_current) {
        sp_repr_css_attr_unref(css_current);
    }
}

/**
 * Returns the font size (in SVG pixels) of the text tool style (if text
 * tool uses its own style) or desktop style (otherwise).
*/
double
sp_desktop_get_font_size_tool(SPDesktop *desktop)
{
    gchar const *desktop_style = inkscape_get_repr(INKSCAPE, "desktop")->attribute("style");
    gchar const *style_str = NULL;
    if ((prefs_get_int_attribute("tools.text", "usecurrent", 0) != 0) && desktop_style) {
        style_str = desktop_style;
    } else {
        Inkscape::XML::Node *tool_repr = inkscape_get_repr(INKSCAPE, "tools.text");
        if (tool_repr) {
            style_str = tool_repr->attribute("style");
        }
    }

    double ret = 12;
    if (style_str) {
        SPStyle *style = sp_style_new(SP_ACTIVE_DOCUMENT);
        sp_style_merge_from_style_string(style, style_str);
        ret = style->font_size.computed;
        sp_style_unref(style);
    }
    return ret;
}

/** Determine average stroke width, simple method */
// see TODO in dialogs/stroke-style.cpp on how to get rid of this eventually
gdouble
stroke_average_width (GSList const *objects)
{
    if (g_slist_length ((GSList *) objects) == 0)
        return NR_HUGE;

    gdouble avgwidth = 0.0;
    bool notstroked = true;
    int n_notstroked = 0;

    for (GSList const *l = objects; l != NULL; l = l->next) {
        if (!SP_IS_ITEM (l->data))
            continue;

        NR::Matrix i2d = sp_item_i2d_affine (SP_ITEM(l->data));

        SPObject *object = SP_OBJECT(l->data);

        if ( object->style->stroke.type == SP_PAINT_TYPE_NONE ) {
            ++n_notstroked;   // do not count nonstroked objects
            continue;
        } else {
            notstroked = false;
        }

        avgwidth += SP_OBJECT_STYLE (object)->stroke_width.computed * i2d.expansion();
    }

    if (notstroked)
        return NR_HUGE;

    return avgwidth / (g_slist_length ((GSList *) objects) - n_notstroked);
}

/**
 * Write to style_res the average fill or stroke of list of objects, if applicable.
 */
int
objects_query_fillstroke (GSList *objects, SPStyle *style_res, bool const isfill)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    SPIPaint *paint_res = isfill? &style_res->fill : &style_res->stroke;
    paint_res->type = SP_PAINT_TYPE_IMPOSSIBLE;
    paint_res->set = TRUE;

    gfloat c[4];
    c[0] = c[1] = c[2] = c[3] = 0.0;
    gint num = 0;

    gfloat prev[3];
    prev[0] = prev[1] = prev[2] = 0.0;
    bool same_color = true;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        SPIPaint *paint = isfill? &style->fill : &style->stroke;

        // We consider paint "effectively set" for anything within text hierarchy
        SPObject *parent = SP_OBJECT_PARENT (obj);
        bool paint_effectively_set = 
            paint->set || (SP_IS_TEXT(parent) || SP_IS_TEXTPATH(parent) || SP_IS_TSPAN(parent)
            || SP_IS_FLOWTEXT(parent) || SP_IS_FLOWDIV(parent) || SP_IS_FLOWPARA(parent) 
            || SP_IS_FLOWTSPAN(parent) || SP_IS_FLOWLINE(parent));

        // 1. Bail out with QUERY_STYLE_MULTIPLE_DIFFERENT if necessary

        if ((paint_res->type != SP_PAINT_TYPE_IMPOSSIBLE) && (paint->type != paint_res->type || (paint_res->set != paint_effectively_set))) {
            return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different types of paint
        }

        if (paint_res->set && paint->set && paint_res->type == SP_PAINT_TYPE_PAINTSERVER) {
            // both previous paint and this paint were a server, see if the servers are compatible

            SPPaintServer *server_res = isfill? SP_STYLE_FILL_SERVER (style_res) : SP_STYLE_STROKE_SERVER (style_res);
            SPPaintServer *server = isfill? SP_STYLE_FILL_SERVER (style) : SP_STYLE_STROKE_SERVER (style);

            if (SP_IS_LINEARGRADIENT (server_res)) {

                if (!SP_IS_LINEARGRADIENT(server))
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different kind of server

                SPGradient *vector = sp_gradient_get_vector ( SP_GRADIENT (server), FALSE );
                SPGradient *vector_res = sp_gradient_get_vector ( SP_GRADIENT (server_res), FALSE );
                if (vector_res != vector)
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different gradient vectors

            } else if (SP_IS_RADIALGRADIENT (server_res)) {

                if (!SP_IS_RADIALGRADIENT(server))
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different kind of server

                SPGradient *vector = sp_gradient_get_vector ( SP_GRADIENT (server), FALSE );
                SPGradient *vector_res = sp_gradient_get_vector ( SP_GRADIENT (server_res), FALSE );
                if (vector_res != vector)
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different gradient vectors

            } else if (SP_IS_PATTERN (server_res)) {

                if (!SP_IS_PATTERN(server))
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different kind of server

                SPPattern *pat = pattern_getroot (SP_PATTERN (server));
                SPPattern *pat_res = pattern_getroot (SP_PATTERN (server_res));
                if (pat_res != pat)
                   return QUERY_STYLE_MULTIPLE_DIFFERENT;  // different pattern roots
            }
        }

        // 2. Sum color, copy server from paint to paint_res

        if (paint_res->set && paint_effectively_set && paint->type == SP_PAINT_TYPE_COLOR) {

            gfloat d[3];
            sp_color_get_rgb_floatv (&paint->value.color, d);

            // Check if this color is the same as previous
            if (paint_res->type == SP_PAINT_TYPE_IMPOSSIBLE) {
                prev[0] = d[0];
                prev[1] = d[1];
                prev[2] = d[2];
            } else {
                if (same_color && (prev[0] != d[0] || prev[1] != d[1] || prev[2] != d[2]))
                    same_color = false;
            }

            // average color
            c[0] += d[0];
            c[1] += d[1];
            c[2] += d[2];
            c[3] += SP_SCALE24_TO_FLOAT (isfill? style->fill_opacity.value : style->stroke_opacity.value);
            num ++;
        }

       paint_res->type = paint->type;
       if (paint_res->set && paint_effectively_set && paint->type == SP_PAINT_TYPE_PAINTSERVER) { // copy the server
           if (isfill) {
               sp_style_set_to_uri_string (style_res, true, style->getFillURI());
           } else {
               sp_style_set_to_uri_string (style_res, false, style->getStrokeURI());
           }
       }
       paint_res->set = paint_effectively_set;
       style_res->fill_rule.computed = style->fill_rule.computed; // no averaging on this, just use the last one
    }

    // After all objects processed, divide the color if necessary and return
    if (paint_res->set && paint_res->type == SP_PAINT_TYPE_COLOR) { // set the color
        g_assert (num >= 1);

        c[0] /= num;
        c[1] /= num;
        c[2] /= num;
        c[3] /= num;
        sp_color_set_rgb_float(&paint_res->value.color, c[0], c[1], c[2]);
        if (isfill) {
            style_res->fill_opacity.value = SP_SCALE24_FROM_FLOAT (c[3]);
        } else {
            style_res->stroke_opacity.value = SP_SCALE24_FROM_FLOAT (c[3]);
        }
        if (num > 1) {
            if (same_color)
                return QUERY_STYLE_MULTIPLE_SAME;
            else
                return QUERY_STYLE_MULTIPLE_AVERAGED;
        } else {
            return QUERY_STYLE_SINGLE;
        }
    }

    // Not color
    if (g_slist_length(objects) > 1) {
        return QUERY_STYLE_MULTIPLE_SAME;
    } else {
        return QUERY_STYLE_SINGLE;
    }
}

/**
 * Write to style_res the average opacity of a list of objects.
 */
int
objects_query_opacity (GSList *objects, SPStyle *style_res)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    gdouble opacity_sum = 0;
    gdouble opacity_prev = -1;
    bool same_opacity = true;
    guint opacity_items = 0;


    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        double opacity = SP_SCALE24_TO_FLOAT(style->opacity.value);
        opacity_sum += opacity;
        if (opacity_prev != -1 && opacity != opacity_prev)
            same_opacity = false;
        opacity_prev = opacity;
        opacity_items ++;
    }
    if (opacity_items > 1)
        opacity_sum /= opacity_items;

    style_res->opacity.value = SP_SCALE24_FROM_FLOAT(opacity_sum);

    if (opacity_items == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (opacity_items == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_opacity)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_AVERAGED;
    }
}

/**
 * Write to style_res the average stroke width of a list of objects.
 */
int
objects_query_strokewidth (GSList *objects, SPStyle *style_res)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    gdouble avgwidth = 0.0;

    gdouble prev_sw = -1;
    bool same_sw = true;

    int n_stroked = 0;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        if (!SP_IS_ITEM(obj)) continue;
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        if ( style->stroke.type == SP_PAINT_TYPE_NONE ) {
            continue;
        }

        n_stroked ++;

        NR::Matrix i2d = sp_item_i2d_affine (SP_ITEM(obj));
        double sw = style->stroke_width.computed * i2d.expansion();

        if (prev_sw != -1 && fabs(sw - prev_sw) > 1e-3)
            same_sw = false;
        prev_sw = sw;

        avgwidth += sw;
    }

    if (n_stroked > 1)
        avgwidth /= (n_stroked);

    style_res->stroke_width.computed = avgwidth;
    style_res->stroke_width.set = true;

    if (n_stroked == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (n_stroked == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_sw)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_AVERAGED;
    }
}

/**
 * Write to style_res the average miter limit of a list of objects.
 */
int
objects_query_miterlimit (GSList *objects, SPStyle *style_res)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    gdouble avgml = 0.0;
    int n_stroked = 0;

    gdouble prev_ml = -1;
    bool same_ml = true;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        if (!SP_IS_ITEM(obj)) continue;
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        if ( style->stroke.type == SP_PAINT_TYPE_NONE ) {
            continue;
        }

        n_stroked ++;

        if (prev_ml != -1 && fabs(style->stroke_miterlimit.value - prev_ml) > 1e-3)
            same_ml = false;
        prev_ml = style->stroke_miterlimit.value;

        avgml += style->stroke_miterlimit.value;
    }

    if (n_stroked > 1)
        avgml /= (n_stroked);

    style_res->stroke_miterlimit.value = avgml;
    style_res->stroke_miterlimit.set = true;

    if (n_stroked == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (n_stroked == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_ml)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_AVERAGED;
    }
}

/**
 * Write to style_res the stroke cap of a list of objects.
 */
int
objects_query_strokecap (GSList *objects, SPStyle *style_res)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    int cap = -1;
    gdouble prev_cap = -1;
    bool same_cap = true;
    int n_stroked = 0;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        if (!SP_IS_ITEM(obj)) continue;
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        if ( style->stroke.type == SP_PAINT_TYPE_NONE ) {
            continue;
        }

        n_stroked ++;

        if (prev_cap != -1 && style->stroke_linecap.value != prev_cap)
            same_cap = false;
        prev_cap = style->stroke_linecap.value;

        cap = style->stroke_linecap.value;
    }

    style_res->stroke_linecap.value = cap;
    style_res->stroke_linecap.set = true;

    if (n_stroked == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (n_stroked == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_cap)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_DIFFERENT;
    }
}

/**
 * Write to style_res the stroke join of a list of objects.
 */
int
objects_query_strokejoin (GSList *objects, SPStyle *style_res)
{
    if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    int join = -1;
    gdouble prev_join = -1;
    bool same_join = true;
    int n_stroked = 0;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        if (!SP_IS_ITEM(obj)) continue;
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        if ( style->stroke.type == SP_PAINT_TYPE_NONE ) {
            continue;
        }

        n_stroked ++;

        if (prev_join != -1 && style->stroke_linejoin.value != prev_join)
            same_join = false;
        prev_join = style->stroke_linejoin.value;

        join = style->stroke_linejoin.value;
    }

    style_res->stroke_linejoin.value = join;
    style_res->stroke_linejoin.set = true;

    if (n_stroked == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (n_stroked == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_join)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_DIFFERENT;
    }
}

/**
 * Write to style_res the average font size and spacing of objects.
 */
int
objects_query_fontnumbers (GSList *objects, SPStyle *style_res)
{
    bool different = false;

    double size = 0;
    double letterspacing = 0;
    double linespacing = 0;
    bool linespacing_normal = false;
    bool letterspacing_normal = false;

    double size_prev = 0;
    double letterspacing_prev = 0;
    double linespacing_prev = 0;

    /// \todo FIXME: add word spacing, kerns? rotates?

    int texts = 0;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);

        if (!SP_IS_TEXT(obj) && !SP_IS_FLOWTEXT(obj)
            && !SP_IS_TSPAN(obj) && !SP_IS_TREF(obj) && !SP_IS_TEXTPATH(obj)
            && !SP_IS_FLOWDIV(obj) && !SP_IS_FLOWPARA(obj) && !SP_IS_FLOWTSPAN(obj))
            continue;

        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        texts ++;
        size += style->font_size.computed * NR::expansion(sp_item_i2d_affine(SP_ITEM(obj))); /// \todo FIXME: we assume non-% units here

        if (style->letter_spacing.normal) {
            if (!different && (letterspacing_prev == 0 || letterspacing_prev == letterspacing))
                letterspacing_normal = true;
        } else {
            letterspacing += style->letter_spacing.computed; /// \todo FIXME: we assume non-% units here
            letterspacing_normal = false;
        }

        double linespacing_current;
        if (style->line_height.normal) {
            linespacing_current = Inkscape::Text::Layout::LINE_HEIGHT_NORMAL;
            if (!different && (linespacing_prev == 0 || linespacing_prev == linespacing_current))
                linespacing_normal = true;
        } else if (style->line_height.unit == SP_CSS_UNIT_PERCENT || style->font_size.computed == 0) {
            linespacing_current = style->line_height.value;
            linespacing_normal = false;
        } else { // we need % here
            linespacing_current = style->line_height.computed / style->font_size.computed;
            linespacing_normal = false;
        }
        linespacing += linespacing_current;

        if ((size_prev != 0 && style->font_size.computed != size_prev) ||
            (letterspacing_prev != 0 && style->letter_spacing.computed != letterspacing_prev) ||
            (linespacing_prev != 0 && linespacing_current != linespacing_prev)) {
            different = true;
        }

        size_prev = style->font_size.computed;
        letterspacing_prev = style->letter_spacing.computed;
        linespacing_prev = linespacing_current;

        // FIXME: we must detect MULTIPLE_DIFFERENT for these too
        style_res->text_anchor.computed = style->text_anchor.computed;
        style_res->writing_mode.computed = style->writing_mode.computed;
    }

    if (texts == 0)
        return QUERY_STYLE_NOTHING;

    if (texts > 1) {
        size /= texts;
        letterspacing /= texts;
        linespacing /= texts;
    }

    style_res->font_size.computed = size;
    style_res->font_size.type = SP_FONT_SIZE_LENGTH;

    style_res->letter_spacing.normal = letterspacing_normal;
    style_res->letter_spacing.computed = letterspacing;

    style_res->line_height.normal = linespacing_normal;
    style_res->line_height.computed = linespacing;
    style_res->line_height.value = linespacing;
    style_res->line_height.unit = SP_CSS_UNIT_PERCENT;

    if (texts > 1) {
        if (different) {
            return QUERY_STYLE_MULTIPLE_AVERAGED;
        } else {
            return QUERY_STYLE_MULTIPLE_SAME;
        }
    } else {
        return QUERY_STYLE_SINGLE;
    }
}

/**
 * Write to style_res the average font style of objects.
 */
int
objects_query_fontstyle (GSList *objects, SPStyle *style_res)
{
    bool different = false;
    bool set = false;

    int texts = 0;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);

        if (!SP_IS_TEXT(obj) && !SP_IS_FLOWTEXT(obj)
            && !SP_IS_TSPAN(obj) && !SP_IS_TREF(obj) && !SP_IS_TEXTPATH(obj)
            && !SP_IS_FLOWDIV(obj) && !SP_IS_FLOWPARA(obj) && !SP_IS_FLOWTSPAN(obj))
            continue;

        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        texts ++;

        if (set &&
            font_style_to_pos(*style_res).signature() != font_style_to_pos(*style).signature() ) {
            different = true;  // different styles
        }

        set = TRUE;
        style_res->font_weight.value = style_res->font_weight.computed = style->font_weight.computed;
        style_res->font_style.value = style_res->font_style.computed = style->font_style.computed;
        style_res->font_stretch.value = style_res->font_stretch.computed = style->font_stretch.computed;
        style_res->font_variant.value = style_res->font_variant.computed = style->font_variant.computed;
        style_res->text_align.value = style_res->text_align.computed = style->text_align.computed;
    }

    if (texts == 0 || !set)
        return QUERY_STYLE_NOTHING;

    if (texts > 1) {
        if (different) {
            return QUERY_STYLE_MULTIPLE_DIFFERENT;
        } else {
            return QUERY_STYLE_MULTIPLE_SAME;
        }
    } else {
        return QUERY_STYLE_SINGLE;
    }
}

/**
 * Write to style_res the average font family of objects.
 */
int
objects_query_fontfamily (GSList *objects, SPStyle *style_res)
{
    bool different = false;
    int texts = 0;

    if (style_res->text->font_family.value) {
        g_free(style_res->text->font_family.value);
        style_res->text->font_family.value = NULL;
    }
    style_res->text->font_family.set = FALSE;

    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);

        if (!SP_IS_TEXT(obj) && !SP_IS_FLOWTEXT(obj)
            && !SP_IS_TSPAN(obj) && !SP_IS_TREF(obj) && !SP_IS_TEXTPATH(obj)
            && !SP_IS_FLOWDIV(obj) && !SP_IS_FLOWPARA(obj) && !SP_IS_FLOWTSPAN(obj))
            continue;

        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;

        texts ++;

        if (style_res->text->font_family.value && style->text->font_family.value &&
            strcmp (style_res->text->font_family.value, style->text->font_family.value)) {
            different = true;  // different fonts
        }

        if (style_res->text->font_family.value) {
            g_free(style_res->text->font_family.value);
            style_res->text->font_family.value = NULL;
        }

        style_res->text->font_family.set = TRUE;
        style_res->text->font_family.value = g_strdup(style->text->font_family.value);
    }

    if (texts == 0 || !style_res->text->font_family.set)
        return QUERY_STYLE_NOTHING;

    if (texts > 1) {
        if (different) {
            return QUERY_STYLE_MULTIPLE_DIFFERENT;
        } else {
            return QUERY_STYLE_MULTIPLE_SAME;
        }
    } else {
        return QUERY_STYLE_SINGLE;
    }
}

int
objects_query_blend (GSList *objects, SPStyle *style_res)
{
    const int empty_prev = -2;
    const int complex_filter = 5;
    int blend = 0;
    float blend_prev = empty_prev;
    bool same_blend = true;
    guint items = 0;
    
    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if(!style || !SP_IS_ITEM(obj)) continue;

        items++;

        //if object has a filter
        if (style->filter.set && style->getFilter()) {
            int blurcount = 0;
            int blendcount = 0;

            // determine whether filter is simple (blend and/or blur) or complex
            for(SPObject *primitive_obj = style->getFilter()->children;
                primitive_obj && SP_IS_FILTER_PRIMITIVE(primitive_obj);
                primitive_obj = primitive_obj->next) {
                SPFilterPrimitive *primitive = SP_FILTER_PRIMITIVE(primitive_obj);
                if(SP_IS_FEBLEND(primitive))
                    ++blendcount;
                else if(SP_IS_GAUSSIANBLUR(primitive))
                    ++blurcount;
                else {
                    blurcount = complex_filter;
                    break;
                }
            }

            // simple filter
            if(blurcount == 1 || blendcount == 1) {
                for(SPObject *primitive_obj = style->getFilter()->children;
                    primitive_obj && SP_IS_FILTER_PRIMITIVE(primitive_obj);
                    primitive_obj = primitive_obj->next) {
                    if(SP_IS_FEBLEND(primitive_obj)) {
                        SPFeBlend *spblend = SP_FEBLEND(primitive_obj);
                        blend = spblend->blend_mode;
                    }
                }
            }
            else {
                blend = complex_filter;
            }
        }
        // defaults to blend mode = "normal"
        else {
            blend = 0;
        }

        if(blend_prev != empty_prev && blend_prev != blend)
            same_blend = false;
        blend_prev = blend;
    }

    if (items > 0) {
        style_res->filter_blend_mode.value = blend;
    }

    if (items == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (items == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if(same_blend)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_DIFFERENT;
    }
}

/**
 * Write to style_res the average blurring of a list of objects.
 */
int
objects_query_blur (GSList *objects, SPStyle *style_res)
{
   if (g_slist_length(objects) == 0) {
        /* No objects, set empty */
        return QUERY_STYLE_NOTHING;
    }

    float blur_sum = 0;
    float blur_prev = -1;
    bool same_blur = true;
    guint blur_items = 0;
    guint items = 0;
    
    for (GSList const *i = objects; i != NULL; i = i->next) {
        SPObject *obj = SP_OBJECT (i->data);
        SPStyle *style = SP_OBJECT_STYLE (obj);
        if (!style) continue;
        if (!SP_IS_ITEM(obj)) continue;

        NR::Matrix i2d = sp_item_i2d_affine (SP_ITEM(obj));

        items ++;

        //if object has a filter
        if (style->filter.set && style->getFilter()) {
            //cycle through filter primitives
            SPObject *primitive_obj = style->getFilter()->children;
            while (primitive_obj) {
                if (SP_IS_FILTER_PRIMITIVE(primitive_obj)) {
                    SPFilterPrimitive *primitive = SP_FILTER_PRIMITIVE(primitive_obj);
                    
                    //if primitive is gaussianblur
                    if(SP_IS_GAUSSIANBLUR(primitive)) {
                        SPGaussianBlur * spblur = SP_GAUSSIANBLUR(primitive);
                        float num = spblur->stdDeviation.getNumber();
                        blur_sum += num * NR::expansion(i2d);
                        if (blur_prev != -1 && fabs (num - blur_prev) > 1e-2) // rather low tolerance because difference in blur radii is much harder to notice than e.g. difference in sizes
                            same_blur = false;
                        blur_prev = num;
                        //TODO: deal with opt number, for the moment it's not necessary to the ui.
                        blur_items ++;
                    }
                }
                primitive_obj = primitive_obj->next;
            }
        }
    }

    if (items > 0) {
        if (blur_items > 0)
            blur_sum /= blur_items;
        style_res->filter_gaussianBlur_deviation.value = blur_sum;
    }

    if (items == 0) {
        return QUERY_STYLE_NOTHING;
    } else if (items == 1) {
        return QUERY_STYLE_SINGLE;
    } else {
        if (same_blur)
            return QUERY_STYLE_MULTIPLE_SAME;
        else
            return QUERY_STYLE_MULTIPLE_AVERAGED;
    }
}

/**
 * Query the given list of objects for the given property, write
 * the result to style, return appropriate flag.
 */
int
sp_desktop_query_style_from_list (GSList *list, SPStyle *style, int property)
{
    if (property == QUERY_STYLE_PROPERTY_FILL) {
        return objects_query_fillstroke (list, style, true);
    } else if (property == QUERY_STYLE_PROPERTY_STROKE) {
        return objects_query_fillstroke (list, style, false);

    } else if (property == QUERY_STYLE_PROPERTY_STROKEWIDTH) {
        return objects_query_strokewidth (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_STROKEMITERLIMIT) {
        return objects_query_miterlimit (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_STROKECAP) {
        return objects_query_strokecap (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_STROKEJOIN) {
        return objects_query_strokejoin (list, style);

    } else if (property == QUERY_STYLE_PROPERTY_MASTEROPACITY) {
        return objects_query_opacity (list, style);

    } else if (property == QUERY_STYLE_PROPERTY_FONTFAMILY) {
        return objects_query_fontfamily (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_FONTSTYLE) {
        return objects_query_fontstyle (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_FONTNUMBERS) {
        return objects_query_fontnumbers (list, style);

    } else if (property == QUERY_STYLE_PROPERTY_BLEND) {
        return objects_query_blend (list, style);
    } else if (property == QUERY_STYLE_PROPERTY_BLUR) {
        return objects_query_blur (list, style);
    }
    return QUERY_STYLE_NOTHING;
}


/**
 * Query the subselection (if any) or selection on the given desktop for the given property, write
 * the result to style, return appropriate flag.
 */
int
sp_desktop_query_style(SPDesktop *desktop, SPStyle *style, int property)
{
    int ret = desktop->_query_style_signal.emit(style, property);

    if (ret != QUERY_STYLE_NOTHING)
        return ret; // subselection returned a style, pass it on

    // otherwise, do querying and averaging over selection
    return sp_desktop_query_style_from_list ((GSList *) desktop->selection->itemList(), style, property);
}

/**
 * Do the same as sp_desktop_query_style for all (defined) style properties, return true if none of
 * the properties returned QUERY_STYLE_NOTHING.
 */
bool
sp_desktop_query_style_all (SPDesktop *desktop, SPStyle *query)
{
        int result_family = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_FONTFAMILY);
        int result_fstyle = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_FONTSTYLE);
        int result_fnumbers = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_FONTNUMBERS);
        int result_fill = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_FILL);
        int result_stroke = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_STROKE);
        int result_strokewidth = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_STROKEWIDTH);
        int result_strokemiterlimit = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_STROKEMITERLIMIT);
        int result_strokecap = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_STROKECAP);
        int result_strokejoin = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_STROKEJOIN);
        int result_opacity = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_MASTEROPACITY);
        int result_blur = sp_desktop_query_style (desktop, query, QUERY_STYLE_PROPERTY_BLUR);
        
        return (result_family != QUERY_STYLE_NOTHING && result_fstyle != QUERY_STYLE_NOTHING && result_fnumbers != QUERY_STYLE_NOTHING && result_fill != QUERY_STYLE_NOTHING && result_stroke != QUERY_STYLE_NOTHING && result_opacity != QUERY_STYLE_NOTHING && result_strokewidth != QUERY_STYLE_NOTHING && result_strokemiterlimit != QUERY_STYLE_NOTHING && result_strokecap != QUERY_STYLE_NOTHING && result_strokejoin != QUERY_STYLE_NOTHING && result_blur != QUERY_STYLE_NOTHING);
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
