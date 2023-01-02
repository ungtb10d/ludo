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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDUPLICATEJOB_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDUPLICATEJOB_H


#include "detailsdeltastate.h"

 


namespace SubstanceAir
{
namespace Details
{

class States;
struct OutputsFilter;
struct LinkGraphs;


//! @brief Render Job duplication context
struct DuplicateJob
{
	//! @brief Constructor
	//! @param linkGraphs_ Snapshot on graphs states to use at link time 
	//! @param filter_ Used to filter outputs (outputs replace) or nullptr if none.
	//! @param states_ Used to retrieve graph instances
	//! @param newFirst_ True if new job must be run before canceled ones
	DuplicateJob(
		const LinkGraphs& linkGraphs_,
		const OutputsFilter* filter_,
		const States &states_,
		bool newFirst_);
	
	//! @param Snapshot on graphs states to use at link time 
	const LinkGraphs& linkGraphs;
	
	//! @param Used to filter outputs (outputs replace) or nullptr if none.
	const OutputsFilter*const filter;
	
	//! @param Used to retrieve graph instances
	const States &states;
	
	
	//! @brief Append a pruned instance delta state
	//! @param instanceUid The GraphInstance UID corr. to delta state
	//! @param deltaState The delta state to append (values overriden, delete
	//!		input entry if return to identity)
	void append(UInt instanceUid,const DeltaState& deltaState);
	
	//! @brief Prepend a canceled instance delta state
	//! @param instanceUid The GraphInstance UID corr. to delta state
	//! @param deltaState The delta state to prepend (reverse values merged)
	void prepend(UInt instanceUid,const DeltaState& deltaState);
	
	//! @brief Fix instance input values w/ accumulated delta state
	//! @param instanceUid The GraphInstance UID corr. to delta state
	//! @param[in,out] deltaState The delta state to fix (merge)
	void fix(UInt instanceUid,DeltaState& deltaState);
	
	//! @brief Return true if  accumulated input delta
	bool hasDelta() const { return !mDeltaStates.empty(); }
	
	//! @brief Return true if current state must be updated by duplicated jobs
	bool needUpdateState() const { return newFirst; }
	
protected:
	//! @brief Delta state per instance UID container
	typedef map<UInt,DeltaState> DeltaStates;
	
	
	//! @brief Pruned inputs list per instance UID
	DeltaStates mDeltaStates;
	
	//! @param True if new job must be run before canceled ones
	const bool newFirst;
	
};  // struct DuplicateJob


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSDUPLICATEJOB_H
