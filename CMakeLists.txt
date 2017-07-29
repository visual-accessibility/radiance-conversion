PROJECT ( DEVA_FILTER )
cmake_minimum_required(VERSION 2.4)

# SET ( CMAKE_VERBOSE_MAKEFILE ON )

ADD_DEFINITIONS ( -Wall -g -O2 )

INCLUDE ( FindTIFF )
INCLUDE ( FindJPEG )
INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindEXIF.cmake )

INCLUDE_DIRECTORIES (
	${TIFF_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${EXIF_INCLUDE_DIR}
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
	sRGB.c
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
	sRGB.c
	radiance/timegm.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( tiff2rad
	${TIFF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( rad2jpeg rad2jpeg.c
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
	sRGB.c
	tifftoolsimage.c tifftools.c
	TT-jpeg.c
	iccjpeg.c
	)
TARGET_LINK_LIBRARIES ( rad2jpeg
	${TIFF_LIBRARIES}
	${JPEG_LIBRARIES}
	${EXIF_LIBRARIES}
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
	sRGB.c
	radiance/timegm.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( make-rad-test-image
	${TIFF_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( tiff32_to_8 tiff32_to_8.c
	sRGB.c
	tifftoolsimage.c tifftools.c
	)
TARGET_LINK_LIBRARIES ( tiff32_to_8
	${TIFF_LIBRARIES}
	-lm
	)
