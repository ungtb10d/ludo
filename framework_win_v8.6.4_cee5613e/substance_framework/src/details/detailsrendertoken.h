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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERTOKEN_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERTOKEN_H

#include <substance/framework/typedefs.h>


namespace SubstanceAir
{

struct RenderResultBase;

namespace Details
{


//! @brief Substance rendering result shell struct
//! Queued in OutputInstance
class RenderToken
{
public:
	//! @brief Constructor
	RenderToken();

	//! @brief Destructor
	//! Delete render result if present
	~RenderToken();

	//! @brief Fill render result (grab ownership)
	void fill(RenderResultBase* renderResult);

	//! @brief Increment push and render counter
	void incrRef();

	//! @brief Decrement render counter
	void canceled();

	//! @brief Return true if can be removed from OutputInstance queue
	//! Can be removed if nullptr filled or push count is >1 or render count ==0
	//! @post Decrement push counter
	bool canRemove();

	//! @brief Return if already computed
	bool isComputed() const { return mFilled; }

	//! @brief Return render result or nullptr if pending, transfer ownership
	//! @post mRenderResult becomes nullptr
	RenderResultBase* grabResult();

	//! @brief Delete render results w/ specific engine UID
	//! @param engineUid The UID of engine to delete render result
	//! @return Return true if render result is effectively released
	bool releaseOwnedByEngine(UInt engineUid);

protected:
	//! @brief The pointer on render result or nullptr if grabbed/skipped/pending
	atomic<RenderResultBase*> mRenderResult;

	//! @brief True if filled, can be removed in OutputInstance
	atomic_bool mFilled;

	//! @brief Pushed in OutputInstance count
	size_t mPushCount;

	//! @brief Render pending count (not canceled)
	size_t mRenderCount;


	//! @brief Delete render result
	static void clearRenderResult(RenderResultBase* renderResult);

private:
	RenderToken(const RenderToken&);
	const RenderToken& operator=(const RenderToken&);
};  // class RenderToken


} // namespace Details
} // namespace SubstanceAir

#endif //_SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSRENDERTOKEN_H
