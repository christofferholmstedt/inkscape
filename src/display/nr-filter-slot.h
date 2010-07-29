#ifndef __NR_FILTER_SLOT_H__
#define __NR_FILTER_SLOT_H__

/*
 * A container class for filter slots. Allows for simple getting and
 * setting images in filter slots without having to bother with
 * table indexes and such.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Niko Kiirala
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <map>
#include <cairo.h>
#include "libnr/nr-pixblock.h"
#include "display/nr-filter-types.h"
#include "display/nr-filter-units.h"

struct NRArenaItem;

namespace Inkscape {
namespace Filters {

class FilterSlot {
public:
    /** Creates a new FilterSlot object.
     * Parameter specifies the surface which should be used
     * for background accesses from filters.
     */
    FilterSlot(NRArenaItem *item, cairo_t *bgct, NRRectL const *bgarea,
        cairo_surface_t *graphic, NRRectL const *graphicarea, FilterUnits const &u);
    /** Destroys the FilterSlot object and all its contents */
    virtual ~FilterSlot();

    /** Returns the pixblock in specified slot.
     * Parameter 'slot' may be either an positive integer or one of
     * pre-defined filter slot types: NR_FILTER_SLOT_NOT_SET,
     * NR_FILTER_SOURCEGRAPHIC, NR_FILTER_SOURCEALPHA,
     * NR_FILTER_BACKGROUNDIMAGE, NR_FILTER_BACKGROUNDALPHA,
     * NR_FILTER_FILLPAINT, NR_FILTER_SOURCEPAINT.
     * If the defined filter slot is not set before, this function
     * returns NULL. Also, that filter slot is created in process.
     */
    cairo_surface_t *getcairo(int slot);
    NRPixBlock *get(int slot) { return NULL; }

    /** Sets or re-sets the pixblock associated with given slot.
     * If there was a pixblock already assigned with this slot,
     * that pixblock is destroyed.
     * Pixblocks passed to this function should be considered
     * managed by this FilterSlot object.
     * Pixblocks passed to this function should be reserved with
     * c++ -style new-operator.
     */
    void set(int slot, cairo_surface_t *s);

    void set(int, NRPixBlock*){}

    cairo_surface_t *get_result(int slot_nr);

    /** Returns the number of slots in use. */
    int get_slot_count();

    /** Sets the unit system to be used for the internal images. */
    //void set_units(FilterUnits const &units);

    /** Sets the filtering quality. Affects used interpolation methods */
    void set_quality(FilterQuality const q);

    /** Sets the gaussian filtering quality. Affects used interpolation methods */
    void set_blurquality(int const q);

    /** Gets the gaussian filtering quality. Affects used interpolation methods */
    int get_blurquality(void);

    FilterUnits const &get_units() const { return _units; }
    NRRectL const &get_slot_area() const { return _slot_area; }
    NRRectL const &get_sg_area() const { return *_source_graphic_area; }

private:
    typedef std::map<int, cairo_surface_t *> SlotMap;
    SlotMap _slots;
    NRArenaItem *_item;

    //Geom::Rect _source_bbox; ///< bounding box of source graphic surface
    //Geom::Rect _intermediate_bbox; ///< bounding box of intermediate surfaces

    NRRectL _slot_area;
    cairo_surface_t *_source_graphic;
    cairo_t *_background_ct;
    NRRectL const *_source_graphic_area;
    NRRectL const *_background_area; ///< needed to extract background
    FilterUnits const &_units;
    int _last_out;
    FilterQuality filterquality;
    int blurquality;

    cairo_surface_t *_get_transformed_source_graphic();
    cairo_surface_t *_get_transformed_background();
    cairo_surface_t *_get_fill_paint();
    cairo_surface_t *_get_stroke_paint();

    /** Returns the table index of given slot. If that slot does not exist,
     * it is created. Table index can be used to read the correct
     * pixblock from _slot */
    int _get_index(int slot);
    void _set_internal(int slot, cairo_surface_t *s);
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif // __NR_FILTER_SLOT_H__
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
