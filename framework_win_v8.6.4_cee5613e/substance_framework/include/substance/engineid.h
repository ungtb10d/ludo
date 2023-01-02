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
	Used by Substance Linker to generate dedicated binaries and by engines
	version structure (see version.h).
*/

#ifndef _SUBSTANCE_ENGINEID_H
#define _SUBSTANCE_ENGINEID_H


/** Basic type defines */
#include <stddef.h>


/** @brief Substance engine ID enumeration */
typedef enum
{
	Substance_EngineID_INVALID      = 0x0,    /**< Unknown engine, do not use */
	Substance_EngineID_BLEND        = 0x0,    /**< BLEND platform API */

	Substance_EngineID_ogl3         = 0xC,    /**< OpenGL3 */
	Substance_EngineID_ogl3m1       = 0xF,    /**< OpenGL3 for Apple Silicon Macs */
	Substance_EngineID_d3d10pc      = 0x7,    /**< Direct3D 10 Windows */
	Substance_EngineID_d3d11pc      = 0xB,    /**< Direct3D 11 Windows */

	Substance_EngineID_xb1d3d11     = 0xD,    /**< Xbox One GPU (Direct3D 11) engine */
	Substance_EngineID_xb1d3d12     = 0x1E,   /**< Xbox One GPU (Direct3D 12) engine */
	/* Deprecated EngineID, use xb1d3d11 instead */
	Substance_EngineID_d3d11xb1     = Substance_EngineID_xb1d3d11,

	Substance_EngineID_ps4gnm       = 0xE,    /**< PS4 GPU (GNM) engine */

	Substance_EngineID_stdc         = 0x10,   /**< Pure Software (at least) engine */
	Substance_EngineID_sse2         = 0x13,   /**< SSE2 (minimum) CPU engine */
	Substance_EngineID_neon         = 0x1B,   /**< NEON CPU engine */
	Substance_EngineID_neonm1       = 0x16,   /**< NEON CPU engine for Apple Silicon Macs */

	Substance_EngineID_xb1avx       = 0x1C,   /**< Xbox One CPU engine (AVX) */
	/* Deprecated EngineID, use xb1avx instead */
	Substance_EngineID_xb1          = Substance_EngineID_xb1avx,
	Substance_EngineID_ps4avx       = 0x1D,   /**< PS4 CPU engine (AVX) */

	Substance_EngineID_vk           = 0x12,   /**< Vulkan */
	Substance_EngineID_vkm1         = 0x17,   /**< Vulkan for Apple Silicon Macs */
	Substance_EngineID_int_vk       = 0x1F,   /**< Vulkan / Internal platform API */
	Substance_EngineID_int_vkm1     = 0x01,   /**< Vulkan / Internal platform API for Apple Silicon Macs */

	Substance_EngineID_mtl          = 0x02,   /**< Metal */
	Substance_EngineID_mtlm1        = 0x03,   /**< Metal for Apple Silicon Macs */
	Substance_EngineID_int_mtl      = 0x04,   /**< Metal / Internal platform API */
	Substance_EngineID_int_mtlm1    = 0x05,   /**< Metal / Internal platform API for Apple Silicon Macs */

	Substance_EngineID_vulkan       = 0x14,   /**< Vulkan platform API */
	Substance_EngineID_metal        = 0x18,   /**< Metal platform API */
	Substance_EngineID_INTERNAL     = 0x15,   /**< Internal platform API */

	/* etc. */

	Substance_EngineID_FORCEDWORD   = 0xFFFFFFFF /**< Force DWORD enumeration */

} SubstanceEngineIDEnum;


#endif /* ifndef _SUBSTANCE_ENGINEID_H */
