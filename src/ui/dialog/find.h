/** @file
 * @brief Find dialog
 */
/* Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004, 2005 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_FIND_H
#define INKSCAPE_UI_DIALOG_FIND_H

#include "ui/widget/panel.h"
#include "ui/widget/button.h"
#include "ui/widget/entry.h" 
#include <glib.h>
#include <gtkmm.h>

#include "desktop.h"
#include "ui/dialog/desktop-tracker.h"

class SPItem;
class SPObject;

namespace Inkscape {
class Selection;

namespace UI {
namespace Dialog {

/**
 * The Find class defines the Find and replace dialog.
 *
 * The Find and replace dialog allows you to search within the
 * current document for specific text or properties of items.
 * Matches items are highlighted and can be replaced as well.
 * Scope can be limited to the entire document, current layer or selected items.
 * Other options allow searching on specific object types and properties.
 */

class Find : public UI::Widget::Panel {
public:
    Find();
    virtual ~Find();

    /**
     * Helper function which returns a new instance of the dialog.
     * getInstance is needed by the dialog manager (Inkscape::UI::Dialog::DialogManager).
     */
    static Find &getInstance() { return *new Find(); }

protected:


    /**
     * Callbacks for pressing the dialog buttons.
     */
    void    onFind();             
    void    onReplace();
    void    onExpander();
    void    onAction();
    void    onToggleAlltypes();
    void    onToggleCheck();
    void    onSearchinText();
    void    onSearchinProperty();

    /**
     * Toggle all the properties checkboxes
     */
    void    searchinToggle(bool on);

    /**
     * Returns true if the SPItem 'item' has the same id
     *
     * @param item the SPItem to check
     * @param id the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     */
    bool        item_id_match (SPItem *item, const gchar *id, bool exact, bool casematch, bool replace=false);
    /**
     * Returns true if the SPItem 'item' has the same text content
     *
     * @param item the SPItem to check
     * @param name the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     *
     */
    bool        item_text_match (SPItem *item, const gchar *text, bool exact, bool casematch, bool replace=false);
    /**
     * Returns true if the SPItem 'item' has the same text in the style attribute
     *
     * @param item the SPItem to check
     * @param name the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     */
    bool        item_style_match (SPItem *item, const gchar *text, bool exact, bool casematch, bool replace=false);
    /**
     * Returns true if found the SPItem 'item' has the same attribute name
     *
     * @param item the SPItem to check
     * @param name the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     */
    bool        item_attr_match (SPItem *item, const gchar *name, bool exact, bool casematch, bool replace=false);
    /**
     * Returns true if the SPItem 'item' has the same attribute value
     *
     * @param item the SPItem to check
     * @param name the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     */
    bool        item_attrvalue_match (SPItem *item, const gchar *name, bool exact, bool casematch, bool replace=false);
    /**
     * Returns true if the SPItem 'item' has the same font values
     *
     * @param item the SPItem to check
     * @param name the value to compare with
     * @param exact do an exacty match
     * @param casematch match the text case exactly
     * @param replace replace the value if found
     *
     */
    bool        item_font_match (SPItem *item, const gchar *name, bool exact, bool casematch, bool replace=false);
    /**
     * Function to filter a list of items based on the item type by calling each item_XXX_match function
     */
    GSList *    filter_fields (GSList *l, bool exact, bool casematch);
    bool        item_type_match (SPItem *item);
    GSList *    filter_types (GSList *l);
    GSList *    filter_list (GSList *l, bool exact, bool casematch);

    /**
     * Find a string within a string and returns true if found with options for exact and casematching
     */
    bool        find_strcmp(const gchar *str, const gchar *find, bool exact, bool casematch);

    /**
     * Find a string within a string and return the position with options for exact, casematching and search start location
     */
    gsize       find_strcmp_pos(const gchar *str, const gchar *find, bool exact, bool casematch, gsize start=0);

    /**
     * Replace a string with another string with options for exact and casematching and replace once/all
     */
    Glib::ustring find_replace(const gchar *str, const gchar *find, const gchar *replace, bool exact, bool casematch, bool replaceall);

    /**
     * recursive function to return a list of all the items in the SPObject tree
     *
     */
    GSList *    all_items (SPObject *r, GSList *l, bool hidden, bool locked);
    /**
     * to return a list of all the selected items
     *
     */
    GSList *    all_selection_items (Inkscape::Selection *s, GSList *l, SPObject *ancestor, bool hidden, bool locked);

