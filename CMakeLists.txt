PROJECT ( DEVA_FILTER )
cmake_minimum_required(VERSION 3.0)

# SET ( CMAKE_VERBOSE_MAKEFILE ON )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  # Radiance tries to redefine things it shouldn't with mignw64 cross-compiling
  set ( CMAKE_C_FLAGS "-Wall -g -O2 -D_mingw64_cross" )
else ( )
  set ( CMAKE_C_FLAGS "-Wall -g -O2" )
endif ( )

ADD_DEFINITIONS ( -Wall -g -O2 )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindPNG_Windows.cmake )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindJPEG_Windows.cmake )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindEXIF_Windows.cmake )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindTIFF_Windows.cmake )
else ( )
  find_package ( PNG REQUIRED )
  find_package ( JPEG REQUIRED )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindEXIF.cmake )
  find_package ( TIFF REQUIRED )
endif ( )

INCLUDE_DIRECTORIES (
	${TIFF_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${EXIF_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	)

ADD_EXECUTABLE ( rad2png rad2png.c
	radianceIO.c
	radiance-header.c
	radiance/color.c
	radiance/header.c
	radiance/fputword.c
	radiance/resolu.c
	radiance/image.c
	radiance/fvect.c
	radiance/badarg.c
	radiance/words.c
	radiance/spec_rgb.c
	radiance/timegm.c
	deva-image.c
	DEVA-sRGB.c
	DEVA-png.c
	)
TARGET_LINK_LIBRARIES ( rad2png
	${PNG_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( rad2jpeg rad2jpeg.c
	DEVA-jpeg.c
	radianceIO.c
	radiance-header.c
	radiance/color.c
	radiance/header.c
	radiance/fputword.c
	radiance/resolu.c
	radiance/image.c
	radiance/fvect.c
	radiance/badarg.c
	radiance/words.c
	radiance/spec_rgb.c
	radiance/timegm.c
	deva-image.c
	DEVA-sRGB.c
	iccjpeg.c
	)
TARGET_LINK_LIBRARIES ( rad2jpeg
	${JPEG_LIBRARIES}
	${EXIF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( rad2tiff rad2tiff.c
	radiance-tiff.c
	radiance-header.c
	radiance/color.c
	radiance/header.c
	radiance/fputword.c
	radiance/resolu.c
	radiance/image.c
	radiance/fvect.c
	radiance/badarg.c
	radiance/words.c
	radiance/spec_rgb.c
	radiance/timegm.c
	FOV.c
	TT-sRGB.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( rad2tiff
	${TIFF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( tiff2rad tiff2rad.c
	radiance-tiff.c
	radiance-header.c
	radiance/color.c
	radiance/header.c
	radiance/fputword.c
	radiance/resolu.c
	radiance/image.c
	radiance/fvect.c
	radiance/badarg.c
	radiance/words.c
	radiance/spec_rgb.c
	FOV.c
	TT-sRGB.c
	radiance/timegm.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( tiff2rad
	${TIFF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( make-rad-test-image make-rad-test-image.c
	radiance-tiff.c
	radiance-header.c
	radiance/color.c
	radiance/header.c
	radiance/fputword.c
	radiance/resolu.c
	radiance/image.c
	radiance/fvect.c
	radiance/badarg.c
	radiance/words.c
	radiance/spec_rgb.c
	FOV.c
	TT-sRGB.c
	radiance/timegm.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( make-rad-test-image
	${TIFF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( tiff32_to_8 tiff32_to_8.c
	TT-sRGB.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( tiff32_to_8
	${TIFF_LIBRARIES}
	-lm
	)
