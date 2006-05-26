#define __INKSCAPE_C__

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2005 authors
 * g++ port Copyright (C) 2003 Nathan Hurst
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "debug/simple-event.h"
#include "debug/event-tracker.h"

#ifndef WIN32
# define HAS_PROC_SELF_EXE  //to get path of executable
#else

// For now to get at is_os_wide().
# include "extension/internal/win32.h"
using Inkscape::Extension::Internal::PrintWin32;

#define _WIN32_IE 0x0400
//#define HAS_SHGetSpecialFolderPath
#define HAS_SHGetSpecialFolderLocation
#define HAS_GetModuleFileName
# include <shlobj.h>
#endif

#include <signal.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkmessagedialog.h>

#include <glibmm/i18n.h>
#include "helper/sp-marshal.h"
#include "dialogs/debugdialog.h"
#include "application/application.h"
#include "application/editor.h"
#include "preferences.h"


#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "event-context.h"
#include "inkscape-private.h"
#include "prefs-utils.h"
#include "xml/repr.h"
#include "io/sys.h"

#include "extension/init.h"

static Inkscape::Application *inkscape = NULL;

/* Backbones of configuration xml data */
#include "menus-skeleton.h"

enum {
    MODIFY_SELECTION, // global: one of selections modified
    CHANGE_SELECTION, // global: one of selections changed
    CHANGE_SUBSELECTION, // global: one of subselections (text selection, gradient handle, etc) changed
    SET_SELECTION, // global: one of selections set
    SET_EVENTCONTEXT, // tool switched
    ACTIVATE_DESKTOP, // some desktop got focus
    DEACTIVATE_DESKTOP, // some desktop lost focus
    SHUTDOWN_SIGNAL, // inkscape is quitting
    DIALOGS_HIDE, // user pressed F12
    DIALOGS_UNHIDE, // user pressed F12
    EXTERNAL_CHANGE, // a document was changed by some external means (undo or XML editor); this
                     // may not be reflected by a selection change and thus needs a separate signal
    LAST_SIGNAL
};

#define DESKTOP_IS_ACTIVE(d) ((d) == inkscape->desktops->data)


/*################################
# FORWARD DECLARATIONS
################################*/

gboolean inkscape_app_use_gui( Inkscape::Application const * app );

static void inkscape_class_init (Inkscape::ApplicationClass *klass);
static void inkscape_init (SPObject *object);
static void inkscape_dispose (GObject *object);

static void inkscape_activate_desktop_private (Inkscape::Application *inkscape, SPDesktop *desktop);
static void inkscape_deactivate_desktop_private (Inkscape::Application *inkscape, SPDesktop *desktop);

static bool inkscape_init_config (Inkscape::XML::Document *doc, const gchar *config_name, const gchar *skeleton,
				  unsigned int skel_size,
				  const gchar *e_mkdir,
				  const gchar *e_notdir,
				  const gchar *e_ccf,
				  const gchar *e_cwf,
				  const gchar *warn);

struct Inkscape::Application {
    GObject object;
    Inkscape::XML::Document *menus;
    GSList *documents;
    GSList *desktops;
    gchar *argv0;
    gboolean dialogs_toggle;
    gboolean use_gui;         // may want to consider a virtual function
                              // for overriding things like the warning dlg's
};

struct Inkscape::ApplicationClass {
    GObjectClass object_class;

    /* Signals */
    void (* change_selection) (Inkscape::Application * inkscape, Inkscape::Selection * selection);
    void (* change_subselection) (Inkscape::Application * inkscape, SPDesktop *desktop);
    void (* modify_selection) (Inkscape::Application * inkscape, Inkscape::Selection * selection, guint flags);
    void (* set_selection) (Inkscape::Application * inkscape, Inkscape::Selection * selection);
    void (* set_eventcontext) (Inkscape::Application * inkscape, SPEventContext * eventcontext);
    void (* activate_desktop) (Inkscape::Application * inkscape, SPDesktop * desktop);
    void (* deactivate_desktop) (Inkscape::Application * inkscape, SPDesktop * desktop);
    void (* destroy_document) (Inkscape::Application *inkscape, SPDocument *doc);
    void (* color_set) (Inkscape::Application *inkscape, SPColor *color, double opacity);
    void (* shut_down) (Inkscape::Application *inkscape);
    void (* dialogs_hide) (Inkscape::Application *inkscape);
    void (* dialogs_unhide) (Inkscape::Application *inkscape);
    void (* external_change) (Inkscape::Application *inkscape);
};

static GObjectClass * parent_class;
static guint inkscape_signals[LAST_SIGNAL] = {0};

static void (* segv_handler) (int) = NULL;

#ifdef WIN32
#define INKSCAPE_PROFILE_DIR "Inkscape"
#else
#define INKSCAPE_PROFILE_DIR ".inkscape"
#endif

#define MENUS_FILE "menus.xml"


/**
 *  Retrieves the GType for the Inkscape Application object.
 */
GType
inkscape_get_type (void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof (Inkscape::ApplicationClass),
            NULL, NULL,
            (GClassInitFunc) inkscape_class_init,
            NULL, NULL,
            sizeof (Inkscape::Application),
            4,
            (GInstanceInitFunc) inkscape_init,
            NULL
        };
        type = g_type_register_static (G_TYPE_OBJECT, "Inkscape_Application", &info, (GTypeFlags)0);
    }
    return type;
}


