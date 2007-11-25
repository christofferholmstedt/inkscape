/** \file
 * Enhanced Metafile Input and Output.
 */
/*
 * Authors:
 *   Ulf Erikson <ulferikson@users.sf.net>
 *
 * Copyright (C) 2006 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
/*
 * References:
 *  - How to Create & Play Enhanced Metafiles in Win32
 *      http://support.microsoft.com/kb/q145999/
 *  - INFO: Windows Metafile Functions & Aldus Placeable Metafiles
 *      http://support.microsoft.com/kb/q66949/
 *  - Metafile Functions
 *      http://msdn.microsoft.com/library/en-us/gdi/metafile_0whf.asp
 *  - Metafile Structures
 *      http://msdn.microsoft.com/library/en-us/gdi/metafile_5hkj.asp
 */

#ifdef WIN32

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "win32.h"
#include "emf-win32-print.h"
#include "emf-win32-inout.h"
#include "inkscape.h"
#include "sp-path.h"
#include "style.h"
#include "color.h"
#include "display/curve.h"
#include "libnr/n-art-bpath.h"
#include "libnr/nr-point-matrix-ops.h"
#include "gtk/gtk.h"
#include "print.h"
#include "glibmm/i18n.h"
#include "extension/extension.h"
#include "extension/system.h"
#include "extension/print.h"
#include "extension/db.h"
#include "extension/output.h"
#include "document.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"

#include "libnr/nr-rect.h"
#include "libnr/nr-matrix.h"
#include "libnr/nr-pixblock.h"

#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>

#include "io/sys.h"

#include "unit-constants.h"

#include "clear-n_.h"


#define PRINT_EMF_WIN32 "org.inkscape.print.emf.win32"

#ifndef PS_JOIN_MASK
#define PS_JOIN_MASK (PS_JOIN_BEVEL|PS_JOIN_MITER|PS_JOIN_ROUND)
#endif


