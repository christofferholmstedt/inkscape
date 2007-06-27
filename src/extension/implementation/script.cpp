/** \file
 * Code for handling extensions (i.e.\ scripts).
 */
/*
 * Authors:
 *   Bryce Harrington <bryce@osdl.org>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
TODO:
FIXME:
  After Inkscape makes a formal requirement for a GTK version above 2.11.4, please
  replace all the instances of ink_ext_XXXXXX in this file that represent
  svg files with ink_ext_XXXXXX.svg . Doing so will prevent errors in extensions
  that call inkscape to manipulate the file.
  
  "** (inkscape:5848): WARNING **: Format autodetect failed. The file is being opened as SVG."
  
  references:
  http://www.gtk.org/api/2.6/glib/glib-File-Utilities.html#g-mkstemp
  http://ftp.gnome.org/pub/gnome/sources/glib/2.11/glib-2.11.4.changes
  http://developer.gnome.org/doc/API/2.0/glib/glib-File-Utilities.html#g-mkstemp
  
  --Aaron Spike
*/
#define __INKSCAPE_EXTENSION_IMPLEMENTATION_SCRIPT_C__

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>

#include <errno.h>
#include <gtkmm.h>

#include "ui/view/view.h"
#include "desktop-handles.h"
#include "selection.h"
#include "sp-namedview.h"
#include "io/sys.h"
#include "prefs-utils.h"
#include "../system.h"
#include "extension/effect.h"
#include "extension/output.h"
#include "extension/db.h"
#include "script.h"
#include "dialogs/dialog-events.h"

#include "util/glib-list-iterators.h"



#ifdef WIN32
#include <windows.h>
#include <sys/stat.h>
#include "registrytool.h"
#endif



/** This is the command buffer that gets allocated from the stack */
#define BUFSIZE (255)



/* Namespaces */
namespace Inkscape {
namespace Extension {
namespace Implementation {



//Interpreter lookup table
struct interpreter_t {
        gchar * identity;
        gchar * prefstring;
        gchar * defaultval;
};


/** \brief  A table of what interpreters to call for a given language

    This table is used to keep track of all the programs to execute a
    given script.  It also tracks the preference to use to overwrite
    the given interpreter to a custom one per user.
*/
static interpreter_t interpreterTab[] = {
        {"perl",   "perl-interpreter",   "perl"   },
        {"python", "python-interpreter", "python" },
        {"ruby",   "ruby-interpreter",   "ruby"   },
        {"shell",  "shell-interpreter",  "sh"     },
        { NULL,    NULL,                  NULL    }
};



/**
 * Look up an interpreter name, and translate to something that
 * is executable
 */
static Glib::ustring
resolveInterpreterExecutable(const Glib::ustring &interpNameArg)
{

    Glib::ustring interpName = interpNameArg;

    interpreter_t *interp;
    bool foundInterp = false;
    for (interp =  interpreterTab ; interp->identity ; interp++ ){
        if (interpName == interp->identity) {
            foundInterp = true;
            break;
        }
    }

    // Do we have a supported interpreter type?
    if (!foundInterp)
        return "";
    interpName = interp->defaultval;

    // 1.  Check preferences
    gchar *prefInterp = (gchar *)prefs_get_string_attribute(
                                "extensions", interp->prefstring);

    if (prefInterp) {
        interpName = prefInterp;
        return interpName;
    }

#ifdef _WIN32

    // 2.  Windows.  Try looking relative to inkscape.exe
    RegistryTool rt;
    Glib::ustring fullPath;
    Glib::ustring path;
    Glib::ustring exeName;
    if (rt.getExeInfo(fullPath, path, exeName)) {
        Glib::ustring interpPath = path;
        interpPath.append("\\");
        interpPath.append(interpName);
        interpPath.append("\\");
        interpPath.append(interpName);
        interpPath.append(".exe");
        struct stat finfo;
        if (stat(interpPath .c_str(), &finfo) ==0) {
            g_message("Found local interpreter, '%s',  Size: %d",
                      interpPath .c_str(),
                      (int)finfo.st_size);
            return interpPath;
        }                       
    }

    // 3. Try searching the path
    char szExePath[MAX_PATH];
    char szCurrentDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurrentDir), szCurrentDir);
    unsigned int ret = (unsigned int)FindExecutable(
                  interpName.c_str(), szCurrentDir, szExePath);
    if (ret > 32) {
        interpName = szExePath;
        return interpName;
    }

#endif // win32


