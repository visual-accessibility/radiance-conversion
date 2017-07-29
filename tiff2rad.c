/*
 * Convert TIFF file to Radiance rgbe file.
 * 
 * Supported TIFF pixel types:
 *
 * 	TT_gray		Single channel (grayscale) 8-bit
 * 	TT_float	Single channel (grayscale) 32-bit float
 * 	TT_RGB		RGB 3 x 8-bit/pixel
 * 	TT_RGBf		RGB 3 x 32-bit float/pixel
 *
 * 8-bit RGB and grayscale files are assumed to have sRGB luminance
 * encoding regardless of whether or not there is an attached color profile
 * and ignoring the actual color profile if attached.  This is different
 * from the Radiance ra_tiff program for these data types, which
 * assumes straight gamma encoding.
 *
 * Special cased to convert TIFF equivalent 35mm focal length EXIF tag
 * value to Radiance field of view VIEW parameters.
 */

// #define	TT_CHECK_BOUNDS

#include <stdlib.h>
#include <stdio.h>
#include "radiance-tiff.h"
#include "radiance-header.h"
#include "FOV.h"
#include "sRGB.h"
#include "sRGB_radiance.h"
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

char	*Usage = "tiff2rad [--sRGBencoding] input.tif output.hdr";
int	args_needed = 2;