/**
 *  Initializes the inkscape class, registering all of its signal handlers
 *  and virtual functions
 */
static void
inkscape_class_init (Inkscape::ApplicationClass * klass)
{
    GObjectClass * object_class;

    object_class = (GObjectClass *) klass;

    parent_class = (GObjectClass *)g_type_class_peek_parent (klass);

    inkscape_signals[MODIFY_SELECTION] = g_signal_new ("modify_selection",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, modify_selection),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER_UINT,
                               G_TYPE_NONE, 2,
                               G_TYPE_POINTER, G_TYPE_UINT);
    inkscape_signals[CHANGE_SELECTION] = g_signal_new ("change_selection",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, change_selection),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[CHANGE_SUBSELECTION] = g_signal_new ("change_subselection",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, change_subselection),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[SET_SELECTION] =    g_signal_new ("set_selection",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, set_selection),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[SET_EVENTCONTEXT] = g_signal_new ("set_eventcontext",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, set_eventcontext),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[ACTIVATE_DESKTOP] = g_signal_new ("activate_desktop",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, activate_desktop),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[DEACTIVATE_DESKTOP] = g_signal_new ("deactivate_desktop",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, deactivate_desktop),
                               NULL, NULL,
                               sp_marshal_NONE__POINTER,
                               G_TYPE_NONE, 1,
                               G_TYPE_POINTER);
    inkscape_signals[SHUTDOWN_SIGNAL] =        g_signal_new ("shut_down",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, shut_down),
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);
    inkscape_signals[DIALOGS_HIDE] =        g_signal_new ("dialogs_hide",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, dialogs_hide),
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);
    inkscape_signals[DIALOGS_UNHIDE] =        g_signal_new ("dialogs_unhide",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, dialogs_unhide),
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);
    inkscape_signals[EXTERNAL_CHANGE] =   g_signal_new ("external_change",
                               G_TYPE_FROM_CLASS (klass),
                               G_SIGNAL_RUN_FIRST,
                               G_STRUCT_OFFSET (Inkscape::ApplicationClass, external_change),
                               NULL, NULL,
                               g_cclosure_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);

    object_class->dispose = inkscape_dispose;

    klass->activate_desktop = inkscape_activate_desktop_private;
    klass->deactivate_desktop = inkscape_deactivate_desktop_private;
}


static void
inkscape_init (SPObject * object)
{
    if (!inkscape) {
        inkscape = (Inkscape::Application *) object;
    } else {
        g_assert_not_reached ();
    }

    inkscape->menus = sp_repr_read_mem (_(menus_skeleton), MENUS_SKELETON_SIZE, NULL);

    inkscape->documents = NULL;
    inkscape->desktops = NULL;

    inkscape->dialogs_toggle = TRUE;
}


static void
inkscape_dispose (GObject *object)
{
    Inkscape::Application *inkscape = (Inkscape::Application *) object;

    while (inkscape->documents) {
        // we don't otherwise unref, so why here?
        sp_document_unref((SPDocument *)inkscape->documents->data);
    }

    g_assert (!inkscape->desktops);

    Inkscape::Preferences::save();

    if (inkscape->menus) {
        /* fixme: This is not the best place */
        Inkscape::GC::release(inkscape->menus);
        inkscape->menus = NULL;
    }

    G_OBJECT_CLASS (parent_class)->dispose (object);

    gtk_main_quit ();
}


void
inkscape_ref (void)
{
    if (inkscape)
        g_object_ref (G_OBJECT (inkscape));
}


void
inkscape_unref (void)
{
    if (inkscape)
        g_object_unref (G_OBJECT (inkscape));
}


static void
inkscape_activate_desktop_private (Inkscape::Application *inkscape, SPDesktop *desktop)
{
    desktop->set_active (true);
}


static void
inkscape_deactivate_desktop_private (Inkscape::Application *inkscape, SPDesktop *desktop)
{
    desktop->set_active (false);
}


/* fixme: This is EVIL, and belongs to main after all */

#define SP_INDENT 8


