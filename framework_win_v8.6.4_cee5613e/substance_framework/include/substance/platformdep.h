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
    Defines platform specific macros. One of these macros must already be
    defined (preprocessor #define):
      - SUBSTANCE_PLATFORM_BLEND: generic platform, results in system memory
      - SUBSTANCE_PLATFORM_D3D10PC: PC Direct3D10 API
      - SUBSTANCE_PLATFORM_D3D11PC: PC Direct3D11 API
      - SUBSTANCE_PLATFORM_OGL3: OpenGL 3 API
      - SUBSTANCE_PLATFORM_PS4: PS4 GNM API
      - SUBSTANCE_PLATFORM_VULKAN: Vulkan API
      - SUBSTANCE_PLATFORM_METAL: Metal API
      - SUBSTANCE_PLATFORM_INTERNAL: INTERNAL API
      etc.
*/

#ifndef _SUBSTANCE_PLATFORMDEP_H
#define _SUBSTANCE_PLATFORMDEP_H

/** Base definitions */
#include "defines.h"

/** Engine ID definitions */
#include "engineid.h"


/* Platform specific headers */

#if defined(SUBSTANCE_PLATFORM_D3D10PC)

	/* PC Direct3D10 API */
	#include <d3d10.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_d3d10pc

#elif defined(SUBSTANCE_PLATFORM_D3D11PC)

	/* PC Direct3D11 API */
	#include <d3d11.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_d3d11pc

#elif defined(SUBSTANCE_PLATFORM_D3D11XBOXONE)

	/* Xbox One Direct3D11 API */
	#include <d3d11_x.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_xb1d3d11

#elif defined(SUBSTANCE_PLATFORM_D3D12XBOXONE)

	/* Xbox One Direct3D12 API */
	#include <d3d12_x.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_xb1d3d12

#elif defined(SUBSTANCE_PLATFORM_OGL3)

	/* OpenGL 3.x GLSL platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_ogl3

#elif defined(SUBSTANCE_PLATFORM_PS4)

	/* PS4 GNM platform */
	#include <gnm.h>
	#include <gnmx.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_ps4gnm

#elif defined(SUBSTANCE_PLATFORM_VULKAN)

	/* Vulkan platform */
	#include <stdint.h>

	#ifndef VULKAN_H_
		#define ALG_VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
		ALG_VK_DEFINE_HANDLE(VkImage)
		ALG_VK_DEFINE_HANDLE(VkInstance)
		ALG_VK_DEFINE_HANDLE(VkPhysicalDevice)
		ALG_VK_DEFINE_HANDLE(VkDevice)
		ALG_VK_DEFINE_HANDLE(VkQueue)
		ALG_VK_DEFINE_HANDLE(VkBuffer)
		ALG_VK_DEFINE_HANDLE(VkPipelineCache)
	#endif

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_vulkan

#elif defined(SUBSTANCE_PLATFORM_METAL)

	/* Metal platform */
	#include <stdint.h>

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_metal

#elif defined(SUBSTANCE_PLATFORM_INTERNAL)
	
	#include "platformdep_internal.h"

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_INTERNAL

#elif defined(SUBSTANCE_PLATFORM_BLEND)

	/* Blend platform */
	/* No header needed */

	#define SUBSTANCE_API_PLATFORM Substance_EngineID_BLEND

#else

	/* platform not supported / define not set */
	#error Platform NYI or proper SUBSTANCE_PLATFORM_xxx preprocessor macro not set

#endif


#endif /* ifndef _SUBSTANCE_PLATFORMDEP_H */
