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
 * LDR color formats are assumed to be in the sRGB color space, even though
 * the rgbe primaries are different.  No check is made for an actual color
 * profile.  Subtleties such as blackpoint and whitepoint are ignored.
 *
 * Special cased to convert TIFF equivalent 35mm focal length EXIF tag
 * value to Radiance field of view VIEW parameters.
 */

#include <stdlib.h>
#include <stdio.h>
#include "radiance-tiff.h"
#include "radiance-header.h"
#include "FOV.h"
#include "sRGB.h"
// #define	TT_CHECK_BOUNDS
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

char	*Usage = "tiff2rad [--sRGBprimaries] input.tif output.hdr";
int	args_needed = 2;

int
main ( int argc, char *argv[] )
{
    int		    sRGBprimaries_flag = FALSE;
    TIFF	    *input;
    DEVA_FOV	    deva_fov;
    TT_gray_image   *gray_image;
    TT_float_image  *float_image;
    TT_RGB_image    *RGB_image;
    TT_RGBf_image   *RGBf_image;
    TT_XYZ_image    *XYZ_image;
    RadianceHeader  header;
    int		    row, col;
    int		    argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( ( strcmp ( argv[argpt], "--sRGBprimaries" ) == 0 ) ||
		( strcmp ( argv[argpt], "-sRGBprimaries" ) == 0
		) ) {
	    sRGBprimaries_flag = TRUE;
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

    switch ( TT_file_type ( input ) ) {

	case TTTypeGray:

	    sRGBprimaries_flag = TRUE;

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

	    sRGBprimaries_flag = TRUE;

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

    if ( sRGBprimaries_flag ) {
	/*
	 * Convert to TIFF XYZ using sRGB primaries, then convert to Radiance
	 * RGB using Radiance primaries.
	 */
	XYZ_image = TT_XYZ_image_new ( TT_image_n_rows ( RGBf_image ),
		TT_image_n_cols ( RGBf_image ) );
	for ( row = 0; row < TT_image_n_rows ( XYZ_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( XYZ_image ); col++ ) {
		TT_image_data ( XYZ_image, row, col ) =
		    RGBf_to_XYZ ( TT_image_data ( RGBf_image, row, col ) );
	    }
	}
	TT_XYZ_image_to_radfilename ( argv[argpt++], XYZ_image, header);
		/* writes Radiance rgbe image using Radiance primaries */
	TT_XYZ_image_delete ( XYZ_image );
    } else {
	TT_RGBf_image_to_radfilename ( argv[argpt++], RGBf_image, header);
    }

    if ( header.header_text != NULL ) {
	free ( header.header_text );
    }

    TT_RGBf_image_delete ( RGBf_image );

    TIFFClose ( input );
    return ( EXIT_SUCCESS );	/* normal exit */
}