static void
inkscape_segv_handler (int signum)
{
    using Inkscape::Debug::SimpleEvent;
    using Inkscape::Debug::EventTracker;
    using Inkscape::Debug::Logger;

    static gint recursion = FALSE;

    /* let any SIGABRTs seen from within this handler dump core */
    signal(SIGABRT, SIG_DFL);

    /* Kill loops */
    if (recursion) {
        abort ();
    }
    recursion = TRUE;

    EventTracker<SimpleEvent<Inkscape::Debug::Event::CORE> > tracker("crash");
    tracker.set<SimpleEvent<> >("emergency-save");

    fprintf(stderr, "\nEmergency save activated!\n");

    time_t sptime = time (NULL);
    struct tm *sptm = localtime (&sptime);
    gchar sptstr[256];
    strftime (sptstr, 256, "%Y_%m_%d_%H_%M_%S", sptm);

    gint count = 0;
    GSList *savednames = NULL;
    GSList *failednames = NULL;
    for (GSList *l = inkscape->documents; l != NULL; l = l->next) {
        SPDocument *doc;
        Inkscape::XML::Node *repr;
        doc = (SPDocument *) l->data;
        repr = sp_document_repr_root (doc);
        if (repr->attribute("sodipodi:modified")) {
            const gchar *docname, *d0, *d;
            gchar n[64], c[1024];
            FILE *file;

            /* originally, the document name was retrieved from
             * the sodipod:docname attribute */
            docname = doc->name;
            if (docname) {
                /* fixme: Quick hack to remove emergency file suffix */
                d0 = strrchr ((char*)docname, '.');
                if (d0 && (d0 > docname)) {
                    d0 = strrchr ((char*)(d0 - 1), '.');
                    if (d0 && (d0 > docname)) {
                        d = d0;
                        while (isdigit (*d) || (*d == '.') || (*d == '_')) d += 1;
                        if (*d) {
                            memcpy (n, docname, MIN (d0 - docname - 1, 64));
                            n[63] = '\0';
                            docname = n;
                        }
                    }
                }
            }

            if (!docname || !*docname) docname = "emergency";
            // try saving to the profile location
            g_snprintf (c, 1024, "%.256s.%s.%d", docname, sptstr, count);
            gchar * location = homedir_path(c);
            Inkscape::IO::dump_fopen_call(location, "E");
            file = Inkscape::IO::fopen_utf8name(location, "w");
            g_free(location);
            if (!file) {
                // try saving to /tmp
                g_snprintf (c, 1024, "/tmp/inkscape-%.256s.%s.%d", docname, sptstr, count);
                Inkscape::IO::dump_fopen_call(c, "G");
                file = Inkscape::IO::fopen_utf8name(c, "w");
            }
            if (!file) {
                // try saving to the current directory
                g_snprintf (c, 1024, "inkscape-%.256s.%s.%d", docname, sptstr, count);
                Inkscape::IO::dump_fopen_call(c, "F");
                file = Inkscape::IO::fopen_utf8name(c, "w");
            }
            if (file) {
                sp_repr_save_stream (sp_repr_document (repr), file, SP_SVG_NS_URI);
                savednames = g_slist_prepend (savednames, g_strdup (c));
                fclose (file);
            } else {
                docname = repr->attribute("sodipodi:docname");
                failednames = g_slist_prepend (failednames, (docname) ? g_strdup (docname) : g_strdup (_("Untitled document")));
            }
            count++;
        }
    }

    savednames = g_slist_reverse (savednames);
    failednames = g_slist_reverse (failednames);
    if (savednames) {
        fprintf (stderr, "\nEmergency save document locations:\n");
        for (GSList *l = savednames; l != NULL; l = l->next) {
            fprintf (stderr, "  %s\n", (gchar *) l->data);
        }
    }
    if (failednames) {
        fprintf (stderr, "\nFailed to do emergency save for documents:\n");
        for (GSList *l = failednames; l != NULL; l = l->next) {
            fprintf (stderr, "  %s\n", (gchar *) l->data);
        }
    }

    Inkscape::Preferences::save();

    fprintf (stderr, "Emergency save completed. Inkscape will close now.\n");
    fprintf (stderr, "If you can reproduce this crash, please file a bug at www.inkscape.org\n");
    fprintf (stderr, "with a detailed description of the steps leading to the crash, so we can fix it.\n");

    /* Show nice dialog box */

    char const *istr = _("Inkscape encountered an internal error and will close now.\n");
    char const *sstr = _("Automatic backups of unsaved documents were done to the following locations:\n");
    char const *fstr = _("Automatic backup of the following documents failed:\n");
    gint nllen = strlen ("\n");
    gint len = strlen (istr) + strlen (sstr) + strlen (fstr);
    for (GSList *l = savednames; l != NULL; l = l->next) {
        len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
    }
    for (GSList *l = failednames; l != NULL; l = l->next) {
        len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
    }
    len += 1;
    gchar *b = g_new (gchar, len);
    gint pos = 0;
    len = strlen (istr);
    memcpy (b + pos, istr, len);
    pos += len;
    if (savednames) {
        len = strlen (sstr);
        memcpy (b + pos, sstr, len);
        pos += len;
        for (GSList *l = savednames; l != NULL; l = l->next) {
            memset (b + pos, ' ', SP_INDENT);
            pos += SP_INDENT;
            len = strlen ((gchar *) l->data);
            memcpy (b + pos, l->data, len);
            pos += len;
            memcpy (b + pos, "\n", nllen);
            pos += nllen;
        }
    }
    if (failednames) {
        len = strlen (fstr);
        memcpy (b + pos, fstr, len);
        pos += len;
        for (GSList *l = failednames; l != NULL; l = l->next) {
            memset (b + pos, ' ', SP_INDENT);
            pos += SP_INDENT;
            len = strlen ((gchar *) l->data);
            memcpy (b + pos, l->data, len);
            pos += len;
            memcpy (b + pos, "\n", nllen);
            pos += nllen;
        }
    }
    *(b + pos) = '\0';

    if ( inkscape_get_instance() && inkscape_app_use_gui( inkscape_get_instance() ) ) {
        GtkWidget *msgbox = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", b);
        gtk_dialog_run (GTK_DIALOG (msgbox));
        gtk_widget_destroy (msgbox);
    }
    else
    {
        g_message( "Error: %s", b );
    }
    g_free (b);

    tracker.clear();
    Logger::shutdown();

    (* segv_handler) (signum);
}



