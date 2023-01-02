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

#include "details/detailsstates.h"
#include "details/detailsutils.h"

#include <substance/framework/renderer.h>
#include <substance/framework/package.h>
#include <substance/framework/graph.h>

#include <algorithm>

#include <assert.h>


namespace SubstanceAir
{
namespace Details
{

struct InstantiateOutput
{
	InstantiateOutput(GraphInstance &parent) : mParent(parent)
	{
	}

	OutputInstance* operator()(const OutputDesc& desc, bool dynamic = false) const
	{
		return AIR_NEW(OutputInstance)(desc,mParent,dynamic);
	}

	GraphInstance &mParent;
};

struct InstantiateInput
{
	InstantiateInput(GraphInstance &parent) : mParent(parent)
	{
	}

	InputInstanceBase* operator()(const InputDescBase* desc) const
	{
		return desc->instantiate(mParent);
	}

	GraphInstance &mParent;
};

static UInt gInstanceUid = 0x0;

static const char* const gGraphTypeNames[GraphType_INTERNAL_COUNT] = {
	"material",
	"decal_material",
	"atlas_material",
	"filter",
	"mesh_based_generator",
	"texture_generator",
	"light",
	"light_texture",
	"UNSPECIFIED",
	"UNKNOWN"};

}   // namespace Details
}   // namespace SubstanceAir


const char*const* SubstanceAir::getGraphTypeNames()
{
	assert(GraphType_INTERNAL_COUNT==(sizeof(Details::gGraphTypeNames)/sizeof(const char*)));
	return Details::gGraphTypeNames;
}


//! @brief Default constructor
SubstanceAir::GraphDesc::GraphDesc() :
	mType(GraphType_UNSPECIFIED),
	mThumbnail(),
	mParent(nullptr)
{
}


//! @brief Copy constructor
SubstanceAir::GraphDesc::GraphDesc(const GraphDesc& src)
{
	*this = src;
}


//! @brief Affectation operator
SubstanceAir::GraphDesc& SubstanceAir::GraphDesc::operator=(
	const GraphDesc& src)
{
	mPackageUrl   = src.mPackageUrl;
	mLabel        = src.mLabel;
	mDescription  = src.mDescription;
	mCategory     = src.mCategory;
	mKeywords     = src.mKeywords;
	mAuthor       = src.mAuthor;
	mAuthorUrl    = src.mAuthorUrl;
	mUserTag      = src.mUserTag;
	mOutputs      = src.mOutputs;
	mPhysicalSize = src.mPhysicalSize;
	mThumbnail    = src.mThumbnail;
	mType         = src.mType;
	mTypeStr      = src.mTypeStr;
	mXMP          = src.mXMP;

	clearInputs();
	mInputs = src.mInputs;
	for (auto& input : mInputs)
	{
		input = input->clone();
	}

	mSortedOutputs = src.mSortedOutputs;
	mSortedInputs = src.mSortedInputs;

	mParent = src.mParent;
	mPresets = src.mPresets;

	return *this;
}


//! @brief Destructor
SubstanceAir::GraphDesc::~GraphDesc()
{
	clearInputs();
}


//! @brief Find an output in mOutputs array by its UID
//! @param uid The uid of the output to retrieve
//! @return Return the index on the output found or outputs array size
//!		if unknown uid.
size_t SubstanceAir::GraphDesc::findOutput(UInt uid) const
{
	assert(mOutputs.size()==mSortedOutputs.size());

	const SortedIndices::const_iterator ite = std::lower_bound(
		mSortedOutputs.begin(),
		mSortedOutputs.end(),
		SortedIndices::value_type(uid,0));

	return ite!=mSortedOutputs.end() && ite->first==uid ?
		ite->second :
		mSortedOutputs.size();
}


