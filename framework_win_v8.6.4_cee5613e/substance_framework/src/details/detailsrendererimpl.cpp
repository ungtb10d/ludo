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

#include "detailsduplicatejob.h"
#include "detailscomputation.h"
#include "detailsgraphstate.h"
#include "detailslinkgraphs.h"
#include "detailsrendererimpl.h"
#include "detailsrenderjob.h"
#include "detailsoutputsfilter.h"
#include "detailsutils.h"

#include <substance/framework/renderer.h>

#include <algorithm>

#include <assert.h>


//! @brief Default constructor
//! @param renderOptions Initial render options.
//! @param module Initial engine module.
SubstanceAir::Details::RendererImpl::RendererImpl(
		const RenderOptions& renderOptions, void* module) :
	mEngine(renderOptions, module),
	mCurrentJob(nullptr),
	mRenderState(RenderState_Idle),
	mHold(ATOMIC_VAR_INIT(false)),
	mCancelOccur(ATOMIC_VAR_INIT(false)),
	mPendingHardRsc(ATOMIC_VAR_INIT(false)),
	mExitRender(ATOMIC_VAR_INIT(false)),
	mUserWaiting(ATOMIC_VAR_INIT(false)),
	mEngineInitialized(false),
	mRenderCallbacks(nullptr),
	mRenderJobUid(0)
{
}


//! @brief Destructor
SubstanceAir::Details::RendererImpl::~RendererImpl()
{
	// Cancel all
	cancel();

	// Wait for cancel effective
	flush();

	// Clear all render results created w/ this engine instance
	mStates.releaseRenderResults(mEngine.getInstanceUid());

	// Exit thread / release engine
	exitRender();

	// Delete render jobs
	std::for_each(
		mRenderJobs.begin(),
		mRenderJobs.end(),
		Utils::Deleter<RenderJob>());
}


//! @brief Release engine and terminates render thread
//! If no thread created and engine need to be released, call
//! wakeupRender before thread termination.
void SubstanceAir::Details::RendererImpl::exitRender()
{
	bool needlaunch = false;

	// Exit required, try to wake up render thread
	{
		Sync::unique_lock slock(mMainMutex);

		mExitRender = mRenderState!=RenderState_Idle || mEngineInitialized;
		needlaunch = mExitRender && !wakeupRender();
	}

	// Launch render just for releasing engine if necessary
	if (needlaunch)
	{
		assert(mExitRender);

		launchRender();
	}

	// Wait that rendering thread ends (loop exit)
	{
		Sync::unique_lock slock(mMainMutex);

		assert(!mUserWaiting);

		while (mExitRender)
		{
			mUserWaiting = true;
			mCondVarUser.wait(slock);
			mUserWaiting = false;
		}
	}

	mEngineInitialized = false;

	if (mThread.joinable())
	{
		mThread.join();
	}

	assert(mRenderState==RenderState_Idle);
}


//! @brief Wait for render thread exit/sleep if currently ongoing
//! @pre mMainMutex Must be NOT locked
//! @note Called from user thread
void SubstanceAir::Details::RendererImpl::waitRender()
{
	{
		Sync::unique_lock slock(mMainMutex);

		assert(!mUserWaiting);

		while (mRenderState==RenderState_OnGoing)
		{
			mUserWaiting = true;
			mCondVarUser.wait(slock);
			mUserWaiting = false;
		}
	}
}


//! @brief Push graph instance current changes to render
//! @param graphInstance The instance to push dirty outputs
//! @return Return true if at least one dirty output
bool SubstanceAir::Details::RendererImpl::push(GraphInstance& graphInstance)
{
	if (mRenderJobs.empty() ||
		mRenderJobs.back()->getState()!=RenderJob::State_Setup)
	{
		// Create setup render job if necessary
		mRenderJobs.push_back(AIR_NEW(RenderJob)(++mRenderJobUid,mRenderCallbacks));
	}

	return mRenderJobs.back()->push(mStates[graphInstance],graphInstance);
}


