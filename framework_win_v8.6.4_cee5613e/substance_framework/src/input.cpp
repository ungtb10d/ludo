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

#include "details/detailsutils.h"

#include <substance/framework/input.h>
#include <substance/framework/graph.h>
#include <substance/framework/visibleif.h>

#include <iterator>
#include <algorithm>

#include <assert.h>

SubstanceAir::InputDescBase::~InputDescBase()
{
}

SubstanceAir::ChannelUse SubstanceAir::InputDescImage::defaultChannelUse() const
{
	if (mChannels.size())
	{
		return mChannels[0];
	}
	return ChannelUse::Channel_UNKNOWN;
}

SubstanceAir::string SubstanceAir::InputDescImage::defaultChannelUseStr() const
{
	if (mChannelsStr.size())
	{
		return mChannelsStr[0];
	}
	return string("UNKNOWN");
}

SubstanceAir::InputInstanceBase*
SubstanceAir::InputDescImage::instantiate(GraphInstance& parent) const
{
	return AIR_NEW(InputInstanceImage)(*this,parent);
}


SubstanceAir::InputInstanceBase*
SubstanceAir::InputDescString::instantiate(GraphInstance& parent) const
{
	return AIR_NEW(InputInstanceString)(*this,parent);
}


SubstanceAir::InputInstanceBase::InputInstanceBase(
		const InputDescBase& desc,
		GraphInstance& parent) :
	mDesc(desc),
	mParentGraph(parent),
	mUseCache(false),
	mIsHeavyDuty(desc.mIsDefaultHeavyDuty)
{
}


SubstanceAir::InputInstanceBase::~InputInstanceBase()
{
}


void SubstanceAir::InputInstanceBase::postModification()
{
	// touch the outputs timestamps
	for (const auto& uid : mDesc.mAlteredOutputUids)
	{
		OutputInstance*const outinst = mParentGraph.findOutput(uid);
		assert(outinst!=nullptr);
		outinst->invalidate();
	}
}

bool SubstanceAir::InputInstanceBase::IsVisible() const
{
	return SubstanceAir::VisibleIf::EvalVisibleIf(this);
}

SubstanceAir::InputInstanceNumericalBase::InputInstanceNumericalBase(
		const InputDescBase& desc,
		GraphInstance& parent) :
	InputInstanceBase(desc,parent)
{
}


const SubstanceAir::InputInstanceNumericalBase::Desc&
SubstanceAir::InputInstanceNumericalBase::getDesc() const
{
	return static_cast<const Desc&>(mDesc);
}


//! @brief Accessor as raw value
SubstanceAir::BinaryData
SubstanceAir::InputInstanceNumericalBase::getRawValue() const
{
	const unsigned char* const dataptr = (const unsigned char*)getRawData();
	return BinaryData(dataptr,dataptr+getRawSize());
}


//! @brief Internal use only
bool SubstanceAir::InputInstanceNumericalBase::isModified(
	const void* numeric) const
{
	const unsigned char* beg0 = (const unsigned char*)getRawData();
	return !std::equal(beg0,beg0+getRawSize(),(const unsigned char*)numeric);
}


const SubstanceAir::InputInstanceImage::Desc&
SubstanceAir::InputInstanceImage::getDesc() const
{
	return static_cast<const Desc&>(mDesc);
}


void SubstanceAir::InputInstanceImage::setImage(
	const InputImage::SPtr& ptr)
{
	if (mInputImage!=ptr)
	{
		mInputImage = ptr;
		postModification();
		mPtrModified = true;
	}
}


void SubstanceAir::InputInstanceImage::reset()
{
	InputImage::SPtr nullPtr;
	setImage(nullPtr);
}


//! @brief Helper: is set to non default value
bool SubstanceAir::InputInstanceImage::isNonDefault() const
{
	return mInputImage.get()!=nullptr;
}


//! @brief Internal use only
bool SubstanceAir::InputInstanceImage::isModified(const void*) const
{
	bool res = mInputImage.get()!=nullptr &&
		mInputImage->resolveDirty();
	res = mPtrModified || res;
	mPtrModified = false;
	return res;
}


SubstanceAir::InputInstanceImage::InputInstanceImage(
		const Desc& desc,
		GraphInstance& parent) :
	InputInstanceBase(desc,parent),
	mPtrModified(false)
{
}


const SubstanceAir::InputInstanceString::Desc& SubstanceAir::InputInstanceString::getDesc() const
{
	return static_cast<const Desc&>(mDesc);
}


void SubstanceAir::InputInstanceString::setString(const string& str)
{
	if (mValue!=str)
	{
		mValue = str;
		mIsModified = true;
		postModification();
	}
}


void SubstanceAir::InputInstanceString::reset()
{
	setString(getDesc().mDefaultValue);
}


bool SubstanceAir::InputInstanceString::isNonDefault() const
{
	return mValue != getDesc().mDefaultValue;
}


bool SubstanceAir::InputInstanceString::isModified(const void*) const
{
	bool res = mIsModified;
	mIsModified = false;
	return res;
}


SubstanceAir::InputInstanceString::InputInstanceString(
		const Desc& desc,
		GraphInstance& parent) :
	InputInstanceBase(desc,parent),
	mValue(getDesc().mDefaultValue),
	mIsModified(true)
{
}


size_t SubstanceAir::getComponentsCount(SubstanceIOType type)
{
	switch (type)
	{
		case Substance_IOType_Float   :
		case Substance_IOType_Integer : return 1;
		case Substance_IOType_Float2  :
		case Substance_IOType_Integer2: return 2;
		case Substance_IOType_Float3  :
		case Substance_IOType_Integer3: return 3;
		case Substance_IOType_Float4  :
		case Substance_IOType_Integer4: return 4;
		default                      : return 0;
	}
}


bool SubstanceAir::isFloatType(SubstanceIOType type)
{
	return (size_t)type<=(size_t)Substance_IOType_Float4;
}
