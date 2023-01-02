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
//! w/ output format of 'normal' channel (if present) overridden to DXT5 w/
//! X,Y set on G,A components.
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

	// Parse package (from Archive AND output options)
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

	// Iterate on all graphs
	for (GraphInstances::const_iterator grite = instances.begin();
		grite!=instances.end();
		++grite)
	{
		const string &pkgurl = (*grite)->mDesc.mPackageUrl;

		// Iterate on all outputs
		const GraphInstance::Outputs& outputs = (*grite)->getOutputs();
		for (GraphInstance::Outputs::const_iterator outite = outputs.begin();
			outite!=outputs.end();
			++outite)
		{
			const auto& channels = (*outite)->mDesc.mChannels;
			const auto normalIt = std::find(channels.cbegin(), channels.cend(), Channel_Normal);
			if (normalIt!=channels.cend())
			{

				std::cout << "Normal found in '"<<pkgurl<<
					"', set format to DXT5 and shuffle XY(RG) to GA.\n";

				OutputFormat outputFormat;
				outputFormat.format = Substance_PF_BC3;
				outputFormat.perComponent[0].outputUid = 0;     // Set zero
				outputFormat.perComponent[1].shuffleIndex = 0;  // R
				outputFormat.perComponent[2].outputUid = 0;     // Set zero
				outputFormat.perComponent[3].shuffleIndex = 1;  // G
				(*outite)->overrideFormat(outputFormat);
			}
		}
	}

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
