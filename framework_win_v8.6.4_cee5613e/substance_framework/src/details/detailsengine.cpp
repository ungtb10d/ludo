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

#include "detailsengine.h"
#include "detailsrenderjob.h"
#include "detailsrenderpushio.h"
#include "detailsrendererimpl.h"
#include "detailsgraphbinary.h"
#include "detailsgraphstate.h"
#include "detailsinputimagetoken.h"
#include "detailslinkgraphs.h"
#include "detailslinkcontext.h"
#include "detailslinkdata.h"
#include "detailsutils.h"

#include <substance/framework/renderresult.h>
#include <substance/framework/renderopt.h>
#include <substance/framework/callbacks.h>

#include <substance/version.h>
#include <substance/linker/linker.h>

#include <algorithm>
#include <iterator>
#include <utility>

#include <assert.h>
#include <memory.h>


SUBSTANCE_EXTERNC {

//! @brief Substance_Linker_Callback_UIDCollision linker callback impl.
//! Fill GraphBinary translated UIDs
static void SUBSTANCE_CALLBACK substanceAirDetailsLinkerCallbackUIDCollision(
	SubstanceLinkerHandle *handle,
	SubstanceLinkerUIDCollisionType collisionType,
	unsigned int previousUid,
	unsigned int newUid)
{
	unsigned int res;
	(void)res;
	SubstanceAir::Details::Engine *engine = nullptr;
	res = substanceLinkerHandleGetUserData(handle,(size_t*)&engine);
	assert(res==0);

	engine->callbackLinkerUIDCollision(
		collisionType,
		previousUid,
		newUid);
}


//! @brief Substance_Callback_OutputCompleted engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackOutputCompleted(
	SubstanceHandle_ *handle,
	unsigned int outputIndex,
	size_t jobUserData)
{
	using namespace SubstanceAir;

	Details::RenderPushIO *pushio = (Details::RenderPushIO*)jobUserData;
	assert(pushio!=nullptr);

	Details::Engine *engine = pushio->getEngine();
	assert(engine!=nullptr);

	RenderResultBase *renderres = NULL;

	const SubstanceIOType outtype = pushio->getOutputType(outputIndex);
	if (outtype==Substance_IOType_Image)
	{
		// Grab texture result
		TextureAgnostic texture;
		texture.platform = engine->getVersion().platformApiEnum;
		unsigned int res = AIR_SBSCALL(HandleGetOutputs,engine->getDynLoad())(
			handle,
			Substance_OutOpt_OutIndex,
			outputIndex,
			1,
			texture.textureAny);
		assert(res==0);
		(void)res;

		// Get parent context
		SubstanceContext_* context = AIR_SBSCALL(HandleGetParentContext,engine->getDynLoad())(
			handle);
		assert(context!=nullptr);

		// Create render result
		renderres = AIR_NEW(RenderResultImage)(texture,context,engine);
	}
	else
	{
		// Grab numerical result
		Vec4Int numres;
		unsigned int res = AIR_SBSCALL(HandleGetOutputs,engine->getDynLoad())(
			handle,
			Substance_OutOpt_OutIndex|Substance_OutOpt_Numerical,
			outputIndex,
			1,
			&numres);
		assert(res==0);
		(void)res;

		switch (outtype)
		{
			case Substance_IOType_Float:    renderres = AIR_NEW(RenderResultFloat )(Substance_IOType_Float   ,&numres); break;
			case Substance_IOType_Float2:   renderres = AIR_NEW(RenderResultFloat2)(Substance_IOType_Float2  ,&numres); break;
			case Substance_IOType_Float3:   renderres = AIR_NEW(RenderResultFloat3)(Substance_IOType_Float3  ,&numres); break;
			case Substance_IOType_Float4:   renderres = AIR_NEW(RenderResultFloat4)(Substance_IOType_Float4  ,&numres); break;
			case Substance_IOType_Integer:  renderres = AIR_NEW(RenderResultInt   )(Substance_IOType_Integer ,&numres); break;
			case Substance_IOType_Integer2: renderres = AIR_NEW(RenderResultInt2  )(Substance_IOType_Integer2,&numres); break;
			case Substance_IOType_Integer3: renderres = AIR_NEW(RenderResultInt3  )(Substance_IOType_Integer3,&numres); break;
			case Substance_IOType_Integer4: renderres = AIR_NEW(RenderResultInt4  )(Substance_IOType_Integer4,&numres); break;
		}
	}

	// Transmit it to Push I/O
	pushio->callbackOutputComplete(outputIndex,renderres);
}


