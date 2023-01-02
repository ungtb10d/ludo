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

#include "detailsrenderjob.h"
#include "detailsduplicatejob.h"
#include "detailsrenderpushio.h"
#include "detailsgraphstate.h"
#include "detailsstates.h"
#include "detailsengine.h"
#include "detailsutils.h"
#include "detailscomputation.h"

#include <substance/framework/callbacks.h>
#include <substance/framework/graph.h>

#include <algorithm>

#include <assert.h>


//! @brief Constructor
//! @param callbacks User callbacks instance (or nullptr if none)
SubstanceAir::Details::RenderJob::RenderJob(
		UInt uid,
		RenderCallbacks *callbacks) :
	mUid(uid|0x80000000u),
	mUserData(0),
	mState(State_Setup),
	mCanceled(ATOMIC_VAR_INIT(false)),
	mNextJob(nullptr),
	mCallbacks(callbacks),
	mEngine(nullptr)
{
}


//! @brief Constructor from job to duplicate and outputs filtering
//! @param src The canceled job to copy
//! @param dup Duplicate job context
//! @param callbacks User callbacks instance (or nullptr if none)
//!
//! Build a pending job from a canceled one and optionally pointer on an
//! other job used to filter outputs.
//! Push again SRC render tokens (of not filtered outputs).
//! @warning Resulting job can be empty if all outputs are filtered. In this
//!		case this job is no more useful and can be removed.
//! @note Called from user thread
SubstanceAir::Details::RenderJob::RenderJob(
		const RenderJob& src,
		DuplicateJob& dup,
		RenderCallbacks *callbacks) :
	mUid(src.getUid()),
	mUserData(src.getUserData()),
	mState(State_Setup),
	mCanceled(ATOMIC_VAR_INIT(false)),
	mNextJob(nullptr),
	mLinkGraphs(dup.linkGraphs),
	mCallbacks(callbacks)
{
	mRenderPushIOs.reserve(src.mRenderPushIOs.size());

	for (const auto& srcpushio : src.mRenderPushIOs)
	{
		RenderPushIO *newpushio =
			AIR_NEW(RenderPushIO)(*this,*srcpushio,dup);
		if (newpushio->hasOutputs())
		{
			// Has outputs, push it in the list
			mRenderPushIOs.push_back(newpushio);
		}
		else
		{
			// No outputs, all filtered
			Utils::checkedDelete(newpushio);
		}
	}
}


//! @brief Destructor
SubstanceAir::Details::RenderJob::~RenderJob()
{
	// Delete RenderPushIO elements
	std::for_each(
		mRenderPushIOs.begin(),
		mRenderPushIOs.end(),
		Utils::Deleter<RenderPushIO>());
}


//! @brief Take states snapshot (used by linker)
//! @param states Used to take a snapshot states to use at link time
//!
//! Must be done before activate it
void SubstanceAir::Details::RenderJob::snapshotStates(const States &states)
{
	states.fill(mLinkGraphs);        // Create snapshot from current states
}


//! @brief Push I/O to render: from current state & current instance
//! @param graphState The current graph state
//! @param graphInstance The pushed graph instance (not kept)
//! @pre Job must be in 'Setup' state
//! @note Called from user thread
//! @return Return true if at least one dirty output
//!
//! Update states, create render tokens.
bool SubstanceAir::Details::RenderJob::push(
	GraphState &graphState,
	const GraphInstance &graphInstance)
{
	assert(State_Setup==mState);

	// Get push IO index
	UInt pushioindex = mStateUsageCount[graphState.getUid()]++;

	assert(pushioindex<=mRenderPushIOs.size());
	const bool newpushio = mRenderPushIOs.size()==(size_t)pushioindex;
	if (newpushio)
	{
		// Create new push IO
		mRenderPushIOs.push_back(AIR_NEW(RenderPushIO)(*this));
	}

	// Push!
	if (mRenderPushIOs.at(pushioindex)->push(graphState,graphInstance))
	{
		return true;
	}
	else if (newpushio)
	{
		// No dirty outputs: Remove just created, not necessary
		Utils::checkedDelete(mRenderPushIOs.back());
		mRenderPushIOs.pop_back();
		--mStateUsageCount[graphState.getUid()];
	}

	return false;
}


//! @brief Put the job in render state, build the render chained list
//! @param previous Previous job in render list or nullptr if first
//! @pre Activation must be done in reverse order, this next job is already
//!		activated.
//! @pre Job must be in 'Setup' state
//! @post Job is in 'Pending' state
//! @note Called from user thread
void SubstanceAir::Details::RenderJob::activate(RenderJob* previous)
{
	assert(State_Setup==mState);

	mState = State_Pending;

	if (previous!=nullptr)
	{
		assert(previous->mNextJob.load()==nullptr);
		previous->mNextJob = this;
	}
}


//! @brief Cancel this job (or this job and next ones)
//! @param cancelList If true this job and next ones (mNextJob chained list)
//! 	are canceled (in reverse order).
//! @note Called from user thread
//! @return Return true if at least one job is effectively canceled
//!
//! Cancel if Pending or Computing.
//! Outputs of canceled jobs are not computed, only inputs are set (state
//! coherency). RenderToken's are notified as canceled.
bool SubstanceAir::Details::RenderJob::cancel(bool cancelList)
{
	bool res = false;
	RenderJob* nextJob = mNextJob.load();

	if (cancelList && nextJob!=nullptr)
	{
		// Recursive call, reverse chained list order canceling
		res = nextJob->cancel(true);
	}

	if (!mCanceled)
	{
		res = true;
		mCanceled = true;

		// notify cancel for each push I/O (needed for RenderToken counter decr)
		for (RenderPushIOs::const_reverse_iterator pioite = mRenderPushIOs.rbegin();
			pioite != mRenderPushIOs.rend();
			++pioite)
		{
			(*pioite)->cancel();
		}
	}

	return res;
}


