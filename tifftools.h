/****************************************************************************
 * MIT License
 *
 * Copyright (c) 2016 University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ****************************************************************************/

/*
 * Portions of this code are based on the code from OpenEXR project with
 * the following copyright:
 *
 * Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
 * Digital Ltd. LLC
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *       Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * *       Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * *       Neither the name of Industrial Light & Magic nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/*
 * Support functions for reading and writing TIFF images in selected
 * formats.  All grayscale and RGB image formats are currently supported
 * (8 bit integer; 16 bit integer; 16, 24, and 32 bit floating point).
 * Not currently supported is indexed color, RGBA, CMYK, and Lab Color.
 * Support is also provided for LogL and LogLuv encoded HDR images, along
 * with several single channel numeric formats not normally used for
 * conventional image data.
 *
 * tiff/jpeg compression is supported, but only for grayscale and YCbCr
 * color encoding.
 */

#ifndef	__TIFFTOOLS_H
#define	__TIFFTOOLS_H

#define	VERSION		1.3.1
#define	VERSIONSTRING	"1.3.1"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <tiffio.h>

#ifndef TRUE
#define	TRUE		1
#endif	/* TRUE */
#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

#define	TT_DEFAULT_ORIENATION		ORIENTATION_TOPLEFT
#define	TT_JPEG_DEFAULT_JPEGQUALITY	75

/* "reasonable" resolution values, since the default is 1200 dpi */
#define	TT_RESOLUTIONUNIT		2	/* inches */
#define	TT_DEFAULT_RESOLUTION		72.0

#if ( HOST_BIGENDIAN == 1 )	/* from tiffconf.h by way of tiff.h */
#define TT_FLOAT_CVT_BIG_ENDIAN
#else
#define TT_FLOAT_CVT_LITTLE_ENDIAN
#endif

#define	TIFF_READ_ERROR_RETURN		-1
#define	TIFF_READ_NORMAL_RETURN		1
#define	TIFF_WRITE_ERROR_RETURN		-1
#define	TIFF_WRITE_NORMAL_RETURN	1

#define	TT_fatal_error( message ) \
    TT_fatal_error_loc ( __FILE__, __LINE__, message )

typedef uint8	TT_gray;
typedef uint16	TT_gray16;

typedef int16	TT_int16;	/* not a standard image type */
typedef uint32	TT_uint32;	/* not a standard image type */
typedef int32	TT_int32;	/* not a standard image type */
typedef	float	TT_float;
typedef	float	TT_Y;		/* not a standard image type */
typedef	double	TT_double;	/* not a standard image type */

typedef	struct {
    uint8    red;
    uint8    green;
    uint8    blue;
} TT_RGB;

typedef	struct {
    uint8    red;
    uint8    green;
    uint8    blue;
    uint8    alpha;
} TT_RGBA;

typedef	struct {
    uint16	red;
    uint16	green;
    uint16	blue;
} TT_RGB16;

typedef	struct {
    int32	red;
    int32	green;
    int32	blue;
} TT_RGBint32;			/* not a standard image type */

typedef	struct {
    float    red;
    float    green;
    float    blue;
} TT_RGBf;

typedef	struct {		/* not yet supported by I/O routines */
    float    red;
    float    green;
    float    blue;
    float    alpha;
} TT_RGBAf;

typedef	struct {
    float    X;
    float    Y;
    float    Z;
} TT_XYZ;			/* Not supported as a TIFF image format, */
				/* except with LogLuv encoding. */

typedef	struct {
    float    x;
    float    y;
    float    Y;
} TT_xyY;			/* Not supported as a TIFF image format, */
				/* but valid imagearray type */
typedef	enum {
    TTTypeUnknown,
    TTTypeGray,
    TTTypeRGB,
    TTTypeColormap,
    TTTypeRGBA,
    TTTypeYCbCr,	/* file format only */
    TTTypeRGB16,	/* Used by Photoshop and other software */
    TTTypeRGB_int,	/* signed int! */
    TTTypeRGBf,
    TTTypeRGBf16,	/* file format only */
    TTTypeRGBf24,	/* file format only */
    TTTypeXYZ,		/* program format only (except for Photoshop hack) */
    TTTypeLogLuv,	/* file format only */
    TTTypeLogL,		/* file format only */
    TTTypeGray16,
    TTTypeShort,
    TTTypeUInt,
    TTTypeInt,
    TTTypeFloat,
    TTTypeFloat16,	/* file format only */
    TTTypeFloat24,	/* file format only */
    TTTypeDouble,
    /***********************/
    TTNumTypes
} TTType;

typedef struct  {
    uint16	bits_per_sample;
    uint16	samples_per_pixel;
    uint16	photometric;
    uint16	sample_format;
    TTType	type;
} TT_TIFFParms;

typedef	struct	{
    TTType	type;
    char	*name;
} TTType2Name;

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