void
inkscape_application_init (const gchar *argv0, gboolean use_gui)
{
    inkscape = (Inkscape::Application *)g_object_new (SP_TYPE_INKSCAPE, NULL);
    /* fixme: load application defaults */

    segv_handler = signal (SIGSEGV, inkscape_segv_handler);
    signal (SIGFPE,  inkscape_segv_handler);
    signal (SIGILL,  inkscape_segv_handler);
#ifndef WIN32
    signal (SIGBUS,  inkscape_segv_handler);
#endif
    signal (SIGABRT, inkscape_segv_handler);

    inkscape->use_gui = use_gui;
    inkscape->argv0 = g_strdup(argv0);

    /* Attempt to load the preferences, and set the save_preferences flag to TRUE
       if we could, or FALSE if we couldn't */
    Inkscape::Preferences::load();
    inkscape_load_menus(inkscape);

    /* DebugDialog redirection.  On Linux, default to OFF, on Win32, default to ON.
	 * Use only if use_gui is enabled 
	 */
#ifdef WIN32
#define DEFAULT_LOG_REDIRECT true
#else
#define DEFAULT_LOG_REDIRECT false
#endif

    if (use_gui == TRUE && prefs_get_int_attribute("dialogs.debug", "redirect", DEFAULT_LOG_REDIRECT))
    {
		Inkscape::UI::Dialogs::DebugDialog::getInstance()->captureLogMessages();
    }

    /* Initialize the extensions */
    Inkscape::Extension::init();

    return;
}

/**
 *  Returns the current Inkscape::Application global object
 */
Inkscape::Application *
inkscape_get_instance()
{
        return inkscape;
}

gboolean inkscape_app_use_gui( Inkscape::Application const * app )
{
    return app->use_gui;
}

/**
 * Preference management
 * We use '.' as separator
 *
 * Returns TRUE if the config file was successfully loaded, FALSE if not.
 */
bool
inkscape_load_config (const gchar *filename, Inkscape::XML::Document *config, const gchar *skeleton,
		      unsigned int skel_size, const gchar *e_notreg, const gchar *e_notxml,
		      const gchar *e_notsp, const gchar *warn)
{
    gchar *fn = profile_path(filename);
    if (!Inkscape::IO::file_test(fn, G_FILE_TEST_EXISTS)) {
        bool result;
        /* No such file */
        result = inkscape_init_config (config, filename, skeleton,
                                       skel_size,
                                       _("Cannot create directory %s.\n%s"),
                                       _("%s is not a valid directory.\n%s"),
                                       _("Cannot create file %s.\n%s"),
                                       _("Cannot write file %s.\n%s"),
                                       _("Although Inkscape will run, it will use default settings,\n"
                                         "and any changes made in preferences will not be saved."));
        g_free (fn);
        return result;
    }

    if (!Inkscape::IO::file_test(fn, G_FILE_TEST_IS_REGULAR)) {
        /* Not a regular file */
        gchar *safeFn = Inkscape::IO::sanitizeString(fn);
        GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_notreg, safeFn, warn);
        gtk_dialog_run (GTK_DIALOG (w));
        gtk_widget_destroy (w);
        g_free(safeFn);
        g_free (fn);
        return false;
    }

    Inkscape::XML::Document *doc = sp_repr_read_file (fn, NULL);
    if (doc == NULL) {
        /* Not an valid xml file */
        gchar *safeFn = Inkscape::IO::sanitizeString(fn);
        GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_notxml, safeFn, warn);
        gtk_dialog_run (GTK_DIALOG (w));
        gtk_widget_destroy (w);
        g_free(safeFn);
        g_free (fn);
        return false;
    }

    Inkscape::XML::Node *root = sp_repr_document_root (doc);
    if (strcmp (root->name(), "inkscape")) {
        gchar *safeFn = Inkscape::IO::sanitizeString(fn);
        GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_notsp, safeFn, warn);
        gtk_dialog_run (GTK_DIALOG (w));
        gtk_widget_destroy (w);
        Inkscape::GC::release(doc);
        g_free(safeFn);
        g_free (fn);
        return false;
    }

    /** \todo this is a hack, need to figure out how to get
     *        a reasonable merge working with the menus.xml file */
    if (skel_size == MENUS_SKELETON_SIZE) {
        if (INKSCAPE)
            INKSCAPE->menus = doc;
        doc = config;
    } else {
        config->root()->mergeFrom(doc->root(), "id");
    }

    Inkscape::GC::release(doc);
    g_free (fn);
    return true;
}

/**
 *  Menus management
 *
 */
