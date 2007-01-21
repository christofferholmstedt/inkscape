#define __SP_IMAGE_C__

/*
 * SVG <image> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Edward Flick (EAF)
 *
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <libnr/nr-matrix-fns.h>

//#define GDK_PIXBUF_ENABLE_BACKEND 1
//#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include "display/nr-arena-image.h"

//Added for preserveAspectRatio support -- EAF
#include "enums.h"
#include "attributes.h"

#include "print.h"
#include "brokenimage.xpm"
#include "document.h"
#include "sp-image.h"
#include <glibmm/i18n.h>
#include "xml/quote.h"
#include <xml/repr.h>

#include "io/sys.h"
#include <png.h>
#if ENABLE_LCMS
#include "color-profile-fns.h"
#include "color-profile.h"
//#define DEBUG_LCMS
#ifdef DEBUG_LCMS
#include "prefs-utils.h"
#include <gtk/gtkmessagedialog.h>
#endif // DEBUG_LCMS
#endif // ENABLE_LCMS
/*
 * SPImage
 */


static void sp_image_class_init (SPImageClass * klass);
static void sp_image_init (SPImage * image);

static void sp_image_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
static void sp_image_release (SPObject * object);
static void sp_image_set (SPObject *object, unsigned int key, const gchar *value);
static void sp_image_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static Inkscape::XML::Node *sp_image_write (SPObject *object, Inkscape::XML::Node *repr, guint flags);

static void sp_image_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags);
static void sp_image_print (SPItem * item, SPPrintContext *ctx);
static gchar * sp_image_description (SPItem * item);
static void sp_image_snappoints(SPItem const *item, SnapPointsIter p);
static NRArenaItem *sp_image_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static NR::Matrix sp_image_set_transform (SPItem *item, NR::Matrix const &xform);

GdkPixbuf * sp_image_repr_read_image (Inkscape::XML::Node * repr);
static GdkPixbuf *sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf);
static void sp_image_update_canvas_image (SPImage *image);
static GdkPixbuf * sp_image_repr_read_dataURI (const gchar * uri_data);
static GdkPixbuf * sp_image_repr_read_b64 (const gchar * uri_data);

static SPItemClass *parent_class;


extern "C"
{
    void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length );
    void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length );
    void user_flush_data( png_structp png_ptr );

}


