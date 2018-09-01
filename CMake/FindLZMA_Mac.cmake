# DEVA version of FindLZMA that supports MacOS build

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  message ( FATAL_ERROR "FindLZMA_Mac: not MacOS build!" )
endif ( )

set ( CMAKE_FIND_ROOT_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/external-libs/macinstall )

find_path ( LZMA_INCLUDE_DIR lzma.h )

find_library ( LZMA_LIBRARY liblzma.a )

# handle the QUIETLY and REQUIRED arguments and set LZMA_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS ( LZMA
	  DEFAULT_MSG
	  LZMA_LIBRARY
	  LZMA_INCLUDE_DIR
	  )

if ( LZMA_FOUND )
  set ( LZMA_LIBRARIES ${LZMA_LIBRARY} )
endif ( )

# Deprecated declarations.
set ( NATIVE_LZMA_INCLUDE_PATH ${LZMA_INCLUDE_DIR} )
  if ( LZMA_LIBRARY )
    get_filename_component ( NATIVE_LZMA_LIB_PATH ${LZMA_LIBRARY} PATH )
  endif ( )

mark_as_advanced ( LZMA_LIBRARY LZMA_INCLUDE_DIR )

