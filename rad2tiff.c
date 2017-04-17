/*
 * Converts RADIANCE image to TIFF image.
 *
 * Default output format is 32-bit float/color.
 *
 * Optional output format (--ldr flag) is 8-bit/color.  For 8-bit output, each
 * color is encoded using the sRGB conventions.
 *
 * Special cased to convert TIFF equivalent 35mm focal length EXIF tag value to
 * Radiance field of view VIEW parameters.
 *
 * --fullrange flag causes output values to be linearly remapped to fill
 *  (almost) all of the available range.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "radiance-tiff.h"
#include "FOV.h"
#include "sRGB.h"
#include "sRGB_radiance.h"
#include "tifftoolsimage.h"
#include "radiance/color.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

#define	GLARE_LEVEL_RATIO	5.0	/* RADIANCE identifies glare sources */
					/* as being brighter than 7 times the */
					/* average luminance level. This is */
					/* slightly more conservative.  */ 

char	*Usage =
"rad2tiff [--ldr] [--exposure=stops] [--fullrange] [--sRGBencoding]"
    "\n\t[--autoadjust] [--original-units|--photometric-units]"
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
TT_RGBf	TT_RGBf_scalar_mult ( TT_RGBf original, double multiplier );
void	original_units ( TT_RGBf_image *image, RadianceHeader *header );
void	photometric_units ( TT_RGBf_image *image, RadianceHeader *header );