//! @brief Launch computation
//! @param options RunOptions flags combination
//! @return Return UID of render job or 0 if not pushed computation to run
SubstanceAir::UInt SubstanceAir::Details::RendererImpl::run(
	unsigned int options,
	size_t userData)
{
	// Cleanup deprecated jobs
	cleanup();

	// Force run if pending hard resource switch
	if (mPendingHardRsc)
	{
		mPendingHardRsc = false;
		if (mRenderJobs.empty())
		{
			// Create empty render job (implies engine start w\ outputs)
			mRenderJobs.push_back(AIR_NEW(RenderJob)(++mRenderJobUid,mRenderCallbacks));
		}
	}
	else
	{
		// Otherwise, run only if non-empty jobs list
		if (mRenderJobs.empty() ||
			mRenderJobs.back()->getState()!=RenderJob::State_Setup ||
			mRenderJobs.back()->isEmpty())
		{
			// No pushed jobs
			return 0;
		}
	}

	// Skip already canceled jobs
	// and currently computed job if Run_PreserveRun flag is present
	RenderJob *curjob = mCurrentJob;    // Current job (can diverge)
	RenderJob *lastjob = curjob;        // Last job in render list
	while (curjob!=nullptr &&
		(curjob->isCanceled() ||
		((options&Renderer::Run_PreserveRun)!=0 &&
			curjob->getState()==RenderJob::State_Computing)))
	{
		lastjob = curjob;
		curjob = curjob->getNextJob();
	}

	// Retrieve last job in render list
	while (lastjob!=nullptr && lastjob->getNextJob()!=nullptr)
	{
		lastjob = lastjob->getNextJob();
	}

	RenderJob *newjob = mRenderJobs.back(); // Currently pushed render job
	RenderJob *begjob = newjob;             // List of new jobs: begin

	// Fill list of graphs to link
	newjob->snapshotStates(mStates);

	// Set user data
	newjob->setUserData(userData);

	if (curjob!=nullptr &&
		(options&(Renderer::Run_Replace|Renderer::Run_First))!=0 &&
		curjob->cancel(true))      // Proceed only if really canceled (diverge)
	{
		// Non empty render job list
		// And must cancel previous computation

		// Notify render loop that cancel operation occur
		mCancelOccur = true;

		if ((options&Renderer::Run_PreserveRun)==0)
		{
			// Stop engine if current computation is not preserved
			mEngine.stop();
		}

		const bool newfirst = (options&Renderer::Run_First)!=0;
		auto filter = (options&Renderer::Run_Replace)!=0 ? make_unique<OutputsFilter>(*newjob) : nullptr;

		// Duplicate job context / accumulation structure
		DuplicateJob dup(
			newjob->getLinkGraphs(),
			filter.get(),
			mStates,
			newfirst);   // Update state if new first

		if (!newfirst)
		{
			mRenderJobs.pop_back();       // Insert other jobs before
		}

		// Accumulate reversed state -> just before curjob
		RenderJob *srcjob;
		for (srcjob=curjob;srcjob!=nullptr;srcjob=srcjob->getNextJob())
		{
			srcjob->prependRevertedDelta(dup);
		}

		// Duplicate job
		// Iterate on canceled jobs
		RenderJob *prevdupjob = nullptr;
		for (srcjob=curjob;srcjob!=nullptr;srcjob=srcjob->getNextJob())
		{
			// Duplicate canceled
			RenderJob *dupjob = AIR_NEW(RenderJob)(*srcjob,dup,mRenderCallbacks);

			if (dupjob->isEmpty())
			{
				// All outputs filtered
				AIR_DELETE(dupjob);
			}
			else
			{
				// Valid, at least one output
				if (prevdupjob==nullptr)
				{
					// First job created, update inputs state
					if (newfirst)
					{
						// Activate duplicated job w/ current as previous
						dupjob->activate(newjob);
					}
					else
					{
						// Record as first
						begjob = dupjob;
					}
				}
				else
				{
					assert(prevdupjob!=nullptr);
					dupjob->activate(prevdupjob);
				}

				prevdupjob = dupjob;      // Store prev for next activation
			}
		}

		// Push new job at the end except if First flag is set
		if (!newfirst)
		{
			// New job at the end of list
			mRenderJobs.push_back(newjob);   // At the end

			// Activate
			if (begjob!=newjob)
			{
				// Not in case that nothing pushed
				assert(prevdupjob!=nullptr);
				newjob->activate(prevdupjob);  // Activate (last, not first)
			}

			assert(!dup.hasDelta());         // State must be reverted
		}
	}

	// Activate the first job (render thread can view and consume them)
	// Active the first in LAST! Otherwise thread unsafe behavior
	begjob->activate(lastjob);

	bool needlaunch = false;
	const bool synchrun = (options&Renderer::Run_Asynchronous)==0;

	{
		// Thread safe modifications
		Sync::unique_lock slock(mMainMutex);
		if (mCurrentJob.load()==nullptr &&                 // No more computation
			begjob->getState()==RenderJob::State_Pending)  // Not already processed
		{
			// Set as current job if no current computations
			mCurrentJob = begjob;
		}

		mHold = mHold && !synchrun;
		if (!mHold)
		{
			needlaunch = !wakeupRender();
		}
	}

	if (needlaunch)
	{
		launchRender();
	}

	if (synchrun)
	{
		assert(!mHold);

		// If synchronous run
		// Wait to be wakeup by rendering thread (render finished)
		waitRender();
	}

	return newjob->getUid();
}


