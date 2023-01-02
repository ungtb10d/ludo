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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_DDSSAVE_H
#define _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_DDSSAVE_H

// Substance include
#if !defined(SUBSTANCE_PLATFORM_BLEND)
#	define SUBSTANCE_PLATFORM_BLEND
#endif
#include <substance/texture.h>


namespace Framework
{
namespace Test
{

//! @brief Write a DDS file from Substance texture
//! @param filename Target filename
//! @param resultTexture The texture to save in DDS
void writeDDSFile(
	const char* filename,
	const SubstanceTexture &resultTexture);
	
} // namespace Test
} // namespace Framework


#endif /* ifndef _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_DDSSAVE_H */
