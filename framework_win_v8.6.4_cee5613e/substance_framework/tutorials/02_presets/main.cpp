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

//!	This tutorial load a Substance Archive file (.SBSAR) AND a Substance
//! presets file (.SBSPRS)
//! Render ALL textures FOR EACH PRESET. Texture are saved in DDS format

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
	// expect .sbsar (Archive) filepath AND preset (.sbsprs) filepath
	if (argc<3) 
	{
		std::cerr<<"Usage: "<<argv[0]<<" <sbsar filepath> <sbsprs filepath>\n";
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
	
	// Load presets file
	std::string presetsData;
	Framework::Test::loadFile(presetsData,argv[2],true);
	
	if (presetsData.empty())
	{	
		std::cerr << "Unable to read presets files : \n\t" 
			<< argv[2] << std::endl;
		return 3;
	}
	
	// Parse presets
	Presets presets;
	parsePresets(presets,presetsData.c_str());
	
	// Instantiate all graphs of the package
	GraphInstances instances;
	instantiate(instances,packagedesc); 
	
	// Create renderer object
	Renderer renderer;
	
	// For each preset, try to apply it on ALL graphs and render ALL outputs
	for (Presets::const_iterator iteprs = presets.begin();
		iteprs != presets.end();
		++iteprs)
	{
		// Iterate on all graphs
		for (GraphInstances::const_iterator grite = instances.begin();
			grite!=instances.end();
			++grite)
		{		
			// Flags all outputs as dirty: force ALL outputs recomputation, 
			// not only ones modified by preset.
			// Iterate on all outputs
			const GraphInstance::Outputs& outputs = (*grite)->getOutputs();
			for (GraphInstance::Outputs::const_iterator outite = outputs.begin();
				outite!=outputs.end();
				++outite)
			{
				(*outite)->flagAsDirty();  // force recompute
			}
			
			// Apply current preset on the graph
			iteprs->apply(*grite->get());
			
			std::cerr << "Apply preset '" <<
				iteprs->mLabel << "' to '" <<
				(*grite)->mDesc.mPackageUrl << "'.\n";
		}
		
		// Push graphs instances into renderer
		renderer.push(instances);

		std::cout << "Rendering..\n";
		
		// Render synchronous
		renderer.run();

		// Grab all outputs of all graphs
		Framework::Test::saveAllOutputsAsDDS(instances,iteprs->mLabel+"_");
	}
	
	std::cout << "Done.\n";
	
	return 0;
}
