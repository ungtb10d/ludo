/*************************************************************************
* ADOBE CONFIDENTIAL
* ___________________ *
*  Copyright 2020-2022 Adobe
*  All Rights Reserved.
* * NOTICE:  All information contained herein is, and remains
* the property of Adobe and its suppliers, if any. The intellectual
* and technical concepts contained herein are proprietary to Adobe
* and its suppliers and are protected by all applicable intellectual
* property laws, including trade secret and copyright laws.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe.
**************************************************************************/

//! Allows to save on the disk in DDS format (RGBA and/or DXT).

#include "frameworktestddssave.h"

#include <substance/pixelformat.h>

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


/** @brief DDS formats enumeration */
typedef enum
{
	ALG_IMGIO_DDSFMT_UNKNOWN  = 0,

	ALG_IMGIO_DDSFMT_ABGR = 0x1,          /**< 8bits/ch, 4 channels */
	ALG_IMGIO_DDSFMT_ARGB = 0x3,          /**< 8bits/ch, 4 channels */
	ALG_IMGIO_DDSFMT_XBGR = 0x0,          /**< 8bits/ch, 4 channels */
	ALG_IMGIO_DDSFMT_XRGB = 0x2,          /**< 8bits/ch, 4 channels */
	ALG_IMGIO_DDSFMT_BGR = 0x80,          /**< 8bits/ch, 3 channels */
	ALG_IMGIO_DDSFMT_RGB = 0x82,          /**< 8bits/ch, 3 channels */
	ALG_IMGIO_DDSFMT_L    = 0x4,          /**< 8bits/ch, 1 channel */
	ALG_IMGIO_DDSFMT_RGBA16 = 0x11,       /**< 16bits/ch, 4 channels */
	ALG_IMGIO_DDSFMT_L16  = 0x14,         /**< 16bits/ch, 1 channel */
	ALG_IMGIO_DDSFMT_RGBA16F = 0x51,      /**< 16bits/ch, 4 channel, float */
	ALG_IMGIO_DDSFMT_L16F = 0x54,         /**< 16bits/ch, 1 channel, float */
	ALG_IMGIO_DDSFMT_RGBA32F = 0x61,      /**< 32bits/ch, 4 channel, float */
	ALG_IMGIO_DDSFMT_L32F = 0x64,         /**< 16bits/ch, 1 channel, float */
	ALG_IMGIO_DDSFMT_DXT1 = 0x008,        /**< DXT1/BC1 compressed */
	ALG_IMGIO_DDSFMT_DXT2 = 0x108,        /**< DXT2/BC2 compressed */
	ALG_IMGIO_DDSFMT_DXT3 = 0x508,        /**< DXT3/BC2 compressed */
	ALG_IMGIO_DDSFMT_DXT4 = 0x308,        /**< DXT4/BC3 compressed */
	ALG_IMGIO_DDSFMT_DXT5 = 0x708,        /**< DXT4/BC3 compressed */
	ALG_IMGIO_DDSFMT_ATI1 = 0x808,        /**< ATI1/BC4 compressed */
	ALG_IMGIO_DDSFMT_ATI2 = 0x908         /**< ATI2/BC5 compressed */
} AlgImgIoDDSFormat;

/** @brief Size of the DDS header */
#define ALG_IMGIO_DDSHEADERSIZE 128


/** @brief Fill a DDS header from format/size informations
	@param width The level 0 (base) width
	@param height The level 0 (base) height
	@param pitch Pitch of the level 0 (size of a line of BLOCKS for compress
		formats). Cannot be ==0.
	@param mips Number of mips (1: no mips). Cannot be ==0.
	@param format One of the AlgImgIoDDSFormat enum
	@param header Pointer on a array of ALG_IMGIO_DDSHEADERSIZE size that will
		receive the result */
static void algImgIoDDSFillHeader(
	unsigned int width,
	unsigned int height,
	size_t pitch,
	unsigned int mips,
	AlgImgIoDDSFormat format,
	unsigned char *header);


