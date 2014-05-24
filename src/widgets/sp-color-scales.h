#ifndef SEEN_SP_COLOR_SCALES_H
#define SEEN_SP_COLOR_SCALES_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <color.h>
#include <widgets/sp-color-selector.h>


struct SPColorScales;
struct SPColorScalesClass;

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorSlider;

}
}
}

typedef enum {
    SP_COLOR_SCALES_MODE_NONE = 0,
    SP_COLOR_SCALES_MODE_RGB = 1,
    SP_COLOR_SCALES_MODE_HSV = 2,
    SP_COLOR_SCALES_MODE_CMYK = 3
} SPColorScalesMode;



class ColorScales: public ColorSelector
{
public:
    static gfloat getScaled( const GtkAdjustment *a );
    static void setScaled( GtkAdjustment *a, gfloat v);

    ColorScales(SPColorSelector *csel);
    virtual ~ColorScales();

    virtual void init();

    virtual void setSubmode(guint submode);
    virtual guint getSubmode() const;

    void setMode(SPColorScalesMode mode);
    SPColorScalesMode getMode() const;


protected:
    virtual void _colorChanged();

    static void _adjustmentAnyChanged(GtkAdjustment *adjustment, SPColorScales *cs);
    void _sliderAnyGrabbed();
    void _sliderAnyReleased();
    void _sliderAnyChanged();
    static void _adjustmentChanged(SPColorScales *cs, guint channel);

    void _getRgbaFloatv(gfloat *rgba);
    void _getCmykaFloatv(gfloat *cmyka);
    guint32 _getRgba32();
    void _updateSliders(guint channels);
    void _recalcColor(gboolean changing);

    void _setRangeLimit( gdouble upper );

    SPColorScalesMode _mode;
    gdouble _rangeLimit;
    gboolean _updating : 1;
    gboolean _dragging : 1;
    GtkAdjustment *_a[5]; /* Channel adjustments */
    Inkscape::UI::Widget::ColorSlider *_s[5]; /* Channel sliders */
    GtkWidget *_b[5]; /* Spinbuttons */
    GtkWidget *_l[5]; /* Labels */

private:
    // By default, disallow copy constructor and assignment operator
    ColorScales(ColorScales const &obj);
    ColorScales &operator=(ColorScales const &obj );
};



#define SP_TYPE_COLOR_SCALES (sp_color_scales_get_type())
#define SP_COLOR_SCALES(o) (G_TYPE_CHECK_INSTANCE_CAST((o), SP_TYPE_COLOR_SCALES, SPColorScales))
#define SP_COLOR_SCALES_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), SP_TYPE_COLOR_SCALES, SPColorScalesClass))
#define SP_IS_COLOR_SCALES(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), SP_TYPE_COLOR_SCALES))
#define SP_IS_COLOR_SCALES_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), SP_TYPE_COLOR_SCALES))

struct SPColorScales {
    SPColorSelector parent;
};

struct SPColorScalesClass {
    SPColorSelectorClass parent_class;
};

GType sp_color_scales_get_type();

GtkWidget *sp_color_scales_new();

#endif /* !SEEN_SP_COLOR_SCALES_H */

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
