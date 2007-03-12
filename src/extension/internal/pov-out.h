/*
 * A simple utility for exporting Inkscape svg Shapes as PovRay bezier
 * prisms.  Note that this is output-only, and would thus seem to be
 * better placed as an 'export' rather than 'output'.  However, Export
 * handles all or partial documents, while this outputs ALL shapes in
 * the current SVG document.
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef EXTENSION_INTERNAL_POV_OUT_H
#define EXTENSION_INTERNAL_POV_OUT_H

#include <glib.h>
#include "extension/implementation/implementation.h"

namespace Inkscape
{
namespace Extension
{
namespace Internal
{



/**
 * Output bezier splines in POVRay format.
 * 
 * For information, @see:  
 * http://www.povray.org 
 */ 
class PovOutput : public Inkscape::Extension::Implementation::Implementation
{


/**
 * Our internal String definition
 */ 
typedef Glib::ustring String;


public:

    /**
     * Check whether we can actually output using this module
     */
	bool check (Inkscape::Extension::Extension * module);

    /**
     * API call to perform the output to a file
     */
	void save (Inkscape::Extension::Output *mod,
	           SPDocument *doc, const gchar *uri);

    /**
     * Inkscape runtime startup call.
     */
	static void init(void);
	
    /**
     * Reset variables to initial state
     */
	void reset();
	
private:

	/**
	 * Format text to our output buffer
	 */     	
	void out(char *fmt, ...);

    /**
     * Output a 2d vector
     */
    void vec2(double a, double b);

    /**
     * Output a 3d vector
     */
    void vec3(double a, double b, double c);

    /**
     * Output a 4d vector
     */
    void vec4(double a, double b, double c, double d);

    /**
     * Output an rgbf color vector
     */
    void rgbf(double r, double g, double b, double f);

    /**
     * Output one bezier's start, start-control, 
     *      end-control, and end nodes
     */
    void segment(int segNr, double a0, double a1,
                            double b0, double b1,
                            double c0, double c1,
                            double d0, double d1);


    /**
     * Output the file header
     */
    void doHeader();

    /**
     * Output the file footer
     */
    void doTail();

    /**
     * Output the SVG document's curve data as POV curves
     */
    void doCurves(SPDocument *doc);

    /**
     * Actual method to save document
     */
	void saveDocument(SPDocument *doc, const gchar *uri);


    /**
     * used for saving information about shapes
     */
    class PovShapeInfo
    {
    public:
        PovShapeInfo()
            {}
        PovShapeInfo(const PovShapeInfo &other)
            { assign(other); }
        PovShapeInfo operator=(const PovShapeInfo &other)
            { assign(other); return *this; }
        virtual ~PovShapeInfo()
            {}
        String id;
        String color;

    private:
        void assign(const PovShapeInfo &other)
            {
            id    = other.id;
            color = other.color;
            }
    };

    //A list for saving information about the shapes
    std::vector<PovShapeInfo> povShapes;
    
    //For formatted output
	String outbuf;
    char fmtbuf[2048];

    //For statistics
    int nrNodes;
    int nrSegments;
    int nrShapes;

};




}  // namespace Internal
}  // namespace Extension
}  // namespace Inkscape



#endif /* EXTENSION_INTERNAL_POV_OUT_H */

