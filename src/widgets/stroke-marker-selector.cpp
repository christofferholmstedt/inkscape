/**
 * @file
 * Combobox for selecting dash patterns - implementation.
 */
/* Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "stroke-marker-selector.h"

#include <cstring>
#include <string>
#include <glibmm/i18n.h>
#include <2geom/coord.h>


#include "style.h"
#include "dialogs/dialog-events.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "preferences.h"
#include "path-prefix.h"
#include "io/sys.h"
#include "marker.h"
#include "sp-defs.h"
#include "sp-root.h"
#include "ui/cache/svg_preview_cache.h"
#include "helper/stock-items.h"

#include <gtkmm/adjustment.h>
#include "ui/widget/spinbutton.h"

static Inkscape::UI::Cache::SvgPreview svg_preview_cache;

MarkerComboBox::MarkerComboBox(gchar const *id) :
            Gtk::ComboBox(),
            combo_id(id),
            updating(false),
            is_history(false)
{

    marker_store = Gtk::ListStore::create(marker_columns);
    set_model(marker_store);
    pack_start(image_renderer, false);
    pack_end(label_renderer, true);
    label_renderer.set_padding(5, 0);
    image_renderer.set_padding(5, 0);
    set_cell_data_func(label_renderer, sigc::mem_fun(*this, &MarkerComboBox::prepareLabelRenderer));
    set_cell_data_func(image_renderer, sigc::mem_fun(*this, &MarkerComboBox::prepareImageRenderer));
    gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(gobj()), MarkerComboBox::separator_cb, NULL, NULL);

    empty_image = new Gtk::Image();

    sandbox = ink_markers_preview_doc ();
    desktop = inkscape_active_desktop();
    doc = sp_desktop_document(desktop);

    init_combo();

    show();
}

MarkerComboBox::~MarkerComboBox() {
    delete combo_id;
    delete sandbox;
}

/**
 * Init the combobox widget to display markers from markers.svg
 */
void
MarkerComboBox::init_combo()
{
    updating = false;

    if (!doc) {
        Gtk::TreeModel::Row row = *(marker_store->append());
        row[marker_columns.label] = _("No document selected");
        row[marker_columns.image] = NULL;
        set_sensitive(false);
        set_current(NULL);
        return;
    }

    static SPDocument *markers_doc = NULL;

    // add "None"
    Gtk::TreeModel::Row row = *(marker_store->append());
    row[marker_columns.label] = _("None");
    row[marker_columns.marker] = g_strdup("none");
    row[marker_columns.image] = NULL;

    // find and load markers.svg
    if (markers_doc == NULL) {
        char *markers_source = g_build_filename(INKSCAPE_MARKERSDIR, "markers.svg", NULL);
        if (Inkscape::IO::file_test(markers_source, G_FILE_TEST_IS_REGULAR)) {
            markers_doc = SPDocument::createNewDoc(markers_source, FALSE);
        }
        g_free(markers_source);
    }

    // suck in from current doc
    is_history = true;
    sp_marker_list_from_doc(doc);
    is_history = false;

    // add separator
    Gtk::TreeModel::Row row_sep = *(marker_store->append());
    row_sep[marker_columns.label] = "Separator";
    row_sep[marker_columns.isseparator] = true;
    row_sep[marker_columns.image] = NULL;

    // suck in from markers.svg
    if (markers_doc) {
        doc->ensureUpToDate();
        sp_marker_list_from_doc(markers_doc);
    }

    set_sensitive(true);

    /* Set history */
    set_current(NULL);
}

/**
 * Sets the current marker in the marker combobox.
 */
void MarkerComboBox::set_current(SPObject *marker)
{
    updating = true;

    if (marker != NULL) {
        bool mark_is_stock = false;
        if (marker->getRepr()->attribute("inkscape:stockid")) {
            mark_is_stock = true;
        }

        gchar *markname = 0;
        if (mark_is_stock) {
            markname = g_strdup(marker->getRepr()->attribute("inkscape:stockid"));
        } else {
            markname = g_strdup(marker->getRepr()->attribute("id"));
        }

        set_selected(markname);

        g_free (markname);
    }
    else {
        set_selected(NULL);
    }

    updating = false;

}
/**
 * Return a uri string representing the current selected marker used for setting the marker style in the document
 */
