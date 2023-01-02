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

#include "detailsgraphstate.h"
#include "detailsgraphbinary.h"
#include "detailsrenderjob.h"
#include "detailslinkdata.h"
#include "detailsdeltastate.h"
#include "detailsutils.h"

#include <algorithm>

#include <assert.h>


namespace SubstanceAir
{
namespace Details
{

static UInt gGraphInstanceUid = 0x0;

}   // namespace Details
}   // namespace SubstanceAir


//! Create from graph instance
//! @param graphInstance The source graph instance, initialize state
//! @note Called from user thread
SubstanceAir::Details::GraphState::GraphState(
		GraphInstance& graphInstance) :
	mUid((++Details::gGraphInstanceUid)|0x80000000u),
	mLinkData(graphInstance.mDesc.mParent->getLinkData()),
	mGraphBinary(AIR_NEW(GraphBinary)(graphInstance))
{
	mInputStates.reserve(graphInstance.getInputs().size());
	mOutputStates.reserve(graphInstance.getOutputs().size());

	for (const auto& inpinst : graphInstance.getInputs())
	{
		mInputStates.resize(mInputStates.size()+1);
		InputState &inpst = mInputStates.back();
		inpst.flags = inpinst->mDesc.mType;

		// Initialize from default
		inpst.initValue(inpinst->mDesc);
	}

	mOutputStates.resize(mOutputStates.size() + graphInstance.getOutputs().size());
}


//! @brief Destructor
SubstanceAir::Details::GraphState::~GraphState()
{
	Utils::checkedDelete(mGraphBinary);
}


//! @brief Apply delta state to current state
void SubstanceAir::Details::GraphState::apply(const DeltaState& deltaState)
{
	for (const auto & inpsrc : deltaState.getInputs())
	{
		assert(inpsrc.index<mInputStates.size());
		const InputState &inpsrcmod = inpsrc.modified;

		if (!inpsrcmod.isCacheOnly())
		{
			InputState &inpdst = mInputStates[inpsrc.index];
			if (inpsrcmod.isImage())
			{
				// Set input image
				if (inpdst.value.imagePtrIndex!=InputState::invalidIndex)
				{
					// Remove previous image pointer
					mInputImagePtrs.remove(inpdst.value.imagePtrIndex);
				}

				const size_t inpind = inpsrcmod.value.imagePtrIndex;
				inpdst.value.imagePtrIndex = inpind!=InputState::invalidIndex ?
					mInputImagePtrs.push(deltaState.getInputImage(inpind)) :
					InputState::invalidIndex;
			}
			else if (inpsrcmod.isString())
			{
				// Set input string
				if (inpdst.value.stringPtrIndex!=InputState::invalidIndex)
				{
					// Remove previous string pointer
					mInputStringPtrs.remove(inpdst.value.stringPtrIndex);
				}

				const size_t inpind = inpsrcmod.value.stringPtrIndex;
				inpdst.value.stringPtrIndex = inpind!=InputState::invalidIndex ?
					mInputStringPtrs.push(deltaState.getInputString(inpind)) :
					InputState::invalidIndex;
			}
			else
			{
				// Set input numeric
				inpdst.value = inpsrcmod.value;
			}
		}
	}

	for (const auto& outsrc : deltaState.getOutputs())
	{
		assert(outsrc.index<mOutputStates.size());
		mOutputStates[outsrc.index] = outsrc.modified;
	}
}


//! @brief This state is currently linked (present in current SBSBIN data)
bool SubstanceAir::Details::GraphState::isLinked() const
{
	return mGraphBinary->isLinked();
}
