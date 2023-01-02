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

#include "detailslinkgraphs.h"
#include "detailsgraphstate.h"

#include <algorithm>
#include <iterator>


//! @brief Predicate used for sorting/union
bool SubstanceAir::Details::LinkGraphs::OrderPredicate::operator()(
	const GraphStatePtr& a,
	const GraphStatePtr& b) const
{
	return a->getUid()<b->getUid();
}


//! @brief Merge another link graph w\ redondancies
void SubstanceAir::Details::LinkGraphs::merge(const LinkGraphs& src)
{
	if (graphStates.empty())
	{
		// Simple copy if current empty
		graphStates = src.graphStates;
	}
	else
	{
		// Merge sorted lists in one sorted 
		const LinkGraphs::GraphStates prevgraphstates = graphStates;
		graphStates.resize(0);
		std::set_union(
			src.graphStates.begin(),
			src.graphStates.end(),
			prevgraphstates.begin(),
			prevgraphstates.end(),
			std::back_inserter(graphStates),
			OrderPredicate());
	}
}

