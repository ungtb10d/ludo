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

#include "detailsrendertoken.h"
#include "detailssync.h"
#include "detailsutils.h"
#include "detailsengine.h"

#include <substance/framework/renderresult.h>

#include <assert.h>


//! @brief Constructor
SubstanceAir::Details::RenderToken::RenderToken() :
	mRenderResult(nullptr),
	mFilled(ATOMIC_VAR_INIT(false)),
	mPushCount(1),
	mRenderCount(1)
{
}


//! @brief Destructor
//! Delete render result if present
SubstanceAir::Details::RenderToken::~RenderToken()
{
	if (mRenderResult.load()!=nullptr)
	{
		clearRenderResult(mRenderResult);
	}
}


//! @brief Fill render result (grab ownership)
void SubstanceAir::Details::RenderToken::fill(RenderResultBase* renderResult)
{
	RenderResultBase* prevrres = mRenderResult.exchange(renderResult);

	mFilled = true;              // do not change affectation order

	if (prevrres!=nullptr)
	{
		clearRenderResult(prevrres);
	}
}


//! @brief Increment push and render counter
void SubstanceAir::Details::RenderToken::incrRef()
{
	++mPushCount;
	++mRenderCount;
}


//! @brief Decrement render counter
void SubstanceAir::Details::RenderToken::canceled()
{
	assert(mRenderCount>0);
	--mRenderCount;
}


//! @brief Return true if can be removed from OutputInstance queue
//! Can be removed if nullptr filled or push count is >1 or render count ==0
//! @post Decrement push counter
bool SubstanceAir::Details::RenderToken::canRemove()
{
	if ((mFilled && mRenderResult.load()==nullptr) ||    // do not change && order
		mPushCount>1 ||
		mRenderCount==0)
	{
		assert(mPushCount>0);
		--mPushCount;
		return true;
	}

	return false;
}


//! @brief Return render result or nullptr if pending, transfer ownership
//! @post mRenderResult becomes nullptr
SubstanceAir::RenderResultBase* SubstanceAir::Details::RenderToken::grabResult()
{
	RenderResultBase* currentResult = mRenderResult.load();
	RenderResultBase* res = nullptr;
	if (currentResult!=nullptr && mRenderCount>0)
	{
		res = mRenderResult.exchange(nullptr);
	}

	return res;
}


//! @brief Delete render results w/ specific engine UID
bool SubstanceAir::Details::RenderToken::releaseOwnedByEngine(UInt engineUid)
{
	RenderResultBase* result = mRenderResult.load();

	if (result!=nullptr &&
		result->isImage() &&
		static_cast<RenderResultImage*>(result)->getEngine()->getInstanceUid()==engineUid)
	{
		clearRenderResult(result);
		mRenderResult = nullptr;
		return true;
	}

	return false;
}


void SubstanceAir::Details::RenderToken::clearRenderResult(
	RenderResultBase* renderResult)
{
	if (renderResult->isImage())
	{
		RenderResultImage*const imgres = static_cast<RenderResultImage*>(renderResult);
		assert(imgres->haveOwnership());

		imgres->getEngine()->enqueueRelease(imgres->releaseTexture());
	}

	Utils::checkedDelete(renderResult);
}