#ifdef DEBUG_LCMS
extern guint update_in_progress;
#define DEBUG_MESSAGE(key, ...) \
{\
    gint dump = prefs_get_int_attribute_limited("options.scislac", #key, 0, 0, 1);\
    gint dumpD = prefs_get_int_attribute_limited("options.scislac", #key"D", 0, 0, 1);\
    gint dumpD2 = prefs_get_int_attribute_limited("options.scislac", #key"D2", 0, 0, 1);\
    dumpD &= ( (update_in_progress == 0) || dumpD2 );\
    if ( dump )\
    {\
        g_message( __VA_ARGS__ );\
\
    }\
    if ( dumpD )\
    {\
        GtkWidget *dialog = gtk_message_dialog_new(NULL,\
                                                   GTK_DIALOG_DESTROY_WITH_PARENT, \
                                                   GTK_MESSAGE_INFO,    \
                                                   GTK_BUTTONS_OK,      \
                                                   __VA_ARGS__          \
                                                   );\
        g_signal_connect_swapped(dialog, "response",\
                                 G_CALLBACK(gtk_widget_destroy),        \
                                 dialog);                               \
        gtk_widget_show_all( dialog );\
    }\
}
#endif // DEBUG_LCMS

namespace Inkscape {
namespace IO {

class PushPull
{
public:
    gboolean    first;
    FILE*       fp;
    guchar*     scratch;
    gsize       size;
    gsize       used;
    gsize       offset;
    GdkPixbufLoader *loader;

    PushPull() : first(TRUE),
                 fp(0),
                 scratch(0),
                 size(0),
                 used(0),
                 offset(0),
                 loader(0) {};

    gboolean readMore()
    {
        gboolean good = FALSE;
        if ( offset )
        {
            g_memmove( scratch, scratch + offset, used - offset );
            used -= offset;
            offset = 0;
        }
        if ( used < size )
        {
            gsize space = size - used;
            gsize got = fread( scratch + used, 1, space, fp );
            if ( got )
            {
                if ( loader )
                {
                    GError *err = NULL;
                    //g_message( " __read %d bytes", (int)got );
                    if ( !gdk_pixbuf_loader_write( loader, scratch + used, got, &err ) )
                    {
                        //g_message("_error writing pixbuf data");
                    }
                }

                used += got;
                good = TRUE;
            }
            else
            {
                good = FALSE;
            }
        }
        return good;
    }

    gsize available() const
    {
        return (used - offset);
    }

    gsize readOut( gpointer data, gsize length )
    {
        gsize giving = available();
        if ( length < giving )
        {
            giving = length;
        }
        g_memmove( data, scratch + offset, giving );
        offset += giving;
        if ( offset >= used )
        {
            offset = 0;
            used = 0;
        }
        return giving;
    }

    void clear()
    {
        offset = 0;
        used = 0;
    }

private:
    PushPull& operator = (const PushPull& other);
    PushPull(const PushPull& other);
};

void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length )
{
//    g_message( "user_read_data(%d)", length );

    PushPull* youme = (PushPull*)png_get_io_ptr(png_ptr);

    gsize filled = 0;
    gboolean canRead = TRUE;

    while ( filled < length && canRead )
    {
        gsize some = youme->readOut( data + filled, length - filled );
        filled += some;
        if ( filled < length )
        {
            canRead &= youme->readMore();
        }
    }
//    g_message("things out");
}

void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length )
{
    //g_message( "user_write_data(%d)", length );
}

void user_flush_data( png_structp png_ptr )
{
    //g_message( "user_flush_data" );
}

GdkPixbuf*  pixbuf_new_from_file( const char *filename, GError **error )
{
    GdkPixbuf* buf = NULL;
    PushPull youme;
    gint dpiX = 0;
    gint dpiY = 0;

    //buf = gdk_pixbuf_new_from_file( filename, error );
    dump_fopen_call( filename, "pixbuf_new_from_file" );
    FILE* fp = fopen_utf8name( filename, "r" );
    if ( fp )
    {
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
        if ( loader )
        {
            GError *err = NULL;

            // short buffer
            guchar scratch[1024];
            gboolean latter = FALSE;
            gboolean isPng = FALSE;
            png_structp pngPtr = NULL;
            png_infop infoPtr = NULL;
            //png_infop endPtr = NULL;

            youme.fp = fp;
            youme.scratch = scratch;
            youme.size = sizeof(scratch);
            youme.used = 0;
            youme.offset = 0;
            youme.loader = loader;

            while ( !feof(fp) )
            {
                if ( youme.readMore() )
                {
                    if ( youme.first )
                    {
                        //g_message( "First data chunk" );
                        youme.first = FALSE;
                        isPng = !png_sig_cmp( scratch + youme.offset, 0, youme.available() );
                        //g_message( "  png? %s", (isPng ? "Yes":"No") );
                        if ( isPng )
                        {
                            pngPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
                                                             NULL,//(png_voidp)user_error_ptr,
                                                             NULL,//user_error_fn,
                                                             NULL//user_warning_fn
                                );
                            if ( pngPtr )
                            {
                                infoPtr = png_create_info_struct( pngPtr );
                                //endPtr = png_create_info_struct( pngPtr );

                                png_set_read_fn( pngPtr, &youme, user_read_data );
                                //g_message( "In" );

                                //png_read_info( pngPtr, infoPtr );
                                png_read_png( pngPtr, infoPtr, PNG_TRANSFORM_IDENTITY, NULL );

                                //g_message("out");

                                //png_read_end(pngPtr, endPtr);

                                /*
                                if ( png_get_valid( pngPtr, infoPtr, PNG_INFO_pHYs ) )
                                {
                                    g_message("pHYs chunk now valid" );
                                }
                                if ( png_get_valid( pngPtr, infoPtr, PNG_INFO_sCAL ) )
                                {
                                    g_message("sCAL chunk now valid" );
                                }
                                */

                                png_uint_32 res_x = 0;
                                png_uint_32 res_y = 0;
                                int unit_type = 0;
                                if ( png_get_pHYs( pngPtr, infoPtr, &res_x, &res_y, &unit_type) )
                                {
//                                     g_message( "pHYs yes (%d, %d) %d (%s)", (int)res_x, (int)res_y, unit_type,
//                                                (unit_type == 1? "per meter" : "unknown")
//                                         );

//                                     g_message( "    dpi: (%d, %d)",
//                                                (int)(0.5 + ((double)res_x)/39.37),
//                                                (int)(0.5 + ((double)res_y)/39.37) );
                                    if ( unit_type == PNG_RESOLUTION_METER )
                                    {
                                        // TODO come up with a more accurate DPI setting
                                        dpiX = (int)(0.5 + ((double)res_x)/39.37);
                                        dpiY = (int)(0.5 + ((double)res_y)/39.37);
                                    }
                                }
                                else
                                {
//                                     g_message( "pHYs no" );
                                }

/*
                                double width = 0;
                                double height = 0;
                                int unit = 0;
                                if ( png_get_sCAL(pngPtr, infoPtr, &unit, &width, &height) )
                                {
                                    gchar* vals[] = {
                                        "unknown", // PNG_SCALE_UNKNOWN
                                        "meter", // PNG_SCALE_METER
                                        "radian", // PNG_SCALE_RADIAN
                                        "last", //
                                        NULL
                                    };

                                    g_message( "sCAL: (%f, %f) %d (%s)",
                                               width, height, unit,
                                               ((unit >= 0 && unit < 3) ? vals[unit]:"???")
                                        );
                                }
*/

                                // now clean it up.
                                png_destroy_read_struct( &pngPtr, &infoPtr, NULL );//&endPtr );
                            }
                            else
                            {
                                g_message("Error when creating PNG read struct");
                            }
                        }
                    }
                    else if ( !latter )
                    {
                        latter = TRUE;
                        //g_message("  READing latter");
                    }
                    // Now clear out the buffer so we can read more.
                    // (dumping out unused)
                    youme.clear();
                }
            }

            gboolean ok = gdk_pixbuf_loader_close(loader, &err);
            if ( ok )
            {
                buf = gdk_pixbuf_loader_get_pixbuf( loader );
                if ( buf )
                {
                    g_object_ref(buf);

                    if ( dpiX )
                    {
                        gchar *tmp = g_strdup_printf( "%d", dpiX );
                        if ( tmp )
                        {
                            //gdk_pixbuf_set_option( buf, "Inkscape::DpiX", tmp );
                            g_free( tmp );
                        }
                    }
                    if ( dpiY )
                    {
                        gchar *tmp = g_strdup_printf( "%d", dpiY );
                        if ( tmp )
                        {
                            //gdk_pixbuf_set_option( buf, "Inkscape::DpiY", tmp );
                            g_free( tmp );
                        }
                    }
                }
            }
            else
            {
                // do something
                g_message("error loading pixbuf at close");
            }

            g_object_unref(loader);
        }
        else
        {
            g_message("error when creating pixbuf loader");
        }
        fclose( fp );
        fp = NULL;
    }
    else
    {
        g_warning ("Unable to open linked file: %s", filename);
    }

/*
    if ( buf )
    {
        const gchar* bloop = gdk_pixbuf_get_option( buf, "Inkscape::DpiX" );
        if ( bloop )
        {
            g_message("DPI X is [%s]", bloop);
        }
        bloop = gdk_pixbuf_get_option( buf, "Inkscape::DpiY" );
        if ( bloop )
        {
            g_message("DPI Y is [%s]", bloop);
        }
    }
*/

    return buf;
}

}
}

