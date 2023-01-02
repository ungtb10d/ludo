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

#include "detailsrenderpushio.h"
#include "detailsduplicatejob.h"
#include "detailscomputation.h"
#include "detailsgraphbinary.h"
#include "detailsgraphstate.h"
#include "detailsrenderjob.h"
#include "detailsrendertoken.h"
#include "detailsoutputsfilter.h"
#include "detailsstates.h"
#include "detailsutils.h"
#include "detailsinputimagetoken.h"

#include <substance/framework/graph.h>
#include <substance/framework/callbacks.h>

#include <iterator>
#include <algorithm>

#include <assert.h>


//! @brief Create from render job
//! @param renderJob Parent render job that owns this instance
//! @note Called from user thread
SubstanceAir::Details::RenderPushIO::RenderPushIO(RenderJob &renderJob) :
	mRenderJob(renderJob),
	mState(ATOMIC_VAR_INIT(State_None))
{
}

//! @brief Constructor from push I/O to duplicate
//! @param renderJob Parent render job that owns this instance
//! @param src The canceled push I/O to copy
//! @param dup Duplicate job context
//! Build a push I/O from a canceled one and optionally filter outputs.
//! Push again SRC render tokens (of not filtered outputs).
//! @warning Resulting push I/O can be empty (hasOutputs()==false) if all
//!		outputs are filtered OR already computed. In this case this push IO
//!		is no more useful and can be removed.
//! @note Called from user thread
SubstanceAir::Details::RenderPushIO::RenderPushIO(
		RenderJob &renderJob,
		const RenderPushIO& src,
		DuplicateJob& dup) :
	mRenderJob(renderJob),
	mState(ATOMIC_VAR_INIT(State_None))
{
	OutputsFilter::Outputs foutsdummy;

	mInstances.reserve(src.mInstances.size());
	for (const auto& srcinst : src.mInstances)
	{
		// Skip if instance deleted
		const UInt uid = srcinst->instanceUid;
		GraphInstance* ginst = dup.states.getInstanceFromUid(uid);
		if (ginst==nullptr)
		{
			// Deleted
			continue;
		}

		Instance *newinst = AIR_NEW(Instance)(*srcinst);

		// Filter outputs
		OutputsFilter::Outputs::const_iterator foutitecur = foutsdummy.end();
		OutputsFilter::Outputs::const_iterator foutiteend = foutitecur;
		if (dup.filter!=nullptr)
		{
			OutputsFilter::Instances::const_iterator finstite =
				dup.filter->instances.find(uid);
			if (finstite!=dup.filter->instances.end())
			{
				// If filtered and instance found, get iterator on indices
				foutitecur = finstite->second.begin();
				foutiteend = finstite->second.end();
			}
		}

		newinst->outputs.reserve(srcinst->outputs.size());
		for (const auto& srcout : srcinst->outputs)
		{
			// Iterate on filtered outputs indices list in same time
			while (foutitecur!=foutiteend && (*foutitecur)<srcout.index)
			{
				++foutitecur;
			}

			if (!srcout.renderToken->isComputed() &&  // Skip if is computed
				(foutitecur==foutiteend ||
					(*foutitecur)!=srcout.index))     // Skip if filtered
			{
				// Duplicate output
				newinst->outputs.push_back(srcout);

				// Push again SRC render tokens
				srcout.renderToken->incrRef();
				assert(ginst->getOutputs().size()>srcout.index);
				ginst->getOutputs()[srcout.index]->push(srcout.renderToken);
			}
		}

		if (newinst->outputs.empty())
		{
			// Pruned, accumulate delta state for next one
			dup.append(uid,newinst->deltaState);

			// Delete instance
			Utils::checkedDelete(newinst);
		}
		else
		{
			// Has outputs, pust it
			mInstances.push_back(newinst);

			// Fix delta state w/ accumulated delta
			dup.fix(uid,newinst->deltaState);

			// Update state if necessary
			if (dup.needUpdateState())
			{
				newinst->graphState.apply(newinst->deltaState);
			}
		}
	}
}


