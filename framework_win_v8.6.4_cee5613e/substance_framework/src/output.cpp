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

#include "details/detailsrendertoken.h"
#include "details/detailsutils.h"

#include <substance/framework/graph.h>
#include <substance/framework/output.h>

#include <assert.h>


SubstanceAir::OutputInstance::OutputInstance(
    const OutputDesc& desc,
    GraphInstance& parentGraph,
    bool dynamic) :
	mDesc(desc),
	mParentGraph(parentGraph),
	mEnabled(true),
	mUserData(0x0),
	mDirty(true),
	mFormatOverridden(false),
	mDynamicOutput(dynamic)
{
}


void SubstanceAir::OutputInstance::overrideFormat(const OutputFormat& newFormat)
{
	if (!mDesc.isImage())
	{
		assert(0);
		return;
	}

	// Backup previous component output composition UIDs
	const unsigned int prevuids[OutputFormat::ComponentsCount] = {
		mFormatOverride.perComponent[0].outputUid,
		mFormatOverride.perComponent[1].outputUid,
		mFormatOverride.perComponent[2].outputUid,
		mFormatOverride.perComponent[3].outputUid};

	mFormatOverride = newFormat;

	// Update component output composition invalidation lists
	for (unsigned int k=0;k<OutputFormat::ComponentsCount;++k)
	{
		if (mFormatOverridden)
		{
			const unsigned int prevuid = prevuids[k];
			if (prevuid!=OutputFormat::UseDefault && prevuid!=0u)
			{
				// Has previous channel composition, remove from target output
				// composite list
				OutputInstance*const outcomp = mParentGraph.findOutput(prevuid);
				assert(outcomp!=nullptr);
				Ptrs::iterator itecomp = std::find(
					outcomp->mCompositeOutput.begin(),
					outcomp->mCompositeOutput.end(),
					this);
				if (itecomp!=outcomp->mCompositeOutput.end())
				{
					// May be already removed if used by several components
					outcomp->mCompositeOutput.erase(itecomp);
				}
			}
		}

		unsigned int &newuid = mFormatOverride.perComponent[k].outputUid;
		if (newuid!=OutputFormat::UseDefault && newuid!=0u)
		{
			// New channel composition
			OutputInstance*const outcomp = mParentGraph.findOutput(newuid);
			if (outcomp!=nullptr)
			{
				// Valid, insert from target output composite list
				if (outcomp->mCompositeOutput.empty() ||
					outcomp->mCompositeOutput.back()!=this) // Don't push twice
				{
					outcomp->mCompositeOutput.push_back(this);
				}
			}
			else
			{
				// Bad UID, use zero filling instead
				assert(0);
				newuid = OutputFormat::ComponentEmpty;
			}
		}
	}

	mFormatOverridden = true;

	flagAsDirty();
}


//! @brief Grab render result corresponding of this output
//! @note If this output was computed several times, this function
//! 	returns the least recent result.
//! @warning Ownership of render result is transferred to caller
//! @return Return the render result or nullptr auto ptr if no results available
SubstanceAir::OutputInstance::Result
SubstanceAir::OutputInstance::grabResult()
{
	Result res;

	// Remove all canceled render results
	while (!mRenderTokens.empty() &&
		mRenderTokens.front()->canRemove())
	{
		mRenderTokens.pop_front();
	}

	// Get first valid
	for (RenderTokens::iterator ite = mRenderTokens.begin();
		res.get()==nullptr && ite != mRenderTokens.end();
		++ite)
	{
		res.reset((*ite)->grabResult());
	}

	return res;
}


//! @brief Internal use only
void SubstanceAir::OutputInstance::push(const Token& rtokenptr)
{
	mRenderTokens.push_back(rtokenptr);
}


//! @brief Internal use only
bool SubstanceAir::OutputInstance::queueRender()
{
	bool res = mDirty;
	mDirty = false;
	return res;
}


void SubstanceAir::OutputInstance::invalidate()
{
	flagAsDirty();

	// Flag as dirty all outputs that use this one as composition
	for (const auto& outptr : mCompositeOutput)
	{
		outptr->flagAsDirty();
	}
}


//! @brief Internal use only
void SubstanceAir::OutputInstance::releaseTokensOwnedByEngine(UInt engineUid)
{
	// Remove deprecated tokens: token w/ render results created from engine
	// w/ UID as argument.
	// Not thread safe, can't be called if render ongoing.

	for (RenderTokens::iterator ite = mRenderTokens.begin();
		ite != mRenderTokens.end();)
	{
		if ((*ite)->releaseOwnedByEngine(engineUid))
		{
			// not optimal on deque, but marginal case
			ite = mRenderTokens.erase(ite);
		}
		else
		{
			++ite;
		}
	}
}

//! @brief Accessor for the default channel usage for this output
//! Returns the default channel type
SubstanceAir::ChannelUse SubstanceAir::OutputDesc::defaultChannelUse() const
{
	if (mChannels.size())
	{
		return mChannels[0];
	}
	return ChannelUse::Channel_UNKNOWN;
}

//! @brief Accessor for the default channel usage for this output in string format
//! Returns the default channel type as a string
SubstanceAir::string SubstanceAir::OutputDesc::defaultChannelUseStr() const
{
	if (mChannelsStr.size())
	{
		return mChannelsStr[0];
	}
	return string("UNKNOWN");
}