GType
sp_image_get_type (void)
{
	static GType image_type = 0;
	if (!image_type) {
		GTypeInfo image_info = {
			sizeof (SPImageClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_image_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPImage),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_image_init,
			NULL,	/* value_table */
		};
		image_type = g_type_register_static (sp_item_get_type (), "SPImage", &image_info, (GTypeFlags)0);
	}
	return image_type;
}

static void
sp_image_class_init (SPImageClass * klass)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gobject_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = (SPItemClass*)g_type_class_ref (sp_item_get_type ());

	sp_object_class->build = sp_image_build;
	sp_object_class->release = sp_image_release;
	sp_object_class->set = sp_image_set;
	sp_object_class->update = sp_image_update;
	sp_object_class->write = sp_image_write;

	item_class->bbox = sp_image_bbox;
	item_class->print = sp_image_print;
	item_class->description = sp_image_description;
	item_class->show = sp_image_show;
	item_class->snappoints = sp_image_snappoints;
	item_class->set_transform = sp_image_set_transform;
}

static void
sp_image_init (SPImage *image)
{
	image->x.unset();
	image->y.unset();
	image->width.unset();
	image->height.unset();
	image->aspect_align = SP_ASPECT_NONE;
}

static void
sp_image_build (SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);

	sp_object_read_attr (object, "xlink:href");
	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "width");
	sp_object_read_attr (object, "height");
	sp_object_read_attr (object, "preserveAspectRatio");
	sp_object_read_attr (object, "color-profile");

	/* Register */
	sp_document_add_resource (document, "image", object);
}