//! @brief Substance_Callback_JobCompleted engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackJobCompleted(
	SubstanceHandle_*,
	size_t jobUserData)
{
	SubstanceAir::Details::RenderPushIO *pushio =
		(SubstanceAir::Details::RenderPushIO*)jobUserData;
	if (pushio!=nullptr)
	{
		pushio->callbackJobComplete();
	}
}


//! @brief Substance_Callback_InputImageLock engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackInputImageLock(
	SubstanceHandle_*,
	size_t,
	unsigned int,
	SubstanceTextureInput_ **currentTextureInputDesc,
	const SubstanceTextureInput_*)
{
	SubstanceAir::Details::TextureInput *const texinp =
		reinterpret_cast<SubstanceAir::Details::TextureInput*>(*currentTextureInputDesc);

	if (texinp!=nullptr)
	{
		texinp->parentToken->lock();
		*currentTextureInputDesc = reinterpret_cast<SubstanceTextureInput_*>(texinp);
	}
}


//! @brief Substance_Callback_InputImageUnlock engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackInputImageUnlock(
	SubstanceHandle_*,
	size_t,
	unsigned int,
	SubstanceTextureInput_* textureInputDesc)
{
	SubstanceAir::Details::TextureInput *const texinp =
		reinterpret_cast<SubstanceAir::Details::TextureInput*>(textureInputDesc);
	if (texinp!=nullptr)
	{
		texinp->parentToken->unlock();
	}
}


//! @ brief Return Engine pointer stored as handle user data
static SubstanceAir::Details::Engine* substanceAirDetailsGetEngineFromHandle(
	SubstanceHandle_* handle);


//! @brief Cache eviction engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackCacheEvict(
	SubstanceHandle_* handle,
	unsigned int flags,
	unsigned int itemUid,
	void *buffer,
	size_t bytesCount)
{
	// Grab user data: substanceHandleGetUserData is platform independent
	SubstanceAir::Details::Engine *const engine =
		substanceAirDetailsGetEngineFromHandle(handle);

	const SubstanceAir::UInt64 itemUid64 =
		static_cast<SubstanceAir::UInt64>(engine->getInstanceUid())<<32 |
		static_cast<SubstanceAir::UInt64>(itemUid);
	SubstanceAir::GlobalCallbacks *const callbacks =
		SubstanceAir::GlobalCallbacks::getInstance();

	assert(sizeof(SubstanceAir::UInt64)>=8);
	assert(callbacks!=nullptr);

	if ((flags&Substance_CacheEvict_Evict)!=0)
	{
		callbacks->renderCacheEvict(itemUid64,buffer,bytesCount);
	}
	else if ((flags&Substance_CacheEvict_Fetch)!=0)
	{
		callbacks->renderCacheFetch(itemUid64,buffer,bytesCount);
	}
	if ((flags&Substance_CacheEvict_Remove)!=0)
	{
		callbacks->renderCacheRemove(itemUid64);
	}
}


//! @brief Substance_Callback_Malloc engine callback impl.
static void* SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackMalloc(
	size_t bytesCount,
	size_t alignment)
{
	return SubstanceAir::alignedMalloc(bytesCount, alignment);
}

//! @brief Substance_Callback_Malloc engine callback impl.
static void SUBSTANCE_CALLBACK substanceAirDetailsEngineCallbackFree(
	void* bufferPtr)
{
	return SubstanceAir::alignedFree(bufferPtr);
}

} // EXTERNC



