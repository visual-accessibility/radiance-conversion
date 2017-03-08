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
 * Support functions for reading and writing TIFF images in selected
 * formats.  All grayscale and RGB image formats are currently supported
 * (8 bit integer; 16 bit integer; 16, 24, and 32 bit floating point).
 * Not currently supported is indexed color, CMYK, and Lab Color.
 * Support is also provided for LogL and LogLuv encoded HDR images, along
 * with several single channel numeric formats not normally used for
 * conventional image data.
 *
 * tiff/jpeg compression is supported, but only for grayscale and YCbCr
 * color encoding.
 */

#include <stdlib.h>
#include <stdio.h>
#include <tiffio.h>
#include <libgen.h>	/* use POSIX version of basename */
#include <string.h>	/* for strdup */
#include "tifftools.h"

static TT_TIFFParms TIFF_file_parms[] = {
    {  8, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_UINT,	    TTTypeGray },
    {  8, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_UINT,	    TTTypeRGB },
    {  8, 1, PHOTOMETRIC_PALETTE,    SAMPLEFORMAT_UINT,	    TTTypeColormap },
    {  8, 4, PHOTOMETRIC_RGB,        SAMPLEFORMAT_UINT,	    TTTypeRGBA },
    {  8, 3, PHOTOMETRIC_YCBCR,	     SAMPLEFORMAT_UINT,	    TTTypeYCbCr },
    { 16, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_UINT,	    TTTypeRGB16 },
    { 32, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_UINT,	    TTTypeRGB_int },
    { 32, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_IEEEFP,   TTTypeRGBf },
    { 16, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_IEEEFP,   TTTypeRGBf16 },
    { 24, 3, PHOTOMETRIC_RGB,        SAMPLEFORMAT_IEEEFP,   TTTypeRGBf24 },
    { 32, 3, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_IEEEFP,   TTTypeXYZ },
    { 16, 3, PHOTOMETRIC_LOGLUV,     SAMPLEFORMAT_INT,	    TTTypeLogLuv },
    { 16, 1, PHOTOMETRIC_LOGL,       SAMPLEFORMAT_INT,	    TTTypeLogL },
    { 16, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_UINT,	    TTTypeGray16 },
    { 16, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_INT,	    TTTypeShort },
    { 32, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_UINT,	    TTTypeUInt },
    { 32, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_INT,	    TTTypeInt },
    { 32, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_IEEEFP,   TTTypeFloat },
    { 16, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_IEEEFP,   TTTypeFloat16 },
    { 24, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_IEEEFP,   TTTypeFloat24 },
    { 64, 1, PHOTOMETRIC_MINISBLACK, SAMPLEFORMAT_IEEEFP,   TTTypeDouble },
    /*
     * Setting TIFFTAG_SGILOGDATAFMT to SGILOGDATAFMT_FLOAT makes
     * TIFFTAG_BITSPERSAMPLE 32 in the directory as seen by the program,
     * but 16 in the actual file directory, and changes TIFFTAG_SAMPLEFORMAT
     * from SAMPLEFORMAT_INT to SAMPLEFORMAT_IEEEFP. As a result, we need to
     * have LogLuv and LogL entries that will work both before and after
     * the setting of TIFFTAG_SGILOGDATAFMT!
     */
    { 32, 3, PHOTOMETRIC_LOGLUV,     SAMPLEFORMAT_IEEEFP,   TTTypeLogLuv },
    { 32, 1, PHOTOMETRIC_LOGL,       SAMPLEFORMAT_IEEEFP,   TTTypeLogL },
    {  0, 0, 0,                      0,			    TTTypeUnknown }
};

static TTType2Name TIFF_type2name[] = {
    { TTTypeGray,	"8 bit grayscale" },
    { TTTypeRGB,	"8 bit RGB" },
    { TTTypeColormap,	"8 bit colormapped" },
    { TTTypeRGBA,	"8 bit RGBA" },
    { TTTypeYCbCr,	"8 bit YCbCr (JPEG)" },
    { TTTypeRGB16,	"16 bit RGB" },
    { TTTypeRGB_int,	"32 bit integer RGB (non-standard image format)" },
    { TTTypeRGBf,	"32 bit float RGB" },
    { TTTypeRGBf16,	"16 bit float RGB" },
    { TTTypeRGBf24,	"24 bit float RGB" },
    { TTTypeXYZ,	"32 bit float XYZ (non-standard image format)" },
    { TTTypeLogLuv,	"LogLuv XYZ" },
    { TTTypeLogL,	"LogL Y" },
    { TTTypeGray16,	"16 bit gray scale" },
    { TTTypeShort,	"16 bit signed integer (non-standard image format)" },
    { TTTypeUInt,	"32 bit unsigned integer (non-standard image format)" },
    { TTTypeInt,	"32 bit signed integer (non-standard image format)" },
    { TTTypeFloat,	"32 bit float grayscale" },
    { TTTypeFloat16,	"16 bit float grayscale" },
    { TTTypeFloat24,	"24 bit float grayscale" },
    { TTTypeDouble,	"64 bit float (non-standard image format)" },
    { TTTypeUnknown,	"unknown type" }	/* must be last! */
};

/* prototypes for non-user-callable utility functions */
static void		get_tiff_tag_error ( char *tagname );
static int		end_of_TIFF_file_parms ( TT_TIFFParms parms );
static int		match_tiff_parms ( TT_TIFFParms p1, TT_TIFFParms p2 );
static TT_TIFFParms	TT_type2parms ( TTType type );
static void		TT_struct_size_check ( void );

static void		convert_float_32_24 ( float *, void *,
				unsigned int count );
static void		convert_float_32_16 ( float *, void *,
				unsigned int count );
static void		convert_float_24_32 ( void *, float *,
				unsigned int count );
static void		convert_float_16_32 ( void *, float *,
				unsigned int count );

static void		convert_RGB_float_32_24 ( TT_RGBf *, void *,
				unsigned int count );
static void		convert_RGB_float_32_16 ( TT_RGBf *, void *,
				unsigned int count );
static void		convert_RGB_float_24_32 ( void *, TT_RGBf *,
				unsigned int count );
static void		convert_RGB_float_16_32 ( void *, TT_RGBf *,
				unsigned int count );

static unsigned int	half_to_float ( unsigned short half );
static unsigned short	float_to_half ( unsigned int iFloat );
static unsigned int	triple_to_float ( unsigned int iTriple );
static unsigned int	float_to_triple ( unsigned int iFloat );

static void		TT_fatal_error_loc ( char *filename, int linenumber,
				const char *message );

