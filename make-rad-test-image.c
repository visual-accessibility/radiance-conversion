/*
 * Make a simple Radiance test image with optional EXPOSURE setting.
 */

// #define	TT_CHECK_BOUNDS

#include <stdlib.h>
#include <stdio.h>
#include "radiance-tiff.h"
#include "radiance-header.h"
#include "FOV.h"
#include "TT-sRGB.h"
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "devas-license.h"

#define	N_ROWS		300
#define	N_COLS		300
#define	V_FOV		30.0
#define	H_FOV		30.0

char	*Usage = "make-rad-test-image value exposure output.hdr";
int	args_needed = 3;

int
main ( int argc, char *argv[] )
{
    TT_RGBf_image   *RGBf_image;
    TT_RGBf	    pixel_value;
    double	    value;
    double	    exposure;
    RadianceHeader  header;
    int		    row, col;
    int		    argpt = 1;

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error return */
    }

    value = atof ( argv[argpt++] );
    pixel_value.red = pixel_value.green = pixel_value.blue = value;

    exposure = atof ( argv[argpt++] );
    header.header_text = NULL;
    header.hFOV = H_FOV;
    header.vFOV = V_FOV;
    if ( exposure <= 0.0 ) {
	header.exposure_set = FALSE;
	header.exposure = 1.0;
    } else {
	header.exposure_set = TRUE;
	header.exposure = exposure;
    }

    RGBf_image = TT_RGBf_image_new ( N_ROWS, N_COLS );

    for ( row = 0; row < N_ROWS; row++ ) {
	for ( col = 0; col < N_COLS; col++ ) {
	    TT_image_data ( RGBf_image, row, col ) = pixel_value;
	}
    }

    TT_RGBf_image_to_radfilename ( argv[argpt++], RGBf_image, header );

    return ( EXIT_SUCCESS );	/* normal exit */
}
