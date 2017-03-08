/*
 * Converts RADIANCE image file to JPEG image file.
 *
 * Radiance values from 0.0 to 1.0 remapped to 8-bit unsigned int using
 * sRGB convention.  Subtleties such as blackpoint and whitepoint are ignored.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "radiance-tiff.h"
#include "TT-jpeg.h"
#include "FOV.h"
#include "sRGB.h"
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

#define GLARE_LEVEL_RATIO	5.0	/* RADIANCE identifies glare sources */
					/* as being brighter than 7 times the */
					/* average luminance level. This is */
					/* slightly more conservative.  */

char	*Usage =
	    "rad2jpeg [--exposure=stops] [--autoadjust] [--unadjusted_values]"
	    "\n\tinput.hdr output.jpg";
int	args_needed = 2;

#include "sRGB_IEC61966-2-1_black_scaled.c"	/* hardwired binary profile */

double	find_glare_threshold ( TT_RGBf_image *image );
double	fmax3 ( double v1, double v2, double v3 );
void	TT_RGBf_rescale ( TT_RGBf_image *image, float new_max, float new_min );

int
main ( int argc, char *argv[] )
{
    int		    argpt = 1;
    int		    exposure_flag = FALSE;
    double	    exposure_stops;
    double	    exposure_adjust = 1.0;
    int		    autoadjust_flag = FALSE;
    float	    adjust_max;
    int		    unadjusted_values_flag = FALSE;
    TT_XYZ_image    *XYZ_image;		/* to help with primaries conversion */
    TT_RGBf_image   *input_image;
    TT_RGB_image    *sRGB_image;
    RadianceHeader  header;
    int		    row, col;
    char	    *new_description = NULL;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	    break;	/* read from stdin */
	} else if ( strncmp ( argv[argpt], "--exposure=",
		    strlen ( "--exposure=" ) ) == 0 ) {
	    exposure_stops = atof ( argv[argpt] + strlen ( "--exposure=" ) );
	    exposure_flag = TRUE;
	    argpt++;
	} else if ( strncmp ( argv[argpt], "-exposure=",
		    strlen ( "-exposure=" ) ) == 0 ) {
	    exposure_stops = atof ( argv[argpt] + strlen ( "-exposure=" ) );
	    exposure_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--autoadjust" ) == 0 ) ||
		( strcmp ( argv[argpt], "-autoadjust" ) == 0 ) ) {
	    autoadjust_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--unadjusted_values" ) == 0 ) ||
		( strcmp ( argv[argpt], "-unadjusted_values" ) == 0 ) ) {
	    unadjusted_values_flag = TRUE;
	    argpt++;

	/* hidden options */
	} else if ( strncmp ( argv[argpt], "-description=",
		    strlen ( "-description=" ) ) == 0 ) {
	    new_description = argv[argpt] + strlen ( "-description=" );
	    argpt++;
	} else if ( strncmp ( argv[argpt], "--description=",
		    strlen ( "--description=" ) ) == 0 ) {
	    new_description = argv[argpt] + strlen ( "--description=" );
	    argpt++;
	} else {
	    fprintf ( stderr, "unknown argument!\n" );
	    return ( EXIT_FAILURE );	/* error return */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );        /* error return */
    }

    /*
     * Convert to XYZ using Radiance color primaries, then convert back
     * to RGBf using sRGB primaries.
     */

    if ( unadjusted_values_flag ) {
	XYZ_image = TT_XYZ_image_from_radfilename_noeadj ( argv[argpt++],
		&header );
    } else {
	XYZ_image = TT_XYZ_image_from_radfilename ( argv[argpt++], &header );
    }

    input_image = TT_RGBf_image_new ( TT_image_n_rows ( XYZ_image ),
	    TT_image_n_cols ( XYZ_image ) );

    for ( row = 0; row < TT_image_n_rows ( XYZ_image ); row++ ) {
	for ( col = 0; col < TT_image_n_cols ( XYZ_image ); col++ )
	{
	    TT_image_data ( input_image, row, col ) =
		XYZ_to_RGBf ( TT_image_data ( XYZ_image, row, col ) );
	}
    }

    if ( autoadjust_flag ) {
	adjust_max = find_glare_threshold ( input_image );
	if ( adjust_max > 0.0 ) {
	    TT_RGBf_rescale ( input_image, adjust_max, 0 );
	}
    }

    if ( exposure_flag ) {
	exposure_adjust = pow ( 2.0, exposure_stops );
	for ( row = 0; row < TT_image_n_rows ( input_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( input_image ); col++ ) {
		TT_image_data ( input_image, row, col ).red *= exposure_adjust;
		TT_image_data ( input_image, row, col ).green
		    *= exposure_adjust;
		TT_image_data ( input_image, row, col ).blue *= exposure_adjust;
	    }
	}
    }

    sRGB_image = TT_RGB_image_new ( TT_image_n_rows ( input_image ),
	    TT_image_n_cols ( input_image ) );

    for ( row = 0; row < TT_image_n_rows ( input_image ); row++ ) {
	for ( col = 0; col < TT_image_n_cols ( input_image ); col++ ) {
	    TT_image_data ( sRGB_image, row, col ) =
		RGBf_to_sRGB ( TT_image_data ( input_image, row, col ) );
	}
    }

    TT_RGB_image_to_filename_jpg ( argv[argpt++], sRGB_image, new_description );

    TT_RGB_image_delete ( sRGB_image );
    TT_RGBf_image_delete ( input_image );
    TT_XYZ_image_delete ( XYZ_image );

    return ( EXIT_SUCCESS );	/* normal exit */
}