//! @brief Last UID used, used to generate Engine::mInstanceUid
SubstanceAir::UInt SubstanceAir::Details::Engine::mLastInstanceUid = 0;


//! @brief Constructor
//! @param renderOptions Initial render options.
//! @param module Initial engine module
SubstanceAir::Details::Engine::Engine(const RenderOptions& renderOptions, void* module) :
	mInstanceUid((++mLastInstanceUid)|0x80000000u),
	mLinkerCacheData(nullptr),
	mHandle(nullptr),
	mLinkerContext(nullptr),
	mLinkerHandle(nullptr),
	mCurrentLinkContext(nullptr),
	mPendingReleaseTextures(ATOMIC_VAR_INIT(false)),
	mMaxIntermRenderSize(renderOptions.mMaxIntermRenderSize)
{
	if (module != nullptr)
		switchLibrary(module);

	fillHardResources(mHardResources,renderOptions);
	createLinker();

	#ifdef AIR_NO_DYNLOAD
		substanceGetCurrentVersion(&mVersion);
	#endif  // ifdef AIR_NO_DYNLOAD
}


const SubstanceVersion& SubstanceAir::Details::Engine::getVersion() const
{
	#ifdef AIR_NO_DYNLOAD
		return mVersion;
	#else  // ifdef AIR_NO_DYNLOAD
		return mDynLoad.getVersion();
	#endif  // ifdef AIR_NO_DYNLOAD
}


//! @brief Create Linker handle and context
void SubstanceAir::Details::Engine::createLinker()
{
	unsigned int res;
	(void)res;
	{
		// Get Engine platform
		SubstanceVersion verengine;
		AIR_SBSCALL(GetCurrentVersionImpl,getDynLoad())(
			&verengine,
			SUBSTANCE_API_VERSION);

		// Create linker context
		res = substanceLinkerContextInit(
			&mLinkerContext,
			SUBSTANCE_LINKER_API_VERSION,
			verengine.platformImplEnum);
		assert(res==0);
	}

	{
		// Set global fusing option
		SubstanceLinkerOptionValue optvalue;
		optvalue.uinteger = Substance_Linker_Global_FuseNone;  // No fusing
		res = substanceLinkerContextSetOption(
			mLinkerContext,
			Substance_Linker_Option_GlobalTweaks,
			optvalue);
		assert(res==0);
	}

	// Set UID collision callback
	res = substanceLinkerContextSetCallback(
		mLinkerContext,
		Substance_Linker_Callback_UIDCollision,
		(void*)substanceAirDetailsLinkerCallbackUIDCollision);
	assert(res==0);

	// Create linker handle
	res = substanceLinkerHandleInit(&mLinkerHandle,mLinkerContext);
	assert(res==0);

	// Set this as user data for callbacks
	res = substanceLinkerHandleSetUserData(mLinkerHandle,(size_t)this);
	assert(res==0);
}


//! @brief Destructor
SubstanceAir::Details::Engine::~Engine()
{
	// Must be already released from render thread
	assert(mHandle==nullptr);
	assert(mContextInstance.get()==nullptr);

	releaseLinker();
}


//! @brief Release linker handle and context
void SubstanceAir::Details::Engine::releaseLinker()
{
	unsigned int res;
	(void)res;

	if (mLinkerHandle != nullptr)
	{
		res = substanceLinkerHandleRelease(mLinkerHandle);
		assert(res==0);
	}

	if (mLinkerContext != nullptr)
	{
		res = substanceLinkerContextRelease(mLinkerContext);
		assert(res==0);
	}
}


//! @brief Destroy engine (handle and context)
//! @pre No render currently running
void SubstanceAir::Details::Engine::releaseEngine()
{
	// Release engine handle
	if (mHandle!=nullptr)
	{
		unsigned int res = AIR_SBSCALL(HandleRelease,getDynLoad())(mHandle);
		(void)res;
		assert(res==0);

		mHandle = nullptr;
	}

	// Release pending textures
	releaseTextures();

	// Release engine context
	mContextInstance.reset();
}


