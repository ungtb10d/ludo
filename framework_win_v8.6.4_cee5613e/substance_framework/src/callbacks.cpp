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

#include <substance/framework/callbacks.h>
#include <substance/framework/inputimage.h>
#include <substance/framework/input.h>

namespace SubstanceAir
{
	static GlobalCallbacks gDefaultGlobalCallbackInstance;
}

//! @brief Current User global callbacks instance
SubstanceAir::GlobalCallbacks* SubstanceAir::GlobalCallbacks::mInstance = &SubstanceAir::gDefaultGlobalCallbackInstance;


//! @brief Destructor
SubstanceAir::RenderCallbacks::~RenderCallbacks()
{
}


//! @brief Output computed callback (render result available)
//! @param runUid The UID of the corresponding computation (returned by 
//! 	Renderer::run()).
//! @param userData The user data set at corresponding run
//!		(as second argument Renderer::run(xxx,userData))
//! @param graphInstance Pointer on output parent graph instance
//! @param outputInstance Pointer on computed output instance w/ new render
//!		result just available (use OutputInstance::grabResult() to grab it).
//!
//! Called each time an new render result is just available.
void SubstanceAir::RenderCallbacks::outputComputed(
	UInt runUid,
	size_t userData,
	const GraphInstance* graphInstance,
	OutputInstance* outputInstance)
{
	(void)runUid;
	(void)userData;
	(void)graphInstance;
	(void)outputInstance;
}

//! @brief Job computed callback
//! @param runUid The UID of the corresponding computation (returned by
//! 	Renderer::run()).
//! @param userData The user data set at corresponding run
//!		(as second argument Renderer::run(xxx,userData))
void SubstanceAir::RenderCallbacks::jobComputed(
	UInt runUid,
	size_t userData)
{
	(void)runUid;
	(void)userData;
}

bool SubstanceAir::RenderCallbacks::runRenderProcess(
	RenderFunction renderFunction,
	void* renderParams)
{
	(void)renderFunction;
	(void)renderParams;

	return false;
}


//! @brief Fill Substance Engine Device callback
//! @param platformId The id corresponding to current engine API platform
//! @param substanceDevice Pointer on substance device to fill
//!		Concrete device type depends on engine API platform.
//! @note Can be discarded on Engine w/ BLEND platform (textures in system 
//!		memory: default). Mandatory on some platforms.
void SubstanceAir::RenderCallbacks::fillSubstanceDevice(
	SubstanceEngineIDEnum platformId,
	SubstanceDevice_ *substanceDevice)
{
	(void)platformId;
	(void)substanceDevice;
}


//! @brief Set global (static) user callbacks
//! @param callbacks Pointer on the user callbacks concrete structure 
//! 	instance that will be used for the global callbacks call.
//! @warning Global callback instance must be set before any render and
//!		never modified. GlobalCallbacks instance should be deleted AFTER
//! 	Renderers instances.
void SubstanceAir::GlobalCallbacks::setInstance(GlobalCallbacks* callbacks)
{
	assert(callbacks != nullptr);
	mInstance = callbacks;
}


//! @brief Destructor
SubstanceAir::GlobalCallbacks::~GlobalCallbacks()
{
}


//! @brief Enabled user callbacks mask
//! @return Returns a combination of Enabled enums, indicates which 
//! 	callbacks are really overridden by the user.
//!
//! In order to override callbacks, the user must override this method in
//! concrete GlobalCallbacks structures and make it return the enabled
//! enums.
unsigned int SubstanceAir::GlobalCallbacks::getEnabledMask() const
{
	return 0;
}


//! @brief Evict cache callback (interm. result render need to be evicted)
//! @param cacheItemUid The UID of the cache item to evict
//! @param cacheBuffer The buffer of the corresponding cache item. Must
//!		be stored by user.
//! @param cacheBufferSize The buffer size of the corresponding cache item
//! @note This callback must be implemented in concrete GlobalCallbacks 
//! 	structures if getEnabledMask() returns Enable_RenderCache bit.
//!
//! Called each time engine need to evict an intermediary result
void SubstanceAir::GlobalCallbacks::renderCacheEvict(
	UInt64 cacheItemUid,
	const void *cacheBuffer,
	size_t cacheBufferSize)
{
	(void)cacheItemUid;
	(void)cacheBuffer;
	(void)cacheBufferSize;
}


//! @brief Fetch cache callback (interm. result render need to be fetched)
//! @param cacheItemUid The UID of the cache item to retrieve
//! @param cacheBuffer The buffer of the corresponding cache item.
//! 	Must be filled w/ the content provided by the previous
//! 	renderCacheEvict() call w/ same 'cacheItemUid'.
//! @param cacheBufferSize The buffer size of the corresponding cache item
//! @note This callback must be implemented in concrete GlobalCallbacks 
//! 	structures if getEnabledMask() returns Enable_RenderCache bit.
//!
//! Called each time engine need to fetch an intermediary result
void SubstanceAir::GlobalCallbacks::renderCacheFetch(
	UInt64 cacheItemUid,
	void *cacheBuffer,
	size_t cacheBufferSize)
{
	(void)cacheItemUid;
	(void)cacheBuffer;
	(void)cacheBufferSize;
}


