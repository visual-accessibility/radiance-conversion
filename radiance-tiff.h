/*
 * Routines for reading and writing Radiance image files.
 *
 * Header information, including fov, returned on read.  Header information
 * on write not yet implemented.
 */

#ifndef __TT_RADIANCEIO_H
#define __TT_RADIANCEIO_H

#include "tifftools.h"
#include "tifftoolsimage.h"

typedef struct {
    char    *header_text;       /* Pointer to externally allocated string.  */
    				/* Does not include the  "#?RADIANCE" */
    				/* identifier or the FORMAT line */
    double  hFOV;
    double  vFOV;
    int	    exposure_set;
    double  exposure;
} RadianceHeader;

#ifdef __cplusplus
extern "C" {
#endif

TT_float_image	*TT_float_image_from_radfilename ( char *filename,
		    RadianceHeader *header );
TT_float_image	*TT_float_image_from_radfile ( FILE *radiance_fp,
		    RadianceHeader *header );
void		TT_float_image_to_radfilename ( char *filename,
		    TT_float_image *luminance, RadianceHeader header );
void		TT_float_image_to_radfile ( FILE *radiance_fp,
		    TT_float_image *luminance, RadianceHeader header );

TT_RGBf_image	*TT_RGBf_image_from_radfilename ( char *filename,
		    RadianceHeader *header );
TT_RGBf_image	*TT_RGBf_image_from_radfile ( FILE *radiance_fp,
		    RadianceHeader *header );
void		TT_RGBf_image_to_radfilename ( char *filename,
		    TT_RGBf_image *RGBf, RadianceHeader header );
void		TT_RGBf_image_to_radfile ( FILE *radiance_fp,
		    TT_RGBf_image *RGBf, RadianceHeader header );

TT_XYZ_image	*TT_XYZ_image_from_radfilename ( char *filename,
		    RadianceHeader *header );
TT_XYZ_image	*TT_XYZ_image_from_radfile ( FILE *radiance_fp,
		    RadianceHeader *header );
void		TT_XYZ_image_to_radfilename ( char *filename,
		    TT_XYZ_image *XYZ, RadianceHeader header );
void		TT_XYZ_image_to_radfile ( FILE *radiance_fp,
		    TT_XYZ_image *XYZ, RadianceHeader header );

TT_xyY_image	*TT_xyY_image_from_radfilename ( char *filename,
		    RadianceHeader *header );
TT_xyY_image	*TT_xyY_image_from_radfile ( FILE *radiance_fp,
		    RadianceHeader *header );
void		TT_xyY_image_to_radfilename ( char *filename,
		    TT_xyY_image *xyY, RadianceHeader header );
void		TT_xyY_image_to_radfile ( FILE *radiance_fp,
		    TT_xyY_image *xyY, RadianceHeader header );

#ifdef __cplusplus
}
#endif

#endif  /* __TT_RADIANCEIO_H */