int
main ( int argc, char *argv[] )
{
    COMPRESSION	    compression_type = none;
    TT_RGBf_image   *input_image;
    TT_RGB_image    *sRGB_image;
    int		    sRGBencoding_flag = FALSE;
    int		    ldr_flag = FALSE;
    int		    exposure_flag = FALSE;
    double	    exposure_stops = 1.0;
    double	    exposure_adjust;
    int		    fullrange_flag = FALSE;
    int		    fullrange_invert_flag = FALSE;
    int		    halfrange_flag = FALSE;
    int		    halfrange_invert_flag = FALSE;
    int		    autoadjust_flag = FALSE;
    int		    original_units_flag = FALSE;
    int		    photometric_units_flag = FALSE;
    int		    exposure_flag_count;
    float	    adjust_max;
    TIFF	    *output;
    RadianceHeader  header;
    int		    row, col;
    DEVA_FOV	    fov;
    char	    *new_description = NULL;
    RGBPRIMS	    radiance_prims = STDPRIMS;
    RGBPRIMS	    sRGB_prims = sRGBPRIMS;
    COLORMAT	    radrgb2sRGBmat;
    COLOR	    radiance_pixel_in;
    COLOR	    radiance_pixel_out;
    TT_RGBf	    TT_pixel;
    int		    argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	    break;	/* read/write from/to stdin/stdout? */
	}
	if ( ( strcmp ( argv[argpt], "--ldr" ) == 0 ) ||
		( strcmp ( argv[argpt], "-ldr" ) == 0 ) ) {
	    ldr_flag = TRUE;
	    sRGBencoding_flag = TRUE;	/* implied by --ldr */
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
	} else if ( ( strcmp ( argv[argpt], "--sRGBencoding" ) == 0 ) ||
		( strcmp ( argv[argpt], "-sRGBencoding" ) == 0 ) ) {
	    sRGBencoding_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--autoadjust" ) == 0 ) ||
		( strcmp ( argv[argpt], "-autoadjust" ) == 0 ) ) {
	    autoadjust_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--original-units" ) == 0 ) ||
		( strcmp ( argv[argpt], "-original-units" ) == 0 ) ) {
	    original_units_flag = TRUE;
	    argpt++;
	} else if ( ( strcmp ( argv[argpt], "--photometric-units" ) == 0 ) ||
		( strcmp ( argv[argpt], "-photometric-units" ) == 0 ) ) {
	    photometric_units_flag = TRUE;
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

    if ( original_units_flag && photometric_units_flag ) {
	fprintf ( stderr,
		"can't mix --original_units and --photometric_units!\n" );
	return ( EXIT_FAILURE );	/* error return */
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

    input_image = TT_RGBf_image_from_radfilename ( argv[argpt++], &header );

    if ( original_units_flag ) {
	original_units ( input_image, &header );
    } else if ( photometric_units_flag ) {
	photometric_units ( input_image, &header );
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

    if ( sRGBencoding_flag ) {
	/* convert to sRGB primaries */
	comprgb2rgbWBmat ( radrgb2sRGBmat, radiance_prims, sRGB_prims );

	for ( row = 0; row < TT_image_n_rows ( input_image ); row++ ) {
	    for ( col = 0; col < TT_image_n_cols ( input_image ); col++ ) {
		TT_pixel = TT_image_data ( input_image, row, col );

		colval ( radiance_pixel_in, RED ) = TT_pixel.red;
		colval ( radiance_pixel_in, GRN ) = TT_pixel.green;
		colval ( radiance_pixel_in, BLU ) = TT_pixel.blue;

		colortrans ( radiance_pixel_out, radrgb2sRGBmat,
			radiance_pixel_in );

		TT_pixel.red = colval ( radiance_pixel_out, RED );
		TT_pixel.green = colval ( radiance_pixel_out, GRN );
		TT_pixel.blue = colval ( radiance_pixel_out, BLU );

		TT_image_data ( input_image, row, col ) = TT_pixel;
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

	if ( original_units_flag ) {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS, WHTEFFICACY ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
		exit ( EXIT_FAILURE );
	    }
	} else if ( photometric_units_flag ) {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS, 1.0 ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
		exit ( EXIT_FAILURE );
	    }
	} else {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS,
			WHTEFFICACY / header.exposure ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
		exit ( EXIT_FAILURE );
	    }
	}

	sRGB_image = TT_RGB_image_new ( TT_image_n_rows ( input_image ),
		TT_image_n_cols ( input_image ) );

	/* convert to 8-bit values using sRGB non-linear encoding */
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

	if ( sRGBencoding_flag ) {
	    /* attach sRGB color profile */
	    if ( !TIFFSetField ( output, TIFFTAG_ICCPROFILE,
			sizeof ( icc_sRGB_linear ), icc_sRGB_linear ) ) {
		fprintf (stderr, "Can't set TIFFTAG_ICCPROFILE!\n" );
		exit ( EXIT_FAILURE );
	    }
	}

	if ( original_units_flag ) {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS, WHTEFFICACY ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
		exit ( EXIT_FAILURE );
	    }
	} else if ( photometric_units_flag ) {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS, 1.0 ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
		exit ( EXIT_FAILURE );
	    }
	} else {
	    if ( !TIFFSetField ( output, TIFFTAG_STONITS,
			WHTEFFICACY / header.exposure ) ) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS!\n" );
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

TT_RGBf
TT_RGBf_scalar_mult ( TT_RGBf original, double multiplier )
{
    TT_RGBf new_value;

    new_value.red = multiplier * original.red;
    new_value.green = multiplier * original.green;
    new_value.blue = multiplier * original.blue;

    return ( new_value );
}

void
original_units ( TT_RGBf_image *image, RadianceHeader *header )
{
    int	    n_rows, n_cols;
    int	    row, col;

    n_rows = TT_image_n_rows ( image );
    n_cols = TT_image_n_cols ( image );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    TT_image_data ( image, row, col ) =
		TT_RGBf_scalar_mult ( TT_image_data ( image, row, col ),
			1.0 / header->exposure );
	}
    }

    header->exposure = 1.0;
}

void
photometric_units ( TT_RGBf_image *image, RadianceHeader *header )
{
    int	    n_rows, n_cols;
    int	    row, col;

    n_rows = TT_image_n_rows ( image );
    n_cols = TT_image_n_cols ( image );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    TT_image_data ( image, row, col ) =
		TT_RGBf_scalar_mult ( TT_image_data ( image, row, col ),
			WHTEFFICACY / header->exposure );
	}
    }

    header->exposure = 1.0;
}
