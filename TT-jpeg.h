/*
 * Like TT_RGB_image_from_filename, TT_RGB_image_from_file,
 * TT_RGB_image_to_filename, and TT_RGB_image to file, execept
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

#ifndef __TT_JPEG_H
#define __TT_JPEG_H

#include "tifftools.h"
#include "tifftoolsimage.h"

#ifdef __cplusplus
extern "C" {
#endif

TT_RGB_image	*TT_RGB_image_from_filename_jpg ( char *filename,
		    char **comment, double *focal_length_35mm_equiv );
TT_RGB_image	*TT_RGB_image_from_file_jpg ( FILE *input,
		    char **comment, double *focal_length_35mm_equiv );

void		TT_RGB_image_to_filename_jpg ( char *filename,
		    TT_RGB_image *image, char *comment );
void		TT_RGB_image_to_file_jpg ( FILE *output,
		    TT_RGB_image *image, char *comment );

#ifdef __cplusplus
	}
#endif
#endif  /* __TT_JPEG_H */