//! @brief Destructor
//! @note Called from user thread
SubstanceAir::Details::RenderPushIO::~RenderPushIO()
{
	// Delete instances elements
	std::for_each(
		mInstances.begin(),
		mInstances.end(),
		Utils::Deleter<Instance>());
}


//! @brief Create a push I/O pair from current state & current instance
//! @param graphState The current graph state
//! @param graphInstance The pushed graph instance (not kept)
//! @note Called from user thread
//! @return Return true if at least one dirty output
bool SubstanceAir::Details::RenderPushIO::push(
	GraphState &graphState,
	const GraphInstance &graphInstance)
{
	Instance *instance = AIR_NEW(Instance)(graphState,graphInstance);

	if (instance->outputs.empty())
	{
		// No outputs, (not necessary to use hasOutputs()), delete it
		AIR_DELETE(instance);
		return false;
	}

	mInstances.push_back(instance);

	return true;
}


//! @brief Prepend reverted input delta into duplicate job context
//! @param dup The duplicate job context to accumulate reversed delta
//! Use to restore the previous state of copied jobs.
//! @note Called from user thread
void SubstanceAir::Details::RenderPushIO::prependRevertedDelta(
	DuplicateJob& dup) const
{
	for (const auto& instance : mInstances)
	{
		dup.prepend(instance->instanceUid,instance->deltaState);
	}
}


bool SubstanceAir::Details::RenderPushIO::pull(
	Computation &computation,
	bool inputsOnly)
{
	if ((mState&State_Process)==0 && !inputsOnly)
	{
		// Not into process, pull iteration should end
		return false;
	}

	if (isComplete(false)==Complete_Yes)
	{
		// Skip completed push I/O
		return true;
	}

	// Set user data for callback
	computation.setUserData((size_t)this);

	// Reset data built during pull
	assert(inputsOnly || (mState&State_Process)!=0);

	std::fill(
		mDestOutputs.begin(),
		mDestOutputs.end(),
		(const Output*)nullptr);
	mInputJobPendingCount = 0;

	// Output SBSBIN indices to push
	vector<UInt> indicesout;

	for (const auto& instance : mInstances)
	{
		const GraphBinary& binary = instance->graphState.getBinary();

		// Push inputs
		for (const auto& input : instance->deltaState.getInputs())
		{
			assert(binary.inputs.size()>input.index);
			const UInt binindex = binary.inputs[input.index].index;
			assert(binindex!=GraphBinary::invalidIndex);

			if (binindex!=GraphBinary::invalidIndex)
			{
				// Only if present in SBSBIN
				void *inpval = nullptr;
				const InputState& inpstate = input.modified;
				if (inpstate.isImage())
				{
					if (inpstate.isIndexValid())
					{
						// Image input, get pointer
						InputImage::SPtr imgptr = instance->deltaState.getInputImage(
							inpstate.value.imagePtrIndex);
						assert(imgptr);
						if (imgptr)
						{
							InputImageToken *imgtkn = imgptr->getToken();
							if (imgtkn!=nullptr)
							{
								inpval = imgtkn->texture.textureInputAny;
							}
						}
					}
				}
				else if (inpstate.isString())
				{
					if (inpstate.isIndexValid())
					{
						// String input, get pointer
						ucs4string& inpstr = *instance->deltaState.getInputString(
							inpstate.value.stringPtrIndex).get();
						if (!inpstr.empty())
						{
							inpval = &inpstr[0];
						}
					}
				}
				else
				{
					// Numeric, pointer on value
					inpval = const_cast<void*>((const void*)inpstate.value.numeric);
				}

				if (computation.pushInput(binindex,inpstate,inpval))
				{
					// If NOT only hint
					++mInputJobPendingCount;       // One more pending input job
					mState |= State_InputsPending; // Has inputs pending
				}

			}
		}

		if (!inputsOnly)
		{
			// Append to outputs SBSBIN indices list
			indicesout.reserve(indicesout.size()+instance->outputs.size());
			for (const auto& output : instance->outputs)
			{
				assert(binary.outputs.size()>output.index);
				if (!output.renderToken->isComputed())
				{
					// Only if not already computed
					const UInt binindex = binary.outputs[output.index].index;
					assert(binindex!=GraphBinary::invalidIndex);
					if (binindex!=GraphBinary::invalidIndex)
					{
						// Only if present in SBSBIN
						indicesout.push_back(binindex);

						// Has outputs pending
						mState |= State_OutputsPending;

						// Fill render token
						if (mDestOutputs.size()<=binindex)
						{
							std::fill_n(
								std::back_inserter(mDestOutputs),
								1+(size_t)binindex-mDestOutputs.size(),
								(const Output*)nullptr);
						}
						mDestOutputs[binindex] = &output;
					}
				}
			}
		}
	}

	if (!indicesout.empty())
	{
		// Push outputs
		computation.pushOutputs(indicesout);
	}

	return true;
}


