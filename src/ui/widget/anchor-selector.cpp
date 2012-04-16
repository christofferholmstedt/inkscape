/*
 * anchor-selector.cpp
 *
 *  Created on: Mar 22, 2012
 *      Author: denis
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#include "ui/widget/anchor-selector.h"
#include <iostream>
#include "widgets/icon.h"
#include "ui/icon-names.h"

void AnchorSelector::setupButton(const Glib::ustring& icon, Gtk::ToggleButton& button) {
	Gtk::Widget*  buttonIcon = Gtk::manage(sp_icon_get_icon(icon, Inkscape::ICON_SIZE_SMALL_TOOLBAR));
	buttonIcon->show();

	button.set_relief(Gtk::RELIEF_NONE);
	button.show();
	button.add(*buttonIcon);
	button.set_can_focus(false);
}

AnchorSelector::AnchorSelector()
	: Gtk::Alignment(0.5, 0, 0, 0),
	  _container(3, 3, true)
{
	setupButton(INKSCAPE_ICON("boundingbox_top_left"),     _buttons[0]);
	setupButton(INKSCAPE_ICON("boundingbox_top"),          _buttons[1]);
	setupButton(INKSCAPE_ICON("boundingbox_top_right"),    _buttons[2]);
	setupButton(INKSCAPE_ICON("boundingbox_left"),         _buttons[3]);
	setupButton(INKSCAPE_ICON("boundingbox_center"),       _buttons[4]);
	setupButton(INKSCAPE_ICON("boundingbox_right"),        _buttons[5]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom_left"),  _buttons[6]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom"),       _buttons[7]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom_right"), _buttons[8]);

	for(int i = 0; i < 9; ++i) {
		_buttons[i].signal_clicked().connect(
				sigc::bind(sigc::mem_fun(*this, &AnchorSelector::btn_activated), i));
		_container.attach(_buttons[i], i % 3, i % 3+1, i / 3, i / 3+1, Gtk::FILL, Gtk::FILL);
	}
	_selection = 4;
	_buttons[4].set_active();

	this->add(_container);
}

AnchorSelector::~AnchorSelector()
{
	// TODO Auto-generated destructor stub
}

void AnchorSelector::btn_activated(int index)
{

	if(_selection == index && _buttons[index].get_active() == false)
	{
		_buttons[index].set_active(true);
	}
	else if(_selection != index && _buttons[index].get_active())
	{
		int old_selection = _selection;
		_selection = index;
		_buttons[old_selection].set_active(false);
		_selectionChanged.emit();
	}
}

void AnchorSelector::setAlignment(int horizontal, int vertical)
{
	int index = 3 * vertical + horizontal;
	if(index >= 0 && index < 9)
	{
		_buttons[index].set_active(!_buttons[index].get_active());
	}
}
