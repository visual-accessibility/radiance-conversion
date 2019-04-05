/*
 * Like DeVAS_RGB_image_from_filename, DeVAS_RGB_image_from_file,
 * DeVAS_RGB_image_to_filename, and DeVAS_RGB_image to file, execept
 * works with PNG files.
 *
 * Read routines are special cased to return value of
 * EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM tag, if present.
 * (General version of these routings, without the tag
 * processing, need to be written at some point.
 *
 * Based on example.c from jpeg-6b source, but appears to
 * also work with jpeg-8.
 */

#ifndef __DeVAS_PNG_H
#define __DeVAS_PNG_H

#include "devas-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_RGB_image	*DeVAS_RGB_image_from_filename_png ( char *filename );
DeVAS_RGB_image	*DeVAS_RGB_image_from_file_png ( FILE *input );

DeVAS_gray_image	*DeVAS_gray_image_from_filename_png ( char *filename );
DeVAS_gray_image	*DeVAS_gray_image_from_file_png ( FILE *input );

void		DeVAS_RGB_image_to_filename_png ( char *filename,
		    DeVAS_RGB_image *image );
void		DeVAS_RGB_image_to_file_png ( FILE *output,
		    DeVAS_RGB_image *image );

void		DeVAS_gray_image_to_filename_png ( char *filename,
		    DeVAS_gray_image *image );
void		DeVAS_gray_image_to_file_png ( FILE *output,
		    DeVAS_gray_image *image );

#ifdef __cplusplus
}
#endif
#endif  /* __DeVAS_PNG_H */

