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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSENGINE_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSENGINE_H

#include "detailsgraphbinary.h"
#include "detailssync.h"
#include "detailsdynload.h"
#include "detailsutils.h"

#include <substance/framework/platform.h>

#include <substance/linker/handle.h>

#include <substance/version.h>

#include <memory>



namespace SubstanceAir
{

struct RenderOptions;
struct TextureAgnostic;
struct RenderCallbacks;

namespace Details
{

class RenderJob;
struct LinkGraphs;
struct LinkContext;


//! @brief Substance Engine Wrapper
class Engine
{
public:
	//! @brief Constructor
	//! @param renderOptions Initial render options.
	//! @param module default engine module/library to use
	Engine(const RenderOptions& renderOptions, void* module);

	//! @brief Destructor
	//! @pre Must be already released (releaseEngine())
	~Engine();

	//! @brief retrieve engine version
	const SubstanceVersion& getVersion() const;

	//! @brief Destroy engine (handle and context)
	//! @pre No render currently running
	void releaseEngine();

	//! @brief Accessor on current Substance Handle
	//! @return Return the current handle or nullptr not already linked
	SubstanceHandle_* getHandle() const { return mHandle; }

	//! @brief Link/Relink all states in pending render jobs
	//! @return Return true if link process succeed
	bool link(RenderCallbacks* callbacks,LinkGraphs& linkGraphs);

	//! @brief Flush internal engine queue
	//! @post Engine render queue is flushed
	void flush();

	//! @brief Stop any generation
	//! Thread safe stop call on current handle if present
	//! @note Called from user thread
	void stop();

	//! @brief Restore graphics API states if context present
	void restoreRenderStates();

	//! @brief Set new memory budget and CPU usage.
	//! @param renderOptions New render options to use.
	//! @note This function can be called at any time, from any thread.
	//!	@return Return true if options are immediately processed.
	bool setOptions(const RenderOptions& renderOptions);

	//! @brief Switch dynamic library
	//! @param module Dynamic library to use, result of dlopen/LoadLibrary call
	//!		use nullptr to revert to default impl.
	//! @return Return true if dynamic library loaded and compatible
	bool switchLibrary(void* module);

	//! @brief Linker Collision UID callback implementation
	//! @param collisionType Output or input collision flag
	//! @param previousUid Initial UID that collide
	//! @param newUid New UID generated
	//! @note Called by linker callback
	void callbackLinkerUIDCollision(
		SubstanceLinkerUIDCollisionType collisionType,
		UInt previousUid,
		UInt newUid);

	//! @brief Accessor on UID of this instance (used for cache eviction)
	//! Each instance has a different UID in the current execution context.
	UInt getInstanceUid() const { return mInstanceUid; }

	#ifndef AIR_NO_DYNLOAD
		//! @brief Dynamic loading pointers accessor
		const DynLoad& getDynLoad() const { return mDynLoad; }
	#endif  // ifndef AIR_NO_DYNLOAD

	//! @brief Enqueue texture for deletion, texture ownership is grabbed
	//! @warning Can be called from user thread
	void enqueueRelease(const TextureAgnostic& texture);

	//! @brief Release enqueued textures (enqueueRelease())
	//! @brief Must be called from render thread
	void releaseTextures();

protected:
	//! @brief Engine Contexts class
	class Context
	{
	public:
		//! @brief Context constructor, create Substance engine context
		Context(Engine& parent,RenderCallbacks* callbacks);

		//! @brief Destructor, release Substance Context
		~Context();

		//! @brief Accessor on Substance Engine Context
		SubstanceContext_ *getContext() const { return mContext; }

	protected:
		//! @brief Parent engine reference
		Engine& mParent;

		//! @brief Substance Engine Context
		SubstanceContext_ *mContext;

	};  // class Context

	//! @brief List of textures to delete
	typedef deque<TextureAgnostic> TexturesList;

	//! @brief SBSBIN container, must be 16 byte aligned
	typedef aligned_vector<std::uint8_t, 16> SbsBinVector;

	//! @brief Last UID used, used to generate Engine::mInstanceUid
	static UInt mLastInstanceUid;

	//! @brief UID of this instance (used for cache eviction)
	//! Each instance has a different UID in the current execution context.
	const UInt mInstanceUid;

	#ifdef AIR_NO_DYNLOAD
		//! @brief Engine version (immutable)
		SubstanceVersion mVersion;
	#else  // ifdef AIR_NO_DYNLOAD
		//! @brief Dynamic loading pointers wrapper
		DynLoad mDynLoad;
	#endif  // ifdef AIR_NO_DYNLOAD

	//! @brief This instance Context (create on render thread)
	unique_ptr<Context> mContextInstance;

	//! @brief SBSBIN data, double buffer
	//! Two buffers are used for cache transfer
	//! @invariant At least one buffer is empty
	SbsBinVector mSbsbinDatas[2];

	//! @brief Linker cache data generated by linker
	const unsigned char* mLinkerCacheData;

	//! @brief The current substance handle
	//! nullptr if not yet linked
	SubstanceHandle_ *mHandle;

	//! @brief Mutex on handle stop/create/switch
	//! Used for stop() thread safe action
	Sync::mutex mMutexHandle;

	//! @brief Substance Linker Context
	//! One linker context/handle per Renderer to avoid concurrency issues
	SubstanceLinkerContext *mLinkerContext;

	//! @brief Substance Linker Handle
	//! One linker context/handle per Renderer to avoid concurrency issues
	SubstanceLinkerHandle *mLinkerHandle;

	//! @brief Substance Engine hard resources used by handle
	SubstanceHardResources mHardResources;

	//! @brief Contains current filled binary, used during link (by callbacks)
	LinkContext *mCurrentLinkContext;

	//! @brief List of textures to delete
	//! R/W access thread safety ensure by mMutexToRelease.
	TexturesList mToReleaseTextures;

	//! @brief Mutex on mToReleaseTextures access
	Sync::mutex mMutexToRelease;

	//! @brief Pending textures to release into mToReleaseTextures
	//! Can be set from any thread. Unset from render thread.
	atomic_bool mPendingReleaseTextures;

	//! @brief Maximum internal node w/h (log2)
	size_t mMaxIntermRenderSize;

	//! @brief Fill Graph binaries w/ new Engine handle SBSBIN indices
	//! @param linkGraphs Contains Graph binaries to fill indices
	void fillIndices(LinkGraphs& linkGraphs) const;

	//! @brief Create Linker handle and context
	void createLinker();

	//! @brief Release linker handle and context
	void releaseLinker();

	//! @brief Fill hard resources from render options
	static void fillHardResources(
		SubstanceHardResources& hardRsc,
		const RenderOptions& renderOptions);

private:
	Engine(const Engine&);
	const Engine& operator=(const Engine&);
};  // class Engine


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSENGINE_H
