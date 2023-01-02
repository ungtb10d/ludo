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

//!	This tutorial load a Substance Archive file (.SBSAR),
//! w/ outputs format overriden to DXT5/DXT1 and w/ full mipmap pyramid.
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
	// expect .sbsar (Archive) filepath
	if (argc<2) 
	{
		std::cerr<<"Usage: "<<argv[0]<<" <sbsar filepath>\n";
		return 1;
	}

	// Load .sbsar file
	std::vector<unsigned char> archiveData;
	Framework::Test::loadFile(archiveData,argv[1],true);
	
	if (archiveData.empty())
	{	
		std::cerr << "Unable to read sbsar files : \n\t" 
			<< argv[1] << std::endl;
		return 2;
	}
	
	using namespace SubstanceAir;
	
	// Fill output options structure
	OutputOptions outputOptions;
	outputOptions.mAllowedFormats = Format_BC1|Format_BC3; // Force DXT1/DXT5
	outputOptions.mMipmap = Mipmap_ForceFull;              // Force full pyramid

	// Parse package (from Archive AND output options)
	PackageDesc packagedesc(
		&archiveData[0],
		archiveData.size(),
		outputOptions);

	if (!packagedesc.isValid())
	{
		std::cerr << "Invalid package.\n";
		return 3;
	}
	
	// Instantiate all graphs of the package
	GraphInstances instances;
	instantiate(instances,packagedesc);
	
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
