/*
 * This is where the implementation of the DBus based document API lives.
 * All the methods in here (except in the helper section) are 
 * designed to be called remotly via DBus. application-interface.cpp
 * has the methods used to connect to the bus and get a document instance.
 *
 * Documentation for these methods is in document-interface.xml
 * which is the "gold standard" as to how the interface should work.
 *
 * Authors:
 *   Soren Berg <Glimmer07@gmail.com>
 *
 * Copyright (C) 2009 Soren Berg
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "file.h" //IO
#include "document-interface.h"
#include "application-interface.h"
#include <string.h>
#include <dbus/dbus-glib.h>
#include "desktop.h"
#include "desktop-handles.h" //sp_desktop_document()
#include "desktop-style.h" //sp_desktop_get_style
#include "display/canvas-text.h" //text
#include "display/sp-canvas.h" //text
#include "document.h" // getReprDoc()
#include "document-undo.h"
#include "extension/output.h" //IO
#include "extension/system.h" //IO
#include "file.h" //IO
#include "helper/action.h" //sp_action_perform
#include "helper/action-context.h"
#include "inkscape.h" //inkscape_find_desktop_by_dkey, activate desktops
#include "layer-fns.h" //LPOS_BELOW
#include "layer-model.h"
#include "live_effects/parameter/text.h" //text
#include "print.h" //IO
#include "selection-chemistry.h"// lots of selection functions
#include "selection.h" //selection struct
#include "sp-ellipse.h"
#include "sp-object.h"
#include "sp-root.h"
#include "style.h" //style_write

#include "extension/system.h" //IO

#include "extension/output.h" //IO

#include "print.h" //IO

#include "live_effects/parameter/text.h" //text
#include "display/canvas-text.h" //text

#include "display/sp-canvas.h" //text
#include "text-editing.h"
#include "verbs.h"
#include "xml/repr.h" //sp_repr_document_new

//#include "2geom/svg-path-parser.h" //get_node_coordinates

#include <glib.h>
#include <dbus/dbus-glib.h>

#if 0
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#endif
 
 enum
 {
   OBJECT_MOVED_SIGNAL,
   LAST_SIGNAL
 };
 
 static guint signals[LAST_SIGNAL] = { 0 };


/****************************************************************************
     HELPER / SHORTCUT FUNCTIONS
****************************************************************************/

/* 
 * This function or the one below it translates the user input for an object
 * into Inkscapes internal representation.  It is called by almost every
 * method so it should be as fast as possible.
 *
 * (eg turns "rect2234" to an SPObject or Inkscape::XML::Node)
 *
 * If the internal representation changes (No more 'id' attributes) this is the
 * place to adjust things.
 */
Inkscape::XML::Node *
get_repr_by_name (SPDocument *doc, gchar *name, GError **error)
{
    /* ALTERNATIVE (is this faster if only repr is needed?)
    Inkscape::XML::Node *node = sp_repr_lookup_name((doc->root)->repr, name);
    */
  SPObject * obj = doc->getObjectById(name);
    if (!obj)
    {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OBJECT, "Object '%s' not found in document.", name);
        return NULL;
    }
    return  obj->getRepr();
}

/* 
 * See comment for get_repr_by_name, above.
 */
SPObject *
get_object_by_name (SPDocument *doc, gchar *name, GError **error)
{
    SPObject * obj = doc->getObjectById(name);
    if (!obj)
    {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OBJECT, "Object '%s' not found in document.", name);
        return NULL;
    }
    return obj;
}

/*
 * Tests for NULL strings and throws an appropriate error.
 * Every method that takes a string parameter (other than the 
 * name of an object, that's tested seperatly) should call this.
 */
gboolean
dbus_check_string (gchar *string, GError ** error, const gchar * errorstr)
{
    if (string == NULL)
    {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "%s", errorstr);
        return FALSE;
    }
    return TRUE;
}

/* 
 * This is used to return object values to the user
 */
const gchar *
get_name_from_object (SPObject * obj)
{
  return obj->getRepr()->attribute("id"); 
}

/*
 * Some verbs (cut, paste) only work on the active layer.
 * This makes sure that the document that is about to recive a command is active.
 */
void
desktop_ensure_active (SPDesktop* desk) {
    if (desk != SP_ACTIVE_DESKTOP)
        inkscape_activate_desktop (desk);
    return;
}

gdouble
selection_get_center_x (Inkscape::Selection *sel){
    Geom::OptRect box = sel->documentBounds(SPItem::GEOMETRIC_BBOX);
    return box ? box->midpoint()[Geom::X] : 0;
}

gdouble
selection_get_center_y (Inkscape::Selection *sel){
    Geom::OptRect box = sel->documentBounds(SPItem::GEOMETRIC_BBOX);
    return box ? box->midpoint()[Geom::X] : 0;
}

/* 
 * This function is used along with selection_restore to
 * take advantage of functionality provided by a selection
 * for a single object.
 *
 * It saves the current selection and sets the selection to 
 * the object specified.  Any selection verb can be used on the
 * object and then selection_restore is called, restoring the 
 * original selection.
 *
 * This should be mostly transparent to the user who need never
 * know we never bothered to implement it seperatly.  Although
 * they might see the selection box flicker if used in a loop.
 */
const GSList *
selection_swap(Inkscape::Selection *sel, gchar *name, GError **error)
{
    const GSList *oldsel = g_slist_copy((GSList *)sel->list());
    
    sel->set(get_object_by_name(sel->layerModel()->getDocument(), name, error));
    return oldsel;
}

/*
 * See selection_swap, above
 */
void
selection_restore(Inkscape::Selection *sel, const GSList * oldsel)
{
    sel->setList(oldsel);
}

/*
 * Shortcut for creating a Node.
 */