    /**
     * Shrink the dialog size when the expander widget is closed
     * Currently not working, no known way to do this
     */
    void        squeeze_window();
    /**
     * Can be invoked for setting the desktop. Currently not used.
     */
    void        setDesktop(SPDesktop *desktop);
    /**
     * Is invoked by the desktop tracker when the desktop changes.
     */
    void        setTargetDesktop(SPDesktop *desktop);
    
    /**
     * Called when desktop selection changes
     */
    void onSelectionChange(void);

private:
    Find(Find const &d);
    Find& operator=(Find const &d);

    /*
     * Find and replace combo box widgets
     */
    Inkscape::UI::Widget::ComboBoxText   entry_find;
    Inkscape::UI::Widget::ComboBoxText   entry_replace;

    /**
     * Scope and search in widgets
     */
    Inkscape::UI::Widget::RadioButton    check_scope_all;
    Inkscape::UI::Widget::RadioButton    check_scope_layer;
    Inkscape::UI::Widget::RadioButton    check_scope_selection;
    Inkscape::UI::Widget::RadioButton    check_searchin_text;
    Inkscape::UI::Widget::RadioButton    check_searchin_property;
    Gtk::HBox hbox_searchin;
    Gtk::VBox vbox_scope;
    Gtk::VBox vbox_searchin;
    Gtk::Frame frame_searchin;
    Gtk::Frame frame_scope;

    /**
     * General option widgets
     */
    Inkscape::UI::Widget::CheckButton    check_case_sensitive;
    Inkscape::UI::Widget::CheckButton    check_exact_match;
    Inkscape::UI::Widget::CheckButton    check_include_hidden;
    Inkscape::UI::Widget::CheckButton    check_include_locked;
    Gtk::VBox vbox_options1;
    Gtk::VBox vbox_options2;
    Gtk::HBox hbox_options;
    Gtk::VBox vbox_expander;
    Gtk::Expander  expander_options;
    Gtk::Frame frame_options;

    /**
     * Property type widgets
     */
    Inkscape::UI::Widget::CheckButton    check_ids;
    Inkscape::UI::Widget::CheckButton    check_attributename;
    Inkscape::UI::Widget::CheckButton    check_attributevalue;
    Inkscape::UI::Widget::CheckButton    check_style;
    Inkscape::UI::Widget::CheckButton    check_font;
    Gtk::VBox vbox_properties;
    Gtk::HBox hbox_properties1;
    Gtk::HBox hbox_properties2;
    Gtk::Frame frame_properties;

    /**
     * A vector of all the properties widgets for easy processing
     */
    std::vector<Inkscape::UI::Widget::CheckButton *> checkProperties;

    /**
     * Object type widgets
     */
    Inkscape::UI::Widget::CheckButton    check_alltypes;
    Inkscape::UI::Widget::CheckButton    check_rects;
    Inkscape::UI::Widget::CheckButton    check_ellipses;
    Inkscape::UI::Widget::CheckButton    check_stars;
    Inkscape::UI::Widget::CheckButton    check_spirals;
    Inkscape::UI::Widget::CheckButton    check_paths;
    Inkscape::UI::Widget::CheckButton    check_texts;
    Inkscape::UI::Widget::CheckButton    check_groups;
    Inkscape::UI::Widget::CheckButton    check_clones;
    Inkscape::UI::Widget::CheckButton    check_images;
    Inkscape::UI::Widget::CheckButton    check_offsets;
    Gtk::VBox vbox_types1;
    Gtk::VBox vbox_types2;
    Gtk::HBox hbox_types;
    Gtk::Frame frame_types;

    /**
     * A vector of all the check option widgets for easy processing
     */
    std::vector<Inkscape::UI::Widget::CheckButton *> checkTypes;

    //Gtk::HBox hbox_text;

    /**
     * Action Buttons and status
     */
    Gtk::Label status;
    Inkscape::UI::Widget::Button button_find;
    Inkscape::UI::Widget::Button button_replace;
    Gtk::HButtonBox box_buttons;
    Gtk::HBox hboxbutton_row;

    /**
     *  Finding or replacing
     */
    bool _action_replace;
    bool blocked;

    SPDesktop *desktop;
    DesktopTracker deskTrack;
    sigc::connection desktopChangeConn;
    sigc::connection selectChangedConn;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_FIND_H

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