//! @brief Write a DDS file from Substance texture
//! @param filename Target filename
//! @param resultTexture The texture to save in DDS
void Framework::Test::writeDDSFile(
	const char* filename,
	const SubstanceTexture &resultTexture)
{
	FILE *f;
	unsigned int width,height,blockw,blockh,k;
	unsigned char ddsheader[ALG_IMGIO_DDSHEADERSIZE];
	int ddsformat = ALG_IMGIO_DDSFMT_XBGR;
	size_t ddsblocksize = 4, ddspitch = 0, ddssize = 0;
	unsigned int mips = resultTexture.mipmapCount, byteswap = 0;

	blockw=width=resultTexture.level0Width;
	blockh=height=resultTexture.level0Height;

	f = fopen(filename, "wb");

	/* Find format */
	switch (resultTexture.pixelFormat&~Substance_PF_sRGB)
	{
		case Substance_PF_RGBA|Substance_PF_16I:
			ddsblocksize = 8;
			ddsformat = ALG_IMGIO_DDSFMT_RGBA16;
		break;

		case Substance_PF_L|Substance_PF_16I:
			ddsblocksize = 2;
			ddsformat = ALG_IMGIO_DDSFMT_L16;
		break;

		case Substance_PF_RGBA|Substance_PF_16F:
			ddsblocksize = 8;
			ddsformat = ALG_IMGIO_DDSFMT_RGBA16F;
		break;

		case Substance_PF_L|Substance_PF_16F:
			ddsblocksize = 2;
			ddsformat = ALG_IMGIO_DDSFMT_L16F;
		break;

		case Substance_PF_RGBA|Substance_PF_32F:
			ddsblocksize = 16;
			ddsformat = ALG_IMGIO_DDSFMT_RGBA32F;
		break;

		case Substance_PF_L|Substance_PF_32F:
			ddsblocksize = 4;
			ddsformat = ALG_IMGIO_DDSFMT_L32F;
		break;

		case Substance_PF_RGBA:
			ddsformat = 0x1;
		case Substance_PF_RGBx:
		{
			switch (resultTexture.channelsOrder)
			{
				case Substance_ChanOrder_ABGR: byteswap = 4;
				case Substance_ChanOrder_RGBA: break;
				case Substance_ChanOrder_ARGB: byteswap = 4;
				case Substance_ChanOrder_BGRA: ddsformat |= 0x2; break;
				
				default:
				{
					assert(0);  // Non supported channel order
				}
				return;
			}
		}
		break;
		
		case Substance_PF_L:
			ddsblocksize = 1;
			ddsformat = ALG_IMGIO_DDSFMT_L;
		break;

		case Substance_PF_BC1:  ddsformat = ALG_IMGIO_DDSFMT_DXT1; break;
		case Substance_PF_DXT2: ddsformat = ALG_IMGIO_DDSFMT_DXT2; break;
		case Substance_PF_BC2:  ddsformat = ALG_IMGIO_DDSFMT_DXT3; break;
		case Substance_PF_DXT4: ddsformat = ALG_IMGIO_DDSFMT_DXT4; break;
		case Substance_PF_BC3:  ddsformat = ALG_IMGIO_DDSFMT_DXT5; break;
		case Substance_PF_BC4:  ddsformat = ALG_IMGIO_DDSFMT_ATI1; break;
		case Substance_PF_BC5:  ddsformat = ALG_IMGIO_DDSFMT_ATI2; break;

		default:
		{
			assert(0);  // Non supported pixel format
		}
		return;
	}

	/* TC formats */
	if ((resultTexture.pixelFormat&Substance_PF_MASK_RAWFormat)==Substance_PF_BC)
	{
		ddsblocksize = ddsformat&0x100 ? 16 : 8;
		ddspitch = ddsblocksize * (blockw=std::max<unsigned int>(1,width/4));
		ddssize = ddspitch * (blockh=std::max<unsigned int>(1,height/4));
	}
	else
	{
		ddspitch = ddsblocksize * width;
		ddssize = ddspitch * height;
	}

	/* Compute mips size */
	for (k=1;k<mips;++k)
	{
		ddssize += ddsblocksize * (std::max<unsigned int>(1,(blockw>>k))) *
			(std::max<unsigned int>(1,(blockh>>k)));
	}

	/* Save DDS file */
	algImgIoDDSFillHeader(
		width,
		height,
		ddspitch,
		mips,
		(AlgImgIoDDSFormat)ddsformat,
		ddsheader);
		
	fwrite((const char*)ddsheader,ALG_IMGIO_DDSHEADERSIZE,1,f);

	if (byteswap>1)
	{
		const unsigned char *ptr = (const unsigned char*)resultTexture.buffer;
		const unsigned char * const ptrend = ptr+ddssize;
		for (;ptr<ptrend;ptr+=byteswap)
		{
			size_t k;
			for (k=0;k<byteswap;++k)
			{
				fwrite(ptr+byteswap-1-k,1,1,f);
			}
		}
	}
	else
	{
		fwrite(resultTexture.buffer,ddssize,1,f);
	}

	fclose(f);
}


