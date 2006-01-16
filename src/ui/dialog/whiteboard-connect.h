/**
 * Whiteboard connection establishment dialog
 *
 * Authors:
 * David Yip <yipdw@rose-hulman.edu>
 * Jason Segal, Jonas Collaros, Stephen Montgomery, Brandi Soggs, Matthew Weinstock (original C/Gtk version)
 *
 * Copyright (c) 2004-2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef __WHITEBOARD_CONNECT_DIALOG_H__
#define __WHITEBOARD_CONNECT_DIALOG_H__

#include "verbs.h"
#include "dialog.h"

struct SPDesktop;

namespace Inkscape {

namespace Whiteboard {

class SessionManager;

}

namespace UI {

namespace Dialog {

class WhiteboardConnectDialog : public Dialog {
public:
	WhiteboardConnectDialog() : Dialog("dialogs.whiteboard_connect", SP_VERB_DIALOG_WHITEBOARD_CONNECT)
	{

	}

	static WhiteboardConnectDialog* create();

	virtual ~WhiteboardConnectDialog()
	{

	}
};

class WhiteboardConnectDialogImpl : public WhiteboardConnectDialog {
public:
	WhiteboardConnectDialogImpl();
	~WhiteboardConnectDialogImpl();
	void present();
	void setSessionManager();

private:
	// GTK+ widgets
	Gtk::Table _layout;
	Gtk::HBox _buttons;

	Gtk::Entry _server;
	Gtk::Entry _username;
	Gtk::Entry _password;
	Gtk::Entry _port;

	Gtk::Label _labels[4];	

	Gtk::CheckButton _usessl;

	Gtk::Button _ok, _cancel;

	// Construction and callbacks
	void _construct();
	void _respCallback(int resp);
	void _useSSLClickedCallback();

	// SessionManager and SPDesktop pointers
	::SPDesktop* _desktop;
	Whiteboard::SessionManager* _sm;
};


}

}

}

#endif
