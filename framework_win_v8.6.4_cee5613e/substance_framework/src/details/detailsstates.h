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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTATES_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTATES_H

#include <substance/framework/typedefs.h>




namespace SubstanceAir
{

class GraphInstance;

namespace Details
{

class GraphState;
struct LinkGraphs;


//! @brief All Graph Instances global States
//! Represent the state of all pushed Graph instances. 
//! Each graph state is associated to one or several instances.
class States
{
public:
	//! @brief Default constructor
	States() {}

	//! @brief Destructor, remove pointer on this in recorded instances
	~States();

	//! @brief Get the GraphState associated to this graph instance
	//! @param graphInstance The source graph instance
	//! @note Called from user thread 
	//!
	//! Create new state from graph instance if necessary (undefined state):
	//! record the state into the instance.
	GraphState& operator[](GraphInstance& graphInstance);
	
	//! @brief Return a graph instance from its UID or nullptr if deleted 
	//! @note Called from user thread 
	GraphInstance* getInstanceFromUid(UInt uid) const;
	
	//! @brief Notify that an instance is deleted
	//! @param uid The uid of the deleted graph instance
	//! @note Called from user thread 
	void notifyDeleted(UInt uid);
	
	//! @brief Fill active graph snapshot: all active graph instance to link
	//! @param snapshot The active graphs array to fill
	void fill(LinkGraphs& snapshot) const;
	
	//! @brief Clear states, remove pointer on this in recorded instances
	//! @note Called from user thread 
	void clear();

	//! @brief Clear all render token w/ render results from a specific engine
	//! @brief Must be called from user thread. Not thread safe, can't be 
	//!		called if render ongoing.
	void releaseRenderResults(UInt engineUid);
	
protected:
	//! @brief Pair of GraphState, GraphInstance
	struct Instance
	{
		//! @brief Pointer on graph state
		shared_ptr<GraphState> state;
		
		//! @brief Pointer on corresponding instance.
		//! Used for deleting pointer on this
		//! @note Only used from user thread 
		GraphInstance* instance;    
		
	};  // struct Instance

	//! @brief Map of UID / instances
	typedef map<UInt,Instance> InstancesMap;
	
	//! @brief GraphInstance per instance UID dictionnary
	//! GraphState can be referenced several times per different instances
	//! Pointer on GraphState keeped by StatesSnapshot while usefull.
	InstancesMap mInstancesMap;

private:
	States(const States&);
	const States& operator=(const States&);		
};  // class States


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSSTATES_H
