/*
 * Converts RADIANCE image to TIFF image.
 *
 * Default output format is 32-bit float/color.
 *
 * Optional output format (--ldr flag) is 8-bit/color.  For 8-bit output, each
 * color is encoded using the sRGB conventions, even though the rgbe primaries
 * are different.  No actual color profile is attached to the ouput file.
 * Subtleties such as blackpoint and whitepoint are ignored.
 *
 * Special cased to convert TIFF equivalent 35mm focal length EXIF tag value to
 * Radiance field of view VIEW parameters.
 *
 * --fullrange flag causes output values to be linearly remapped to fill
 *  (almost) all of the available range.
 *
 * Does not set stonits tag.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "radiance-tiff.h"
#include "FOV.h"
#include "sRGB.h"
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

#define	GLARE_LEVEL_RATIO	5.0	/* RADIANCE identifies glare sources */
					/* as being brighter than 7 times the */
					/* average luminance level. This is */
					/* slightly more conservative.  */ 

char	*Usage =
"rad2tiff [--ldr] [--exposure=stops] [--fullrange] [--sRGBprimaries]"
    "\n\t[--autoadjust] [--unadjusted_values]"
    "\n\t[--compresszip] [--compresszipp] [--compresslzw] [--compresslzwp]"
    "\n\t\tinput.hdr output.tif";
int	args_needed = 2;

#include "sRGB_IEC61966-2-1_black_scaled.c"	/* hardwired binary profile */
#include "sRGB_IEC61966-2-1-linear.c"		/* hardwired binary profile */

/* for --fullrange flag: */
#define FULLRANGE_MAX		0.95	/* stay away from LCD clipping range */
#define FULLRANGE_MIN		0.0

/* for --halfrange flag: */
#define HALFRANGE_MAX		0.95	/* stay away from LCD clipping range */
#define HALFRANGE_MIN		0.22	/* ~ perceptual mid-gray */

typedef	enum { none,
		compresszip,
		compresszipp,
		compresslzw,
		compresslzwp
	    } COMPRESSION;

void	TT_RGBf_rescale ( TT_RGBf_image *image, float new_max, float new_min );
void	TT_RGBf_invert ( TT_RGBf_image *image );
void	set_compression ( TIFF *file, COMPRESSION compression_type );
double	find_glare_threshold ( TT_RGBf_image *image );
double	fmax3 ( double v1, double v2, double v3 );