Inkscape::XML::Node *
dbus_create_node (SPDocument *doc, const gchar *type)
{
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    return xml_doc->createElement(type);
}

/*
 * Called by the shape creation functions.  Gets the default style for the doc
 * or sets it arbitrarily if none.
 *
 * There is probably a better way to do this (use the shape tools default styles)
 * but I'm not sure how.
 */
gchar *finish_create_shape (DocumentInterface *object, GError ** /*error*/, Inkscape::XML::Node *newNode, gchar *desc)
{
    SPCSSAttr *style = NULL;
    if (object->context.getDesktop()) {
        style = sp_desktop_get_style(object->context.getDesktop(), TRUE);
    }
    if (style) {
        Glib::ustring str;
        sp_repr_css_write_string(style, str);
        newNode->setAttribute("style", str.c_str(), TRUE);
    }
    else {
        newNode->setAttribute("style", "fill:#0000ff;fill-opacity:1;stroke:#c900b9;stroke-width:0;stroke-miterlimit:0;stroke-opacity:1;stroke-dasharray:none", TRUE);
    }

    object->context.getSelection()->layerModel()->currentLayer()->appendChildRepr(newNode);
    object->context.getSelection()->layerModel()->currentLayer()->updateRepr();

    if (object->updates) {
        Inkscape::DocumentUndo::done(object->context.getDocument(),  0, (gchar *)desc);
    //} else {
        //document_interface_pause_updates(object, error);
    }

    return strdup(newNode->attribute("id"));
}

/*
 * This is the code used internally to call all the verbs.
 *
 * It handles error reporting and update pausing (which needs some work.)
 * This is a good place to improve efficiency as it is called a lot.
 *
 * document_interface_call_verb is similar but is called by the user.
 */
gboolean
dbus_call_verb (DocumentInterface *object, int verbid, GError **error)
{    
    SPDesktop *desk = object->context.getDesktop();    
    if ( desk ) {
        desktop_ensure_active (desk);
    }
    Inkscape::Verb *verb = Inkscape::Verb::get( verbid );
    if ( verb ) {
        SPAction *action = verb->get_action(object->context);
        if ( action ) {
            //if (!object->updates)
                //document_interface_pause_updates (object, error);
            sp_action_perform( action, NULL );
            if (object->updates)
                Inkscape::DocumentUndo::done(object->context.getDocument(),  verb->get_code(), g_strdup(verb->get_tip()));
            //if (!object->updates)
                //document_interface_pause_updates (object, error);
            return TRUE;
        }
    }
    g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_VERB, "Verb failed to execute");
    return FALSE;
}

/****************************************************************************
     DOCUMENT INTERFACE CLASS STUFF
****************************************************************************/

G_DEFINE_TYPE(DocumentInterface, document_interface, G_TYPE_OBJECT)

static void
document_interface_finalize (GObject *object)
{
        G_OBJECT_CLASS (document_interface_parent_class)->finalize (object);
}


static void
document_interface_class_init (DocumentInterfaceClass *klass)
{
        GObjectClass *object_class;
        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = document_interface_finalize;
        signals[OBJECT_MOVED_SIGNAL] =
        g_signal_new ("object_moved",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);        
}

static void
document_interface_init (DocumentInterface *object)
{
    object->context = Inkscape::ActionContext();
}


DocumentInterface *
document_interface_new (void)
{
        return (DocumentInterface*)g_object_new (TYPE_DOCUMENT_INTERFACE, NULL);
}



/****************************************************************************
     MISC FUNCTIONS
****************************************************************************/

gboolean document_interface_delete_all(DocumentInterface *object, GError ** /*error*/)
{
    sp_edit_clear_all(object->context.getSelection());
    return TRUE;
}

gboolean
document_interface_call_verb (DocumentInterface *object, gchar *verbid, GError **error)
{
    SPDesktop *desk = object->context.getDesktop();
    if ( desk ) {
        desktop_ensure_active (desk);
    }
    Inkscape::Verb *verb = Inkscape::Verb::getbyid( verbid );
    if ( verb ) {
        SPAction *action = verb->get_action(object->context);
        if ( action ) {
            sp_action_perform( action, NULL );
            if (object->updates) {
                Inkscape::DocumentUndo::done(object->context.getDocument(),  verb->get_code(), g_strdup(verb->get_tip()));
            }
            return TRUE;
        }
    }
    g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_VERB, "Verb '%s' failed to execute or was not found.", verbid);
    return FALSE;
}


/****************************************************************************
     CREATION FUNCTIONS
****************************************************************************/

gchar* 
document_interface_rectangle (DocumentInterface *object, int x, int y, 
                              int width, int height, GError **error)
{


    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:rect");
    sp_repr_set_int(newNode, "x", x);  //could also use newNode->setAttribute()
    sp_repr_set_int(newNode, "y", y);
    sp_repr_set_int(newNode, "width", width);
    sp_repr_set_int(newNode, "height", height);
    return finish_create_shape (object, error, newNode, (gchar *)"create rectangle");
}

gchar*
document_interface_ellipse_center (DocumentInterface *object, int cx, int cy, 
                                   int rx, int ry, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:path");
    newNode->setAttribute("sodipodi:type", "arc");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:rx", rx);
    sp_repr_set_int(newNode, "sodipodi:ry", ry);
    return finish_create_shape (object, error, newNode, (gchar *)"create circle");
}