bool SubstanceAir::Details::Engine::link(
	RenderCallbacks* callbacks,
	LinkGraphs& linkGraphs)
{
	unsigned int res;
	(void)res;

	// Disable all outputs by default
	res = substanceLinkerHandleSelectOutputs(
		mLinkerHandle,
		Substance_Linker_Select_UnselectAll,
		0);
	assert(res==0);

	// Push all states to link
	for (auto& graphstateptr : linkGraphs.graphStates)
	{
		LinkContext linkContext(
			mLinkerHandle,
			graphstateptr->getBinary(),
			graphstateptr->getUid());
		linkContext.graphBinary.resetTranslatedUids();  // Reset translated UID
		mCurrentLinkContext = &linkContext;  // Set as current context to fill
		graphstateptr->getLinkData()->push(linkContext);

		// Select outputs, set format and shuffle
		for (auto& entry : linkContext.graphBinary.outputs)
		{
			// Reset flag for next link enqueue
			entry.enqueuedLink = false;

			const OutputFormat& outfmt = entry.state.formatOverride;
			const bool stateover = entry.state.formatOverridden;
			const bool useshuffle = stateover && (
				!outfmt.perComponent[0].isIdentity(0) ||
				!outfmt.perComponent[1].isIdentity(1) ||
				!outfmt.perComponent[2].isIdentity(2) ||
				!outfmt.perComponent[3].isIdentity(3));
			if (stateover &&
				(outfmt.hvFlip!=OutputFormat::HVFlip_No ||
					outfmt.forceHeight!=OutputFormat::UseDefault ||
					outfmt.forceWidth!=OutputFormat::UseDefault ||
					useshuffle))
			{
				// Requires output creation
				SubstanceLinkerOutputCreate outcreate;
				outcreate.format = outfmt.format!=OutputFormat::UseDefault ?
					outfmt.format :
					entry.descFormat;
				outcreate.mipmapLevelsCount = outfmt.mipmapLevelsCount!=OutputFormat::UseDefault ?
					outfmt.mipmapLevelsCount :
					entry.descMipmap;
				outcreate.hvFlip = (SubstanceLinkerHVFlip)outfmt.hvFlip;
				outcreate.forceWidth = (int)outfmt.forceWidth;
				outcreate.forceHeight = (int)outfmt.forceHeight;
				outcreate.useShuffling = useshuffle ? 1u : 0u;
				outcreate.outputUID = entry.uidTranslated;

				if (useshuffle)
				{
					outcreate.shuffle.useLevels = false;
					for (unsigned int k=0;k<OutputFormat::ComponentsCount;++k)
					{
						SubstanceLinkerChannel &cmpdst = outcreate.shuffle.channels[k];
						const OutputFormat::Component &cmpsrc = outfmt.perComponent[k];
						cmpdst.levelMin = cmpsrc.levelMin;
						cmpdst.levelMax = cmpsrc.levelMax;
						outcreate.shuffle.useLevels = outcreate.shuffle.useLevels ||
							cmpdst.levelMin!=0.f || cmpdst.levelMax!=1.f;
						cmpdst.channelIndex = cmpsrc.shuffleIndex;
						cmpdst.outputUID = cmpsrc.outputUid!=OutputFormat::UseDefault ?
							(cmpsrc.outputUid!=0 ?
								linkContext.translateUid(true,cmpsrc.outputUid,true).first :
								0) :
							outcreate.outputUID;
					}
				}

				res = substanceLinkerHandleCreateOutputEx(
					mLinkerHandle,
					&(entry.uidTranslated),
					&outcreate);

				if (res)
				{
// 					TODO2 Warning: create output not valid
					assert(0);
				}
			}
			else
			{
				// Select existing
				res = substanceLinkerHandleSelectOutputs(
					mLinkerHandle,
					Substance_Linker_Select_Select,
					entry.uidTranslated);

				if (res)
				{
// 					TODO2 Warning: output UID not valid
					assert(0);
				}
				else if (stateover || entry.descForced)
				{
					// Requires format/mipmap change
					res = substanceLinkerHandleSetOutputFormat(
						mLinkerHandle,
						entry.uidTranslated,
						stateover && outfmt.format != OutputFormat::UseDefault ?
							outfmt.format :
							entry.descFormat,
						stateover && outfmt.mipmapLevelsCount != OutputFormat::UseDefault ?
							outfmt.mipmapLevelsCount :
							entry.descMipmap);

					if (res)
					{
//						TODO2 Warning: format/mipmap not valid
						assert(0);
					}
				}
			}
		}
	}

	// Link, Grab assembly
	size_t sbsbindataindex = mSbsbinDatas[0].empty() ? 0 : 1;
	SbsBinVector &sbsbindata = mSbsbinDatas[sbsbindataindex];
	assert(mSbsbinDatas[0].empty() || mSbsbinDatas[1].empty());
	{
		const unsigned char* resultData = nullptr;
		size_t resultSize = 0;
		res = substanceLinkerHandleLink(
			mLinkerHandle,
			&resultData,
			&resultSize);
		assert(res==0);

		if (resultSize<(sbsbindata.size()>>1ull))
		{
			// Force reallocation if at least half of the size (heuristic)
			sbsbindata = SbsBinVector(resultData,resultData + resultSize);
		}
		else
		{
			sbsbindata.assign(resultData,resultData + resultSize);
		}
	}

	// Grab new cache data blob
	res = substanceLinkerHandleGetCacheMapping(
		mLinkerHandle,
		&mLinkerCacheData,
		nullptr,
		mLinkerCacheData);
	assert(res==0);

	// Create Substance context if necessary, done on render thread (required
	//	by some impl.)
	if (mContextInstance.get()==nullptr)
	{
		mContextInstance = make_unique<Context>(*this, callbacks);
	}

	// Create new handle
	SubstanceHandle_ *newhandle = nullptr;
	res = AIR_SBSCALL(HandleInit,getDynLoad())(
		&newhandle,
		mContextInstance->getContext(),
		sbsbindata.data(),
		sbsbindata.size(),
		&mHardResources);
	assert(res==0);

	// Apply node size limit
	res = AIR_SBSCALL(HandleSetMaxNodeSize,getDynLoad())(
		newhandle,
		(unsigned int) mMaxIntermRenderSize,
		(unsigned int) mMaxIntermRenderSize);
	assert(res == 0);

	// Set engine pointer as user data
	res = AIR_SBSCALL(HandleSetUserData,getDynLoad())(
		newhandle,
		reinterpret_cast<size_t>(this));
	assert(res==0);

	// Switch handle
	{
		// Scoped modification
		Sync::unique_lock slock(mMutexHandle);

		if (mHandle!=nullptr)
		{
			// Transfer cache
			res = AIR_SBSCALL(HandleTransferCache,getDynLoad())(
				newhandle,
				mHandle,
				mLinkerCacheData);
			assert(res==0);

			// Delete previous handle
			res = AIR_SBSCALL(HandleRelease,getDynLoad())(mHandle);
			assert(res==0);

			// Erase previous sbsbin data
			mSbsbinDatas[sbsbindataindex^1].clear();
		}

		// Use new one
		mHandle = newhandle;
	}

	// Fill Graph binary SBSBIN indices
	fillIndices(linkGraphs);

	return true;
}