bool
inkscape_load_menus (Inkscape::Application *inkscape)
{
    gchar *fn = profile_path(MENUS_FILE);
    bool retval = false;
    if (Inkscape::IO::file_test(fn, G_FILE_TEST_EXISTS)) {
        retval = inkscape_load_config (MENUS_FILE,
				 inkscape->menus,
				 menus_skeleton,
				 MENUS_SKELETON_SIZE,
				 _("%s is not a regular file.\n%s"),
				 _("%s not a valid XML file, or\n"
				   "you don't have read permissions on it.\n%s"),
				 _("%s is not a valid menus file.\n%s"),
				 _("Inkscape will run with default menus.\n"
                                   "New menus will not be saved."));
    } else {
        INKSCAPE->menus = sp_repr_read_mem(menus_skeleton, MENUS_SKELETON_SIZE, NULL);
        if (INKSCAPE->menus != NULL)
            retval = true;
    }
    g_free(fn);
    return retval;
}

/**
 * We use '.' as separator
 * \param inkscape Unused
 */
Inkscape::XML::Node *
inkscape_get_repr (Inkscape::Application *inkscape, const gchar *key)
{
    if (key == NULL) {
        return NULL;
    }

    Inkscape::XML::Node *repr = sp_repr_document_root (Inkscape::Preferences::get());
    g_assert (!(strcmp (repr->name(), "inkscape")));

    gchar const *s = key;
    while ((s) && (*s)) {

        /* Find next name */
        gchar const *e = strchr (s, '.');
        guint len;
        if (e) {
            len = e++ - s;
        } else {
            len = strlen (s);
        }

        Inkscape::XML::Node* child;
        for (child = repr->firstChild(); child != NULL; child = child->next()) {
            gchar const *id = child->attribute("id");
            if ((id) && (strlen (id) == len) && (!strncmp (id, s, len)))
            {
                break;
            }
        }
        if (child == NULL) {
            return NULL;
        }

        repr = child;
        s = e;
    }
    return repr;
}



void
inkscape_selection_modified (Inkscape::Selection *selection, guint flags)
{
    if (Inkscape::NSApplication::Application::getNewGui()) {
        Inkscape::NSApplication::Editor::selectionModified (selection, flags);
        return;
    }
    g_return_if_fail (selection != NULL);

    if (DESKTOP_IS_ACTIVE (selection->desktop())) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[MODIFY_SELECTION], 0, selection, flags);
    }
}


void
inkscape_selection_changed (Inkscape::Selection * selection)
{
    if (Inkscape::NSApplication::Application::getNewGui()) {
        Inkscape::NSApplication::Editor::selectionChanged (selection);
        return;
    }
    g_return_if_fail (selection != NULL);

    if (DESKTOP_IS_ACTIVE (selection->desktop())) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SELECTION], 0, selection);
    }
}

void
inkscape_subselection_changed (SPDesktop *desktop)
{
    if (Inkscape::NSApplication::Application::getNewGui()) {
        Inkscape::NSApplication::Editor::subSelectionChanged (desktop);
        return;
    }
    g_return_if_fail (desktop != NULL);

    if (DESKTOP_IS_ACTIVE (desktop)) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SUBSELECTION], 0, desktop);
    }
}


void
inkscape_selection_set (Inkscape::Selection * selection)
{
    if (Inkscape::NSApplication::Application::getNewGui()) {
        Inkscape::NSApplication::Editor::selectionSet (selection);
        return;
    }
    g_return_if_fail (selection != NULL);

    if (DESKTOP_IS_ACTIVE (selection->desktop())) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_SELECTION], 0, selection);
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SELECTION], 0, selection);
    }
}


void
inkscape_eventcontext_set (SPEventContext * eventcontext)
{
    if (Inkscape::NSApplication::Application::getNewGui()) {
        Inkscape::NSApplication::Editor::eventContextSet (eventcontext);
        return;
    }
    g_return_if_fail (eventcontext != NULL);
    g_return_if_fail (SP_IS_EVENT_CONTEXT (eventcontext));

    if (DESKTOP_IS_ACTIVE (eventcontext->desktop)) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_EVENTCONTEXT], 0, eventcontext);
    }
}


void
inkscape_add_desktop (SPDesktop * desktop)
{
    g_return_if_fail (desktop != NULL);

    if (Inkscape::NSApplication::Application::getNewGui())
    {
        Inkscape::NSApplication::Editor::addDesktop (desktop);
        return;
    }
    g_return_if_fail (inkscape != NULL);

    g_assert (!g_slist_find (inkscape->desktops, desktop));

    inkscape->desktops = g_slist_append (inkscape->desktops, desktop);

    if (DESKTOP_IS_ACTIVE (desktop)) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[ACTIVATE_DESKTOP], 0, desktop);
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_EVENTCONTEXT], 0, sp_desktop_event_context (desktop));
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_SELECTION], 0, sp_desktop_selection (desktop));
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SELECTION], 0, sp_desktop_selection (desktop));
    }
}



