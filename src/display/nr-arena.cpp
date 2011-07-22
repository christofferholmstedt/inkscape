#define __NR_ARENA_C__

/*
 * RGBA display list system for inkscape
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "nr-arena-item.h"
#include "nr-arena.h"
#include "nr-filter-gaussian.h"
#include "nr-filter-types.h"
#include "preferences.h"
#include "color.h"
#include "libnr/nr-rect.h"

static void nr_arena_class_init (NRArenaClass *klass);
static void nr_arena_init (NRArena *arena);
static void nr_arena_finalize (NRObject *object);

static NRActiveObjectClass *parent_class;

NRType
nr_arena_get_type (void)
{
    static NRType type = 0;
    if (!type) {
        type = nr_object_register_type (NR_TYPE_ACTIVE_OBJECT,
                                        "NRArena",
                                        sizeof (NRArenaClass),
                                        sizeof (NRArena),
                                        (void (*) (NRObjectClass *)) nr_arena_class_init,
                                        (void (*) (NRObject *)) nr_arena_init);
    }
    return type;
}

static void
nr_arena_class_init (NRArenaClass *klass)
{
    NRObjectClass *object_class = (NRObjectClass *) klass;

    parent_class = (NRActiveObjectClass *) (((NRObjectClass *) klass)->parent);

    object_class->finalize = nr_arena_finalize;
    object_class->cpp_ctor = NRObject::invoke_ctor<NRArena>;
}

static void
nr_arena_init (NRArena *arena)
{
    arena->delta = 0; // to be set by desktop from prefs
    arena->renderoffscreen = false; // use render values from preferences otherwise render exact
    arena->rendermode = Inkscape::RENDERMODE_NORMAL; // default is normal render
    arena->colorrendermode = Inkscape::COLORRENDERMODE_NORMAL; // default is normal color
    arena->blurquality = BLUR_QUALITY_NORMAL;
    arena->filterquality = Inkscape::Filters::FILTER_QUALITY_NORMAL;
    arena->outlinecolor = 0xff; // black; to be set by desktop from bg color
    arena->canvasarena = NULL;
}

static void
nr_arena_finalize (NRObject *object)
{
    ((NRObjectClass *) (parent_class))->finalize (object);
}

void
nr_arena_request_update (NRArena *arena, NRArenaItem *item)
{
    NRActiveObject *aobject = (NRActiveObject *) arena;

    nr_return_if_fail (arena != NULL);
    nr_return_if_fail (NR_IS_ARENA (arena));
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    // setup render parameter
    if (arena->renderoffscreen == false) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        arena->blurquality = prefs->getInt("/options/blurquality/value", 0);
        arena->filterquality = prefs->getInt("/options/filterquality/value", 0);
    } else {
        arena->blurquality =  BLUR_QUALITY_BEST;
        arena->filterquality = Inkscape::Filters::FILTER_QUALITY_BEST;
        arena->rendermode = Inkscape::RENDERMODE_NORMAL;
        arena->colorrendermode = Inkscape::COLORRENDERMODE_NORMAL;
    }

    if (aobject->callbacks) {
        for (unsigned int i = 0; i < aobject->callbacks->length; i++) {
            NRObjectListener *listener = aobject->callbacks->listeners + i;
            NRArenaEventVector *avector = (NRArenaEventVector *) listener->vector;
            if ((listener->size >= sizeof (NRArenaEventVector)) && avector->request_update) {
                avector->request_update (arena, item, listener->data);
            }
        }
    }
}

void
nr_arena_request_render_rect (NRArena *arena, Geom::OptIntRect const &area)
{
    NRActiveObject *aobject = (NRActiveObject *) arena;

    nr_return_if_fail (arena != NULL);
    nr_return_if_fail (NR_IS_ARENA (arena));
    if (!area) return;

    // setup render parameter
    if (arena->renderoffscreen == false) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        arena->blurquality = prefs->getInt("/options/blurquality/value", 0);
        arena->filterquality = prefs->getInt("/options/filterquality/value", 0);
    } else {
        arena->blurquality =  BLUR_QUALITY_BEST;
        arena->filterquality = Inkscape::Filters::FILTER_QUALITY_BEST;
        arena->rendermode = Inkscape::RENDERMODE_NORMAL;
        arena->colorrendermode = Inkscape::COLORRENDERMODE_NORMAL;
    }
    NRRectL nr_area(*area);
    if (aobject->callbacks) {
        for (unsigned int i = 0; i < aobject->callbacks->length; i++) {
            NRObjectListener *listener = aobject->callbacks->listeners + i;
            NRArenaEventVector *avector = (NRArenaEventVector *) listener->vector;
            if ((listener->size >= sizeof (NRArenaEventVector)) && avector->request_render) {
                avector->request_render (arena, &nr_area, listener->data);
            }
        }
    }
}

/**
    set arena to offscreen mode
    rendering will be exact
    @param arena NRArena object
*/
void
nr_arena_set_renderoffscreen (NRArena *arena)
{
    nr_return_if_fail (arena != NULL);
    nr_return_if_fail (NR_IS_ARENA (arena));

    // the real assignment to the quality indicators is in the update function
    arena->renderoffscreen = true;

}

#define FLOAT_TO_UINT8(f) (int(f*255))
#define RGBA_R(v) ((v) >> 24)
#define RGBA_G(v) (((v) >> 16) & 0xff)
#define RGBA_B(v) (((v) >> 8) & 0xff)
#define RGBA_A(v) ((v) & 0xff)

void nr_arena_separate_color_plates(guint32* rgba){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool render_cyan = prefs->getBool("/options/printcolorspreview/cyan", true);
    bool render_magenta = prefs->getBool("/options/printcolorspreview/magenta", true);
    bool render_yellow = prefs->getBool("/options/printcolorspreview/yellow", true);
    bool render_black = prefs->getBool("/options/printcolorspreview/black", true);

    float rgb_v[3];
    float cmyk_v[4];
    sp_color_rgb_to_cmyk_floatv (cmyk_v, RGBA_R(*rgba)/256.0, RGBA_G(*rgba)/256.0, RGBA_B(*rgba)/256.0); 
    sp_color_cmyk_to_rgb_floatv (rgb_v, render_cyan ? cmyk_v[0] : 0,
                                        render_magenta ? cmyk_v[1] : 0,
                                        render_yellow ? cmyk_v[2] : 0,
                                        render_black ? cmyk_v[3] : 0);
    *rgba = (FLOAT_TO_UINT8(rgb_v[0])<<24) + (FLOAT_TO_UINT8(rgb_v[1])<<16) + (FLOAT_TO_UINT8(rgb_v[2])<<8) + 0xff;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