//! @brief Flush internal engine queue
//! @post Engine render queue is flushed
void SubstanceAir::Details::Engine::flush()
{
	assert(mHandle!=nullptr);

	unsigned int res = AIR_SBSCALL(HandleFlush,getDynLoad())(mHandle);
	(void)res;
	assert(res==0);
}


//! @brief Stop any generation
//! Thread safe stop call on current handle if present
//! @note Called from user thread
void SubstanceAir::Details::Engine::stop()
{
	Sync::unique_lock slock(mMutexHandle);
	if (mHandle!=nullptr)
	{
		// Stop, may be not very useful, do not check return code!
		AIR_SBSCALL(HandleStop,getDynLoad())(mHandle,Substance_Sync_Asynchronous);
	}
}


void SubstanceAir::Details::Engine::restoreRenderStates()
{
	if (mContextInstance.get()!=nullptr)
	{
		unsigned int res = AIR_SBSCALL(ContextRestoreStates,getDynLoad())(
			mContextInstance->getContext());
		assert(res==0);
	}
}


//! @brief Set new memory budget and CPU usage.
//! @param renderOptions New render options to use.
//! @note This function can be called at any time, from any thread.
//!	@return Return true if options are immediately processed.
bool SubstanceAir::Details::Engine::setOptions(
	const RenderOptions& renderOptions)
{
	unsigned int res, state = 0;
	(void)res;

	Sync::unique_lock slock(mMutexHandle);

	fillHardResources(mHardResources,renderOptions);

	if (mHandle!=nullptr)
	{
		// Switch resources
		res = AIR_SBSCALL(HandleSwitchHard,getDynLoad())(
			mHandle,
			Substance_Sync_Asynchronous,
			&mHardResources,
			nullptr,
			0);
		assert(res==0);

		// Get current engine state
		res = AIR_SBSCALL(HandleGetState,getDynLoad())(
			mHandle,
			nullptr,
			&state);
		assert(res==0);
	}

	// Apply the new node size limit
	if (renderOptions.mMaxIntermRenderSize != mMaxIntermRenderSize)
	{
		mMaxIntermRenderSize = renderOptions.mMaxIntermRenderSize;
		res = AIR_SBSCALL(HandleSetMaxNodeSize, getDynLoad())(
			mHandle,
			(unsigned int) mMaxIntermRenderSize,
			(unsigned int) mMaxIntermRenderSize);
		assert(res == 0);
	}

	// New memory will be processed if engine render job not empty
	return (state&Substance_State_NoMoreRender)==0;
}


