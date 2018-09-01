# DEVA version of FindJPEG that supports Windows cross-build

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  message ( FATAL_ERROR "FindJPEG_Windows: not Windows cross-compile!" )
endif ( )

set_property ( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON )

set ( CMAKE_FIND_ROOT_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/external-libs/windowsinstall )

find_path ( JPEG_INCLUDE_DIR jpeglib.h )

find_library ( JPEG_LIBRARY libjpeg.a )

# handle the QUIETLY and REQUIRED arguments and set JPEG_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS ( JPEG
	  DEFAULT_MSG
	  JPEG_LIBRARY
	  JPEG_INCLUDE_DIR
	  )

if ( JPEG_FOUND )
  set ( JPEG_LIBRARIES ${JPEG_LIBRARY} )
endif ( )

# Deprecated declarations.
set ( NATIVE_JPEG_INCLUDE_PATH ${JPEG_INCLUDE_DIR} )
  if ( JPEG_LIBRARY )
    get_filename_component ( NATIVE_JPEG_LIB_PATH ${JPEG_LIBRARY} PATH )
  endif ( )

mark_as_advanced ( JPEG_LIBRARY JPEG_INCLUDE_DIR )

