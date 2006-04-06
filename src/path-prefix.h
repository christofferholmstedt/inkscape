/*
 * Separate the inkscape paths from the prefix code, as that is kind of
 * a separate package (binreloc)
 * 	http://autopackage.org/downloads.html
 *
 * Since the directories set up by autoconf end up in config.h, we can't
 * _change_ them, since config.h isn't protected by a set of
 * one-time-include directives and is repeatedly re-included by some
 * chains of .h files.  As a result, nothing should refer to those
 * define'd directories, and instead should use only the paths defined here.
 *
 */
#ifndef _PATH_PREFIX_H_
#define _PATH_PREFIX_H_

#include "require-config.h"  // INKSCAPE_DATADIR
#include "prefix.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ENABLE_BINRELOC
#  define INKSCAPE_APPICONDIR     BR_DATADIR( "/pixmaps" )
#  define INKSCAPE_EXTENSIONDIR   BR_DATADIR( "/inkscape/extensions" )
#  define INKSCAPE_GRADIENTSDIR   BR_DATADIR( "/inkscape/gradients" )
#  define INKSCAPE_KEYSDIR        BR_DATADIR( "/inkscape/keys" )
#  define INKSCAPE_PIXMAPDIR      BR_DATADIR( "/inkscape/icons" )
#  define INKSCAPE_MARKERSDIR     BR_DATADIR( "/inkscape/markers" )
#  define INKSCAPE_PALETTESDIR    BR_DATADIR( "/inkscape/palettes" )
#  define INKSCAPE_PATTERNSDIR    BR_DATADIR( "/inkscape/patterns" )
#  define INKSCAPE_SCREENSDIR     BR_DATADIR( "/inkscape/screens" )
#  define INKSCAPE_TUTORIALSDIR   BR_DATADIR( "/inkscape/tutorials" )
#  define INKSCAPE_PLUGINDIR      BR_LIBDIR(  "/inkscape/plugins" )
#  define INKSCAPE_TEMPLATESDIR   BR_DATADIR( "/inkscape/templates" )
#  define INKSCAPE_UIDIR          BR_DATADIR( "/inkscape/ui" )
#else
#  ifdef WIN32
#    define INKSCAPE_APPICONDIR   "pixmaps"
#    define INKSCAPE_EXTENSIONDIR "share\\extensions"
#    define INKSCAPE_GRADIENTSDIR "share\\gradients"
#    define INKSCAPE_KEYSDIR      "share\\keys"
#    define INKSCAPE_PIXMAPDIR    "share\\icons"
#    define INKSCAPE_MARKERSDIR   "share\\markers"
#    define INKSCAPE_PALETTESDIR  "share\\palettes"
#    define INKSCAPE_PATTERNSDIR  "share\\patterns"
#    define INKSCAPE_SCREENSDIR   "share\\screens"
#    define INKSCAPE_TUTORIALSDIR "share\\tutorials"
#    define INKSCAPE_PLUGINDIR    "plugins"
#    define INKSCAPE_TEMPLATESDIR "share\\templates"
#    define INKSCAPE_UIDIR        INKSCAPE_DATADIR "\\share\\ui"
#  elif defined ENABLE_OSX_APP_LOCATIONS
#    define INKSCAPE_APPICONDIR   "Contents/Resources/pixmaps"
#    define INKSCAPE_EXTENSIONDIR "Contents/Resources/extensions"
#    define INKSCAPE_GRADIENTSDIR "Contents/Resources/gradients"
#    define INKSCAPE_KEYSDIR      "Contents/Resources/keys"
#    define INKSCAPE_PIXMAPDIR    "Contents/Resources/icons"
#    define INKSCAPE_MARKERSDIR   "Contents/Resources/markers"
#    define INKSCAPE_PALETTESDIR  "Contents/Resources/palettes"
#    define INKSCAPE_PATTERNSDIR  "Contents/Resources/patterns"
#    define INKSCAPE_SCREENSDIR   "Contents/Resources/screens"
#    define INKSCAPE_TUTORIALSDIR "Contents/Resources/tutorials"
#    define INKSCAPE_PLUGINDIR    "Contents/Resources/plugins"
#    define INKSCAPE_TEMPLATESDIR "Contents/Resources/templates"
#    define INKSCAPE_UIDIR        "Contents/Resources/ui"
#  else
#    define INKSCAPE_APPICONDIR   INKSCAPE_DATADIR "/pixmaps"
#    define INKSCAPE_EXTENSIONDIR INKSCAPE_DATADIR "/inkscape/extensions"
#    define INKSCAPE_GRADIENTSDIR INKSCAPE_DATADIR "/inkscape/gradients"
#    define INKSCAPE_KEYSDIR      INKSCAPE_DATADIR "/inkscape/keys"
#    define INKSCAPE_PIXMAPDIR    INKSCAPE_DATADIR "/inkscape/icons"
#    define INKSCAPE_MARKERSDIR   INKSCAPE_DATADIR "/inkscape/markers"
#    define INKSCAPE_PALETTESDIR  INKSCAPE_DATADIR "/inkscape/palettes"
#    define INKSCAPE_PATTERNSDIR  INKSCAPE_DATADIR "/inkscape/patterns"
#    define INKSCAPE_SCREENSDIR   INKSCAPE_DATADIR "/inkscape/screens"
#    define INKSCAPE_TUTORIALSDIR INKSCAPE_DATADIR "/inkscape/tutorials"
#    define INKSCAPE_PLUGINDIR    INKSCAPE_LIBDIR  "/inkscape/plugins"
#    define INKSCAPE_TEMPLATESDIR INKSCAPE_DATADIR "/inkscape/templates"
#    define INKSCAPE_UIDIR        INKSCAPE_DATADIR "/inkscape/ui"
#  endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PATH_PREFIX_H_ */