TTType
TT_file_type ( TIFF *file )
/*
 * Returns pixel type of open TIFF file.
 */
{
    TT_TIFFParms    tiff_parms;
    uint16          planarconfig;
    uint32          height, width;
    int		    i;

    if ( !TIFFGetFieldDefaulted ( file, TIFFTAG_PLANARCONFIG,
		                &planarconfig ) ) {
	get_tiff_tag_error ( "TIFFTAG_PLANARCONFIG" );
    }
    if ( planarconfig != PLANARCONFIG_CONTIG ) {
	fprintf ( stderr,
		"can't handle files with separate planes of data!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( !TIFFGetField ( file, TIFFTAG_IMAGELENGTH, &height ) ) {
	get_tiff_tag_error ( "TIFFTAG_IMAGELENGTH" );
    }
    if ( !TIFFGetField ( file, TIFFTAG_IMAGEWIDTH, &width ) ) {
	get_tiff_tag_error ( "TIFFTAG_IMAGEWIDTH" );
    }

    if ( !TIFFGetFieldDefaulted ( file, TIFFTAG_SAMPLESPERPIXEL,
		&tiff_parms.samples_per_pixel ) ) {
	get_tiff_tag_error ( "TIFFTAG_SAMPLESPERPIXEL" );
    }

    if ( !TIFFGetFieldDefaulted ( file, TIFFTAG_BITSPERSAMPLE,
		                &tiff_parms.bits_per_sample ) ) {
	get_tiff_tag_error ( "TIFFTAG_BITSPERSAMPLE" );
    }

    if ( !TIFFGetFieldDefaulted ( file, TIFFTAG_PHOTOMETRIC,
		&tiff_parms.photometric ) ) {
	get_tiff_tag_error ( "TIFFTAG_PHOTOMETRIC" );
    }

    if ( !TIFFGetFieldDefaulted ( file, TIFFTAG_SAMPLEFORMAT,
		&tiff_parms.sample_format ) ) {
	get_tiff_tag_error ( "TIFFTAG_SAMPLEFORMAT" );
    }

    i = 0;
    while ( !end_of_TIFF_file_parms ( TIFF_file_parms[i] ) ) {
	if ( match_tiff_parms ( tiff_parms, TIFF_file_parms[i] ) ) {
	    if ( TIFF_file_parms[i].type == TTTypeYCbCr ) {
		return ( TTTypeRGB ); /* jpg: the program will see it as RGB */
				      /* don't do this for LogLuv or LogL, */
				      /* since program may need to know file */
				      /* type */
	    } else {
		return ( TIFF_file_parms[i].type );
	    }
	}
	i++;
    }

    return ( TTTypeUnknown );
}

char
*TT_type2name ( TTType type )
/*
 * Returns character string describing TIFF type.
 */
{
    int	    i;

    i = 0;
    while ( TIFF_type2name[i].type != TTTypeUnknown ) {
	if ( TIFF_type2name[i].type == type ) {
	    return ( TIFF_type2name[i].name );
	}
	i++;
    }

    return ( TIFF_type2name[i].name );	/* unknown! */
}

void
TT_image_size ( TIFF *file, unsigned int *pheight, unsigned int *pwidth )
/*
 * Returns height and width of an open TIFF image.
 *
 * Note returned values are UNSIGNED integers.  Explicity conversion of
 * values between uini32 and unsigned int is done to be absolutely sure
 * that there are no incompatibilities.
 */
{
    uint32          height, width;

    if ( !TIFFGetField ( file, TIFFTAG_IMAGELENGTH, &height ) ) {
	get_tiff_tag_error ( "TIFFTAG_IMAGELENGTH" );
    }
    if ( !TIFFGetField ( file, TIFFTAG_IMAGEWIDTH, &width ) ) {
	get_tiff_tag_error ( "TIFFTAG_IMAGEWIDTH" );
    }

    *pheight = height;
    *pwidth = width;
}

double
TT_photometric_normalization ( TIFF *file )
/*
 * Returns photometric normalization.  Technically, this is only valid
 * for LogLuv and LogL images.  However, libTIFF supports the tag for
 * any image format.
 */
{
    double	photometric_normalization;

    if ( TIFFGetField ( file, TIFFTAG_STONITS, &photometric_normalization ) ) {
	return ( photometric_normalization );
    } else {
	return ( 0.0 );
    }
}

TIFF *
TT_open_read ( char *filename, TTType *ptype, unsigned int *height,
	unsigned int *width )
{
    TIFF	*input;
    TTType	type;
    uint16	compression;

    TT_struct_size_check ( );	/* runtime check of struct size compatibility */

    input = TIFFOpen ( filename, "r" );
    if ( input == NULL ) {
	/* TIFFOpen should output error message */
	exit ( EXIT_FAILURE );
    }

    type = TT_file_type ( input );

    switch ( type ) {

	case TTTypeYCbCr:
	    if ( !TIFFGetField ( input, TIFFTAG_COMPRESSION, &compression ) ) {
		get_tiff_tag_error ( "TIFFTAG_COMPRESSION" );
	    }

	    if ( ( compression == COMPRESSION_JPEG ) ||
			( compression == COMPRESSION_OJPEG ) ) {
		    		
		TIFFSetField ( input, TIFFTAG_JPEGCOLORMODE,
			JPEGCOLORMODE_RGB );
		type = TTTypeRGB;	/* Codec does conversion */
	    }

	    break;

	case TTTypeLogLuv:
	case TTTypeLogL:

	    /* Force values to be returned as 32 bit floats */
	    TIFFSetField ( input, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT );
	    /*
	     * This forces TIFFTAG_BITSPERSAMPLE to be 32 in the directory
	     * as seen by the program, while it is (still) 16 in the file
	     * directory, and changes TIFFTAG_SAMPLEFORMAT from
	     * SAMPLEFORMAT_INT to SAMPLEFORMAT_IEEEFP.
	     * Bug or "advanced feature"???
	     */
	    break;

	default:
	    break;	/* don't do anything */
    }

    TT_image_size ( input, height, width );
    *ptype = type;

    return ( input );
}

#define	TT_GETROWBUF( NAME, TYPE, TYPE_CODE )				\
TYPE	*NAME ( TIFF *tif )						\
{									\
    void	    *p;							\
    unsigned int    height, width;					\
    TTType	    type;						\
									\
    type = TT_file_type ( tif );					\
    if ( ( type == TTTypeFloat16 ) || ( type == TTTypeFloat24 ) ) {	\
	type = TTTypeFloat;						\
    }									\
    if ( ( type == TTTypeRGBf16 ) || ( type == TTTypeRGBf24 ) ) {	\
	type = TTTypeRGBf;						\
    }									\
    if ( type != TYPE_CODE ) {				\
	TT_fatal_error ( "TT_getrowbuf: pixel type in file doesn't match!" );\
    }									\
									\
    TT_image_size ( tif, &height, &width );				\
									\
    p = malloc ( width * sizeof ( TYPE ) );				\
    	/* User might free() this, so not clear if we should */		\
        /* use malloc or _TIFFmalloc */					\
    if ( p == NULL ) {							\
	TT_fatal_error ( "TT_getrowbuf: malloc failed!" );		\
    }									\
									\
    return ( p );							\
}

TT_GETROWBUF ( TT_gray_getrowbuf, TT_gray, TTTypeGray );
TT_GETROWBUF ( TT_gray16_getrowbuf, TT_gray16, TTTypeGray16 );
TT_GETROWBUF ( TT_int16_getrowbuf, TT_int16, TTTypeShort );
TT_GETROWBUF ( TT_uint32_getrowbuf, TT_uint32, TTTypeUInt );
TT_GETROWBUF ( TT_int32_getrowbuf, TT_int32, TTTypeInt );
TT_GETROWBUF ( TT_float_getrowbuf, TT_float, TTTypeFloat );
TT_GETROWBUF ( TT_Y_getrowbuf, TT_Y, TTTypeLogL );
TT_GETROWBUF ( TT_double_getrowbuf, TT_double, TTTypeDouble );
TT_GETROWBUF ( TT_RGB_getrowbuf, TT_RGB, TTTypeRGB );
TT_GETROWBUF ( TT_RGB16_getrowbuf, TT_RGB16, TTTypeRGB16 );
TT_GETROWBUF ( TT_RGBA_getrowbuf, TT_RGBA, TTTypeRGBA );
TT_GETROWBUF ( TT_RGBf_getrowbuf, TT_RGBf, TTTypeRGBf );
TT_GETROWBUF ( TT_XYZ_getrowbuf, TT_XYZ, TTTypeLogLuv );


#define TT_OPEN_READ( NAME, TYPECODE )					\
TIFF *									\
NAME ( char *filename, unsigned int *height, unsigned int *width )	\
{									\
    TIFF    *tiff;							\
    TTType  type;							\
									\
    tiff = TT_open_read ( filename, &type, height, width );		\
    /* errors in TT_open_read are fatal! */				\
									\
    type = TT_file_type ( tiff );					\
    if ( type != TYPECODE ) {						\
	fprintf ( stderr, "%s:\n", filename );				\
	TT_fatal_error ( "invalid pixel type in file!" );		\
    }									\
									\
    return ( tiff );							\
}

#define TT_READ( NAME, TYPECODE, TYPE )					\
void									\
NAME ( TIFF *tiff, TYPE *buf, uint32 row )				\
{									\
    TTType	type;							\
									\
    type = TT_file_type ( tiff );					\
    /* LogLuv and LogL file types are different in program! */		\
    if ( type == TTTypeLogLuv ) {					\
	type = TTTypeXYZ;						\
    }									\
    if ( type == TTTypeLogL ) {						\
	type = TTTypeFloat;						\
    }									\
									\
    if ( type != TYPECODE ) {						\
	TT_fatal_error ( #NAME ": incompatible pixel type in file!" );	\
    }									\
									\
    if ( TIFFReadScanline ( tiff, (tdata_t) buf, row, 0 ) != 1 ) {	\
	exit ( EXIT_FAILURE );						\
	/* libtiff should print error message */			\
    }									\
}

TT_OPEN_READ ( TT_gray_open_read, TTTypeGray );
TT_OPEN_READ ( TT_gray16_open_read, TTTypeGray16 );
TT_OPEN_READ ( TT_RGB_open_read, TTTypeRGB );
TT_OPEN_READ ( TT_RGBA_open_read, TTTypeRGBA );
TT_OPEN_READ ( TT_RGB16_open_read, TTTypeRGB16 );
TT_OPEN_READ ( TT_float_open_read, TTTypeFloat );
TT_OPEN_READ ( TT_RGB_open_readf, TTTypeRGBf );
TT_OPEN_READ ( TT_XYZ_open_read, TTTypeXYZ );
TT_OPEN_READ ( TT_open_read_LogLuv, TTTypeLogLuv );
TT_OPEN_READ ( TT_open_read_LogL, TTTypeLogL );

TT_READ ( TT_gray_read, TTTypeGray, TT_gray );
TT_READ ( TT_gray16_read, TTTypeGray16, TT_gray16 );
TT_READ ( TT_RGB_read, TTTypeRGB, TT_RGB );
TT_READ ( TT_RGB16_read, TTTypeRGB16, TT_RGB16 );
/***************************************************************
Handled with individualized code due to need to accomodate 16 and 32 bit floats
TT_READ ( TT_float_read, TTTypeFloat, TT_float );
TT_READ ( TT_RGBf_read, TTTypeRGBf, TT_RGBf );
 ***************************************************************/
TT_READ ( TT_XYZ_read, TTTypeXYZ, TT_XYZ );
TT_READ ( TT_LogLuv_read, TTTypeXYZ, TT_XYZ );
TT_READ ( TT_LogL_read, TTTypeFloat, TT_float );

TIFF *
TT_open_write ( char *filename, TTType type, unsigned int height,
	unsigned int width )
{
    TIFF	    *output;
    TT_TIFFParms    tiff_parms;
    uint16	    p;		/* for setting TIFFTAG_EXTRASAMPLES */

    TT_struct_size_check ( );	/* runtime check of struct size compatibility */

    if ( ( height <= 0 ) || ( width <= 0 ) ) {
	fprintf ( stderr, "TT_open_write: invalid image size!\n" );
	exit ( EXIT_FAILURE );
	}

    output = TIFFOpen ( filename, "w" );
    if ( output == NULL ) {
	/* TIFFOpen should output error message */
	exit ( EXIT_FAILURE );
    }

    if ( type == TTTypeUnknown ) {
	fprintf ( stderr, "TT_open_write: unknowns type!\n" );
	exit ( EXIT_FAILURE );
    }

    tiff_parms = TT_type2parms ( type );

    TIFFSetField ( output, TIFFTAG_IMAGELENGTH, height );
    TIFFSetField ( output, TIFFTAG_IMAGEWIDTH, width );
    TIFFSetField ( output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    TIFFSetField ( output, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
    TIFFSetField ( output, TIFFTAG_SOFTWARE, "tifftools" );

    TIFFSetField ( output, TIFFTAG_SAMPLESPERPIXEL,
	    tiff_parms.samples_per_pixel );
    TIFFSetField ( output, TIFFTAG_PHOTOMETRIC, tiff_parms.photometric );

    if ( ( type == TTTypeLogLuv ) || ( type == TTTypeLogL ) ) {
	TIFFSetField ( output, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG );
	TIFFSetField ( output, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT );
	/*
	 * This forces TIFFTAG_BITSPERSAMPLE to be 32 in the directory as
	 * seen by the program, but 16 in the directory that is written to
	 * the file, and changes TIFFTAG_SAMPLEFORMAT from SAMPLEFORMAT_INT
	 * to SAMPLEFORMAT_IEEEFP.  Bug or "advanced feature"???
	 */
    } else {
	TIFFSetField ( output, TIFFTAG_BITSPERSAMPLE,
		tiff_parms.bits_per_sample );
	TIFFSetField ( output, TIFFTAG_SAMPLEFORMAT, tiff_parms.sample_format );
    }

    if ( type == TTTypeRGBA ) {
	/*
	 * Default to an associated alpha interpretation.  (This is the
	 * interpretation that Photoshop uses.) It is up to the calling program
	 * to do the premultiplies of the RGB values.  The tag value can be
	 * overridden if desired.
	 */

	p = EXTRASAMPLE_ASSOCALPHA; 
	TIFFSetField ( output, TIFFTAG_EXTRASAMPLES, 1, &p );
    }

    TIFFSetField ( output, TIFFTAG_ROWSPERSTRIP,
	    TIFFDefaultStripSize ( output, 0 ) );

    return ( output );
}

#define TT_OPEN_WRITE( NAME, TYPECODE )					\
TIFF *									\
NAME ( char *filename, unsigned int height, unsigned int width )	\
{									\
    TIFF    *tiff;							\
									\
    tiff = TT_open_write ( filename, TYPECODE, height, width );		\
    /* errors in TT_open_write are fatal! */				\
									\
    return ( tiff );							\
}

#define TT_WRITE( NAME, TYPECODE, TYPE )				\
void									\
NAME ( TIFF *tiff, TYPE *buf, uint32 row )				\
{									\
    TTType	type;							\
									\
    type = TT_file_type ( tiff );					\
    /* LogLuv and LogL file types are different in program! */		\
    if ( type == TTTypeLogLuv ) {					\
	type = TTTypeXYZ;						\
    }									\
    if ( type == TTTypeLogL ) {						\
	type = TTTypeFloat;						\
    }									\
									\
    if ( type != TYPECODE ) {						\
	TT_fatal_error ( #NAME ": incompatible pixel type in file!" );	\
    }									\
    									\
    if ( TIFFWriteScanline ( tiff, (tdata_t) buf, row, 0 ) == -1 ) {	\
	/*								\
	 * A successful write of a LogLuv or LogL files produces a	\
	 * return code of 0, rather than 1 as documented!		\
	 */								\
	exit ( EXIT_FAILURE );						\
	/* libtiff should print error message */			\
    }									\
}

TT_OPEN_WRITE ( TT_gray_open_write, TTTypeGray );
TT_OPEN_WRITE ( TT_gray16_open_write, TTTypeGray16 );
TT_OPEN_WRITE ( TT_RGB_open_write, TTTypeRGB );
TT_OPEN_WRITE ( TT_RGBA_open_write, TTTypeRGBA );
TT_OPEN_WRITE ( TT_RGB16_open_write, TTTypeRGB16 );
TT_OPEN_WRITE ( TT_float_open_write, TTTypeFloat );
TT_OPEN_WRITE ( TT_RGBf_open_write, TTTypeRGBf );
TT_OPEN_WRITE ( TT_RGBf16_open_write, TTTypeRGBf16 );
TT_OPEN_WRITE ( TT_RGBf24_open_write, TTTypeRGBf24 );
TT_OPEN_WRITE ( TT_XYZ_open_write, TTTypeXYZ );
TT_OPEN_WRITE ( TT_LogLuv_open_write, TTTypeLogLuv );
TT_OPEN_WRITE ( TT_logL_open_write, TTTypeLogL );

TT_WRITE ( TT_gray_write, TTTypeGray, TT_gray );
TT_WRITE ( TT_gray16_write, TTTypeGray16, TT_gray16 );
TT_WRITE ( TT_RGB_write, TTTypeRGB, TT_RGB );
TT_WRITE ( TT_RGBA_write, TTTypeRGBA, TT_RGBA );
TT_WRITE ( TT_RGB16_write, TTTypeRGB16, TT_RGB16 );
/***************************************************************
Handled with individualized code due to need to accomodate 16 and 24 bit floats
Note that there are NOT specialized user-callable write routines for these types
TT_WRITE ( TT_float_write, TTTypeFloat, TT_float );
TT_WRITE ( TT_RGBf_write, TTTypeRGBf, TT_RGBf );
 ***************************************************************/
TT_WRITE ( TT_XYZ_write, TTTypeXYZ, TT_XYZ );
TT_WRITE ( TT_LogLuv_write, TTTypeXYZ, TT_XYZ );
TT_WRITE ( TT_LogL_write, TTTypeFloat, TT_float );

/*
 * User-callable routines to read/write floating point values, supporting
 * 32 bit, 24 bit, and 16 bit floats.
 */

void
TT_float_read ( TIFF *tif, TT_float *buf, uint32 row )
/*
 * Read a scanline from an alread open TIFF input file containing data in one
 * of the following grayscale floating point formats:
 *
 *     16 bit float
 *     24 bit float
 *     32 bit float
 *
 * Values are returned as grayscale using 32 bit floats.
 *
 * The routine queries the TIFF descriptor on each call to find out the
 * type of the data returned by TIFFRead, which is tedious but saves
 * the need to keep state elsewhere.
 *
 * A fatal error is generated if this routine is passed a handle to a TIFF
 * file that is other than 16, 24, or 32 bit floating point grayscale.
 *
 * Errors detected by libtiff are handled as in libtiff: the routine returns
 * -1 in the case of an error and 1 otherwise, with error messages directed
 * to the TIFFError routine.
 */
{
    /* uint16	planarconfig; */
    uint16	photometric;
    uint16	sampleformat;
    uint16	bitspersample;
    uint16	samplesperpixel;
    uint32	width;
    tsize_t	scanline_size;
    tsample_t	sample = 0;

    static tdata_t	scanline_buffer = NULL;
    static tsize_t	old_scanline_size = -1;

    /* must be grayscale */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLESPERPIXEL,
		&samplesperpixel ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }
    if ( samplesperpixel != 1 ) {
	TT_fatal_error ( "TT_float_read: not grayscale image!" );
    }

    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PHOTOMETRIC, &photometric ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }
    if ( ( photometric != PHOTOMETRIC_MINISWHITE ) &&
	    ( photometric != PHOTOMETRIC_MINISBLACK ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }

    /* must be planar-contiguous [but this is irrelevant for grayscale!] */
    /*******************************
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PLANARCONFIG, &planarconfig ) ) {
	return ( TIFF_READ_ERROR_RETURN );
    }
    if ( planarconfig != PLANARCONFIG_CONTIG ) {
	TT_fatal_error ( "TT_float_read: invalid PlanarConfig!" );
    }
     *******************************/

    /* must be floating point */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLEFORMAT, &sampleformat ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }
    if ( sampleformat != SAMPLEFORMAT_IEEEFP ) {
	TT_fatal_error ( "TT_float_read: not floating point!" );
    }

    /* size of numeric values (should be 16, 24, or 32) */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_BITSPERSAMPLE,
		&bitspersample ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }

    /* need # of pixels per line for conversion routines */
    if ( !TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) ) {
	TT_fatal_error ( "TT_float_read: TIFF read error" );
    }

    scanline_size = TIFFRasterScanlineSize ( tif );

    if ( ( bitspersample != 32 ) && ( scanline_size > old_scanline_size ) ) {
	if ( scanline_buffer != NULL ) {
	    _TIFFfree ( scanline_buffer );
	}

	scanline_buffer = _TIFFmalloc ( scanline_size );
	if ( scanline_buffer == NULL ) {
	    TT_fatal_error ( "TT_float_read: malloc failed!" );
	}

	old_scanline_size = scanline_size;
    }

    if ( bitspersample == 32 ) {
	/* no need for conversion! */
	if ( TIFFReadScanline ( tif, buf, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
	return;
    } else {
	if ( TIFFReadScanline ( tif, scanline_buffer, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    }

    switch ( bitspersample ) {
	case 16:
	    convert_float_16_32 ( scanline_buffer, buf, width );
	    break;

	case 24:
	    convert_float_24_32 ( scanline_buffer, buf, width );
	    break;

	case 32:
	    TT_fatal_error ( "TT_float_read: internal error!" );
	    break;

	default:
	    TT_fatal_error (
		"TT_float_read: invalid floating point type!"
		);
	    break;
    }
}

void
TT_RGBf_read ( TIFF *tif, TT_RGBf *buf, uint32 row )
/*
 * Read a scanline from an alread open TIFF input file containing data in one
 * of the following RGB floating point formats:
 *
 *     16 bit float
 *     24 bit float
 *     32 bit float
 *
 * Values are returned as RGB using 32 bit floats.
 *
 * The routine queries the TIFF descriptor on each call to find out the
 * type of the data returned by TIFFRead, which is tedious but saves
 * the need to keep state elsewhere.
 *
 * A fatal error is generated if this routine is passed a handle to a TIFF
 * file that is other than 16, 24, or 32 bit floating point RGB.
 *
 * Errors detected by libtiff are handled as in libtiff: the routine returns
 * -1 in the case of an error and 1 otherwise, with error messages directed
 * to the TIFFError routine.
 */
{
    uint16	planarconfig;
    uint16	photometric;
    uint16	sampleformat;
    uint16	bitspersample;
    uint16	samplesperpixel;
    uint32	width;
    tsize_t	scanline_size;
    tsample_t	sample = 0;

    static tdata_t	scanline_buffer = NULL;
    static tsize_t	old_scanline_size = -1;

    /* must be RGB */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLESPERPIXEL,
		&samplesperpixel ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }
    if ( samplesperpixel != 3 ) {
	TT_fatal_error ( "TIFFReadScanlineRGBfloat: not RGB image!" );
    }

    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PHOTOMETRIC, &photometric ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }
    if ( photometric != PHOTOMETRIC_RGB ) {
	TT_fatal_error ( "TIFFReadScanlineRGBfloat: not RGB image!" );
    }

    /* must be planar-contiguous */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PLANARCONFIG, &planarconfig ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }
    if ( planarconfig != PLANARCONFIG_CONTIG ) {
	TT_fatal_error (
		"TIFFReadScanlineRGBfloat: invalid PlanarConfig!"
		);
    }

    /* must be floating point */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLEFORMAT, &sampleformat ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }
    if ( sampleformat != SAMPLEFORMAT_IEEEFP ) {
	TT_fatal_error ( "TIFFReadScanlineRGBfloat: not floating point!" );
    }

    /* size of numeric values (should be 16, 24, or 32) */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_BITSPERSAMPLE,
		&bitspersample ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }

    /* need # of pixels per line for conversion routines */
    if ( !TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) ) {
	TT_fatal_error ( "TT_RGBf_read: TIFF read error" );
    }

    scanline_size = TIFFRasterScanlineSize ( tif );

    if ( ( bitspersample != 32 ) && ( scanline_size > old_scanline_size ) ) {
	if ( scanline_buffer != NULL ) {
	    _TIFFfree ( scanline_buffer );
	}

	scanline_buffer = _TIFFmalloc ( scanline_size );
	if ( scanline_buffer == NULL ) {
	    TT_fatal_error ( "TIFFReadScanlineRGBfloat: malloc failed!" );
	}

	old_scanline_size = scanline_size;
    }

    if ( bitspersample == 32 ) {
	/* no need for conversion! */
	if ( TIFFReadScanline ( tif, buf, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    } else {
	if ( TIFFReadScanline ( tif, scanline_buffer, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    }

    switch ( bitspersample ) {
	case 16:
	    convert_RGB_float_16_32 ( scanline_buffer, buf, width );
	    break;

	case 24:
	    convert_RGB_float_24_32 ( scanline_buffer, buf, width );
	    break;

	case 32:
	    /* everything is fine... */
	    break;

	default:
	    TT_fatal_error (
		"TIFFReadScanlineRGBfloat: invalid floating point type!"
		);
	    break;
    }
}

void
TT_float_write ( TIFF *tif, TT_float *buf, uint32 row  )
/*
 * Writes a scanline to an alread open TIFF input file supporting data in one
 * of the following grayscale floating point formats:
 *
 *     16 bit float
 *     24 bit float
 *     32 bit float
 *
 * Values are passed to the routine as grayscale using 32 bit floats.
 *
 * The routine queries the TIFF descriptor on each call to find out the
 * type of the data expected by TIFFWrite, which is tedious but saves
 * the need to keep state elsewhere.
 *
 * A fatal error is generated if this routine is passed a handle to a TIFF
 * file that is other than 16, 24, or 32 bit floating point grayscale.
 *
 * Errors detected by libtiff are handled as in libtiff: the routine returns
 * -1 in the case of an error and 1 otherwise, with error messages directed
 * to the TIFFError routine.
 */
{
    /* uint16	planarconfig; */
    uint16	photometric;
    uint16	sampleformat;
    uint16	bitspersample;
    uint16	samplesperpixel;
    uint32	width;
    tsize_t	scanline_size;
    tsample_t	sample = 0;

    static tdata_t	scanline_buffer = NULL;
    static tsize_t	old_scanline_size = -1;

    /* must be grayscale */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLESPERPIXEL,
		&samplesperpixel ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }
    if ( samplesperpixel != 1 ) {
	TT_fatal_error ( "TT_float_write: not grayscale image!" );
    }

    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PHOTOMETRIC, &photometric ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }
    if ( ( photometric != PHOTOMETRIC_MINISWHITE ) &&
	    ( photometric != PHOTOMETRIC_MINISBLACK ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }

    /* must be planar-contiguous [but this is irrelevant for grayscale!] */
    /*******************************
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PLANARCONFIG, &planarconfig ) ) {
	return ( TIFF_WRITE_ERROR_RETURN );
    }
    if ( planarconfig != PLANARCONFIG_CONTIG ) {
	TT_fatal_error ( "TT_float_write: invalid PlanarConfig!" );
    }
     *******************************/

    /* must be floating point */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLEFORMAT, &sampleformat ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }
    if ( sampleformat != SAMPLEFORMAT_IEEEFP ) {
	TT_fatal_error ( "TT_float_write: not floating point!" );
    }

    /* size of numeric values (should be 16, 24, or 32) */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_BITSPERSAMPLE,
		&bitspersample ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }

    /* need # of pixels per line for conversion routines */
    if ( !TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) ) {
	TT_fatal_error ( "TT_float_write: TIFF write error" );
    }

    scanline_size = TIFFRasterScanlineSize ( tif );

    if ( ( bitspersample != 32 ) && ( scanline_size != old_scanline_size ) ) {
	if ( scanline_buffer != NULL ) {
	    _TIFFfree ( scanline_buffer );
	}

	scanline_buffer = _TIFFmalloc ( scanline_size );
	if ( scanline_buffer == NULL ) {
	    TT_fatal_error ( "TT_float_write: malloc failed!" );
	}

	old_scanline_size = scanline_size;
    }

    switch ( bitspersample ) {
	case 16:
	    convert_float_32_16 ( buf, scanline_buffer, width );
	    break;

	case 24:
	    convert_float_32_24 ( buf, scanline_buffer, width );
	    break;

	case 32:
	    /* see below... */
	    break;

	default:
	    TT_fatal_error (
		"TT_float_write: invalid floating point type!"
		);
	    break;
    }

    if ( bitspersample == 32 ) {
	/* no need for conversion! */
	if ( TIFFWriteScanline ( tif, buf, row, sample ) != 1) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    } else {
	if ( TIFFWriteScanline ( tif, scanline_buffer, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    }
}

void
TT_RGBf_write ( TIFF *tif, TT_RGBf *buf, uint32 row )
/*
 * Writes a scanline to an alread open TIFF input file supporting data in one
 * of the following RGB floating point formats:
 *
 *     16 bit float
 *     24 bit float
 *     32 bit float
 *
 * Values are passed to the routine as RGB using 32 bit floats.
 *
 * The routine queries the TIFF descriptor on each call to find out the
 * type of the data returned by TIFFRead, which is tedious but saves
 * the need to keep state elsewhere.
 *
 * A fatal error is generated if this routine is passed a handle to a TIFF
 * file that is other than 16, 24, or 32 bit floating point RGB.
 *
 * Errors detected by libtiff are handled as in libtiff: the routine returns
 * -1 in the case of an error and 1 otherwise, with error messages directed
 * to the TIFFError routine.
 */
{
    uint16	planarconfig;
    uint16	photometric;
    uint16	sampleformat;
    uint16	bitspersample;
    uint16	samplesperpixel;
    uint32	width;
    tsize_t	scanline_size;
    tsample_t	sample = 0;

    static tdata_t	scanline_buffer = NULL;
    static tsize_t	old_scanline_size = -1;

    /* must be RGB */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLESPERPIXEL,
		&samplesperpixel ) ) {
	TT_fatal_error ( "TT_RGBf_write: TIFF write error" );
    }
    if ( samplesperpixel != 3 ) {
	TT_fatal_error ( "TT_RGBf_write: not RGB image!" );
    }

    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PHOTOMETRIC, &photometric ) ) {
	TT_fatal_error ( "TT_RGBf_write: not RGB image!" );
    }
    if ( photometric != PHOTOMETRIC_RGB ) {
	TT_fatal_error ( "TT_RGBf_write: not RGB image!" );
    }

    /* must be planar-contiguous */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_PLANARCONFIG, &planarconfig ) ) {
	TT_fatal_error ( "TT_RGBf_write: TIFF write error" );
    }
    if ( planarconfig != PLANARCONFIG_CONTIG ) {
	TT_fatal_error (
		"TT_RGBf_write: invalid PlanarConfig!"
		);
    }

    /* must be floating point */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_SAMPLEFORMAT, &sampleformat ) ) {
	TT_fatal_error ( "TT_RGBf_write: TIFF write error" );
    }
    if ( sampleformat != SAMPLEFORMAT_IEEEFP ) {
	TT_fatal_error ( "TT_RGBf_write: not floating point!" );
    }

    /* size of numeric values (should be 16, 24, or 32) */
    if ( !TIFFGetFieldDefaulted ( tif, TIFFTAG_BITSPERSAMPLE,
		&bitspersample ) ) {
	TT_fatal_error ( "TT_RGBf_write: TIFF write error" );
    }

    /* need # of pixels per line for conversion routines */
    if ( !TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) ) {
	TT_fatal_error ( "TT_RGBf_write: TIFF write error" );
    }

    scanline_size = TIFFRasterScanlineSize ( tif );

    if ( ( bitspersample != 32 ) && ( scanline_size != old_scanline_size ) ) {
	if ( scanline_buffer != NULL ) {
	    _TIFFfree ( scanline_buffer );
	}

	scanline_buffer = _TIFFmalloc ( scanline_size );
	if ( scanline_buffer == NULL ) {
	    TT_fatal_error ( "TT_RGBf_write: malloc failed!" );
	}

	old_scanline_size = scanline_size;
    }

    switch ( bitspersample ) {
	case 16:
	    convert_RGB_float_32_16 ( buf, scanline_buffer, width );
	    break;

	case 24:
	    convert_RGB_float_32_24 ( buf, scanline_buffer, width );
	    break;

	case 32:
	    /* see below... */
	    break;

	default:
	    TT_fatal_error (
		"TT_RGBf_write: invalid floating point type!"
		);
	    break;
    }

    if ( bitspersample == 32 ) {
	/* no need for conversion! */
	if ( TIFFWriteScanline ( tif, buf, row, sample ) != 1) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    } else {
	if ( TIFFWriteScanline ( tif, scanline_buffer, row, sample ) != 1 ) {
	    /* libtiff should print error message */
	    exit ( EXIT_FAILURE );
	}
    }
}

