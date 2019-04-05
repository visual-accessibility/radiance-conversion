#ifndef	__DeVAS_SRGB_RAD
#define	__DeVAS_SRGB_RAD

/* sRGB primaries */
#define	CIE_x_r_sRGB		0.64
#define	CIE_y_r_sRGB		0.33
#define	CIE_x_g_sRGB		0.30
#define	CIE_y_g_sRGB		0.60
#define	CIE_x_b_sRGB		0.15
#define	CIE_y_b_sRGB		0.06
#define	CIE_x_w_sRGB		0.3127
#define	CIE_y_w_sRGB		0.3290

#define	sRGBPRIMS	{					\
    			    { CIE_x_r_sRGB, CIE_y_r_sRGB },	\
			    { CIE_x_g_sRGB, CIE_y_g_sRGB },	\
			    { CIE_x_b_sRGB, CIE_y_b_sRGB },	\
			    { CIE_x_w_sRGB, CIE_y_w_sRGB }	\
			}

#endif	/* __DeVAS_SRGB_RAD */