int
main ( int argc, char *argv[] )
{
    int		    sRGBencoding_flag = FALSE;
    TIFF	    *input;
    double	    stonits;
    DEVA_FOV	    deva_fov;
    TT_gray_image   *gray_image;
    TT_float_image  *float_image;
    TT_RGB_image    *RGB_image;
    TT_RGBf_image   *RGBf_image;
    RadianceHeader  header;
    RGBPRIMS	    radiance_prims = STDPRIMS;
    RGBPRIMS	    sRGB_prims = sRGBPRIMS;
    COLORMAT	    sRGB2radrgbmat;
    COLOR	    radiance_pixel_in;
    COLOR	    radiance_pixel_out;
    TT_RGBf	    TT_pixel;
    int		    row, col;
    int		    argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( ( strcmp ( argv[argpt], "--sRGBencoding" ) == 0 ) ||
		( strcmp ( argv[argpt], "-sRGBencoding" ) == 0
		) ) {
	    sRGBencoding_flag = TRUE;
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

    if ( TIFFGetField ( input, TIFFTAG_STONITS, &stonits ) ) {
	header.exposure_set = TRUE;
	header.exposure = WHTEFFICACY / stonits;
    } else {
	header.exposure_set = FALSE;
    }

    switch ( TT_file_type ( input ) ) {

	case TTTypeGray:

	    sRGBencoding_flag = TRUE;

	    gray_image = TT_gray_image_from_file ( input );
	    RGBf_image = TT_RGBf_image_new ( TT_image_n_rows ( gray_image ),
		    TT_image_n_cols ( gray_image ) );

	    /* based on diagonal */
	    deva_fov = get_tiff_fov_diag ( input,
		    TT_image_n_rows ( gray_image ),
		    TT_image_n_cols ( gray_image ) );
	    header.hFOV = deva_fov.h_fov;
	    header.vFOV = deva_fov.v_fov;

	    header.header_text = TT_get_description ( input );

	    for ( row = 0; row < TT_image_n_rows ( gray_image ); row++ ) {
		for ( col = 0; col < TT_image_n_cols ( gray_image ); col++ ) {
		    TT_image_data ( RGBf_image, row, col ).red =
			TT_image_data ( RGBf_image, row, col ).green =
			TT_image_data ( RGBf_image, row, col ).blue =
			gray_to_Y ( TT_image_data ( gray_image, row, col ) );
		    		/* assumes sRGB luminance encoding, */
				/* but does not alter primaries */
		}
	    }

	    TT_gray_image_delete ( gray_image );

	    break;

	case TTTypeFloat:

	    float_image = TT_float_image_from_file ( input );
	    RGBf_image = TT_RGBf_image_new ( TT_image_n_rows ( float_image ),
		    TT_image_n_cols ( float_image ) );

	    /* based on diagonal */
	    deva_fov = get_tiff_fov_diag ( input,
		    TT_image_n_rows ( float_image ),
		    TT_image_n_cols ( float_image ) );
	    header.hFOV = deva_fov.h_fov;
	    header.vFOV = deva_fov.v_fov;

	    header.header_text = TT_get_description ( input );

	    for ( row = 0; row < TT_image_n_rows ( float_image ); row++ ) {
		for ( col = 0; col < TT_image_n_cols ( float_image ); col++ ) {
		    TT_image_data ( RGBf_image, row, col ).red =
			TT_image_data ( RGBf_image, row, col ).green =
			TT_image_data ( RGBf_image, row, col ).blue =
			TT_image_data ( float_image, row, col );
		}
	    }

	    TT_float_image_delete ( float_image );

	    break;

	case TTTypeRGB:

	    sRGBencoding_flag = TRUE;

	    RGB_image = TT_RGB_image_from_file ( input );
	    RGBf_image = TT_RGBf_image_new ( TT_image_n_rows ( RGB_image ),
		    TT_image_n_cols ( RGB_image ) );

	    /* based on diagonal */
	    deva_fov = get_tiff_fov_diag ( input,
		    TT_image_n_rows ( RGB_image ),
		    TT_image_n_cols ( RGB_image ) );
	    header.hFOV = deva_fov.h_fov;
	    header.vFOV = deva_fov.v_fov;

	    header.header_text = TT_get_description ( input );

	    for ( row = 0; row < TT_image_n_rows ( RGB_image ); row++ ) {
		for ( col = 0; col < TT_image_n_cols ( RGB_image ); col++ ) {
		     TT_image_data ( RGBf_image, row, col ) =
		     sRGB_to_RGBf ( TT_image_data ( RGB_image, row, col ) );
		}
	    }

	    TT_RGB_image_delete ( RGB_image );

	    break;

	case TTTypeRGBf:

	    RGBf_image = TT_RGBf_image_from_file ( input );

	    /* based on diagonal */
	    deva_fov = get_tiff_fov_diag ( input,
		    TT_image_n_rows ( RGBf_image ),
		    TT_image_n_cols ( RGBf_image ) );
	    header.hFOV = deva_fov.h_fov;
	    header.vFOV = deva_fov.v_fov;

	    header.header_text = TT_get_description ( input );

	    break;

	default:
	    fprintf ( stderr, "can't convert file of type %s!\n",
		    TT_type2name ( TT_file_type ( input ) ) );
	    exit ( EXIT_FAILURE );
	    break;
    }

    if ( sRGBencoding_flag ) {
	/* convert to sRGB primaries */
	comprgb2rgbWBmat ( sRGB2radrgbmat, sRGB_prims, radiance_prims );

	for ( row = 0; row < TT_image_n_rows ( RGBf_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( RGBf_image ); col++ ) {
		TT_pixel = TT_image_data ( RGBf_image, row, col );

		colval ( radiance_pixel_in, RED ) = TT_pixel.red;
		colval ( radiance_pixel_in, GRN) = TT_pixel.green;
		colval ( radiance_pixel_in, BLU ) = TT_pixel.blue;

		colortrans ( radiance_pixel_out, sRGB2radrgbmat,
			radiance_pixel_in );

		TT_pixel.red = colval ( radiance_pixel_out, RED );
		TT_pixel.green = colval ( radiance_pixel_out, GRN );
		TT_pixel.blue = colval ( radiance_pixel_out, BLU );

		TT_image_data ( RGBf_image, row, col) = TT_pixel;
	    }
	}
    }

    TT_RGBf_image_to_radfilename ( argv[argpt++], RGBf_image, header);

    if ( header.header_text != NULL ) {
	free ( header.header_text );
    }

    TT_RGBf_image_delete ( RGBf_image );

    TIFFClose ( input );
    return ( EXIT_SUCCESS );	/* normal exit */
}