//! @brief Find an input in mInputs array by its UID
//! @param uid The uid of the input to retrieve
//! @return Return the iterator on the input found or inputs array size
//!		if unknown uid.
size_t SubstanceAir::GraphDesc::findInput(UInt uid) const
{
	assert(mInputs.size()==mSortedInputs.size());

	const SortedIndices::const_iterator ite = std::lower_bound(
		mSortedInputs.begin(),
		mSortedInputs.end(),
		SortedIndices::value_type(uid,0));

	return ite!=mSortedInputs.end() && ite->first==uid ?
		ite->second :
		mSortedInputs.size();
}


//! @brief Internal use only
//! Fill mSortedOutputs from created outputs
//! @pre All outputs are pushed into mOutputs; commit is not already done
//! @post Find function can be used, mOutputs cannot be modified
void SubstanceAir::GraphDesc::commitOutputs()
{
	assert(mSortedOutputs.empty());
	mSortedOutputs.reserve(mOutputs.size());

	for (const auto& output : mOutputs)
	{
		mSortedOutputs.push_back(SortedIndices::value_type(
			output.mUid,mSortedOutputs.size()));
	}

	std::sort(mSortedOutputs.begin(),mSortedOutputs.end());
}


//! @brief Internal use only
//! Fill mSortedInputs from created inputs
//! @pre All inputs are pushed into mInputs; commit is not already done
//! @post Find function can be used, mInputs cannot be modified
void SubstanceAir::GraphDesc::commitInputs()
{
	assert(mSortedInputs.empty());
	mSortedInputs.reserve(mInputs.size());

	for (const auto& input : mInputs)
	{
		mSortedInputs.push_back(SortedIndices::value_type(
			input->mUid,mSortedInputs.size()));
	}

	std::sort(mSortedInputs.begin(),mSortedInputs.end());
}


void SubstanceAir::GraphDesc::clearInputs()
{
	std::for_each(
		mInputs.begin(),
		mInputs.end(),
		Details::Utils::Deleter<InputDescBase>());
}


//! @brief Constructor from description
SubstanceAir::GraphInstance::GraphInstance(const GraphDesc& desc) :
	mDesc(desc),
	mInstanceUid((++Details::gInstanceUid)|0x80000000u)
{
	assert(mDesc.mParent!=nullptr);
	mDesc.mParent->mInstancesCount++;

	mOutputs.resize(mDesc.mOutputs.size());
	std::transform(
		mDesc.mOutputs.begin(),
		mDesc.mOutputs.end(),
		mOutputs.begin(),
		Details::InstantiateOutput(*this));

	// Append shallow copies of the outputs to the texture or value outputs
	// lists
	for (const auto& output : mOutputs)
	{
		if (output->mDesc.isImage())
		{
			mTextureOutputs.push_back(output);
		}
		else
		{
			mValueOutputs.push_back(output);
		}
	}

	mInputs.resize(mDesc.mInputs.size());
	std::transform(
		mDesc.mInputs.begin(),
		mDesc.mInputs.end(),
		mInputs.begin(),
		Details::InstantiateInput(*this));
}


//! @brief Destructor
SubstanceAir::GraphInstance::~GraphInstance()
{
	assert(mDesc.mParent!=nullptr);
	assert(mDesc.mParent->mInstancesCount>0);
	mDesc.mParent->mInstancesCount--;

	// Notify all renderers that this instance will be deleted
	for (const auto& states : mStates)
	{
		states->notifyDeleted(mInstanceUid);
	}

	std::for_each(
		mOutputs.begin(),
		mOutputs.end(),
		Details::Utils::Deleter<OutputInstance>());

	std::for_each(
		mInputs.begin(),
		mInputs.end(),
		Details::Utils::Deleter<InputInstanceBase>());
}


SubstanceAir::OutputInstance*
SubstanceAir::GraphInstance::findOutput(UInt uid) const
{
	assert(getOutputs().size()>=mDesc.mOutputs.size());

	Outputs::const_iterator institem = getOutputs().begin();
	std::advance(institem,mDesc.findOutput(uid));
	return institem!=getOutputs().end() ?
		*institem :
		(OutputInstance*)nullptr;
}


