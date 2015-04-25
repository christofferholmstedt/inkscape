#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sp-color-wheel-selector.h"

#include <math.h>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include "ui/dialog-events.h"
#include "sp-color-scales.h"
#include "sp-color-icc-selector.h"
#include "../svg/svg-icc-color.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/gimpcolorwheel.h"

G_BEGIN_DECLS

static void sp_color_wheel_selector_dispose(GObject *object);

static void sp_color_wheel_selector_show_all (GtkWidget *widget);
static void sp_color_wheel_selector_hide(GtkWidget *widget);


G_END_DECLS

#define XPAD 4
#define YPAD 1

const gchar* ColorWheelSelector::MODE_NAME = N_("Wheel");

G_DEFINE_TYPE(SPColorWheelSelector, sp_color_wheel_selector, SP_TYPE_COLOR_SELECTOR);

static void sp_color_wheel_selector_class_init(SPColorWheelSelectorClass *klass)
{
    static const gchar* nameset[] = {N_("Wheel"), 0};
    GObjectClass   *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    SPColorSelectorClass *selector_class = SP_COLOR_SELECTOR_CLASS (klass);

    selector_class->name = nameset;
    selector_class->submode_count = 1;

    object_class->dispose = sp_color_wheel_selector_dispose;

    widget_class->show_all = sp_color_wheel_selector_show_all;
    widget_class->hide = sp_color_wheel_selector_hide;
}

ColorWheelSelector::ColorWheelSelector( SPColorSelector* csel )
    : ColorSelector( csel ),
      _updating( FALSE ),
      _dragging( FALSE ),
      _adj(0),
      _wheel(0),
      _slider(0),
      _sbtn(0),
      _label(0)
{
}

ColorWheelSelector::~ColorWheelSelector()
{
    _adj = 0;
    _wheel = 0;
    _sbtn = 0;
    _label = 0;
}

void sp_color_wheel_selector_init (SPColorWheelSelector *cs)
{
    SP_COLOR_SELECTOR(cs)->base = new ColorWheelSelector( SP_COLOR_SELECTOR(cs) );

    if ( SP_COLOR_SELECTOR(cs)->base )
    {
        SP_COLOR_SELECTOR(cs)->base->init();
    }
}

void ColorWheelSelector::init()
{
    gint row = 0;

    _updating = FALSE;
    _dragging = FALSE;

#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *t = gtk_grid_new();
#else
    GtkWidget *t = gtk_table_new (5, 3, FALSE);
#endif

    gtk_widget_show (t);
    gtk_box_pack_start (GTK_BOX (_csel), t, TRUE, TRUE, 0);

    /* Create components */
    row = 0;

    _wheel = gimp_color_wheel_new();
    gtk_widget_show( _wheel );

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(_wheel, GTK_ALIGN_FILL);
    gtk_widget_set_valign(_wheel, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(_wheel, TRUE);
    gtk_widget_set_vexpand(_wheel, TRUE);
    gtk_grid_attach(GTK_GRID(t), _wheel, 0, row, 3, 1);
#else
    gtk_table_attach(GTK_TABLE(t), _wheel, 0, 3, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
#endif

    row++;

    /* Label */
    _label = gtk_label_new_with_mnemonic (_("_A:"));
    gtk_misc_set_alignment (GTK_MISC (_label), 1.0, 0.5);
    gtk_widget_show (_label);

#if GTK_CHECK_VERSION(3,0,0)
  #if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(_label, XPAD);
    gtk_widget_set_margin_end(_label, XPAD);
  #else
    gtk_widget_set_margin_left(_label, XPAD);
    gtk_widget_set_margin_right(_label, XPAD);
  #endif
    gtk_widget_set_margin_top(_label, YPAD);
    gtk_widget_set_margin_bottom(_label, YPAD);
    gtk_widget_set_halign(_label, GTK_ALIGN_FILL);
    gtk_widget_set_valign(_label, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(t), _label, 0, row, 1, 1);
#else
    gtk_table_attach (GTK_TABLE (t), _label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, XPAD, YPAD);
#endif

    /* Adjustment */
    _adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 255.0, 1.0, 10.0, 10.0));

    /* Slider */
    _slider = Gtk::manage(new Inkscape::UI::Widget::ColorSlider(Glib::wrap(_adj, true)));
    _slider->set_tooltip_text(_("Alpha (opacity)"));
    _slider->show();

#if GTK_CHECK_VERSION(3,0,0)
  #if GTK_CHECK_VERSION(3,12,0)
    _slider->set_margin_start(XPAD);
    _slider->set_margin_end(XPAD);
  #else
    _slider->set_margin_left(XPAD);
    _slider->set_margin_right(XPAD);
  #endif
    _slider->set_margin_top(YPAD);
    _slider->set_margin_bottom(YPAD);
    _slider->set_hexpand(true);
    _slider->set_halign(Gtk::ALIGN_FILL);
    _slider->set_valign(Gtk::ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(t), _slider->gobj(), 1, row, 1, 1);
#else
    gtk_table_attach(GTK_TABLE (t), _slider->gobj(), 1, 2, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_FILL, XPAD, YPAD);
#endif

    _slider->set_colors(SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 0.0),
                        SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 0.5),
                        SP_RGBA32_F_COMPOSE (1.0, 1.0, 1.0, 1.0));

    /* Spinbutton */
    _sbtn = gtk_spin_button_new (GTK_ADJUSTMENT (_adj), 1.0, 0);
    gtk_widget_set_tooltip_text (_sbtn, _("Alpha (opacity)"));
    sp_dialog_defocus_on_enter (_sbtn);
    gtk_label_set_mnemonic_widget (GTK_LABEL(_label), _sbtn);
    gtk_widget_show (_sbtn);

