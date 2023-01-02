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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_COMMON_H
#define _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_COMMON_H

#include "frameworktestddssave.h"

#include <substance/framework/graph.h>

#include <string>
#include <fstream>

#define CURRENT_DIR_PATH ""

namespace Framework
{
namespace Test
{


template <class Container>
void loadFile(Container &container,const SubstanceAir::string &path,bool binary)
{
	std::ifstream is(
		path.c_str(),
		std::ios::in|(binary?std::ios::binary:(std::ios::openmode)0));
	is.seekg(0,std::ios::end);
	container.resize(is.tellg());
	is.seekg(0,std::ios::beg);
	is.read((char*)&container[0],container.size());
}


//! @brief Save all pending render result of all output of all graph instances
//! Numerical outputs are only printed to console
void saveAllOutputsAsDDS(
	const SubstanceAir::GraphInstances &instances,
	const SubstanceAir::string& prefix);


} // namespace Test
} // namespace Framework



#endif /* ifndef _SUBSTANCE_AIR_FRAMEWORK_TEST_COMMON_COMMON_H */
