/*
 * Convert TIFF file floating point file to TIFF 8-bit file.
 * (This is a strange program to have in a Radiance-related repository,
 * but there was no other good place to put it.)
 *
 * Converts:	TT_float to TT_gray (single channel)
 * 		TT_RGBf to TT_RGB (3 x 8-bit/pixel)
 *
 * 8-bit output images use sRGB luminance enconding unless the --linear
 * flag is given.
 *
 * --offset option adds 0.5 to floating point values to help with displaying
 * negative values.
 */

// #define	TT_CHECK_BOUNDS

#include <stdlib.h>
#include <stdio.h>
#include "TT-sRGB.h"
#include "tifftoolsimage.h"
#include "deva-license.h"

char	*Usage = "tiff32_to_8 [--linear] [--offset] input.tif output.hdr";
int	args_needed = 2;

int
main ( int argc, char *argv[] )
{
    int		    linear_flag = FALSE;
    int		    offset_flag = FALSE;
    TIFF	    *input;
    TT_gray_image   *gray_image;
    TT_float_image  *float_image;
    TT_RGB_image    *RGB_image;
    TT_RGBf_image   *RGBf_image;
    int		    row, col;
    int		    argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( ( strcmp ( argv[argpt], "--linear" ) == 0 ) ||
		( strcmp ( argv[argpt], "-linear" ) == 0 ) ) {
	    linear_flag = TRUE;
	    argpt++;

	/* hidden options */

	} else if ( ( strcmp ( argv[argpt], "--offset" ) == 0 ) ||
		( strcmp ( argv[argpt], "-offset" ) == 0 ) ) {
	    offset_flag = TRUE;
	    argpt++;
	} else {
	    fprintf ( stderr, "unknown argument (%s)!\n", argv[argpt] );
	    return ( EXIT_FAILURE );	/* error return */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error return */
    }

    input = TIFFOpen ( argv[argpt++], "r" );
    if ( input == NULL ) {
	perror ( argv[argpt-1] );
	exit ( EXIT_FAILURE );
    }

    if ( offset_flag && ( TT_file_type ( input ) != TTTypeFloat ) ) {
	fprintf ( stderr,
		"--offset only works for float (1 channel) input!\n" );
	exit ( EXIT_FAILURE );
    }

    switch ( TT_file_type ( input ) ) {

	case TTTypeFloat:

	    float_image = TT_float_image_from_file ( input );
	    gray_image = TT_gray_image_new ( TT_image_n_rows ( float_image ),
		    TT_image_n_cols ( float_image ) );

	    if ( offset_flag ) {
		for ( row = 0; row < TT_image_n_rows ( float_image ); row++ ) {
		    for ( col = 0; col < TT_image_n_cols ( float_image );
			    col++ ) {
			TT_image_data ( float_image, row, col ) += 0.5;
		    }
		}
	    }

	    if ( linear_flag ) {
		for ( row = 0; row < TT_image_n_rows ( float_image ); row++ ) {
		    for ( col = 0; col < TT_image_n_cols ( float_image );
			    col++ ) {
			TT_image_data ( gray_image, row, col ) =
			    Y_to_graylinear ( TT_image_data ( float_image, row,
					col ) );
		    }
		}
	    } else {
		for ( row = 0; row < TT_image_n_rows ( float_image ); row++ ) {
		    for ( col = 0; col < TT_image_n_cols ( float_image );
			    col++ ) {
			TT_image_data ( gray_image, row, col ) =
			    Y_to_gray ( TT_image_data ( float_image, row,
					col ) );
		    }
		}
	    }

	    TT_gray_image_to_filename ( argv[argpt++], gray_image );

	    TT_float_image_delete ( float_image );
	    TT_gray_image_delete ( gray_image );

	    break;

	case TTTypeRGBf:

	    RGBf_image = TT_RGBf_image_from_file ( input );
	    RGB_image = TT_RGB_image_new ( TT_image_n_rows ( RGBf_image ),
		    TT_image_n_cols ( RGBf_image ) );

	    if ( linear_flag ) {
		for ( row = 0; row < TT_image_n_rows ( RGBf_image ); row++ ) {
		    for ( col = 0; col < TT_image_n_cols ( RGBf_image );
			    col++ ) {
			TT_image_data ( RGB_image, row, col ) =
			    RGBf_to_RGB ( TT_image_data ( RGBf_image, row,
					col ) );
		    }
		}
	    } else {
		for ( row = 0; row < TT_image_n_rows ( RGBf_image ); row++ ) {
		    for ( col = 0; col < TT_image_n_cols ( RGBf_image );
			    col++ ) {
			TT_image_data ( RGB_image, row, col ) =
			    RGBf_to_sRGB ( TT_image_data ( RGBf_image, row,
					col ) );
		    }
		}
	    }

	    TT_RGB_image_to_filename ( argv[argpt++], RGB_image );

	    TT_RGBf_image_delete ( RGBf_image );
	    TT_RGB_image_delete ( RGB_image );

	    break;

	default:
	    fprintf ( stderr, "can't convert file of type %s!\n",
		    TT_type2name ( TT_file_type ( input ) ) );
	    exit ( EXIT_FAILURE );
	    break;
    }

    TIFFClose ( input );

    return ( EXIT_SUCCESS );	/* normal exit */
}