//! @brief Switch dynamic library
//! @param module Dynamic library to use, result of dlopen/LoadLibrary call
//!		use nullptr to revert to default impl.
//! @return Return true if dynamic library loaded and compatible
bool SubstanceAir::Details::Engine::switchLibrary(void* module)
{
	#ifdef AIR_NO_DYNLOAD

		(void)module;
		return false;

	#else  // ifdef AIR_NO_DYNLOAD

		// Must be already released from render thread
		assert(mHandle==nullptr);
		assert(mContextInstance.get()==nullptr);
		assert(mToReleaseTextures.empty());

		releaseLinker();

		mSbsbinDatas[0].clear();
		mSbsbinDatas[1].clear();
		mLinkerCacheData = nullptr;

		bool res = mDynLoad.switchLibrary(module);

		createLinker();

		return res;

	#endif  // ifdef AIR_NO_DYNLOAD
}


//! @brief Linker Collision UID callback implementation
//! @param collisionType Output or input collision flag
//! @param previousUid Initial UID that collide
//! @param newUid New UID generated
//! @note Called by linker callback
void SubstanceAir::Details::Engine::callbackLinkerUIDCollision(
	SubstanceLinkerUIDCollisionType collisionType,
	UInt previousUid,
	UInt newUid)
{
	assert(mCurrentLinkContext!=nullptr);
	mCurrentLinkContext->notifyLinkerUIDCollision(
		collisionType,
		previousUid,
		newUid);
}


