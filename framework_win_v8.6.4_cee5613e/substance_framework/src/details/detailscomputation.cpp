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

#include "detailscomputation.h"
#include "detailsengine.h"
#include "detailsinputstate.h"
#include "detailsinputimagetoken.h"

#include <algorithm>
#include <iterator>

#include <assert.h>


//! @brief Constructor from engine
//! @param flush Flush internal engine queue if true
//! @pre Engine is valid and correctly linked
//! @post Engine render queue is flushed if 'flush' is true
SubstanceAir::Details::Computation::Computation(Engine& engine,bool flush) :
	mEngine(engine)
{
	assert(mEngine.getHandle()!=nullptr);
	if (flush)
	{
		mEngine.flush();
	}
}


bool SubstanceAir::Details::Computation::pushInput(
	UInt index,
	const InputState& inputState,
	void* value)
{
	bool res = false;

	if (!inputState.isCacheOnly())
	{
		// Push input value
		int err = AIR_SBSCALL(HandlePushSetInput,mEngine.getDynLoad())(
			mEngine.getHandle(),
			0,
			index,
			inputState.getType(),
			value,
			mUserData);

		if (err)
		{
			// TODO2 Warning: pust input failed
			assert(0);
		}
		else
		{
			res = true;
		}
	}

	if (inputState.isCacheEnabled())
	{
		// To push as hint
		mHintInputs.push_back(getHintInput(inputState.getType(),index));
	}

	return res;
}


//! @brief Push outputs SBSBIN indices to compute
void SubstanceAir::Details::Computation::pushOutputs(const Indices& indices)
{
	int err = AIR_SBSCALL(HandlePushOutputs,mEngine.getDynLoad())(
		mEngine.getHandle(),
		0,
		&indices[0],
		(unsigned int)indices.size(),
		mUserData);

	if (err)
	{
		// TODO2 Warning: pust outputs failed
		assert(0);
	}

	// To push as hint
	mHintOutputs.insert(mHintOutputs.end(),indices.begin(),indices.end());
}


//! @brief Run computation
//! Push hints I/O and run
void SubstanceAir::Details::Computation::run()
{
	SubstanceHandle_* handle = mEngine.getHandle();

	if (!mHintInputs.empty())
	{
		// Make unique
		std::sort(mHintInputs.begin(),mHintInputs.end());
		mHintInputs.erase(std::unique(
			mHintInputs.begin(),mHintInputs.end()),mHintInputs.end());
	}

	for (Indices::const_iterator iite = mHintInputs.begin();
		iite != mHintInputs.end();
		++iite)
	{
		// Push input hint
		int err = AIR_SBSCALL(HandlePushSetInput,mEngine.getDynLoad())(
			handle,
			Substance_PushOpt_HintOnly,
			getIndex(*iite),
			getType(*iite),
			nullptr,
			0);

		if (err)
		{
			// TODO2 Warning: pust input failed
			assert(0);
		}
	}

	if (!mHintOutputs.empty())
	{
		// Make unique
		std::sort(mHintOutputs.begin(),mHintOutputs.end());
		const size_t newsize = std::distance(mHintOutputs.begin(),
			std::unique(mHintOutputs.begin(),mHintOutputs.end()));

		// Push output hints
		int err = AIR_SBSCALL(HandlePushOutputs,mEngine.getDynLoad())(
			handle,
			Substance_PushOpt_HintOnly,
			&mHintOutputs[0],
			(unsigned int)newsize,
			0);

		if (err)
		{
			// TODO2 Warning: pust outputs failed
			assert(0);
		}
	}

	// Start computation
	int err = AIR_SBSCALL(HandleStart,mEngine.getDynLoad())(
		handle,
		Substance_Sync_Synchronous);

	if (err)
	{
		// TODO2 Error: start failed
		assert(0);
	}
}

