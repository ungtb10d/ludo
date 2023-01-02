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
	Defines the SubstanceLinkerOutputCreate structure: used as parameter of the
	substanceLinkerHandleCreateOutputEx function (handle.h).<BR>
*/

#ifndef _SUBSTANCE_LINKER_OUTPUTCREATE_H
#define _SUBSTANCE_LINKER_OUTPUTCREATE_H


/** Create output structures definition */
#include "outputshuffle.h"


/** @brief Horizontal and vertical flip enumeration
	Used by SubstanceLinkerOutputCreate */
typedef enum
{

	Substance_Linker_Flip_No         = 0x0, /**< No fliping */
	Substance_Linker_Flip_Horizontal = 0x1, /**< Horizontal flip */
	Substance_Linker_Flip_Vertical   = 0x2, /**< Vertical flip */
	Substance_Linker_Flip_Both       = 0x3  /**< Horizontal and Vertical flip */

} SubstanceLinkerHVFlip;


/** @brief Output creation options description
 
	Used by substanceLinkerHandleCreateOutputEx function (handle.h). */
typedef struct
{
	/** @brief Pixel format of the created output (or -1: same as source)
		Combination of SubstancePixelFormat enums (see substance/pixelformat.h
		for the complete list). Set to -1 to use format of the source output 
		'outputUID' */
	int format;
	
	/** @brief Mipmap pyramid depth of the created output (or -1: same as source)
		 * -1: Mipmap pyramid depth of output 'outputUID'
		 * 0: full mipmap pyramid 
		 * 1: no mipmaps */
	int mipmapLevelsCount;
		
	/** @brief Horizontal and vertical flipping: one of SubstanceLinkerHVFlip
		enumeration */
	SubstanceLinkerHVFlip hvFlip;
	
	/** @brief Width override of the output (set to -1 for default) 
		The upper power of 2 value is actually used. Either both width & height
		should be overriden or none. */
	int forceWidth;
		
	/** @brief Height override of the output (set to -1 for default) 
		The upper power of 2 value is actually used. Either both height & width 
		should be overriden or none. */
	int forceHeight;
	
	/** @brief Enable(!=0) or Disable (0) channel shuffing 
		If disabled 'shuffle' member is ignored and the UID of the Output to 
		use as source is given by 'outputUID' member. */
	unsigned int useShuffling;
	
	/** @brief UID of the Output to use as source (if shuffling is disabled)
		@warning The assembly that contains this output UID must be already 
			pushed into the handle. 
			
		Only used if 'useShuffling' flag is disabled or for fetching default 
		values of 'format' and/or 'mipmapLevelsCount'. */
	unsigned int outputUID;
	
	/** @brief Description of new output channels make-up (see outputshuffle.h)
		
		Shuffling is actually used only if 'useShuffling' flag is enabled, 
		otherwise this member is ignored (the output source is determined by
		'outputUID' member). */
	SubstanceLinkerShuffle shuffle;

} SubstanceLinkerOutputCreate;


#endif /* ifndef _SUBSTANCE_LINKER_OUTPUTCREATE_H */
