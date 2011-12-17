/* Authors:
 *  Lauris Kaplinski <lauris@ximian.com>
 *  Abhishek Sharma
 *  Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2011, authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtk/gtk.h>
#include "xml/repr.h"
#include "macros.h"
#include "document.h"
#include "sp-object.h"
#include <glibmm/i18n.h>

#include <sigc++/functors/ptr_fun.h>
#include <sigc++/adaptors/bind.h>

#include "sp-attribute-widget.h"

using Inkscape::DocumentUndo;

static void sp_attribute_widget_object_modified ( SPObject *object,
                                                  guint flags,
                                                  SPAttributeWidget *spaw );


SPAttributeWidget::SPAttributeWidget () : 
    blocked(0),
    hasobj(0),
    _attribute(),
    modified_connection()
{
    src.object = NULL;
}

SPAttributeWidget::~SPAttributeWidget ()
{
    if (hasobj)
    {
        if (src.object)
        {
            modified_connection.disconnect();
            src.object = NULL;
        }
    }
    else
    {
        if (src.repr)
        {
            src.repr = Inkscape::GC::release(src.repr);
        }
    }
}

void SPAttributeWidget::set_object(SPObject *object, const gchar *attribute)
{
    if (hasobj) {
        if (src.object) {
            modified_connection.disconnect();
            src.object = NULL;
        }
    } else {

        if (src.repr) {
            src.repr = Inkscape::GC::release(src.repr);
        }
    }

    hasobj = true;
    
    if (object) {
        const gchar *val;

        blocked = true;
        src.object = object;

        modified_connection = object->connectModified(sigc::bind<2>(sigc::ptr_fun(&sp_attribute_widget_object_modified), this));

        _attribute = attribute;

        val = object->getRepr()->attribute(attribute);
        set_text (val ? val : (const gchar *) "");
        blocked = false;
    }
    gtk_widget_set_sensitive (GTK_WIDGET(this), (src.object != NULL));
}

void SPAttributeWidget::set_repr(Inkscape::XML::Node *repr, const gchar *attribute)
{
    if (hasobj) {
        if (src.object) {
            modified_connection.disconnect();
            src.object = NULL;
        }
    } else {

        if (src.repr) {
            src.repr = Inkscape::GC::release(src.repr);
        }
    }

    hasobj = false;
    
    if (repr) {
        const gchar *val;

        blocked = true;
        src.repr = Inkscape::GC::anchor(repr);
        attribute = g_strdup (attribute);

        val = repr->attribute(attribute);
        set_text (val ? val : (const gchar *) "");
        blocked = false;
    }
    gtk_widget_set_sensitive (GTK_WIDGET (this), (src.repr != NULL));
}

void SPAttributeWidget::on_changed (void)
{
    if (!blocked)
    {
        Glib::ustring text1;
        const gchar *text;
        blocked = true;
        text1 = get_text ();
		text = text1.c_str();
        if (!*text)
            text = NULL;

        if (hasobj && src.object) {        
            src.object->getRepr()->setAttribute(_attribute.c_str(), text, false);
            DocumentUndo::done(src.object->document, SP_VERB_NONE,
                                _("Set attribute"));

        } else if (src.repr) {
            src.repr->setAttribute(_attribute.c_str(), text, false);
            /* TODO: Warning! Undo will not be flushed in given case */
        }
        blocked = false;
    }
}

static void sp_attribute_widget_object_modified ( SPObject */*object*/,
                                      guint flags,
                                      SPAttributeWidget *spaw )
{

    if (flags && SP_OBJECT_MODIFIED_FLAG) {

        const gchar *val;
        Glib::ustring text;
        Glib::ustring attr = spaw->get_attribute();
        val = spaw->src.object->getRepr()->attribute(attr.c_str());
        text = spaw->get_text();

        if (val || !text.empty()) {

            if (!val || text.empty() || (text == val)) {
                /* We are different */
                spaw->set_blocked(true);
                spaw->set_text(val ? val : (const gchar *) "");
                spaw->set_blocked(false);
            } // end of if()

        } // end of if()

    } //end of if()

} // end of sp_attribute_widget_object_modified()



