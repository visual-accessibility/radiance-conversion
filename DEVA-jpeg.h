/*
 * Like DEVA_RGB_image_from_filename, DEVA_RGB_image_from_file,
 * DEVA_RGB_image_to_filename, and DEVA_RGB_image to file, execept
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

#ifndef __DEVA_JPEG_H
#define __DEVA_JPEG_H

#include "deva-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DEVA_RGB_image	*DEVA_RGB_image_from_filename_jpg ( char *filename,
		    char **comment, double *focal_length_35mm_equiv );
DEVA_RGB_image	*DEVA_RGB_image_from_file_jpg ( FILE *input,
		    char **comment, double *focal_length_35mm_equiv );

void		DEVA_RGB_image_to_filename_jpg ( char *filename,
		    DEVA_RGB_image *image, char *comment );
void		DEVA_RGB_image_to_file_jpg ( FILE *output,
		    DEVA_RGB_image *image, char *comment );

#ifdef __cplusplus
	}
#endif
#endif  /* __DEVA_JPEG_H */

