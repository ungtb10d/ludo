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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSFILTER_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSFILTER_H


#include <substance/framework/typedefs.h>

 
 


namespace SubstanceAir
{
namespace Details
{

class RenderJob;


//! @brief Contains a list of outputs to filter per Graph Instance
struct OutputsFilter
{
	//! @brief List of outputs to filter container
	//! GraphState/GraphInstance order indices.
	typedef vector<UInt> Outputs;
	
	//! @brief List of outputs to filter per Instance UID container
	typedef map<UInt,Outputs> Instances;
	
	//! @brief Constructor from RenderJob contains the list of outputs to filter
	OutputsFilter(const RenderJob& src);
	
	//! @brief List of outputs to filter per Instance UID
	//! Outputs are GraphState/GraphInstance order indices.
	Instances instances;
	
};  // struct OutputsFilter


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSFILTER_H