//! @brief Cancel a computation or all computations
//! @param runUid UID of the render job to cancel (returned by run()), set
//!		to 0 to cancel ALL jobs.
//! @return Return true if the job is retrieved (pending)
bool SubstanceAir::Details::RendererImpl::cancel(UInt runUid)
{
	bool hasCancel = false;
	Sync::unique_lock slock(mMainMutex);
	RenderJob* currentJob = mCurrentJob.load();

	if (currentJob!=nullptr)
	{
		if (runUid!=0)
		{
			// Search for job to cancel
			for (RenderJob *rjob=currentJob;rjob!=nullptr;rjob=rjob->getNextJob())
			{
				if (runUid==rjob->getUid())
				{
					// Cancel this job
					hasCancel = rjob->cancel();
					break;
				}
			}
		}
		else
		{
			// Cancel all
			hasCancel = currentJob->cancel(true);
		}

		if (hasCancel)
		{
			// Notify render loop that cancel operation occur
			mCancelOccur = true;

			// Stop engine if necessary
			mEngine.stop();
		}
	}

	return hasCancel;
}


//! @brief Return if a computation is pending
//! @param runUid UID of the render job to retreive state (returned by run())
bool SubstanceAir::Details::RendererImpl::isPending(UInt runUid) const
{
	for (RenderJobs::const_reverse_iterator rite = mRenderJobs.rbegin();
		rite != mRenderJobs.rend();
		++rite)
	{
		const RenderJob*const renderJob = *rite;
		if (renderJob->getUid()==runUid)
		{
			return renderJob->getState() != RenderJob::State_Done;
		}
	}

	return false;
}


//! @brief Hold rendering
void SubstanceAir::Details::RendererImpl::hold()
{
	mHold = true;
	mEngine.stop();
}


//! @brief Continue held rendering
void SubstanceAir::Details::RendererImpl::resume()
{
	bool needlaunch = false;

	// Wake up render if really hold
	{
		Sync::unique_lock slock(mMainMutex);

		const bool needwakeup = mHold && mCurrentJob.load()!=nullptr;

		mHold = false;

		if (needwakeup)
		{
			needlaunch = !wakeupRender();
		}
	}

	// Launch render process
	if (needlaunch)
	{
		assert(!mHold);
		assert(mCurrentJob.load()!=nullptr);

		launchRender();
	}
}


//! @brief Flush computation, wait for all render jobs to be complete
void SubstanceAir::Details::RendererImpl::flush()
{
	// Resume if hold
	resume();

	assert(!mHold);

	// Wait for active/pending end
	waitRender();

	// Cleanup deprecated jobs
	cleanup();
}


void SubstanceAir::Details::RendererImpl::restoreRenderStates()
{
	assert(mEngineInitialized);

	mEngine.restoreRenderStates();
}


//! @brief Set new memory budget and CPU usage.
//! @param renderOptions New render options to use.
//! @note This function can be called at any time, from any thread.
void SubstanceAir::Details::RendererImpl::setOptions(
	const RenderOptions& renderOptions)
{
	mPendingHardRsc = !mEngine.setOptions(renderOptions);
}


