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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_CONSTANTS_H
#define _SUBSTANCE_AIR_FRAMEWORK_CONSTANTS_H

namespace SubstanceAir
{
namespace Constants
{

//! @brief Default memory budget used when rendering actively
const unsigned int RenderingMemoryBudget = 512*1024*1024;

//! @brief Default memory budget used when idle (no rendering, but keeping cache)
const unsigned int IdleMemoryBudget = 256*1024*1024;

}
}

#endif //_SUBSTANCE_AIR_FRAMEWORK_CONSTANTS_H