    return interpName;
}


class file_listener {
    Glib::ustring _string;
    sigc::connection _conn;
    Glib::RefPtr<Glib::IOChannel> _channel;
    Glib::RefPtr<Glib::MainLoop> _main_loop;
    
public:
    file_listener () { };
    ~file_listener () {
        _conn.disconnect();
    };

    void init (int fd, Glib::RefPtr<Glib::MainLoop> main) {
        _channel = Glib::IOChannel::create_from_fd(fd);
        _channel->set_encoding();
        _conn = Glib::signal_io().connect(sigc::mem_fun(*this, &file_listener::read), _channel, Glib::IO_IN | Glib::IO_HUP | Glib::IO_ERR);
        _main_loop = main;

        return;
    };

    bool read (Glib::IOCondition condition) {
        if (condition != Glib::IO_IN) {
            _main_loop->quit();
            return false;
        }

        Glib::IOStatus status;
        Glib::ustring out;
        status = _channel->read_to_end(out);

        if (status != Glib::IO_STATUS_NORMAL) {
            _main_loop->quit();
            return false;
        }

        _string += out;
        return true;
    };

    // Note, doing a copy here, on purpose
    Glib::ustring string (void) { return _string; };

    void toFile (const Glib::ustring &name) {
        Glib::RefPtr<Glib::IOChannel> stdout_file = Glib::IOChannel::create_from_file(name, "w");
        stdout_file->write(_string);
        return;
    };
};

int execute (const std::list<std::string> &in_command,
             const std::list<std::string> &in_params,
             const Glib::ustring &filein,
             file_listener &fileout);
void checkStderr (const Glib::ustring &data,
                        Gtk::MessageType type,
                  const Glib::ustring &message);


/** \brief     This function creates a script object and sets up the
               variables.
    \return    A script object

   This function just sets the command to NULL.  It should get built
   officially in the load function.  This allows for less allocation
   of memory in the unloaded state.
*/
Script::Script() :
    Implementation()
{
}


/**
 *   brief     Destructor
 */
Script::~Script()
{
}



/**
    \return    A string with the complete string with the relative directory expanded
    \brief     This function takes in a Repr that contains a reldir entry
               and returns that data with the relative directory expanded.
               Mostly it is here so that relative directories all get used
               the same way.
    \param     reprin   The Inkscape::XML::Node with the reldir in it.

    Basically this function looks at an attribute of the Repr, and makes
    a decision based on that.  Currently, it is only working with the
    'extensions' relative directory, but there will be more of them.
    One thing to notice is that this function always returns an allocated
    string.  This means that the caller of this function can always
    free what they are given (and should do it too!).
*/
Glib::ustring
Script::solve_reldir(Inkscape::XML::Node *reprin) {

    gchar const *s = reprin->attribute("reldir");

    if (!s) {
        Glib::ustring str = sp_repr_children(reprin)->content();
        return str;
    }

    Glib::ustring reldir = s;

    if (reldir == "extensions") {

        for (unsigned int i=0;
            i < Inkscape::Extension::Extension::search_path.size();
            i++) {

            gchar * fname = g_build_filename(
               Inkscape::Extension::Extension::search_path[i],
               sp_repr_children(reprin)->content(),
               NULL);
            Glib::ustring filename = fname;
            g_free(fname);

            if ( Inkscape::IO::file_test(filename.c_str(), G_FILE_TEST_EXISTS) )
                return filename;

        }
    } else {
        Glib::ustring str = sp_repr_children(reprin)->content();
        return str;
    }

    return "";
}



/**
    \return   Whether the command given exists, including in the path
    \brief    This function is used to find out if something exists for
              the check command.  It can look in the path if required.
    \param    command   The command or file that should be looked for

    The first thing that this function does is check to see if the
    incoming file name has a directory delimiter in it.  This would
    mean that it wants to control the directories, and should be
    used directly.

    If not, the path is used.  Each entry in the path is stepped through,
    attached to the string, and then tested.  If the file is found
    then a TRUE is returned.  If we get all the way through the path
    then a FALSE is returned, the command could not be found.
*/
bool
Script::check_existance(const Glib::ustring &command)
{

    // Check the simple case first
    if (command.size() == 0) {
        return false;
    }

    //Don't search when it contains a slash. */
    if (command.find(G_DIR_SEPARATOR) != command.npos) {
        if (Inkscape::IO::file_test(command.c_str(), G_FILE_TEST_EXISTS))
            return true;
        else
            return false;
    }


    Glib::ustring path; 
    gchar *s = (gchar *) g_getenv("PATH");
    if (s)
        path = s;
    else
       /* There is no `PATH' in the environment.
           The default search path is the current directory */
        path = G_SEARCHPATH_SEPARATOR_S;

    std::string::size_type pos  = 0;
    std::string::size_type pos2 = 0;
    while ( pos < path.size() ) {

        Glib::ustring localPath;

        pos2 = path.find(G_SEARCHPATH_SEPARATOR, pos);
        if (pos2 == path.npos) {
            localPath = path.substr(pos);
            pos = path.size();
        } else {
            localPath = path.substr(pos, pos2-pos);
            pos = pos2+1;
        }
        
        //printf("### %s\n", localPath.c_str());
        Glib::ustring candidatePath = 
                      Glib::build_filename(localPath, command);

        if (Inkscape::IO::file_test(candidatePath .c_str(),
                      G_FILE_TEST_EXISTS)) {
            return true;
        }

    }

    return false;
}





/**
    \return   none
    \brief    This function 'loads' an extention, basically it determines
              the full command for the extention and stores that.
    \param    module  The extention to be loaded.

    The most difficult part about this function is finding the actual
    command through all of the Reprs.  Basically it is hidden down a
    couple of layers, and so the code has to move down too.  When
    the command is actually found, it has its relative directory
    solved.

    At that point all of the loops are exited, and there is an
    if statement to make sure they didn't exit because of not finding
    the command.  If that's the case, the extention doesn't get loaded
    and should error out at a higher level.
*/

bool
Script::load(Inkscape::Extension::Extension *module)
{
    if (module->loaded())
        return TRUE;

    helper_extension = "";

    /* This should probably check to find the executable... */
    Inkscape::XML::Node *child_repr = sp_repr_children(module->get_repr());
    while (child_repr != NULL) {
        if (!strcmp(child_repr->name(), "script")) {
            child_repr = sp_repr_children(child_repr);
            while (child_repr != NULL) {
                if (!strcmp(child_repr->name(), "command")) {
                    const gchar *interpretstr = child_repr->attribute("interpreter");
                    if (interpretstr != NULL) {
                        Glib::ustring interpString =
                            resolveInterpreterExecutable(interpretstr);
                        command.insert(command.end(), interpretstr);
                    }

                    command.insert(command.end(), solve_reldir(child_repr));
                }
                if (!strcmp(child_repr->name(), "helper_extension")) {
                    helper_extension = sp_repr_children(child_repr)->content();
                }
                child_repr = sp_repr_next(child_repr);
            }

            break;
        }
        child_repr = sp_repr_next(child_repr);
    }

    //g_return_val_if_fail(command.length() > 0, FALSE);

    return true;
}


/**
    \return   None.
    \brief    Unload this puppy!
    \param    module  Extension to be unloaded.

    This function just sets the module to unloaded.  It free's the
    command if it has been allocated.
*/
void
Script::unload(Inkscape::Extension::Extension *module)
{
    command.clear();
    helper_extension = "";
}




/**
    \return   Whether the check passed or not
    \brief    Check every dependency that was given to make sure we should keep this extension
    \param    module  The Extension in question

*/
bool
Script::check(Inkscape::Extension::Extension *module)
{
    Inkscape::XML::Node *child_repr = sp_repr_children(module->get_repr());
    while (child_repr != NULL) {
        if (!strcmp(child_repr->name(), "script")) {
            child_repr = sp_repr_children(child_repr);
            while (child_repr != NULL) {
                if (!strcmp(child_repr->name(), "check")) {
                    Glib::ustring command_text = solve_reldir(child_repr);
                    if (command_text.size() > 0) {
                        /* I've got the command */
                        bool existance = check_existance(command_text);
                        if (!existance)
                            return FALSE;
                    }
                }

                if (!strcmp(child_repr->name(), "helper_extension")) {
                    gchar const *helper = sp_repr_children(child_repr)->content();
                    if (Inkscape::Extension::db.get(helper) == NULL) {
                        return FALSE;
                    }
                }

                child_repr = sp_repr_next(child_repr);
            }

            break;
        }
        child_repr = sp_repr_next(child_repr);
    }

    return true;
}



/**
    \return   A dialog for preferences
    \brief    A stub funtion right now
    \param    module    Module who's preferences need getting
    \param    filename  Hey, the file you're getting might be important

    This function should really do something, right now it doesn't.
*/
Gtk::Widget *
Script::prefs_input(Inkscape::Extension::Input *module,
                    const gchar *filename)
{
    /*return module->autogui(); */
    return NULL;
}



/**
    \return   A dialog for preferences
    \brief    A stub funtion right now
    \param    module    Module whose preferences need getting

    This function should really do something, right now it doesn't.
*/
Gtk::Widget *
Script::prefs_output(Inkscape::Extension::Output *module)
{
    return module->autogui(NULL, NULL); 
}



/**
    \return   A dialog for preferences
    \brief    A stub funtion right now
    \param    module    Module who's preferences need getting

    This function should really do something, right now it doesn't.
*/
Gtk::Widget *
Script::prefs_effect(Inkscape::Extension::Effect *module,
                     Inkscape::UI::View::View *view,
                     sigc::signal<void> * changeSignal)
{
    SPDocument * current_document = view->doc();

    using Inkscape::Util::GSListConstIterator;
    GSListConstIterator<SPItem *> selected =
           sp_desktop_selection((SPDesktop *)view)->itemList();
    Inkscape::XML::Node * first_select = NULL;
    if (selected != NULL) {
        const SPItem * item = *selected;
        first_select = SP_OBJECT_REPR(item);
    }

    return module->autogui(current_document, first_select);
}




/**
    \return  A new document that has been opened
    \brief   This function uses a filename that is put in, and calls
             the extension's command to create an SVG file which is
             returned.
    \param   module   Extension to use.
    \param   filename File to open.

    First things first, this function needs a temporary file name.  To
    create on of those the function g_file_open_tmp is used with
    the header of ink_ext_.

    The extension is then executed using the 'execute' function
    with the filname coming in, and the temporary filename.  After
    That executing, the SVG should be in the temporary file.

    Finally, the temporary file is opened using the SVG input module and
    a document is returned.  That document has its filename set to
    the incoming filename (so that it's not the temporary filename).
    That document is then returned from this function.
*/
SPDocument *
Script::open(Inkscape::Extension::Input *module,
             const gchar *filenameArg)
{
#if 0
    Glib::ustring filename = filenameArg;

    gchar *tmpname;

    // FIXME: process the GError instead of passing NULL
    gint tempfd = g_file_open_tmp("ink_ext_XXXXXX", &tmpname, NULL);
    if (tempfd == -1) {
        /* Error, couldn't create temporary filename */
        if (errno == EINVAL) {
            /* The  last  six characters of template were not XXXXXX.  Now template is unchanged. */
            perror("Extension::Script:  template for filenames is misconfigured.\n");
            exit(-1);
        } else if (errno == EEXIST) {
            /* Now the  contents of template are undefined. */
            perror("Extension::Script:  Could not create a unique temporary filename\n");
            return NULL;
        } else {
            perror("Extension::Script:  Unknown error creating temporary filename\n");
            exit(-1);
        }
    }

    Glib::ustring tempfilename_out = tmpname;
    g_free(tmpname);

    gsize bytesRead = 0;
    gsize bytesWritten = 0;
    GError *error = NULL;
    Glib::ustring local_filename =
            g_filename_from_utf8( filename.c_str(), -1,
                                  &bytesRead,  &bytesWritten, &error);

    int data_read = execute(command, local_filename, tempfilename_out);

    SPDocument *mydoc = NULL;
    if (data_read > 10) {
        if (helper_extension.size()==0) {
            mydoc = Inkscape::Extension::open(
                Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG),
                                            tempfilename_out.c_str());
        } else {
            mydoc = Inkscape::Extension::open(
                Inkscape::Extension::db.get(helper_extension.c_str()),
                                            tempfilename_out.c_str());
        }
    }

