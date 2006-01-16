/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */



#ifndef INKSCAPE_EXTENSION_OUTPUT_H__
#define INKSCAPE_EXTENSION_OUTPUT_H__

#include <gtk/gtkdialog.h>
#include "extension.h"
struct SPDocument;

namespace Inkscape {
namespace Extension {

class Output : public Extension {
    gchar *mimetype;             /**< What is the mime type this inputs? */
    gchar *extension;            /**< The extension of the input files */
    gchar *filetypename;         /**< A userfriendly name for the file type */
    gchar *filetypetooltip;      /**< A more detailed description of the filetype */
    bool   dataloss;             /**< The extension causes data loss on save */

public:
    class save_failed {};        /**< Generic failure for an undescribed reason */
    class no_extension_found {}; /**< Failed because we couldn't find an extension to match the filename */

                 Output (Inkscape::XML::Node * in_repr,
                         Implementation::Implementation * in_imp);
    virtual     ~Output (void);
    virtual bool check                (void);
    void         save (SPDocument *doc,
                       gchar const *uri);
    bool         prefs (void);
    gchar *      get_mimetype(void);
    gchar *      get_extension(void);
    gchar *      get_filetypename(void);
    gchar *      get_filetypetooltip(void);
};

} }  /* namespace Inkscape, Extension */
#endif /* INKSCAPE_EXTENSION_OUTPUT_H__ */

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