//! @brief Switch engine dynamic library
//! @param module Dynamic library to use, result of dlopen/LoadLibrary call
//!		use nullptr to revert to default impl.
//! Flush computation and clear internal cache before switching engine
//! @return Return true if dynamic library loaded and compatible
bool SubstanceAir::Details::RendererImpl::switchEngineLibrary(void* module)
{
	// Flush pending computations
	flush();

	assert(mCurrentJob.load()==nullptr);
	assert(mRenderJobs.empty());

	// Clear all render token create w/ this engine version
	mStates.releaseRenderResults(mEngine.getInstanceUid());

	// Clear states
	mStates.clear();

	// Release engine (thread exit)
	exitRender();

	// Switch engine library
	return mEngine.switchLibrary(module);
}


//! @brief Set user callbacks
//! @param callbacks Pointer on the user callbacks concrete structure
//! 	instance or nullptr.
//! @warning The callbacks instance must remains valid until all
//!		render job created w/ this callback instance set are processed.
void SubstanceAir::Details::RendererImpl::setRenderCallbacks(
	RenderCallbacks* callbacks)
{
	mRenderCallbacks = callbacks;
}


//! @brief Retrieve currently dynamic-linked lib version
//! @return Return SubstanceVersion object from active engine
SubstanceVersion SubstanceAir::Details::RendererImpl::getCurrentVersion() const
{
	return mEngine.getVersion();
}


//! @brief Wake up render thread if currently locked into mCondVarRender
//! @pre mMainMutex Must be locked
//! @post If thread loop was waiting, switch to OnGoing state 
//! @note Called from user thread
//! @brief Return false if wakeup failed: the render thread is idle and
//!		a launch is necessary
bool SubstanceAir::Details::RendererImpl::wakeupRender()
{
	const bool res = mRenderState!=RenderState_Idle;

	if (mRenderState==RenderState_Wait)
	{
		mRenderState = RenderState_OnGoing;    // Set render state immediatly, required if immediatly waiting

		// Render thread used, resume render loop
		mCondVarRender.notify_one();
	}

	return res;
}


//! @brief Call process callback or create up render thread
//! @pre mMainMutex Must be NOT locked, render thread must be currently
//!		idle.
//! @note Called from user thread
void SubstanceAir::Details::RendererImpl::launchRender()
{
	assert(mRenderState==RenderState_Idle);

	mEngineInitialized = true;
	mRenderState = RenderState_OnGoing;    // Set render state immediatly, prevent to launching twice!

	// Try to use process callback
	if (mRenderCallbacks==nullptr || !mRenderCallbacks->runRenderProcess(
		&RendererImpl::renderProcess,
		this))
	{
		// Otherwise create render thread
		mThread = Sync::thread(&renderThread,this);
	}
}


//! @brief Clean consumed render jobs
//! @note Called from user thread
void SubstanceAir::Details::RendererImpl::cleanup()
{
	while (!mRenderJobs.empty() &&
		mRenderJobs.front()->getState()==RenderJob::State_Done)
	{
		Utils::checkedDelete<RenderJob>(mRenderJobs.front());
		mRenderJobs.pop_front();
	}
}


//! @brief Rendering thread call function
void SubstanceAir::Details::RendererImpl::renderThread(RendererImpl* impl)
{
	impl->renderLoop(true);
}


//! @brief Rendering process call function
void SubstanceAir::Details::RendererImpl::renderProcess(void* impl)
{
	((RendererImpl*)impl)->renderLoop(false);
}