#if GTK_CHECK_VERSION(3,0,0)
  #if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(_sbtn, XPAD);
    gtk_widget_set_margin_end(_sbtn, XPAD);
  #else
    gtk_widget_set_margin_left(_sbtn, XPAD);
    gtk_widget_set_margin_right(_sbtn, XPAD);
  #endif
    gtk_widget_set_margin_top(_sbtn, YPAD);
    gtk_widget_set_margin_bottom(_sbtn, YPAD);
    gtk_widget_set_halign(_sbtn, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(_sbtn, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(t), _sbtn, 2, row, 1, 1);
#else
    gtk_table_attach (GTK_TABLE (t), _sbtn, 2, 3, row, row + 1, (GtkAttachOptions)0, (GtkAttachOptions)0, XPAD, YPAD);
#endif

    /* Signals */
    g_signal_connect (G_OBJECT (_adj), "value_changed",
                        G_CALLBACK (_adjustmentChanged), _csel);

    _slider->signal_grabbed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderGrabbed));
    _slider->signal_released.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderReleased));
    _slider->signal_value_changed.connect(sigc::mem_fun(*this, &ColorWheelSelector::_sliderChanged));

    g_signal_connect( G_OBJECT(_wheel), "changed",
                        G_CALLBACK (_wheelChanged), _csel );
}

static void sp_color_wheel_selector_dispose(GObject *object)
{
    if (G_OBJECT_CLASS(sp_color_wheel_selector_parent_class)->dispose)
        G_OBJECT_CLASS(sp_color_wheel_selector_parent_class)->dispose(object);
}

static void
sp_color_wheel_selector_show_all (GtkWidget *widget)
{
    gtk_widget_show (widget);
}

static void sp_color_wheel_selector_hide(GtkWidget *widget)
{
    gtk_widget_hide(widget);
}

GtkWidget *sp_color_wheel_selector_new()
{
    SPColorWheelSelector *csel = SP_COLOR_WHEEL_SELECTOR(g_object_new (SP_TYPE_COLOR_WHEEL_SELECTOR, NULL));

    return GTK_WIDGET (csel);
}

/* Helpers for setting color value */

void ColorWheelSelector::_preserve_icc(SPColor *color) const {
    color->icc = getColor().icc ? new SVGICCColor(*getColor().icc) : 0;
}