static void
sp_image_release (SPObject *object)
{
	SPImage *image;

	image = SP_IMAGE (object);

	if (SP_OBJECT_DOCUMENT (object)) {
		/* Unregister ourselves */
		sp_document_remove_resource (SP_OBJECT_DOCUMENT (object), "image", SP_OBJECT (object));
	}

	if (image->href) {
		g_free (image->href);
		image->href = NULL;
	}

	if (image->pixbuf) {
		gdk_pixbuf_unref (image->pixbuf);
		image->pixbuf = NULL;
	}

#if ENABLE_LCMS
	if (image->color_profile) {
		g_free (image->color_profile);
		image->color_profile = NULL;
	}
#endif // ENABLE_LCMS

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_image_set (SPObject *object, unsigned int key, const gchar *value)
{
	SPImage *image;

	image = SP_IMAGE (object);

	switch (key) {
	case SP_ATTR_XLINK_HREF:
		g_free (image->href);
		image->href = (value) ? g_strdup (value) : NULL;
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
		break;
	case SP_ATTR_X:
		if (!image->x.readAbsolute(value)) {
		    /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
			image->x.unset();
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (!image->y.readAbsolute(value)) {
		    /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
			image->y.unset();
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_WIDTH:
		if (!image->width.readAbsolute(value)) {
		    /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
			image->width.unset();
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_HEIGHT:
		if (!image->height.readAbsolute(value)) {
		    /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
			image->height.unset();
		}
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_PRESERVEASPECTRATIO:
		/* Do setup before, so we can use break to escape */
		image->aspect_align = SP_ASPECT_NONE;
		image->aspect_clip = SP_ASPECT_MEET;
		object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		if (value) {
			int len;
			gchar c[256];
			const gchar *p, *e;
			unsigned int align, clip;
			p = value;
			while (*p && *p == 32) p += 1;
			if (!*p) break;
			e = p;
			while (*e && *e != 32) e += 1;
			len = e - p;
			if (len > 8) break;
			memcpy (c, value, len);
			c[len] = 0;
			/* Now the actual part */
			if (!strcmp (c, "none")) {
				align = SP_ASPECT_NONE;
			} else if (!strcmp (c, "xMinYMin")) {
				align = SP_ASPECT_XMIN_YMIN;
			} else if (!strcmp (c, "xMidYMin")) {
				align = SP_ASPECT_XMID_YMIN;
			} else if (!strcmp (c, "xMaxYMin")) {
				align = SP_ASPECT_XMAX_YMIN;
			} else if (!strcmp (c, "xMinYMid")) {
				align = SP_ASPECT_XMIN_YMID;
			} else if (!strcmp (c, "xMidYMid")) {
				align = SP_ASPECT_XMID_YMID;
			} else if (!strcmp (c, "xMaxYMid")) {
				align = SP_ASPECT_XMAX_YMID;
			} else if (!strcmp (c, "xMinYMax")) {
				align = SP_ASPECT_XMIN_YMAX;
			} else if (!strcmp (c, "xMidYMax")) {
				align = SP_ASPECT_XMID_YMAX;
			} else if (!strcmp (c, "xMaxYMax")) {
				align = SP_ASPECT_XMAX_YMAX;
			} else {
				break;
			}
			clip = SP_ASPECT_MEET;
			while (*e && *e == 32) e += 1;
			if (e) {
				if (!strcmp (e, "meet")) {
					clip = SP_ASPECT_MEET;
				} else if (!strcmp (e, "slice")) {
					clip = SP_ASPECT_SLICE;
				} else {
					break;
				}
			}
			image->aspect_align = align;
			image->aspect_clip = clip;
		}
		break;
#if ENABLE_LCMS
        case SP_PROP_COLOR_PROFILE:
                if ( image->color_profile ) {
                    g_free (image->color_profile);
                }
                image->color_profile = (value) ? g_strdup (value) : NULL;
#ifdef DEBUG_LCMS
                if ( value ) {
                    DEBUG_MESSAGE( lcmsFour, "<image> color-profile set to '%s'", value );
                } else {
                    DEBUG_MESSAGE( lcmsFour, "<image> color-profile cleared" );
                }
#endif // DEBUG_LCMS
                // TODO check on this HREF_MODIFIED flag
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
                break;
#endif // ENABLE_LCMS
	default:
		if (((SPObjectClass *) (parent_class))->set)
			((SPObjectClass *) (parent_class))->set (object, key, value);
		break;
	}
}

static void
sp_image_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	SPImage *image;

	image = (SPImage *) object;

	if (((SPObjectClass *) (parent_class))->update)
		((SPObjectClass *) (parent_class))->update (object, ctx, flags);

	if (flags & SP_IMAGE_HREF_MODIFIED_FLAG) {
		if (image->pixbuf) {
			gdk_pixbuf_unref (image->pixbuf);
			image->pixbuf = NULL;
		}
		if (image->href) {
			GdkPixbuf *pixbuf;
			pixbuf = sp_image_repr_read_image (object->repr);
			if (pixbuf) {
				pixbuf = sp_image_pixbuf_force_rgba (pixbuf);
// BLIP
#if ENABLE_LCMS
                                if ( image->color_profile )
                                {
                                    int imagewidth = gdk_pixbuf_get_width( pixbuf );
                                    int imageheight = gdk_pixbuf_get_height( pixbuf );
                                    int rowstride = gdk_pixbuf_get_rowstride( pixbuf );
                                    guchar* px = gdk_pixbuf_get_pixels( pixbuf );

                                    if ( px ) {
#ifdef DEBUG_LCMS
                                        DEBUG_MESSAGE( lcmsFive, "in <image>'s sp_image_update. About to call colorprofile_get_handle()" );
#endif // DEBUG_LCMS
                                        guint profIntent = Inkscape::RENDERING_INTENT_UNKNOWN;
                                        cmsHPROFILE prof = Inkscape::colorprofile_get_handle( SP_OBJECT_DOCUMENT( object ),
                                                                                              &profIntent,
                                                                                              image->color_profile );
                                        if ( prof ) {
                                            icProfileClassSignature profileClass = cmsGetDeviceClass( prof );
                                            if ( profileClass != icSigNamedColorClass ) {
                                                int intent = INTENT_PERCEPTUAL;
                                                switch ( profIntent ) {
                                                    case Inkscape::RENDERING_INTENT_RELATIVE_COLORIMETRIC:
                                                        intent = INTENT_RELATIVE_COLORIMETRIC;
                                                        break;
                                                    case Inkscape::RENDERING_INTENT_SATURATION:
                                                        intent = INTENT_SATURATION;
                                                        break;
                                                    case Inkscape::RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
                                                        intent = INTENT_ABSOLUTE_COLORIMETRIC;
                                                        break;
                                                    case Inkscape::RENDERING_INTENT_PERCEPTUAL:
                                                    case Inkscape::RENDERING_INTENT_UNKNOWN:
                                                    case Inkscape::RENDERING_INTENT_AUTO:
                                                    default:
                                                        intent = INTENT_PERCEPTUAL;
                                                }
                                                cmsHPROFILE destProf = cmsCreate_sRGBProfile();
                                                cmsHTRANSFORM transf = cmsCreateTransform( prof, 
                                                                                           TYPE_RGBA_8,
                                                                                           destProf,
                                                                                           TYPE_RGBA_8,
                                                                                           intent, 0 );
                                                if ( transf ) {
                                                    guchar* currLine = px;
                                                    for ( int y = 0; y < imageheight; y++ ) {
                                                        // Since the types are the same size, we can do the transformation in-place
                                                        cmsDoTransform( transf, currLine, currLine, imagewidth );
                                                        currLine += rowstride;
                                                    }

                                                    cmsDeleteTransform( transf );
                                                }
#ifdef DEBUG_LCMS
                                                else
                                                {
                                                    DEBUG_MESSAGE( lcmsSix, "in <image>'s sp_image_update. Unable to create LCMS transform." );
                                                }
#endif // DEBUG_LCMS
                                                cmsCloseProfile( destProf );
                                            }
#ifdef DEBUG_LCMS
                                            else
                                            {
                                                DEBUG_MESSAGE( lcmsSeven, "in <image>'s sp_image_update. Profile type is named color. Can't transform." );
                                            }
#endif // DEBUG_LCMS
                                        }
#ifdef DEBUG_LCMS
                                        else
                                        {
                                            DEBUG_MESSAGE( lcmsEight, "in <image>'s sp_image_update. No profile found." );
                                        }
#endif // DEBUG_LCMS
                                    }
                                }
#endif // ENABLE_LCMS
				image->pixbuf = pixbuf;
			}
		}
	}
	// preserveAspectRatio calculate bounds / clipping rectangle -- EAF
	if (image->pixbuf && (image->aspect_align != SP_ASPECT_NONE)) {
		        int imagewidth, imageheight;
			double x,y;

		        imagewidth = gdk_pixbuf_get_width (image->pixbuf);
		        imageheight = gdk_pixbuf_get_height (image->pixbuf);

			switch (image->aspect_align) {
			case SP_ASPECT_XMIN_YMIN:
				x = 0.0;
				y = 0.0;
				break;
			case SP_ASPECT_XMID_YMIN:
				x = 0.5;
				y = 0.0;
				break;
			case SP_ASPECT_XMAX_YMIN:
				x = 1.0;
				y = 0.0;
				break;
			case SP_ASPECT_XMIN_YMID:
				x = 0.0;
				y = 0.5;
				break;
			case SP_ASPECT_XMID_YMID:
				x = 0.5;
				y = 0.5;
				break;
			case SP_ASPECT_XMAX_YMID:
				x = 1.0;
				y = 0.5;
				break;
			case SP_ASPECT_XMIN_YMAX:
				x = 0.0;
				y = 1.0;
				break;
			case SP_ASPECT_XMID_YMAX:
				x = 0.5;
				y = 1.0;
				break;
			case SP_ASPECT_XMAX_YMAX:
				x = 1.0;
				y = 1.0;
				break;
			default:
				x = 0.0;
				y = 0.0;
				break;
			}

			if (image->aspect_clip == SP_ASPECT_SLICE) {
				image->viewx = image->x.computed;
				image->viewy = image->y.computed;
				image->viewwidth = image->width.computed;
				image->viewheight = image->height.computed;
				if ((imagewidth*image->height.computed)>(image->width.computed*imageheight)) {
					// Pixels aspect is wider than bounding box
					image->trimheight = imageheight;
					image->trimwidth = static_cast<int>(static_cast<double>(imageheight) * image->width.computed / image->height.computed);
					image->trimy = 0;
					image->trimx = static_cast<int>(static_cast<double>(imagewidth - image->trimwidth) * x);
				} else {
					// Pixels aspect is taller than bounding box
					image->trimwidth = imagewidth;
					image->trimheight = static_cast<int>(static_cast<double>(imagewidth) * image->height.computed / image->width.computed);
					image->trimx = 0;
					image->trimy = static_cast<int>(static_cast<double>(imageheight - image->trimheight) * y);
				}
			} else {
				// Otherwise, assume SP_ASPECT_MEET
				image->trimx = 0;
				image->trimy = 0;
				image->trimwidth = imagewidth;
				image->trimheight = imageheight;
				if ((imagewidth*image->height.computed)>(image->width.computed*imageheight)) {
					// Pixels aspect is wider than bounding boz
					image->viewwidth = image->width.computed;
					image->viewheight = image->viewwidth * imageheight / imagewidth;
					image->viewx=image->x.computed;
					image->viewy=(image->height.computed - image->viewheight) * y + image->y.computed;
				} else {
					// Pixels aspect is taller than bounding box
					image->viewheight = image->height.computed;
					image->viewwidth = image->viewheight * imagewidth / imageheight;
					image->viewy=image->y.computed;
					image->viewx=(image->width.computed - image->viewwidth) * x + image->x.computed;
				}
			}
	}

	sp_image_update_canvas_image ((SPImage *) object);
}

static Inkscape::XML::Node *
sp_image_write (SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
	SPImage *image;

	image = SP_IMAGE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
                Inkscape::XML::Document *xml_doc = sp_document_repr_doc(SP_OBJECT_DOCUMENT(object));
		repr = xml_doc->createElement("svg:image");
	}

	repr->setAttribute("xlink:href", image->href);
	/* fixme: Reset attribute if needed (Lauris) */
	if (image->x._set) sp_repr_set_svg_double(repr, "x", image->x.computed);
	if (image->y._set) sp_repr_set_svg_double(repr, "y", image->y.computed);
	if (image->width._set) sp_repr_set_svg_double(repr, "width", image->width.computed);
	if (image->height._set) sp_repr_set_svg_double(repr, "height", image->height.computed);
	repr->setAttribute("preserveAspectRatio", object->repr->attribute("preserveAspectRatio"));
#if ENABLE_LCMS
        if (image->color_profile) repr->setAttribute("color-profile", image->color_profile);
#endif // ENABLE_LCMS

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_image_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags)
{
	SPImage const &image = *SP_IMAGE(item);

	if ((image.width.computed > 0.0) && (image.height.computed > 0.0)) {
		double const x0 = image.x.computed;
		double const y0 = image.y.computed;
		double const x1 = x0 + image.width.computed;
		double const y1 = y0 + image.height.computed;

		nr_rect_union_pt(bbox, NR::Point(x0, y0) * transform);
		nr_rect_union_pt(bbox, NR::Point(x1, y0) * transform);
		nr_rect_union_pt(bbox, NR::Point(x1, y1) * transform);
		nr_rect_union_pt(bbox, NR::Point(x0, y1) * transform);
	}
}

static void
sp_image_print (SPItem *item, SPPrintContext *ctx)
{
	SPImage *image;
	NRMatrix tp, ti, s, t;
	guchar *px;
	int w, h, rs, pixskip;

	image = SP_IMAGE (item);

	if (!image->pixbuf) return;
	if ((image->width.computed <= 0.0) || (image->height.computed <= 0.0)) return;

	px = gdk_pixbuf_get_pixels (image->pixbuf);
	w = gdk_pixbuf_get_width (image->pixbuf);
	h = gdk_pixbuf_get_height (image->pixbuf);
	rs = gdk_pixbuf_get_rowstride (image->pixbuf);
	pixskip = gdk_pixbuf_get_n_channels (image->pixbuf) * gdk_pixbuf_get_bits_per_sample (image->pixbuf) / 8;

	if (image->aspect_align == SP_ASPECT_NONE) {
		/* fixme: (Lauris) */
		nr_matrix_set_translate (&tp, image->x.computed, image->y.computed);
		nr_matrix_set_scale (&s, image->width.computed, -image->height.computed);
		nr_matrix_set_translate (&ti, 0.0, -1.0);
	} else { // preserveAspectRatio
		nr_matrix_set_translate (&tp, image->viewx, image->viewy);
		nr_matrix_set_scale (&s, image->viewwidth, -image->viewheight);
		nr_matrix_set_translate (&ti, 0.0, -1.0);
	}

	nr_matrix_multiply (&t, &s, &tp);
	nr_matrix_multiply (&t, &ti, &t);

	if (image->aspect_align == SP_ASPECT_NONE)
		sp_print_image_R8G8B8A8_N (ctx, px, w, h, rs, &t, SP_OBJECT_STYLE (item));
	else // preserveAspectRatio
		sp_print_image_R8G8B8A8_N (ctx, px + image->trimx*pixskip + image->trimy*rs, image->trimwidth, image->trimheight, rs, &t, SP_OBJECT_STYLE (item));
}

static gchar *
sp_image_description(SPItem *item)
{
	SPImage *image = SP_IMAGE(item);
	char *href_desc;
        if (image->href) {
            href_desc = (strncmp(image->href, "data:", 5) == 0)
                ? g_strdup(_("embedded"))
                : xml_quote_strdup(image->href);
        } else {
            g_warning("Attempting to call strncmp() with a null pointer.");
            href_desc = g_strdup("(null_pointer)"); // we call g_free() on href_desc
        }

	char *ret = ( image->pixbuf == NULL
		      ? g_strdup_printf(_("<b>Image with bad reference</b>: %s"), href_desc)
		      : g_strdup_printf(_("<b>Image</b> %d &#215; %d: %s"),
					gdk_pixbuf_get_width(image->pixbuf),
					gdk_pixbuf_get_height(image->pixbuf),
					href_desc) );
	g_free(href_desc);
	return ret;
}

static NRArenaItem *
sp_image_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	int pixskip, rs;
	SPImage * image;
	NRArenaItem *ai;

	image = (SPImage *) item;

	ai = NRArenaImage::create(arena);

	if (image->pixbuf) {
	        pixskip = gdk_pixbuf_get_n_channels (image->pixbuf) * gdk_pixbuf_get_bits_per_sample (image->pixbuf) / 8;
		rs = gdk_pixbuf_get_rowstride (image->pixbuf);
                nr_arena_image_set_style(NR_ARENA_IMAGE(ai), SP_OBJECT_STYLE(SP_OBJECT(item)));
	        if (image->aspect_align == SP_ASPECT_NONE)
			nr_arena_image_set_pixels (NR_ARENA_IMAGE (ai),
					   gdk_pixbuf_get_pixels (image->pixbuf),
					   gdk_pixbuf_get_width (image->pixbuf),
					   gdk_pixbuf_get_height (image->pixbuf),
					   rs);
		else // preserveAspectRatio
			nr_arena_image_set_pixels (NR_ARENA_IMAGE (ai),
					   gdk_pixbuf_get_pixels (image->pixbuf) + image->trimx*pixskip + image->trimy*rs,
					   image->trimwidth,
					   image->trimheight,
					   rs);
	} else {
		nr_arena_image_set_pixels (NR_ARENA_IMAGE (ai), NULL, 0, 0, 0);
	}
	if (image->aspect_align == SP_ASPECT_NONE)
		nr_arena_image_set_geometry (NR_ARENA_IMAGE (ai), image->x.computed, image->y.computed, image->width.computed, image->height.computed);
	else // preserveAspectRatio
		nr_arena_image_set_geometry (NR_ARENA_IMAGE (ai), image->viewx, image->viewy, image->viewwidth, image->viewheight);

	return ai;
}

/*
 * utility function to try loading image from href
 *
 * docbase/relative_src
 * absolute_src
 *
 */

GdkPixbuf *
sp_image_repr_read_image (Inkscape::XML::Node * repr)
{
	const gchar * filename, * docbase;
	gchar * fullname;
	GdkPixbuf * pixbuf;

	filename = repr->attribute("xlink:href");
	if (filename == NULL) filename = repr->attribute("href"); /* FIXME */
	if (filename != NULL) {
		if (strncmp (filename,"file:",5) == 0) {
			fullname = g_filename_from_uri(filename, NULL, NULL);
			if (fullname) {
				// TODO check this. Was doing a UTF-8 to filename conversion here.
				pixbuf = Inkscape::IO::pixbuf_new_from_file (fullname, NULL);
				if (pixbuf != NULL) return pixbuf;
			}
		} else if (strncmp (filename,"data:",5) == 0) {
			/* data URI - embedded image */
			filename += 5;
			pixbuf = sp_image_repr_read_dataURI (filename);
			if (pixbuf != NULL) return pixbuf;
		} else if (!g_path_is_absolute (filename)) {
			/* try to load from relative pos */
			docbase = sp_repr_document_root (sp_repr_document (repr))->attribute("sodipodi:docbase");
			if (!docbase) docbase = ".";
			fullname = g_build_filename(docbase, filename, NULL);
			pixbuf = Inkscape::IO::pixbuf_new_from_file( fullname, NULL );
			g_free (fullname);
			if (pixbuf != NULL) return pixbuf;
		} else {
			/* try absolute filename */
			pixbuf = Inkscape::IO::pixbuf_new_from_file( filename, NULL );
			if (pixbuf != NULL) return pixbuf;
		}
	}
	/* at last try to load from sp absolute path name */
	filename = repr->attribute("sodipodi:absref");
	if (filename != NULL) {
		pixbuf = Inkscape::IO::pixbuf_new_from_file( filename, NULL );
		if (pixbuf != NULL) return pixbuf;
	}
	/* Nope: We do not find any valid pixmap file :-( */
	pixbuf = gdk_pixbuf_new_from_xpm_data ((const gchar **) brokenimage_xpm);

	/* It should be included xpm, so if it still does not does load, */
	/* our libraries are broken */
	g_assert (pixbuf != NULL);

	return pixbuf;
}

static GdkPixbuf *
sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf)
{
	GdkPixbuf * newbuf;

	if (gdk_pixbuf_get_has_alpha (pixbuf)) return pixbuf;

	newbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
	gdk_pixbuf_unref (pixbuf);

	return newbuf;
}