    if (mydoc != NULL)
        sp_document_set_uri(mydoc, (const gchar *)filename.c_str());

    // make sure we don't leak file descriptors from g_file_open_tmp
    close(tempfd);
    // FIXME: convert to utf8 (from "filename encoding") and unlink_utf8name
    unlink(tempfilename_out.c_str());


    return mydoc;
#endif
}



/**
    \return   none
    \brief    This function uses an extention to save a document.  It first
              creates an SVG file of the document, and then runs it through
              the script.
    \param    module    Extention to be used
    \param    doc       Document to be saved
    \param    filename  The name to save the final file as

    Well, at some point people need to save - it is really what makes
    the entire application useful.  And, it is possible that someone
    would want to use an extetion for this, so we need a function to
    do that eh?

    First things first, the document is saved to a temporary file that
    is an SVG file.  To get the temporary filename g_file_open_tmp is used with
    ink_ext_ as a prefix.  Don't worry, this file gets deleted at the
    end of the function.

    After we have the SVG file, then extention_execute is called with
    the temporary file name and the final output filename.  This should
    put the output of the script into the final output file.  We then
    delete the temporary file.
*/
void
Script::save(Inkscape::Extension::Output *module,
             SPDocument *doc,
             const gchar *filenameArg)
{
#if 0
    Glib::ustring filename = filenameArg;

    gchar *tmpname;
    // FIXME: process the GError instead of passing NULL
    gint tempfd = g_file_open_tmp("ink_ext_XXXXXX", &tmpname, NULL);
    if (tempfd == -1) {
        /* Error, couldn't create temporary filename */
        if (errno == EINVAL) {
            /* The  last  six characters of template were not XXXXXX.  Now template is unchanged. */
            perror("Extension::Script:  template for filenames is misconfigured.\n");
            exit(-1);
        } else if (errno == EEXIST) {
            /* Now the  contents of template are undefined. */
            perror("Extension::Script:  Could not create a unique temporary filename\n");
            return;
        } else {
            perror("Extension::Script:  Unknown error creating temporary filename\n");
            exit(-1);
        }
    }

    Glib::ustring tempfilename_in = tmpname;
    g_free(tmpname);

    if (helper_extension.size() == 0) {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
                   doc, tempfilename_in.c_str(), FALSE, FALSE, FALSE);
    } else {
        Inkscape::Extension::save(
                   Inkscape::Extension::db.get(helper_extension.c_str()),
                   doc, tempfilename_in.c_str(), FALSE, FALSE, FALSE);
    }

    gsize bytesRead = 0;
    gsize bytesWritten = 0;
    GError *error = NULL;
    Glib::ustring local_filename =
            g_filename_from_utf8( filename.c_str(), -1,
                                 &bytesRead,  &bytesWritten, &error);

    Glib::ustring local_command = command;
    Glib::ustring paramString   = *module->paramString();
    local_command.append(paramString);

    execute(local_command, tempfilename_in, local_filename);


    // make sure we don't leak file descriptors from g_file_open_tmp
    close(tempfd);
    // FIXME: convert to utf8 (from "filename encoding") and unlink_utf8name
    unlink(tempfilename_in.c_str());
