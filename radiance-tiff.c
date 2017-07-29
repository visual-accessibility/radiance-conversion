// #define TT_CHECK_BOUNDS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sRGB.h"
#include "radiance-tiff.h"
#include "radiance-header.h"
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "tifftoolsimage.h"
#include "radiance-conversion-version.h"
#include "deva-license.h"

void	set_header ( RadianceHeader *header, VIEW *view, int exposure_set,
            double exposure, char *description );
void	set_fov_in_view ( VIEW *view, RadianceHeader *header );

TT_float_image *
TT_float_image_from_radfilename ( char *filename, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory luminance image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    TT_float_image	*luminance;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    luminance = TT_float_image_from_radfile ( radiance_fp, header );
    fclose ( radiance_fp );

    return ( luminance );
}

TT_float_image *
TT_float_image_from_radfile ( FILE *radiance_fp, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory luminance image.
 */
{
    TT_float_image	*luminance;
    COLOR		*radiance_scanline;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_float_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    luminance = TT_float_image_new ( n_rows, n_cols );

    set_header ( header, &view, exposure_set, exposure, description );

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_float_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		TT_image_data ( luminance, row, col ) =
		    bright ( radiance_scanline[col] );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		TT_image_data ( luminance, row, col ) =
		    colval ( radiance_scanline[col], CIEY );
	    }
	} else {
	    fprintf ( stderr,
		    "TT_float_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( luminance );
}

void
TT_float_image_to_radfilename ( char *filename, TT_float_image *luminance,
       RadianceHeader header )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    TT_float_image_to_radfile ( radiance_fp, luminance, header );

    fclose ( radiance_fp );
}

void
TT_float_image_to_radfile ( FILE *radiance_fp, TT_float_image *luminance,
	RadianceHeader header )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;
    VIEW		view = STDVIEW;

    n_rows = TT_image_n_rows ( luminance );
    n_cols = TT_image_n_cols ( luminance );
    description = header.header_text;
    color_format = radcolor_rgbe;
    set_fov_in_view ( &view, &header );

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, header.exposure_set, header.exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_float_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],	/* output is grayscale */
		    TT_image_data ( luminance, row, col ),
		    TT_image_data ( luminance, row, col ),
		    TT_image_data ( luminance, row, col ) );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_float_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

TT_RGBf_image *
TT_RGBf_image_from_radfilename ( char *filename, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory RGBf image.  A pathname of "-" specifies standard input.
 */
{
    FILE	    *radiance_fp;
    TT_RGBf_image   *RGBf;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    RGBf = TT_RGBf_image_from_radfile ( radiance_fp, header );
    fclose ( radiance_fp );

    return ( RGBf );
}

TT_RGBf_image *
TT_RGBf_image_from_radfile ( FILE *radiance_fp, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory RGBf image.
 */
{
    TT_RGBf_image   	*RGBf;
    COLOR		*radiance_scanline;
    COLOR		RGBf_rad_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_RGBf_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    RGBf = TT_RGBf_image_new ( n_rows, n_cols );

    set_header ( header, &view, exposure_set, exposure, description );

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_RGBf_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( RGBf_rad_pixel, xyz2rgbmat,
			radiance_scanline[col] );
		TT_image_data ( RGBf, row, col ).red =
		    colval ( RGBf_rad_pixel, RED );
		TT_image_data ( RGBf, row, col ).green =
		    colval ( RGBf_rad_pixel, GRN );
		TT_image_data ( RGBf, row, col ).blue =
		    colval ( RGBf_rad_pixel, BLU );
	    }
	} else if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		TT_image_data ( RGBf, row, col ).red =
		    colval ( radiance_scanline[col], RED );
		TT_image_data ( RGBf, row, col ).green =
		    colval ( radiance_scanline[col], GRN );
		TT_image_data ( RGBf, row, col ).blue =
		    colval ( radiance_scanline[col], BLU );
	    }
	} else {
	    fprintf ( stderr,
		    "TT_RGBf_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( RGBf );
}

void
TT_RGBf_image_to_radfilename ( char *filename, TT_RGBf_image *RGBf,
	RadianceHeader header )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    TT_RGBf_image_to_radfile ( radiance_fp, RGBf, header );

    fclose ( radiance_fp );
}

void
TT_RGBf_image_to_radfile ( FILE *radiance_fp, TT_RGBf_image *RGBf,
	RadianceHeader header )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;
    VIEW		view = STDVIEW;

    n_rows = TT_image_n_rows ( RGBf );
    n_cols = TT_image_n_cols ( RGBf );
    description = header.header_text;
    color_format = radcolor_rgbe;
    set_fov_in_view ( &view, &header );

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, header.exposure_set, header.exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_RGBf_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],
		    TT_image_data ( RGBf, row, col ).red,
		    TT_image_data ( RGBf, row, col ).green,
		    TT_image_data ( RGBf, row, col ).blue );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_RGBf_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

TT_XYZ_image *
TT_XYZ_image_from_radfilename ( char *filename, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory XYZ image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    TT_XYZ_image	*XYZ;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    XYZ = TT_XYZ_image_from_radfile ( radiance_fp, header );
    fclose ( radiance_fp );

    return ( XYZ );
}