gchar* 
document_interface_polygon (DocumentInterface *object, int cx, int cy, 
                            int radius, int rotation, int sides, 
                            GError **error)
{
    gdouble rot = ((rotation / 180.0) * 3.14159265) - ( 3.14159265 / 2.0);
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:path");
    newNode->setAttribute("inkscape:flatsided", "true");
    newNode->setAttribute("sodipodi:type", "star");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:r1", radius);
    sp_repr_set_int(newNode, "sodipodi:r2", radius);
    sp_repr_set_int(newNode, "sodipodi:sides", sides);
    sp_repr_set_int(newNode, "inkscape:randomized", 0);
    sp_repr_set_svg_double(newNode, "sodipodi:arg1", rot);
    sp_repr_set_svg_double(newNode, "sodipodi:arg2", rot);
    sp_repr_set_svg_double(newNode, "inkscape:rounded", 0);

    return finish_create_shape (object, error, newNode, (gchar *)"create polygon");
}

gchar* 
document_interface_star (DocumentInterface *object, int cx, int cy, 
                         int r1, int r2, int sides, gdouble rounded,
                         gdouble arg1, gdouble arg2, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:path");
    newNode->setAttribute("inkscape:flatsided", "false");
    newNode->setAttribute("sodipodi:type", "star");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:r1", r1);
    sp_repr_set_int(newNode, "sodipodi:r2", r2);
    sp_repr_set_int(newNode, "sodipodi:sides", sides);
    sp_repr_set_int(newNode, "inkscape:randomized", 0);
    sp_repr_set_svg_double(newNode, "sodipodi:arg1", arg1);
    sp_repr_set_svg_double(newNode, "sodipodi:arg2", arg2);
    sp_repr_set_svg_double(newNode, "inkscape:rounded", rounded);

    return finish_create_shape (object, error, newNode, (gchar *)"create star");
}

gchar* 
document_interface_ellipse (DocumentInterface *object, int x, int y, 
                            int width, int height, GError **error)
{
    int rx = width/2;
    int ry = height/2;
    return document_interface_ellipse_center (object, x+rx, y+ry, rx, ry, error);
}

gchar* 
document_interface_line (DocumentInterface *object, int x, int y, 
                              int x2, int y2, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:path");
    std::stringstream out;
    // Not sure why this works.
	out << "m " << x << "," << y << " " << x2 - x << "," << y2 - y;
    newNode->setAttribute("d", out.str().c_str());
    return finish_create_shape (object, error, newNode, (gchar *)"create line");
}

gchar* 
document_interface_spiral (DocumentInterface *object, int cx, int cy, 
                           int r, int revolutions, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:path");
    newNode->setAttribute("sodipodi:type", "spiral");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:radius", r);
    sp_repr_set_int(newNode, "sodipodi:revolution", revolutions);
    sp_repr_set_int(newNode, "sodipodi:t0", 0);
    sp_repr_set_int(newNode, "sodipodi:argument", 0);
    sp_repr_set_int(newNode, "sodipodi:expansion", 1);
    gchar * retval = finish_create_shape (object, error, newNode, (gchar *)"create spiral");
    //Makes sure there is no fill for spirals by default.
    gchar* newString = g_strconcat(newNode->attribute("style"), ";fill:none", NULL);
    newNode->setAttribute("style", newString);
    g_free(newString);
    return retval;
}

gchar*
document_interface_text (DocumentInterface *object, int x, int y, gchar *text, GError **error)
{

  Inkscape::XML::Node *text_node = dbus_create_node(object->context.getDocument(), "svg:text");
    sp_repr_set_int(text_node, "x", x);
    sp_repr_set_int(text_node, "y", y);
    //just a workaround so i can get an spitem from the name
    gchar  *name = finish_create_shape (object, error, text_node, (gchar *)"create text");
    
    SPItem* text_obj=(SPItem* )get_object_by_name(object->context.getDocument(), name, error);
    sp_te_set_repr_text_multiline(text_obj, text);

    return name;
}

gchar *
document_interface_image (DocumentInterface *object, int x, int y, gchar *filename, GError **error)
{
    gchar * uri = g_filename_to_uri (filename, FALSE, error);
    if (!uri)
        return FALSE;
    
    Inkscape::XML::Node *newNode = dbus_create_node(object->context.getDocument(), "svg:image");
    sp_repr_set_int(newNode, "x", x);
    sp_repr_set_int(newNode, "y", y);
    newNode->setAttribute("xlink:href", uri);
    
    object->context.getSelection()->layerModel()->currentLayer()->appendChildRepr(newNode);
    object->context.getSelection()->layerModel()->currentLayer()->updateRepr();

    if (object->updates)
        Inkscape::DocumentUndo::done(object->context.getDocument(),  0, "Imported bitmap.");

    //g_free(uri);
    return strdup(newNode->attribute("id"));
}

gchar *document_interface_node(DocumentInterface *object, gchar *type, GError ** /*error*/)
{
    SPDocument * doc = object->context.getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    Inkscape::XML::Node *newNode =  xml_doc->createElement(type);

    object->context.getSelection()->layerModel()->currentLayer()->appendChildRepr(newNode);
    object->context.getSelection()->layerModel()->currentLayer()->updateRepr();

    if (object->updates) {
        Inkscape::DocumentUndo::done(doc, 0, (gchar *)"created empty node");
    //} else {
        //document_interface_pause_updates(object, error);
    }

    return strdup(newNode->attribute("id"));
}

/****************************************************************************
     ENVIRONMENT FUNCTIONS
****************************************************************************/
gdouble
document_interface_document_get_width (DocumentInterface *object)
{
  return object->context.getDocument()->getWidth();
}

gdouble
document_interface_document_get_height (DocumentInterface *object)
{
  return object->context.getDocument()->getHeight();
}

gchar *document_interface_document_get_css(DocumentInterface *object, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document get CSS requires a GUI");
        return NULL;
    }
    SPCSSAttr *current = desk->current;
    Glib::ustring str;
    sp_repr_css_write_string(current, str);
    return (str.empty() ? NULL : g_strdup (str.c_str()));
}

gboolean document_interface_document_merge_css(DocumentInterface *object,
                                               gchar *stylestring, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document merge CSS requires a GUI");
        return FALSE;
    }
    SPCSSAttr * style = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(style, stylestring);
    sp_desktop_set_style(desk, style);
    return TRUE;
}