const gchar * MarkerComboBox::get_active_marker_uri()
{
    /* Get Marker */
    const gchar *markid = get_active()->get_value(marker_columns.marker);
    if (!markid)
    {
        return NULL;
    }

    gchar const *marker = "";
    if (strcmp(markid, "none")) {
       bool stockid = get_active()->get_value(marker_columns.isstock);

       gchar *markurn = g_strdup(markid);
       if (stockid) markurn = g_strconcat("urn:inkscape:marker:",markid,NULL);
       SPObject *mark = get_stock_item(markurn);
       g_free(markurn);
       if (mark) {
           Inkscape::XML::Node *repr = mark->getRepr();
            marker = g_strconcat("url(#", repr->attribute("id"), ")", NULL);
        }
    } else {
        marker = g_strdup(markid);
    }

    return marker;
}


void MarkerComboBox::set_active_history() {
    set_selected(get_active()->get_value(marker_columns.marker));
}


void MarkerComboBox::set_selected(const gchar *name) {

    if (!name) {
        set_active(0);
        return;
    }

    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.marker] &&
            		!strcmp(row[marker_columns.marker], name)) {
                set_active(iter);
                if (strcmp(name, "none"))
                    set_history(row);
                return;
            }
    }
}

void MarkerComboBox::set_history(Gtk::TreeModel::Row match_row) {

    if (!match_row) {
        return;
    }

    for(Gtk::TreeIter iter = marker_store->children().begin();
        iter != marker_store->children().end(); ++iter) {
            Gtk::TreeModel::Row row = (*iter);
            if (row[marker_columns.history] &&
                    !strcmp(row[marker_columns.marker], match_row[marker_columns.marker])) {
                    return;
            }
    }

    // Add a new row to the history
    Gtk::TreeModel::Row row = *(marker_store->insert_after(marker_store->children().begin()));
    row[marker_columns.marker] = g_strdup(match_row.get_value(marker_columns.marker));
    row[marker_columns.label] = match_row.get_value(marker_columns.label);
    row[marker_columns.isstock] = match_row.get_value(marker_columns.isstock);
    row[marker_columns.image] = match_row.get_value(marker_columns.image);
    row[marker_columns.history] = true;
}

/**
 * Pick up all markers from source, except those that are in
 * current_doc (if non-NULL), and add items to the combo.
 */
void MarkerComboBox::sp_marker_list_from_doc(SPDocument *source)
{
    GSList *ml = get_marker_list(source);
    GSList *clean_ml = NULL;

    for (; ml != NULL; ml = ml->next) {
        if (!SP_IS_MARKER(ml->data))
            continue;

        // Add to the list of markers we really do wish to show
        clean_ml = g_slist_prepend (clean_ml, ml->data);
    }
    add_markers(clean_ml, source);

    g_slist_free (ml);
    g_slist_free (clean_ml);
}

/**
 *  Returns a list of markers in the defs of the given source document as a GSList object
 *  Returns NULL if there are no markers in the document.
 */
GSList *MarkerComboBox::get_marker_list (SPDocument *source)
{
    if (source == NULL)
        return NULL;

    GSList *ml   = NULL;
    SPDefs *defs = source->getDefs();
    for ( SPObject *child = defs->firstChild(); child; child = child->getNext() )
    {
        if (SP_IS_MARKER(child)) {
            ml = g_slist_prepend (ml, child);
        }
    }
    return ml;
}

/**
 * Adds previews of markers in marker_list to the combo
 */
void MarkerComboBox::add_markers (GSList *marker_list, SPDocument *source)
{
    // Do this here, outside of loop, to speed up preview generation:
    Inkscape::Drawing drawing;
    unsigned const visionkey = SPItem::display_key_new(1);
    drawing.setRoot(sandbox->getRoot()->invoke_show(drawing, visionkey, SP_ITEM_SHOW_DISPLAY));

    for (; marker_list != NULL; marker_list = marker_list->next) {

        Inkscape::XML::Node *repr = reinterpret_cast<SPItem *>(marker_list->data)->getRepr();
        bool isstock = (repr->attribute("inkscape:stockid"));
        gchar const *markid = repr->attribute("id");

        // generate preview
        Gtk::Image *prv = create_marker_image (22, markid, source, drawing, visionkey);
        prv->show();

        Gtk::TreeModel::Row row = *(marker_store->append());
        row[marker_columns.label] = g_strdup(markid);
        row[marker_columns.marker] = g_strdup(markid);
        row[marker_columns.isstock] = isstock;
        row[marker_columns.image] = prv;
        row[marker_columns.history] = is_history;

    }

    sandbox->getRoot()->invoke_hide(visionkey);
}

/**
 * Creates a copy of the marker named mname, determines its visible and renderable
 * area in the bounding box, and then renders it.  This allows us to fill in
 * preview images of each marker in the marker combobox.
 */