/* SPAttributeTable */
static void sp_attribute_table_object_modified (SPObject *object, guint flags, SPAttributeTable *spaw);
static void sp_attribute_table_entry_changed (Gtk::Editable *editable, SPAttributeTable *spat);

#define XPAD 4
#define YPAD 0

SPAttributeTable::SPAttributeTable () : 
    blocked(0),
    hasobj(0),
    table(0),
    _attributes(),
    _entries(),
    modified_connection()
{
    src.object = NULL;
}

SPAttributeTable::SPAttributeTable (SPObject *object, std::vector<Glib::ustring> &labels, std::vector<Glib::ustring> &attributes, GtkWidget* parent) : 
    blocked(0),
    hasobj(0),
    table(0),
    _attributes(),
    _entries(),
    modified_connection()
{
    src.object = NULL;
    set_object(object, labels, attributes, parent);
}

SPAttributeTable::~SPAttributeTable ()
{
    clear();
}

void SPAttributeTable::clear(void)
{
    Gtk::Widget *w;
    
    if (table)
    {
    std::vector<Widget*> ch = table->get_children();
    
    for (int i = (ch.size())-1; i >=0 ; i--)
    {
        w = ch[i];
        ch.pop_back();
        if (w != NULL)
        {
            try
            {
                delete w;
            }
            catch(...)
            {
            }
        }
    }
    ch.clear();
    _attributes.clear();
    _entries.clear();

        delete table;
        table = NULL;
    }

    if (hasobj) {
        if (src.object) {
            modified_connection.disconnect();
            src.object = NULL;
        }
    } else {
        if (src.repr) {
            src.repr = Inkscape::GC::release(src.repr);
        }
    }
}

void SPAttributeTable::set_object(SPObject *object,
                            std::vector<Glib::ustring> &labels,
                            std::vector<Glib::ustring> &attributes,
                            GtkWidget* parent)
{
    g_return_if_fail (parent);
    g_return_if_fail (!object || SP_IS_OBJECT (object));
    g_return_if_fail (!object || !labels.empty() || !attributes.empty());
    g_return_if_fail (labels.size() == attributes.size());

    clear();
    hasobj = true;

    if (object) {
        blocked = true;

        // Set up object
        src.object = object;
        modified_connection = object->connectModified(sigc::bind<2>(sigc::ptr_fun(&sp_attribute_table_object_modified), this));

        // Create table
        table = Gtk::manage(new Gtk::Table (attributes.size(), 2, false));
        gtk_container_add (GTK_CONTAINER (parent),(GtkWidget*)table->gobj());
        
        // Fill rows
		_attributes = attributes;
        for (guint i = 0; i < (attributes.size()); i++) {
            Gtk::Label *ll;
            Gtk::Entry *ee;
            Gtk::Widget *w;
            const gchar *val;

            ll = new Gtk::Label (_(labels[i].c_str()));
            w = (Gtk::Widget *) ll;
            ll->show();
            ll->set_alignment (1.0, 0.5);
            table->attach (*w, 0, 1, i, i + 1,
                               Gtk::FILL,
                               (Gtk::EXPAND | Gtk::FILL),
                               XPAD, YPAD );
            ee = new Gtk::Entry();
            w = (Gtk::Widget *) ee;
            ee->show();
            val = object->getRepr()->attribute(attributes[i].c_str());
            ee->set_text (val ? val : (const gchar *) "");
            table->attach (*w, 1, 2, i, i + 1,
                               (Gtk::EXPAND | Gtk::FILL),
                               (Gtk::EXPAND | Gtk::FILL),
                               XPAD, YPAD );
            _entries.push_back(w);
            g_signal_connect ( w->gobj(), "changed",
                               G_CALLBACK (sp_attribute_table_entry_changed),
                               this );
        }
        /* Show table */
        table->show ();
        blocked = false;
    }
}