gboolean document_interface_document_set_css(DocumentInterface *object,
                                             gchar *stylestring, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document set CSS requires a GUI");
        return FALSE;
    }
    SPCSSAttr * style = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string (style, stylestring);
    //Memory leak?
    desk->current = style;
    return TRUE;
}

gboolean 
document_interface_document_resize_to_fit_selection (DocumentInterface *object,
                                                     GError **error)
{
    return dbus_call_verb (object, SP_VERB_FIT_CANVAS_TO_SELECTION, error);
}

gboolean
document_interface_document_set_display_area (DocumentInterface *object,
                                              double x0,
                                              double y0,
                                              double x1,
                                              double y1,
                                              double border,
                                              GError **error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document set display area requires a GUI");
        return FALSE;
    }
    desk->set_display_area (x0,
                    y0,
                    x1,
                    y1,
                    border, false);
    return TRUE;
}


GArray *
document_interface_document_get_display_area (DocumentInterface *object)
{
  SPDesktop *desk = object->context.getDesktop();
  if (!desk) {
      return NULL;
  }
  Geom::Rect const d = desk->get_display_area();
  
  GArray * dArr = g_array_new (TRUE, TRUE, sizeof(double));

  double x0 = d.min()[Geom::X];
  double y0 = d.min()[Geom::Y];
  double x1 = d.max()[Geom::X];
  double y1 = d.max()[Geom::Y];
  g_array_append_val (dArr, x0); //
  g_array_append_val (dArr, y0);
  g_array_append_val (dArr, x1);
  g_array_append_val (dArr, y1);
  return dArr;

}


/****************************************************************************
     OBJECT FUNCTIONS
****************************************************************************/

gboolean
document_interface_set_attribute (DocumentInterface *object, char *shape, 
                                  char *attribute, char *newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_repr_by_name(object->context.getDocument(), shape, error);

    /* ALTERNATIVE (is this faster?)
    Inkscape::XML::Node *newnode = sp_repr_lookup_name((doc->root)->repr, name);
    */
    if (!dbus_check_string(newval, error, "New value string was empty."))
        return FALSE;
        
    if (!newNode)
        return FALSE;
        
    newNode->setAttribute(attribute, newval, TRUE);
    return TRUE;
}

gboolean 
document_interface_set_int_attribute (DocumentInterface *object, 
                                      char *shape, char *attribute, 
                                      int newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_repr_by_name (object->context.getDocument(), shape, error);
    if (!newNode)
        return FALSE;
        
    sp_repr_set_int (newNode, attribute, newval);
    return TRUE;
}


gboolean
document_interface_set_double_attribute (DocumentInterface *object, 
                                         char *shape, char *attribute, 
                                         double newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_repr_by_name (object->context.getDocument(), shape, error);
    
    if (!dbus_check_string (attribute, error, "New value string was empty."))
        return FALSE;
    if (!newNode)
        return FALSE;
    
    sp_repr_set_svg_double (newNode, attribute, newval);
    return TRUE;
}

gchar *
document_interface_get_attribute (DocumentInterface *object, char *shape, 
                                  char *attribute, GError **error)
{
    Inkscape::XML::Node *newNode = get_repr_by_name(object->context.getDocument(), shape, error);

    if (!dbus_check_string (attribute, error, "Attribute name empty."))
        return NULL;
    if (!newNode)
        return NULL;
        
    return g_strdup(newNode->attribute(attribute));
}

gboolean
document_interface_move (DocumentInterface *object, gchar *name, gdouble x, 
                         gdouble y, GError **error)
{
    const GSList *oldsel = selection_swap(object->context.getSelection(), name, error);
    if (!oldsel)
        return FALSE;
    sp_selection_move (object->context.getSelection(), x, 0 - y);
    selection_restore(object->context.getSelection(), oldsel);
    return TRUE;
}

gboolean
document_interface_move_to (DocumentInterface *object, gchar *name, gdouble x, 
                         gdouble y, GError **error)
{
    const GSList *oldsel = selection_swap(object->context.getSelection(), name, error);
    if (!oldsel)
        return FALSE;
    Inkscape::Selection * sel = object->context.getSelection();
    sp_selection_move (object->context.getSelection(), x - selection_get_center_x(sel),
                                     0 - (y - selection_get_center_y(sel)));
    selection_restore(object->context.getSelection(), oldsel);
    return TRUE;
}

gboolean
document_interface_object_to_path (DocumentInterface *object, 
                                   char *shape, GError **error)
{
    const GSList *oldsel = selection_swap(object->context.getSelection(), shape, error);
    if (!oldsel)
        return FALSE;
    dbus_call_verb (object, SP_VERB_OBJECT_TO_CURVE, error);
    selection_restore(object->context.getSelection(), oldsel);
    return TRUE;
}

gchar *
document_interface_get_path (DocumentInterface *object, char *pathname, GError **error)
{
    Inkscape::XML::Node *node = get_repr_by_name(object->context.getDocument(), pathname, error);
    
    if (!node)
        return NULL;
        
    if (node->attribute("d") == NULL)
    {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OBJECT, "Object is not a path.");
        return NULL;
    }
    return strdup(node->attribute("d"));
}

gboolean 
document_interface_transform (DocumentInterface *object, gchar *shape,
                              gchar *transformstr, GError **error)
{
    //FIXME: This should merge transformations.
    gchar trans[] = "transform";
    document_interface_set_attribute (object, shape, trans, transformstr, error);
    return TRUE;
}

gchar *
document_interface_get_css (DocumentInterface *object, gchar *shape,
                            GError **error)
{
    gchar style[] = "style";
    return document_interface_get_attribute (object, shape, style, error);
}