//! @brief Remove cache callback (interm. result render no more necessary)
//! @param cacheItemUid The UID of the cache item to remove from cache.
//! @note This callback must be implemented in concrete GlobalCallbacks 
//! 	structures if getEnabledMask() returns Enable_RenderCache bit.
//!
//! Called each time an intermediary result is deprecated
void SubstanceAir::GlobalCallbacks::renderCacheRemove(
	UInt64 cacheItemUid)
{
	(void)cacheItemUid;
}

//! @brief User system memory allocation callback
//! @param bytesCount Number of bytes to allocate.
//! @param alignment Required buffer address alignment (in bytes).
//! @return Must return the corresponding buffer.
//! @note This callback must be implemented in concrete GlobalCallbacks 
//! 	structures if getEnabledMask() returns Enable_UserAlloc bit.
//!
//! This callback allows the use of user-defined dynamic memory allocation.
void* SubstanceAir::GlobalCallbacks::memoryAlloc(
	size_t bytesCount,
	size_t alignment)
{
    // make sure alignment is at least platform default
    alignment = std::max((size_t)AIR_DEFAULT_ALIGNMENT, alignment);

#if defined (AIR_NO_DEFAULT_ALLOCATOR)
	//If NO_DEFAULT_ALLOCATOR is set, then that means we should not 
	//call this function at all. The developer/implementor should
	//appropriately override the memoryAlloc function in their
	//custom GlobalCallbacks class.
	(bytesCount);
	(alignment);
	assert(false);
	return nullptr;
#elif defined (_MSC_VER)
	return _aligned_malloc(bytesCount, alignment);
#elif defined(_POSIX_VERSION) || defined(_POSIX_SOURCE) || defined(__APPLE__)
	void* res;
	int err = posix_memalign(
		&res,
		alignment,
		bytesCount);
	assert(err==0);
	return (err ? nullptr : res);
#elif defined(AIR_CPP_MODERN_MEMORY)
	return aligned_alloc(alignment,bytesCount);
#else 
	#error Substance Framework - No aligned allocator found
#endif
}


//! @brief User system memory de-allocation callback
//! @param bufferPtr Buffer to free.
//! @note This callback must be implemented in concrete GlobalCallbacks 
//! 	structures if getEnabledMask() returns Enable_UserAlloc bit.
//!
//! This callback allows to free buffers allocated with the malloc()
//! callback.
void SubstanceAir::GlobalCallbacks::memoryFree(
	void* bufferPtr)
{
#if defined (AIR_NO_DEFAULT_ALLOCATOR)
	//If NO_DEFAULT_ALLOCATOR is set, then that means we should not 
	//call this function at all. The developer/implementor should
	//appropriately override the memoryFree function in their
	//custom GlobalCallbacks class.
	(bufferPtr);
	assert(false);
#elif defined (_MSC_VER)
	_aligned_free(bufferPtr);
#elif defined(_POSIX_VERSION) || defined(_POSIX_SOURCE) || defined(__APPLE__)
	free(bufferPtr);
#elif defined(AIR_CPP_MODERN_MEMORY)
	free(bufferPtr);
#endif
}


//! @brief Preset apply, image loading callback
//! @param[out] inst The image input instance to set loaded image (using 
//! 	InputInstanceImage::setImage).
//! @param filepath Filepath of the image to load in UTF-8 format,
//!		<b>relative to preset filepath</b>.
//! @warning The filepath is relative to loaded preset file location.
//!
//! This callback is called for each input image set by an applied preset 
//! (Preset::apply). The image loading operation must be done by this 
//! callback.
void SubstanceAir::GlobalCallbacks::loadInputImage(
	InputInstanceImage& inst,
	const SubstanceAir::string& filepath)
{
	(void)inst;
	(void)filepath;
}


//! @brief Preset filling, image filepath callback
//! @param inputImage The image input to retrieve corresponding bitmap 
//! 	filepath. The InputImage::mUserData can be useful to tag input
//!		images and retrieve filepath in a easier way.
//! @return Filepath of the bitmap corresponding to inputImage in UTF-8 
//!		format. The filepath must be <b>relative to preset filepath</b>.
//!		Return an empty string to skip this input in the preset.
//! @warning The returned filepath must be relative to saved preset file 
//!		location.
//!
//! This callback is called for each input image to serialize during a 
//!	preset creation (Preset::fill).
SubstanceAir::string SubstanceAir::GlobalCallbacks::getImageFilepath(
	const InputImage& inputImage)
{
	string res;
	(void)inputImage;
	return res;
}

