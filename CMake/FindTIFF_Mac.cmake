# DEVA version of FindTIFF that supports Mac build

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  message ( FATAL_ERROR "FindTIFF_Mac: not Mac compile!" )
endif ( )

set_property ( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON )

INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindZLIB_Mac.cmake )
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindJPEG_Mac.cmake )

if ( ZLIB_FOUND AND JPEG_FOUND )

  set ( CMAKE_FIND_ROOT_PATH
	  ${CMAKE_CURRENT_SOURCE_DIR}/external-libs/macinstall )

  find_path ( TIFF_INCLUDE_DIR tiff.h )

  find_library ( TIFF_LIBRARY libtiff.a )

  # handle the QUIETLY and REQUIRED arguments and set TIFF_FOUND to TRUE if
  # all listed variables are TRUE
  include ( FindPackageHandleStandardArgs )
  FIND_PACKAGE_HANDLE_STANDARD_ARGS ( TIFF
  	  DEFAULT_MSG
  	  TIFF_LIBRARY
  	  TIFF_INCLUDE_DIR
  	  )

  if ( TIFF_FOUND )
    set ( TIFF_LIBRARIES ${TIFF_LIBRARY} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} )

  endif ( )

  # Deprecated declarations.
  set ( NATIVE_TIFF_INCLUDE_PATH ${TIFF_INCLUDE_DIR} )
    if ( TIFF_LIBRARY )
      get_filename_component ( NATIVE_TIFF_LIB_PATH ${TIFF_LIBRARY} PATH )
    endif ( )

endif ( )

mark_as_advanced ( TIFF_LIBRARY TIFF_INCLUDE_DIR )
