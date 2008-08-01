/*
 * A simple utility for exporting Inkscape svg Shapes as JavaFX paths.
 *
 *  For information on the JavaFX file format, see:
 *      https://openjfx.dev.java.net/
 *
 * Authors:
 *   Bob Jamison <ishmal@inkscape.org>
 *
 * Copyright (C) 2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "javafx-out.h"
#include <inkscape.h>
#include <inkscape_version.h>
#include <sp-path.h>
#include <style.h>
#include <display/curve.h>
#include <libnr/n-art-bpath.h>
#include <extension/system.h>
#include <2geom/pathvector.h>
#include <2geom/rect.h>
#include <2geom/bezier-curve.h>
#include <2geom/hvlinesegment.h>
#include "helper/geom.h"
#include <io/sys.h>

#include <string>
#include <stdio.h>
#include <stdarg.h>


namespace Inkscape
{
namespace Extension
{
namespace Internal
{




//########################################################################
//# OUTPUT FORMATTING
//########################################################################


/**
 * We want to control floating output format
 */
static JavaFXOutput::String dstr(double d)
{
    char dbuf[G_ASCII_DTOSTR_BUF_SIZE+1];
    g_ascii_formatd(dbuf, G_ASCII_DTOSTR_BUF_SIZE,
                  "%.8f", (gdouble)d);
    JavaFXOutput::String s = dbuf;
    return s;
}




/**
 *  Output data to the buffer, printf()-style
 */
void JavaFXOutput::out(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gchar *output = g_strdup_vprintf(fmt, args);
    va_end(args);
    outbuf.append(output);
    g_free(output);
}


/**
 * Output the file header
 */
bool JavaFXOutput::doHeader(const String &name)
{
    time_t tim = time(NULL);
    out("/*###################################################################\n");
    out("### This JavaFX document was generated by Inkscape\n");
    out("### http://www.inkscape.org\n");
    out("### Created: %s", ctime(&tim));
    out("### Version: %s\n", INKSCAPE_VERSION);
    out("#####################################################################\n");
    out("### NOTES:\n");
    out("### ============\n");
    out("### JavaFX information can be found at\n");
    out("### hhttps://openjfx.dev.java.net\n");
    out("###\n");
    out("### If you have any problems with this output, please see the\n");
    out("### Inkscape project at http://www.inkscape.org, or visit\n");
    out("### the #inkscape channel on irc.freenode.net . \n");
    out("###\n");
    out("###################################################################*/\n");
    out("\n\n");
    out("/*###################################################################\n");
    out("##   Exports in this file\n");
    out("##==========================\n");
    out("##    Shapes   : %d\n", nrShapes);
    out("##    Segments : %d\n", nrSegments);
    out("##    Nodes    : %d\n", nrNodes);
    out("###################################################################*/\n");
    out("\n\n");
    out("import javafx.ui.UIElement;\n");
    out("import javafx.ui.*;\n");
    out("import javafx.ui.canvas.*;\n");
    out("\n");
    out("import java.lang.System;\n");
    out("\n\n");
    out("public class %s extends CompositeNode {\n", name.c_str());
    out("}\n");
    out("\n\n");
    out("function %s.composeNode() =\n", name.c_str());
    out("Group\n");
    out("    {\n");
    out("    content:\n");
    out("        [\n");
    return true;
}



/**
 *  Output the file footer
 */
bool JavaFXOutput::doTail(const String &name)
{
    out("        ] // content\n");
    out("    }; // Group\n");
    out("// end function %s.composeNode()\n", name.c_str());
    out("\n\n\n\n");
    out("Frame {\n");
    out("    title: \"Test\"\n");
    out("    width: 500\n");
    out("    height: 500\n");
    out("    onClose: function()\n");
    out("        {\n");
    out("        return System.exit( 0 );\n");
    out("        }\n");
    out("    visible: true\n");
    out("    content: Canvas {\n");
    out("        content: tux{}\n");
    out("    }\n");
    out("}\n");
    out("/*###################################################################\n");
    out("### E N D   C L A S S    %s\n", name.c_str());
    out("###################################################################*/\n");
    out("\n\n");
    return true;
}


/**
 *  Output the curve data to buffer
 */