double
find_glare_threshold ( TT_RGBf_image *image )
/*
 * Suggests a luminance level above which pixel should be considered
 * a glare source.  (This version uses maximum over R, G, and B, instead 
 * of luminance.)
 *
 * Uses a variant of the RADIANCE glare identification heuristic.  First,
 * average luminance is computed and used to set a preliminary glare
 * threshold value.  This average is not a robust estimator, since it is
 * strongly affected by very bright glare pixels or glare pixels covering
 * a large portion of the image.  To compensate for this, a second pass
 * is done in which a revised average luminance is computed based only on
 * pixels <= the preliminary glare threshold.  This revised average luminance
 * is then used to compute a revised glare threshold, which is returned as
 * the value of the function.
 */
{
    int		    row, col;
    double	    max_pixel_value;
    double	    max_value;
    double	    average_value_initial;
    double	    glare_cutoff_initial;

    max_value = average_value_initial = 0.0;

    for ( row = 0; row < TT_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < TT_image_n_cols ( image ); col++ ) {
	    max_pixel_value = fmax3 ( TT_image_data (image, row, col ) . red,
		    TT_image_data (image, row, col ) . green,
		    TT_image_data (image, row, col ) . blue );
	    if ( max_value < max_pixel_value ) {
		max_value = max_pixel_value;
	    }

	    average_value_initial += max_pixel_value;
	}
    }

    average_value_initial /=
	( ((double) TT_image_n_rows ( image ) ) *
	    ((double) TT_image_n_cols ( image ) ) );
    glare_cutoff_initial = GLARE_LEVEL_RATIO * average_value_initial;

    if ( glare_cutoff_initial >= max_value ) {
	/* no need for glare source clipping */
	return ( max_value );
    }

    max_value = 0.0;

    for ( row = 0; row < TT_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < TT_image_n_cols ( image ); col++ ) {
	    max_pixel_value = fmax3 ( TT_image_data (image, row, col ) . red,
		    TT_image_data (image, row, col ) . green,
		    TT_image_data (image, row, col ) . blue );
	    if ( max_pixel_value <= glare_cutoff_initial ) {
		if ( max_value < max_pixel_value ) {
		    max_value = max_pixel_value;
		}
	    }
	}
    }

    return ( max_value );
}

double
fmax3 ( double v1, double v2, double v3 )
{
    return ( fmax ( v1, fmax ( v2, v3 ) ) );
}

void
TT_RGBf_rescale ( TT_RGBf_image *image, float new_max, float new_min )
{
    int	    row, col;
    int	    n_rows, n_cols;
    double  old_max, old_min;
    double  old_max_red, old_max_green, old_max_blue;
    double  old_min_red, old_min_green, old_min_blue;
    double  rescale;

    n_rows = TT_image_n_rows ( image );
    n_cols = TT_image_n_cols ( image );

    old_max_red = old_min_red = TT_image_data ( image, 0, 0 ).red;
    old_max_green = old_min_green = TT_image_data ( image, 0, 0 ).green;
    old_max_blue = old_min_blue = TT_image_data ( image, 0, 0 ).blue;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    old_max_red =
		fmax ( old_max_red, TT_image_data ( image, row, col ).red );
	    old_max_green =
		fmax ( old_max_green, TT_image_data ( image, row, col ).green );
	    old_max_blue =
		fmax ( old_max_blue, TT_image_data ( image, row, col ).blue );

	    old_min_red =
		fmin ( old_min_red, TT_image_data ( image, row, col ).red );
	    old_min_green =
		fmin ( old_min_green, TT_image_data ( image, row, col ).green );
	    old_min_blue =
		fmin ( old_min_blue, TT_image_data ( image, row, col ).blue );
	}
    }

    old_max = fmax ( old_max_red, fmax ( old_max_green, old_max_blue ) );
    old_min = fmin ( old_min_red, fmin ( old_min_green, old_min_blue ) );

    if ( old_max == old_min ) {
	fprintf ( stderr,
		"TT_RGBf_rescale: no variability in values (warning)\n" );

	for ( row = 0; row < n_rows; row++ ) {
	    for ( col = 0; col < n_cols; col++ ) {
		TT_image_data ( image, row, col ).red =
		    TT_image_data ( image, row, col ).green =
		    TT_image_data ( image, row, col ).blue =
		   	 0.5 * ( new_max + new_min );
	    }
	}
	return;
    }

    rescale = ( new_max - new_min ) / ( old_max - old_min );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    TT_image_data ( image, row, col ).red = ( rescale *
		    ( TT_image_data ( image, row, col ).red - old_min ) ) +
				new_min;
	    TT_image_data ( image, row, col ).green = ( rescale *
		    ( TT_image_data ( image, row, col ).green - old_min ) ) +
				new_min;
	    TT_image_data ( image, row, col ).blue = ( rescale *
		    ( TT_image_data ( image, row, col ).blue - old_min ) ) +
				new_min;
	}
    }
}