SubstanceAir::InputInstanceBase*
SubstanceAir::GraphInstance::findInput(UInt uid) const
{
	assert(getInputs().size()>=mDesc.mInputs.size());

	Inputs::const_iterator institem = getInputs().begin();
	std::advance(institem,mDesc.findInput(uid));
	return institem!=getInputs().end() ?
		*institem :
		(InputInstanceBase*)nullptr;
}


//! @brief Helper: Create a new output instance from a format override
//! @param fmt the OutputFormat to use to initialize this output instance
//! @param desc the OutputDesc to attach to the new OutputInstance
//! @return Return the pointer of the OutputInstance created
SubstanceAir::OutputInstance* SubstanceAir::GraphInstance::createOutput(const SubstanceAir::OutputFormat& fmt, const SubstanceAir::OutputDesc& desc)
{
	static UInt sDynamicOutputUid = 0;

	UInt uid = sDynamicOutputUid++ | 0x80000000u;

	//safeguard against uid collision
	for (const auto& output : mOutputs)
	{
		uid = std::max(uid, output->mDesc.mUid);
	}

	uid += 1;

	//push the provided OutputDesc to our deque
	mOutputDescs.push_back(desc);

	OutputDesc& descReference = mOutputDescs.back();

	//override incoming uid with generated one
	descReference.mUid = uid;

	OutputInstance* inst = Details::InstantiateOutput(*this)(descReference, true);

	//because we must force output creation, we need to force component shuffling.
	//if the caller did not change component formats, then use the incoming desc uid
	//as the source and apply that on a per component basis
	SubstanceAir::OutputFormat fixedFmt = fmt;

	for (unsigned int i = 0u; i < SubstanceAir::OutputFormat::ComponentsCount; i++)
	{
		unsigned int& uidReference = fixedFmt.perComponent[i].outputUid;

		if (uidReference == SubstanceAir::OutputFormat::UseDefault)
		{
			uidReference = desc.mUid;
		}
	}

	inst->overrideFormat(fixedFmt);

	//generate altering input UIDs if the caller left the list empty
	if (descReference.mAlteringInputUids.empty())
	{
		SubstanceAir::Uids aggregateUids;

		//aggregate alteringInputUIDs from incoming source output uids
		for (unsigned int i = 0u; i < SubstanceAir::OutputFormat::ComponentsCount; i++)
		{
			for (const auto& output : mOutputs)
			{
				if (output->mDesc.mUid == inst->getFormatOverride().perComponent[i].outputUid)
				{
					aggregateUids.insert(aggregateUids.begin(),
										 output->mDesc.mAlteringInputUids.begin(),
										 output->mDesc.mAlteringInputUids.end());

					break;
				}
			}
		}

		//sort uids and remove duplicates
		std::sort(aggregateUids.begin(), aggregateUids.end());
		aggregateUids.erase(std::unique(aggregateUids.begin(), aggregateUids.end()), aggregateUids.end());

		descReference.mAlteringInputUids = aggregateUids;
	}

	mOutputs.push_back(inst);

	// Append to the appropriate value or texture outputs as a shallow copy
	if (inst->mDesc.isImage())
	{
		mTextureOutputs.push_back(inst);
	}
	else
	{
		mValueOutputs.push_back(inst);
	}

	return inst;
}


//! @brief Internal use only
void SubstanceAir::GraphInstance::plugState(Details::States* states)
{
	assert(std::find(mStates.begin(),mStates.end(),states)==mStates.end());
	mStates.push_back(states);
}


//! @brief Internal use only
void SubstanceAir::GraphInstance::unplugState(Details::States* states)
{
	States::iterator ite = std::find(mStates.begin(),mStates.end(),states);
	assert(ite!=mStates.end());
	mStates.erase(ite);
}

