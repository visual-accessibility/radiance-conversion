# DEVA version of FindEXIF that supports Windows cross-build

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  message ( FATAL_ERROR "FindEXIF_Windows: not Windows cross-compile!" )
endif ( )

set_property ( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON )

set ( CMAKE_FIND_ROOT_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/external-libs/windowsinstall )

find_path ( EXIF_INCLUDE_DIR libexif/exif-data.h )

find_library ( EXIF_LIBRARY libexif.a )

# handle the QUIETLY and REQUIRED arguments and set EXIF_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS ( EXIF
	  DEFAULT_MSG
	  EXIF_LIBRARY
	  EXIF_INCLUDE_DIR
	  )

if ( EXIF_FOUND )
  set ( EXIF_LIBRARIES ${EXIF_LIBRARY} )
endif ( )

# Deprecated declarations.
set ( NATIVE_EXIF_INCLUDE_PATH ${EXIF_INCLUDE_DIR} )
  if ( EXIF_LIBRARY )
    get_filename_component ( NATIVE_EXIF_LIB_PATH ${EXIF_LIBRARY} PATH )
  endif ( )

mark_as_advanced ( EXIF_LIBRARY EXIF_INCLUDE_DIR )

