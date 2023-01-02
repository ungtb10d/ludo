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

#include <substance/framework/channel.h>

#include <assert.h>


// The order of the strings in this array must match the order of the ChannelUse enum values
static const char* const gChannelNames[] = {
	"diffuse",
	"baseColor",
	"opacity",
	"emissive",
	"emissiveIntensity",
	"ambient",
	"ambientOcclusion",
	"mask",
	"normal",
	"bump",
	"height",
	"heightScale",
	"heightLevel",
	"displacement",
	"specular",
	"specularLevel",
	"specularEdgeColor",
	"specularColor",
	"glossiness",
	"roughness",
	"anisotropyLevel",
	"anisotropyAngle",
	"transmissive",
	"reflection",
	"refraction",
	"coatWeight",
	"coatOpacity",
	"coatColor",
	"coatRoughness",
	"coatIOR",
	"coatSpecularLevel",
	"coatNormal",
	"environment",
	"IOR",
	"dispersion",
	"translucency",
	"absorptionColor",
	"absorptionDistance",
	"scattering",
	"scatteringColor",
	"scatteringDistance",
	"scatteringDistanceScale",
	"scatteringRedShift",
	"scatteringRayleigh",
	"scattering0",
	"scattering1",
	"scattering2",
	"scattering3",
	"any",
	"metallic",
	"sheenOpacity",
	"sheenColor",
	"sheenRoughness",
	"panorama",
	"physicalSize",
	"UNKNOWN"
};



// The order of the strings in this array must match the order of the ColorSpace enum values
static const char* const gColorSpaceNames[] = {
	"Linear",
	"sRGB",
	"Passthru",
	"SNorm",
	"NormalXYZRight",
	"NormalXYZLeft",
	"UNKNOWN"
};


//! @brief Helper: get channel names corresponding to ChannelUse
//! @return Return array of Channel_INTERNAL_COUNT strings
const char*const* SubstanceAir::getChannelNames()
{
	assert(Channel_INTERNAL_COUNT == (sizeof(gChannelNames) / sizeof(const char*)));
	return gChannelNames;
}


//! @brief Helper: get colorspace names corresponding to ColorSpace
//! @return Return array of ColorSpace_INTERNAL_COUNT strings
const char* const* SubstanceAir::getColorSpaceNames()
{
	assert(ColorSpace_INTERNAL_COUNT == (sizeof(gColorSpaceNames) / sizeof(const char*)));
	return gColorSpaceNames;
}


//! @brief Choose color space automatically for the given channel and format.
//! @param channel the ChannelUse enum
//! @param isFloatFormat must be false for 8 or 16 bits integer formats, true otherwise.
//!
//!  * Diffuse, Emissive, Specular
//!       -> Integer formats : sRGB
//!       -> Float formats   : Linear
//!  * Height, Displacement
//!       -> Integer formats : SNorm
//!       -> Float formats   : Passthru
//!  * Normal
//!       -> All formats     : NormalXYZLeft
//!  * <Others>
//!       -> All formats     : Passthru
SubstanceAir::ColorSpace SubstanceAir::getDefaultColorSpace(ChannelUse channel, bool isFloatFormat)
{
	switch (channel)
	{
	case Channel_BaseColor:
	case Channel_Emissive:
	case Channel_Specular:
	case Channel_Transmissive:
	case Channel_Diffuse:
	case Channel_CoatColor:
		return isFloatFormat ? ColorSpace_Linear : ColorSpace_sRGB;

	case Channel_Height:
	case Channel_Displacement:
		return isFloatFormat ? ColorSpace_Passthru : ColorSpace_SNorm;

	case Channel_Normal:
	case Channel_CoatNormal:
		return ColorSpace_NormalXYZLeft;

	case Channel_SpecularLevel:
	default:
		return ColorSpace_Passthru;
	}
}