#endif
}



/**
    \return    none
    \brief     This function uses an extention as a effect on a document.
    \param     module   Extention to effect with.
    \param     doc      Document to run through the effect.

    This function is a little bit trickier than the previous two.  It
    needs two temporary files to get it's work done.  Both of these
    files have random names created for them using the g_file_open_temp function
    with the ink_ext_ prefix in the temporary directory.  Like the other
    functions, the temporary files are deleted at the end.

    To save/load the two temporary documents (both are SVG) the internal
    modules for SVG load and save are used.  They are both used through
    the module system function by passing their keys into the functions.

    The command itself is built a little bit differently than in other
    functions because the effect support selections.  So on the command
    line a list of all the ids that are selected is included.  Currently,
    this only works for a single selected object, but there will be more.
    The command string is filled with the data, and then after the execution
    it is freed.

    The execute function is used at the core of this function
    to execute the Script on the two SVG documents (actually only one
    exists at the time, the other is created by that script).  At that
    point both should be full, and the second one is loaded.
*/
void
Script::effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *doc)
{
    std::list<std::string> params;

    if (module->no_doc) { 
        // this is a no-doc extension, e.g. a Help menu command; 
        // just run the command without any files, ignoring errors
        module->paramListString(params);

        Glib::ustring empty;
        file_listener outfile;
        execute(command, params, empty, outfile);

        return;
    }

    gchar *tmpname;
    // FIXME: process the GError instead of passing NULL
    gint tempfd_in = g_file_open_tmp("ink_ext_XXXXXX", &tmpname, NULL);
    if (tempfd_in == -1) {
        /* Error, couldn't create temporary filename */
        if (errno == EINVAL) {
            /* The  last  six characters of template were not XXXXXX.  Now template is unchanged. */
            perror("Extension::Script:  template for filenames is misconfigured.\n");
            exit(-1);
        } else if (errno == EEXIST) {
            /* Now the  contents of template are undefined. */
            perror("Extension::Script:  Could not create a unique temporary filename\n");
            return;
        } else {
            perror("Extension::Script:  Unknown error creating temporary filename\n");
            exit(-1);
        }
    }

    Glib::ustring tempfilename_in = tmpname;
    g_free(tmpname);


    // FIXME: process the GError instead of passing NULL
    gint tempfd_out = g_file_open_tmp("ink_ext_XXXXXX", &tmpname, NULL);
    if (tempfd_out == -1) {
        /* Error, couldn't create temporary filename */
        if (errno == EINVAL) {
            /* The  last  six characters of template were not XXXXXX.  Now template is unchanged. */
            perror("Extension::Script:  template for filenames is misconfigured.\n");
            exit(-1);
        } else if (errno == EEXIST) {
            /* Now the  contents of template are undefined. */
            perror("Extension::Script:  Could not create a unique temporary filename\n");
            return;
        } else {
            perror("Extension::Script:  Unknown error creating temporary filename\n");
            exit(-1);
        }
    }

    Glib::ustring tempfilename_out= tmpname;
    g_free(tmpname);

    SPDesktop *desktop = (SPDesktop *) doc;
    sp_namedview_document_from_window(desktop);

    Inkscape::Extension::save(
              Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE),
              doc->doc(), tempfilename_in.c_str(), FALSE, FALSE, FALSE);

    if (desktop != NULL) {
        Inkscape::Util::GSListConstIterator<SPItem *> selected =
             sp_desktop_selection(desktop)->itemList();
        while ( selected != NULL ) {
            Glib::ustring selected_id;
            selected_id += "--id=";
            selected_id += SP_OBJECT_ID(*selected);
            params.insert(params.begin(), selected_id);
            ++selected;
        }
    }

    file_listener fileout;
    int data_read = execute(command, params, tempfilename_in, fileout);
    fileout.toFile(tempfilename_out);

    SPDocument * mydoc = NULL;
    if (data_read > 10)
        mydoc = Inkscape::Extension::open(
              Inkscape::Extension::db.get(SP_MODULE_KEY_INPUT_SVG),
              tempfilename_out.c_str());

    // make sure we don't leak file descriptors from g_file_open_tmp
    close(tempfd_in);
    close(tempfd_out);

    // FIXME: convert to utf8 (from "filename encoding") and unlink_utf8name
    unlink(tempfilename_in.c_str());
    unlink(tempfilename_out.c_str());

    /* Do something with mydoc.... */
    if (mydoc) {
        doc->doc()->emitReconstructionStart();
        copy_doc(doc->doc()->rroot, mydoc->rroot);
        doc->doc()->emitReconstructionFinish();
        mydoc->release();
        sp_namedview_update_layers_from_document(desktop);
    }

    return;
}



