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

//!	This tutorial load two Substance Archive file (.SBSAR),
//! connect outputs of first SBSAR to inputs of second SBSAR (post filter) 
//!	and render ALL textures. Texture are saved in DDS format

#include "../common/frameworktestcommon.h"

// Substance Framework include
#include <substance/framework/framework.h>

#include <vector>
#include <iostream>
#include <string>



/** @brief main entry point */
int main(int argc,char **argv)
{
	// expect two .sbsar (Archive) filepaths
	if (argc<3) 
	{
		std::cerr<<"Usage: "<<argv[0]<<" <sbsar filepath> <sbsar filepath>\n";
		return 1;
	}

	// Load .sbsar files
	std::vector<unsigned char> archiveDatas[2];
	for (size_t k=0;k<2;++k)
	{
		Framework::Test::loadFile(archiveDatas[k],argv[1+k],true);
		
		if (archiveDatas[k].empty())
		{	
			std::cerr << "Unable to read sbsar files : \n\t" 
				<< argv[1+k] << std::endl;
			return 2;
		}
	}
	
	using namespace SubstanceAir;
	
	// Parse first package
	PackageDesc packagedesc0(
		&archiveDatas[0][0],
		archiveDatas[0].size());
		
	// Parse second package
	PackageDesc packagedesc1(
		&archiveDatas[1][0],
		archiveDatas[1].size());

	if (!packagedesc0.isValid() || !packagedesc1.isValid())
	{
		std::cerr << "Invalid package.\n";
		return 3;
	}
	
	// Stack the first graph of the 1st package w/ first graph of the 2nd package
	PackageStackDesc stackeddesc(
		packagedesc0.getGraphs().front(),
		packagedesc1.getGraphs().front());
	
	// Instantiate all graphs of the package
	GraphInstances instances;
	instantiate(instances,stackeddesc);
	
	// Create renderer object
	Renderer renderer;
	
	// Push graphs instances into renderer
	renderer.push(instances);

	// Render synchronous
	renderer.run();

	// Grab all outputs of all graphs
	Framework::Test::saveAllOutputsAsDDS(instances,"output_");

	std::cout << "Done.\n";
	
	return 0;
}