void
TT_set_compression_jpeg ( TIFF *file, int jpeg_quality )
{
    if ( ( jpeg_quality > 95 ) || ( jpeg_quality < 5 ) ) {
	fprintf ( stderr,
		"TT_set_compression_jpeg: JPEG quality out of range (%d)\n",
		jpeg_quality );
	exit ( EXIT_FAILURE );
    }

    TIFFSetField ( file, TIFFTAG_COMPRESSION, COMPRESSION_JPEG );
    TIFFSetField ( file, TIFFTAG_JPEGQUALITY, jpeg_quality );

    switch ( TT_file_type ( file ) ) {
	case TTTypeGray:
	    TIFFSetField ( file, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW );
	    break;

	case TTTypeRGB:
	    TIFFSetField ( file, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB );
	    TIFFSetField ( file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR );
	    break;

	default:
	    fprintf ( stderr,
	"TT_set_compression_jpeg: file not JPEG compressible (warning)\n" );
	    return;
    }

    /* need to reset strip size now that compression is turned on */
    TIFFSetField ( file, TIFFTAG_ROWSPERSTRIP,
	    TIFFDefaultStripSize ( file, 0) );
}

void
TT_set_compression_LZW ( TIFF *file, int prediction )
/*
 * WARNING:  With prediction enabled, input buffer is corrupted after
 * a write!!!!  
 */
{
    switch ( TT_file_type ( file ) ) {
	case TTTypeRGBf:
	case TTTypeRGBf16:	/* ??? */
	case TTTypeRGBf24:	/* ??? */
	case TTTypeFloat:
	case TTTypeFloat16:	/* ??? */
	case TTTypeFloat24:	/* ??? */
	case TTTypeDouble:	/* ??? */
	    TIFFSetField ( file, TIFFTAG_COMPRESSION, COMPRESSION_LZW );
	    if ( prediction ) {
		TIFFSetField ( file, TIFFTAG_PREDICTOR,
			PREDICTOR_FLOATINGPOINT );
	    }
	    break;

	default:  /* non-floating types */
	    TIFFSetField ( file, TIFFTAG_COMPRESSION, COMPRESSION_LZW );
	    if ( prediction ) {
		TIFFSetField ( file, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL );
	    }
    }
}

