/*
 * Support for using FocalLengthIn35mmFilm EXIF tag to encode field-of-view
 * in image files.  Includes basic value conversions, plus libtiff support
 * for setting and getting the relevant tag value in TIFF files.  JPEG
 * support for accessing the FocalLengthIn35mmFilm EXIF tag is included
 * elsewhere, since it requires close integration with the rest of the
 * JPEG read/write process.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "FOV.h"

#define	NO_TIFF_35MM_EQUIV	-1.0

#define	DIAGONAL_35MM		43.266615	/* mm */

#define	V_H_DIFF_TOLERANCE	0.01	/* allowable mismatch between */
					/* vFOV/v_dim and hFOV/h_dim */

#define	SQR(x)	( (x) * (x) )

DEVA_FOV
FocalLength_35mm_2_FOV_diag ( double FocalLength_35mm, int v_dim, int h_dim )
/*
 * Convert 35mm equivalent focal length to diagonal-axis field of view.
 *
 * FocalLength_35mm:	35mm equivalent focal length in mm.  Note that
 * 			this value is stored in the EXIF tag as a short int.
 *
 * v_dim, h_dim:	Vertical and horizontal dimensions of image in pixels.
 *
 * Returns:		Corresponding vertical and horizontal FOVs in degrees.
 */
{
    double	diagonal_px;		/* of image (in pixels) */
    double	diagonal_fov_rad;	/* radians */
    double	focal_length_px;	/* of image (in pixels) */
    DEVA_FOV	fov_degrees;

    diagonal_fov_rad = 2.0 * atan2 ( 0.5 * DIAGONAL_35MM, FocalLength_35mm );

    diagonal_px = sqrt ( SQR ( (double) v_dim ) + SQR ( (double) h_dim ) );

    focal_length_px = ( 0.5 * diagonal_px ) / tan ( 0.5 * diagonal_fov_rad );

    fov_degrees.v_fov = radian2degree ( 2.0 * atan2 ( 0.5 * ((double) v_dim ),
		focal_length_px ) );
    fov_degrees.h_fov = radian2degree ( 2.0 * atan2 ( 0.5 * ((double) h_dim),
		focal_length_px ) );

    return ( fov_degrees );
}

double
FOV_diag_2_FocalLength_35mm ( DEVA_FOV fov, int v_dim, int h_dim )
/*
 * Convert vertical and horizontal fields-of-view to 35mm equivalent focal
 * length (based on diagonal).
 *
 * fov:			Vertical and horizontal FOVs in degrees
 *
 * v_dim, h_dim:	Vertical and horizontal dimensions of image in pixels.
 *
 * Returns:		35mm equivalent focal length in mm.  Note that
 *                      this value is stored in the EXIF tag as a short int.
 */
{
    double  focal_length_px_v;	/* based on vertical dimensions */
    double  focal_length_px_h;	/* based on horizontal dimensions */
    double  focal_length_px;	/* average */
    double  diagonal_px;
    double  diagonal_fov_rad;
    double  focal_length_35mm;	/* 35mm equivalent focal length */

    focal_length_px_v = ( 0.5 * ((double) v_dim ) ) /
	tan ( 0.5 * degree2radian ( fov.v_fov ) );
    focal_length_px_h = ( 0.5 * ((double) h_dim ) ) /
	tan ( 0.5 * degree2radian ( fov.h_fov ) );

    focal_length_px = 0.5 * ( focal_length_px_v + focal_length_px_h );

    if ( ( fabs ( focal_length_px_v - focal_length_px_h ) / focal_length_px ) >
	    		V_H_DIFF_TOLERANCE ) {
	fprintf ( stderr,
"FOV_diag_2_FocalLength_35mm: Mismatch between vFOV/v_dim and hFOV/h_dim\n" );
    }

    diagonal_px = sqrt ( SQR ( (double) v_dim ) + SQR ( (double) h_dim ) );

    diagonal_fov_rad = 2.0 * atan2 ( 0.5 * diagonal_px, focal_length_px );

    focal_length_35mm = ( 0.5 * DIAGONAL_35MM ) /
	tan ( 0.5 * diagonal_fov_rad );

    return ( focal_length_35mm );
}