TIFF	*TT_gray_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_gray16_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_RGB_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_RGBA_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_RGB16_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_float_open_read ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_RGB_open_readf ( char *filename, unsigned int *height,
		unsigned int *width );
TIFF	*TT_XYZ_open_read ( char *filename, unsigned int *height,
		unsigned int *width );

void	TT_gray_read ( TIFF *tif, TT_gray *buf, uint32 row );
void	TT_gray16_read ( TIFF *tif, TT_gray16 *buf, uint32 row );
void	TT_RGB_read ( TIFF *tif, TT_RGB *buf, uint32 row );
void	TT_RGB16_read ( TIFF *tif, TT_RGB16 *buf, uint32 row );
void	TT_float_read ( TIFF *tif, TT_float *buf, uint32 row );
void	TT_RGBf_read ( TIFF *tif, TT_RGBf *buf, uint32 row );
void	TT_XYZ_read ( TIFF *tif, TT_XYZ *buf, uint32 row );
void	TT_LogLuv_read ( TIFF *tif, TT_XYZ *buf, uint32 row );
void	TT_LogL_read ( TIFF *tif, TT_float *buf, uint32 row );

TIFF	*TT_gray_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_gray16_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGB_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGBA_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGB16_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_float_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_float16_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_float24_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGBf_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGBf16_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_RGBf24_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_XYZ_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_LogLuv_open_write ( char *filename, unsigned int height,
		unsigned int width );
TIFF	*TT_logL_open_write ( char *filename, unsigned int height,
		unsigned int width );

void	TT_gray_write ( TIFF *tif, TT_gray *buf, uint32 row );
void	TT_gray16_write ( TIFF *tif, TT_gray16 *buf, uint32 row );
void	TT_RGB_write ( TIFF *tif, TT_RGB *buf, uint32 row );
void	TT_RGBA_write ( TIFF *tif, TT_RGBA *buf, uint32 row );
void	TT_RGB16_write ( TIFF *tif, TT_RGB16 *buf, uint32 row );
void	TT_RGBf_write ( TIFF *tif, TT_RGBf *buf, uint32 row );
void	TT_float_write ( TIFF *tif, float *buf, uint32 row );
void	TT_XYZ_write ( TIFF *tif, TT_XYZ *buf, uint32 row );
void	TT_LogLuv_write ( TIFF *tif, TT_XYZ *buf, uint32 row );
void	TT_LogL_write ( TIFF *tif, TT_float *buf, uint32 row );

TTType	TT_file_type ( TIFF *file );
char	*TT_type2name ( TTType type );
void	TT_image_size ( TIFF *file, unsigned int *pheight,
		unsigned int *pwidth );
double	TT_photometric_normalization ( TIFF *file );
TIFF	*TT_open_write ( char *filename, TTType type, unsigned int height,
		unsigned int width );
TIFF	*TT_open_read ( char *filename, TTType *type, unsigned int *height,
	        unsigned int *width );

TT_gray	    *TT_gray_getrowbuf ( TIFF *tif );
TT_gray16   *TT_gray16_getrowbuf ( TIFF *tif );
TT_int16    *TT_int16_getrowbuf ( TIFF *tif );
TT_uint32   *TT_uint32_getrowbuf ( TIFF *tif );
TT_int32    *TT_int32_getrowbuf ( TIFF *tif );
TT_float    *TT_float_getrowbuf ( TIFF *tif );
TT_Y	    *TT_Y_getrowbuf ( TIFF *tif );
TT_double   *TT_double_getrowbuf ( TIFF *tif );
TT_RGB	    *TT_RGB_getrowbuf ( TIFF *tif );
TT_RGB16    *TT_RGB16_getrowbuf ( TIFF *tif );
TT_RGBA	    *TT_RGBA_getrowbuf ( TIFF *tif );
TT_RGBf	    *TT_RGBf_getrowbuf ( TIFF *tif );
TT_XYZ	    *TT_XYZ_getrowbuf ( TIFF *tif );

void	    TT_set_compression_jpeg ( TIFF *file, int jpeg_quality );
void	    TT_set_compression_LZW ( TIFF *file, int prediction );
void	    TT_set_compression_zip ( TIFF *file, int prediction );

int	    TT_is_alpha_unspecified ( TIFF *file );
int	    TT_is_alpha_associated ( TIFF *file );
int	    TT_is_alpha_unassociated ( TIFF *file );
void	    TT_set_alpha_unspecified ( TIFF *file );
void	    TT_set_alpha_associated ( TIFF *file );
void	    TT_set_alpha_unassociated ( TIFF *file );

char	    *TT_get_description ( TIFF *tif );
void	    TT_set_description ( TIFF *tif, char *description );
void	    TT_cat_description ( TIFF *tif, char *description );
void	    TT_set_description_arguments ( TIFF *tif, int argc, char *argv[] );

#ifdef __cplusplus
}
#endif

#endif	/* __TIFFTOOLS_H */