void
inkscape_remove_desktop (SPDesktop * desktop)
{
    g_return_if_fail (desktop != NULL);
    if (Inkscape::NSApplication::Application::getNewGui())
    {
        Inkscape::NSApplication::Editor::removeDesktop (desktop);
        return;
    }
    g_return_if_fail (inkscape != NULL);

    g_assert (g_slist_find (inkscape->desktops, desktop));

    if (DESKTOP_IS_ACTIVE (desktop)) {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[DEACTIVATE_DESKTOP], 0, desktop);
        if (inkscape->desktops->next != NULL) {
            SPDesktop * new_desktop = (SPDesktop *) inkscape->desktops->next->data;
            inkscape->desktops = g_slist_remove (inkscape->desktops, new_desktop);
            inkscape->desktops = g_slist_prepend (inkscape->desktops, new_desktop);
            g_signal_emit (G_OBJECT (inkscape), inkscape_signals[ACTIVATE_DESKTOP], 0, new_desktop);
            g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_EVENTCONTEXT], 0, sp_desktop_event_context (new_desktop));
            g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_SELECTION], 0, sp_desktop_selection (new_desktop));
            g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SELECTION], 0, sp_desktop_selection (new_desktop));
        } else {
            g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_EVENTCONTEXT], 0, NULL);
            if (sp_desktop_selection(desktop))
                sp_desktop_selection(desktop)->clear();
        }
    }

    inkscape->desktops = g_slist_remove (inkscape->desktops, desktop);

    // if this was the last desktop, shut down the program
    if (inkscape->desktops == NULL) {
        inkscape_exit (inkscape);
    }
}



void
inkscape_activate_desktop (SPDesktop * desktop)
{
    g_return_if_fail (desktop != NULL);
    if (Inkscape::NSApplication::Application::getNewGui())
    {
        Inkscape::NSApplication::Editor::activateDesktop (desktop);
        return;
    }
    g_return_if_fail (inkscape != NULL);

    if (DESKTOP_IS_ACTIVE (desktop)) {
        return;
    }

    g_assert (g_slist_find (inkscape->desktops, desktop));

    SPDesktop *current = (SPDesktop *) inkscape->desktops->data;

    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[DEACTIVATE_DESKTOP], 0, current);

    inkscape->desktops = g_slist_remove (inkscape->desktops, desktop);
    inkscape->desktops = g_slist_prepend (inkscape->desktops, desktop);

    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[ACTIVATE_DESKTOP], 0, desktop);
    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_EVENTCONTEXT], 0, sp_desktop_event_context (desktop));
    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[SET_SELECTION], 0, sp_desktop_selection (desktop));
    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[CHANGE_SELECTION], 0, sp_desktop_selection (desktop));
}


/**
 *  Resends ACTIVATE_DESKTOP for current desktop; needed when a new desktop has got its window that dialogs will transientize to
 */
void
inkscape_reactivate_desktop (SPDesktop * desktop)
{
    g_return_if_fail (desktop != NULL);
    if (Inkscape::NSApplication::Application::getNewGui())
    {
        Inkscape::NSApplication::Editor::reactivateDesktop (desktop);
        return;
    }
    g_return_if_fail (inkscape != NULL);

    if (DESKTOP_IS_ACTIVE (desktop))
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[ACTIVATE_DESKTOP], 0, desktop);
}



SPDesktop *
inkscape_find_desktop_by_dkey (unsigned int dkey)
{
    for (GSList *r = inkscape->desktops; r; r = r->next) {
        if (((SPDesktop *) r->data)->dkey == dkey)
            return ((SPDesktop *) r->data);
    }
    return NULL;
}




unsigned int
inkscape_maximum_dkey()
{
    unsigned int dkey = 0;

    for (GSList *r = inkscape->desktops; r; r = r->next) {
        if (((SPDesktop *) r->data)->dkey > dkey)
            dkey = ((SPDesktop *) r->data)->dkey;
    }

    return dkey;
}



SPDesktop *
inkscape_next_desktop ()
{
    SPDesktop *d = NULL;
    unsigned int dkey_current = ((SPDesktop *) inkscape->desktops->data)->dkey;

    if (dkey_current < inkscape_maximum_dkey()) {
        // find next existing
        for (unsigned int i = dkey_current + 1; i <= inkscape_maximum_dkey(); i++) {
            d = inkscape_find_desktop_by_dkey (i);
            if (d) {
                break;
            }
        }
    } else {
        // find first existing
        for (unsigned int i = 0; i <= inkscape_maximum_dkey(); i++) {
            d = inkscape_find_desktop_by_dkey (i);
            if (d) {
                break;
            }
        }
    }

    g_assert (d);

    return d;
}



SPDesktop *
inkscape_prev_desktop ()
{
    SPDesktop *d = NULL;
    unsigned int dkey_current = ((SPDesktop *) inkscape->desktops->data)->dkey;

    if (dkey_current > 0) {
        // find prev existing
        for (signed int i = dkey_current - 1; i >= 0; i--) {
            d = inkscape_find_desktop_by_dkey (i);
            if (d) {
                break;
            }
        }
    }
    if (!d) {
        // find last existing
        d = inkscape_find_desktop_by_dkey (inkscape_maximum_dkey());
    }

    g_assert (d);

    return d;
}



void
inkscape_switch_desktops_next ()
{
    inkscape_next_desktop()->presentWindow();
}



void
inkscape_switch_desktops_prev ()
{
    inkscape_prev_desktop()->presentWindow();
}



void
inkscape_dialogs_hide ()
{
    if (Inkscape::NSApplication::Application::getNewGui())
        Inkscape::NSApplication::Editor::hideDialogs();
    else
    {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[DIALOGS_HIDE], 0);
        inkscape->dialogs_toggle = FALSE;
    }
}