/**
    \brief  A function to take all the svg elements from one document
            and put them in another.
    \param  oldroot  The root node of the document to be replaced
    \param  newroot  The root node of the document to replace it with

    This function first deletes all of the data in the old document.  It
    does this by creating a list of what needs to be deleted, and then
    goes through the list.  This two pass approach removes issues with
    the list being change while parsing through it.  Lots of nasty bugs.

    Then, it goes through the new document, duplicating all of the
    elements and putting them into the old document.  The copy
    is then complete.
*/
void
Script::copy_doc (Inkscape::XML::Node * oldroot, Inkscape::XML::Node * newroot)
{
    std::vector<Inkscape::XML::Node *> delete_list;
    for (Inkscape::XML::Node * child = oldroot->firstChild();
            child != NULL;
            child = child->next()) {
        if (!strcmp("sodipodi:namedview", child->name()))
            continue;
        delete_list.push_back(child);
    }
    for (unsigned int i = 0; i < delete_list.size(); i++)
        sp_repr_unparent(delete_list[i]);

    for (Inkscape::XML::Node * child = newroot->firstChild();
            child != NULL;
            child = child->next()) {
        if (!strcmp("sodipodi:namedview", child->name()))
            continue;
        oldroot->appendChild(child->duplicate(newroot->document()));
    }

    /** \todo  Restore correct layer */
    /** \todo  Restore correct selection */
}

