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

/*
	Defines the SubstanceTextureDesc structure. Contains information about the 
	format, size, etc.
*/

#ifndef _SUBSTANCE_TEXTUREDESC_H
#define _SUBSTANCE_TEXTUREDESC_H


/** Pixel formats definitions include */
#include "pixelformat.h"


/** @brief Substance engine texture description structure */
typedef struct SubstanceTextureDesc_
{
	/** @brief Width of the texture */
	unsigned int level0Width;
	
	/** @brief Height of the texture */
	unsigned int level0Height;
	
	/** @brief Pixel format of the texture
		@see pixelformat.h */
	unsigned char pixelFormat;
	
	/** @brief Channels order in memory

		One of the SubstanceChannelsOrder enum (defined in pixelformat.h).
		Set to Substance_ChanOrder_NC if not RGBA format or not relevant for the
		current platform.

		The default channel order depends on the format and on the 
		Substance engine concrete implementation (CPU/GPU, GPU API, etc.).
		See the platform specific documentation. */
	unsigned char channelsOrder;
	
	/** @brief Depth of the mipmaps (0: full pyramid, 1: only level0, etc.) */
	unsigned char mipmapCount;
	
} SubstanceTextureDesc;	


#endif /* ifndef _SUBSTANCE_TEXTUREDESC_H */