#define ALG_IMGIO_DDS_WRITEUINT32(array,startindex,val) \
	(header[(startindex)+3]=(unsigned char)((val)>>24), \
	header[(startindex)+2]=(unsigned char)(((val)>>16)&0xFF), \
	header[(startindex)+1]=(unsigned char)(((val)>>8)&0xFF), \
	header[(startindex)]=(unsigned char)((val)&0xFF))



/** @brief Fill a DDS header from format/size informations
	@param width The level 0 (base) width
	@param height The level 0 (base) height
	@param pitch Pitch of the level 0 (size of a line of BLOCKS for compress
		formats). Cannot be ==0.
	@param mips Number of mips (1: no mips). Cannot be ==0.
	@param format One of the AlgImgIoDDSFormat enum
	@param header Pointer on a array of ALG_IMGIO_DDSHEADERSIZE size that will
		receive the result */
static void algImgIoDDSFillHeader(
	unsigned int width,
	unsigned int height,
	size_t pitch,
	unsigned int mips,
	AlgImgIoDDSFormat format,
	unsigned char *header)
{
	unsigned int v;
	unsigned int israw = (format&0x8)==0;

	header[0]='D';
	header[1]='D';
	header[2]='S';
	header[3]=' ';                                /* DDS tag */

	ALG_IMGIO_DDS_WRITEUINT32(header,4,0x7c);     /* Header size */

	v = 0x1007 |
		(israw?((format&0x40)==0 ? 0x8 : 0):0x80000) | // No pitch for FP
		(mips>1?0x20000:0);

	ALG_IMGIO_DDS_WRITEUINT32(header,8,v);        /* Flags */
	ALG_IMGIO_DDS_WRITEUINT32(header,12,height);  /* Height */
	ALG_IMGIO_DDS_WRITEUINT32(header,16,width);   /* Width */
	v = israw ? (unsigned int)pitch : ((unsigned int)pitch*std::max<unsigned int>(1,height/4));
	ALG_IMGIO_DDS_WRITEUINT32(header,20,v);       /* Pitch/LinearSize */
	ALG_IMGIO_DDS_WRITEUINT32(header,24,0);       /* :pad: */
	v = mips>1 ? mips : 0;
	ALG_IMGIO_DDS_WRITEUINT32(header,28,v);       /* Mips */

	memset(&header[32],0,128-32);                 /* Fill other fields with 0 */

	ALG_IMGIO_DDS_WRITEUINT32(header,76,0x20);    /* Pixel format size */
	switch (format)
	{
		case ALG_IMGIO_DDSFMT_ABGR:
		case ALG_IMGIO_DDSFMT_ARGB: v = 0x41; break;
		case ALG_IMGIO_DDSFMT_XBGR:
		case ALG_IMGIO_DDSFMT_XRGB: v = 0x40; break;
		case ALG_IMGIO_DDSFMT_L:
		case ALG_IMGIO_DDSFMT_L16: v = 0x20000; break;
		default: v = 0x4;
	}
	ALG_IMGIO_DDS_WRITEUINT32(header,80,v);       /* Pixel format flags */

	switch (format)
	{
		case ALG_IMGIO_DDSFMT_ABGR:
		case ALG_IMGIO_DDSFMT_ARGB:
		case ALG_IMGIO_DDSFMT_XBGR:
		case ALG_IMGIO_DDSFMT_XRGB:
		{
			/* RAW pixel format desc. */
			ALG_IMGIO_DDS_WRITEUINT32(header,88,32);  /* Number of bits */
			v = format&0x2 ? 0x00ff0000 : 0x000000ff ;
			ALG_IMGIO_DDS_WRITEUINT32(header,92,v);
			ALG_IMGIO_DDS_WRITEUINT32(header,96,0x0000ff00);
			v = format&0x2 ? 0x000000ff : 0x00ff0000 ;
			ALG_IMGIO_DDS_WRITEUINT32(header,100,v);
			v = format&0x1 ? 0xff000000 : 0 ;
			ALG_IMGIO_DDS_WRITEUINT32(header,104,v);
		}
		break;

		case ALG_IMGIO_DDSFMT_DXT1:
			header[84]='D'; header[85]='X'; header[86]='T'; header[87]='1'; break;
		case ALG_IMGIO_DDSFMT_DXT2:
			header[84]='D'; header[85]='X'; header[86]='T'; header[87]='2'; break;
		case ALG_IMGIO_DDSFMT_DXT3:
			header[84]='D'; header[85]='X'; header[86]='T'; header[87]='3'; break;
		case ALG_IMGIO_DDSFMT_DXT4:
			header[84]='D'; header[85]='X'; header[86]='T'; header[87]='4'; break;
		case ALG_IMGIO_DDSFMT_DXT5:
			header[84]='D'; header[85]='X'; header[86]='T'; header[87]='5'; break;
		case ALG_IMGIO_DDSFMT_ATI1:
			header[84]='A'; header[85]='T'; header[86]='I'; header[87]='1'; break;
		case ALG_IMGIO_DDSFMT_ATI2:
			header[84]='A'; header[85]='T'; header[86]='I'; header[87]='2'; break;

		case ALG_IMGIO_DDSFMT_RGBA16:
		{
			/* FourCC */
			header[84]=0x24;
		}
		break;

		case ALG_IMGIO_DDSFMT_L:
		case ALG_IMGIO_DDSFMT_L16:
		{
			v = format&0x10 ? 0x10 : 0x8;
			ALG_IMGIO_DDS_WRITEUINT32(header,88,v);
			v = format&0x10 ? 0xFFFF : 0xFF;
			ALG_IMGIO_DDS_WRITEUINT32(header,92,v);
		}
		break;
		case ALG_IMGIO_DDSFMT_L16F:
		{
			//four cc code for this format
			v = 0x6f;
			ALG_IMGIO_DDS_WRITEUINT32(header,84,v);
		}
		break;
		case ALG_IMGIO_DDSFMT_RGBA16F:
		{
			v = 0x71;
			ALG_IMGIO_DDS_WRITEUINT32(header,84,v);
		}
		break;
		case ALG_IMGIO_DDSFMT_L32F:
		{
			//four cc code for this format
			v = 0x72;
			ALG_IMGIO_DDS_WRITEUINT32(header,84,v);
		}
		break;
		case ALG_IMGIO_DDSFMT_RGBA32F:
		{
			v = 0x74;
			ALG_IMGIO_DDS_WRITEUINT32(header,84,v);
		}
		break;
		default:
			assert(0); // Unsupported pixel format
	};

	v = 0x1000 | (mips>1?0x400008:0);
	ALG_IMGIO_DDS_WRITEUINT32(header,108,v);      /* Caps */
}