//! @brief Prepend reverted input delta into duplicate job context
//! @param dup The duplicate job context to accumulate reversed delta
//! Use to restore the previous state of copied jobs.
//! @note Called from user thread
void SubstanceAir::Details::RenderJob::prependRevertedDelta(
	DuplicateJob& dup) const
{
	for (const auto& pushio : mRenderPushIOs)
	{
		pushio->prependRevertedDelta(dup);
	}
}


//! @brief Fill filter outputs structure from this job
//! @param[in,out] filter The filter outputs structure to fill
void SubstanceAir::Details::RenderJob::fill(OutputsFilter& filter) const
{
	for (const auto& pushio : mRenderPushIOs)
	{
		pushio->fill(filter);
	}
}

//! @brief Mark as complete
//! Called from render thread.
void SubstanceAir::Details::RenderJob::setComplete()
{
	mState = State_Done;

	if (mCallbacks)
	{
		mCallbacks->jobComputed(getUid(), getUserData());
	}
}

//! @brief Push input and output in engine handle
void SubstanceAir::Details::RenderJob::pull(Computation &computation)
{
	mState = State_Computing;
	mEngine = &computation.getEngine();
	const bool canceled = mCanceled;

	for (const auto& pushio : mRenderPushIOs)
	{
		// If canceled, only inputs
		if (!pushio->pull(computation,canceled) && !canceled)
		{
			// Not into process early exit
			break;
		}
	}
}


//! @brief Is render job complete
//! @return Return if all push I/O completed
bool SubstanceAir::Details::RenderJob::isComplete() const
{
	for (RenderPushIOs::const_reverse_iterator pioite = mRenderPushIOs.rbegin();
		pioite != mRenderPushIOs.rend();
		++pioite)
	{
		// Reverse test, ordered computation
		const RenderPushIO::Complete complete = (*pioite)->isComplete(mCanceled);

		// Complete if last push I/O complete or only Inputs pushed only if
		// job canceled or no outputs to push
		if (RenderPushIO::Complete_DontKnow!=complete)
		{
			return RenderPushIO::Complete_Yes==complete;
		}
	}

	return true;
}


bool SubstanceAir::Details::RenderJob::enqueueProcess(ProcessJobs& processJobs)
{
	assert(mState==State_Pending || mState==State_Computing);

	mRenderPushIOs.begin();

	for (RenderPushIOs::const_iterator ioite = firstNonComplete(); // Skip complete
		ioite!=mRenderPushIOs.end();
		++ioite)
	{
		switch ((*ioite)->enqueueProcess())
		{
			case RenderPushIO::Process_LinkRequired:
				processJobs.linkNeeded = true;
			// no break here

			case RenderPushIO::Process_Append:
				processJobs.last = this;          // At least one to process
			break;

			case RenderPushIO::Process_Collision:
			{
				processJobs.linkCollision = true;
				return false;                     // Stop iterating
			}
			break;
		}
	}

	return getNextJob()!=nullptr;
}


bool SubstanceAir::Details::RenderJob::needResume() const
{
	assert(mState==State_Pending || mState==State_Computing);

	if (isCanceled())
	{
		// Do not resume if canceled
		return false;
	}

	// Skip complete
	RenderPushIOs::const_iterator ioite = firstNonComplete();

	assert(ioite!=mRenderPushIOs.end() ||    // Should be skipped before
		mRenderPushIOs.empty());             // Or hard res. change only run

	// True if first non complete is still in process
	return ioite!=mRenderPushIOs.end() &&
		((*ioite)->getState()&RenderPushIO::State_Process)!=0;
}


bool SubstanceAir::Details::RenderJob::ProcessJobs::enqueue()
{
	bool resumeonly = true;

	assert(first!=nullptr);

	// Test if not strict resume
	for (RenderJob* curjob=first;
		curjob!=nullptr && resumeonly;
		curjob=curjob->getNextJob())
	{
		resumeonly = curjob->needResume();
		last = curjob;                         // Last job updated
	}

	if (!resumeonly)
	{
		// Enqueue to process job list if not strict resume
		RenderJob* curjob = first;
		last = first;                          // Restore last
		while (curjob->enqueueProcess(*this))
		{
			curjob = curjob->getNextJob();     // Next if no collision or end
			assert(curjob!=nullptr);
		}
	}

	return !resumeonly;
}


void SubstanceAir::Details::RenderJob::ProcessJobs::pull(
	Computation &computation) const
{
	RenderJob* curjob = first;
	do
	{
		// Push I/O
		curjob->pull(computation);
	}
	while (curjob!=last && (curjob=curjob->getNextJob())!=nullptr);
}


SubstanceAir::Details::RenderJob::RenderPushIOs::const_iterator
SubstanceAir::Details::RenderJob::firstNonComplete() const
{
	RenderPushIOs::const_iterator ioite = mRenderPushIOs.begin();

	// Skip complete
	while (ioite!=mRenderPushIOs.end() &&
		(*ioite)->isComplete()!=RenderPushIO::Complete_No)
	{
		++ioite;
	}

	return ioite;
}