gboolean 
document_interface_modify_css (DocumentInterface *object, gchar *shape,
                               gchar *cssattrb, gchar *newval, GError **error)
{
    // Doesn't like non-variable strings for some reason.
    gchar style[] = "style";
    Inkscape::XML::Node *node = get_repr_by_name(object->context.getDocument(), shape, error);
    
    if (!dbus_check_string (cssattrb, error, "Attribute string empty."))
        return FALSE;
    if (!node)
        return FALSE;
        
    SPCSSAttr * oldstyle = sp_repr_css_attr (node, style);
    sp_repr_css_set_property(oldstyle, cssattrb, newval);
    Glib::ustring str;
    sp_repr_css_write_string (oldstyle, str);
    node->setAttribute (style, str.c_str(), TRUE);
    return TRUE;
}

gboolean 
document_interface_merge_css (DocumentInterface *object, gchar *shape,
                               gchar *stylestring, GError **error)
{
    gchar style[] = "style";
    
    Inkscape::XML::Node *node = get_repr_by_name(object->context.getDocument(), shape, error);
    
    if (!dbus_check_string (stylestring, error, "Style string empty."))
        return FALSE;
    if (!node)
        return FALSE;
        
    SPCSSAttr * newstyle = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string (newstyle, stylestring);

    SPCSSAttr * oldstyle = sp_repr_css_attr (node, style);

    sp_repr_css_merge(oldstyle, newstyle);
    Glib::ustring str;
    sp_repr_css_write_string (oldstyle, str);
    node->setAttribute (style, str.c_str(), TRUE);
    
    return TRUE;
}

gboolean 
document_interface_set_color (DocumentInterface *object, gchar *shape,
                              int r, int g, int b, gboolean fill, GError **error)
{
    gchar style[15];
    if (r<0 || r>255 || g<0 || g>255 || b<0 || b>255)
    {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Given (%d,%d,%d).  All values must be between 0-255 inclusive.", r, g, b);
        return FALSE;
    }
    
    if (fill)
        snprintf(style, 15, "fill:#%.2x%.2x%.2x", r, g, b);
    else
        snprintf(style, 15, "stroke:#%.2x%.2x%.2x", r, g, b);
    
    if (strcmp(shape, "document") == 0)
        return document_interface_document_merge_css (object, style, error);
    
    return document_interface_merge_css (object, shape, style, error);
}

gboolean 
document_interface_move_to_layer (DocumentInterface *object, gchar *shape, 
                              gchar *layerstr, GError **error)
{
    const GSList *oldsel = selection_swap(object->context.getSelection(), shape, error);
    if (!oldsel)
        return FALSE;
        
    document_interface_selection_move_to_layer(object, layerstr, error);
    selection_restore(object->context.getSelection(), oldsel);
    return TRUE;
}

GArray *document_interface_get_node_coordinates(DocumentInterface * /*object*/, gchar * /*shape*/)
{
    //FIXME: Needs lot's of work.
/*
    Inkscape::XML::Node *shapenode = get_repr_by_name (object->context.getDocument(), shape, error);
    if (shapenode == NULL || shapenode->attribute("d") == NULL) {
        return FALSE;
    }
    char * path = strdup(shapenode->attribute("d"));
    printf("PATH: %s\n", path);
    
    Geom::parse_svg_path (path);
    return NULL;
    */
    return NULL;
}


gboolean
document_interface_set_text (DocumentInterface *object, gchar *name, gchar *text, GError **error)
{

  SPItem* text_obj=(SPItem* )get_object_by_name(object->context.getDocument(), name, error);
  //TODO verify object type
  if (!text_obj)
    return FALSE;
  sp_te_set_repr_text_multiline(text_obj, text);
  return TRUE;
      
}



gboolean
document_interface_text_apply_style (DocumentInterface *object, gchar *name,
                                     int start_pos, int end_pos,  gchar *style, gchar *styleval,
                                     GError **error)
{

  SPItem* text_obj=(SPItem* )get_object_by_name(object->context.getDocument(), name, error);

  //void sp_te_apply_style(SPItem *text, Inkscape::Text::Layout::iterator const &start, Inkscape::Text::Layout::iterator const &end, SPCSSAttr const *css)
  //TODO verify object type
  if (!text_obj)
    return FALSE;
  Inkscape::Text::Layout const *layout = te_get_layout(text_obj);
  Inkscape::Text::Layout::iterator start = layout->charIndexToIterator (start_pos);
  Inkscape::Text::Layout::iterator end = layout->charIndexToIterator (end_pos);

  SPCSSAttr *css = sp_repr_css_attr_new();
  sp_repr_css_set_property(css, style, styleval);
    
  sp_te_apply_style(text_obj,
                    start,
                    end,
                    css);
  return TRUE;
      
}


/****************************************************************************
     FILE I/O FUNCTIONS
****************************************************************************/

gboolean 
document_interface_save (DocumentInterface *object, GError **error)
{
    SPDocument * doc = object->context.getDocument();
    printf("1:  %s\n2:  %s\n3:  %s\n", doc->getURI(), doc->getBase(), doc->getName());
    if (doc->getURI())
      return document_interface_save_as (object, doc->getURI(), error);
    return FALSE;
}

gboolean document_interface_load(DocumentInterface *object, 
                                 gchar *filename, GError ** /*error*/)
{
    SPDesktop *desk = object->context.getDesktop();
    if (desk) {
        desktop_ensure_active(desk);
    }
    const Glib::ustring file(filename);
    sp_file_open(file, NULL, TRUE, TRUE);
    if (object->updates) {
        Inkscape::DocumentUndo::done(object->context.getDocument(),  SP_VERB_FILE_OPEN, "Opened File");
    }
    return TRUE;
}