namespace Inkscape {
namespace Extension {
namespace Internal {


EmfWin32::EmfWin32 (void) // The null constructor
{
    return;
}


EmfWin32::~EmfWin32 (void) //The destructor
{
    return;
}


bool
EmfWin32::check (Inkscape::Extension::Extension * module)
{
    if (NULL == Inkscape::Extension::db.get(PRINT_EMF_WIN32))
        return FALSE;
    return TRUE;
}


static void
emf_print_document_to_file(SPDocument *doc, gchar const *filename)
{
    Inkscape::Extension::Print *mod;
    SPPrintContext context;
    gchar const *oldconst;
    gchar *oldoutput;
    unsigned int ret;

    sp_document_ensure_up_to_date(doc);

    mod = Inkscape::Extension::get_print(PRINT_EMF_WIN32);
    oldconst = mod->get_param_string("destination");
    oldoutput = g_strdup(oldconst);
    mod->set_param_string("destination", (gchar *)filename);

/* Start */
    context.module = mod;
    /* fixme: This has to go into module constructor somehow */
    /* Create new arena */
    mod->base = SP_ITEM(sp_document_root(doc));
    mod->arena = NRArena::create();
    mod->dkey = sp_item_display_key_new(1);
    mod->root = sp_item_invoke_show(mod->base, mod->arena, mod->dkey, SP_ITEM_SHOW_DISPLAY);
    /* Print document */
    ret = mod->begin(doc);
    if (ret) {
        throw Inkscape::Extension::Output::save_failed();
    }
    sp_item_invoke_print(mod->base, &context);
    ret = mod->finish();
    /* Release arena */
    sp_item_invoke_hide(mod->base, mod->dkey);
    mod->base = NULL;
    nr_arena_item_unref(mod->root);
    mod->root = NULL;
    nr_object_unref((NRObject *) mod->arena);
    mod->arena = NULL;
/* end */

    mod->set_param_string("destination", oldoutput);
    g_free(oldoutput);

    return;
}


void
EmfWin32::save (Inkscape::Extension::Output *mod, SPDocument *doc, const gchar *uri)
{
    Inkscape::Extension::Extension * ext;

    ext = Inkscape::Extension::db.get(PRINT_EMF_WIN32);
    if (ext == NULL)
        return;

    bool old_textToPath  = ext->get_param_bool("textToPath");
    bool new_val         = mod->get_param_bool("textToPath");
    ext->set_param_bool("textToPath", new_val);

    gchar * final_name;
    final_name = g_strdup_printf("%s", uri);
    emf_print_document_to_file(doc, final_name);
    g_free(final_name);

    ext->set_param_bool("textToPath", old_textToPath);

    return;
}



typedef struct {
    int type;
    ENHMETARECORD *lpEMFR;
} EMF_OBJECT, *PEMF_OBJECT;

typedef struct emf_callback_data {
    Glib::ustring *outsvg;
    Glib::ustring *path;
    struct SPStyle style;
    class SPTextStyle tstyle;
    bool stroke_set;
    bool fill_set;
    double xDPI, yDPI;
    bool pathless_stroke;

    SIZEL sizeWnd;
    SIZEL sizeView;
    float PixelsX;
    float PixelsY;
    float MMX;
    float MMY;
    float dwInchesX;
    float dwInchesY;
    POINTL winorg;
    POINTL vieworg;
    double ScaleX, ScaleY;

    int n_obj;
    PEMF_OBJECT emf_obj;
} EMF_CALLBACK_DATA, *PEMF_CALLBACK_DATA;


static void
output_style(PEMF_CALLBACK_DATA d, int iType)
{
    SVGOStringStream tmp_style;
    char tmp[1024] = {0};

    float fill_rgb[3];
    sp_color_get_rgb_floatv( &(d->style.fill.value.color), fill_rgb );
    
    float stroke_rgb[3];
    sp_color_get_rgb_floatv(&(d->style.stroke.value.color), stroke_rgb);

    *(d->outsvg) += "\n\tstyle=\"";
    if (iType == EMR_STROKEPATH || !d->fill_set) {
        tmp_style << "fill:none;";
    } else {
        snprintf(tmp, 1023,
                 "fill:#%02x%02x%02x;",
                 SP_COLOR_F_TO_U(fill_rgb[0]),
                 SP_COLOR_F_TO_U(fill_rgb[1]),
                 SP_COLOR_F_TO_U(fill_rgb[2]));
        tmp_style << tmp;
        snprintf(tmp, 1023,
                 "fill-rule:%s;",
                 d->style.fill_rule.value != 0 ? "evenodd" : "nonzero");
        tmp_style << tmp;
        tmp_style << "fill-opacity:1;";

        if (d->fill_set && d->stroke_set && d->style.stroke_width.value == 1 &&
            fill_rgb[0]==stroke_rgb[0] && fill_rgb[1]==stroke_rgb[1] && fill_rgb[2]==stroke_rgb[2])
        {
            d->stroke_set = false;
        }
    }

    if (iType == EMR_FILLPATH || !d->stroke_set) {
        tmp_style << "stroke:none;";
    } else {
        snprintf(tmp, 1023,
                 "stroke:#%02x%02x%02x;",
                 SP_COLOR_F_TO_U(stroke_rgb[0]),
                 SP_COLOR_F_TO_U(stroke_rgb[1]),
                 SP_COLOR_F_TO_U(stroke_rgb[2]));
        tmp_style << tmp;

        tmp_style << "stroke-width:" <<
            MAX( 0.001, d->style.stroke_width.value ) << "px;";

        tmp_style << "stroke-linejoin:" <<
            (d->style.stroke_linejoin.computed == 0 ? "miter" :
             d->style.stroke_linejoin.computed == 1 ? "round" :
             d->style.stroke_linejoin.computed == 2 ? "bevel" :
             "unknown") << ";";

        if (d->style.stroke_linejoin.computed == 0) {
            tmp_style << "stroke-miterlimit:" <<
                MAX( 0.01, d->style.stroke_miterlimit.value ) << ";";
        }

        if (d->style.stroke_dasharray_set &&
            d->style.stroke_dash.n_dash && d->style.stroke_dash.dash)
        {
            tmp_style << "stroke-dasharray:";
            for (int i=0; i<d->style.stroke_dash.n_dash; i++) {
                if (i)
                    tmp_style << ",";
                tmp_style << d->style.stroke_dash.dash[i];
            }
            tmp_style << ";";
            tmp_style << "stroke-dashoffset:0;";
        } else {
            tmp_style << "stroke-dasharray:none;";
        }
        tmp_style << "stroke-opacity:1;";
    }
    tmp_style << "\" ";

    *(d->outsvg) += tmp_style.str().c_str();
}


static double
pix_x_to_point(PEMF_CALLBACK_DATA d, double px)
{
    double tmp = px - d->winorg.x;
    tmp *= (double) PX_PER_IN / d->ScaleX;
    tmp += d->vieworg.x;
    return tmp;
}


static double
pix_y_to_point(PEMF_CALLBACK_DATA d, double px)
{
    double tmp = px - d->winorg.y;
    tmp *= (double) PX_PER_IN / d->ScaleY;
    tmp += d->vieworg.y;
    return tmp;
}


static double
pix_size_to_point(PEMF_CALLBACK_DATA d, double px)
{
    double tmp = px;
    tmp *= (double) PX_PER_IN / d->ScaleX;
    return tmp;
}


static void
select_pen(PEMF_CALLBACK_DATA d, int index)
{
    PEMRCREATEPEN pEmr = NULL;

    if (index >= 0 && index < d->n_obj)
        pEmr = (PEMRCREATEPEN) d->emf_obj[index].lpEMFR;

    if (!pEmr)
        return;

    switch (pEmr->lopn.lopnStyle) {
        case PS_DASH:
        case PS_DOT:
        case PS_DASHDOT:
        case PS_DASHDOTDOT:
        {
            int i = 0;
            d->style.stroke_dash.n_dash =
                PS_DASHDOTDOT ? 6 : PS_DASHDOT ? 4 : 2;
            if (d->style.stroke_dash.dash)
                delete[] d->style.stroke_dash.dash;
            d->style.stroke_dash.dash = new double[d->style.stroke_dash.n_dash];
            if (pEmr->lopn.lopnStyle==PS_DASH || pEmr->lopn.lopnStyle==PS_DASHDOT || pEmr->lopn.lopnStyle==PS_DASHDOTDOT) {
                d->style.stroke_dash.dash[i++] = 3;
                d->style.stroke_dash.dash[i++] = 1;
            }
            if (pEmr->lopn.lopnStyle==PS_DOT || pEmr->lopn.lopnStyle==PS_DASHDOT || pEmr->lopn.lopnStyle==PS_DASHDOTDOT) {
                d->style.stroke_dash.dash[i++] = 1;
                d->style.stroke_dash.dash[i++] = 1;
            }
            if (pEmr->lopn.lopnStyle==PS_DASHDOTDOT) {
                d->style.stroke_dash.dash[i++] = 1;
                d->style.stroke_dash.dash[i++] = 1;
            }
            
            d->style.stroke_dasharray_set = 1;
            break;
        }
        
        case PS_SOLID:
        default:
        {
            d->style.stroke_dasharray_set = 0;
            break;
        }
    }

    d->stroke_set = true;

    if (pEmr->lopn.lopnStyle == PS_NULL) {
        d->style.stroke_width.value = 0;
        d->stroke_set = false;
    } else if (pEmr->lopn.lopnWidth.x) {
        d->style.stroke_width.value = pix_size_to_point( d, pEmr->lopn.lopnWidth.x );
    } else { // this stroke should always be rendered as 1 pixel wide, independent of zoom level (can that be done in SVG?)
        //d->style.stroke_width.value = 1.0;
        d->style.stroke_width.value = pix_size_to_point( d, 1 );
    }

    double r, g, b;
    r = SP_COLOR_U_TO_F( GetRValue(pEmr->lopn.lopnColor) );
    g = SP_COLOR_U_TO_F( GetGValue(pEmr->lopn.lopnColor) );
    b = SP_COLOR_U_TO_F( GetBValue(pEmr->lopn.lopnColor) );
    d->style.stroke.value.color.set( r, g, b );

    d->style.stroke_linejoin.computed = 1;
}


static void
select_extpen(PEMF_CALLBACK_DATA d, int index)
{
    PEMREXTCREATEPEN pEmr = NULL;

    if (index >= 0 && index < d->n_obj)
        pEmr = (PEMREXTCREATEPEN) d->emf_obj[index].lpEMFR;

    if (!pEmr)
        return;

    switch (pEmr->elp.elpPenStyle & PS_STYLE_MASK) {
        case PS_USERSTYLE:
        {
            if (pEmr->elp.elpNumEntries) {
                d->style.stroke_dash.n_dash = pEmr->elp.elpNumEntries;
                if (d->style.stroke_dash.dash)
                    delete[] d->style.stroke_dash.dash;
                d->style.stroke_dash.dash = new double[pEmr->elp.elpNumEntries];
                for (unsigned int i=0; i<pEmr->elp.elpNumEntries; i++) {
                    d->style.stroke_dash.dash[i] = pix_size_to_point( d, pEmr->elp.elpStyleEntry[i] );
                }
                d->style.stroke_dasharray_set = 1;
            } else {
                d->style.stroke_dasharray_set = 0;
            }
            break;
        }
        default:
        {
            d->style.stroke_dasharray_set = 0;
            break;
        }
    }

    switch (pEmr->elp.elpPenStyle & PS_ENDCAP_MASK) {
        case PS_ENDCAP_ROUND:
        {
            d->style.stroke_linecap.computed = 1;
            break;
        }
        case PS_ENDCAP_SQUARE:
        {
            d->style.stroke_linecap.computed = 2;
            break;
        }
        case PS_ENDCAP_FLAT:
        default:
        {
            d->style.stroke_linecap.computed = 0;
            break;
        }
    }

    switch (pEmr->elp.elpPenStyle & PS_JOIN_MASK) {
        case PS_JOIN_BEVEL:
        {
            d->style.stroke_linejoin.computed = 2;
            break;
        }
        case PS_JOIN_MITER:
        {
            d->style.stroke_linejoin.computed = 0;
            break;
        }
        case PS_JOIN_ROUND:
        default:
        {
            d->style.stroke_linejoin.computed = 1;
            break;
        }
    }

    d->style.stroke_width.value = pix_size_to_point( d, pEmr->elp.elpWidth );

    double r, g, b;
    r = SP_COLOR_U_TO_F( GetRValue(pEmr->elp.elpColor) );
    g = SP_COLOR_U_TO_F( GetGValue(pEmr->elp.elpColor) );
    b = SP_COLOR_U_TO_F( GetBValue(pEmr->elp.elpColor) );

    d->style.stroke.value.color.set( r, g, b );

    d->stroke_set = true;
}


static void
select_brush(PEMF_CALLBACK_DATA d, int index)
{
    PEMRCREATEBRUSHINDIRECT pEmr = NULL;

    if (index >= 0 && index < d->n_obj)
        pEmr = (PEMRCREATEBRUSHINDIRECT) d->emf_obj[index].lpEMFR;

    if (!pEmr)
        return;

    if (pEmr->lb.lbStyle == BS_SOLID) {
        double r, g, b;
        r = SP_COLOR_U_TO_F( GetRValue(pEmr->lb.lbColor) );
        g = SP_COLOR_U_TO_F( GetGValue(pEmr->lb.lbColor) );
        b = SP_COLOR_U_TO_F( GetBValue(pEmr->lb.lbColor) );
        d->style.fill.value.color.set( r, g, b );
    }

    d->fill_set = true;
}


static void
select_font(PEMF_CALLBACK_DATA d, int index)
{
    PEMREXTCREATEFONTINDIRECTW pEmr = NULL;

    if (index >= 0 && index < d->n_obj)
        pEmr = (PEMREXTCREATEFONTINDIRECTW) d->emf_obj[index].lpEMFR;

    if (!pEmr)
        return;

    d->style.font_size.computed = pix_size_to_point( d, pEmr->elfw.elfLogFont.lfHeight );
    d->style.font_weight.value = pEmr->elfw.elfLogFont.lfWeight;
    d->style.font_style.value = (pEmr->elfw.elfLogFont.lfItalic ? SP_CSS_FONT_STYLE_ITALIC : SP_CSS_FONT_STYLE_NORMAL);
    d->style.text_decoration.underline = pEmr->elfw.elfLogFont.lfUnderline;
    d->style.text_decoration.line_through = pEmr->elfw.elfLogFont.lfStrikeOut;
    if (d->tstyle.font_family.value)
        g_free(d->tstyle.font_family.value);
    d->tstyle.font_family.value =
        (gchar *) g_utf16_to_utf8( (gunichar2*) pEmr->elfw.elfLogFont.lfFaceName, -1, NULL, NULL, NULL );
}

static void
delete_object(PEMF_CALLBACK_DATA d, int index)
{
    if (index >= 0 && index < d->n_obj) {
        d->emf_obj[index].type = 0;
        if (d->emf_obj[index].lpEMFR)
            free(d->emf_obj[index].lpEMFR);
        d->emf_obj[index].lpEMFR = NULL;
    }
}


static void
insert_object(PEMF_CALLBACK_DATA d, int index, int type, ENHMETARECORD *pObj)
{
    if (index >= 0 && index < d->n_obj) {
        delete_object(d, index);
        d->emf_obj[index].type = type;
        d->emf_obj[index].lpEMFR = pObj;
    }
}


static int CALLBACK
myEnhMetaFileProc(HDC hDC, HANDLETABLE *lpHTable, ENHMETARECORD *lpEMFR, int nObj, LPARAM lpData)
{
    PEMF_CALLBACK_DATA d;
    SVGOStringStream tmp_outsvg;
    SVGOStringStream tmp_path;
    SVGOStringStream tmp_str;

    d = (PEMF_CALLBACK_DATA) lpData;

    if (d->pathless_stroke) {
        if (lpEMFR->iType!=EMR_POLYBEZIERTO && lpEMFR->iType!=EMR_POLYBEZIERTO16 &&
            lpEMFR->iType!=EMR_POLYLINETO && lpEMFR->iType!=EMR_POLYLINETO16 &&
            lpEMFR->iType!=EMR_LINETO && lpEMFR->iType!=EMR_ARCTO)
        {
            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEPATH);
            *(d->outsvg) += "\n\t";
            *(d->outsvg) += *(d->path);
            *(d->outsvg) += " \" /> \n";
            *(d->path) = "";
            d->pathless_stroke = false;
        }
    }

