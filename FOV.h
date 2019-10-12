/*
 * Support for using FocalLengthIn35mmFilm EXIF tag to encode field-of-view
 * in image files.  Includes basic value conversions, plus libtiff support
 * for setting and getting the relevant tag value in TIFF files.  JPEG
 * support for accessing the FocalLengthIn35mmFilm EXIF tag is included
 * elsewhere, since it requires close integration with the rest of the
 * JPEG read/write process.
 *
 * 35 mm equivalent focal length is defined in two different ways:
 * <https://en.wikipedia.org/wiki/35_mm_equivalent_focal_length>,
 * <http://www.panoramafactory.com/equiv35/equiv35.html>.  One is based on
 * the diagonal field of view, the other is based on the horizontal field
 * of view.  These routines provide both versions, except that the larger
 * of hFOV and vFOV are used instead of the hFOV.
 */

#ifndef __DeVAS_FOV_H
#define __DeVAS_FOV_H

#include <tiffio.h>

#define	NO_TIFF_35MM_EQUIV	-1.0

typedef struct {
    double  v_fov;	/* degrees */
    double  h_fov;	/* degrees */
} DeVAS_FOV;

#ifdef __cplusplus
extern "C" {
#endif

/* FOV-FL conversions based on diagonal (needs aspect ratio information) */
DeVAS_FOV    FocalLength_35mm_2_FOV_diag ( double FocalLength_35mm, int v_dim,
		int h_dim );
double	    FOV_diag_2_FocalLength_35mm ( DeVAS_FOV fov, int v_dim, int h_dim );

/* FOV-FL conversions based on horizontal (longest) image side */
double	FocalLength_35mm_2_FOV ( double FocalLength_35mm );
double	FOV_2_FocalLength_35mm ( double fov );

/* Data convertion routines */
double	degree2radian ( double degrees );
double	radian2degree ( double radians );

/* TIFF-specific tag manipulation */

/*   Based on fov of diagonal image side: */
void	set_tiff_fov_diag ( TIFF *file, DeVAS_FOV fov, int v_dim, int h_dim );
DeVAS_FOV  get_tiff_fov_diag ( TIFF *file, int v_dim, int h_dim );

/*   Based on fov of horizontal (longest) image side: */
void	set_tiff_fov ( TIFF *file, double fov );
double	get_tiff_fov ( TIFF *file );


/* routines to actually set/get the tag */
void	set_tiff_35mm_equiv ( TIFF *file, double focal_length_35mm );
double	get_tiff_35mm_equiv ( TIFF *file );

/* h vs. v fov */
double  short_side_fov ( int long_side_dim, int short_side_dim,
	    double long_side_fov );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_FOV_H */
