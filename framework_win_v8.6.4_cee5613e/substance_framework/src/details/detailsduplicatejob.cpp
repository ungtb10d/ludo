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

#include "detailsduplicatejob.h"
#include "detailsstates.h"
#include "detailsoutputsfilter.h"
#include "detailslinkgraphs.h"


//! @brief Constructor
//! @param linkGraphs_ Snapshot on graphs states to use at link time 
//! @param filter_ Used to filter outputs (outputs replace) or nullptr if none.
//! @param states_ Used to retrieve graph instances
//! @param newFirst_ True if new job must be run before canceled ones
SubstanceAir::Details::DuplicateJob::DuplicateJob(
		const LinkGraphs& linkGraphs_,
		const OutputsFilter* filter_,
		const States &states_,
		bool newFirst_) :
	linkGraphs(linkGraphs_),
	filter(filter_),
	states(states_),
	newFirst(newFirst_)
{
}


//! @brief Append a pruned instance delta state
//! @param instanceUid The GraphInstance UID corr. to delta state
//! @param deltaState The delta state to append (values overriden, delete
//!		input entry if return to identity)
void SubstanceAir::Details::DuplicateJob::append(
	UInt instanceUid,
	const DeltaState& deltaState)
{
	DeltaStates::iterator ite = mDeltaStates.find(instanceUid);
	if (ite!=mDeltaStates.end())
	{
		ite->second.append(deltaState,DeltaState::Append_Override);
		if (ite->second.isIdentity())
		{
			// Remove if all identity
			mDeltaStates.erase(ite);
		}
	}
	else
	{
		mDeltaStates.insert(std::make_pair(instanceUid,deltaState));
	}
}


//! @brief Prepend a canceled instance delta state
//! @param instanceUid The GraphInstance UID corr. to delta state
//! @param deltaState The delta state to prepend (reverse values merged)
void SubstanceAir::Details::DuplicateJob::prepend(
	UInt instanceUid,
	const DeltaState& deltaState)
{
	DeltaState& dst = mDeltaStates[instanceUid];
	dst.append(deltaState,DeltaState::Append_Reverse);
}


//! @brief Fix instance input values w/ accumulated delta state
//! @param instanceUid The GraphInstance UID corr. to delta state
//! @param[in,out] deltaState The delta state to fix (merge)
void SubstanceAir::Details::DuplicateJob::fix(
	UInt instanceUid,
	DeltaState& deltaState)
{
	DeltaStates::iterator ite = mDeltaStates.find(instanceUid);
	if (ite!=mDeltaStates.end())
	{
		deltaState.append(ite->second,DeltaState::Append_Default);
		mDeltaStates.erase(ite);
	}
}