void
inkscape_dialogs_unhide ()
{
    if (Inkscape::NSApplication::Application::getNewGui())
        Inkscape::NSApplication::Editor::unhideDialogs();
    else
    {
        g_signal_emit (G_OBJECT (inkscape), inkscape_signals[DIALOGS_UNHIDE], 0);
        inkscape->dialogs_toggle = TRUE;
    }
}



void
inkscape_dialogs_toggle ()
{
    if (inkscape->dialogs_toggle) {
        inkscape_dialogs_hide ();
    } else {
        inkscape_dialogs_unhide ();
    }
}

void
inkscape_external_change ()
{
    g_return_if_fail (inkscape != NULL);

    g_signal_emit (G_OBJECT (inkscape), inkscape_signals[EXTERNAL_CHANGE], 0);
}

/**
 * fixme: These need probably signals too
 */
void
inkscape_add_document (SPDocument *document)
{
    g_return_if_fail (document != NULL);

    if (!Inkscape::NSApplication::Application::getNewGui())
    {
        g_assert (!g_slist_find (inkscape->documents, document));
        inkscape->documents = g_slist_append (inkscape->documents, document);
    }
    else
    {
        Inkscape::NSApplication::Editor::addDocument (document);
    }
}



void
inkscape_remove_document (SPDocument *document)
{
    g_return_if_fail (document != NULL);

    if (!Inkscape::NSApplication::Application::getNewGui())
    {
        g_assert (g_slist_find (inkscape->documents, document));
        inkscape->documents = g_slist_remove (inkscape->documents, document);
    }
    else
    {
        Inkscape::NSApplication::Editor::removeDocument (document);
    }

    return;
}

SPDesktop *
inkscape_active_desktop (void)
{
    if (Inkscape::NSApplication::Application::getNewGui())
        return Inkscape::NSApplication::Editor::getActiveDesktop();

    if (inkscape->desktops == NULL) {
        return NULL;
    }

    return (SPDesktop *) inkscape->desktops->data;
}

SPDocument *
inkscape_active_document (void)
{
    if (Inkscape::NSApplication::Application::getNewGui())
        return Inkscape::NSApplication::Editor::getActiveDocument();

    if (SP_ACTIVE_DESKTOP) {
        return sp_desktop_document (SP_ACTIVE_DESKTOP);
    }

    return NULL;
}

bool inkscape_is_sole_desktop_for_document(SPDesktop const &desktop) {
    SPDocument const* document = desktop.doc();
    if (!document) {
        return false;
    }
    for ( GSList *iter = inkscape->desktops ; iter ; iter = iter->next ) {
        SPDesktop *other_desktop=(SPDesktop *)iter->data;
        SPDocument *other_document=other_desktop->doc();
        if ( other_document == document && other_desktop != &desktop ) {
            return false;
        }
    }
    return true;
}

SPEventContext *
inkscape_active_event_context (void)
{
    if (SP_ACTIVE_DESKTOP) {
        return sp_desktop_event_context (SP_ACTIVE_DESKTOP);
    }

    return NULL;
}



/*#####################
# HELPERS
#####################*/

static bool
inkscape_init_config (Inkscape::XML::Document *doc, const gchar *config_name, const gchar *skeleton,
		      unsigned int skel_size,
		      const gchar *e_mkdir,
		      const gchar *e_notdir,
		      const gchar *e_ccf,
		      const gchar *e_cwf,
		      const gchar *warn)
{
    gchar *dn = profile_path(NULL);
    bool use_gui = (Inkscape::NSApplication::Application::getNewGui())? Inkscape::NSApplication::Application::getUseGui() : inkscape->use_gui;
    if (!Inkscape::IO::file_test(dn, G_FILE_TEST_EXISTS)) {
        if (Inkscape::IO::mkdir_utf8name(dn))
        {
            if (use_gui) {
                // Cannot create directory
                gchar *safeDn = Inkscape::IO::sanitizeString(dn);
                GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_mkdir, safeDn, warn);
                gtk_dialog_run (GTK_DIALOG (w));
                gtk_widget_destroy (w);
                g_free(safeDn);
                g_free (dn);
                return false;
            } else {
                g_warning(e_mkdir, dn, warn);
                g_free (dn);
                return false;
            }
        }
    } else if (!Inkscape::IO::file_test(dn, G_FILE_TEST_IS_DIR)) {
        if (use_gui) {
            // Not a directory
            gchar *safeDn = Inkscape::IO::sanitizeString(dn);
            GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_notdir, safeDn, warn);
            gtk_dialog_run (GTK_DIALOG (w));
            gtk_widget_destroy (w);
            g_free( safeDn );
            g_free (dn);
            return false;
        } else {
            g_warning(e_notdir, dn, warn);
            g_free(dn);
            return false;
        }
    }
    g_free (dn);

    gchar *fn = profile_path(config_name);

    Inkscape::IO::dump_fopen_call(fn, "H");
    FILE *fh = Inkscape::IO::fopen_utf8name(fn, "w");
    if (!fh) {
        if (use_gui) {
            /* Cannot create file */
            gchar *safeFn = Inkscape::IO::sanitizeString(fn);
            GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_ccf, safeFn, warn);
            gtk_dialog_run (GTK_DIALOG (w));
            gtk_widget_destroy (w);
            g_free(safeFn);
            g_free (fn);
            return false;
        } else {
            g_warning(e_ccf, fn, warn);
            g_free(fn);
            return false;
        }
    }
    if ( fwrite(skeleton, 1, skel_size, fh) != skel_size ) {
        if (use_gui) {
            /* Cannot create file */
            gchar *safeFn = Inkscape::IO::sanitizeString(fn);
            GtkWidget *w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, e_cwf, safeFn, warn);
            gtk_dialog_run (GTK_DIALOG (w));
            gtk_widget_destroy (w);
            g_free(safeFn);
            g_free (fn);
            fclose(fh);
            return false;
        } else {
            g_warning(e_cwf, fn, warn);
            g_free(fn);
            fclose(fh);
            return false;
        }
    }

    g_free(fn);
    fclose(fh);
    return true;
}