TT_XYZ_image *
TT_XYZ_image_from_radfile ( FILE *radiance_fp, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory XYZ image.
 */
{
    TT_XYZ_image	*XYZ;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_XYZ_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    XYZ = TT_XYZ_image_new ( n_rows, n_cols );

    set_header ( header, &view, exposure_set, exposure, description );

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		    "TT_XYZ_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );
		TT_image_data ( XYZ, row, col ).X =
		    colval ( XYZ_rad_pixel, CIEX );
		TT_image_data ( XYZ, row, col ).Y =
		    colval ( XYZ_rad_pixel, CIEY );
		TT_image_data ( XYZ, row, col ).Z =
		    colval ( XYZ_rad_pixel, CIEZ );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		TT_image_data ( XYZ, row, col ).X =
		    colval ( radiance_scanline[col], CIEX );
		TT_image_data ( XYZ, row, col ).Y =
		    colval ( radiance_scanline[col], CIEY );
		TT_image_data ( XYZ, row, col ).Z =
		    colval ( radiance_scanline[col], CIEZ );
	    }
	} else {
	    fprintf ( stderr,
		    "TT_XYZ_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( XYZ );
}

void
TT_XYZ_image_to_radfilename ( char *filename, TT_XYZ_image *XYZ,
	RadianceHeader header )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    TT_XYZ_image_to_radfile ( radiance_fp, XYZ, header );

    fclose ( radiance_fp );
}

void
TT_XYZ_image_to_radfile ( FILE *radiance_fp, TT_XYZ_image *XYZ,
	RadianceHeader header )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    VIEW		view = STDVIEW;

    n_rows = TT_image_n_rows ( XYZ );
    n_cols = TT_image_n_cols ( XYZ );
    description = header.header_text;
    color_format = radcolor_rgbe;
    set_fov_in_view ( &view, &header );

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, header.exposure_set, header.exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_XYZ_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    colval ( XYZ_rad_pixel, CIEX ) = TT_image_data ( XYZ, row, col ).X;
	    colval ( XYZ_rad_pixel, CIEY ) = TT_image_data ( XYZ, row, col ).Y;
	    colval ( XYZ_rad_pixel, CIEZ ) = TT_image_data ( XYZ, row, col ).Z;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_XYZ_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

TT_xyY_image *
TT_xyY_image_from_radfilename ( char *filename, RadianceHeader *header  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory xyY image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    TT_xyY_image	*xyY;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    xyY = TT_xyY_image_from_radfile ( radiance_fp, header );
    fclose ( radiance_fp );

    return ( xyY );
}

TT_xyY_image *
TT_xyY_image_from_radfile ( FILE *radiance_fp, RadianceHeader *header )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory xyY image.
 */
{
    TT_xyY_image	*xyY;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    TT_XYZ		XYZ_TT_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_xyY_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    xyY = TT_xyY_image_new ( n_rows, n_cols );

    set_header ( header, &view, exposure_set, exposure, description );

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_xyY_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );
		XYZ_TT_pixel.X = colval ( XYZ_rad_pixel, CIEX );
		XYZ_TT_pixel.Y = colval ( XYZ_rad_pixel, CIEY );
		XYZ_TT_pixel.Z = colval ( XYZ_rad_pixel, CIEZ );

		TT_image_data ( xyY, row, col ) = XYZ_to_xyY ( XYZ_TT_pixel );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		XYZ_TT_pixel.X =
		    colval ( radiance_scanline[col], CIEX );
		XYZ_TT_pixel.Y =
		    colval ( radiance_scanline[col], CIEY );
		XYZ_TT_pixel.Z =
		    colval ( radiance_scanline[col], CIEZ );

		TT_image_data ( xyY, row, col ) = XYZ_to_xyY ( XYZ_TT_pixel );
	    }
	} else {
	    fprintf ( stderr,
		    "TT_xyY_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( xyY );
}

void
TT_xyY_image_to_radfilename ( char *filename, TT_xyY_image *xyY,
	RadianceHeader header )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    TT_xyY_image_to_radfile ( radiance_fp, xyY, header );

    fclose ( radiance_fp );
}

void
TT_xyY_image_to_radfile ( FILE *radiance_fp, TT_xyY_image *xyY,
	RadianceHeader header )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    TT_XYZ		XYZ_TT_pixel;
    COLOR		XYZ_rad_pixel;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;
    VIEW		view = STDVIEW;

    n_rows = TT_image_n_rows ( xyY );
    n_cols = TT_image_n_cols ( xyY );
    description = header.header_text;
    color_format = radcolor_rgbe;
    set_fov_in_view ( &view, &header );

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, header.exposure_set, header.exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "TT_xyY_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    XYZ_TT_pixel = xyY_to_XYZ ( TT_image_data ( xyY, row, col ) );
	    colval ( XYZ_rad_pixel, CIEX ) = XYZ_TT_pixel.X;
	    colval ( XYZ_rad_pixel, CIEY ) = XYZ_TT_pixel.Y;
	    colval ( XYZ_rad_pixel, CIEZ ) = XYZ_TT_pixel.Z;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"TT_xyY_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

void
set_header ( RadianceHeader *header, VIEW *view, int exposure_set,
	double exposure, char *description )
{
    if ( header != NULL ) {
	header->vFOV = view->vert;
	header->hFOV = view->horiz;

	header->exposure_set = exposure_set;
	header->exposure = exposure;

	if ( description == NULL ) {
	    header->header_text = NULL;
	} else {
	    header->header_text = strdup ( description );
	}
    }
}

void
set_fov_in_view ( VIEW *view, RadianceHeader *header )
{
    view->type = VT_PER;
    view->horiz = header->hFOV;
    view->vert = header->vFOV;
}