    switch (lpEMFR->iType)
    {
        case EMR_HEADER:
        {
            ENHMETAHEADER *pEmr = (ENHMETAHEADER *) lpEMFR;
            tmp_outsvg << "<svg\n";

            d->xDPI = 2540;
            d->yDPI = 2540;

            d->PixelsX = pEmr->rclFrame.right - pEmr->rclFrame.left;
            d->PixelsY = pEmr->rclFrame.bottom - pEmr->rclFrame.top;

            d->MMX = d->PixelsX / 100.0;
            d->MMY = d->PixelsY / 100.0;

            tmp_outsvg <<
                "  width=\"" << d->MMX << "mm\"\n" <<
                "  height=\"" << d->MMY << "mm\">\n";

            if (pEmr->nHandles) {
                d->n_obj = pEmr->nHandles;
                d->emf_obj = new EMF_OBJECT[d->n_obj];
                
                // Init the new emf_obj list elements to null, provided the
                // dynamic allocation succeeded.
                if ( d->emf_obj != NULL )
                {
                    for( unsigned int i=0; i < d->n_obj; ++i )
                        d->emf_obj[i].lpEMFR = NULL;
                } //if

            } else {
                d->emf_obj = NULL;
            }

            break;
        }
        case EMR_POLYBEZIER:
        {
            PEMRPOLYBEZIER pEmr = (PEMRPOLYBEZIER) lpEMFR;
            DWORD i,j;

            if (pEmr->cptl<4)
                break;

            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            tmp_str <<
                "\n\tM " <<
                pix_x_to_point( d, pEmr->aptl[0].x ) << " " <<
                pix_x_to_point( d, pEmr->aptl[0].y) << " ";

            for (i=1; i<pEmr->cptl; ) {
                tmp_str << "\n\tC ";
                for (j=0; j<3 && i<pEmr->cptl; j++,i++) {
                    tmp_str <<
                        pix_x_to_point( d, pEmr->aptl[i].x ) << " " <<
                        pix_y_to_point( d, pEmr->aptl[i].y ) << " ";
                }
            }

            *(d->outsvg) += tmp_str.str().c_str();
            *(d->outsvg) += " \" /> \n";

            break;
        }
        case EMR_POLYGON:
        {
            EMRPOLYGON *pEmr = (EMRPOLYGON *) lpEMFR;
            DWORD i;

            if (pEmr->cptl < 2)
                break;

            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEANDFILLPATH);
            *(d->outsvg) += "\n\td=\"";

            tmp_str <<
                "\n\tM " <<
                pix_x_to_point( d, pEmr->aptl[0].x ) << " " <<
                pix_y_to_point( d, pEmr->aptl[0].y ) << " ";

            for (i=1; i<pEmr->cptl; i++) {
                tmp_str <<
                    "\n\tL " <<
                    pix_x_to_point( d, pEmr->aptl[i].x ) << " " <<
                    pix_y_to_point( d, pEmr->aptl[i].y ) << " ";
            }

            *(d->outsvg) += tmp_str.str().c_str();
            *(d->outsvg) += " z \" /> \n";

            break;
        }
        case EMR_POLYLINE:
        {
            EMRPOLYLINE *pEmr = (EMRPOLYLINE *) lpEMFR;
            DWORD i;

            if (pEmr->cptl<2)
                break;

            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            tmp_str <<
                "\n\tM " <<
                pix_x_to_point( d, pEmr->aptl[0].x ) << " " <<
                pix_y_to_point( d, pEmr->aptl[0].y ) << " ";

            for (i=1; i<pEmr->cptl; i++) {
                tmp_str <<
                    "\n\tL " <<
                    pix_x_to_point( d, pEmr->aptl[i].x ) << " " <<
                    pix_y_to_point( d, pEmr->aptl[i].y ) << " ";
            }

            *(d->outsvg) += tmp_str.str().c_str();
            *(d->outsvg) += " \" /> \n";

            break;
        }
        case EMR_POLYBEZIERTO:
        {
            PEMRPOLYBEZIERTO pEmr = (PEMRPOLYBEZIERTO) lpEMFR;
            DWORD i,j;

            if (d->path->empty())
                *(d->path) = "d=\"";

            for (i=0; i<pEmr->cptl;) {
                tmp_path << "\n\tC ";
                for (j=0; j<3 && i<pEmr->cptl; j++,i++) {
                    tmp_path <<
                        pix_x_to_point( d, pEmr->aptl[i].x ) << " " <<
                        pix_y_to_point( d, pEmr->aptl[i].y ) << " ";
                }
            }

            break;
        }
        case EMR_POLYLINETO:
        {
            PEMRPOLYLINETO pEmr = (PEMRPOLYLINETO) lpEMFR;
            DWORD i;

            if (d->path->empty())
                *(d->path) = "d=\"";

            for (i=0; i<pEmr->cptl;i++) {
                tmp_path <<
                    "\n\tL " <<
                    pix_x_to_point( d, pEmr->aptl[i].x ) << " " <<
                    pix_y_to_point( d, pEmr->aptl[i].y ) << " ";
            }

            break;
        }
        case EMR_POLYPOLYLINE:
        case EMR_POLYPOLYGON:
        {
            PEMRPOLYPOLYGON pEmr = (PEMRPOLYPOLYGON) lpEMFR;
            unsigned int n, i, j;

            if (lpEMFR->iType == EMR_POLYPOLYGON)
                *(d->outsvg) += "<!-- EMR_POLYPOLYGON... -->\n";
            else
                *(d->outsvg) += "<!-- EMR_POLYPOLYLINE... -->\n";

            *(d->outsvg) += "<path ";
            output_style(d, lpEMFR->iType==EMR_POLYPOLYGON ? EMR_STROKEANDFILLPATH : EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            POINTL *aptl = (POINTL *) &pEmr->aPolyCounts[pEmr->nPolys];

            i = 0;
            for (n=0; n<pEmr->nPolys && i<pEmr->cptl; n++) {
                SVGOStringStream poly_path;

                poly_path << "\n\tM " <<
                    pix_x_to_point( d, aptl[i].x ) << " " <<
                    pix_y_to_point( d, aptl[i].y ) << " ";
                i++;

                for (j=1; j<pEmr->aPolyCounts[n] && i<pEmr->cptl; j++) {
                    poly_path << "\n\tL " <<
                        pix_x_to_point( d, aptl[i].x ) << " " <<
                        pix_y_to_point( d, aptl[i].y ) << " ";
                    i++;
                }

                *(d->outsvg) += poly_path.str().c_str();
                if (lpEMFR->iType == EMR_POLYPOLYGON)
                    *(d->outsvg) += " z";
                *(d->outsvg) += " \n";
            }

            *(d->outsvg) += " \" /> \n";
            break;
        }
        case EMR_SETWINDOWEXTEX:
        {
            PEMRSETWINDOWEXTEX pEmr = (PEMRSETWINDOWEXTEX) lpEMFR;

            *(d->outsvg) += "<!-- EMR_SETWINDOWEXTEX -->\n";

            d->sizeWnd = pEmr->szlExtent;
            d->PixelsX = d->sizeWnd.cx;
            d->PixelsY = d->sizeWnd.cy;

            d->ScaleX = d->xDPI / (100*d->MMX / d->PixelsX);
            d->ScaleY = d->yDPI / (100*d->MMY / d->PixelsY);

            break;
        }
        case EMR_SETWINDOWORGEX:
        {
            PEMRSETWINDOWORGEX pEmr = (PEMRSETWINDOWORGEX) lpEMFR;
            d->winorg = pEmr->ptlOrigin;
            break;
        }
        case EMR_SETVIEWPORTEXTEX:
        {
            PEMRSETVIEWPORTEXTEX pEmr = (PEMRSETVIEWPORTEXTEX) lpEMFR;

            *(d->outsvg) += "<!-- EMR_SETVIEWPORTEXTEX -->\n";

            d->sizeView = pEmr->szlExtent;

            if (d->sizeWnd.cx && d->sizeWnd.cy) {
                HDC hScreenDC = GetDC( NULL );

                float scrPixelsX = (float)GetDeviceCaps( hScreenDC, HORZRES );
                float scrPixelsY = (float)GetDeviceCaps( hScreenDC, VERTRES );
                float scrMMX = (float)GetDeviceCaps( hScreenDC, HORZSIZE );
                float scrMMY = (float)GetDeviceCaps( hScreenDC, VERTSIZE );

                ReleaseDC( NULL, hScreenDC );

                d->dwInchesX = d->sizeView.cx / (25.4f*scrPixelsX/scrMMX);
                d->dwInchesY = d->sizeView.cy / (25.4f*scrPixelsY/scrMMY);
                d->xDPI = d->sizeWnd.cx / d->dwInchesX;
                d->yDPI = d->sizeWnd.cy / d->dwInchesY;

                if (1) {
                    d->xDPI = 2540;
                    d->yDPI = 2540;
                    d->dwInchesX = d->PixelsX / d->xDPI;
                    d->dwInchesY = d->PixelsY / d->yDPI;
                    d->ScaleX = d->xDPI;
                    d->ScaleY = d->yDPI;
                }

                d->MMX = d->dwInchesX * MM_PER_IN;
                d->MMY = d->dwInchesY * MM_PER_IN;
            }

            break;
        }
        case EMR_SETVIEWPORTORGEX:
        {
            PEMRSETVIEWPORTORGEX pEmr = (PEMRSETVIEWPORTORGEX) lpEMFR;
            d->vieworg = pEmr->ptlOrigin;
            break;
        }
        case EMR_SETBRUSHORGEX:
            *(d->outsvg) += "<!-- EMR_SETBRUSHORGEX -->\n";
            break;
        case EMR_EOF:
        {
            tmp_outsvg << "</svg>\n";
            break;
        }
        case EMR_SETPIXELV:
            *(d->outsvg) += "<!-- EMR_SETPIXELV -->\n";
            break;
        case EMR_SETMAPPERFLAGS:
            *(d->outsvg) += "<!-- EMR_SETMAPPERFLAGS -->\n";
            break;
        case EMR_SETMAPMODE:
            *(d->outsvg) += "<!-- EMR_SETMAPMODE -->\n";
            break;
        case EMR_SETBKMODE:
            *(d->outsvg) += "<!-- EMR_SETBKMODE -->\n";
            break;
        case EMR_SETPOLYFILLMODE:
        {
            PEMRSETPOLYFILLMODE pEmr = (PEMRSETPOLYFILLMODE) lpEMFR;
            d->style.fill_rule.value =
                (pEmr->iMode == WINDING ? 0 :
                 pEmr->iMode == ALTERNATE ? 1 : 0);
            break;
        }
        case EMR_SETROP2:
            *(d->outsvg) += "<!-- EMR_SETROP2 -->\n";
            break;
        case EMR_SETSTRETCHBLTMODE:
            *(d->outsvg) += "<!-- EMR_SETSTRETCHBLTMODE -->\n";
            break;
        case EMR_SETTEXTALIGN:
            *(d->outsvg) += "<!-- EMR_SETTEXTALIGN -->\n";
            break;
        case EMR_SETCOLORADJUSTMENT:
            *(d->outsvg) += "<!-- EMR_SETCOLORADJUSTMENT -->\n";
            break;
        case EMR_SETTEXTCOLOR:
            *(d->outsvg) += "<!-- EMR_SETTEXTCOLOR -->\n";
            break;
        case EMR_SETBKCOLOR:
            *(d->outsvg) += "<!-- EMR_SETBKCOLOR -->\n";
            break;
        case EMR_OFFSETCLIPRGN:
            *(d->outsvg) += "<!-- EMR_OFFSETCLIPRGN -->\n";
            break;
        case EMR_MOVETOEX:
        {
            PEMRMOVETOEX pEmr = (PEMRMOVETOEX) lpEMFR;

            if (d->path->empty()) {
                d->pathless_stroke = true;
                *(d->path) = "d=\"";
            }

            tmp_path <<
                "\n\tM " <<
                pix_x_to_point( d, pEmr->ptl.x ) << " " <<
                pix_y_to_point( d, pEmr->ptl.y ) << " ";
            break;
        }
        case EMR_SETMETARGN:
            *(d->outsvg) += "<!-- EMR_SETMETARGN -->\n";
            break;
        case EMR_EXCLUDECLIPRECT:
            *(d->outsvg) += "<!-- EMR_EXCLUDECLIPRECT -->\n";
            break;
        case EMR_INTERSECTCLIPRECT:
            *(d->outsvg) += "<!-- EMR_INTERSECTCLIPRECT -->\n";
            break;
        case EMR_SCALEVIEWPORTEXTEX:
            *(d->outsvg) += "<!-- EMR_SCALEVIEWPORTEXTEX -->\n";
            break;
        case EMR_SCALEWINDOWEXTEX:
            *(d->outsvg) += "<!-- EMR_SCALEWINDOWEXTEX -->\n";
            break;
        case EMR_SAVEDC:
            *(d->outsvg) += "<!-- EMR_SAVEDC -->\n";
            break;
        case EMR_RESTOREDC:
            *(d->outsvg) += "<!-- EMR_RESTOREDC -->\n";
            break;
        case EMR_SETWORLDTRANSFORM:
            *(d->outsvg) += "<!-- EMR_SETWORLDTRANSFORM -->\n";
            break;
        case EMR_MODIFYWORLDTRANSFORM:
            *(d->outsvg) += "<!-- EMR_MODIFYWORLDTRANSFORM -->\n";
            break;
        case EMR_SELECTOBJECT:
        {
            PEMRSELECTOBJECT pEmr = (PEMRSELECTOBJECT) lpEMFR;
            unsigned int index = pEmr->ihObject;

            if (index >= ENHMETA_STOCK_OBJECT) {
                index -= ENHMETA_STOCK_OBJECT;
                switch (index) {
                    case NULL_BRUSH:
                        d->fill_set = false;
                        break;
                    case BLACK_BRUSH:
                    case DKGRAY_BRUSH:
                    case GRAY_BRUSH:
                    case LTGRAY_BRUSH:
                    case WHITE_BRUSH:
                    {
                        float val = 0;
                        switch (index) {
                            case BLACK_BRUSH:
                                val = 0.0 / 255.0;
                                break;
                            case DKGRAY_BRUSH:
                                val = 64.0 / 255.0;
                                break;
                            case GRAY_BRUSH:
                                val = 128.0 / 255.0;
                                break;
                            case LTGRAY_BRUSH:
                                val = 192.0 / 255.0;
                                break;
                            case WHITE_BRUSH:
                                val = 255.0 / 255.0;
                                break;
                        }
                        d->style.fill.value.color.set( val, val, val );

                        d->fill_set = true;
                        break;
                    }
                    case NULL_PEN:
                        d->stroke_set = false;
                        break;
                    case BLACK_PEN:
                    case WHITE_PEN:
                    {
                        float val = index == BLACK_PEN ? 0 : 1;
                        d->style.stroke_dasharray_set = 0;
                        d->style.stroke_width.value = 1.0;
                        d->style.stroke.value.color.set( val, val, val );

                        d->stroke_set = true;

                        break;
                    }
                }
            } else {
                if (index >= 0 && index < (unsigned int) d->n_obj) {
                    switch (d->emf_obj[index].type)
                    {
                        case EMR_CREATEPEN:
                            select_pen(d, index);
                            break;
                        case EMR_CREATEBRUSHINDIRECT:
                            select_brush(d, index);
                            break;
                        case EMR_EXTCREATEPEN:
                            select_extpen(d, index);
                            break;
                        case EMR_EXTCREATEFONTINDIRECTW:
                            select_font(d, index);
                            break;
                    }
                }
            }
            break;
        }
        case EMR_CREATEPEN:
        {
            PEMRCREATEPEN pEmr = (PEMRCREATEPEN) lpEMFR;
            int index = pEmr->ihPen;

            EMRCREATEPEN *pPen =
                (EMRCREATEPEN *) malloc( sizeof(EMREXTCREATEPEN) );
            pPen->lopn = pEmr->lopn;
            insert_object(d, index, EMR_CREATEPEN, (ENHMETARECORD *) pPen);

            break;
        }
        case EMR_CREATEBRUSHINDIRECT:
        {
            PEMRCREATEBRUSHINDIRECT pEmr = (PEMRCREATEBRUSHINDIRECT) lpEMFR;
            int index = pEmr->ihBrush;

            EMRCREATEBRUSHINDIRECT *pBrush =
                (EMRCREATEBRUSHINDIRECT *) malloc( sizeof(EMRCREATEBRUSHINDIRECT) );
            pBrush->lb = pEmr->lb;
            insert_object(d, index, EMR_CREATEBRUSHINDIRECT, (ENHMETARECORD *) pBrush);

            break;
        }
        case EMR_DELETEOBJECT:
            break;
        case EMR_ANGLEARC:
            *(d->outsvg) += "<!-- EMR_ANGLEARC -->\n";
            break;
        case EMR_ELLIPSE:
            *(d->outsvg) += "<!-- EMR_ELLIPSE -->\n";
            break;
        case EMR_RECTANGLE:
            *(d->outsvg) += "<!-- EMR_RECTANGLE -->\n";
            break;
        case EMR_ROUNDRECT:
            *(d->outsvg) += "<!-- EMR_ROUNDRECT -->\n";
            break;
        case EMR_ARC:
            *(d->outsvg) += "<!-- EMR_ARC -->\n";
            break;
        case EMR_CHORD:
            *(d->outsvg) += "<!-- EMR_CHORD -->\n";
            break;
        case EMR_PIE:
            *(d->outsvg) += "<!-- EMR_PIE -->\n";
            break;
        case EMR_SELECTPALETTE:
            *(d->outsvg) += "<!-- EMR_SELECTPALETTE -->\n";
            break;
        case EMR_CREATEPALETTE:
            *(d->outsvg) += "<!-- EMR_CREATEPALETTE -->\n";
            break;
        case EMR_SETPALETTEENTRIES:
            *(d->outsvg) += "<!-- EMR_SETPALETTEENTRIES -->\n";
            break;
        case EMR_RESIZEPALETTE:
            *(d->outsvg) += "<!-- EMR_RESIZEPALETTE -->\n";
            break;
        case EMR_REALIZEPALETTE:
            *(d->outsvg) += "<!-- EMR_REALIZEPALETTE -->\n";
            break;
        case EMR_EXTFLOODFILL:
            *(d->outsvg) += "<!-- EMR_EXTFLOODFILL -->\n";
            break;
        case EMR_LINETO:
        {
            PEMRLINETO pEmr = (PEMRLINETO) lpEMFR;

            if (d->path->empty())
                *(d->path) = "d=\"";

            tmp_path <<
                "\n\tL " <<
                pix_x_to_point( d, pEmr->ptl.x ) << " " <<
                pix_y_to_point( d, pEmr->ptl.y ) << " ";
            break;
        }
        case EMR_ARCTO:
            *(d->outsvg) += "<!-- EMR_ARCTO -->\n";
            break;
        case EMR_POLYDRAW:
            *(d->outsvg) += "<!-- EMR_POLYDRAW -->\n";
            break;
        case EMR_SETARCDIRECTION:
            *(d->outsvg) += "<!-- EMR_SETARCDIRECTION -->\n";
            break;
        case EMR_SETMITERLIMIT:
        {
            PEMRSETMITERLIMIT pEmr = (PEMRSETMITERLIMIT) lpEMFR;
            d->style.stroke_miterlimit.value = pix_size_to_point( d, pEmr->eMiterLimit );

            if (d->style.stroke_miterlimit.value < 1)
                d->style.stroke_miterlimit.value = 1.0;

            break;
        }
        case EMR_BEGINPATH:
        {
            tmp_path << "d=\"";
            *(d->path) = "";
            break;
        }
        case EMR_ENDPATH:
        {
            tmp_path << "\"";
            break;
        }
        case EMR_CLOSEFIGURE:
        {
            tmp_path << "\n\tz";
            break;
        }
        case EMR_FILLPATH:
        case EMR_STROKEANDFILLPATH:
        case EMR_STROKEPATH:
        {
            *(d->outsvg) += "    <path ";
            output_style(d, lpEMFR->iType);
            *(d->outsvg) += "\n\t";
            *(d->outsvg) += *(d->path);
            *(d->outsvg) += " /> \n";
            *(d->path) = "";
            break;
        }
        case EMR_FLATTENPATH:
            *(d->outsvg) += "<!-- EMR_FLATTENPATH -->\n";
            break;
        case EMR_WIDENPATH:
            *(d->outsvg) += "<!-- EMR_WIDENPATH -->\n";
            break;
        case EMR_SELECTCLIPPATH:
            *(d->outsvg) += "<!-- EMR_SELECTCLIPPATH -->\n";
            break;
        case EMR_ABORTPATH:
            *(d->outsvg) += "<!-- EMR_ABORTPATH -->\n";
            break;
        case EMR_GDICOMMENT:
            *(d->outsvg) += "<!-- EMR_GDICOMMENT -->\n";
            break;
        case EMR_FILLRGN:
            *(d->outsvg) += "<!-- EMR_FILLRGN -->\n";
            break;
        case EMR_FRAMERGN:
            *(d->outsvg) += "<!-- EMR_FRAMERGN -->\n";
            break;
        case EMR_INVERTRGN:
            *(d->outsvg) += "<!-- EMR_INVERTRGN -->\n";
            break;
        case EMR_PAINTRGN:
            *(d->outsvg) += "<!-- EMR_PAINTRGN -->\n";
            break;
        case EMR_EXTSELECTCLIPRGN:
            *(d->outsvg) += "<!-- EMR_EXTSELECTCLIPRGN -->\n";
            break;
        case EMR_BITBLT:
            *(d->outsvg) += "<!-- EMR_BITBLT -->\n";
            break;
        case EMR_STRETCHBLT:
            *(d->outsvg) += "<!-- EMR_STRETCHBLT -->\n";
            break;
        case EMR_MASKBLT:
            *(d->outsvg) += "<!-- EMR_MASKBLT -->\n";
            break;
        case EMR_PLGBLT:
            *(d->outsvg) += "<!-- EMR_PLGBLT -->\n";
            break;
        case EMR_SETDIBITSTODEVICE:
            *(d->outsvg) += "<!-- EMR_SETDIBITSTODEVICE -->\n";
            break;
        case EMR_STRETCHDIBITS:
            *(d->outsvg) += "<!-- EMR_STRETCHDIBITS -->\n";
            break;
        case EMR_EXTCREATEFONTINDIRECTW:
        {
            PEMREXTCREATEFONTINDIRECTW pEmr = (PEMREXTCREATEFONTINDIRECTW) lpEMFR;
            int index = pEmr->ihFont;

            EMREXTCREATEFONTINDIRECTW *pFont =
                (EMREXTCREATEFONTINDIRECTW *) malloc( sizeof(EMREXTCREATEFONTINDIRECTW) );
            pFont->elfw = pEmr->elfw;
            insert_object(d, index, EMR_EXTCREATEFONTINDIRECTW, (ENHMETARECORD *) pFont);

            break;
        }
        case EMR_EXTTEXTOUTA:
        {
            *(d->outsvg) += "<!-- EMR_EXTTEXTOUTA -->\n";
            break;
        }
        case EMR_EXTTEXTOUTW:
        {
            PEMREXTTEXTOUTW pEmr = (PEMREXTTEXTOUTW) lpEMFR;

            double x = pEmr->emrtext.ptlReference.x;
            double y = pEmr->emrtext.ptlReference.y;

            x = pix_x_to_point(d, x);
            y = pix_y_to_point(d, y);

            wchar_t *text = (wchar_t *) ((char *) pEmr + pEmr->emrtext.offString);
/*
            int i;
            for (i=0; i<pEmr->emrtext.nChars; i++) {
                if (text[i] < L' ' || text[i] > L'z')
                    text[i] = L'?';
            }
*/
            gchar *t =
                (gchar *) g_utf16_to_utf8( (gunichar2 *) text, pEmr->emrtext.nChars, NULL, NULL, NULL );

            if (t) {
                SVGOStringStream ts;

                gchar *escaped = g_markup_escape_text(t, -1);

                int j;
                for (j=0; j<strlen(escaped); j++) {
                    if (escaped[j] < 32 || escaped[j] > 127)
                        escaped[j] = '?';
                }

                float text_rgb[3];
                sp_color_get_rgb_floatv( &(d->style.fill.value.color), text_rgb );

                char tmp[128];
                snprintf(tmp, 127,
                         "fill:#%02x%02x%02x;",
                         SP_COLOR_F_TO_U(text_rgb[0]),
                         SP_COLOR_F_TO_U(text_rgb[1]),
                         SP_COLOR_F_TO_U(text_rgb[2]));

                bool i = (d->style.font_style.value == SP_CSS_FONT_STYLE_ITALIC);
                bool o = (d->style.font_style.value == SP_CSS_FONT_STYLE_OBLIQUE);
                bool b = (d->style.font_weight.value == SP_CSS_FONT_WEIGHT_BOLD) ||
                    (d->style.font_weight.value >= SP_CSS_FONT_WEIGHT_500 && d->style.font_weight.value <= SP_CSS_FONT_WEIGHT_900);

                ts << "    <text\n";
                ts << "        xml:space=\"preserve\"\n";
                ts << "        x=\"" << x << "\"\n";
                ts << "        y=\"" << y << "\"\n";
                ts << "        style=\""
                   << "font-size:" << d->style.font_size.computed << "px;"
                   << tmp
                   << "font-style:" << (i ? "italic" : "normal") << ";"
                   << "font-weight:" << (b ? "bold" : "normal") << ";"
//                   << "text-align:" << (b ? "start" : "center" : "end") << ";"
                   << "font-family:" << d->tstyle.font_family.value << ";"
                   << "\"\n";
                ts << "    >";
                ts << escaped;
                ts << "</text>\n";
                
                *(d->outsvg) += ts.str().c_str();
                
                g_free(escaped);
                g_free(t);
            }
            
            break;
        }
        case EMR_POLYBEZIER16:
        {
            PEMRPOLYBEZIER16 pEmr = (PEMRPOLYBEZIER16) lpEMFR;
            POINTS *apts = (POINTS *) pEmr->apts; // Bug in MinGW wingdi.h ?
            DWORD i,j;

            if (pEmr->cpts<4)
                break;

            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            tmp_str <<
                "\n\tM " <<
                pix_x_to_point( d, apts[0].x ) << " " <<
                pix_y_to_point( d, apts[0].y ) << " ";

            for (i=1; i<pEmr->cpts; ) {
                tmp_str << "\n\tC ";
                for (j=0; j<3 && i<pEmr->cpts; j++,i++) {
                    tmp_str <<
                        pix_x_to_point( d, apts[i].x ) << " " <<
                        pix_y_to_point( d, apts[i].y ) << " ";
                }
            }

            *(d->outsvg) += tmp_str.str().c_str();
            *(d->outsvg) += " \" /> \n";

            break;
        }
        case EMR_POLYGON16:
        {
            PEMRPOLYGON16 pEmr = (PEMRPOLYGON16) lpEMFR;
            POINTS *apts = (POINTS *) pEmr->apts; // Bug in MinGW wingdi.h ?
            unsigned int i;

            *(d->outsvg) += "<path ";
            output_style(d, EMR_STROKEANDFILLPATH);
            *(d->outsvg) += "\n\td=\"";

            // skip the first point?
            tmp_path << "\n\tM " <<
                pix_x_to_point( d, apts[1].x ) << " " <<
                pix_y_to_point( d, apts[1].y ) << " ";

            for (i=2; i<pEmr->cpts; i++) {
                tmp_path << "\n\tL " <<
                    pix_x_to_point( d, apts[i].x ) << " " <<
                    pix_y_to_point( d, apts[i].y ) << " ";
            }

            *(d->outsvg) += tmp_path.str().c_str();
            *(d->outsvg) += " z \" /> \n";

            break;
        }
        case EMR_POLYLINE16:
        {
            EMRPOLYLINE16 *pEmr = (EMRPOLYLINE16 *) lpEMFR;
            POINTS *apts = (POINTS *) pEmr->apts; // Bug in MinGW wingdi.h ?
            DWORD i;

            if (pEmr->cpts<2)
                break;

            *(d->outsvg) += "    <path ";
            output_style(d, EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            tmp_str <<
                "\n\tM " <<
                pix_x_to_point( d, apts[0].x ) << " " <<
                pix_y_to_point( d, apts[0].y ) << " ";

            for (i=1; i<pEmr->cpts; i++) {
                tmp_str <<
                    "\n\tL " <<
                    pix_x_to_point( d, apts[i].x ) << " " <<
                    pix_y_to_point( d, apts[i].y ) << " ";
            }

            *(d->outsvg) += tmp_str.str().c_str();
            *(d->outsvg) += " \" /> \n";

            break;
        }
        case EMR_POLYBEZIERTO16:
        {
            PEMRPOLYBEZIERTO16 pEmr = (PEMRPOLYBEZIERTO16) lpEMFR;
            POINTS *apts = (POINTS *) pEmr->apts; // Bug in MinGW wingdi.h ?
            DWORD i,j;

            if (d->path->empty())
                *(d->path) = "d=\"";

            for (i=0; i<pEmr->cpts;) {
                tmp_path << "\n\tC ";
                for (j=0; j<3 && i<pEmr->cpts; j++,i++) {
                    tmp_path <<
                        pix_x_to_point( d, apts[i].x ) << " " <<
                        pix_y_to_point( d, apts[i].y ) << " ";
                }
            }

            break;
        }
        case EMR_POLYLINETO16:
        {
            PEMRPOLYLINETO16 pEmr = (PEMRPOLYLINETO16) lpEMFR;
            POINTS *apts = (POINTS *) pEmr->apts; // Bug in MinGW wingdi.h ?
            DWORD i;

            if (d->path->empty())
                *(d->path) = "d=\"";

            for (i=0; i<pEmr->cpts;i++) {
                tmp_path <<
                    "\n\tL " <<
                    pix_x_to_point( d, apts[i].x ) << " " <<
                    pix_y_to_point( d, apts[i].y ) << " ";
            }

            break;
        }
        case EMR_POLYPOLYLINE16:
        case EMR_POLYPOLYGON16:
        {
            PEMRPOLYPOLYGON16 pEmr = (PEMRPOLYPOLYGON16) lpEMFR;
            unsigned int n, i, j;

            if (lpEMFR->iType == EMR_POLYPOLYGON16)
                *(d->outsvg) += "<!-- EMR_POLYPOLYGON16... -->\n";
            else
                *(d->outsvg) += "<!-- EMR_POLYPOLYLINE16... -->\n";

            *(d->outsvg) += "<path ";
            output_style(d, lpEMFR->iType==EMR_POLYPOLYGON16 ? EMR_STROKEANDFILLPATH : EMR_STROKEPATH);
            *(d->outsvg) += "\n\td=\"";

            POINTS *apts = (POINTS *) &pEmr->aPolyCounts[pEmr->nPolys];

            i = 0;
            for (n=0; n<pEmr->nPolys && i<pEmr->cpts; n++) {
                SVGOStringStream poly_path;

                poly_path << "\n\tM " <<
                    pix_x_to_point( d, apts[i].x ) << " " <<
                    pix_y_to_point( d, apts[i].y ) << " ";
                i++;

                for (j=1; j<pEmr->aPolyCounts[n] && i<pEmr->cpts; j++) {
                    poly_path << "\n\tL " <<
                        pix_x_to_point( d, apts[i].x ) << " " <<
                        pix_y_to_point( d, apts[i].y ) << " ";
                    i++;
                }

                *(d->outsvg) += poly_path.str().c_str();
                if (lpEMFR->iType == EMR_POLYPOLYGON16)
                    *(d->outsvg) += " z";
                *(d->outsvg) += " \n";
            }

            *(d->outsvg) += " \" /> \n";
            break;
        }
        case EMR_POLYDRAW16:
            *(d->outsvg) += "<!-- EMR_POLYDRAW16 -->\n";
            break;
        case EMR_CREATEMONOBRUSH:
            *(d->outsvg) += "<!-- EMR_CREATEMONOBRUSH -->\n";
            break;
        case EMR_CREATEDIBPATTERNBRUSHPT:
            *(d->outsvg) += "<!-- EMR_CREATEDIBPATTERNBRUSHPT -->\n";
            break;
        case EMR_EXTCREATEPEN:
        {
            PEMREXTCREATEPEN pEmr = (PEMREXTCREATEPEN) lpEMFR;
            int index = pEmr->ihPen;

            EMREXTCREATEPEN *pPen =
                (EMREXTCREATEPEN *) malloc( sizeof(EMREXTCREATEPEN) +
                                            sizeof(DWORD) * pEmr->elp.elpNumEntries );
            pPen->ihPen = pEmr->ihPen;
            pPen->offBmi = pEmr->offBmi;
            pPen->cbBmi = pEmr->cbBmi;
            pPen->offBits = pEmr->offBits;
            pPen->cbBits = pEmr->cbBits;
            pPen->elp = pEmr->elp;
            for (unsigned int i=0; i<pEmr->elp.elpNumEntries; i++) {
                pPen->elp.elpStyleEntry[i] = pEmr->elp.elpStyleEntry[i];
            }
            insert_object(d, index, EMR_EXTCREATEPEN, (ENHMETARECORD *) pPen);

            break;
        }
        case EMR_POLYTEXTOUTA:
            *(d->outsvg) += "<!-- EMR_POLYTEXTOUTA -->\n";
            break;
        case EMR_POLYTEXTOUTW:
            *(d->outsvg) += "<!-- EMR_POLYTEXTOUTW -->\n";
            break;
        case EMR_SETICMMODE:
            *(d->outsvg) += "<!-- EMR_SETICMMODE -->\n";
            break;
        case EMR_CREATECOLORSPACE:
            *(d->outsvg) += "<!-- EMR_CREATECOLORSPACE -->\n";
            break;
        case EMR_SETCOLORSPACE:
            *(d->outsvg) += "<!-- EMR_SETCOLORSPACE -->\n";
            break;
        case EMR_DELETECOLORSPACE:
            *(d->outsvg) += "<!-- EMR_DELETECOLORSPACE -->\n";
            break;
        case EMR_GLSRECORD:
            *(d->outsvg) += "<!-- EMR_GLSRECORD -->\n";
            break;
        case EMR_GLSBOUNDEDRECORD:
            *(d->outsvg) += "<!-- EMR_GLSBOUNDEDRECORD -->\n";
            break;
        case EMR_PIXELFORMAT:
            *(d->outsvg) += "<!-- EMR_PIXELFORMAT -->\n";
            break;
    }
    
    *(d->outsvg) += tmp_outsvg.str().c_str();
    *(d->path) += tmp_path.str().c_str();

    return 1;
}


// Aldus Placeable Header ===================================================
// Since we are a 32bit app, we have to be sure this structure compiles to
// be identical to a 16 bit app's version. To do this, we use the #pragma
// to adjust packing, we use a WORD for the hmf handle, and a SMALL_RECT
// for the bbox rectangle.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	DWORD		dwKey;
	WORD		hmf;
	SMALL_RECT	bbox;
	WORD		wInch;
	DWORD		dwReserved;
	WORD		wCheckSum;
} APMHEADER, *PAPMHEADER;
#pragma pack( pop )


SPDocument *
EmfWin32::open( Inkscape::Extension::Input *mod, const gchar *uri )
{
    EMF_CALLBACK_DATA d = {0};

    gsize bytesRead = 0;
    gsize bytesWritten = 0;
    GError* error = NULL;
    gchar *local_fn =
        g_filename_from_utf8( uri, -1,  &bytesRead,  &bytesWritten, &error );

    if (local_fn == NULL) {
        return NULL;
    }

    d.outsvg = new Glib::ustring("");
    d.path = new Glib::ustring("");

    CHAR *ansi_uri = (CHAR *) local_fn;
    gunichar2 *unicode_fn = g_utf8_to_utf16( local_fn, -1, NULL, NULL, NULL );
    WCHAR *unicode_uri = (WCHAR *) unicode_fn;

    // Try open as Enhanced Metafile
    HENHMETAFILE hemf;
    if (PrintWin32::is_os_wide())
        hemf = GetEnhMetaFileW(unicode_uri);
    else
        hemf = GetEnhMetaFileA(ansi_uri);

    if (!hemf) {
        // Try open as Windows Metafile
        HMETAFILE hmf;
        if (PrintWin32::is_os_wide())
            hmf = GetMetaFileW(unicode_uri);
        else
            hmf = GetMetaFileA(ansi_uri);

	METAFILEPICT mp;
	HDC hDC;

        if (!hmf) {
            if (PrintWin32::is_os_wide()) {
                WCHAR szTemp[MAX_PATH];

                DWORD dw = GetShortPathNameW( unicode_uri, szTemp, MAX_PATH );
                if (dw) {
                    hmf = GetMetaFileW( szTemp );
                }
            } else {
                CHAR szTemp[MAX_PATH];

                DWORD dw = GetShortPathNameA( ansi_uri, szTemp, MAX_PATH );
                if (dw) {
                    hmf = GetMetaFileA( szTemp );
                }
            }
        }

        if (hmf) {
            DWORD nSize = GetMetaFileBitsEx( hmf, 0, NULL );
            if (nSize) {
                BYTE *lpvData = new BYTE[nSize];
                if (lpvData) {
                    DWORD dw = GetMetaFileBitsEx( hmf, nSize, lpvData );
                    if (dw) {
                        // Fill out a METAFILEPICT structure
                        mp.mm = MM_ANISOTROPIC;
                        mp.xExt = 1000;
                        mp.yExt = 1000;
                        mp.hMF = NULL;
                        // Get a reference DC
                        hDC = GetDC( NULL );
                        // Make an enhanced metafile from the windows metafile
                        hemf = SetWinMetaFileBits( nSize, lpvData, hDC, &mp );
                        // Clean up
                        ReleaseDC( NULL, hDC );
                    }
                    delete[] lpvData;
                }
                DeleteMetaFile( hmf );
            }
        } else {
            // Try open as Aldus Placeable Metafile
            HANDLE hFile;
            if (PrintWin32::is_os_wide())
                hFile = CreateFileW( unicode_uri, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            else
                hFile = CreateFileA( ansi_uri, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD nSize = GetFileSize( hFile, NULL );
                if (nSize) {
                    BYTE *lpvData = new BYTE[nSize];
                    if (lpvData) {
                        DWORD dw = ReadFile( hFile, lpvData, nSize, &nSize, NULL );
                        if (dw) {
                            if ( ((PAPMHEADER)lpvData)->dwKey == 0x9ac6cdd7l ) {
                                // Fill out a METAFILEPICT structure
                                mp.mm = MM_ANISOTROPIC;
                                mp.xExt = ((PAPMHEADER)lpvData)->bbox.Right - ((PAPMHEADER)lpvData)->bbox.Left;
                                mp.xExt = ( mp.xExt * 2540l ) / (DWORD)(((PAPMHEADER)lpvData)->wInch);
                                mp.yExt = ((PAPMHEADER)lpvData)->bbox.Bottom - ((PAPMHEADER)lpvData)->bbox.Top;
                                mp.yExt = ( mp.yExt * 2540l ) / (DWORD)(((PAPMHEADER)lpvData)->wInch);
                                mp.hMF = NULL;
                                // Get a reference DC
                                hDC = GetDC( NULL );
                                // Create an enhanced metafile from the bits
                                hemf = SetWinMetaFileBits( nSize, lpvData+sizeof(APMHEADER), hDC, &mp );
                                // Clean up
                                ReleaseDC( NULL, hDC );
                            }
                        }
                        delete[] lpvData;
                    }
                }
                CloseHandle( hFile );
            }
        }
    }

    if (!hemf || !d.outsvg || !d.path) {
        if (d.outsvg)
            delete d.outsvg;
        if (d.path)
            delete d.path;
        if  (local_fn)
            g_free(local_fn);
        if  (unicode_fn)
            g_free(unicode_fn);
        return NULL;
    }

    EnumEnhMetaFile(NULL, hemf, myEnhMetaFileProc, (LPVOID) &d, NULL);
    DeleteEnhMetaFile(hemf);

//    std::cout << "SVG Output: " << std::endl << *(d.outsvg) << std::endl;

    SPDocument *doc = sp_document_new_from_mem(d.outsvg->c_str(), d.outsvg->length(), TRUE);

    delete d.outsvg;
    delete d.path;

    if (d.emf_obj) {
        int i;
        for (i=0; i<d.n_obj; i++)
            delete_object(&d, i);
        delete[] d.emf_obj;
    }
    
    if (d.style.stroke_dash.dash)
        delete[] d.style.stroke_dash.dash;

    if  (local_fn)
        g_free(local_fn);
    if  (unicode_fn)
        g_free(unicode_fn);

    return doc;
}


void
EmfWin32::init (void)
{
    Inkscape::Extension::Extension * ext;

    /* EMF in */
    ext = Inkscape::Extension::build_from_mem(
        "<inkscape-extension>\n"
            "<name>" N_("EMF Input") "</name>\n"
            "<id>org.inkscape.input.emf.win32</id>\n"
            "<input>\n"
                "<extension>.emf</extension>\n"
                "<mimetype>image/x-emf</mimetype>\n"
                "<filetypename>" N_("Enhanced Metafiles (*.emf)") "</filetypename>\n"
                "<filetypetooltip>" N_("Enhanced Metafiles") "</filetypetooltip>\n"
                "<output_extension>org.inkscape.output.emf.win32</output_extension>\n"
            "</input>\n"
        "</inkscape-extension>", new EmfWin32());

    /* WMF in */
    ext = Inkscape::Extension::build_from_mem(
        "<inkscape-extension>\n"
            "<name>" N_("WMF Input") "</name>\n"
            "<id>org.inkscape.input.wmf.win32</id>\n"
            "<input>\n"
                "<extension>.wmf</extension>\n"
                "<mimetype>image/x-wmf</mimetype>\n"
                "<filetypename>" N_("Windows Metafiles (*.wmf)") "</filetypename>\n"
                "<filetypetooltip>" N_("Windows Metafiles") "</filetypetooltip>\n"
                "<output_extension>org.inkscape.output.emf.win32</output_extension>\n"
            "</input>\n"
        "</inkscape-extension>", new EmfWin32());

    /* EMF out */
    ext = Inkscape::Extension::build_from_mem(
        "<inkscape-extension>\n"
            "<name>" N_("EMF Output") "</name>\n"
            "<id>org.inkscape.output.emf.win32</id>\n"
            "<param name=\"textToPath\" gui-text=\"" N_("Convert texts to paths") "\" type=\"boolean\">true</param>\n"
            "<output>\n"
                "<extension>.emf</extension>\n"
                "<mimetype>image/x-emf</mimetype>\n"
                "<filetypename>" N_("Enhanced Metafile (*.emf)") "</filetypename>\n"
                "<filetypetooltip>" N_("Enhanced Metafile") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>", new EmfWin32());

    return;
}


} } }  /* namespace Inkscape, Extension, Implementation */


#endif /* WIN32 */


/*
  Local Variables:
  mode:cpp
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
