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

#include "detailslinkcontext.h"
#include "detailslinkdata.h"
#include "detailsgraphbinary.h"
#include "detailsutils.h"

#include <algorithm>

#include <assert.h>
	
	
//! @brief Constructor from content
SubstanceAir::Details::LinkContext::LinkContext(
		SubstanceLinkerHandle *hnd,
		GraphBinary& gbinary,
		UInt uid) :
	handle(hnd), 
	graphBinary(gbinary), 
	stateUid(uid)
{
}


//! @brief Record Linker Collision UID 
//! @param collisionType Output or input collision flag
//! @param previousUid Initial UID that collide
//! @param newUid New UID generated
//! @note Called by linker callback
void SubstanceAir::Details::LinkContext::notifyLinkerUIDCollision(
	SubstanceLinkerUIDCollisionType collisionType,
	UInt previousUid,
	UInt newUid)
{
	UInt* truid = getTranslated(
		collisionType==Substance_Linker_UIDCollision_Output,
		previousUid);
		
	if (truid!=nullptr)
	{
		*truid = newUid;
	}
}


template <typename EntriesType>
SubstanceAir::UInt* SubstanceAir::Details::LinkContext::searchTranslatedUid(
	EntriesType& entries,
	UInt previousUid) const
{
	typename EntriesType::iterator ite = std::lower_bound(
		entries.begin(),
		entries.end(),
		previousUid,
		GraphBinary::InitialUidOrder());
	
	if (ite!=entries.end() && (*ite)->uidInitial==previousUid)
	{
		// Present in binary
		return &(*ite)->uidTranslated;
	}
	else
	{
		// unused I/O (multi-graph case)
		return nullptr;
	}
}


//! @return Return pointer on translated UID following stacking
//! @param isOutput Set to true if output UID searched, false if input
//! @param previousUid Initial UID of the I/O to search for
//! @param disabledValid If false, return nullptr if I/O disabled by stacking
//!		(connection, fuse, disable)
//! @return Return the pointer on translated UID value (R/W) or nullptr if not
//!		found or found in stack and disabledValid is false
SubstanceAir::UInt* SubstanceAir::Details::LinkContext::getTranslated(
	bool isOutput,
	UInt previousUid,
	bool disabledValid) const
{
	// Search if I/O not disabled by stack
	// translate previous UID in function of stacking UID collisions
	for (StackLevels::const_reverse_iterator lite = stackLevels.rbegin();
		lite != stackLevels.rend();
		++lite)
	{
		StackLevel* const level = *lite;
		if (isOutput==!level->isPost())
		{
			// Output of pre graph OR Input of post graph
			{
				// Search if disabled at this level
				UidTranslates *uidtr = isOutput ? 
					&level->trPreOutputs : 
					&level->trPostInputs;
				const UidTranslates::iterator ite = std::lower_bound(
					uidtr->begin(),
					uidtr->end(),
					UidTranslates::value_type(previousUid,0));
				if (ite!=uidtr->end() && ite->first==previousUid)
				{
					return disabledValid ?  
						&ite->second :      // return it
						nullptr;               // not valid return nullptr
				}
			}
			
			// Follow stack collision
			const UidTranslates *uidtr = isOutput ? 
				&level->linkData.mTrPreOutputs : 
				&level->linkData.mTrPostInputs;
			previousUid = translate(*uidtr,previousUid).first;
		}
	}

	// Search in graph binary
	return isOutput ?
		searchTranslatedUid<>(graphBinary.sortedOutputs,previousUid) :
		searchTranslatedUid<>(graphBinary.sortedInputs,previousUid);
}


//! @return Return translated UID following stacking
//! @param isOutput Set to true if output UID searched, false if input
//! @param previousUid Initial UID of the I/O to search for
//! @param disabledValid If false, return false if I/O disabled by stacking
//!		(connection, fuse, disable)
//! @return Return a pair w/ translated UID value or previousUid if not
//!		found, and boolean set to false if not found or found in stack and
//!		disabledValid is false
SubstanceAir::Details::LinkContext::TrResult 
SubstanceAir::Details::LinkContext::translateUid(
	bool isOutput,
	UInt previousUid,
	bool disabledValid) const
{
	UInt* truid = getTranslated(
		isOutput,
		previousUid,
		disabledValid);
		
	return truid!=nullptr ?
		TrResult(*truid,true) :
		TrResult(previousUid,false);
}


//! @return Search for translated UID 
//! @param uidTranslates Container of I/O translated UIDs
//! @param previousUid Initial UID of the I/O to search for
//! @return Return a pair w/ translated UID value or previousUid if not
//!		found, and boolean set to true if found
SubstanceAir::Details::LinkContext::TrResult 
SubstanceAir::Details::LinkContext::translate(
	const UidTranslates& uidTranslates,
	UInt previousUid)
{
	const UidTranslates::const_iterator ite = std::lower_bound(
		uidTranslates.begin(),
		uidTranslates.end(),
		UidTranslates::value_type(previousUid,0));
	return ite!=uidTranslates.end() && ite->first==previousUid ?
		TrResult(ite->second,true) :
		TrResult(previousUid,false);
}


//! @brief Constructor from context and stacking link data
SubstanceAir::Details::LinkContext::StackLevel::StackLevel(LinkContext &cxt,const LinkDataStacking &data) : 
	linkData(data), 
	mLinkContext(cxt), 
	mIsPost(false)
{
	mLinkContext.stackLevels.push_back(this);
	
	// Fill trPostInputs and trPreOutputs
	
	// From connections
	for (const auto& c : linkData.mOptions.mConnections)
	{
		trPostInputs.push_back(std::make_pair(c.second,c.second));
		trPreOutputs.push_back(std::make_pair(c.first,c.first));
	}
	
	// From non connected outputs 
	for (const auto& uid : linkData.mDisabledOutputs)
	{
		trPreOutputs.push_back(std::make_pair(uid,uid));
	}
	
	// From fused inputs
	for (const auto& fuse : linkData.mFuseInputs)
	{
		trPostInputs.push_back(std::make_pair(fuse.second,fuse.second));
	}
	
	// Sort
	std::sort(trPostInputs.begin(),trPostInputs.end());
	std::sort(trPreOutputs.begin(),trPreOutputs.end());
}


//! @brief Destructor, pop this level from stack
SubstanceAir::Details::LinkContext::StackLevel::~StackLevel()
{
	assert(mLinkContext.stackLevels.back()==this);
	mLinkContext.stackLevels.pop_back();
}


//! @brief Notify that pre graph is pushed, about to push post graph
void SubstanceAir::Details::LinkContext::StackLevel::beginPost()
{
	mIsPost = true;
}



