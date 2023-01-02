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
	Defines the SubstanceDevice structure. Platform dependent definition. Used
	to initialize the SubstanceContext structure.
*/

#ifndef _SUBSTANCE_DEVICE_H
#define _SUBSTANCE_DEVICE_H


/** Platform dependent definitions */
#include "platformdep.h"


/* Platform specific management */
#ifdef SUBSTANCE_PLATFORM_D3D10PC
	#define SUBSTANCE_DEVICE_D3D10 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D10PC */

#if defined(SUBSTANCE_PLATFORM_D3D11PC) || \
    defined(SUBSTANCE_PLATFORM_D3D11XBOXONE)
	#define SUBSTANCE_DEVICE_D3D11 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D11PC */

#if defined(SUBSTANCE_PLATFORM_D3D12XBOXONE)
	#define SUBSTANCE_DEVICE_D3D12 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D12XBOXONE */

#if defined(SUBSTANCE_PLATFORM_OGL3)
	#define SUBSTANCE_DEVICE_NULL 1
#endif

#ifdef SUBSTANCE_PLATFORM_PS4
	#define SUBSTANCE_DEVICE_GNM 1
#endif /* ifdef SUBSTANCE_PLATFORM_PS4 */

#ifdef SUBSTANCE_PLATFORM_VULKAN
#define SUBSTANCE_DEVICE_VULKAN 1
#endif /* ifdef SUBSTANCE_PLATFORM_VULKAN */

#ifdef SUBSTANCE_PLATFORM_METAL
#define SUBSTANCE_DEVICE_METAL 1
#endif /* ifdef SUBSTANCE_PLATFORM_METAL */


#ifdef SUBSTANCE_PLATFORM_BLEND
	#define SUBSTANCE_DEVICE_NULL 1
#endif /* ifdef SUBSTANCE_PLATFORM_RAWMEMORYOUTPUT */

#if __cplusplus >= 201103L
#define SUBSTANCE_ZERO_INIT(x) x = 0
#else
#define SUBSTANCE_ZERO_INIT(x) x
#endif

#if defined(SUBSTANCE_DEVICE_D3D10)

	/** @brief Substance engine device structure

	    Direct3D10 PC. Must be correctly filled when used to initialize the
		SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		ID3D10Device *handle;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_D3D11)

	/** @brief Substance engine device structure

	    Direct3D11 PC. Must be correctly filled when used to initialize the
		SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		#ifdef SUBSTANCE_PLATFORM_D3D11XBOXONE
		ID3D11DeviceX *handle;
		#else
		ID3D11Device *handle;
		#endif

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_D3D12)

	/** @brief Substance engine device structure

	    Direct3D12. Must be correctly filled when used to initialize the
	    SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief Direct3D Device handle */
		ID3D12Device *handle;

		/** @brief Direct3D Command Queue */
		ID3D12CommandQueue* commandQueue;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_GNM)

	/** @brief Substance engine device structure

	    PS4 GNM version. Must be correctly filled when used to initialize the
	    SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		/** @brief GNM Context data structure pointer

		    This structure is used for managing and controlling
		    the command buffer. */
		sce::Gnmx::LightweightGfxContext* gnmContext;

	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_VULKAN)

	/** @brief Substance engine device structure

	    Vulkan. Must be correctly filled when used to initialize the
	    SubstanceContext structure. */
	typedef struct SubstanceDevice_
	{
		void* vkGetInstanceProcAddr;       //!< NULL, or the address of an already-loaded
		                                   //   vkGetInstanceProcAddr function
		uint32_t apiVersion;               //!< Vulkan instance version
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		void* metalDevice;                 //!< MoltenVK
		VkQueue graphicsQueue;
		VkQueue computeQueue;
		VkQueue transferQueue;
		VkPipelineCache pipelineCache;
	} SubstanceDevice;

#elif defined(SUBSTANCE_DEVICE_METAL)

	/** @brief Substance engine device structure

	    Metal. Must be correctly filled when used to initialize the
	    SubstanceContext structure. */

	typedef struct SubstanceDevice_
	{
		// This should be filled with the handle of an id <MTLDevice> object from Obj-C side
		// For instance:
		//    id <MTLDevice> mtlDevice = MTLCreateSystemDefaultDevice();
		//    substanceDevice.mtlDevice = (__bridge void*) mtlDevice;
		void* mtlDevice;

		// Likewise for the MTLCommandQueue to use
		void* mtlCommandQueue;
	} SubstanceDevice;

#elif defined(SUBSTANCE_PLATFORM_INTERNAL)

	#include "device_internal.h"

#elif defined(SUBSTANCE_DEVICE_NULL)

	/** @brief Substance engine device structure

	    Generic version. Dummy structure. No device-like objects are necessary.
		The Substance context can be built without any device pointer. */
	typedef struct SubstanceDevice_
	{
		/** @brief Dummy value (avoid 0 sized structure).
		    @note  Kept for historical reasons */
		int dummy_;

		/** @brief Index of the GPU to use (Windows/D3D and Vulkan only)
		    @note  For D3D backends the GPUs are considered in the order used by
		           the IDXGIFactory::EnumAdapters() function.
		           For Vulkan backend the GPUs are considered in the order used
		           by the vkEnumeratePhysicalDevices() function. */
		SUBSTANCE_ZERO_INIT(unsigned int gpuIndex);

	} SubstanceDevice;

#else

	/** @todo Specify device structures for other APIs */
	#error NYI

#endif



#endif /* ifndef _SUBSTANCE_DEVICE_H */
