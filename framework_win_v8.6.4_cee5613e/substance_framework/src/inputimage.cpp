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

#include <substance/framework/inputimage.h>

#include "details/detailsinputimagetoken.h"
#include "details/detailsutils.h"

#include <assert.h>
#include <memory.h>



//! @brief Constructor from InputImage to access (thread safe)
//! @param inputImage The input image to lock, cannot be nullptr pointer
//! @post The access to InputImage is locked until destructor call.
SubstanceAir::InputImage::ScopedAccess::ScopedAccess(
		const InputImage::SPtr& inputImage) :
	mInputImage(inputImage)
{
	assert(mInputImage);
	mInputImage->mInputImageToken->lock();
	mInputImage->mDirty = true;
}


//! @brief Destructor, unlock InputImage access
//! @warning Do not modify buffer outside ScopedAccess scope
SubstanceAir::InputImage::ScopedAccess::~ScopedAccess()
{
	mInputImage->mInputImageToken->unlock();
}


//! @brief Accessor on texture description
//! @warning Do not delete buffer pointer. However its content can be
//!		freely modified inside ScopedAccess scope.
const SubstanceAir::TextureInputAgnostic&
SubstanceAir::InputImage::ScopedAccess::getTextureInput() const
{
	return mInputImage->mInputImageToken->texture;
}


//! @brief Helper: returns buffer content size in bytes
//! Only valid w/ BLEND Substance Engine API platform
//!		(system memory texture).
size_t SubstanceAir::InputImage::ScopedAccess::getSize() const
{
	const Details::InputImageTokenBlend*const tknblend = mInputImage->mInputImageToken != nullptr ?
		mInputImage->mInputImageToken->castToImageInputTokenBlend() : nullptr;
	assert(tknblend!=nullptr);

	return tknblend!=nullptr ? tknblend->bufferSize : 0;
}


//! @brief Destructor
//! @warning Do not delete this class directly if it was already set
//!		into a InputImageImage: it can be still used in rendering process.
//!		Use shared pointer mechanism instead.
SubstanceAir::InputImage::~InputImage()
{
	Details::Utils::checkedDelete(mInputImageToken);
}


SubstanceAir::Details::InputImageToken*
SubstanceAir::InputImage::getToken() const
{
	return mInputImageToken;
}


SubstanceAir::InputImage::InputImage(Details::InputImageToken* token) :
	mDirty(true),
	mInputImageToken(token)
{
}


//! @brief Internal use only
bool SubstanceAir::InputImage::resolveDirty()
{
	const bool res = mDirty;
	mDirty = false;
	return res;
}

SubstanceAir::InputImage::SPtr
SubstanceAir::InputImage::create(const TextureInputAgnostic& textureInput)
{
	Details::InputImageToken* token = AIR_NEW(Details::InputImageToken);
	static_cast<TextureInputAgnostic&>(token->texture) = textureInput;
	return make_shared<make_constructible<InputImage>>(token);
}


#ifdef SUBSTANCE_PLATFORM_BLEND

#include <substance/handle.h>

SubstanceAir::InputImage::SPtr
SubstanceAir::InputImage::create(
	const SubstanceTexture_& srctex,
	size_t bufferSize)
{
	auto tknblend = make_unique<Details::InputImageTokenBlend>();

	SubstanceTextureInput& dsttexinp = *reinterpret_cast<SubstanceTextureInput*>(
		tknblend->texture.textureInputAny);
	dsttexinp.level0Width = srctex.level0Width;
	dsttexinp.level0Height = srctex.level0Height;
	dsttexinp.mipmapCount = srctex.mipmapCount;
	dsttexinp.pixelFormat = srctex.pixelFormat;

	tknblend->texture.platform = (SubstanceEngineIDEnum)SUBSTANCE_API_PLATFORM;
	SubstanceTexture& dsttex = dsttexinp.mTexture;
	dsttex = srctex;

	const size_t nbPixels =
		dsttex.level0Width*dsttex.level0Height;
	const size_t nbDxtBlocks =
		((dsttex.level0Width+3)>>2)*((dsttex.level0Height+3)>>2);

	switch (dsttex.pixelFormat & Substance_PF_MASK)
	{
		// 8bpc
		case Substance_PF_RGBA:
		case Substance_PF_RGBx: bufferSize = nbPixels*4; break;
		case Substance_PF_RGB : bufferSize = nbPixels*3; break;
		case Substance_PF_L   : bufferSize = nbPixels;   break;

		// 16bpc
		case Substance_PF_RGBA|Substance_PF_16I:
		case Substance_PF_RGBx|Substance_PF_16I:
		case Substance_PF_RGBA|Substance_PF_16F:
		case Substance_PF_RGBx|Substance_PF_16F: bufferSize = nbPixels*8; break;
		case Substance_PF_RGB |Substance_PF_16I:
		case Substance_PF_RGB |Substance_PF_16F: bufferSize = nbPixels*6; break;
		case Substance_PF_L   |Substance_PF_16I:
		case Substance_PF_L   |Substance_PF_16F: bufferSize = nbPixels*2; break;

		// 32bpc
		case Substance_PF_RGBA|Substance_PF_32F:
		case Substance_PF_RGBx|Substance_PF_32F: bufferSize = nbPixels*16; break;
		case Substance_PF_RGB |Substance_PF_32F: bufferSize = nbPixels*12; break;
		case Substance_PF_L   |Substance_PF_32F: bufferSize = nbPixels* 4; break;

		// Compressed formats
		case Substance_PF_BC1:
		case Substance_PF_BC4: bufferSize = nbDxtBlocks*8;  break;
		case Substance_PF_DXT2:
		case Substance_PF_BC2:
		case Substance_PF_DXT4:
		case Substance_PF_BC3:
		case Substance_PF_BC5: bufferSize = nbDxtBlocks*16; break;
		case Substance_PF_JPEG: break;
	}

	if (bufferSize>0)
	{
		// Create buffer, ensure 16bytes alignment
		tknblend->bufferSize = bufferSize;
		tknblend->bufferData.resize(bufferSize+0xF);
		dsttex.buffer =
			(void*)((((size_t)&tknblend->bufferData[0])+0xF)&~(size_t)0xF);

		if (srctex.buffer)
		{
			memcpy(dsttex.buffer,srctex.buffer,bufferSize);
		}
	}

	SPtr ptr;
	if (dsttex.buffer!=nullptr)
	{
		ptr = make_shared<make_constructible<InputImage>>(tknblend.release());
	}

	return ptr;                   // Return nullptr ptr if invalid
}


#endif  // SUBSTANCE_PLATFORM_BLEND
