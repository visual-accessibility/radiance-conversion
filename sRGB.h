/*
 * Conversion between sRGB and XYZ, and more.
 *
 * Uses tifftools data types.
 *
 * Uses algorithmic definition of sRGB, not color management software
 */

#ifndef __DEVA_sRGB_CONVERT_H
#define __DEVA_sRGB_CONVERT_H

#include "tifftools.h"

#ifdef __cplusplus
extern "C" {
#endif

TT_float    gray_to_Y ( TT_gray gray );

TT_RGB	    Y_to_sRGB ( TT_float Y );
TT_gray	    Y_to_gray ( TT_float Y );

TT_RGBf	    sRGB_to_RGBf (TT_RGB sRGB );
TT_XYZ	    sRGB_to_XYZ ( TT_RGB sRGB );
TT_float    sRGB_to_Y ( TT_RGB sRGB );

TT_RGB	    RGBf_to_sRGB ( TT_RGBf RGBf );
TT_float    RGBf_to_Y ( TT_RGBf RGBf );
TT_XYZ	    RGBf_to_XYZ ( TT_RGBf RGBf );

TT_RGB	    XYZ_to_sRGB ( TT_XYZ XYZ );
TT_RGBf	    XYZ_to_RGBf ( TT_XYZ XYZ );

TT_xyY	    XYZ_to_xyY ( TT_XYZ XYZ );
TT_XYZ	    xyY_to_XYZ ( TT_xyY xyY );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_sRGB_CONVERT_H */