bool JavaFXOutput::doCurve(SPItem *item, const String &id)
{
    using Geom::X;
    using Geom::Y;

    Geom::Matrix tf = sp_item_i2d_affine(item);

    //### Get the Shape
    if (!SP_IS_SHAPE(item))//Bulia's suggestion.  Allow all shapes
        return true;

    SPShape *shape = SP_SHAPE(item);
    SPCurve *curve = shape->curve;
    if (curve->is_empty())
        return true;

    nrShapes++;

    out("        /*###################################################\n");
    out("        ### PATH:  %s\n", id.c_str());
    out("        ###################################################*/\n");
    out("        Path \n");
    out("        {\n");
    out("        id: \"%s\"\n", id.c_str());

    /**
     * Get the fill and stroke of the shape
     */
    SPStyle *style = SP_OBJECT_STYLE(shape);
    if (style)
        {
        /**
         * Fill
         */
        if (style->fill.isColor())
            {
            // see color.h for how to parse SPColor
            gint alpha = 0xffffffff;
            guint32 rgba = style->fill.value.color.toRGBA32(alpha);
            unsigned int r = SP_RGBA32_R_U(rgba);
            unsigned int g = SP_RGBA32_G_U(rgba);
            unsigned int b = SP_RGBA32_B_U(rgba);
            unsigned int a = SP_RGBA32_A_U(rgba);
            out("        fill: rgba(0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
                               r, g, b, a);
            }
        /**
         * Stroke
         */
        /**
         * TODO:  stroke code here
         */
        }


    // convert the path to only lineto's and cubic curveto's:
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers( curve->get_pathvector() * tf );

    //Count the NR_CURVETOs/LINETOs (including closing line segment)
    guint segmentCount = 0;
    for(Geom::PathVector::const_iterator it = pathv.begin(); it != pathv.end(); ++it) {
        segmentCount += (*it).size();
        if (it->closed())
            segmentCount += 1;
    }

    out("        d:\n");
    out("            [\n");

    unsigned int segmentNr = 0;

    nrSegments += segmentCount;

    /**
     * For all Subpaths in the <path>
     */	     
    for (Geom::PathVector::const_iterator pit = pathv.begin(); pit != pathv.end(); ++pit)
        {
        Geom::Point p = pit->front().initialPoint() * tf;
        out("            MoveTo {\n");
        out("                x: %s\n", dstr(p[X]).c_str());
        out("                y: %s\n", dstr(p[Y]).c_str());
        out("                absolute: true\n");
        out("                },\n");
        
        /**
         * For all segments in the subpath
         */		         
        for (Geom::Path::const_iterator cit = pit->begin(); cit != pit->end_closed(); ++cit)
            {
            //### LINE
            if( dynamic_cast<Geom::LineSegment const *> (&*cit) ||
                dynamic_cast<Geom::HLineSegment const *>(&*cit) ||
                dynamic_cast<Geom::VLineSegment const *>(&*cit) )
                {
                Geom::Point p = cit->initialPoint() * tf;
                out("            LineTo {\n");
                out("                x: %s\n", dstr(p[X]).c_str());
                out("                y: %s\n", dstr(p[Y]).c_str());
                out("                absolute: true\n");
                out("                },\n");
                nrNodes++;
                }
            //### BEZIER
            else if(Geom::CubicBezier const *cubic = dynamic_cast<Geom::CubicBezier const*>(&*cit))
                {
                std::vector<Geom::Point> points = cubic->points();
                Geom::Point p0 = points[1] * tf;
                Geom::Point p1 = points[2] * tf;
                Geom::Point p2 = points[3] * tf;
                out("            CurveTo {\n");
                out("                x1: %s\n", dstr(p0[X]).c_str());
                out("                y1: %s\n", dstr(p0[Y]).c_str());
                out("                x2: %s\n", dstr(p1[X]).c_str());
                out("                y2: %s\n", dstr(p1[Y]).c_str());
                out("                x3: %s\n", dstr(p2[X]).c_str());
                out("                y3: %s\n", dstr(p2[Y]).c_str());
                out("                absolute: true\n");
                out("                },\n");
                nrNodes++;
                }
            else
                {
                g_error ("logical error, because pathv_to_linear_and_cubic_beziers was used");
                }
            segmentNr++;
            }
        if (pit->closed())
            {
            out("            ClosePath {},\n");
            }
        }

    out("            ] // d\n");
    out("        }, // Path\n");

                 
    out("        /*###################################################\n");
    out("        ### end path %s\n", id.c_str());
    out("        ###################################################*/\n\n\n\n");

    return true;
}


