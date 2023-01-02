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

#include "details/detailsrendererimpl.h"

#include <substance/framework/renderresult.h>

#include <memory.h>
#include <assert.h>


SubstanceAir::RenderResultImage::RenderResultImage(
		const TextureAgnostic& texture,
		SubstanceContext_* context,
		Details::Engine* engine) :
	RenderResultBase(Substance_IOType_Image),
	mTextureAgnostic(texture),
	mHaveOwnership(true),
	mContext(context),
	mEngine(engine)
{
	assert(mEngine!=nullptr);
}


SubstanceAir::RenderResultImage::~RenderResultImage()
{
	if (mHaveOwnership)
	{
		mEngine->enqueueRelease(mTextureAgnostic);
	}
}


SubstanceAir::TextureAgnostic SubstanceAir::RenderResultImage::releaseTexture()
{
	TextureAgnostic res(mTextureAgnostic);
	assert(haveOwnership());
	mHaveOwnership = false;
	return res;
}