gchar *
document_interface_import (DocumentInterface *object, 
                           gchar *filename, GError **error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (desk) {
        desktop_ensure_active(desk);
    }
    const Glib::ustring file(filename);
    SPDocument * doc = object->context.getDocument();

    SPObject *new_obj = NULL;
    new_obj = file_import(doc, file, NULL);
    return strdup(new_obj->getRepr()->attribute("id"));
}

gboolean 
document_interface_save_as (DocumentInterface *object, 
                           const gchar *filename, GError **error)
{
    SPDocument * doc = object->context.getDocument();
    #ifdef WITH_GNOME_VFS
    const Glib::ustring file(filename);
    return file_save_remote(doc, file, NULL, TRUE, TRUE);
    #endif
    if (!doc || strlen(filename)<1) { //Safety check
        return false;
    }

    try {
        Inkscape::Extension::save(NULL, doc, filename,
                 false, false, true, Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
    } catch (...) {
        //SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        return false;
    }

    //SP_ACTIVE_DESKTOP->event_log->rememberFileSave();
    //SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::NORMAL_MESSAGE, "Document saved.");
    return true;
}

gboolean document_interface_mark_as_unmodified(DocumentInterface *object, GError ** /*error*/)
{
    SPDocument * doc = object->context.getDocument();
    if (doc) {
        doc->modified_since_save = FALSE;
    }
    return TRUE;
}

/*
gboolean 
document_interface_print_to_file (DocumentInterface *object, GError **error)
{
    SPDocument * doc = object->context.getDocument();
    sp_print_document_to_file (doc, g_strdup("/home/soren/test.pdf"));
                               
    return TRUE;
}
*/
/****************************************************************************
     PROGRAM CONTROL FUNCTIONS
****************************************************************************/

gboolean
document_interface_close (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_FILE_CLOSE_VIEW, error);
}

gboolean
document_interface_exit (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_FILE_QUIT, error);
}

gboolean
document_interface_undo (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_EDIT_UNDO, error);
}

gboolean
document_interface_redo (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_EDIT_REDO, error);
}



/****************************************************************************
     UPDATE FUNCTIONS 
     FIXME: This would work better by adding a flag to SPDesktop to prevent
     updating but that would be very intrusive so for now there is a workaround.
     Need to make sure it plays well with verbs because they are used so much.
****************************************************************************/

void document_interface_pause_updates(DocumentInterface *object, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document pause updates requires a GUI");
        return;
    }
    object->updates = FALSE;
    desk->canvas->drawing_disabled = 1;
    //desk->canvas->need_redraw = 0;
    //desk->canvas->need_repick = 0;
    //object->context.getDocument()->root->uflags = FALSE;
    //object->context.getDocument()->root->mflags = FALSE;
}

void document_interface_resume_updates(DocumentInterface *object, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document resume updates requires a GUI");
        return;
    }
    object->updates = TRUE;
    desk->canvas->drawing_disabled = 0;
    //desk->canvas->need_redraw = 1;
    //desk->canvas->need_repick = 1;
    //object->context.getDocument()->root->uflags = TRUE;
    //object->context.getDocument()->root->mflags = TRUE;
    //object->context.getDocument()->_updateDocument();
    //FIXME: use better verb than rect.
    Inkscape::DocumentUndo::done(object->context.getDocument(),  SP_VERB_CONTEXT_RECT, "Multiple actions");
}

void document_interface_update(DocumentInterface *object, GError ** error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document update requires a GUI");
        return;
    }
    SPDocument *doc = object->context.getDocument();
    doc->getRoot()->uflags = TRUE;
    doc->getRoot()->mflags = TRUE;
    desk->enableInteraction();
    doc->_updateDocument();
    desk->disableInteraction();
    doc->getRoot()->uflags = FALSE;
    doc->getRoot()->mflags = FALSE;
    //Inkscape::DocumentUndo::done(doc, SP_VERB_CONTEXT_RECT, "Multiple actions");
}

/****************************************************************************
     SELECTION FUNCTIONS FIXME: use call_verb where appropriate (once update system is tested.)
****************************************************************************/

gboolean document_interface_selection_get(DocumentInterface *object, char ***out, GError ** /*error*/)
{
    Inkscape::Selection * sel = object->context.getSelection();
    GSList const *oldsel = sel->list();

    int size = g_slist_length((GSList *) oldsel);

    *out = g_new0 (char *, size + 1);

    int i = 0;
    for (GSList const *iter = oldsel; iter != NULL; iter = iter->next) {
      (*out)[i] = g_strdup(SP_OBJECT(iter->data)->getRepr()->attribute("id"));
        i++;
    }
    (*out)[i] = NULL;

    return TRUE;
}

gboolean
document_interface_selection_add (DocumentInterface *object, char *name, GError **error)
{
    SPObject * obj = get_object_by_name(object->context.getDocument(), name, error);
    if (!obj)
        return FALSE;
    
    Inkscape::Selection *selection = object->context.getSelection();

    selection->add(obj);
    return TRUE;
}

gboolean
document_interface_selection_add_list (DocumentInterface *object, 
                                       char **names, GError **error)
{
    int i;
    for (i=0;names[i] != NULL;i++) {
        document_interface_selection_add(object, names[i], error);       
    }
    return TRUE;
}

gboolean document_interface_selection_set(DocumentInterface *object, char *name, GError ** /*error*/)
{
    SPDocument * doc = object->context.getDocument();
    Inkscape::Selection *selection = object->context.getSelection();
    selection->set(doc->getObjectById(name));
    return TRUE;
}

gboolean
document_interface_selection_set_list (DocumentInterface *object, 
                                       gchar **names, GError **error)
{
    object->context.getSelection()->clear();
    int i;
    for (i=0;names[i] != NULL;i++) {
        document_interface_selection_add(object, names[i], error);       
    }
    return TRUE;
}

