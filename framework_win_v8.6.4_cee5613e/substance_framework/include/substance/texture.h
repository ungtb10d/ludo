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
	Defines the SubstanceTexture structure. Platform dependent definition. Used
	to return resulting output textures.
*/

#ifndef _SUBSTANCE_TEXTURE_H
#define _SUBSTANCE_TEXTURE_H


/** Platform dependent definitions */
#include "platformdep.h"


#if defined(SUBSTANCE_PLATFORM_OGL3)

	/** @brief Substance engine texture (results) structure

	    OpenGL version.
		@note Only first member 'textureName' is used for texture input. Other members are discarded. */
	typedef struct SubstanceTexture_
	{
		/** @brief OpenGL texture name */
		unsigned int textureName;

		/** @brief Width of the texture base level */
		unsigned short level0Width;

		/** @brief Height of the texture base level */
		unsigned short level0Height;

		/** @brief OpenGL internal format of the texture */
		int internalFormat;

		/** @brief Depth of the mipmap pyramid: number of levels */
		unsigned char mipmapCount;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_D3D10PC)

	/** @brief Substance engine texture (results) structure

	    Direct3D10 PC version. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Texture handle */
		ID3D10Texture2D* handle;

		/** @brief Format override if the texture is typeless (only used for inputs) */
		DXGI_FORMAT format;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_D3D11PC) || \
      defined(SUBSTANCE_PLATFORM_D3D11XBOXONE)

	/** @brief Substance engine texture (results) structure

	    Direct3D11 PC/XboxOne versions. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Texture handle */
		ID3D11Texture2D* handle;

		/** @brief Format override if the texture is typeless (only used for inputs) */
		DXGI_FORMAT format;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_D3D12XBOXONE)

	/** @brief Substance engine texture (results) structure

	    Direct3D12 XboxOne version. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Resource handle */
		ID3D12Resource* handle;

		/** @brief Format override if the texture is typeless (only used for inputs) */
		DXGI_FORMAT format;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_PS4)

	/** @brief Substance engine texture (results) structure

	    PS4 GNM version. */

	/** @brief Helper structure used to keep storage-relatd information */
	typedef struct GnmStorageInfo {
		void*  mPtr;    /* Mapped pointer, visible from CPU and GPU */
		off_t  mOffset; /* Physical offset, only valid if memory allocation was NOT performed via the callback */
		size_t mLength; /* Size (in bytes), might be different from the requested size because of API constraints */
	} GnmStorageInfo;

	typedef struct SubstanceTexture_
	{
		/** @brief GNM texture description. */
		sce::Gnm::Texture gnmTexture;

		/** @brief Storage-related info, needed for memory release purposes
		           Set to NULL for texture inputs, and do not modify for regular output textures */
		GnmStorageInfo storageInfo;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_VULKAN)

	/** @brief Substance engine texture (results) structure

		Vulkan version.

		@warning Texture OUTPUT must be deleted by substanceContextTextureRelease()
		@note Only first member 'image' is used for texture input. Other members are discarded. */
	typedef struct SubstanceTexture_
	{
		/** @brief Vulkan image,
			If used as INPUT: must be ready for pixel read, pixel read ready
				again after use.
			If used as OUTPUT: ready for pixel read

			Stage: VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			Access: VK_ACCESS_SHADER_READ_BIT,
			Layout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

			@warning Handle OUTPUT must be deleted by substanceContextTextureRelease() */
		VkImage image;

		/** @brief Internal texture identifier
			Only valid if OUTPUT. Used for texture deletion by substanceContextTextureRelease() */
		uint32_t internalId;

		/** @brief Width of the texture base level */
		unsigned short level0Width;

		/** @brief Height of the texture base level */
		unsigned short level0Height;

		/** @brief Vulkan format of the texture (VkFormat) */
		int format;

		/** @brief Depth of the mipmap pyramid: number of levels */
		unsigned char mipmapCount;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_METAL)

	/** @brief Substance engine texture (results) structure

		Metal version.

		@warning Texture OUTPUT must be deleted by substanceContextTextureRelease()
		@note Only first member 'texture' is used for texture input. Other members are discarded. */
	typedef struct SubstanceTexture_
	{
		/** @brief Metal texture,
		    If used as INPUT: must be ready for pixel read, pixel read ready
		    again after use.
		    If used as OUTPUT: ready for pixel read

		    @warning Handle OUTPUT must be deleted by substanceContextTextureRelease()
		    @note id <MTLTexture> objects are __bridge cast to void* */
		void* texture;

		/** @brief Internal texture identifier
		    Only valid if OUTPUT. Used for texture deletion by substanceContextTextureRelease() */
		uint32_t internalId;

		/** @brief Width of the texture base level */
		unsigned short level0Width;

		/** @brief Height of the texture base level */
		unsigned short level0Height;

		/** @brief Metal format of the texture (MTLPixelFormat) */
		unsigned int format;

		/** @brief Depth of the mipmap pyramid: number of levels */
		unsigned char mipmapCount;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_INTERNAL)

	#include "texture_internal.h"

#elif defined(SUBSTANCE_PLATFORM_BLEND)

	/** @brief Substance engine texture (results) structure

	    Blend platform. Result in system memory. */
	typedef struct SubstanceTexture_
	{
		/** @brief Pointer to the result array

			This value is NULL if the buffer of this output has been allocated
			by the SubstanceCallbackInPlaceOutput callback. See callbacks.h for
			further information.

			If present, the buffer contains all the mipmap levels concatenated
			without any padding. If a different memory layout is required, use
			the SubstanceCallbackInPlaceOutput callback. */
		void *buffer;

		/** @brief Width of the texture base level */
		unsigned short level0Width;

		/** @brief Height of the texture base level */
		unsigned short level0Height;

		/** @brief Pixel format of the texture
			@see pixelformat.h */
		unsigned char pixelFormat;

		/** @brief Channels order in memory

			One of the SubstanceChannelsOrder enum (defined in pixelformat.h).

			The default channel order  depends on the format and on the
			Substance engine concrete implementation (CPU/GPU, GPU API, etc.).
			See the platform specific documentation. */
		unsigned char channelsOrder;

		/** @brief Depth of the mipmap pyramid: number of levels */
		unsigned char mipmapCount;

	} SubstanceTexture;

#else

	/** @todo Add other APIs texture definition */
	#error NYI

#endif



#endif /* ifndef _SUBSTANCE_TEXTURE_H */
