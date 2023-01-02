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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKGRAPHS_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKGRAPHS_H

#include <substance/framework/typedefs.h>


#include <utility>


namespace SubstanceAir
{
namespace Details
{

class GraphState;


//! @brief All valid graph states snapshot, used at link time
//! Represent the state of all valid Graph states. 
struct LinkGraphs
{
	//! @brief Pointer on graph state
	typedef shared_ptr<GraphState> GraphStatePtr;
	
	//! @brief Vector of graph states, ordered by GraphState::mUid
	typedef vector<GraphStatePtr> GraphStates;
	
	//! @brief Predicate used for sorting/union
	struct OrderPredicate
	{
		bool operator()(const GraphStatePtr& a,const GraphStatePtr& b) const;
	};  // struct OrderPredicate
	
	
	//! @brief Graph states pointer, ordered by GraphState::mUid
	GraphStates graphStates;
	
	
	//! @brief Merge another link graph w\ redondancies
	void merge(const LinkGraphs& src);
	
};  // class LinkGraphs


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSLINKGRAPHS_H