void
TT_set_compression_zip ( TIFF *file, int prediction )
/*
 * WARNING:  With prediction enabled, input buffer is corrupted after
 * a write!!!!  
 */
{
    switch ( TT_file_type ( file ) ) {
	case TTTypeRGBf:
	case TTTypeRGBf16:	/* ??? */
	case TTTypeRGBf24:	/* ??? */
	case TTTypeFloat:
	case TTTypeFloat16:	/* ??? */
	case TTTypeFloat24:	/* ??? */
	case TTTypeDouble:	/* ??? */
	    TIFFSetField ( file, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE );
	    if ( prediction ) {
		TIFFSetField ( file, TIFFTAG_PREDICTOR,
			PREDICTOR_FLOATINGPOINT );
	    }
	    break;

	default:  /* non-floating types */
	    TIFFSetField ( file, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE );
	    if ( prediction ) {
		TIFFSetField ( file, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL );
	    }
	    break;
    }
}

int
TT_is_alpha_unspecified ( TIFF *file )
{
    uint16	count, *value;

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to get alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    TIFFGetField ( file, TIFFTAG_EXTRASAMPLES, &count, &value );

    if ( ( count != 1 ) || ( *value != EXTRASAMPLE_UNSPECIFIED ) ) {
	return ( FALSE );
    } else {
	return ( TRUE );
    }
}

