SET(INKSCAPE_DEPENDS
    gtk+-2.0
    gtkmm-2.4
    cairo
    cairomm-1.0
    sigc++-2.0
    lcms
    libgc1c2
    libpng
    glib-2.0
    glibmm-2.4
    libxml-2.0
    libxslt
    ImageMagick++
    libpopt
    freetype2)
SET(INKSCAPE_OPTIONAL
    gnome-vfs-2.0
    libwpg-0.1
    libssl)

include(UsePkgConfig)

# Dependencies Packages
message(STATUS "")
message(STATUS "")
message(STATUS "Checking For REQUIRED Libraries for Building Inkscape.")
SET(INKSCAPE_LINK_FLAGS "")
FOREACH(dep ${INKSCAPE_DEPENDS})
    # This is a hack due to a bug in Cmake vars system, Uncomment if using a version older than 2.4 //verbalshadow
#    IF("${dep}" MATCHES "gtk\\+-2.0")
#        SET(dep_name "GTK2")
#    ELSE("${dep}" MATCHES "gtk\\+-2.0")
       SET(dep_name "${dep}")
#    ENDIF("${dep}" MATCHES "gtk\\+-2.0")

    PKGCONFIG_FOUND(${dep} "${dep}_FOUND")
    PKGCONFIG(${dep} "${dep_name}_INCLUDE_DIR" "${dep_name}_LINK_DIR" "${dep_name}_LINK_FLAGS" "${dep_name}_CFLAGS")
#    PKGCONFIG_VERSION(${dep} "${dep}_VERSION")
    IF("${dep}_FOUND")
        message(STATUS "${dep}: FOUND")
        # Set Compiler Flags
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${dep_name}_CFLAGS}")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${${dep_name}_CFLAGS}")
        SET(INKSCAPE_LINK_FLAGS "${INKSCAPE_LINK_FLAGS} ${${dep_name}_LINK_FLAGS}")
    ELSE("${dep}_FOUND")
        message(STATUS "${dep}: NOT FOUND")
    ENDIF("${dep}_FOUND")
ENDFOREACH(dep)
# Include non pkg-config dependencies:
INCLUDE(FindBoost)
INCLUDE(FindGC)
# end Dependencies

# Optional Dependencies Packages
message(STATUS "")
message(STATUS "")
message(STATUS "Checking For OPTIONAL Libraries for Building Inkscape.")
message(STATUS "These add additional functionality to Inkscape.")
FOREACH(opt ${INKSCAPE_OPTIONAL})
    SET(opt_name "${opt}")
    PKGCONFIG_FOUND(${opt} "${opt}_FOUND")
    PKGCONFIG(${opt} "${opt_name}_INCLUDE_DIR" "${opt_name}_LINK_DIR" "${opt_name}_LINK_FLAGS" "${opt_name}_CFLAGS")
#    PKGCONFIG_VERSION(${opt} "${opt}_VERSION")
    IF("${opt}_FOUND")
        message(STATUS "${opt}: FOUND")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${dep_name}_CFLAGS}")
        SET(INKSCAPE_LINK_FLAGS "${INKSCAPE_LINK_FLAGS} ${${dep_name}_LINK_FLAGS}")
    ELSE("${opt}_FOUND")
        message(STATUS "${opt}: NOT FOUND")
    ENDIF("${opt}_FOUND")
ENDFOREACH(opt)
# end Optional Dependencies
message(STATUS "")
message(STATUS "")

SET(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j2")

INCLUDE (CheckIncludeFiles)
# usage: CHECK_INCLUDE_FILES (<header> <RESULT_VARIABLE> )

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (boost/concept_check.hpp HAVE_BOOST_CONCEPT_CHECK_HPP)
CHECK_INCLUDE_FILES (gc/gc.h HAVE_GC_GC_H)
CHECK_INCLUDE_FILES (gc.h HAVE_GC_H)
CHECK_INCLUDE_FILES (getopt.h HAVE_GETOPT_H)
CHECK_INCLUDE_FILES (ieeefp.h HAVE_IEEEFP_H)
CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES (locale.h HAVE_LC_MESSAGES)
CHECK_INCLUDE_FILES (locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILES (libintl.h HAVE_LIBINTL_H)
CHECK_INCLUDE_FILES (fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (sys/filio.h HAVE_SYS_FILIO_H)
CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES (sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (zlib.h HAVE_ZLIB_H)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Create File inkscape_version.h
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/inkscape_version.h "#define INKSCAPE_VERSION \"${INKSCAPE_VERSION}\"\n")
