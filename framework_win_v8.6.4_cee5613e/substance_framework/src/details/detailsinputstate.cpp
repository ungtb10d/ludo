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

#include "detailsinputstate.h"
#include "detailsstringutils.h"

#include <substance/framework/input.h>

#include <algorithm>

#include <assert.h>


const size_t SubstanceAir::Details::InputState::invalidIndex =
	(size_t)((ptrdiff_t)-1);


void SubstanceAir::Details::InputState::fillValue(
	const InputInstanceBase* inst,
	InputImagePtrs &imagePtrs,
	InputStringPtrs &stringPtrs)
{
	assert(getType()==inst->mDesc.mType);

	if (inst->mDesc.isNumerical())
	{
		// Input numeric
		const UInt*const newvalue = (const UInt*)(
			static_cast<const InputInstanceNumericalBase*>(inst)->getRawData());
		std::copy(newvalue,newvalue+getComponentsCount(getType()),value.numeric);
	}
	else if (inst->mDesc.isImage())
	{
		// Input image
		const InputImage::SPtr imgptr =
			static_cast<const InputInstanceImage*>(inst)->getImage();
		if (imgptr.get()!=nullptr)
		{
			value.imagePtrIndex = imagePtrs.size();
			imagePtrs.push_back(imgptr);
		}
		else
		{
			value.imagePtrIndex = invalidIndex;
		}
	}
	else if (inst->mDesc.isString())
	{
		// Input string
		const string& strsrc = static_cast<const InputInstanceString*>(inst)->getString();
		InputStringPtr strptr = make_shared<ucs4string>();
		*strptr = StringUtils::utf8ToUcs4(strsrc);

		value.stringPtrIndex = stringPtrs.size();
		stringPtrs.push_back(strptr);
	}
}


//! @brief Initialize value from description default value
//! @param desc Input description to get default value
void SubstanceAir::Details::InputState::initValue(const InputDescBase& desc)
{
	assert(getType()==desc.mType);
	if (!desc.isNumerical())
	{
		// Input image or string, nullptr
		value.imagePtrIndex = invalidIndex;
	}
	else if (desc.mType==Substance_IOType_Integer2 &&
		desc.mIdentifier=="$outputsize")
	{
		// Input numeric, $outputsize case
		// Value may be forced in description
		// Set default to invalid value
		value.numeric[0] = value.numeric[1] = 0xFFFFFFFFu;
	}
	else
	{
		// Input numeric, no $outputsize
		const UInt*const dflvalue = (const UInt*)(desc.getRawDefault());
		std::copy(dflvalue,dflvalue+getComponentsCount(getType()),value.numeric);
	}
}