int
TT_is_alpha_associated ( TIFF *file )
{
    uint16	count, *value;

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to get alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    TIFFGetField ( file, TIFFTAG_EXTRASAMPLES, &count, &value );

    if ( ( count != 1 ) || ( *value != EXTRASAMPLE_ASSOCALPHA ) ) {
	return ( FALSE );
    } else {
	return ( TRUE );
    }
}

int
TT_is_alpha_unassociated ( TIFF *file )
{
    uint16	count, *value;

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to get alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    TIFFGetField ( file, TIFFTAG_EXTRASAMPLES, &count, &value );

    if ( ( count != 1 ) || ( *value != EXTRASAMPLE_UNASSALPHA ) ) {
	return ( FALSE );
    } else {
	return ( TRUE );
    }
}

void
TT_set_alpha_unspecified ( TIFF *file )
{
    uint16	    p;		/* for setting TIFFTAG_EXTRASAMPLES */

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to set alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    p = EXTRASAMPLE_UNSPECIFIED; 
    TIFFSetField ( file, TIFFTAG_EXTRASAMPLES, 1, &p );
}

void
TT_set_alpha_associated ( TIFF *file )
{
    uint16	    p;		/* for setting TIFFTAG_EXTRASAMPLES */

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to set alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    p = EXTRASAMPLE_ASSOCALPHA; 
    TIFFSetField ( file, TIFFTAG_EXTRASAMPLES, 1, &p );
}