/**  \brief  This function checks the stderr file, and if it has data,
             shows it in a warning dialog to the user
     \param  filename  Filename of the stderr file
*/
void
checkStderr (const Glib::ustring &data,
                   Gtk::MessageType type,
             const Glib::ustring &message)
{
    Gtk::MessageDialog warning(message, false, type, Gtk::BUTTONS_OK, true);
    warning.set_resizable(true);
    GtkWidget *dlg = GTK_WIDGET(warning.gobj());
    sp_transientize(dlg);

    Gtk::VBox * vbox = warning.get_vbox();

    /* Gtk::TextView * textview = new Gtk::TextView(Gtk::TextBuffer::create()); */
    Gtk::TextView * textview = new Gtk::TextView();
    textview->set_editable(false);
    textview->set_wrap_mode(Gtk::WRAP_WORD);
    textview->show();

    textview->get_buffer()->set_text(data.c_str());

    Gtk::ScrolledWindow * scrollwindow = new Gtk::ScrolledWindow();
    scrollwindow->add(*textview);
    scrollwindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollwindow->set_shadow_type(Gtk::SHADOW_IN);
    scrollwindow->show();

    vbox->pack_start(*scrollwindow, true, true, 5 /* fix these */);

    warning.run();

    return;
}

/** \brief    This is the core of the extension file as it actually does
              the execution of the extension.
    \param    in_command  The command to be executed
    \param    filein      Filename coming in
    \param    fileout     Filename of the out file
    \return   Number of bytes that were read into the output file.

    The first thing that this function does is build the command to be
    executed.  This consists of the first string (in_command) and then
    the filename for input (filein).  This file is put on the command
    line.

    The next thing is that this function does is open a pipe to the
    command and get the file handle in the ppipe variable.  It then
    opens the output file with the output file handle.  Both of these
    operations are checked extensively for errors.

    After both are opened, then the data is copied from the output
    of the pipe into the file out using fread and fwrite.  These two
    functions are used because of their primitive nature they make
    no assumptions about the data.  A buffer is used in the transfer,
    but the output of fread is stored so the exact number of bytes
    is handled gracefully.

    At the very end (after the data has been copied) both of the files
    are closed, and we return to what we were doing.
*/
int
execute (const std::list<std::string> &in_command,
         const std::list<std::string> &in_params,
         const Glib::ustring &filein,
         file_listener &fileout)
{
    g_return_val_if_fail(in_command.size() > 0, 0);
    // printf("Executing: %s\n", in_command);

    std::vector <std::string> argv;

    for (std::list<std::string>::const_iterator i = in_command.begin();
            i != in_command.end(); i++) {
        argv.push_back(*i);
    }

    if (!(filein.empty())) {
        argv.push_back(filein);
    }

    for (std::list<std::string>::const_iterator i = in_params.begin();
            i != in_params.end(); i++) {
        argv.push_back(*i);
    }

/*
    for (std::vector<std::string>::const_iterator i = argv.begin();
            i != argv.end(); i++) {
        std::cout << *i << std::endl;
    }
*/

    Glib::Pid pid;
    int stdout_pipe, stderr_pipe;

    try {
        Glib::spawn_async_with_pipes(Glib::get_tmp_dir(), // working directory
                                     argv,  // arg v
                                     Glib::SPAWN_SEARCH_PATH /*| Glib::SPAWN_DO_NOT_REAP_CHILD*/,
                                     sigc::slot<void>(),
                                     &pid,           // Pid
                                     NULL,           // STDIN
                                     &stdout_pipe,   // STDOUT
                                     &stderr_pipe);  // STDERR
    } catch (Glib::SpawnError e) {
        printf("Can't Spawn!!! %d\n", e.code());
        return 0;
    }

    Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create(false);

    file_listener fileerr;
    fileout.init(stdout_pipe, main_loop);
    fileerr.init(stderr_pipe, main_loop);

    main_loop->run();

    Glib::ustring stderr_data = fileerr.string();
    if (stderr_data.length() != 0) {
        checkStderr(stderr_data, Gtk::MESSAGE_INFO,
                                 _("Inkscape has received additional data from the script executed.  "
                                   "The script did not return an error, but this may indicate the results will not be as expected."));
    }

    Glib::ustring stdout_data = fileout.string();
    if (stdout_data.length() == 0) {
        return 0;
    }

    // std::cout << "Finishing Execution." << std::endl;
    return stdout_data.length();
}




}  // namespace Implementation
}  // namespace Extension
}  // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