void ColorWheelSelector::_colorChanged()
{
#ifdef DUMP_CHANGE_INFO
    g_message("ColorWheelSelector::_colorChanged( this=%p, %f, %f, %f,   %f)", this, color.v.c[0], color.v.c[1], color.v.c[2], alpha );
#endif
    _updating = TRUE;
    {
        float hsv[3] = {0,0,0};
        sp_color_rgb_to_hsv_floatv(hsv, _color.v.c[0], _color.v.c[1], _color.v.c[2]);
        gimp_color_wheel_set_color( GIMP_COLOR_WHEEL(_wheel), hsv[0], hsv[1], hsv[2] );
    }

    guint32 start = _color.toRGBA32( 0x00 );
    guint32 mid = _color.toRGBA32( 0x7f );
    guint32 end = _color.toRGBA32( 0xff );

    _slider->set_colors(start, mid, end);

    ColorScales::setScaled(_adj, _alpha);

    _updating = FALSE;
}

void ColorWheelSelector::_adjustmentChanged( GtkAdjustment *adjustment, SPColorWheelSelector *cs )
{
// TODO check this. It looks questionable:
    // if a value is entered between 0 and 1 exclusive, normalize it to (int) 0..255  or 0..100
    gdouble value = gtk_adjustment_get_value (adjustment);
    gdouble upper = gtk_adjustment_get_upper (adjustment);
    
    if (value > 0.0 && value < 1.0) {
        gtk_adjustment_set_value( adjustment, floor (value * upper + 0.5) );
    }

    ColorWheelSelector* wheelSelector = static_cast<ColorWheelSelector*>(SP_COLOR_SELECTOR(cs)->base);
    if (wheelSelector->_updating) return;

    wheelSelector->_updating = TRUE;

    wheelSelector->_preserve_icc(&wheelSelector->_color);
    wheelSelector->_updateInternals( wheelSelector->_color, ColorScales::getScaled( wheelSelector->_adj ), wheelSelector->_dragging );

    wheelSelector->_updating = FALSE;
}

void ColorWheelSelector::_sliderGrabbed()
{
    if (!_dragging) {
        _dragging = TRUE;
        _grabbed();

        _preserve_icc(&_color);
        _updateInternals( _color, ColorScales::getScaled( _adj ), _dragging );
    }
}

void ColorWheelSelector::_sliderReleased()
{
    if (_dragging) {
        _dragging = FALSE;
        _released();

        _preserve_icc(&_color);
        _updateInternals( _color, ColorScales::getScaled( _adj ), _dragging );
    }
}

void ColorWheelSelector::_sliderChanged()
{
    _preserve_icc(&_color);
    _updateInternals( _color, ColorScales::getScaled( _adj ), _dragging );
}

void ColorWheelSelector::_wheelChanged( GimpColorWheel *wheel, SPColorWheelSelector *cs )
{
    ColorWheelSelector* wheelSelector = static_cast<ColorWheelSelector*>(SP_COLOR_SELECTOR(cs)->base);

    gdouble h = 0;
    gdouble s = 0;
    gdouble v = 0;
    gimp_color_wheel_get_color( wheel, &h, &s, &v );
    
    float rgb[3] = {0,0,0};
    sp_color_hsv_to_rgb_floatv (rgb, h, s, v); 

    SPColor color(rgb[0], rgb[1], rgb[2]);

    guint32 start = color.toRGBA32( 0x00 );
    guint32 mid = color.toRGBA32( 0x7f );
    guint32 end = color.toRGBA32( 0xff );

    wheelSelector->_slider->set_colors(start, mid, end);

    wheelSelector->_preserve_icc(&color);
    wheelSelector->_updateInternals( color, wheelSelector->_alpha, gimp_color_wheel_is_adjusting(wheel) );
}


Gtk::Widget *ColorWheelSelectorFactory::createWidget(Inkscape::UI::SelectedColor &color) const {
    GtkWidget *w = sp_color_selector_new(SP_TYPE_COLOR_WHEEL_SELECTOR);
    Gtk::Widget *wrapped = Glib::wrap(w);
    return wrapped;
}

Glib::ustring ColorWheelSelectorFactory::modeName() const {
    return gettext(ColorWheelSelector::MODE_NAME);
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