SubstanceAir::Details::Engine*
SubstanceAir::Details::RenderPushIO::getEngine() const
{
	return mRenderJob.getEngine();
}


//! @brief Fill filter outputs structure from this push IO
//! @param[in,out] filter The filter outputs structure to fill
void SubstanceAir::Details::RenderPushIO::fill(OutputsFilter& filter) const
{
	for (const auto& instance : mInstances)
	{
		OutputsFilter::Outputs intermouts;
		OutputsFilter::Outputs &instouts =
			filter.instances[instance->instanceUid];
		OutputsFilter::Outputs* foutputs = instouts.empty() ?
			&instouts :
			&intermouts;

		// Get output indices
		foutputs->reserve(instance->outputs.size());
		for (const auto& output : instance->outputs)
		{
			foutputs->push_back(output.index);
		}

		if (foutputs==&intermouts)
		{
			// If previous data, unite both lists
			OutputsFilter::Outputs mergedouts;
			mergedouts.reserve(instouts.size()+intermouts.size());

			std::set_union(
				instouts.begin(),
				instouts.end(),
				intermouts.begin(),
				intermouts.end(),
				std::back_inserter(mergedouts));

			assert(mergedouts.size()>=instouts.size() &&
				mergedouts.size()>=intermouts.size());
			instouts = mergedouts;
		}
	}
}


SubstanceAir::Details::RenderPushIO::Process
SubstanceAir::Details::RenderPushIO::enqueueProcess()
{
	bool linkneeded = false;
	const bool alreadyprocess = (mState&State_Process)!=0;

	if (!alreadyprocess)
	{
		// Verify first if no collision
		for (const auto& instance : mInstances)
		{
			const GraphBinary& graphbin = instance->graphState.getBinary();
			for (const auto& outstate : instance->deltaState.getOutputs())
			{
				if (graphbin.outputs.at(outstate.index).enqueuedLink)
				{
					// Already pending override for this link
					return Process_Collision;
				}
			}
		}
	}

	for (auto& instance : mInstances)
	{
		GraphBinary& graphbin = instance->graphState.getBinary();

		if (!alreadyprocess)
		{
			linkneeded = linkneeded || !graphbin.isLinked();
			for (const auto& outstate : instance->deltaState.getOutputs())
			{
				GraphBinary::Output& outbin = graphbin.outputs.at(outstate.index);
				assert(!outbin.enqueuedLink);
				outbin.state = outstate.modified;
				linkneeded = true;
			}
		}

		// Mark all output of instances as used
		for (const auto& output : instance->outputs)
		{
			GraphBinary::Output& outbin = graphbin.outputs.at(output.index);
			outbin.enqueuedLink = true;
		}
	}

	assert(!alreadyprocess || !linkneeded);

	mState |= State_Process;

	return linkneeded ?
		Process_LinkRequired :
		Process_Append;
}


//! @brief Cancel notification, decrease RenderToken counters
void SubstanceAir::Details::RenderPushIO::cancel()
{
	for (const auto& instance : mInstances)
	{
		for (const auto& output : instance->outputs)
		{
			output.renderToken->canceled();
		}
	}
}


//! @brief Accessor: At least one output to compute
//! Check all render tokens if not already filled
bool SubstanceAir::Details::RenderPushIO::hasOutputs() const
{
	for (const auto& instance : mInstances)
	{
		if (instance->hasOutputs())
		{
			return true;
		}
	}

	return false;
}


