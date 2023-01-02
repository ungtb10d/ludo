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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSTATE_H
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSTATE_H

#include <substance/framework/output.h>


namespace SubstanceAir
{
namespace Details
{


//! @brief Output Instance State
//! Contains output format override values
struct OutputState
{
	//! @brief Format override structure, identity if formatOverridden is true
	OutputFormat formatOverride;

	//! @brief Optimization flag: format is really overridden only if true
	bool formatOverridden;
	
	
	//! @brief Default constructor: not overridden
	OutputState() : formatOverridden(false) {}

	//! @brief Set output state value from instance
	//! @param inst Output instance to get format override
	void fill(const OutputInstance& inst);
	
};  // struct OutputState


} // namespace Details
} // namespace SubstanceAir

#endif // ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSOUTPUTSTATE_H