void
TT_set_alpha_unassociated ( TIFF *file )
{
    uint16	    p;		/* for setting TIFFTAG_EXTRASAMPLES */

    if ( TT_file_type ( file ) != TTTypeRGBA ) {
	fprintf ( stderr, "attempt to set alpha type on non-RGBA image!\n" );
	exit ( EXIT_FAILURE );
    }

    p = EXTRASAMPLE_UNASSALPHA; 
    TIFFSetField ( file, TIFFTAG_EXTRASAMPLES, 1, &p );
}

char *
TT_get_description ( TIFF *tif )
/*
 * Returns malloced copy of the string, which can freed if memory leaks
 * are a concern.
 */
{
    char    *description, *description_copy;

    if ( TIFFGetField ( tif, TIFFTAG_IMAGEDESCRIPTION, &description ) != 1 ) {
	return ( NULL );
    } else {
	description_copy = strdup ( description );
	if ( description_copy == NULL ) {
	    fprintf ( stderr, "TT_get_description: malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
	return ( description_copy );
    }
}

void
TT_set_description ( TIFF *tif, char *description )
/*
 * Does not check to see if file is open for writing (but should!).
 * Does not do sanity checking on description string.
 */
{
    if ( description != NULL ) {
    	TIFFSetField ( tif, TIFFTAG_IMAGEDESCRIPTION, description );
    }
}

void
TT_cat_description ( TIFF *tif, char *description )
/*
 * Does not check to see if file is open for writing (but should!).
 */
{
    char    *old_description, *new_description;
    int	    size;

    if ( description == NULL ) {
	return;
    }

    if ( TIFFGetField ( tif, TIFFTAG_IMAGEDESCRIPTION,
		&old_description ) != 1 ) {
	fprintf ( stderr, "TT_cat_description failed (warning)...\n" );
	return;
    }

    size = strlen ( old_description ) + strlen ( description ) + 1;
    new_description = (char *) _TIFFmalloc ( size * sizeof ( char ) );
    if ( new_description == NULL ) {
	fprintf ( stderr, "malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }
    strcpy ( new_description, old_description );
    strcat ( new_description, description );

    TIFFSetField ( tif, TIFFTAG_IMAGEDESCRIPTION, new_description );
    _TIFFfree ( new_description );
}

void
TT_set_description_arguments ( TIFF *tif, int argc, char *argv[] )
{
    char    *description;
    char    *command, *full_command;
    int	    arg;
    int	    size;

    full_command = strdup ( argv[0] );	/* might be modified by basename */
    command = basename ( full_command );

    size = strlen ( command );
    for ( arg = 1; arg < argc; arg++ ) {
	size += 1 + strlen ( argv[arg] );
    }

    description = (char *) _TIFFmalloc ( ( size + 1 ) * sizeof ( char ) );
    if ( description == NULL ) {
	TT_fatal_error ( "TT_set_description_arguments: malloc failed!" );
    }

    description[0] = '\0';

    strcat ( description, command );

    for ( arg = 1; arg < argc; arg++ ) {
	strcat ( description, " " );
	strcat ( description, argv[arg] );
    }

    TT_set_description ( tif, description );

    free ( full_command );
    _TIFFfree ( description );
}

/************************************************************************
 * Remaining routines are utility functions that are not user-callable. *
 ************************************************************************/

static void
get_tiff_tag_error ( char *tagname )
/*
 * Simple error routine.
 */
{
    fprintf ( stderr, "Failed to get TIFF tag (%s)!\n", tagname );
    exit ( EXIT_FAILURE );
}

static int
end_of_TIFF_file_parms ( TT_TIFFParms parms )
{
    return ( ( parms.bits_per_sample == 0 ) &&
	( parms.samples_per_pixel == 0 ) &&
	( parms.photometric == 0 ) &&
	( parms.sample_format == 0 ) &&
	( parms.type == TTTypeUnknown ) );
}

static int
match_tiff_parms ( TT_TIFFParms p1, TT_TIFFParms p2 )
{
    return ( ( p1.bits_per_sample == p2.bits_per_sample ) &&
	( p1.samples_per_pixel == p2.samples_per_pixel ) &&
	( p1.photometric == p2.photometric ) &&
	( p1.sample_format == p2.sample_format ) );
}

static TT_TIFFParms
TT_type2parms ( TTType type )
/*
 * Returns pointer to parameter structure describing TIFF type.
 */
{
    int	    i;

    i = 0;
    while ( !end_of_TIFF_file_parms ( TIFF_file_parms[i] ) ) {
	if ( TIFF_file_parms[i].type == type ) {
	    return ( TIFF_file_parms[i] );
	}
	i++;
    }

    return ( TIFF_file_parms[i] );	/* unknown! */
}

static void
TT_fatal_error_loc ( char *filename, int linenumber, const char *message )
{
    fprintf ( stderr, "tifftools fatal error at line %d in file %s\n",
	    linenumber, filename );
    fprintf ( stderr, "%s", message );
    if ( message[strlen(message)-1] != '\n' ) {
	fprintf ( stderr, "\n" );
    }

    exit ( EXIT_FAILURE );
}

static void
TT_struct_size_check ( void )
{
    static int	first = TRUE;
    int		violation_count = 0;

    if ( !first ) {
	return;
    }

    first = FALSE;

    if ( sizeof ( TT_RGB ) != sizeof ( uint8[3] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGB)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_RGBA ) != sizeof ( uint8[4] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGBA)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_RGB16 ) != sizeof ( uint16[3] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGB16)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_RGBint32 ) != sizeof ( int32[3] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGBint32)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_RGBf ) != sizeof ( float[3] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGBf)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_RGBAf ) != sizeof ( float[4] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_RGBAf)!\n" );
	violation_count++;
    }
    if ( sizeof ( TT_XYZ ) != sizeof ( float[3] ) ) {
	fprintf ( stderr, "TT_struct_size_check violation (TT_XYZ)!\n" );
	violation_count++;
    }

    if ( violation_count > 0 ) {
	exit ( EXIT_FAILURE );
    }
}

/************************************************************************/
/*                           half_to_float()                            */
/*                                                                      */
/*  16-bit floating point number to 32-bit one.                         */
/************************************************************************/

