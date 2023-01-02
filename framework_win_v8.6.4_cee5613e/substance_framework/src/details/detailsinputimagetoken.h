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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTIMAGETOKEN_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTIMAGETOKEN_H

#include <substance/framework/platform.h>
#include <substance/framework/typedefs.h>

#include "detailssync.h"



#include <memory.h>


namespace SubstanceAir
{
namespace Details
{

struct InputImageToken;
struct InputImageTokenBlend;


//! @brief Definition of the platform agnostic texture input description
//! Extend agnostic texture input holder.
//! Pointer on parent image input token (to retrieve token from texture input)
struct TextureInput : TextureInputAgnostic
{
	//! @brief Pointer on parent token, used to retrieve token inside callbacks
	InputImageToken*const parentToken;


	//! @brief Default constructor, 0 filled texture input data
	TextureInput(InputImageToken* parent) : parentToken(parent) {}
};


//! @brief Definition of the struct used for scoped image access sync
//! Contains texture input description
//! Use a simple mutex (to replace w/ R/W mutex)
struct InputImageToken
{
	//! @brief Generic texture holder agnostic from concrete platform
	TextureInput texture;


	//! @brief Default constructor
	InputImageToken() : texture(this) {}

	//! @brief Virtual destructor, allows polymorphism
	virtual ~InputImageToken() {}

	//! @brief Lock access
	void lock() { mMutex.lock(); }
	
	//! @brief Unlock access
	void unlock()  { mMutex.unlock(); }

	//! @brief cast to InputImageTokenBlend (if valid)
	virtual const InputImageTokenBlend* castToImageInputTokenBlend() const { return nullptr; }

protected:
	Sync::mutex mMutex;           //!< Main mutex
	
private:
	InputImageToken(const InputImageToken&);
	const InputImageToken& operator=(const InputImageToken&);	
};  // class InputImageToken



//! @brief Image input token overload w/ blend data ownership
struct InputImageTokenBlend : public InputImageToken
{
	//! @brief Blend platform binary data
	BinaryData bufferData; 

	//! @brief Size in bytes of the texture data buffer.
	//!	Required and only used variable size formats (Substance_PF_JPEG).
	size_t bufferSize;


	//! @brief Constructor, 0 buffer size by default 
	InputImageTokenBlend() : bufferSize(0) {}
	
	//! @brief cast to InputImageTokenBlend
	virtual const InputImageTokenBlend* castToImageInputTokenBlend() const { return this; }
};


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSINPUTIMAGETOKEN_H
