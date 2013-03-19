/** @file
 * @brief Enhanced Metafile Input/Output
 */
/* Authors:
 *   Ulf Erikson <ulferikson@users.sf.net>
 *
 * Copyright (C) 2006-2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_EXTENSION_INTERNAL_EMF_H
#define SEEN_EXTENSION_INTERNAL_EMF_H

#define PNG_SKIP_SETJMP_CHECK // else any further png.h include blows up in the compiler
#include <png.h>
#include "extension/implementation/implementation.h"
#include "style.h"
#include "uemf.h"
#include "text_reassemble.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

#define DIRTY_NONE   0x00
#define DIRTY_TEXT   0x01
#define DIRTY_FILL   0x02
#define DIRTY_STROKE 0x04

typedef struct {
    int type;
    int level;
    char *lpEMFR;
} EMF_OBJECT, *PEMF_OBJECT;

typedef struct {
    int size;         // number of slots allocated in strings
    int count;        // number of slots used in strings
    char **strings;   // place to store strings
} EMF_STRINGS, *PEMF_STRINGS;

typedef struct emf_device_context {
    struct SPStyle  style;
    char           *font_name;
    bool            stroke_set;
    int             stroke_mode;  // enumeration from drawmode, not used if fill_set is not True
    int             stroke_idx;   // used with DRAW_PATTERN and DRAW_IMAGE to return the appropriate fill
    int             stroke_recidx;// record used to regenerate hatch when it needs to be redone due to bkmode, textmode, etc. change
    bool            fill_set;
    int             fill_mode;    // enumeration from drawmode, not used if fill_set is not True
    int             fill_idx;     // used with DRAW_PATTERN and DRAW_IMAGE to return the appropriate fill
    int             fill_recidx;  // record used to regenerate hatch when it needs to be redone due to bkmode, textmode, etc. change
    int             dirty;        // holds the dirty bits for text, stroke, fill
    U_SIZEL         sizeWnd;
    U_SIZEL         sizeView;
    U_POINTL        winorg;
    U_POINTL        vieworg;
    double          ScaleInX, ScaleInY;
    double          ScaleOutX, ScaleOutY;
    uint16_t        bkMode;
    U_COLORREF      bkColor;
    U_COLORREF      textColor;
    uint32_t        textAlign;
    U_XFORM         worldTransform;
    U_POINTL        cur;
} EMF_DEVICE_CONTEXT, *PEMF_DEVICE_CONTEXT;

#define EMF_MAX_DC 128

/*
  both emf-inout.h and wmf-inout.h are included by init.cpp, so whichever one goes in first defines these ommon types
*/
#ifndef SEEN_EXTENSION_INTERNAL_METAFILECOMMON_
#define SEEN_EXTENSION_INTERNAL_METAFILECOMMON_
/* A coloured pixel. */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t opacity;
} pixel_t;

/* A picture. */
    
typedef struct  {
    pixel_t *pixels;
    size_t width;
    size_t height;
} bitmap_t;
    
/* structure to store PNG image bytes */
typedef struct {
      char *buffer;
      size_t size;
} MEMPNG, *PMEMPNG;
#endif

typedef struct {
    Glib::ustring *outsvg;
    Glib::ustring *path;
    Glib::ustring *outdef;
    Glib::ustring *defs;

    EMF_DEVICE_CONTEXT dc[EMF_MAX_DC+1]; // FIXME: This should be dynamic..
    int level;
    
    double E2IdirY;                     // EMF Y direction relative to Inkscape Y direction.  Will be negative for MM_LOMETRIC etc.
    double D2PscaleX,D2PscaleY;         // EMF device to Inkscape Page scale.
    float  MM100InX, MM100InY;          // size of the drawing in hundredths of a millimeter
    float  PixelsInX, PixelsInY;        // size of the drawing, in EMF device pixels
    float  PixelsOutX, PixelsOutY;      // size of the drawing, in Inkscape pixels
    double ulCornerInX,ulCornerInY;     // Upper left corner, from header rclBounds, in logical units
    double ulCornerOutX,ulCornerOutY;   // Upper left corner, in Inkscape pixels
    uint32_t mask;                      // Draw properties
    int arcdir;                         //U_AD_COUNTERCLOCKWISE 1 or U_AD_CLOCKWISE 2
    
    uint32_t dwRop2;                    // Binary raster operation, 0 if none (use brush/pen unmolested)
    uint32_t dwRop3;                    // Ternary raster operation, 0 if none (use brush/pen unmolested)

    float MMX;
    float MMY;

    unsigned int id;
    unsigned int drawtype;  // one of 0 or U_EMR_FILLPATH, U_EMR_STROKEPATH, U_EMR_STROKEANDFILLPATH
    char *pDesc;
                              // both of these end up in <defs> under the names shown here.  These structures allow duplicates to be avoided.
    EMF_STRINGS hatches;      // hold pattern names, all like EMFhatch#_$$$$$$ where # is the EMF hatch code and $$$$$$ is the color
    EMF_STRINGS images;       // hold images, all like Image#, where # is the slot the image lives.
    TR_INFO    *tri;          // Text Reassembly data structure


    int n_obj;
    PEMF_OBJECT emf_obj;
} EMF_CALLBACK_DATA, *PEMF_CALLBACK_DATA;

class Emf : Inkscape::Extension::Implementation::Implementation { //This is a derived class

public:
    Emf(); // Empty constructor

    virtual ~Emf();//Destructor

    bool check(Inkscape::Extension::Extension *module); //Can this module load (always yes for now)

    void save(Inkscape::Extension::Output *mod, // Save the given document to the given filename
              SPDocument *doc,
              gchar const *filename);

    virtual SPDocument *open( Inkscape::Extension::Input *mod,
                                const gchar *uri );

    static void init(void);//Initialize the class

private:
protected:
   static pixel_t    *pixel_at (bitmap_t * bitmap, int x, int y);
   static void        my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length);
   static void        toPNG(PMEMPNG accum, int width, int height, const char *px);
   static uint32_t    sethexcolor(U_COLORREF color);
   static void        print_document_to_file(SPDocument *doc, const gchar *filename);
   static double      current_scale(PEMF_CALLBACK_DATA d);
   static std::string current_matrix(PEMF_CALLBACK_DATA d, double x, double y, int useoffset);
   static double      current_rotation(PEMF_CALLBACK_DATA d);
   static void        enlarge_hatches(PEMF_CALLBACK_DATA d);
   static int         in_hatches(PEMF_CALLBACK_DATA d, char *test);
   static uint32_t    add_hatch(PEMF_CALLBACK_DATA d, uint32_t hatchType, U_COLORREF hatchColor);
   static void        enlarge_images(PEMF_CALLBACK_DATA d);
   static int         in_images(PEMF_CALLBACK_DATA d, char *test);
   static uint32_t    add_image(PEMF_CALLBACK_DATA d,  void *pEmr, uint32_t cbBits, uint32_t cbBmi, 
                         uint32_t iUsage, uint32_t offBits, uint32_t offBmi);
   static void        output_style(PEMF_CALLBACK_DATA d, int iType);
   static double      _pix_x_to_point(PEMF_CALLBACK_DATA d, double px);
   static double      _pix_y_to_point(PEMF_CALLBACK_DATA d, double py);
   static double      pix_to_x_point(PEMF_CALLBACK_DATA d, double px, double py);
   static double      pix_to_y_point(PEMF_CALLBACK_DATA d, double px, double py);
   static double      pix_to_abs_size(PEMF_CALLBACK_DATA d, double px);
   static std::string pix_to_xy(PEMF_CALLBACK_DATA d, double x, double y);
   static void        select_pen(PEMF_CALLBACK_DATA d, int index);
   static void        select_extpen(PEMF_CALLBACK_DATA d, int index);
   static void        select_brush(PEMF_CALLBACK_DATA d, int index);
   static void        select_font(PEMF_CALLBACK_DATA d, int index);
   static void        delete_object(PEMF_CALLBACK_DATA d, int index);
   static void        insert_object(PEMF_CALLBACK_DATA d, int index, int type, PU_ENHMETARECORD pObj);
   static int         AI_hack(PU_EMRHEADER pEmr);
   static uint32_t   *unknown_chars(size_t count);
   static void        common_image_extraction(PEMF_CALLBACK_DATA d, void *pEmr,
                         double dx, double dy, double dw, double dh, int sx, int sy, int sw, int sh,  
                         uint32_t iUsage, uint32_t offBits, uint32_t cbBits, uint32_t offBmi, uint32_t cbBmi);
   static int         myEnhMetaFileProc(char *contents, unsigned int length, PEMF_CALLBACK_DATA d);
   static void        free_emf_strings(EMF_STRINGS name);

};

} } }  /* namespace Inkscape, Extension, Implementation */


#endif /* EXTENSION_INTERNAL_EMF_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
