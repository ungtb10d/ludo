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

#include "detailsgraphbinary.h"
#include "detailsutils.h"

#include <substance/framework/graph.h>

#include <algorithm>

#include <assert.h>

//! @brief Invalid index
const SubstanceAir::UInt
SubstanceAir::Details::GraphBinary::invalidIndex = 0xFFFFFFFFu;


//! Create from graph instance
//! @param graphInstance The graph instance, used to fill UIDs
SubstanceAir::Details::GraphBinary::GraphBinary(GraphInstance& graphInstance) :
	mLinked(false)
{
	// Inputs
	{
		inputs.resize(graphInstance.getInputs().size());
		sortedInputs.resize(inputs.size());
		Inputs::iterator eite = inputs.begin();
		InputsPtr::iterator eptrite = sortedInputs.begin();
		for (const auto& inpinst : graphInstance.getInputs())
		{
			eite->uidInitial = eite->uidTranslated = inpinst->mDesc.mUid;
			eite->index = invalidIndex;
			*(eptrite++) = &*(eite++);
		}
	}

	// Outputs
	{
		outputs.resize(graphInstance.getOutputs().size());
		sortedOutputs.resize(outputs.size());
		Outputs::iterator eite = outputs.begin();
		OutputsPtr::iterator eptrite = sortedOutputs.begin();
		for (const auto& outinst : graphInstance.getOutputs())
		{
			const OutputDesc& outdesc = outinst->mDesc;
			eite->uidInitial = eite->uidTranslated = outdesc.mUid;
			eite->index = invalidIndex;
			eite->enqueuedLink = false;
			eite->descFormat = outdesc.mFormat;
			eite->descMipmap = outdesc.mMipmaps;
			eite->descForced = outdesc.mFmtOverridden;
			*(eptrite++) = &*(eite++);
		}
	}

	// Sort by initial uid
	std::sort(sortedInputs.begin(),sortedInputs.end(),InitialUidOrder());
	std::sort(sortedOutputs.begin(),sortedOutputs.end(),InitialUidOrder());
}


//! Reset translated UIDs before new link process
void SubstanceAir::Details::GraphBinary::resetTranslatedUids()
{
	for (auto& entry : inputs)
	{
		entry.uidTranslated = entry.uidInitial;
		entry.index = invalidIndex;
	}

	for (auto& entry : outputs)
	{
		entry.uidTranslated = entry.uidInitial;
		entry.index = invalidIndex;
	}
}


//! @brief Notify that this binary is linked
void SubstanceAir::Details::GraphBinary::linked()
{
	mLinked = true;
}


void SubstanceAir::Details::GraphBinary::resetLinked()
{
	mLinked = false;

	for (auto& entry : outputs)
	{
		entry.enqueuedLink = false;
	}
}