int
main ( int argc, char *argv[] )
{
    int		    argpt = 1;
    COMPRESSION	    compression_type = none;
    TT_RGBf_image   *input_image;
    TT_XYZ_image    *XYZ_image;
    TT_RGB_image    *sRGB_image;
    int		    sRGBprimaries_flag = FALSE;
    int		    ldr_flag = FALSE;
    int		    exposure_flag = FALSE;
    double	    exposure_stops = 1.0;
    double	    exposure_adjust;
    int		    fullrange_flag = FALSE;
    int		    fullrange_invert_flag = FALSE;
    int		    halfrange_flag = FALSE;
    int		    halfrange_invert_flag = FALSE;
    int		    autoadjust_flag = FALSE;
    int		    unadjusted_values_flag = FALSE;
    int		    exposure_flag_count;
    float	    adjust_max;
    TIFF	    *output;
    RadianceHeader  header;
    int		    row, col;
    DEVA_FOV	    fov;
    char	    *new_description = NULL;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	    break;	/* read/write from/to stdin/stdout? */
	}
	if ( ( strcmp ( argv[argpt], "--ldr" ) == 0 ) ||
		( strcmp ( argv[argpt], "-ldr" ) == 0 ) ) {
	    ldr_flag = TRUE;
	    sRGBprimaries_flag = TRUE;	/* implied by --ldr */
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--fullrange" ) == 0 ) ||
		( strcmp ( argv[argpt], "-fullrange" ) == 0 ) ) {
	    fullrange_flag = TRUE;
	    argpt++;
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
	} else if ( ( strcmp ( argv[argpt], "--sRGBprimaries" ) == 0 ) ||
		( strcmp ( argv[argpt], "-sRGBprimaries" ) == 0 ) ) {
	    sRGBprimaries_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--autoadjust" ) == 0 ) ||
		( strcmp ( argv[argpt], "-autoadjust" ) == 0 ) ) {
	    autoadjust_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--unadjusted_values" ) == 0 ) ||
		( strcmp ( argv[argpt], "-unadjusted_values" ) == 0 ) ) {
	    unadjusted_values_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--compresszip" ) == 0 ) ||
		( strcmp ( argv[argpt], "-compresszip" ) == 0 ) ) {
	    if ( compression_type != none ) {
		fprintf ( stderr, "Can't mix compression types!\n" );
		exit ( EXIT_FAILURE );
	    }
	    compression_type = compresszip;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--compresszipp" ) == 0 ) ||
		( strcmp ( argv[argpt], "-compresszipp" ) == 0 ) ) {
	    if ( compression_type != none ) {
		fprintf ( stderr, "Can't mix compression types!\n" );
		exit ( EXIT_FAILURE );
	    }
	    compression_type = compresszipp;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--compresslzw" ) == 0 ) ||
		( strcmp ( argv[argpt], "-compresslzw" ) == 0 ) ) {
	    if ( compression_type != none ) {
		fprintf ( stderr, "Can't mix compression types!\n" );
		exit ( EXIT_FAILURE );
	    }
	    compression_type = compresslzw;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--compresslzwp" ) == 0 ) ||
		( strcmp ( argv[argpt], "-compresslzwp" ) == 0 ) ) {
	    if ( compression_type != none ) {
		fprintf ( stderr, "Can't mix compression types!\n" );
		exit ( EXIT_FAILURE );
	    }
	    compression_type = compresslzwp;
	    argpt++;

	/* hidden options */
	} else if ( ( strcmp ( argv[argpt], "--fullrangeinvert" ) == 0 ) ||
		( strcmp ( argv[argpt], "-fullrangeinvert" ) == 0) ) {
	    fullrange_invert_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--halfrange" ) == 0 ) ||
		( strcmp ( argv[argpt], "-halfrange" ) == 0) ) {
	    halfrange_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--halfrangeinvert" ) == 0 ) ||
		( strcmp ( argv[argpt], "-halfrangeinvert" ) == 0) ) {
	    halfrange_invert_flag = TRUE;
	    argpt++;
	} else if ( strncmp ( argv[argpt], "-description=",
		    strlen ( "-description=" ) ) == 0 ) {
	    new_description = argv[argpt] + strlen ( "-description=" );
	    argpt++;
	} else if ( strncmp ( argv[argpt], "--description=",
		    strlen ( "--description=" ) ) == 0 ) {
	    new_description = argv[argpt] + strlen ( "--description=" );
	    argpt++;
	} else {
	    fprintf ( stderr, "unknown argument (%s)!\n", argv[argpt] );
	    return ( EXIT_FAILURE );	/* error return */
	}
    }

    exposure_flag_count = 0;
    if ( exposure_flag ) { exposure_flag_count++; }
    if ( fullrange_flag ) { exposure_flag_count++; }
    if ( fullrange_invert_flag ) { exposure_flag_count++; }
    if ( halfrange_flag ) { exposure_flag_count++; }
    if ( halfrange_invert_flag ) { exposure_flag_count++; }
    if ( exposure_flag_count > 1 ) {
	fprintf ( stderr, "can't have more than one exposure-related flag!\n" );
	return ( EXIT_FAILURE );	/* error return */
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error return */
    }

    if ( sRGBprimaries_flag ) {
	/*
	 * Convert to XYZ using Radiance color primaries, then convert back
	 * to RGBf using sRGB primaries.
	 */
	if ( unadjusted_values_flag ) {
	    XYZ_image = TT_XYZ_image_from_radfilename_noeadj ( argv[argpt++],
		    &header );
	} else {
	    XYZ_image = TT_XYZ_image_from_radfilename ( argv[argpt++],
		    &header );
	}
	input_image = TT_RGBf_image_new ( TT_image_n_rows ( XYZ_image ),
		TT_image_n_cols ( XYZ_image ) );

	for ( row = 0; row < TT_image_n_rows ( XYZ_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( XYZ_image ); col++ ) {
		TT_image_data ( input_image, row, col ) =
		    XYZ_to_RGBf ( TT_image_data ( XYZ_image, row, col ) );
	    }
	}

	TT_XYZ_image_delete ( XYZ_image );
    } else {
	input_image = TT_RGBf_image_from_radfilename ( argv[argpt++], &header );
    }

    if ( fullrange_flag ) {
	TT_RGBf_rescale ( input_image, FULLRANGE_MAX, FULLRANGE_MIN );
    } else if ( fullrange_invert_flag ) {
	TT_RGBf_invert ( input_image );
	TT_RGBf_rescale ( input_image, FULLRANGE_MAX, FULLRANGE_MIN );
    } else if ( halfrange_flag ) {
	TT_RGBf_rescale ( input_image, HALFRANGE_MAX, HALFRANGE_MIN );
    } else if ( halfrange_invert_flag ) {
	TT_RGBf_invert ( input_image );
	TT_RGBf_rescale ( input_image, HALFRANGE_MAX, HALFRANGE_MIN );
    } else if ( autoadjust_flag ) {
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

    if ( ldr_flag ) {
	output = TT_RGB_open_write ( argv[argpt++],
		TT_image_n_rows ( input_image ),
		TT_image_n_cols ( input_image ) );

	set_compression ( output, compression_type );

	/* attach sRGB color profile */
	if ( !TIFFSetField ( output, TIFFTAG_ICCPROFILE, sizeof ( icc_sRGB ),
		    icc_sRGB ) ) {
	    fprintf (stderr, "can't set TIFFTAG_ICCPROFILE!\n" );
	    exit ( EXIT_FAILURE );
	}

	sRGB_image = TT_RGB_image_new ( TT_image_n_rows ( input_image ),
		TT_image_n_cols ( input_image ) );

	for ( row = 0; row < TT_image_n_rows ( input_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( input_image ); col++ ) {
		TT_image_data ( sRGB_image, row, col ) =
		    RGBf_to_sRGB ( TT_image_data ( input_image, row, col ) );
	    }
	}

	TT_RGB_image_to_file ( output, sRGB_image );
	if ( new_description == NULL ) {
	    /* strip trailing newline if present */
	    if ( ( header.header_text != NULL ) &&
		    ( strlen ( header.header_text ) >= 1 ) &&
		    (header.header_text[strlen ( header.header_text ) - 1]
		     == '\n' ) ) {
		header.header_text[strlen ( header.header_text ) - 1] = '\0';
	    }
	    TT_set_description ( output, header.header_text );
	} else {
	    TT_set_description ( output, new_description );
	}
	if ( fmax ( header.hFOV, header.vFOV ) > 0.0 ) {
	    /* based on diagonal */
	    fov.v_fov = header.vFOV;
	    fov.h_fov = header.hFOV;
	    set_tiff_fov_diag ( output, fov, TT_image_n_rows ( input_image ),
		    TT_image_n_cols ( input_image ) );
	    /************************* based on longest side
	      set_tiff_fov ( output, fmax ( header.hFOV, header.vFOV ) );
	     *************************/
	}
	TIFFClose ( output );
	TT_RGB_image_delete ( sRGB_image );
    } else {
	output = TT_RGBf_open_write ( argv[argpt++],
		TT_image_n_rows ( input_image ),
		TT_image_n_cols ( input_image ) );

	set_compression ( output, compression_type );

	if ( sRGBprimaries_flag ) {
	    /* attach sRGB color profile */
	    if ( !TIFFSetField ( output, TIFFTAG_ICCPROFILE,
			sizeof ( icc_sRGB_linear ), icc_sRGB_linear ) ) {
		fprintf (stderr, "Can't set TIFFTAG_ICCPROFILE!\n" );
		exit ( EXIT_FAILURE );
	    }
	}

	TT_RGBf_image_to_file ( output, input_image );
	if ( new_description == NULL ) {
	    /* strip trailing newline if present */
	    if ( ( header.header_text != NULL ) &&
		    ( strlen ( header.header_text ) >= 1 ) &&
		    ( header.header_text[strlen(header.header_text)-1]
		      == '\n' ) ) {
		header.header_text[strlen ( header.header_text ) - 1] = '\0';
	    }
	    TT_set_description ( output, header.header_text );
	} else {
	    TT_set_description ( output, new_description );
	}
	if ( fmax ( header.hFOV, header.vFOV ) > 0.0 ) {
	    /* based on diagonal */
	    fov.v_fov = header.vFOV;
	    fov.h_fov = header.hFOV;
	    set_tiff_fov_diag ( output, fov, TT_image_n_rows ( input_image ),
		    TT_image_n_cols ( input_image ) );
	    /************************* based on longest side
	      set_tiff_fov ( output, fmax ( header.hFOV, header.vFOV ) );
	     *************************/
	}
	TIFFClose ( output );
    }

    TT_RGBf_image_delete ( input_image );

    return ( EXIT_SUCCESS );	/* normal exit */
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

void
TT_RGBf_invert ( TT_RGBf_image *image )
{
    int	    row, col;
    int	    n_rows, n_cols;
    double  old_max, old_min;
    double  old_max_red, old_max_green, old_max_blue;
    double  old_min_red, old_min_green, old_min_blue;

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
		"TT_RGBf_invert: no variability in values (warning)\n" );

	return;
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    TT_image_data ( image, row, col ).red = old_max -
		( TT_image_data ( image, row, col ).red - old_min );
	    TT_image_data ( image, row, col ).green = old_max -
		( TT_image_data ( image, row, col ).green - old_min );
	    TT_image_data ( image, row, col ).blue = old_max -
		( TT_image_data ( image, row, col ).blue - old_min );
	}
    }
}

void
set_compression ( TIFF *file, COMPRESSION compression_type )
{
    switch ( compression_type ) {
	case none:
	    /* do nothing */
	    break;

	case compresszip:
	    TT_set_compression_zip ( file, FALSE );
	    break;

	case compresszipp:
	    TT_set_compression_zip ( file, TRUE );
	    break;

	case compresslzw:
	    TT_set_compression_LZW ( file, FALSE );
	    break;

	case compresslzwp:
	    TT_set_compression_LZW ( file, TRUE );
	    break;

	default:
	    fprintf ( stderr, "internal error!\n" );
	    exit ( EXIT_FAILURE );
    }
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