static unsigned int half_to_float( unsigned short half )
{
  unsigned int iHalf = half;

  unsigned int iSign =     (iHalf >> 15) & 0x00000001;
  unsigned int iExponent = (iHalf >> 10) & 0x0000001f;
  unsigned int iMantissa = iHalf         & 0x000003ff;
  
  if (iExponent == 0)
  {
    if (iMantissa == 0)
    {
/* -------------------------------------------------------------------- */
/*      Plus or minus zero.                                             */
/* -------------------------------------------------------------------- */

      return iSign << 31;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*      Denormalized number -- renormalize it.                          */
/* -------------------------------------------------------------------- */

      while (!(iMantissa & 0x00000400))
      {
	iMantissa <<= 1;
	iExponent -=  1;
      }
      
      iExponent += 1;
      iMantissa &= ~0x00000400;
    }
  }
  else if (iExponent == 31)
  {
    if (iMantissa == 0)
    {
/* -------------------------------------------------------------------- */
/*       Positive or negative infinity.                                 */
/* -------------------------------------------------------------------- */

      return (iSign << 31) | 0x7f800000;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*       NaN -- preserve sign and significand bits.                     */
/* -------------------------------------------------------------------- */
      
      return (iSign << 31) | 0x7f800000 | (iMantissa << 13);
    }
  }

/* -------------------------------------------------------------------- */
/*       Normalized number.                                             */
/* -------------------------------------------------------------------- */

  iExponent = iExponent + (127 - 15);
  iMantissa = iMantissa << 13;

/* -------------------------------------------------------------------- */
/*       Assemble sign, exponent and mantissa.                          */
/* -------------------------------------------------------------------- */

  return (iSign << 31) | (iExponent << 23) | iMantissa;
}

/************************************************************************/
/*                           float_to_half()                            */
/*                                                                      */
/* 32-bit floating point number to 16-bit one.                          */
/************************************************************************/

static unsigned short float_to_half(unsigned int iFloat)
{
  int iSign =      (iFloat >> 16) & 0x00008000;
  int iExponent = ((iFloat >> 23) & 0x000000ff) - (127 - 15);
  int iMantissa =   iFloat        & 0x007fffff;

  if( iExponent <= 0 )
  {
    if( iExponent < -10 )
    {
/* -------------------------------------------------------------------- */
/* If the exponent is less than -10, the absolute value of f is         */
/* less than HALF_MIN, so convert to a half zero with correct sign.     */
/* -------------------------------------------------------------------- */
      return iSign;
    }

/* -------------------------------------------------------------------- */
/* Exponent is between -10 and 0, so iFloat is a normalized float with  */
/* magnitude less than HALF_NRM_MIN, so convert to denormalized half.   */
/* -------------------------------------------------------------------- */

    iMantissa = (iMantissa | 0x00800000) >> (1 - iExponent);

/* -------------------------------------------------------------------- */
/* Round to nearest value, may cause an overflow, which is handled      */
/* below as a separate case.                                            */
/* -------------------------------------------------------------------- */

    if(iMantissa & 0x00001000)
      iMantissa += 0x00002000;

/* -------------------------------------------------------------------- */
/* Assemble and return (iExponent = 0)                                  */
/* -------------------------------------------------------------------- */

    return iSign | (iMantissa >> 13);
  }
  else if( iExponent == 0xff - (127-15) )
  {
    if( iMantissa == 0 )
    {
/* -------------------------------------------------------------------- */
/* iFloat is infinity; convert to half infinity with correct sign.      */
/* -------------------------------------------------------------------- */

      return iSign | 0x7c00;
    }
    else
    {
/* -------------------------------------------------------------------- */
/* iFloat is NAN; convert appropriately                                 */
/* -------------------------------------------------------------------- */
      
      iMantissa >>= 13;
      return iSign | 0x7c00 | iMantissa | (iMantissa == 0);
    }
  }
  else
  {
/* -------------------------------------------------------------------- */
/* iExponent is greater than zero, iFloat is normalized, convert to     */
/* normalized half.                                                     */
/* -------------------------------------------------------------------- */

    /* round */
    if( iMantissa & 0x00001000 )
    {
      iMantissa += 0x00002000;

      /* overflow */
      if( iMantissa & 0x00800000 )
      {
	iMantissa = 0;
	iExponent += 1;
      }
    }

    /* handle exponent overflow */
    if( iExponent > 30 )
    {
      /* force a hardware overflow */
      volatile float f = 1e10;
      int i;
      for(i = 0; i < 10; ++i)
	f *= f;
      return iSign | 0x7c00; /* return infinity */
    }
    
/* -------------------------------------------------------------------- */
/* Assemble and return                                                  */
/* -------------------------------------------------------------------- */

    return iSign | (iExponent << 10) | (iMantissa >> 13);
  }
}

/************************************************************************/
/*                           triple_to_float()                          */
/*                                                                      */
/*  24-bit floating point number to 32-bit one.                         */
/************************************************************************/

static unsigned int triple_to_float( unsigned int iTriple )
{
  unsigned int iSign       = (iTriple >> 23) & 0x00000001;
  unsigned int iExponent   = (iTriple >> 16) & 0x0000007f;
  unsigned int iMantissa   = iTriple         & 0x0000ffff;

  if (iExponent == 0)
  {
    if (iMantissa == 0)
    {
/* -------------------------------------------------------------------- */
/*      Plus or minus zero.                                             */
/* -------------------------------------------------------------------- */

      return iSign << 31;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*      Denormalized number -- renormalize it.                          */
/* -------------------------------------------------------------------- */

      while (!(iMantissa & 0x00002000))
      {
	iMantissa <<= 1;
	iExponent -=  1;
      }

      iExponent += 1;
      iMantissa &= ~0x00002000;
    }
  }
  else if (iExponent == 127)
  {
    if (iMantissa == 0)
    {
/* -------------------------------------------------------------------- */
/*       Positive or negative infinity.                                 */
/* -------------------------------------------------------------------- */

      return (iSign << 31) | 0x7f800000;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*       NaN -- preserve sign and significand bits.                     */
/* -------------------------------------------------------------------- */

      return (iSign << 31) | 0x7f800000 | (iMantissa << 7);
    }
  }

/* -------------------------------------------------------------------- */
/*       Normalized number.                                             */
/* -------------------------------------------------------------------- */

  iExponent = iExponent + (127 - 63);
  iMantissa = iMantissa << 7;

/* -------------------------------------------------------------------- */
/*       Assemble sign, exponent and mantissa.                          */
/* -------------------------------------------------------------------- */

  return (iSign << 31) | (iExponent << 23) | iMantissa;
}


/************************************************************************/
/*                           float_to_triple()                          */
/*                                                                      */
/* 32-bit floating point number to 24-bit one.                          */
/************************************************************************/
static unsigned int float_to_triple(unsigned int iFloat)
{
  int iSign =      (iFloat >>  8) & 0x00800000;
  int iExponent = ((iFloat >> 23) & 0x000000ff) - (127 - 63);
  int iMantissa =   iFloat        & 0x007fffff;

  if( iExponent <= 0 )
  {
    if( iExponent < -16 )
    {
/* -------------------------------------------------------------------- */
/* If the exponent is less than -16, the absolute value of f is         */
/* less than TRIPLE_MIN, so convert to a half zero with correct sign.   */
/* -------------------------------------------------------------------- */
      return iSign;
    }

/* -------------------------------------------------------------------- */
/* Exponent is between -16 and 0, so iFloat is a normalized float with  */
/* magnitude less than TRIPLE_NRM_MIN, so convert to denormalized       */
/* triple.                                                              */
/* -------------------------------------------------------------------- */

    iMantissa = (iMantissa | 0x00800000) >> (1 - iExponent);

/* -------------------------------------------------------------------- */
/* Round to nearest value, may cause an overflow, which is handled      */
/* below as a separate case.                                            */
/* -------------------------------------------------------------------- */

    if(iMantissa & 0x00000040)
      iMantissa += 0x00000080;

/* -------------------------------------------------------------------- */
/* Assemble and return (iExponent = 0)                                  */
/* -------------------------------------------------------------------- */

    return iSign | (iMantissa >> 7);
  }
  else if( iExponent == 0xff - (127-63) )
  {
    if( iMantissa == 0 )
    {
/* -------------------------------------------------------------------- */
/* iFloat is infinity; convert to half infinity with correct sign.      */
/* -------------------------------------------------------------------- */

      return iSign | 0x7f0000;
    }
    else 
    {
/* -------------------------------------------------------------------- */
/* iFloat is NAN; convert appropriately                                 */
/* -------------------------------------------------------------------- */

      iMantissa >>= 7;
      return iSign | 0x7f0000 | iMantissa | (iMantissa == 0);
    }
  }
  else 
  {
/* -------------------------------------------------------------------- */
/* iExponent is greater than zero, iFloat is normalized, convert to     */
/* normalized triple.                                                     */
/* -------------------------------------------------------------------- */

    /* round */
    if( iMantissa & 0x00000040 )
    {
      iMantissa += 0x00000080;

      /* overflow */
      if( iMantissa & 0x00800000 )
      {
	iMantissa = 0;
	iExponent += 1;
      }
    }


    /* handle exponent overflow */
    if( iExponent > 126 )
    {
      /* force a hardware overflow */
      volatile float f = 1e10;
      int i;
      for(i = 0; i < 10; ++i)
	f *= f;
      return iSign | 0x7f0000; /* return infinity */
    }

/* -------------------------------------------------------------------- */
/* Assemble and return                                                  */
/* -------------------------------------------------------------------- */

    return iSign | (iExponent << 16) | (iMantissa >> 7);
  }
}