//! @brief Return if the current Push I/O is completed
//! @param inputOnly Only inputs are required
//! @note Called from Render queue thread
//! @return Can return "don't know" if no pending I/O
SubstanceAir::Details::RenderPushIO::Complete
SubstanceAir::Details::RenderPushIO::isComplete(bool inputOnly) const
{
	if (State_None==mState)
	{
		return Complete_No;
	}
	else if (State_Process==mState)
	{
		return Complete_DontKnow;
	}

	const UInt flags = inputOnly || (mState&State_OutputsPending)==0 ?
		State_InputsPushed|State_InputsPending :
		State_OutputsComputed|State_OutputsPending;
	return (flags&mState)==flags ?
		Complete_Yes :
		Complete_No;
}


//! @brief Engine job completed
//! @note Called by engine callback from Render queue thread
void SubstanceAir::Details::RenderPushIO::callbackJobComplete()
{
	assert(mInputJobPendingCount>=0);
	--mInputJobPendingCount;

	if (mInputJobPendingCount==0)
	{
		// 0 reached, all inputs processed
		assert((mState&State_InputsPending)!=0);
		assert((mState&State_InputsPushed)==0);
		mState |= State_InputsPushed;
	}
	else if (mInputJobPendingCount==-1)
	{
		// -1 reached, outputs computed
		assert((mState&State_OutputsPending)!=0);
		assert((mState&State_OutputsComputed)==0);
		mState |= State_OutputsComputed;
	}
}


SubstanceIOType SubstanceAir::Details::RenderPushIO::getOutputType(UInt index) const
{
	assert(index<mDestOutputs.size());
	assert(mDestOutputs[index]!=nullptr);

	return mDestOutputs[index]->type;
}


//! @brief Engine output completed
//! @param index Output SBSBIN index of completed output
//! @param renderResult The result texture, ownership transfered
//! @note Called by engine callback from Render queue thread
void SubstanceAir::Details::RenderPushIO::callbackOutputComplete(
	UInt index,
	RenderResultBase* renderResult)
{
	assert(index<mDestOutputs.size());
	assert(mDestOutputs[index]!=nullptr);

	const Output& output = *mDestOutputs[index];
	output.renderToken->fill(renderResult);

	// Emit callback if plugged
	RenderCallbacks* callbacks = mRenderJob.getCallbacks();
	if (callbacks!=nullptr)
	{
		callbacks->outputComputed(
			mRenderJob.getUid(),
			mRenderJob.getUserData(),
			output.graphInstance,
			output.outputInstance);
	}
}


//! @brief Build a push I/O pair from current state & current instance
//! @param state The current graph state
//! @param graphInstance The pushed graph instance (not kept)
SubstanceAir::Details::RenderPushIO::Instance::Instance(
		GraphState &state,
		const GraphInstance &graphInstance) :
	graphState(state),
	instanceUid(graphInstance.mInstanceUid)
{
	UInt outindex = 0;
	outputs.reserve(graphInstance.getOutputs().size());
	for (const auto& srcout : graphInstance.getOutputs())
	{
		if (srcout->mEnabled && srcout->queueRender())
		{
			// Push output to compute
			outputs.resize(outputs.size()+1);
			Output &newout = outputs.back();
			newout.index = outindex;
			newout.type = srcout->mDesc.mType;
			newout.outputInstance = srcout;
			newout.graphInstance = &graphInstance;

			// Create render token
			newout.renderToken = make_shared<RenderToken>();
			srcout->push(newout.renderToken);
		}
		++outindex;
	}

	if (!outputs.empty())
	{
		// Create input delta
		deltaState.fill(graphState,graphInstance);

		// Update state
		graphState.apply(deltaState);
	}
}


//! @brief Duplicate instance (except outputs)
SubstanceAir::Details::RenderPushIO::Instance::Instance(const Instance &src) :
	graphState(src.graphState),
	instanceUid(src.instanceUid),
	deltaState(src.deltaState)
{
}


//! @brief Accessor: At least one output to compute
//! Check all render tokens if not already filled
bool SubstanceAir::Details::RenderPushIO::Instance::hasOutputs() const
{
	for (const auto& output : outputs)
	{
		if (!output.renderToken->isComputed())
		{
			return true;
		}
	}

	return false;
}


