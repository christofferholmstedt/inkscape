/**
 * From the code of Liam P.White from his Power Stroke Knot dialog
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED &&HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include "lpe-fillet-chamfer-properties.h"
#include <boost/lexical_cast.hpp>
#include <gtkmm/stock.h>
#include <glibmm/main.h>
#include <glibmm/i18n.h>
#include "inkscape.h"
#include "desktop.h"
#include "document.h"
#include "document-undo.h"
#include "layer-manager.h"
#include "message-stack.h"
#include "desktop-handles.h"
#include "sp-object.h"
#include "sp-item.h"
#include "verbs.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "ui/icon-names.h"
#include "ui/widget/imagetoggler.h"
#include <cmath>
#include <gtkmm/radiobutton.h>
#include "util/units.h"

//#include "event-context.h"

namespace Inkscape {
namespace UI {
namespace Dialogs {

FilletChamferPropertiesDialog::FilletChamferPropertiesDialog()
    : _desktop(NULL), _knotpoint(NULL), _position_visible(false)
{
    Gtk::Box *mainVBox = this->get_vbox();
    mainVBox->set_homogeneous(false);
    _layout_table.set_spacings(4);
    _layout_table.resize(2, 2);

    // Layer name widgets
    _fillet_chamfer_position_entry.set_activates_default(true);
    _fillet_chamfer_position_label.set_label(_("Radius (pixels):"));
    _fillet_chamfer_position_label.set_alignment(1.0, 0.5);

    _layout_table.attach(_fillet_chamfer_position_label, 0, 1, 0, 1, Gtk::FILL,
                         Gtk::FILL);
    _layout_table.attach(_fillet_chamfer_position_entry, 1, 2, 0, 1,
                         Gtk::FILL | Gtk::EXPAND, Gtk::FILL);
    _fillet_chamfer_type_fillet.set_label(_("Fillet"));
    _fillet_chamfer_type_fillet.set_group(_fillet_chamfer_type_group);
    _fillet_chamfer_type_inverse_fillet.set_label(_("Inverse fillet"));
    _fillet_chamfer_type_inverse_fillet.set_group(_fillet_chamfer_type_group);
    _fillet_chamfer_type_chamfer.set_label(_("Chamfer"));
    _fillet_chamfer_type_chamfer.set_group(_fillet_chamfer_type_group);
    _fillet_chamfer_type_double_chamfer.set_label(_("Double chamfer"));
    _fillet_chamfer_type_double_chamfer.set_group(_fillet_chamfer_type_group);

    mainVBox->pack_start(_layout_table, true, true, 4);
    mainVBox->pack_start(_fillet_chamfer_type_fillet, true, true, 4);
    mainVBox->pack_start(_fillet_chamfer_type_inverse_fillet, true, true, 4);
    mainVBox->pack_start(_fillet_chamfer_type_chamfer, true, true, 4);
    mainVBox->pack_start(_fillet_chamfer_type_double_chamfer, true, true, 4);

    // Buttons
    _close_button.set_use_stock(true);
    _close_button.set_label(Gtk::Stock::CANCEL.id);
    _close_button.set_can_default();

    _apply_button.set_use_underline(true);
    _apply_button.set_can_default();

    _close_button.signal_clicked()
    .connect(sigc::mem_fun(*this, &FilletChamferPropertiesDialog::_close));
    _apply_button.signal_clicked()
    .connect(sigc::mem_fun(*this, &FilletChamferPropertiesDialog::_apply));

    signal_delete_event().connect(sigc::bind_return(
                                      sigc::hide(sigc::mem_fun(*this, &FilletChamferPropertiesDialog::_close)),
                                      true));

    add_action_widget(_close_button, Gtk::RESPONSE_CLOSE);
    add_action_widget(_apply_button, Gtk::RESPONSE_APPLY);

    _apply_button.grab_default();

    show_all_children();

    set_focus(_fillet_chamfer_position_entry);
}

FilletChamferPropertiesDialog::~FilletChamferPropertiesDialog()
{

    _setDesktop(NULL);
}

void FilletChamferPropertiesDialog::showDialog(
    SPDesktop *desktop, Geom::Point knotpoint,
    const Inkscape::LivePathEffect::
    FilletChamferPointArrayParamKnotHolderEntity *pt,
    const gchar *unit)
{
    FilletChamferPropertiesDialog *dialog = new FilletChamferPropertiesDialog();

    dialog->_setDesktop(desktop);
    dialog->_setUnit(unit);
    dialog->_setKnotPoint(knotpoint);
    dialog->_setPt(pt);

    dialog->set_title(_("Modify Fillet-Chamfer"));
    dialog->_apply_button.set_label(_("_Modify"));

    dialog->set_modal(true);
    desktop->setWindowTransient(dialog->gobj());
    dialog->property_destroy_with_parent() = true;

    dialog->show();
    dialog->present();
}

void FilletChamferPropertiesDialog::_apply()
{
    std::istringstream i_pos(_fillet_chamfer_position_entry.get_text());
    double d_pos, d_width;
    if (i_pos >> d_pos) {
        if (_fillet_chamfer_type_fillet.get_active() == true) {
            d_width = 1;
        } else if (_fillet_chamfer_type_inverse_fillet.get_active() == true) {
            d_width = 2;
        } else if (_fillet_chamfer_type_chamfer.get_active() == true) {
            d_width = 3;
        } else {
            d_width = 4;
        }
        if (_flexible) {
            if (d_pos > 99.99999 || d_pos < 0) {
                d_pos = 0;
            }
            d_pos = _index + (d_pos / 100);
        } else {
            d_pos = Inkscape::Util::Quantity::convert(d_pos, unit, "px");
            d_pos = d_pos * -1;
        }
        _knotpoint->knot_set_offset(Geom::Point(d_pos, d_width));
    }
    _close();
}

void FilletChamferPropertiesDialog::_close()
{
    _setDesktop(NULL);
    destroy_();
    Glib::signal_idle().connect(
        sigc::bind_return(
            sigc::bind(sigc::ptr_fun(&::operator delete), this),
            false
        )
    );
}

bool FilletChamferPropertiesDialog::_handleKeyEvent(GdkEventKey *event)
{
    return false;
}

void FilletChamferPropertiesDialog::_handleButtonEvent(GdkEventButton *event)
{
    if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
        _apply();
    }
}

void FilletChamferPropertiesDialog::_setKnotPoint(Geom::Point knotpoint)
{
    double position;
    if (knotpoint.x() > 0) {
        double intpart;
        position = modf(knotpoint[Geom::X], &intpart) * 100;
        _flexible = true;
        _index = intpart;
        _fillet_chamfer_position_label.set_label(_("Position (%):"));
    } else {
        _flexible = false;
        std::string posConcat =
            std::string(_("Position (")) + std::string(unit) + std::string(")");
        _fillet_chamfer_position_label.set_label(_(posConcat.c_str()));
        position = knotpoint[Geom::X] * -1;
        position = Inkscape::Util::Quantity::convert(position, "px", unit);
    }
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << position;
    _fillet_chamfer_position_entry.set_text(s.str());
    if (knotpoint.y() == 1) {
        _fillet_chamfer_type_fillet.set_active(true);
    } else if (knotpoint.y() == 2) {
        _fillet_chamfer_type_inverse_fillet.set_active(true);
    } else if (knotpoint.y() == 3) {
        _fillet_chamfer_type_chamfer.set_active(true);
    } else if (knotpoint.y() == 1) {
        _fillet_chamfer_type_double_chamfer.set_active(true);
    }
}

void FilletChamferPropertiesDialog::_setPt(
    const Inkscape::LivePathEffect::
    FilletChamferPointArrayParamKnotHolderEntity *pt)
{
    _knotpoint = const_cast<
                 Inkscape::LivePathEffect::FilletChamferPointArrayParamKnotHolderEntity *>(
                     pt);
}

void FilletChamferPropertiesDialog::_setUnit(const gchar *abbr)
{
    unit = abbr;
}

void FilletChamferPropertiesDialog::_setDesktop(SPDesktop *desktop)
{
    if (desktop) {
        Inkscape::GC::anchor(desktop);
    }
    if (_desktop) {
        Inkscape::GC::release(_desktop);
    }
    _desktop = desktop;
}

} // namespace
} // namespace
} // namespace

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:
// filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99
// :
