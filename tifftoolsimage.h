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

#ifndef	__TIFFTOOLSIMAGE_H
#define	__TIFFTOOLSIMAGE_H

#include "tifftools.h"

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define TT_DEFINE_IMAGE_TYPE( TYPE )					     \
typedef struct {							     \
    unsigned int    n_rows, n_cols;	/* order reversed from x,y! */	     \
    TYPE    	    *start_data;	/* start of allocated data block */  \
    TYPE    	    **data;		/* array of pointers to array rows */\
} TYPE##_image;

TT_DEFINE_IMAGE_TYPE ( TT_gray )
TT_DEFINE_IMAGE_TYPE ( TT_gray16 )
TT_DEFINE_IMAGE_TYPE ( TT_RGB )
TT_DEFINE_IMAGE_TYPE ( TT_RGB16 )
TT_DEFINE_IMAGE_TYPE ( TT_RGBf )
TT_DEFINE_IMAGE_TYPE ( TT_RGBA )
TT_DEFINE_IMAGE_TYPE ( TT_XYZ )
TT_DEFINE_IMAGE_TYPE ( TT_xyY )
TT_DEFINE_IMAGE_TYPE ( TT_float )

/*
 * methods on TT_image objects:
 */

#ifdef  TT_CHECK_BOUNDS
#define TT_image_data(TT_image,row,col)					\
	    (TT_image)->data[TT_image_check_bounds (			\
		(TT_gray_image *) TT_image, row, col,			\
		    __LINE__, __FILE__  ), row][col]
#else
#define TT_image_data(TT_image,row,col)     (TT_image)->data[row][col]
			/* read/write */
#endif

#define TT_image_n_rows(TT_image)           (TT_image)->n_rows
			/* read only (but not enforced ) */

#define TT_image_n_cols(TT_image)           (TT_image)->n_cols
			/* read only (but not enforced ) */

#define TT_image_samesize(TT_image_1,TT_image_2)			\
		( ( TT_image_n_rows ( TT_image_1 ) ==			\
		    TT_image_n_rows ( TT_image_2 ) ) &&			\
		  ( TT_image_n_cols ( TT_image_1 ) ==			\
		    TT_image_n_cols ( TT_image_2 ) ) )

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define	TT_PROTOTYPE_IMAGE_NEW( TYPE )					\
TYPE##_image	*TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols );

TT_PROTOTYPE_IMAGE_NEW ( TT_gray )
TT_PROTOTYPE_IMAGE_NEW ( TT_gray16 )
TT_PROTOTYPE_IMAGE_NEW ( TT_RGB )
TT_PROTOTYPE_IMAGE_NEW ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_NEW ( TT_RGBf )
TT_PROTOTYPE_IMAGE_NEW ( TT_RGBA )
TT_PROTOTYPE_IMAGE_NEW ( TT_XYZ )
TT_PROTOTYPE_IMAGE_NEW ( TT_xyY )
TT_PROTOTYPE_IMAGE_NEW ( TT_float )

#define	TT_PROTOTYPE_IMAGE_DELETE( TYPE )				\
void	TYPE##_image_delete ( TYPE##_image *image );

TT_PROTOTYPE_IMAGE_DELETE ( TT_gray )
TT_PROTOTYPE_IMAGE_DELETE ( TT_gray16 )
TT_PROTOTYPE_IMAGE_DELETE ( TT_RGB )
TT_PROTOTYPE_IMAGE_DELETE ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_DELETE ( TT_RGBf )
TT_PROTOTYPE_IMAGE_DELETE ( TT_RGBA )
TT_PROTOTYPE_IMAGE_DELETE ( TT_XYZ )
TT_PROTOTYPE_IMAGE_DELETE ( TT_xyY )
TT_PROTOTYPE_IMAGE_DELETE ( TT_float )

#define	TT_PROTOTYPE_IMAGE_FROM_FILE( TYPE )				\
TYPE##_image	*TYPE##_image_from_file ( TIFF *file );

TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_gray )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_gray16 )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_RGB )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_RGBf )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_RGBA )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_XYZ )
TT_PROTOTYPE_IMAGE_FROM_FILE ( TT_float )

#define	TT_PROTOTYPE_IMAGE_FROM_FILENAME( TYPE )			\
TYPE##_image	*TYPE##_image_from_filename ( char *filename );

TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_gray )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_gray16 )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGB )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGBf )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGBA )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_XYZ )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_float )

#define	TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION( TYPE )		\
TYPE##_image	*TYPE##_image_from_file_description ( TIFF *file,	\
		    char **description );

TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_gray )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_gray16 )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGB )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGBf )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_RGBA )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_XYZ )
TT_PROTOTYPE_IMAGE_FROM_FILE_DESCRIPTION ( TT_float )

#define	TT_PROTOTYPE_IMAGE_FROM_FILENAME_DESCRIPTION( TYPE )		\
TYPE##_image	*TYPE##_image_from_filename ( char *filename,		\
		    char **description );

TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_gray )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_gray16 )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGB )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGBf )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_RGBA )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_XYZ )
TT_PROTOTYPE_IMAGE_FROM_FILENAME ( TT_float )

#define	TT_PROTOTYPE_IMAGE_TO_FILE( TYPE )				\
void	TYPE##_image_to_file ( TIFF *file, TYPE##_image *image );

TT_PROTOTYPE_IMAGE_TO_FILE ( TT_gray )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_gray16 )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_RGB )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_RGBf )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_RGBA )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_XYZ )
TT_PROTOTYPE_IMAGE_TO_FILE ( TT_float )

#define	TT_PROTOTYPE_IMAGE_TO_FILENAME( TYPE )				\
void	TYPE##_image_to_filename ( char *filename, TYPE##_image *image );

TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_gray )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_gray16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_RGB )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_RGBf )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_RGBA )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_XYZ )
TT_PROTOTYPE_IMAGE_TO_FILENAME ( TT_float )

#define	TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION( TYPE )		\
void	TYPE##_image_to_filename_description ( char *filename,		\
		TYPE##_image *image, char *description  );

TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_gray )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_gray16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGB )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGBf )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_RGBA )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_XYZ )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION ( TT_float )

#define	TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS( TYPE )	\
void	TYPE##_image_to_filename_description_arguments ( char *filename,\
		TYPE##_image *image, int argc, char *argv[] );

TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_gray )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_gray16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_RGB )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_RGB16 )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_RGBf )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_RGBA )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_XYZ )
TT_PROTOTYPE_IMAGE_TO_FILENAME_DESCRIPTION_ARGUEMENTS ( TT_float )

void    TT_image_check_bounds ( TT_gray_image *deva_image, int row,
	            int col, int lineno, char *file );

#ifdef __cplusplus
}
#endif

#endif	/* __TIFFTOOLSIMAGE_H */
