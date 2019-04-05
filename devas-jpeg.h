/*
 * Like DeVAS_RGB_image_from_filename, DeVAS_RGB_image_from_file,
 * DeVAS_RGB_image_to_filename, and DeVAS_RGB_image to file, execept
 * works with JPEG files.
 *
 * Read routines are special cased to return value of
 * EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM tag, if present.
 * (General version of these routings, without the tag
 * processing, need to be written at some point.
 *
 * Based on example.c from jpeg-6b source, but appears to
 * also work with jpeg-8.
 */

#ifndef __DeVAS_JPEG_H
#define __DeVAS_JPEG_H

#include "devas-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_RGB_image	*DeVAS_RGB_image_from_filename_jpg ( char *filename,
		    char **comment, double *focal_length_35mm_equiv );
DeVAS_RGB_image	*DeVAS_RGB_image_from_file_jpg ( FILE *input,
		    char **comment, double *focal_length_35mm_equiv );

void		DeVAS_RGB_image_to_filename_jpg ( char *filename,
		    DeVAS_RGB_image *image, char *comment );
void		DeVAS_RGB_image_to_file_jpg ( FILE *output,
		    DeVAS_RGB_image *image, char *comment );

#ifdef __cplusplus
}
#endif
#endif  /* __DeVAS_JPEG_H */