//! @brief Context constructor, create Substance engine context
SubstanceAir::Details::Engine::Context::Context(
		Engine &parent,
		RenderCallbacks* callbacks) :
	mParent(parent),
	mContext(nullptr)
{
	unsigned int res;
	(void)res;

	// Create context
	// multi-platform device buffer, must be large enough to contains any
	// concrete device
	void* device[16];
	memset(device,0,sizeof(device));

	if (callbacks!=nullptr)
	{
		callbacks->fillSubstanceDevice(
			mParent.getVersion().platformImplEnum,
			(SubstanceDevice_*)device);
	}

	res = AIR_SBSCALL(ContextInitImpl,mParent.getDynLoad())(
		&mContext,
		(SubstanceDevice_*)device,
		SUBSTANCE_API_VERSION,
		parent.getVersion().platformApiEnum);
	assert(res==0);

	// Plug callbacks
	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_OutputCompleted,
		(void*)substanceAirDetailsEngineCallbackOutputCompleted);
	assert(res==0);

	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_JobCompleted,
		(void*)substanceAirDetailsEngineCallbackJobCompleted);
	assert(res==0);

	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_InputImageLock,
		(void*)substanceAirDetailsEngineCallbackInputImageLock);
	assert(res==0);

	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_InputImageUnlock,
		(void*)substanceAirDetailsEngineCallbackInputImageUnlock);
	assert(res==0);

	// Plug user global callbacks if defined
	GlobalCallbacks *globalCallbacks = GlobalCallbacks::getInstance();
	assert(globalCallbacks != nullptr);

	if ((globalCallbacks->getEnabledMask()&
		GlobalCallbacks::Enable_RenderCache)!=0)
	{
		res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
			mContext,
			Substance_Callback_CacheEvict,
			(void*)substanceAirDetailsEngineCallbackCacheEvict);
		assert(res==0);
	}

	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_Malloc,
		(void*)substanceAirDetailsEngineCallbackMalloc);
	assert(res==0);

	res = AIR_SBSCALL(ContextSetCallback,mParent.getDynLoad())(
		mContext,
		Substance_Callback_Free,
		(void*)substanceAirDetailsEngineCallbackFree);
	assert(res==0);
}


//! @brief Destructor, release Substance Context
SubstanceAir::Details::Engine::Context::~Context()
{
	unsigned int res = AIR_SBSCALL(ContextRelease,mParent.getDynLoad())(mContext);
	(void)res;
	assert(res==0);
}


//! @brief Fill Graph binaries w/ new Engine handle SBSBIN indices
//! @param linkGraphs Contains Graph binaries to fill indices
void SubstanceAir::Details::Engine::fillIndices(LinkGraphs& linkGraphs) const
{
	typedef vector<std::pair<UInt,UInt> > UidIndexPairs;
	UidIndexPairs trinputs,troutputs;

	// Get indices/UIDs pairs
	{
		unsigned int dindex;
		SubstanceDataDesc datadesc;
		unsigned int res = AIR_SBSCALL(HandleGetDesc,getDynLoad())(
			mHandle,
			&datadesc);
		(void)res;
		assert(res==0);

		// Parse Inputs
		trinputs.reserve(datadesc.inputsCount);
		for (dindex=0;dindex<datadesc.inputsCount;++dindex)
		{
			SubstanceInputDesc indesc;
			res = AIR_SBSCALL(HandleGetInputDesc,getDynLoad())(
				mHandle,
				dindex,
				&indesc);
			assert(res==0);

			trinputs.push_back(std::make_pair(indesc.inputId,dindex));
		}

		// Parse outputs
		troutputs.reserve(datadesc.outputsCount);
		for (dindex=0;dindex<datadesc.outputsCount;++dindex)
		{
			SubstanceOutputDesc outdesc;
			res = AIR_SBSCALL(HandleGetOutputDesc,getDynLoad())(
				mHandle,
				dindex,
				&outdesc);
			assert(res==0);

			troutputs.push_back(std::make_pair(outdesc.outputId,dindex));
		}
	}

	typedef vector<std::pair<UInt,GraphBinary::Entry*> > UidEntryPairs;
	UidEntryPairs einputs,eoutputs;

	// Get graph binaries entries per translated uids
	for (const auto& graphstateptr : linkGraphs.graphStates)
	{
		GraphBinary& binary = graphstateptr->getBinary();

		// Get all input entries
		for (auto& entry : binary.inputs)
		{
			einputs.push_back(std::make_pair(entry.uidTranslated,&entry));
		}

		// Get all output entries
		for (auto& entry : binary.outputs)
		{
			eoutputs.push_back(std::make_pair(entry.uidTranslated,&entry));
		}

		// Mark as linked
		binary.linked();
	}

	// Sort all, per translated UID
	std::sort(trinputs.begin(),trinputs.end());
	std::sort(troutputs.begin(),troutputs.end());
	std::sort(einputs.begin(),einputs.end());
	std::sort(eoutputs.begin(),eoutputs.end());

	// Fill input entries w/ indices
	UidIndexPairs::const_iterator trite = trinputs.begin();
	for (auto& epair : einputs)
	{
		// Skip unused inputs (multi-graph case)
		while (trite!=trinputs.end() && trite->first<epair.first)
		{
			++trite;
		}
		assert(trite!=trinputs.end() && trite->first==epair.first);
		if (trite!=trinputs.end() && trite->first==epair.first)
		{
			epair.second->index = (trite++)->second;
		}
	}

	// Fill output entries w/ indices
	trite = troutputs.begin();
	for (auto& epair : eoutputs)
	{
		assert(trite!=troutputs.end() && trite->first==epair.first);
		while (trite!=troutputs.end() && trite->first<epair.first)
		{
			++trite;
		}
		if (trite!=troutputs.end() && trite->first==epair.first)
		{
			epair.second->index = (trite++)->second;
		}
	}
}


