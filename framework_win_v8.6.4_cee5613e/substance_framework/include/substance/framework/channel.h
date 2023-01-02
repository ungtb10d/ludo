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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_CHANNEL_H
#define _SUBSTANCE_AIR_FRAMEWORK_CHANNEL_H

#include "typedefs.h"

namespace SubstanceAir
{

//! @brief The list of channels roles available in Designer
enum ChannelUse
{
	Channel_Diffuse,
	Channel_BaseColor,
	Channel_Opacity,
	Channel_Emissive,
	Channel_EmissiveIntensity,
	Channel_Ambient,
	Channel_AmbientOcclusion,
	Channel_Mask,
	Channel_Normal,
	Channel_Bump,
	Channel_Height,
	Channel_HeightScale,
	Channel_HeightLevel,
	Channel_Displacement,
	Channel_Specular,
	Channel_SpecularLevel,
	Channel_SpecularEdgeColor,
	Channel_SpecularColor,
	Channel_Glossiness,
	Channel_Roughness,
	Channel_AnisotropyLevel,
	Channel_AnisotropyAngle,
	Channel_Transmissive,
	Channel_Reflection,
	Channel_Refraction,
	Channel_CoatWeight,
	Channel_CoatOpacity,
	Channel_CoatColor,
	Channel_CoatRoughness,
	Channel_CoatIOR,
	Channel_CoatSpecularLevel,
	Channel_CoatNormal,
	Channel_Environment,
	Channel_IOR,
	Channel_Dispersion,
	Channel_Translucency,
	Channel_AbsorptionColor,
	Channel_AbsorptionDistance,
	Channel_Scattering,
	Channel_ScatteringColor,
	Channel_ScatteringDistance,
	Channel_ScatteringDistanceScale,
	Channel_ScatteringRedShift,
	Channel_ScatteringRayleigh,
	/* The SCATTERINGx channels below are deprecated and replaced by Scattering */
	Channel_SCATTERING0,
	Channel_SCATTERING1,
	Channel_SCATTERING2,
	Channel_SCATTERING3,
	Channel_Any,
	Channel_Metallic,
	Channel_SheenOpacity,
	Channel_SheenColor,
	Channel_SheenRoughness,
	Channel_Panorama,
	Channel_PhysicalSize,
	Channel_UNKNOWN,
	Channel_INTERNAL_COUNT
};


//! @brief List of components to match with a particular channel usage
enum ChannelComponent
{
	ChannelComponent_RGBA,
	ChannelComponent_RGB,
	ChannelComponent_R,
	ChannelComponent_G,
	ChannelComponent_B,
	ChannelComponent_A,
	ChannelComponent_INTERNAL_COUNT
};


//! @brief Enumeration of color space interpretation
//! Used as argument of each external resources output
enum ColorSpace
{
	//! @brief Interpreted as Linear color space image
	//! @note Equivalent to Passthru, interpreted as-is.
	ColorSpace_Linear,

	//! @brief Interpreted as sRGB color space image
	//! @warning L8 format stored as sRGB8, sL8 is not supported.
	ColorSpace_sRGB,

	//! @brief Interpreted as raw values, NOT color/luminance space.
	//! Passthru of normalized/float values.
	ColorSpace_Passthru,

	//! @brief Interpreted as signed normalized raw values.
	//! Signed -1..1 values stored as 0..1 normalized values.
	ColorSpace_SNorm,

	//! @brief Interpreted as XYZ Direct normal stored in RGB channel.
	//! Right handed syscoord: OpenGL
	ColorSpace_NormalXYZRight,

	//! @brief Interpreted as XYZ Indirect normal stored in RGB channel.
	//! Left handed syscoord: Direct3D
	ColorSpace_NormalXYZLeft,

	//! @brief Used when the color space cannot be parsed
	ColorSpace_UNKNOWN,

	ColorSpace_COUNT,
	ColorSpace_INTERNAL_COUNT = ColorSpace_COUNT
};


//! @brief Complete Channel usage description
//! Accessible for inputs and outputs
struct ChannelFullDesc
{
	//! @brief Describes which particular components hold the information from this channel usage
	ChannelComponent mComponent;

	//! @brief Channel usage in ChannelUse form
	ChannelUse mUsage;

	//! @brief Channel usage in string form
	//! To be used when mUsage is Channel_UNKNOWN
	string mUsageStr;

	//! @brief Colorspace in which the data should be interpreted, in ColorSpace form
	ColorSpace mColorSpace;

	//! @brief Colorspace in which the data should be interpreted, in string form
	//! To be used then mColorSpace is ColorSpace_UNKNOWN
	string mColorSpaceStr;
};


//! @brief Helper: get channel names corresponding to ChannelUse
//! @return Return array of Channel_INTERNAL_COUNT strings
const char* const* getChannelNames();

//! @brief Helper: get colorspace names corresponding to ColorSpace
//! @return Return array of ColorSpace_INTERNAL_COUNT strings
const char* const* getColorSpaceNames();


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
ColorSpace getDefaultColorSpace(ChannelUse channel, bool isFloatFormat);

} // namespace SubstanceAir

#endif //_SUBSTANCE_AIR_FRAMEWORK_CHANNEL_H