gboolean document_interface_selection_rotate(DocumentInterface *object, int angle, GError ** /*error*/)
{
    Inkscape::Selection *selection = object->context.getSelection();
    sp_selection_rotate(selection, angle);
    return TRUE;
}

gboolean
document_interface_selection_delete (DocumentInterface *object, GError **error)
{
    //sp_selection_delete (object->desk);
    return dbus_call_verb (object, SP_VERB_EDIT_DELETE, error);
}

gboolean document_interface_selection_clear(DocumentInterface *object, GError ** /*error*/)
{
    object->context.getSelection()->clear();
    return TRUE;
}

gboolean
document_interface_select_all (DocumentInterface *object, GError **error)
{
    //sp_edit_select_all (object->desk);
    return dbus_call_verb (object, SP_VERB_EDIT_SELECT_ALL, error);
}

gboolean
document_interface_select_all_in_all_layers(DocumentInterface *object, 
                                            GError **error)
{
    //sp_edit_select_all_in_all_layers (object->desk);
    return dbus_call_verb (object, SP_VERB_EDIT_SELECT_ALL_IN_ALL_LAYERS, error);
}

gboolean document_interface_selection_box(DocumentInterface * /*object*/, int /*x*/, int /*y*/,
                                          int /*x2*/, int /*y2*/, gboolean /*replace*/,
                                          GError ** /*error*/)
{
    //FIXME: implement.
    return FALSE;
}

gboolean
document_interface_selection_invert (DocumentInterface *object, GError **error)
{
    //sp_edit_invert (object->desk);
    return dbus_call_verb (object, SP_VERB_EDIT_INVERT, error);
}

gboolean
document_interface_selection_group (DocumentInterface *object, GError **error)
{
    //sp_selection_group (object->desk);
    return dbus_call_verb (object, SP_VERB_SELECTION_GROUP, error);
}
gboolean
document_interface_selection_ungroup (DocumentInterface *object, GError **error)
{
    //sp_selection_ungroup (object->desk);
    return dbus_call_verb (object, SP_VERB_SELECTION_UNGROUP, error);
}
 
gboolean
document_interface_selection_cut (DocumentInterface *object, GError **error)
{
    //desktop_ensure_active (object->desk);
    //sp_selection_cut (object->desk);
    return dbus_call_verb (object, SP_VERB_EDIT_CUT, error);
}

gboolean
document_interface_selection_copy (DocumentInterface *object, GError **error)
{
    //desktop_ensure_active (object->desk);
    //sp_selection_copy ();
    return dbus_call_verb (object, SP_VERB_EDIT_COPY, error);
}
/*
gboolean
document_interface_selection_paste (DocumentInterface *object, GError **error)
{
    SPDesktop *desk = object->context.getDesktop();
    if (!desk) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document selection paste requires a GUI");
        return FALSE;
    }
    desktop_ensure_active (desk);
                    if (!object->updates)
                    document_interface_pause_updates (object, error);
    sp_selection_paste (object->desk, TRUE);
                    if (!object->updates)
                    document_interface_pause_updates (object, error);
    return TRUE;
    //return dbus_call_verb (object, SP_VERB_EDIT_PASTE, error);
}
*/
gboolean
document_interface_selection_paste (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_EDIT_PASTE, error);
}

gboolean document_interface_selection_scale(DocumentInterface *object, gdouble grow, GError ** /*error*/)
{
    Inkscape::Selection *selection = object->context.getSelection();
    if (!selection)
    {
        return FALSE;
    }     
    sp_selection_scale (selection, grow);
    return TRUE;
}

gboolean document_interface_selection_move(DocumentInterface *object, gdouble x, gdouble y, GError ** /*error*/)
{
    sp_selection_move(object->context.getSelection(), x, 0 - y); //switching coordinate systems.
    return TRUE;
}

gboolean document_interface_selection_move_to(DocumentInterface *object, gdouble x, gdouble y, GError ** /*error*/)
{
    Inkscape::Selection * sel = object->context.getSelection();

    Geom::OptRect sel_bbox = sel->visualBounds();
    if (sel_bbox) {
        Geom::Point m( x - selection_get_center_x(sel) , 0 - (y - selection_get_center_y(sel)) );
        sp_selection_move_relative(sel, m, true);
    }
    return TRUE;
}

//FIXME: does not paste in new layer.
// This needs to use lower level cut_impl and paste_impl (messy)
// See the built-in sp_selection_to_next_layer and duplicate.
gboolean 
document_interface_selection_move_to_layer (DocumentInterface *object,
                                            gchar *layerstr, GError **error)
{
    SPDesktop *dt = object->context.getDesktop();
    if (!dt) {
        g_set_error(error, INKSCAPE_ERROR, INKSCAPE_ERROR_OTHER, "Document selection move to layer requires a GUI");
        return FALSE;
    }

    Inkscape::Selection *selection = object->context.getSelection();

    // check if something is selected
    if (selection->isEmpty())
        return FALSE;

    SPObject *next = get_object_by_name(object->context.getDocument(), layerstr, error);
    
    if (!next)
        return FALSE;

    if (strcmp("layer", (next->getRepr())->attribute("inkscape:groupmode")) == 0) {

        sp_selection_cut(dt);

        object->context.getSelection()->layerModel()->setCurrentLayer(next);

        sp_selection_paste(dt, TRUE);
        }
    return TRUE;
}

GArray *
document_interface_selection_get_center (DocumentInterface *object)
{
    Inkscape::Selection * sel = object->context.getSelection();

    if (sel) 
    {
        gdouble x = selection_get_center_x(sel);
        gdouble y = selection_get_center_y(sel);
        GArray * intArr = g_array_new (TRUE, TRUE, sizeof(double));

        g_array_append_val (intArr, x);
        g_array_append_val (intArr, y);
        return intArr;
    }

    return NULL;
}