void SPAttributeTable::set_repr (Inkscape::XML::Node *repr,
                            std::vector<Glib::ustring> &labels,
                            std::vector<Glib::ustring> &attributes,
                            GtkWidget* parent)
{
    g_return_if_fail (!labels.empty() || !attributes.empty());
    g_return_if_fail (labels.size() == attributes.size());

    clear();

    hasobj = false;

    if (repr) {
        blocked = true;

        // Set up repr
        src.repr = Inkscape::GC::anchor(repr);
        
        // Create table
        table = new Gtk::Table (attributes.size(), 2, false);
        gtk_container_add (GTK_CONTAINER (parent),(GtkWidget*)table->gobj());
        
        // Fill rows
        _attributes = attributes;
        for (guint i = 0; i < (attributes.size()); i++) {
            Gtk::Label *ll;
            Gtk::Entry *ee;
            Gtk::Widget *w;
            const gchar *val;

            ll = new Gtk::Label (_(labels[i].c_str()));
            w = (Gtk::Widget *) ll;
            ll->show ();
            ll->set_alignment (1.0, 0.5);
            table->attach (*w, 0, 1, i, i + 1,
                               Gtk::FILL,
                               (Gtk::EXPAND | Gtk::FILL),
                               XPAD, YPAD );
            ee = new Gtk::Entry();
            w = (Gtk::Widget *) ee;
            ee->show();
            val = repr->attribute(attributes[i].c_str());
            ee->set_text (val ? val : (const gchar *) "");
            table->attach (*w, 1, 2, i, i + 1,
                               (Gtk::EXPAND | Gtk::FILL),
                               (Gtk::EXPAND | Gtk::FILL),
                               XPAD, YPAD );
            _entries.push_back(w);
            g_signal_connect ( w->gobj(), "changed",
                               G_CALLBACK (sp_attribute_table_entry_changed),
                               this );
        }
        /* Show table */
        table->show ();
        blocked = false;
    }
}


static void sp_attribute_table_object_modified ( SPObject */*object*/,
                                     guint flags,
                                     SPAttributeTable *spat )
{
    if (flags && SP_OBJECT_MODIFIED_FLAG)
    {
        guint i;
        std::vector<Glib::ustring> attributes = spat->get_attributes();
        std::vector<Gtk::Widget *> entries = spat->get_entries();
        Gtk::Entry* e;
        Glib::ustring text;
        for (i = 0; i < (attributes.size()); i++) {
            const gchar *val;
            e = (Gtk::Entry*) entries[i];
            val = spat->src.object->getRepr()->attribute(attributes[i].c_str());
            text = e->get_text ();
            if (val || !text.empty()) {
                if (text != val) {
                    /* We are different */
                    spat->blocked = true;
                    e->set_text (val ? val : (const gchar *) "");
                    spat->blocked = false;
                }
            }
        }
    } // end of if()

} // end of sp_attribute_table_object_modified()

static void sp_attribute_table_entry_changed ( Gtk::Editable *editable,
                                   SPAttributeTable *spat )
{
    if (!spat->blocked)
    {
        guint i;
        std::vector<Glib::ustring> attributes = spat->get_attributes();
        std::vector<Gtk::Widget *> entries = spat->get_entries();
        Gtk::Entry *e;
        for (i = 0; i < (attributes.size()); i++) {
            e = (Gtk::Entry *) entries[i];
            if ((GtkWidget*) (editable) == (GtkWidget*) e->gobj()) {
                spat->blocked = true;
                Glib::ustring text = e->get_text ();

                if (spat->hasobj && spat->src.object) {
                    spat->src.object->getRepr()->setAttribute(attributes[i].c_str(), text.c_str(), false);
                    DocumentUndo::done(spat->src.object->document, SP_VERB_NONE,
                                       _("Set attribute"));

                } else if (spat->src.repr) {

                    spat->src.repr->setAttribute(attributes[i].c_str(), text.c_str(), false);
                    /* TODO: Warning! Undo will not be flushed in given case */
                }
                spat->blocked = false;
                return;
            }
        }
        g_warning ("file %s: line %d: Entry signalled change, but there is no such entry", __FILE__, __LINE__);
    } // end of if()

} // end of sp_attribute_table_entry_changed()

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