/************************************************************************/
/* A note about endianness:                                             */
/*                                                                      */
/* These routines assume that the data coming in is in the native byte  */
/* ordering of the machine running the code.  As such, data must be     */
/* byte swapped into the appropriate ordering before being passed in.   */
/*                                                                      */
/* For exammple, if an array of big-endian data is passed to a little-  */
/* endian machine the results of these conversion routines will be      */
/* incorrect.  The data must be reordered *before* being passed here.   */
/************************************************************************/


/************************************************************************/
/*                       convert_float_32_16()                          */
/*                                                                      */
/* Convert an array of floats into an array of halfs, this function     */
/* assumes that the half array has already been allocated and will      */
/* store the halfs as a consecutive array of 16-bit numbers.            */
/************************************************************************/

void convert_float_32_16 ( float *floats, void *halfs, unsigned int count )
{
  assert(sizeof(unsigned int) == sizeof(float));

  unsigned int i;
  unsigned short *h = halfs;
  unsigned int *f = (unsigned int*)floats;

  for( i = 0; i < count; ++i )
    *h++ = float_to_half(*f++);
}

/* Same as above, but for 24-bit floats */
void convert_float_32_24 ( float *floats, void *triples, unsigned int count )
{
  assert(sizeof(unsigned int) == sizeof(float));

  unsigned int i,j;
  unsigned int *f = (unsigned int*)floats;
  char *p = triples;

  char *t;
  unsigned int triple;

  for( i = 0; i < count; ++i )
  {
    triple = float_to_triple(*f++);
    t = (char*)&triple;

#ifdef FLOAT_CVT_BIG_ENDIAN
    t++; /* skip first byte */
#endif

    for( j = 0; j < 3; ++j )
      *p++ = *t++;
  }
}

/************************************************************************/
/*                       convert_float_16_32()                          */
/*                                                                      */
/* Convert an array of halfs into an array of floats, this function     */
/* assumes that the float array has already been allocated and will     */
/* store the floats as a consecutive array of 32-bit numbers.           */
/************************************************************************/


void convert_float_16_32 ( void *halfs, float *floats, unsigned int count )
{
  assert(sizeof(unsigned int) == sizeof(float));

  unsigned int i;
  unsigned short* h = halfs;
  unsigned int *f = (unsigned int *)floats;

  for( i = 0; i < count; ++i )
    *f++ = half_to_float(*h++);
}

/* Same as above, but for 24-bit floats */
void convert_float_24_32 ( void *triples, float *floats, unsigned int count )
{
  assert(sizeof(unsigned int) == sizeof(float));
  
  unsigned int i,j;
  unsigned int *f = (unsigned int*)floats;
  char *p = triples;

  char *t;
  unsigned int triple;

  for( i = 0; i < count; ++i )
  {
    t = (char*)&triple;

#ifdef FLOAT_CVT_BIG_ENDIAN
    t++; /* skip first 8 bits */
#endif

    for( j = 0; j < 3; ++j )
      *t++ = *p++;

    *f++ = triple_to_float(triple);
  }
}

/************************************************************************/
/*                       convert_RGB_float_32_16()                      */
/*                                                                      */
/* Convert an array of float RGBs into an array of halfs, this function */
/* assumes that the half array has already been allocated and will      */
/* store the halfs as a consecutive array of 16-bit numbers.            */
/*                                                                      */
/* Note that we cannot just cast the TT_RGBf* to an unsigned int* becase*/
/* of struct alignment issues.  We also pull the components out of the  */
/* struct by name rather than relying on the memory layout of the       */
/* struct because I'm paranoid.                                         */
/************************************************************************/

void convert_RGB_float_32_16 ( TT_RGBf *rgbs, void *halfs, unsigned int count )
{
  unsigned int i;
  TT_RGBf *c = rgbs;
  unsigned short* h = halfs;

  union {
    unsigned int packed_f;
    float f;
  } u;

  for( i = 0; i < count; ++i)
  {
    u.f = c->red;
    *h++ = float_to_half(u.packed_f);

    u.f = c->green;
    *h++ = float_to_half(u.packed_f);

    u.f = c->blue;
    *h++ = float_to_half(u.packed_f);

    c++;
  }
}

/* Same as above, but for 24-bit floats */
void convert_RGB_float_32_24 ( TT_RGBf *rgbs, void *triples,
	unsigned int count )
{
  unsigned int i,j;
  TT_RGBf *c = rgbs;
  char *p = (char *)triples;
  unsigned int triple;
  char *t;

  union {
    unsigned int packed_f;
    float f;
  } u;
  
  for( i = 0; i < count; ++i )
  {
    u.f = c->red;
    triple = float_to_triple(u.packed_f);
    t = (char *)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++;
#endif
    for( j = 0; j < 3; ++j )
      *p++ = *t++;

    /* */

    u.f = c->green;
    triple = float_to_triple(u.packed_f);
    t = (char *)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++;
#endif
    for( j = 0; j < 3; ++j )
      *p++ = *t++;

    /* */

    u.f = c->blue;
    triple = float_to_triple(u.packed_f);
    t = (char *)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++;
#endif
    for( j = 0; j < 3; ++j )
      *p++ = *t++;

    /* */

    c++;
  }
}

/************************************************************************/
/*                       convert_RGB_float_16_32()                      */
/*                                                                      */
/* Convert an array of halfs into an array of float RGBs, this function */
/* assumes that the RGB array has already been allocated and will       */
/* store the RGBs as a consecutive array of structs.                    */
/*                                                                      */
/* Note that we cannot just cast the TT_RGBf* to an unsigned int* becase*/
/* of struct alignment issues.  We also pull the components out of the  */
/* struct by name rather than relying on the memory layout of the       */
/* struct because I'm paranoid.                                         */
/************************************************************************/

void convert_RGB_float_16_32 ( void *halfs, TT_RGBf *rgbs, unsigned int count )
{
  unsigned int i;
  TT_RGBf* c = rgbs;
  unsigned short* h = halfs;
  union {
    unsigned int packed_f;
    float f;
  } u;
  
  for( i = 0; i < count; ++i )
  {
    u.packed_f = half_to_float(*h++);
    c->red = u.f;

    u.packed_f = half_to_float(*h++);
    c->green = u.f;

    u.packed_f = half_to_float(*h++);
    c->blue = u.f;

    c++;
  }
}

/* Same as above, but for 24-bit floats */
void convert_RGB_float_24_32 ( void *triples, TT_RGBf *rgbs,
	unsigned int count )
{
  unsigned int i,j;
  TT_RGBf *c = rgbs;
  char *p = (char *)triples;
  unsigned int triple;
  char *t;

  union {
    unsigned int packed_f;
    float f;
  } u;

  for( i = 0; i < count; ++i )
  {
    t = (char*)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++; /* skip first 8 bits */
#endif
    for( j = 0; j < 3; ++j )
      *t++ = *p++;
    u.packed_f = triple_to_float(triple);
    c->red = u.f;

    /* */

    t = (char*)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++; /* skip first 8 bits */
#endif
    for( j = 0; j < 3; ++j )
      *t++ = *p++;
    u.packed_f = triple_to_float(triple);
    c->green = u.f;

    /* */

    t = (char*)&triple;
#ifdef FLOAT_CVT_BIG_ENDIAN
    t++; /* skip first 8 bits */
#endif
    for( j = 0; j < 3; ++j )
      *t++ = *p++;
    u.packed_f = triple_to_float(triple);
    c->blue = u.f;

    /* */

    c++;
  }
}