/* We assert that realpixbuf is either NULL or identical size to pixbuf */

static void
sp_image_update_canvas_image (SPImage *image)
{
	int rs, pixskip;
	SPItem *item;
	SPItemView *v;

	item = SP_ITEM (image);

	if (image->pixbuf) {
		/* fixme: We are slightly violating spec here (Lauris) */
		if (!image->width._set) {
			image->width.computed = gdk_pixbuf_get_width (image->pixbuf);
		}
		if (!image->height._set) {
			image->height.computed = gdk_pixbuf_get_height (image->pixbuf);
		}
	}

	for (v = item->display; v != NULL; v = v->next) {
	        pixskip = gdk_pixbuf_get_n_channels (image->pixbuf) * gdk_pixbuf_get_bits_per_sample (image->pixbuf) / 8;
		rs = gdk_pixbuf_get_rowstride (image->pixbuf);
                nr_arena_image_set_style (NR_ARENA_IMAGE(v->arenaitem), SP_OBJECT_STYLE(SP_OBJECT(image)));
		if (image->aspect_align == SP_ASPECT_NONE) {
			nr_arena_image_set_pixels (NR_ARENA_IMAGE (v->arenaitem),
					   gdk_pixbuf_get_pixels (image->pixbuf),
					   gdk_pixbuf_get_width (image->pixbuf),
					   gdk_pixbuf_get_height (image->pixbuf),
					   rs);
			nr_arena_image_set_geometry (NR_ARENA_IMAGE (v->arenaitem),
					     image->x.computed, image->y.computed,
					     image->width.computed, image->height.computed);
		} else { // preserveAspectRatio
			nr_arena_image_set_pixels (NR_ARENA_IMAGE (v->arenaitem),
					   gdk_pixbuf_get_pixels (image->pixbuf) + image->trimx*pixskip + image->trimy*rs,
					   image->trimwidth,
					   image->trimheight,
					   rs);
			nr_arena_image_set_geometry (NR_ARENA_IMAGE (v->arenaitem),
					     image->viewx, image->viewy,
					     image->viewwidth, image->viewheight);
		}
	}
}

