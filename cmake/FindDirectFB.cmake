# Try to find DirectFB
#
# This will define:
#
#   DIRECTFB_FOUND       - True if DirectFB is found
#   DIRECTFB_LIBRARIES   - Link these to use DirectFB
#   DIRECTFB_INCLUDE_DIR - Include directory for DirectFB
#   DIRECTFB_DEFINITIONS - Compiler flags for using DirectFB
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (NOT WIN32)
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(PKG_DIRECTFB QUIET directfb)

  SET(DIRECTFB_DEFINITIONS ${PKG_DIRECTFB_CFLAGS})

  FIND_PATH(DIRECTFB_INCLUDE_DIR  NAMES directfb.h HINTS ${PKG_DIRECTFB_INCLUDE_DIRS})

  FIND_LIBRARY(DIRECTFB_LIBRARIES NAMES directfb   HINTS ${PKG_DIRECTFB_LIBRARY_DIRS})

  include(FindPackageHandleStandardArgs)

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIRECTFB DEFAULT_MSG DIRECTFB_LIBRARIES DIRECTFB_INCLUDE_DIR)

  MARK_AS_ADVANCED(DIRECTFB_INCLUDE_DIR DIRECTFB_LIBRARIES)
ENDIF ()