/**
 *  Output the curve data to buffer
 */
bool JavaFXOutput::doCurvesRecursive(SPDocument *doc, Inkscape::XML::Node *node)
{
    /**
     * If the object is an Item, try processing it
     */	     
    char *str  = (char *) node->attribute("id");
    SPObject *reprobj = doc->getObjectByRepr(node);
    if (SP_IS_ITEM(reprobj) && str)
        {
        SPItem *item = SP_ITEM(reprobj);
        String id = str;
        if (!doCurve(item, id))
            return false;
        }

    /**
     * Descend into children
     */	     
    for (Inkscape::XML::Node *child = node->firstChild() ; child ;
              child = child->next())
        {
		if (!doCurvesRecursive(doc, child))
		    return false;
		}

    return true;
}


/**
 *  Output the curve data to buffer
 */
bool JavaFXOutput::doCurves(SPDocument *doc)
{

    double bignum = 1000000.0;
    minx  =  bignum;
    maxx  = -bignum;
    miny  =  bignum;
    maxy  = -bignum;

    if (!doCurvesRecursive(doc, doc->rroot))
        return false;

    return true;

}




//########################################################################
//# M A I N    O U T P U T
//########################################################################



/**
 *  Set values back to initial state
 */
void JavaFXOutput::reset()
{
    nrNodes    = 0;
    nrSegments = 0;
    nrShapes   = 0;
    outbuf.clear();
}



/**
 * Saves the <paths> of an Inkscape SVG file as JavaFX spline definitions
 */
bool JavaFXOutput::saveDocument(SPDocument *doc, gchar const *uri)
{
    reset();


    String name = Glib::path_get_basename(uri);
    int pos = name.find('.');
    if (pos > 0)
        name = name.substr(0, pos);


    //###### SAVE IN POV FORMAT TO BUFFER
    //# Lets do the curves first, to get the stats
    
    if (!doCurves(doc))
        return false;
    String curveBuf = outbuf;
    outbuf.clear();

    if (!doHeader(name))
        return false;
    
    outbuf.append(curveBuf);
    
    if (!doTail(name))
        return false;




    //###### WRITE TO FILE
    FILE *f = Inkscape::IO::fopen_utf8name(uri, "w");
    if (!f)
        {
        g_warning("Could open JavaFX file '%s' for writing", uri);
        return false;
        }

    for (String::iterator iter = outbuf.begin() ; iter!=outbuf.end(); iter++)
        {
        fputc(*iter, f);
        }
        
    fclose(f);
    
    return true;
}




//########################################################################
//# EXTENSION API
//########################################################################



#include "clear-n_.h"



/**
 * API call to save document
*/
void
JavaFXOutput::save(Inkscape::Extension::Output */*mod*/,
                        SPDocument *doc, gchar const *uri)
{
    if (!saveDocument(doc, uri))
        {
        g_warning("Could not save JavaFX file '%s'", uri);
        }
}



/**
 * Make sure that we are in the database
 */
bool JavaFXOutput::check (Inkscape::Extension::Extension */*module*/)
{
    /* We don't need a Key
    if (NULL == Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_JFX))
        return FALSE;
    */

    return true;
}



/**
 * This is the definition of JavaFX output.  This function just
 * calls the extension system with the memory allocated XML that
 * describes the data.
*/
void
JavaFXOutput::init()
{
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("JavaFX Output") "</name>\n"
            "<id>org.inkscape.output.jfx</id>\n"
            "<output>\n"
                "<extension>.fx</extension>\n"
                "<mimetype>text/x-javafx-script</mimetype>\n"
                "<filetypename>" N_("JavaFX (*.fx)") "</filetypename>\n"
                "<filetypetooltip>" N_("JavaFX Raytracer File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        new JavaFXOutput());
}





}  // namespace Internal
}  // namespace Extension
}  // namespace Inkscape


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
