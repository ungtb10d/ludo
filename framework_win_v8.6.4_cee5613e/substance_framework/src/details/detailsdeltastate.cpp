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

#include "detailsdeltastate.h"
#include "detailsrenderjob.h"
#include "detailsgraphstate.h"
#include "detailsutils.h"

#include <substance/framework/graph.h>

#include <algorithm>
#include <iterator>

#include <assert.h>


//! @brief Fill a delta state from current state & current instance values
//! @param graphState The current graph state used to generate delta
//! @param graphInstance The pushed graph instance (not keeped)
void SubstanceAir::Details::DeltaState::fill(
	const GraphState &graphState,
	const GraphInstance &graphInstance)
{
	size_t inpindex = 0;
	for (const auto& inpinst : graphInstance.getInputs())
	{
		const InputState &inpst = graphState.getInput(inpindex);
		assert(inpinst->mDesc.mType==inpst.getType());
		const bool ismod = inpinst->isModified(inpst.value.numeric);

		if (ismod || inpinst->mUseCache)
		{
			// Different or cache forced, create delta entry
			mInputs.resize(mInputs.size()+1);
			Input &inpdelta = mInputs.back();
			inpdelta.index = inpindex;
			inpdelta.modified.flags = inpst.getType();
			inpdelta.previous = inpst;

			if (inpst.isImage() &&
				inpst.value.imagePtrIndex!=InputState::invalidIndex)
			{
				// Save previous pointer
				inpdelta.previous.value.imagePtrIndex = mInputImagePtrs.size();
				mInputImagePtrs.push_back(
					graphState.getInputImage(inpst.value.imagePtrIndex));
			}
			else if (inpst.isString() &&
				inpst.value.stringPtrIndex!=InputState::invalidIndex)
			{
				// Save previous string
				inpdelta.previous.value.stringPtrIndex = mInputStringPtrs.size();
				mInputStringPtrs.push_back(
					graphState.getInputString(inpst.value.stringPtrIndex));
			}

			if (ismod)
			{
				// Really modified,
				inpdelta.modified.fillValue(
					inpinst,
					mInputImagePtrs,
					mInputStringPtrs);
			}
			else
			{
				// Only for cache
				inpdelta.modified.value = inpdelta.previous.value;
				inpdelta.modified.flags |= InputState::Flag_CacheOnly;
			}

			if (!inpinst->mIsHeavyDuty)
			{
				inpdelta.modified.flags |= InputState::Flag_Cache;
			}
		}

		++inpindex;
	}

	size_t outindex = 0;
	for (const auto& outinst : graphInstance.getOutputs())
	{
		const OutputState &outst = graphState.getOutput(outindex);

		if (outst.formatOverridden!=outinst->isFormatOverridden() ||
			(outst.formatOverridden &&
				!(outst.formatOverride==outinst->getFormatOverride())))
		{
			// Different, create delta entry
			mOutputs.resize(mOutputs.size()+1);
			Output &outdelta = mOutputs.back();
			outdelta.index = outindex;
			outdelta.modified.fill(*outinst);
			outdelta.previous = outst;
		}

		++outindex;
	}
}


namespace SubstanceAir
{
namespace Details
{

template <>
bool DeltaState::isEqual(
	const InputState& a,
	const InputState& b,
	const DeltaState &bparent) const
{
	assert(a.isImage()==b.isImage());
	assert(a.isString()==b.isString());
	if (b.isImage())
	{
		// Image pointer equality
		const bool bvalid = b.value.imagePtrIndex!=InputState::invalidIndex;
		if (bvalid!=(a.value.imagePtrIndex!=InputState::invalidIndex))
		{
			return false;          // One valid, not the other
		}

		return !bvalid ||
			mInputImagePtrs[a.value.imagePtrIndex]==
				bparent.mInputImagePtrs[b.value.imagePtrIndex];
	}
	else if (b.isString())
	{
		// string pointer equality
		const bool bvalid = b.value.stringPtrIndex!=InputState::invalidIndex;
		if (bvalid!=(a.value.stringPtrIndex!=InputState::invalidIndex))
		{
			return false;          // One valid, not the other
		}

		return !bvalid ||
			mInputStringPtrs[a.value.stringPtrIndex]==
				bparent.mInputStringPtrs[b.value.stringPtrIndex];
	}
	else
	{
		// Numeric equality
		return std::equal(
			b.value.numeric,
			b.value.numeric+getComponentsCount(b.getType()),
			a.value.numeric);
	}
}


template <>
bool DeltaState::isEqual(
	const OutputState& a,
	const OutputState& b,
	const DeltaState &) const
{
	return a.formatOverridden==b.formatOverridden &&
		(!a.formatOverridden || a.formatOverride==b.formatOverride);
}


template <>
void DeltaState::record(
	InputState& inputState,
	const DeltaState &parent)
{
	const size_t srcindex = inputState.value.imagePtrIndex;
	if (inputState.isImage() && srcindex!=InputState::invalidIndex)
	{
		inputState.value.imagePtrIndex = mInputImagePtrs.size();
		mInputImagePtrs.push_back(parent.mInputImagePtrs[srcindex]);
	}
	else if (inputState.isString() && srcindex!=InputState::invalidIndex)
	{
		inputState.value.stringPtrIndex = mInputStringPtrs.size();
		mInputStringPtrs.push_back(parent.mInputStringPtrs[srcindex]);
	}
}


template <>
void DeltaState::record(
	OutputState&,
	const DeltaState&)
{
	// do nothing
}


}  // namespace SubstanceAir
}  // namespace Details


template <typename Items>
void SubstanceAir::Details::DeltaState::append(
	Items& destItems,
	const Items& srcItems,
	const DeltaState &src,
	AppendMode mode)
{
	typename Items::const_iterator previte = destItems.begin();
	Items newitems;
	newitems.reserve(srcItems.size()+destItems.size());

	for (const auto& srcitem : srcItems)
	{
		while (previte!=destItems.end() && previte->index<srcitem.index)
		{
			// Copy directly non modified items
			newitems.push_back(*previte);
			++previte;
		}

		if (previte!=destItems.end() && previte->index==srcitem.index)
		{
			// Collision
			switch (mode)
			{
				case Append_Default:
					newitems.push_back(*previte);   // Existing used
				break;

				case Append_Reverse:
					newitems.push_back(*previte);    // Existing used for modified
					newitems.back().previous = srcitem.modified; // Update previous
					record(newitems.back().previous,src);
				break;

				case Append_Override:
					if (!isEqual(previte->previous,srcitem.modified,src))
					{
						// Combine only if no identity
						newitems.push_back(*previte); // Existing used for previous
						newitems.back().modified = srcitem.modified; // Update modified
						record(newitems.back().modified,src);
					}
				break;
			}

			++previte;
		}
		else
		{
			// Insert (into newitems)
			newitems.push_back(srcitem);
			if (mode==Append_Reverse)
			{
				std::swap(newitems.back().previous,newitems.back().modified);
			}
			record(newitems.back().modified,src);
			record(newitems.back().previous,src);
		}
	}

	std::copy(
		previte,
		const_cast<const Items&>(destItems).end(),
		std::back_inserter(newitems));    // copy remains
	destItems = newitems;                   // replace inputs
}


//! @brief Append a delta state to current
//! @param src Source delta state to append
//! @param mode Append policy flag
void SubstanceAir::Details::DeltaState::append(
	const DeltaState &src,
	AppendMode mode)
{
	append<Inputs>(mInputs,src.getInputs(),src,mode);
	append<Outputs>(mOutputs,src.getOutputs(),src,mode);
}

