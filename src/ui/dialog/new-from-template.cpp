/** @file
 * @brief New From Template main dialog - implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński    
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "new-from-template.h"

#include <gtkmm/alignment.h>

#include "file.h"


namespace Inkscape {
namespace UI {


NewFromTemplate::NewFromTemplate()
    : _create_template_button("Create from template")
{
    set_title("New From Template");
    resize(400, 400);
    
    get_vbox()->pack_start(_main_widget);
    _main_widget.append_page(_tab1, "Static Templates");
    _main_widget.append_page(_tab2, "Procedural Templates");
    
    Gtk::Alignment *align;
    align = manage(new Gtk::Alignment(Gtk::ALIGN_END, Gtk::ALIGN_CENTER, 0.0, 0.0));
    get_vbox()->pack_end(*align, Gtk::PACK_SHRINK, 5);
    align->add(_create_template_button);
    
    _create_template_button.signal_pressed().connect(
    sigc::mem_fun(*this, &NewFromTemplate::_createFromTemplate));
   
    show_all();
}


void NewFromTemplate::_createFromTemplate()
{
    if ( _main_widget.get_current_page() == 0 ) {
        _tab1.createTemplate();
    } else {
        _tab2.createTemplate();
    }
    
    response(0);
}

void NewFromTemplate::load_new_from_template()
{
    NewFromTemplate dl;
    dl.run();
}

}
}
