#ifndef __INK_EXTENSION_PARAMRADIOBUTTON_H__
#define __INK_EXTENSION_PARAMRADIOBUTTON_H__

/** \file
 * Radiobutton parameter for extensions.
 */

/*
 * Author:
 *   Johan Engelen <johan@shouraizou.nl>
 *
 * Copyright (C) 2006-2007 Johan Engelen
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm/widget.h>

#include "xml/document.h"
#include "extension-forward.h"

#include "parameter.h"

namespace Inkscape {
namespace Extension {



// \brief  A class to represent a radiobutton parameter of an extension
class ParamRadioButton : public Parameter {
private:
    /** \brief  Internal value.  This should point to a string that has
                been allocated in memory.  And should be free'd. 
                It is the value of the current selected string */
    gchar * _value;
    
    GSList * choices; /**< A table to store the choice strings  */
    
public:
    ParamRadioButton(const gchar * name, const gchar * guitext, const gchar * desc, const Parameter::_scope_t scope, Inkscape::Extension::Extension * ext, Inkscape::XML::Node * xml);
    ~ParamRadioButton(void);
    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal);
    Glib::ustring * string (void);
        
    const gchar * get (const SPDocument * doc, const Inkscape::XML::Node * node) { return _value; }
    const gchar * set (const gchar * in, SPDocument * doc, Inkscape::XML::Node * node);
}; /* class ParamRadioButton */





}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* __INK_EXTENSION_PARAMRADIOBUTTON_H__ */