static void sp_image_snappoints(SPItem const *item, SnapPointsIter p)
{
     if (((SPItemClass *) parent_class)->snappoints) {
         ((SPItemClass *) parent_class)->snappoints (item, p);
     }
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static NR::Matrix
sp_image_set_transform(SPItem *item, NR::Matrix const &xform)
{
	SPImage *image = SP_IMAGE(item);

	/* Calculate position in parent coords. */
	NR::Point pos( NR::Point(image->x.computed, image->y.computed) * xform );

	/* This function takes care of translation and scaling, we return whatever parts we can't
	   handle. */
	NR::Matrix ret(NR::transform(xform));
	NR::Point const scale(hypot(ret[0], ret[1]),
			      hypot(ret[2], ret[3]));
	if ( scale[NR::X] > 1e-9 ) {
		ret[0] /= scale[NR::X];
		ret[1] /= scale[NR::X];
	} else {
		ret[0] = 1.0;
		ret[1] = 0.0;
	}
	if ( scale[NR::Y] > 1e-9 ) {
		ret[2] /= scale[NR::Y];
		ret[3] /= scale[NR::Y];
	} else {
		ret[2] = 0.0;
		ret[3] = 1.0;
	}

	image->width = image->width.computed * scale[NR::X];
	image->height = image->height.computed * scale[NR::Y];

	/* Find position in item coords */
	pos = pos * ret.inverse();
	image->x = pos[NR::X];
	image->y = pos[NR::Y];

	item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

	return ret;
}

static GdkPixbuf *
sp_image_repr_read_dataURI (const gchar * uri_data)
{	GdkPixbuf * pixbuf = NULL;

	gint data_is_image = 0;
	gint data_is_base64 = 0;

	const gchar * data = uri_data;

	while (*data) {
		if (strncmp (data,"base64",6) == 0) {
			/* base64-encoding */
			data_is_base64 = 1;
			data_is_image = 1; // Illustrator produces embedded images without MIME type, so we assume it's image no matter what
			data += 6;
		}
		else if (strncmp (data,"image/png",9) == 0) {
			/* PNG image */
			data_is_image = 1;
			data += 9;
		}
		else if (strncmp (data,"image/jpg",9) == 0) {
			/* JPEG image */
			data_is_image = 1;
			data += 9;
		}
		else if (strncmp (data,"image/jpeg",10) == 0) {
			/* JPEG image */
			data_is_image = 1;
			data += 10;
		}
		else { /* unrecognized option; skip it */
			while (*data) {
				if (((*data) == ';') || ((*data) == ',')) break;
				data++;
			}
		}
		if ((*data) == ';') {
			data++;
			continue;
		}
		if ((*data) == ',') {
			data++;
			break;
		}
	}

	if ((*data) && data_is_image && data_is_base64) {
		pixbuf = sp_image_repr_read_b64 (data);
	}

	return pixbuf;
}

static GdkPixbuf *
sp_image_repr_read_b64 (const gchar * uri_data)
{	GdkPixbuf * pixbuf = NULL;
	GdkPixbufLoader * loader = NULL;

	gint j;
	gint k;
	gint l;
	gint b;
	gint len;
	gint eos = 0;
	gint failed = 0;

	guint32 bits;

	static const gchar B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const gchar* btr = uri_data;

	gchar ud[4];

	guchar bd[57];

	loader = gdk_pixbuf_loader_new ();

	if (loader == NULL) return NULL;

	while (eos == 0) {
		l = 0;
		for (j = 0; j < 19; j++) {
			len = 0;
			for (k = 0; k < 4; k++) {
				while (isspace ((int) (*btr))) {
					if ((*btr) == '\0') break;
					btr++;
				}
				if (eos) {
					ud[k] = 0;
					continue;
				}
				if (((*btr) == '\0') || ((*btr) == '=')) {
					eos = 1;
					ud[k] = 0;
					continue;
				}
				ud[k] = 64;
				for (b = 0; b < 64; b++) { /* There must a faster way to do this... ?? */
					if (B64[b] == (*btr)) {
						ud[k] = (gchar) b;
						break;
					}
				}
				if (ud[k] == 64) { /* data corruption ?? */
					eos = 1;
					ud[k] = 0;
					continue;
				}
				btr++;
				len++;
			}
			bits = (guint32) ud[0];
			bits = (bits << 6) | (guint32) ud[1];
			bits = (bits << 6) | (guint32) ud[2];
			bits = (bits << 6) | (guint32) ud[3];
			bd[l++] = (guchar) ((bits & 0xff0000) >> 16);
			if (len > 2) {
				bd[l++] = (guchar) ((bits & 0xff00) >>  8);
			}
			if (len > 3) {
				bd[l++] = (guchar)  (bits & 0xff);
			}
		}

		if (!gdk_pixbuf_loader_write (loader, (const guchar *) bd, (size_t) l, NULL)) {
			failed = 1;
			break;
		}
	}

	gdk_pixbuf_loader_close (loader, NULL);

	if (!failed) pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

	return pixbuf;
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
