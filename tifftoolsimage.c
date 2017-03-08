/*
 * Support for in-memory access of image data, including creating
 * image arrays from TIFF images and writing image array data to TIFF
 * files.
 */

/****************************************************************************
 * MIT License
 *
 * Copyright (c) 2016 University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ****************************************************************************/

#include "tifftoolsimage.h"

#define TT_IMAGE_NEW( TYPE )						\
TYPE##_image *								\
TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols  )		\
{									\
    TYPE##_image    *new_image;						\
    int		    row;						\
    TYPE	    **line_pointers;					\
									\
    new_image = ( TYPE##_image * ) malloc ( sizeof ( TYPE##_image ) );	\
    if ( new_image == NULL ) {                                          \
	fprintf ( stderr, "TT_image_new: malloc failed!" );		\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    new_image->n_rows = n_rows;						\
    new_image->n_cols = n_cols;						\
									\
    new_image->start_data = (TYPE *) malloc ( n_rows * n_cols *		\
	        sizeof ( TYPE ) );					\
    if ( new_image->start_data == NULL ) {				\
	fprintf ( stderr, "tttools_image_new: malloc failed!" );	\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    line_pointers = (TYPE **) malloc ( n_rows * sizeof ( TYPE * ) );	\
    if ( line_pointers == NULL ) {					\
	fprintf ( stderr, "tttools_image_new: malloc failed!" );	\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    for ( row = 0; row < n_rows; row++ ) {				\
	line_pointers[row] = new_image->start_data + ( row * n_cols );	\
    }									\
									\
    new_image->data = &line_pointers[0];				\
									\
    return ( new_image );						\
}

TT_IMAGE_NEW ( TT_gray )
TT_IMAGE_NEW ( TT_gray16 )
TT_IMAGE_NEW ( TT_RGB )
TT_IMAGE_NEW ( TT_RGBf )
TT_IMAGE_NEW ( TT_RGBA )
TT_IMAGE_NEW ( TT_XYZ )
TT_IMAGE_NEW ( TT_xyY )
TT_IMAGE_NEW ( TT_float )

#define TT_IMAGE_DELETE( TYPE )						\
void									\
TYPE##_image_delete ( TYPE##_image *image )				\
{									\
    free ( image->start_data );						\
    free ( image->data );						\
    free ( image );							\
}

TT_IMAGE_DELETE ( TT_gray )
TT_IMAGE_DELETE ( TT_gray16 )
TT_IMAGE_DELETE ( TT_RGB )
TT_IMAGE_DELETE ( TT_RGBf )
TT_IMAGE_DELETE ( TT_RGBA )
TT_IMAGE_DELETE ( TT_XYZ )
TT_IMAGE_DELETE ( TT_xyY )
TT_IMAGE_DELETE ( TT_float )

void
TT_image_check_bounds ( TT_gray_image *deva_image, int row, int col,
       int line, char *file )
{
    if ( ( row < 0 ) || ( row >= TT_image_n_rows ( deva_image ) ) ||
	    ( col < 0 ) || ( col >= TT_image_n_cols ( deva_image ) ) ) {
	fprintf ( stderr, "TT_image_data out-of-bounds reference!\n" );
	fprintf ( stderr, "line %d in file %s\n", line, file );
	exit ( EXIT_FAILURE );
    }
}

#define TT_IMAGE_FROM_FILE( TYPE, TYPE_CODE )				\
TYPE##_image *								\
TYPE##_image_from_file ( TIFF *input )					\
{									\
    unsigned int    n_rows, n_cols;					\
    int		    row;						\
    TYPE##_image    *image;						\
    TTType	    type;						\
									\
    TT_image_size ( input, &n_rows, &n_cols );				\
									\
    type = TT_file_type ( input );					\
    if ( type != TYPE_CODE ) {						\
	fprintf ( stderr,						\
	    "tifftools_image_from_file: Input file not in correct format\n" );\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    if ( type == TTTypeLogLuv ) {					\
	TIFFSetField ( input, TIFFTAG_SGILOGDATAFMT,			\
		SGILOGDATAFMT_FLOAT );					\
    }									\
									\
    image = TYPE##_image_new ( n_rows, n_cols );			\
									\
    for ( row = 0; row < n_rows; row++ ) {				\
	if ( TIFFReadScanline ( input, &image->data[row][0], row, 0 ) != 1 ) {\
	    /* libtiff should print error message */			\
	    exit ( EXIT_FAILURE );					\
	}								\
    }									\
									\
     /* Leave open, since there may still be need to access tags */	\
     /* TIFFClose ( input ); */						\
									\
    return ( image );							\
}

TT_IMAGE_FROM_FILE ( TT_gray, TTTypeGray )
TT_IMAGE_FROM_FILE ( TT_gray16, TTTypeGray16 )
TT_IMAGE_FROM_FILE ( TT_RGB, TTTypeRGB )
TT_IMAGE_FROM_FILE ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_FROM_FILE ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_FROM_FILE ( TT_XYZ, TTTypeLogLuv )
TT_IMAGE_FROM_FILE ( TT_float, TTTypeFloat )

#define TT_IMAGE_FROM_FILENAME( TYPE, TYPE_CODE )			\
TYPE##_image *								\
TYPE##_image_from_filename ( char *filename )				\
{									\
    TIFF	    *file;						\
    TTType	    type;						\
    TYPE##_image    *image;						\
									\
    file = TIFFOpen ( filename, "r" );					\
    if ( file == NULL ) {						\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    type = TT_file_type ( file );					\
    if ( type != TYPE_CODE ) {						\
	fprintf ( stderr, "tifftools_image_from_filename (%s):\n",	\
		filename );						\
	fprintf ( stderr, "Input file not in correct format\n" );	\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    image = TYPE##_image_from_file ( file ) ;				\
    TIFFClose ( file );							\
    return ( image );							\
}

TT_IMAGE_FROM_FILENAME ( TT_gray, TTTypeGray )
TT_IMAGE_FROM_FILENAME ( TT_gray16, TTTypeGray16 )
TT_IMAGE_FROM_FILENAME ( TT_RGB, TTTypeRGB )
TT_IMAGE_FROM_FILENAME ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_FROM_FILENAME ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_FROM_FILENAME ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_FROM_FILENAME ( TT_float, TTTypeFloat )

#define TT_IMAGE_FROM_FILE_DESCRIPTION( TYPE, TYPE_CODE )		\
TYPE##_image *								\
TYPE##_image_from_file_description ( TIFF *input, char **description )	\
{									\
    unsigned int    n_rows, n_cols;					\
    int		    row;						\
    TYPE##_image    *image;						\
    TTType	    type;						\
									\
    TT_image_size ( input, &n_rows, &n_cols );				\
									\
    type = TT_file_type ( input );					\
    if ( type != TYPE_CODE ) {						\
	fprintf ( stderr,						\
	    "tifftools_image_from_file: Input file not in correct format\n" );\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    if ( type == TTTypeLogLuv ) {					\
	TIFFSetField ( input, TIFFTAG_SGILOGDATAFMT,			\
		SGILOGDATAFMT_FLOAT );					\
    }									\
									\
    image = TYPE##_image_new ( n_rows, n_cols );			\
									\
    for ( row = 0; row < n_rows; row++ ) {				\
	if ( TIFFReadScanline ( input, &image->data[row][0], row, 0 ) != 1 ) {\
	    /* libtiff should print error message */			\
	    exit ( EXIT_FAILURE );					\
	}								\
    }									\
									\
    *description = TT_get_description ( input );			\
									\
     /* Leave open, since there may still be need to access tags */	\
     /* TIFFClose ( input ); */						\
									\
    return ( image );							\
}

TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_gray, TTTypeGray )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_gray16, TTTypeGray16 )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGB, TTTypeRGB )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_XYZ, TTTypeLogLuv )
TT_IMAGE_FROM_FILE_DESCRIPTION ( TT_float, TTTypeFloat )

#define TT_IMAGE_FROM_FILENAME_DESCRIPTION( TYPE, TYPE_CODE )		\
TYPE##_image *								\
TYPE##_image_from_filename_description ( char *filename, char **description ) \
{									\
    TIFF	    *file;						\
    TTType	    type;						\
    TYPE##_image    *image;						\
									\
    file = TIFFOpen ( filename, "r" );					\
    if ( file == NULL ) {						\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    type = TT_file_type ( file );					\
    if ( type != TYPE_CODE ) {						\
	fprintf ( stderr, "tifftools_image_from_filename (%s):\n",	\
		filename );						\
	fprintf ( stderr, "Input file not in correct format\n" );	\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    image = TYPE##_image_from_file_description ( file, description );	\
    TIFFClose ( file );							\
    return ( image );							\
}

TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_gray, TTTypeGray )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_gray16, TTTypeGray16 )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_RGB, TTTypeRGB )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_FROM_FILENAME_DESCRIPTION ( TT_float, TTTypeFloat )


#define TT_IMAGE_TO_FILE( TYPE, TYPE_CODE )				\
void									\
TYPE##_image_to_file ( TIFF *output, TYPE##_image *image )		\
{									\
    int		row;							\
    TTType	type;							\
									\
    type = TT_file_type ( output );					\
    if ( type != TYPE_CODE ) {						\
	fprintf ( stderr,						\
	    "tifftools_image_to_file: Output file not in correct format\n" );\
	exit ( EXIT_FAILURE );						\
    }									\
									\
    for ( row = 0; row < image->n_rows; row++ ) {			\
	if ( TIFFWriteScanline ( output, &image->data[row][0], row, 0 ) \
							!= 1 ) {;	\
	    /* libtiff should print error message */			\
	    exit ( EXIT_FAILURE );					\
	}								\
    }									\
}

TT_IMAGE_TO_FILE ( TT_gray, TTTypeGray )
TT_IMAGE_TO_FILE ( TT_gray16, TTTypeGray16 )
TT_IMAGE_TO_FILE ( TT_RGB, TTTypeRGB )
TT_IMAGE_TO_FILE ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_TO_FILE ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_TO_FILE ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_TO_FILE ( TT_float, TTTypeFloat )

#define TT_IMAGE_TO_FILENAME( TYPE, TYPE_CODE )				\
void									\
TYPE##_image_to_filename ( char *filename, TYPE##_image *image )	\
{									\
    TIFF	*output;						\
									\
    output = TT_open_write ( filename, TYPE_CODE, image->n_rows,	\
	    image->n_cols );						\
									\
    TYPE##_image_to_file ( output, image );				\
									\
    TIFFClose ( output );						\
}

TT_IMAGE_TO_FILENAME ( TT_gray, TTTypeGray )
TT_IMAGE_TO_FILENAME ( TT_gray16, TTTypeGray16 )
TT_IMAGE_TO_FILENAME ( TT_RGB, TTTypeRGB )
TT_IMAGE_TO_FILENAME ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_TO_FILENAME ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_TO_FILENAME ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_TO_FILENAME ( TT_float, TTTypeFloat )

#define TT_IMAGE_TO_FILENAME_DESCRIPTION( TYPE, TYPE_CODE )		\
void									\
TYPE##_image_to_filename_description ( char *filename,			\
	TYPE##_image *image, char *description )			\
{									\
    TIFF	*output;						\
									\
    output = TT_open_write ( filename, TYPE_CODE, image->n_rows,	\
	    image->n_cols );						\
									\
    TYPE##_image_to_file ( output, image );				\
    TT_set_description ( output, description );				\
									\
    TIFFClose ( output );						\
}

TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_gray, TTTypeGray )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_gray16, TTTypeGray16 )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGB, TTTypeRGB )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_TO_FILENAME_DESCRIPTION ( TT_float, TTTypeFloat )

#define TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS( TYPE, TYPE_CODE )	\
void									\
TYPE##_image_to_filename_description_arguments ( char *filename,	\
	TYPE##_image *image, int argc, char *argv[] )			\
{									\
    TIFF	*output;						\
									\
    output = TT_open_write ( filename, TYPE_CODE, image->n_rows,	\
	    image->n_cols );						\
									\
    TYPE##_image_to_file ( output, image );				\
    TT_set_description_arguments ( output, argc, argv );		\
									\
    TIFFClose ( output );						\
}

TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_gray, TTTypeGray )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_gray16, TTTypeGray16 )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_RGB, TTTypeRGB )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_RGBf, TTTypeRGBf )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_RGBA, TTTypeRGBA )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_XYZ, TTTypeXYZ )
TT_IMAGE_TO_FILENAME_DESCRIPTION_ARGUMENTS ( TT_float, TTTypeFloat )
