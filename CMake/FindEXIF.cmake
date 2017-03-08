# - Find EXIF
# Find the native EXIF includes and library
# This module defines
#  EXIF_INCLUDE_DIR, where to find jpeglib.h, etc.
#  EXIF_LIBRARIES, the libraries needed to use EXIF.
#  EXIF_FOUND, If false, do not try to use EXIF.
# also defined, but not for general use are
#  EXIF_LIBRARY, where to find the EXIF library.

#=============================================================================
# based on FindJPEG.cmake
#
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(EXIF_INCLUDE_DIR libexif/exif-data.h)

set(EXIF_NAMES ${EXIF_NAMES} exif)
find_library(EXIF_LIBRARY NAMES ${EXIF_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set EXIF_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EXIF DEFAULT_MSG EXIF_LIBRARY EXIF_INCLUDE_DIR)

if(EXIF_FOUND)
  set(EXIF_LIBRARIES ${EXIF_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_EXIF_INCLUDE_PATH ${EXIF_INCLUDE_DIR} )
if(EXIF_LIBRARY)
  get_filename_component (NATIVE_EXIF_LIB_PATH ${EXIF_LIBRARY} PATH)
endif()

mark_as_advanced(EXIF_LIBRARY EXIF_INCLUDE_DIR )
