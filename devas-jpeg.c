/*
 * Read routines are special cased to return value of
 * EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM tag, if present.
 * (General version of these routings, without the tag
 * processing, need to be written at some point.)
 *
 * Based on example.c from jpeg-6b source, but appears to
 * also work with jpeg-8.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jpeglib.h>
#include <setjmp.h>	/* needed for error returns from libjpeg */
#include <assert.h>
#include <libexif/exif-data.h>
#include "devas-image.h"
#include "devas-jpeg.h"
#include "iccjpeg.h"
#include "radiance-conversion-version.h"
#include "devas-license.h"

#define	DEFAULT_QUALITY		80

#ifdef _mingw64_cross
/* Windows stdlib doesn't have strndup. */
static char *strndup ( const char *s, size_t n );
#endif	/*  _mingw64_cross */

DeVAS_RGB_image *
DeVAS_RGB_image_from_filename_jpg ( char *filename, char **comment,
	double *focal_length_35mm_equiv )
{
    FILE    *file;

    file = fopen ( filename, "rb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    return ( DeVAS_RGB_image_from_file_jpg ( file, comment,
		focal_length_35mm_equiv ) );
}

/* libjpeg error handling */

struct jpeg_DeVAS_error_mgr {
    struct  jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct jpeg_DeVAS_error_mgr *jpeg_DeVAS_error_ptr;

METHODDEF(void)
jpeg_DeVAS_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a jpeg_DeVAS_error_mgr struct,
       so coerce pointer */
    jpeg_DeVAS_error_ptr myerr = (jpeg_DeVAS_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp ( myerr->setjmp_buffer, 1 );
}

DeVAS_RGB_image *
DeVAS_RGB_image_from_file_jpg ( FILE *input, char **comment,
	double *focal_length_35mm_equiv )
{
    struct	    jpeg_decompress_struct cinfo;
    struct	    jpeg_DeVAS_error_mgr jerr;
    jpeg_saved_marker_ptr    marker_list;
    int		    image_data_offset;	/* in units of DeVAS_RGB! */
    JSAMPROW	    row_pointer[1];	/* pointer to JSAMPLE row[s] */
    unsigned int    n_rows, n_cols;
    DeVAS_RGB_image    *image;
    ExifData	    *exif_data;
    ExifContent	    *exif_content;
    ExifEntry	    *exif_entry;
    ExifByteOrder   exif_byte_order;
    char	    *comment_malloc;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error ( &jerr.pub );
    jerr.pub.error_exit = jpeg_DeVAS_error_exit;
    /* Establish the setjmp return context for jpeg_DeVAS_error_exit to use. */
    if ( setjmp ( jerr.setjmp_buffer ) ) {
	/*
	 * If we get here, the JPEG code has signaled an error.  We need to
	 * clean up the JPEG object, close the input file, and return.
	 */
	jpeg_destroy_decompress ( &cinfo );
	fclose ( input );
	exit ( EXIT_FAILURE );
    }

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress ( &cinfo );

    /* Step 2: specify data source (eg, a file) */

    jpeg_stdio_src ( &cinfo, input );

    /* Step 2.5: grab COM and APP1 markers (EXIF data) if present */

    jpeg_save_markers ( &cinfo, JPEG_COM, 0xffff );
    jpeg_save_markers ( &cinfo, JPEG_APP0+1, 0xffff );

    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header ( &cinfo, TRUE );
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

    /* Step 3.5:  Decode comment marker and (partially) decode EXIF data */

    if ( focal_length_35mm_equiv != NULL ) {
	*focal_length_35mm_equiv = 0.0;
    }

    for ( marker_list = cinfo.marker_list; marker_list;
	    marker_list = marker_list->next ) {
	if ( focal_length_35mm_equiv != NULL ) {
	    if ( ( marker_list->marker == (JPEG_APP0+1) ) &&
		    ( strncmp ( (const char *) marker_list->data, "Exif", 4 )
		      == 0 ) ) {
		/* found EXIF record */
		exif_data = exif_data_new_from_data ( marker_list->data,
			marker_list->data_length );
		/* exif_data_dump ( exif_data ); */
		if (exif_data->ifd[EXIF_IFD_EXIF] &&
			exif_data->ifd[EXIF_IFD_EXIF]->count) {
		    exif_content = exif_data->ifd[EXIF_IFD_EXIF];
		    exif_entry = exif_content_get_entry ( exif_content,
			    EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM );
		    if ( exif_entry != NULL ) {
			exif_byte_order =
			    exif_data_get_byte_order
			    (exif_entry->parent->parent);
			*focal_length_35mm_equiv =
			    (double) exif_get_short ( exif_entry->data,
				    exif_byte_order );
		    }
		}
	    }
	}

	if ( comment != NULL ) {
	    if ( marker_list->marker == JPEG_COM ) {
		comment_malloc = (char *) malloc ( sizeof ( char ) *
			( marker_list->data_length + 1 ) );
				/* leave room for trailing null */
		comment_malloc = strndup ( (const char *) marker_list->data,
					(size_t ) marker_list->data_length );
		if ( comment_malloc == NULL ) {
		    fprintf ( stderr,
			    "DeVAS_RGB_image_from_file_jpg: malloc failed!\n " );
		    exit ( EXIT_FAILURE );
		}
		*comment = comment_malloc;
	    } else {
		*comment = NULL;
	    }
	}
    }

    /* Step 4: set parameters for decompression */

    /* Use defaults */

    /* Step 5: Start decompressor */

    (void) jpeg_start_decompress ( &cinfo );
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available.
     */

    if ( ( cinfo.out_color_space != JCS_RGB ) ||
	    ( cinfo.out_color_components != 3 ) ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_file_jpg: input image not RGB!\n" );
	exit ( EXIT_FAILURE );
    }

    n_rows = cinfo.output_height;
    n_cols = cinfo.output_width;

    image = DeVAS_RGB_image_new ( n_rows, n_cols );

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */


    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    while (cinfo.output_scanline < cinfo.output_height) {
	/*
	 * jpeg_read_scanlines expects an array of pointers to scanlines.
	 * Here the array is only one element long, but you could ask for
	 * more than one scanline at a time if that's more convenient.
	 */
	image_data_offset = cinfo.output_scanline * cinfo.output_width;
	/*
	 * Assume DeVAS_RGB_image_new allocates image data contiguously
	 * with no padding.
	 */
	assert ( sizeof ( DeVAS_RGB ) == 3 );	/* just checking... */
	row_pointer[0] =
	    (JSAMPROW) ( &DeVAS_image_data ( image, 0, 0 ) + image_data_offset );

	(void) jpeg_read_scanlines ( &cinfo, row_pointer, 1 );
    }

    /* Step 7: Finish decompression */

    (void) jpeg_finish_decompress ( &cinfo );
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress ( &cinfo );

    /* After finish_decompress, we can close the input file.
     * Here we postpone it until after no more JPEG errors are possible,
     * so as to simplify the setjmp error logic above.  (Actually, I don't
     * think that jpeg_destroy can do an error exit, but why assume anything...)
     */
    fclose ( input );

    /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

    if ( jerr.pub.num_warnings > 0 ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_file_jpg: %d decompression warnings!\n",
	       		(int) jerr.pub.num_warnings );
	exit ( EXIT_FAILURE );
    }