void
inkscape_refresh_display (Inkscape::Application *inkscape)
{
    for (GSList *l = inkscape->desktops; l != NULL; l = l->next) {
        (static_cast<Inkscape::UI::View::View*>(l->data))->requestRedraw();
    }
}


/**
 *  Handler for Inkscape's Exit verb.  This emits the shutdown signal,
 *  saves the preferences if appropriate, and quits.
 */
void
inkscape_exit (Inkscape::Application *inkscape)
{
    g_assert (INKSCAPE);

    //emit shutdown signal so that dialogs could remember layout
    g_signal_emit (G_OBJECT (INKSCAPE), inkscape_signals[SHUTDOWN_SIGNAL], 0);

    Inkscape::Preferences::save();
    gtk_main_quit ();
}

gchar *
homedir_path(const char *filename)
{
    static const gchar *homedir = NULL;
    if (!homedir) {
        homedir = g_get_home_dir();
        gchar* utf8Path = g_filename_to_utf8( homedir, -1, NULL, NULL, NULL );
        if ( utf8Path )
        {
                homedir = utf8Path;
                if (!g_utf8_validate(homedir, -1, NULL)) {
                    g_warning( "g_get_home_dir() post A IS NOT UTF-8" );
                }
        }
    }
    if (!homedir) {
        gchar * path = g_path_get_dirname(INKSCAPE->argv0);
        gchar* utf8Path = g_filename_to_utf8( path, -1, NULL, NULL, NULL );
        g_free(path);
        if ( utf8Path )
        {
            homedir = utf8Path;
            if (!g_utf8_validate(homedir, -1, NULL)) {
                g_warning( "g_get_home_dir() post B IS NOT UTF-8" );
            }
        }
    }
    return g_build_filename(homedir, filename, NULL);
}


/**
 * Get, or guess, or decide the location where the preferences.xml
 * file should be located.
 */
gchar *
profile_path(const char *filename)
{
    static const gchar *prefdir = NULL;
    if (!prefdir) {
#ifdef HAS_SHGetSpecialFolderLocation
        // prefer c:\Documents and Settings\UserName\Application Data\ to
        // c:\Documents and Settings\userName\;
        if (!prefdir) {
            ITEMIDLIST *pidl = 0;
            if ( SHGetSpecialFolderLocation( NULL, CSIDL_APPDATA, &pidl ) == NOERROR ) {
                gchar * utf8Path = NULL;

                if ( PrintWin32::is_os_wide() ) {
                    wchar_t pathBuf[MAX_PATH+1];
                    g_assert(sizeof(wchar_t) == sizeof(gunichar2));

                    if ( SHGetPathFromIDListW( pidl, pathBuf ) ) {
                        utf8Path = g_utf16_to_utf8( (gunichar2*)(&pathBuf[0]), -1, NULL, NULL, NULL );
                    }
                } else {
                    char pathBuf[MAX_PATH+1];

                    if ( SHGetPathFromIDListA( pidl, pathBuf ) ) {
                        utf8Path = g_filename_to_utf8( pathBuf, -1, NULL, NULL, NULL );
                    }
                }

                if ( utf8Path ) {
                    if (!g_utf8_validate(utf8Path, -1, NULL)) {
                        g_warning( "SHGetPathFromIDList%c() resulted in invalid UTF-8", (PrintWin32::is_os_wide() ? 'W' : 'A') );
                        g_free( utf8Path );
                        utf8Path = 0;
                    } else {
                        prefdir = utf8Path;
                    }
                }


                /* not compiling yet...

                // Remember to free the list pointer
                IMalloc * imalloc = 0;
                if ( SHGetMalloc(&imalloc) == NOERROR) {
                    imalloc->lpVtbl->Free( imalloc, pidl );
                    imalloc->lpVtbl->Release( imalloc );
                }
                */
            }
        }
#endif
        if (!prefdir) {
            prefdir = homedir_path(NULL);
        }
    }
    return g_build_filename(prefdir, INKSCAPE_PROFILE_DIR, filename, NULL);
}

Inkscape::XML::Node *
inkscape_get_menus (Inkscape::Application * inkscape)
{
    Inkscape::XML::Node *repr = sp_repr_document_root (inkscape->menus);
    g_assert (!(strcmp (repr->name(), "inkscape")));
    return repr->firstChild();
}



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