Gtk::Image *
MarkerComboBox::create_marker_image(unsigned psize, gchar const *mname,
                   SPDocument *source,  Inkscape::Drawing &drawing, unsigned /*visionkey*/)
{
    // Retrieve the marker named 'mname' from the source SVG document
    SPObject const *marker = source->getObjectById(mname);
    if (marker == NULL) {
        return NULL;
    }

    // Create a copy repr of the marker with id="sample"
    Inkscape::XML::Document *xml_doc = sandbox->getReprDoc();
    Inkscape::XML::Node *mrepr = marker->getRepr()->duplicate(xml_doc);
    mrepr->setAttribute("id", "sample");

    // Replace the old sample in the sandbox by the new one
    Inkscape::XML::Node *defsrepr = sandbox->getObjectById("defs")->getRepr();
    SPObject *oldmarker = sandbox->getObjectById("sample");
    if (oldmarker) {
        oldmarker->deleteObject(false);
    }

    // TODO - This causes a SIGTRAP on windows
    defsrepr->appendChild(mrepr);

    Inkscape::GC::release(mrepr);

// Uncomment this to get the sandbox documents saved (useful for debugging)
    //FILE *fp = fopen (g_strconcat(combo_id, mname, ".svg", NULL), "w");
    //sp_repr_save_stream(sandbox->getReprDoc(), fp);
    //fclose (fp);

    // object to render; note that the id is the same as that of the combo we're building
    SPObject *object = sandbox->getObjectById(combo_id);
    sandbox->getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    sandbox->ensureUpToDate();

    if (object == NULL || !SP_IS_ITEM(object)) {
        return NULL; // sandbox broken?
    }

    SPItem *item = SP_ITEM(object);
    // Find object's bbox in document
    Geom::OptRect dbox = item->documentVisualBounds();

    if (!dbox) {
        return NULL;
    }

    /* Update to renderable state */
    double sf = 0.8;

    gchar *cache_name = g_strconcat(combo_id, mname, NULL);
    Glib::ustring key = svg_preview_cache.cache_key(source->getURI(), cache_name, psize);
    g_free (cache_name);
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(svg_preview_cache.get_preview_from_cache(key));

    if (!pixbuf) {
        pixbuf = Glib::wrap(render_pixbuf(drawing, sf, *dbox, psize));
        svg_preview_cache.set_preview_in_cache(key, pixbuf->gobj());
    }

    // Create widget
    Gtk::Image *pb = new Gtk::Image(pixbuf);

    return pb;
}

void MarkerComboBox::prepareLabelRenderer( Gtk::TreeModel::const_iterator const &row ) {
    Glib::ustring name=(*row)[marker_columns.label];
    label_renderer.property_markup() = name.c_str();
}

void MarkerComboBox::prepareImageRenderer( Gtk::TreeModel::const_iterator const &row ) {

    Gtk::Image *image = (*row)[marker_columns.image];
    if (image)
        image_renderer.property_pixbuf() = image->get_pixbuf();
    else
        image_renderer.property_pixbuf() = empty_image->get_pixbuf();
}

gboolean MarkerComboBox::separator_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {

    gboolean sep = FALSE;
    gtk_tree_model_get(model, iter, 5, &sep, -1);
    return sep;
}

/**
 * Returns a new document containing default start, mid, and end markers.
 */
SPDocument *MarkerComboBox::ink_markers_preview_doc ()
{
gchar const *buffer = "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\" xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
"  <defs id=\"defs\" />"

"  <g id=\"marker-start\">"
"    <path style=\"fill:none;stroke:black;stroke-width:1.7;marker-start:url(#sample);marker-mid:none;marker-end:none\""
"       d=\"M 12.5,13 L 25,13\" id=\"path1\" />"
"    <rect style=\"fill:none;stroke:none\" id=\"rect2\""
"       width=\"25\" height=\"25\" x=\"0\" y=\"0\" />"
"  </g>"

"  <g id=\"marker-mid\">"
"    <path style=\"fill:none;stroke:black;stroke-width:1.7;marker-start:none;marker-mid:url(#sample);marker-end:none\""
"       d=\"M 0,113 L 12.5,113 L 25,113\" id=\"path11\" />"
"    <rect style=\"fill:none;stroke:none\" id=\"rect22\""
"       width=\"25\" height=\"25\" x=\"0\" y=\"100\" />"
"  </g>"

"  <g id=\"marker-end\">"
"    <path style=\"fill:none;stroke:black;stroke-width:1.7;marker-start:none;marker-mid:none;marker-end:url(#sample)\""
"       d=\"M 0,213 L 12.5,213\" id=\"path111\" />"
"    <rect style=\"fill:none;stroke:none\" id=\"rect222\""
"       width=\"25\" height=\"25\" x=\"0\" y=\"200\" />"
"  </g>"

"</svg>";

    return SPDocument::createNewDocFromMem (buffer, strlen(buffer), FALSE);
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