    /* And we're done! */
    return ( image );
}

void
DeVAS_RGB_image_to_filename_jpg ( char *filename, DeVAS_RGB_image *image,
	char *comment )
{
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DeVAS_RGB_image_to_file_jpg ( file, image, comment );

    fclose ( file );
}

void
DeVAS_RGB_image_to_file_jpg ( FILE *output, DeVAS_RGB_image *image,
	char *comment )
{
    struct jpeg_compress_struct	cinfo;
    struct jpeg_DeVAS_error_mgr	jerr;

    JSAMPROW	    row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int		    image_data_offset;	/* in units of DeVAS_RGB! */
    unsigned int    n_rows, n_cols;
    int		    quality;
#include "sRGB_IEC61966-2-1_black_scaled.c"	/* hardwared binary profile */

    /* Step 1: allocate and initialize JPEG compression object */

    /* We have to set up the error handler first, in case the initialization
     * step fails.  (Unlikely, but it could happen if you are out of memory.)
     * This routine fills in the contents of struct jerr, and returns jerr's
     * address which we place into the link field in cinfo.
     */
    cinfo.err = jpeg_std_error ( &jerr.pub );
    jerr.pub.error_exit = jpeg_DeVAS_error_exit;
    /* Establish the setjmp return context for jpeg_DeVAS_error_exit to use. */
    if ( setjmp ( jerr.setjmp_buffer ) ) {
	/*
	 * If we get here, the JPEG code has signaled an error.  We need to
	 * clean up the JPEG object, close the input file, and return.
	 */
	jpeg_destroy_compress ( &cinfo );
	fclose ( output );
	exit ( EXIT_FAILURE );
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress ( &cinfo );

    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */

    /* Here we use the library-supplied code to send compressed data to a
     * stdio stream.  You can also write your own code to do something else.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to write binary files.
     */
    jpeg_stdio_dest ( &cinfo, output );

    n_rows = DeVAS_image_n_rows ( image );
    n_cols = DeVAS_image_n_cols ( image );

    /* Step 3: set parameters for compression */

    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */
    cinfo.image_width = n_cols; 	/* image width and height, in pixels */
    cinfo.image_height = n_rows;
    cinfo.input_components = 3;		/* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
    /* Now use the library's routine to set default compression parameters.
     * (You must set at least cinfo.in_color_space before calling this,
     * since the defaults depend on the source color space.)
     */
    jpeg_set_defaults ( &cinfo );
    /* Now you can set any non-default parameters you wish to.
     * Here we just illustrate the use of quality (quantization table) scaling:
     */
    quality = DEFAULT_QUALITY;
    jpeg_set_quality ( &cinfo, quality, TRUE );
	    			/* limit to baseline-JPEG values */

    /* Step 4: Start compressor */

    /* TRUE ensures that we will write a complete interchange-JPEG file.
     * Pass TRUE unless you are very sure of what you're doing.
     */
    jpeg_start_compress ( &cinfo, TRUE );

    /* Step 4.25: copy comment. */

    if ( ( comment != NULL ) && ( *comment != '\0' ) ) {
	jpeg_write_marker ( &cinfo, JPEG_COM, (const JOCTET *) comment,
		strlen ( comment ) );
    }

    /* Step 4.5: Attache sRGB profile (*not* optional!!!) */
    jpeg_write_icc_profile ( &cinfo, (const JOCTET *) icc_sRGB,
	    sizeof ( icc_sRGB ) );

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */

    /* Here we use the library's state variable cinfo.next_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     * To keep things simple, we pass one scanline per call; you can pass
     * more if you wish, though.
     */
    while (cinfo.next_scanline < cinfo.image_height) {
	/*
	 * jpeg_write_scanlines expects an array of pointers to scanlines.
	 * Here the array is only one element long, but you could pass
	 * more than one scanline at a time if that's more convenient.
	 */
	image_data_offset = cinfo.next_scanline * n_cols;
	/*
	 * Assume DeVAS_RGB_image_new allocates image data contiguously
	 * with no padding.
	 */
	assert ( sizeof ( DeVAS_RGB ) == 3 );      /* just checking... */
	row_pointer[0] =
	   (JSAMPROW) ( &DeVAS_image_data ( image, 0, 0 ) + image_data_offset );
	(void) jpeg_write_scanlines ( &cinfo, row_pointer, 1 );
    }

    /* Step 6: Finish compression */

    jpeg_finish_compress ( &cinfo );

    fflush ( output );	/* don't actually close file */

    /* Step 7: release JPEG compression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_compress ( &cinfo );

    /* And we're done! */
}

#ifdef _mingw64_cross
/* Windows stdlib doesn't have strndup. */
static char *
strndup ( const char *s, size_t n )
{
    char    *retbuf;

    if ( strlen ( s ) > n ) {
	retbuf = malloc ( ( n + 1 ) * sizeof ( char ) );
	if ( retbuf == NULL ) {
	    fprintf ( stderr, "strndup (Windows substitute): malloc failed!" );
	    exit ( EXIT_FAILURE );
	}

	memcpy ( retbuf, s, n );
	retbuf[n] = '\0';

	return ( retbuf );
    }

    return ( strdup ( s ) );
}
#endif	/*  _mingw64_cross */
