/*
    Author:  Ted Gould <ted@gould.cx>
    Copyright (c) 2003-2005

    This code is licensed under the GNU GPL.  See COPYING for details.
 
    This file is the backend to the extensions system.  These are
    the parts of the system that most users will never see, but are
    important for implementing the extensions themselves.  This file
    contains the base class for all of that.
*/
#ifndef __INKSCAPE_EXTENSION_IMPLEMENTATION_H__
#define __INKSCAPE_EXTENSION_IMPLEMENTATION_H__

#include <gtk/gtkdialog.h>
#include <gdkmm/types.h>
#include <gtkmm/widget.h>

#include "forward.h"
#include "extension/extension-forward.h"
#include "libnr/nr-forward.h"
#include "libnr/nr-point.h"

namespace Inkscape {
namespace Extension {
namespace Implementation {

/**
 * Base class for all implementations of modules.  This is whether they are done systematically by
 * having something like the scripting system, or they are implemented internally they all derive
 * from this class.
 */
class Implementation {
public:
    /* ----- Constructor / destructor ----- */
    Implementation() {}
    
    virtual ~Implementation() {}

    /* ----- Basic functions for all Extension ----- */
    virtual bool load(Inkscape::Extension::Extension *module);

    virtual void unload(Inkscape::Extension::Extension *module);

    /** Verify any dependencies. */
    virtual bool check(Inkscape::Extension::Extension *module);


    /* ----- Input functions ----- */
    /** Find out information about the file. */
    virtual Gtk::Widget *prefs_input(Inkscape::Extension::Input *module,
                             gchar const *filename);

    virtual SPDocument *open(Inkscape::Extension::Input *module,
                             gchar const *filename);

    /* ----- Output functions ----- */
    /** Find out information about the file. */
    virtual Gtk::Widget *prefs_output(Inkscape::Extension::Output *module);
    virtual void save(Inkscape::Extension::Output *module, SPDocument *doc, gchar const *filename);

    /* ----- Effect functions ----- */
    /** Find out information about the file. */
    virtual Gtk::Widget * prefs_effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View * view);
    /* TODO: need to figure out what we need here */

    virtual void effect(Inkscape::Extension::Effect *module,
                        Inkscape::UI::View::View *document);

    /* ----- Print functions ----- */
    virtual unsigned setup(Inkscape::Extension::Print *module);
    virtual unsigned set_preview(Inkscape::Extension::Print *module);

    virtual unsigned begin(Inkscape::Extension::Print *module,
                           SPDocument *doc);
    virtual unsigned finish(Inkscape::Extension::Print *module);
    virtual bool     textToPath(Inkscape::Extension::Print *ext);

    /* ----- Rendering methods ----- */
    virtual unsigned bind(Inkscape::Extension::Print *module,
                          NRMatrix const *transform,
                          float opacity);
    virtual unsigned release(Inkscape::Extension::Print *module);
    virtual unsigned comment(Inkscape::Extension::Print *module, const char * comment);
    virtual unsigned fill(Inkscape::Extension::Print *module,
                          NRBPath const *bpath,
                          NRMatrix const *ctm,
                          SPStyle const *style,
                          NRRect const *pbox,
                          NRRect const *dbox,
                          NRRect const *bbox);
    virtual unsigned stroke(Inkscape::Extension::Print *module,
                            NRBPath const *bpath,
                            NRMatrix const *transform,
                            SPStyle const *style,
                            NRRect const *pbox,
                            NRRect const *dbox,
                            NRRect const *bbox);
    virtual unsigned image(Inkscape::Extension::Print *module,
                           unsigned char *px,
                           unsigned int w,
                           unsigned int h,
                           unsigned int rs,
                           NRMatrix const *transform,
                           SPStyle const *style);
    virtual unsigned text(Inkscape::Extension::Print *module,
                          char const *text,
                          NR::Point p,
                          SPStyle const *style);
    virtual void     processPath(Inkscape::XML::Node * node);
};

}  /* namespace Implementation */
}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* __INKSCAPE_EXTENSION_IMPLEMENTATION_H__ */

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
