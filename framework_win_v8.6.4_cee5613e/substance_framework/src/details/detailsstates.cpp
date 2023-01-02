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

#include "detailsstates.h"
#include "detailsgraphstate.h"
#include "detailslinkgraphs.h"
#include "detailsutils.h"

#include <substance/framework/graph.h>

#include <algorithm>
#include <utility>
#include <set>

#include <assert.h>


//! @brief Destructor, remove pointer on this in recorded instances
SubstanceAir::Details::States::~States()
{
	clear();
}


//! @brief Clear states, remove pointer on this in recorded instances
//! @note Called from user thread 
void SubstanceAir::Details::States::clear()
{
	for (const auto& inst : mInstancesMap)
	{
		inst.second.instance->unplugState(this);
	}
	
	mInstancesMap.clear();
}


//! @brief Get the GraphState associated to this graph instance
//! @param graphInstance The source graph instance
//! @note Called from user thread 
//!
//! Create new state from graph instance if necessary (undefined state):
//! record the state into the instance.
SubstanceAir::Details::GraphState& 
SubstanceAir::Details::States::operator[](GraphInstance& graphInstance)
{
	InstancesMap::iterator ite = mInstancesMap.find(graphInstance.mInstanceUid);
	if (ite==mInstancesMap.end())
	{
		Instance inst;
		inst.state = make_shared<GraphState>(graphInstance);
		inst.instance = &graphInstance;
		ite = mInstancesMap.insert(
			std::make_pair(graphInstance.mInstanceUid,inst)).first;
		graphInstance.plugState(this);
	}
	
	return *ite->second.state.get();
}


//! @brief Return a graph instance from its UID or nullptr if deleted 
//! @note Called from user thread 
SubstanceAir::GraphInstance* 
SubstanceAir::Details::States::getInstanceFromUid(UInt uid) const
{
	InstancesMap::const_iterator ite = mInstancesMap.find(uid);
	return ite!=mInstancesMap.end() ? ite->second.instance : nullptr;
}


//! @brief Notify that an instance is deleted
//! @param uid The uid of the deleted graph instance
//! @note Called from user thread 
void SubstanceAir::Details::States::notifyDeleted(UInt uid)
{
	InstancesMap::iterator ite = mInstancesMap.find(uid);
	assert(ite!=mInstancesMap.end());
	mInstancesMap.erase(ite);
}


//! @brief Fill active graph snapshot: all active graph instance to link
//! @param snapshot The active graphs array to fill
void SubstanceAir::Details::States::fill(LinkGraphs& snapshot) const
{
	snapshot.graphStates.reserve(mInstancesMap.size());
	std::set<UInt> pushedstates; 
	for (const auto& inst : mInstancesMap)
	{
		if (pushedstates.insert(inst.second.state->getUid()).second)
		{
			snapshot.graphStates.push_back(inst.second.state);
		}
	}
	
	// Sort list of states per State UID
	std::sort(
		snapshot.graphStates.begin(),
		snapshot.graphStates.end(),
		LinkGraphs::OrderPredicate());
}


//! @brief Clear all render token w/ render results from a specific engine
//! @brief Must be called from user thread Not thread safe, can't be 
//!		called if render ongoing.
void SubstanceAir::Details::States::releaseRenderResults(UInt engineUid)
{
	for (auto& inst : mInstancesMap)
	{
		for (const auto& output : inst.second.instance->getOutputs())
		{
			output->releaseTokensOwnedByEngine(engineUid);
		}
	}
}