//! @brief Rendering loop function
//! @param waitAndLoop Starvation behavior flag: if true, wait using
//!		mCondVarRender for new job to process (or exit), otherwise exit
//!		immediately (revert to State_Idle state).
void SubstanceAir::Details::RendererImpl::renderLoop(bool waitAndLoop)
{
	RenderJob* nextJob = nullptr;              // Next job to proceed
	bool nextAvailable = false;             // Flag: next job available

	while (1)
	{
		// Job processing loop, exit when Renderer is deleted
		{
			// Thread safe operations
			Sync::unique_lock slock(mMainMutex); // mState/mHold safety

			if (nextAvailable)
			{
				// Jump to next job
				assert(mCurrentJob.load()!=nullptr);
				mCurrentJob = nextJob;
				nextAvailable = false; // NOLINT
			}

			while (mExitRender ||
				mHold ||
				mCurrentJob.load()==nullptr)
			{
				if (mExitRender)
				{
					mEngine.releaseEngine();        // Release engine
					mExitRender = false;            // Exit render taken account
					waitAndLoop = false;            // Exit loop
				}

				if (mUserWaiting)
				{
					mCondVarUser.notify_one();      // Wake up user thread
				}

				if (waitAndLoop)
				{
					mRenderState = RenderState_Wait;
					mCondVarRender.wait(slock);     // Wait to be wakeup
				}
				else
				{
					mRenderState = RenderState_Idle;
					return;                         // Exit function
				}
			}

			assert(mRenderState==RenderState_OnGoing);
		}

		// Release pending textures
		mEngine.releaseTextures();

		// New hardware resource will be used
		mPendingHardRsc = false;

		// Process current job
		nextJob = processJob(mCurrentJob);
		nextAvailable = nextJob!=mCurrentJob;
	}
}


//! @brief Process job and next ones
//! @param begjob The first job to process
//! @note Called from render thread
//! @post Job is in 'Done' state if not canceled/hold
//! @warning The job can be deleted by user thread BEFORE returning process().
//! @return Return next job to proceed if the job is fully completed
//!		(may be nullptr) and is possible to jump to next job. Return begjob if
//!		NOT completed.
SubstanceAir::Details::RenderJob*
SubstanceAir::Details::RendererImpl::processJob(RenderJob *begjob)
{
	// Iterate on jobs to render, find last one or first w/ link collision
	// Check if not strict resume (jobs are already pulled, w\ cancel or addon)
	// Check if link required
	RenderJob::ProcessJobs process(begjob);
	bool strictresume = !mCancelOccur;
	RenderCallbacks*const callbacks = process.first->getCallbacks();

	// Cancel will be accounted for
	mCancelOccur = false;

	// Enqueue jobs, returns if strict resume
	strictresume = !process.enqueue() && strictresume;

	// Collect all graph states for linking step and/or
	// reset link state process if collision occurs
	// But keep the actual jobs to render
	LinkGraphs linkgraphs;
	if (process.linkNeeded || process.linkCollision)
	{
		RenderJob* curjob = process.first;
		while (true)
		{
			linkgraphs.merge(curjob->getLinkGraphs());
			if (curjob==process.last)
			{
				break;
			}
			curjob = curjob->getNextJob();
		}
	}

	// Link if necessary
	if (process.linkNeeded)
	{
		assert(!strictresume);
		if (callbacks!=nullptr) callbacks->profileEvent(ProfileEvent::LinkBegin);
		mEngine.link(process.first->getCallbacks(),linkgraphs);
		if (callbacks!=nullptr) callbacks->profileEvent(ProfileEvent::LinkEnd);
	}

	// Compute (only if something linked, test necessary w/ run forced to
	// update hardware resources)
	if (mEngine.getHandle()!=nullptr)
	{
		// Create computation context
		// Flush internal engine queue if push I/O needed
		Computation computation(mEngine,!strictresume);

		// If pull is necessary (not strict resume)
		if (!strictresume)
		{
			// Push I/O to engine (not already pushed, until end or collision)
			process.pull(computation);
		}

		// Run if no cancel occurs during push/link
		if (!mCancelOccur)
		{
			if (callbacks!=nullptr) callbacks->profileEvent(ProfileEvent::ComputeBegin);
			computation.run();
			if (callbacks!=nullptr) callbacks->profileEvent(ProfileEvent::ComputeEnd);
		}
	}

	// If collision occurs, next iteration will relink: reset link state
	if (process.linkCollision && !mCancelOccur)
	{
		assert(!strictresume);
		for (const auto& state : linkgraphs.graphStates)
		{
			state->getBinary().resetLinked();
		}
	}

	// Mark completed jobs as Done
	bool allcomplete = false;
	RenderJob* curjob = process.first;
	while (!allcomplete && curjob->isComplete())
	{
		RenderJob* nextjob = curjob->getNextJob();
		curjob->setComplete();                // may be destroyed just here!
		allcomplete = curjob==process.last;
		curjob = nextjob;
	}

	return curjob;
}