//! @brief Fill hard resources from render options
void SubstanceAir::Details::Engine::fillHardResources(
	SubstanceHardResources& hardRsc,
	const RenderOptions& renderOptions)
{
	memset(&hardRsc,0,sizeof(SubstanceHardResources));

	// 16kb minimum
	hardRsc.systemMemoryBudget = hardRsc.videoMemoryBudget[0] =
		renderOptions.mMemoryBudget|0x4000;

	// Switch on/off CPU usage
	for (size_t k=0;k<SUBSTANCE_CPU_COUNT_MAX;++k)
	{
		hardRsc.cpusUse[k] = k < std::max<size_t>(renderOptions.mCoresCount,1) ?
			Substance_Resource_Default :
			Substance_Resource_DoNotUse;
	}
}


//! @brief Enqueue texture for deletion, texture ownership is grabbed
//! @warning Can be called from user thread
void SubstanceAir::Details::Engine::enqueueRelease(
	const TextureAgnostic& texture)
{
	assert(mContextInstance.get()!=nullptr);
	assert(getVersion().platformApiEnum==texture.platform);

	{
		Sync::unique_lock slock(mMutexToRelease);

		mToReleaseTextures.push_back(texture);
		mPendingReleaseTextures = true;
	}
}


//! @brief Release enqueued textures (enqueueRelease())
//! @brief Must be called from render thread
void SubstanceAir::Details::Engine::releaseTextures()
{
	if (mContextInstance.get()!=nullptr &&
		mPendingReleaseTextures)
	{
		Sync::unique_lock slock(mMutexToRelease);

		for (auto& texture : mToReleaseTextures)
		{
			unsigned int res = AIR_SBSCALL(ContextTextureRelease,getDynLoad())(
				mContextInstance->getContext(),
				(SubstanceTexture_*)texture.textureAny);
			assert(res==0);
			(void)res;
		}

		mToReleaseTextures.resize(0);
		mPendingReleaseTextures = false;
	}
}


#ifndef AIR_NO_DEFAULT_ENGINE

#include <substance/handle.h>

//! @brief Return Engine pointer stored as handle user data
static SubstanceAir::Details::Engine* substanceAirDetailsGetEngineFromHandle(
	SubstanceHandle_* handle)
{
	// Grab user data: substanceHandleGetUserData is platform independent
	SubstanceAir::Details::Engine *engine = nullptr;
	unsigned int res = substanceHandleGetUserData(handle,(size_t*)&engine);
	(void)res;
	assert(res==0);
	assert(engine!=nullptr);

	return engine;
}

#else //ifndef AIR_NO_DEFAULT_ENGINE

//! @brief Return Engine pointer stored as handle user data
//! Hack: user data is first member of engine MainHandle structure
static SubstanceAir::Details::Engine* substanceAirDetailsGetEngineFromHandle(
	SubstanceHandle_* handle)
{
	return *(SubstanceAir::Details::Engine**)handle;
}

#endif //ifndef AIR_NO_DEFAULT_ENGINE