double
FocalLength_35mm_2_FOV ( double FocalLength_35mm )
/*
 * Convert 35mm equivalent focal length to long-axis field of view.
 *
 * FocalLength_35mm:	35mm equivalent focal length in mm.  Note that
 * 			this value is stored in the EXIF tag as a short int.
 *
 * Returns:		Corresponding field-of-view in degrees.
 */
{
    double  fov;	/* in degrees */

    fov = radian2degree ( 2.0 * atan2 ( 18.0, FocalLength_35mm ) );
    		/* image plane for the long axis of standard 35mm film
		 * is 36mm wide */

    return ( fov );
}

double
FOV_2_FocalLength_35mm ( double fov )
/*
 * Convert long-axis field of view to 35mm equivalent focal length.
 *
 * fov:			Field-of-view in degrees.
 *
 * Returns:		35mm equivalent focal length in mm.  Note that
 *                      this value is stored in the EXIF tag as a short int.
 */
{
    double  FocalLength_35mm;	/* in mm */

    FocalLength_35mm = 18.0 / tan ( degree2radian ( 0.5 * fov ) );

    return ( FocalLength_35mm );
}

double
degree2radian ( double degrees )
{
    double  radians;

    radians = degrees * ( ( 2.0 * M_PI ) / 360.0 );

    return ( radians );
}

double
radian2degree ( double radians )
{
    double  degrees;

    degrees = radians * ( 360.0 / ( 2.0 * M_PI ) );

    return ( degrees );
}

void
set_tiff_fov_diag ( TIFF *file, DEVA_FOV fov, int v_dim, int h_dim )
{
    double  focal_length_35mm;

    focal_length_35mm = FOV_diag_2_FocalLength_35mm ( fov, v_dim, h_dim );

    set_tiff_35mm_equiv ( file, focal_length_35mm );
}

DEVA_FOV
get_tiff_fov_diag ( TIFF *file, int v_dim, int h_dim )
{
    DEVA_FOV	fov;
    double	FocalLength_35mm;

    FocalLength_35mm = get_tiff_35mm_equiv ( file );
    if ( FocalLength_35mm < 0.0 ) {
	fov.v_fov = fov.h_fov = NO_TIFF_35MM_EQUIV;
    } else {
	fov = FocalLength_35mm_2_FOV_diag ( FocalLength_35mm, v_dim, h_dim );
    }

    return ( fov );
}

void
set_tiff_fov ( TIFF *file, double fov )
/*
 * Creates an EXIF directory for an open TIFF file and add a TIFFTAG_EXIFIFD
 * tag with the value corresponding to the specified field-of-view.
 * This routine is specialized for the DEVA software bundle.  In particular,
 * the TIFF file cannot already have an EXIF directory when this routine
 * is called.
 *
 * Probably needs to be called after contents of file have been written.
 *
 * fov:			Field-of-view in degrees.
 */
{
    short	    focal_length_35mm;	/* in mm */

    if ( fov <= 0.0 ) {
	fprintf ( stderr, "set_exif_35mm_equivalent: invalid fov (%g)!\n",
		fov );
	exit ( EXIT_FAILURE );
    }

    /* image plane for the long axis of standard 35mm film is 36mm wide */
    focal_length_35mm = round ( FOV_2_FocalLength_35mm ( fov ) );

    set_tiff_35mm_equiv ( file, focal_length_35mm );
}

double
short_side_fov ( int long_side_dim, int short_side_dim, double long_side_fov )
/*
 * Computes fov for smaller dimension given fov for larger dimension.
 * All angles in degrees.
 */
{
    double  focal_length;
    double  short_side_fov;

    focal_length = ( 0.5 * ((double) long_side_dim ) ) /
	    		tan ( degree2radian ( 0.5 * long_side_fov ) );

    short_side_fov = 2.0 *
	radian2degree ( atan2 ( 0.5 * ((double) short_side_dim ),
	    						focal_length ) );

    return ( short_side_fov );
}