gboolean 
document_interface_selection_to_path (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_OBJECT_TO_CURVE, error);    
}


gchar *
document_interface_selection_combine (DocumentInterface *object, gchar *cmd,
                                      GError **error)
{
    if (strcmp(cmd, "union") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_UNION, error);
    else if (strcmp(cmd, "intersection") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_INTERSECT, error);
    else if (strcmp(cmd, "difference") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_DIFF, error);
    else if (strcmp(cmd, "exclusion") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_SYMDIFF, error);
    else
        return NULL;

    if (object->context.getSelection()->singleRepr() != NULL)
        return g_strdup(object->context.getSelection()->singleRepr()->attribute("id"));
    return NULL;
}

gboolean
document_interface_selection_divide (DocumentInterface *object, char ***out, GError **error)
{
    dbus_call_verb (object, SP_VERB_SELECTION_CUT, error);

    return document_interface_selection_get (object, out, error);
}

gboolean
document_interface_selection_change_level (DocumentInterface *object, gchar *cmd,
                                      GError **error)
{
    if (strcmp(cmd, "raise") == 0)
        return dbus_call_verb (object, SP_VERB_SELECTION_RAISE, error);
    if (strcmp(cmd, "lower") == 0)
        return dbus_call_verb (object, SP_VERB_SELECTION_LOWER, error);
    if ((strcmp(cmd, "to_top") == 0) || (strcmp(cmd, "to_front") == 0))
        return dbus_call_verb (object, SP_VERB_SELECTION_TO_FRONT, error);
    if ((strcmp(cmd, "to_bottom") == 0) || (strcmp(cmd, "to_back") == 0))
        return dbus_call_verb (object, SP_VERB_SELECTION_TO_BACK, error);
    return TRUE;
}

/****************************************************************************
     LAYER FUNCTIONS
****************************************************************************/

gchar *document_interface_layer_new(DocumentInterface *object, GError ** /*error*/)
{
    Inkscape::LayerModel * layerModel = object->context.getSelection()->layerModel();
    SPObject *new_layer = Inkscape::create_layer(layerModel->currentRoot(), layerModel->currentLayer(), Inkscape::LPOS_BELOW);
    layerModel->setCurrentLayer(new_layer);
    return g_strdup(get_name_from_object(new_layer));
}

gboolean 
document_interface_layer_set (DocumentInterface *object,
                              gchar *layerstr, GError **error)
{
    SPObject * obj = get_object_by_name (object->context.getDocument(), layerstr, error);
    
    if (!obj)
        return FALSE;
        
    object->context.getSelection()->layerModel()->setCurrentLayer (obj);
    return TRUE;
}

gchar **document_interface_layer_get_all(DocumentInterface * /*object*/)
{
    //FIXME: implement.
    return NULL;
}

gboolean 
document_interface_layer_change_level (DocumentInterface *object,
                                       gchar *cmd, GError **error)
{
    if (strcmp(cmd, "raise") == 0)
        return dbus_call_verb (object, SP_VERB_LAYER_RAISE, error);
    if (strcmp(cmd, "lower") == 0)
        return dbus_call_verb (object, SP_VERB_LAYER_LOWER, error);
    if ((strcmp(cmd, "to_top") == 0) || (strcmp(cmd, "to_front") == 0))
        return dbus_call_verb (object, SP_VERB_LAYER_TO_TOP, error);
    if ((strcmp(cmd, "to_bottom") == 0) || (strcmp(cmd, "to_back") == 0))
        return dbus_call_verb (object, SP_VERB_LAYER_TO_BOTTOM, error);
    return TRUE;
}

gboolean 
document_interface_layer_next (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_LAYER_NEXT, error);
}

gboolean 
document_interface_layer_previous (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_LAYER_PREV, error);
}


//////////////signals


DocumentInterface *fugly;
gboolean dbus_send_ping (SPDesktop* desk,     SPItem *item)
{
  //DocumentInterface *obj;
  if (!item) return TRUE;
  g_signal_emit (desk->dbus_document_interface, signals[OBJECT_MOVED_SIGNAL], 0, item->getId());
  return TRUE;
}

//////////tree


gboolean
document_interface_get_children (DocumentInterface *object,  char *name, char ***out, GError **error)
{
  SPItem* parent=(SPItem* )get_object_by_name(object->context.getDocument(), name, error);

  GSList const *children = parent->childList(false);

    int size = g_slist_length((GSList *) children);

    *out = g_new0 (char *, size + 1);

    int i = 0;
    for (GSList const *iter = children; iter != NULL; iter = iter->next) {
      (*out)[i] = g_strdup(SP_OBJECT(iter->data)->getRepr()->attribute("id"));
        i++;
    }
    (*out)[i] = NULL;

    return TRUE;

}


gchar* 
document_interface_get_parent (DocumentInterface *object,  char *name, GError **error)
{
  SPItem* node=(SPItem* )get_object_by_name(object->context.getDocument(), name, error);
  
  SPObject* parent=node->parent;

  return g_strdup(parent->getRepr()->attribute("id"));

}

#if 0
//just pseudo code
gboolean
document_interface_get_xpath (DocumentInterface *object,  char *xpath_expression, char ***out, GError **error){
  SPDocument * doc = object->context.getDocument();
  Inkscape::XML::Document *repr = doc->getReprDoc();

  xmlXPathObjectPtr xpathObj;
  xmlXPathContextPtr xpathCtx;
  xpathCtx = xmlXPathNewContext(repr);//XmlDocPtr
  xpathObj = xmlXPathEvalExpression(xmlCharStrdup(xpath_expression), xpathCtx);
  
  //xpathresult result = xpatheval(repr, xpath_selection);
  //convert resut to a string array we can return via dbus
  return TRUE;
}
#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
