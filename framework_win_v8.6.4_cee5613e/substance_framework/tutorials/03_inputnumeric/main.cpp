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
//! Set all "$outputsize" tweaks to 1024x1024 (Render result size)
//!	and render ALL textures. Texture are saved in DDS format

#include "../common/frameworktestcommon.h"

// Substance Framework include
#include <substance/framework/framework.h>

#include <vector>
#include <iostream>
#include <sstream>
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

	// Parse package (from Archive)
	PackageDesc packagedesc(
		&archiveData[0],
		archiveData.size());

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

	// Set all "$outputsize" tweaks to 1024x1024
	// Iterate on all graphs
	for (GraphInstances::const_iterator grite = instances.begin();
		grite!=instances.end();
		++grite)
	{
		// Iterate on all inputs
		const GraphInstance::Inputs& inputs = (*grite)->getInputs();
		for (GraphInstance::Inputs::const_iterator inite = inputs.begin();
			inite!=inputs.end();
			++inite)
		{
			InputInstanceBase *const input = *inite;
			if (input->mDesc.mIdentifier=="$outputsize" &&     // size tweak
				input->mDesc.mType==Substance_IOType_Integer2)  // security!
			{
				static_cast<InputInstanceInt2*>(input)->setValue(
					Vec2Int(10,10));  // size are in log2 : 1024x1024
			}
		}
	}

	// Push graphs instances into renderer
	renderer.push(instances);

	// Render synchronous
	renderer.run();

	// Grab all outputs of all graphs
	Framework::Test::saveAllOutputsAsDDS(instances,"output_");

	std::cout << "Done.\n";

	return 0;
}