double
get_tiff_fov ( TIFF *file )
/*
 * Reads EXIF directory for an open TIFF file, gets TIFFTAG_EXIFIFD tag,
 * and converts value to the corresponding field-of-view.
 *
 * ***** Warning *****
 * TIFFReadEXIFDirectory seems to leave libtiff in a strange state
 * (see comments in OpenImageIO tiff.imageio).  It is possible that
 * this problem is fixed by subsequently calling TIFFSetDirectory with
 * a dirnum of 0.  Safest, howerver, is to have the routine be the last
 * access to the open file.
 */
{
    double  fov;
    double  focal_length_35mm;

    focal_length_35mm = get_tiff_35mm_equiv ( file );

    fov = FocalLength_35mm_2_FOV ( focal_length_35mm );

    return ( fov );
}

void
set_tiff_35mm_equiv ( TIFF *file, double focal_length_35mm )
/*
 * Creates an EXIF directory for an open TIFF file and add a TIFFTAG_EXIFIFD
 * tag with the value corresponding to the specified effective focal length.
 * This routine is specialized for the DEVA software bundle.  In particular,
 * the TIFF file cannot already have an EXIF directory when this routine
 * is called.
 *
 * Probably needs to be called after contents of file have been written.
 *
 * fov:			Field-of-view in degrees.
 */
{
    short	    focal_length_35mm_s;	/* in mm */
    uint64	    exif_dir_offset = 0;

    if ( focal_length_35mm <= 0.0 ) {
	fprintf ( stderr, "set_tiff_35mm_equiv: invalid focal length (%g)!\n",
		focal_length_35mm );
	exit ( EXIT_FAILURE );
    }

    focal_length_35mm_s = round ( focal_length_35mm );

    /* Need to write main directory before we can create EXIF directory */
    if ( !TIFFWriteDirectory ( file ) ) {
	fprintf (stderr, "TIFFWriteDirectory() failed!\n" );
	exit ( EXIT_FAILURE );
    }

    /* Create EXIF directory */
    if ( TIFFCreateEXIFDirectory ( file ) != 0) {
	fprintf (stderr, "TIFFCreateEXIFDirectory() failed!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( !TIFFSetField ( file, EXIFTAG_FOCALLENGTHIN35MMFILM,
		focal_length_35mm_s ) ) {
	fprintf (stderr, "Can't set EXIFTAG_FOCALLENGTHIN35MMFILM!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( !TIFFWriteCustomDirectory ( file, &exif_dir_offset ) ) {
	fprintf (stderr, "TIFFWriteCustomDirectory() with EXIF failed.\n");
	exit ( EXIT_FAILURE );
    }

    /* Go back to the first directory, and add the EXIFIFD pointer */
    TIFFSetDirectory ( file, 0 );
    TIFFSetField ( file, TIFFTAG_EXIFIFD, exif_dir_offset );
}

double
get_tiff_35mm_equiv ( TIFF *file )
/*
 * Reads EXIF directory for an open TIFF file and gets the 35mm_equivalent
 * TIFFTAG_EXIFIFD tag,
 *
 * ***** Warning *****
 * TIFFReadEXIFDirectory seems to leave libtiff in a strange state
 * (see comments in OpenImageIO tiff.imageio).  It is possible that
 * this problem is fixed by subsequently calling TIFFSetDirectory with
 * a dirnum of 0.  Safest, howerver, is to have the routine be the last
 * access to the open file.
 */
{
    short   focal_length_35mm_s;
    uint64  exif_dir_offset = 0;

    if ( !TIFFGetField ( file, TIFFTAG_EXIFIFD, &exif_dir_offset ) ) {
	/* EXIF directory doesn't exit */
	return ( NO_TIFF_35MM_EQUIV );
    }
    if ( exif_dir_offset == 0 ) {
	fprintf (stderr,
		"get_tiff_35mm_equiv: Did not get expected EXIFIFD!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( !TIFFReadEXIFDirectory ( file, exif_dir_offset) ) {
	fprintf (stderr,
		"get_tiff_35mm_equiv: TIFFReadEXIFDirectory() failed!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( !TIFFGetField ( file, EXIFTAG_FOCALLENGTHIN35MMFILM,
		&focal_length_35mm_s ) ) {
	/* EXIFTAG_FOCALLENGTHIN35MMFILM tag doesn't exit */
	TIFFSetDirectory ( file, 0 );	/* reset back to "normal" directory */
	return ( NO_TIFF_35MM_EQUIV );
    }

    TIFFSetDirectory ( file, 0 );	/* reset back to "normal" directory */

    return ( (double) focal_length_35mm_s );
}
