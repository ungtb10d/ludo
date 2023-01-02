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

//!	This tutorial load a Substance Archive file (.SBSAR) and render ALL
//!	textures. Texture are saved in DDS format

#include "../common/frameworktestddssave.h"
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

	std::cout << "SBSAR Version number: " << packagedesc.getSBSASMVersionNumber() << "\n";

	if (!packagedesc.isValid())
	{
		std::cerr << "Invalid package.\n";
		return 3;
	}

	if (!packagedesc.getXMP().empty())
		std::cout << "XMP:\n" << packagedesc.getXMP() << "\n";

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

	// Iterate on all graphs
	for (GraphInstances::const_iterator grite = instances.begin();
		grite!=instances.end();
		++grite)
	{
		const string &pkgurl = (*grite)->mDesc.mPackageUrl;

		std::cout << "Graph:\n"
			"\tPackage URL: " << pkgurl <<
			"\n\tType: " << (*grite)->mDesc.mTypeStr << " (" << (*grite)->mDesc.mType << ")\n";

		if (!(*grite)->mDesc.mXMP.empty())
			std::cout << "\tXMP:\n" << (*grite)->mDesc.mXMP << "\n";

		// Iterate on all inputs
		const GraphInstance::Inputs& inputs = (*grite)->getInputs();
		for (GraphInstance::Inputs::const_iterator inite = inputs.begin();
			inite!=inputs.end();
			++inite)
		{
			std::cout << "\tInput:\n"
				"\t\tInput identifier: " << (*inite)->mDesc.mIdentifier <<
				"\n\t\tInput UID: " << (*inite)->mDesc.mUid <<
				"\n\t\tType: " << (*inite)->mDesc.mType << "\n";

			if (!(*inite)->mDesc.mXMP.empty())
				std::cout << "\t\tXMP:\n" << (*inite)->mDesc.mXMP << "\n";
		}

		// Iterate on all outputs
		const GraphInstance::Outputs& outputs = (*grite)->getOutputs();
		for (GraphInstance::Outputs::const_iterator outite = outputs.begin();
			outite!=outputs.end();
			++outite)
		{

			// Grab result (auto pointer on RenderResultBase)
			OutputInstance::Result result((*outite)->grabResult());
			if (result.get()!=nullptr)
			{
				std::cout << "\tRender result:\n"
					"\t\tOutput identifier: " << (*outite)->mDesc.mIdentifier <<
					"\n\t\tOutput UID: " << (*outite)->mDesc.mUid <<
					"\n\t\tType: " << (*outite)->mDesc.mType << "\n";

				if (result->isImage())
				{
					RenderResultImage*const resimg = dynamic_cast<RenderResultImage*>(result.get());
					std::cout <<
						"\t\tWidth: " << resimg->getTexture().level0Width <<
						"\n\t\tHeight: " << resimg->getTexture().level0Height <<
						"\n\t\tFormat: " << (int)resimg->getTexture().pixelFormat<<"\n";

					// resimg->getTexture().buffer -> Pixel data

					// DDS filename
					std::stringstream filenamesstr;
					filenamesstr << CURRENT_DIR_PATH "output_" <<
						pkgurl.substr(pkgurl.find_last_of('/')+1) << "_" <<  // Graph name
						(*outite)->mDesc.mIdentifier << "[" <<
						(*outite)->mDesc.mUid << "].dds";

					// Save output as DDS or numerical print to console
					Framework::Test::writeDDSFile(
						filenamesstr.str().c_str(),
						resimg->getTexture());
				}
				else
				{
					std::cout <<
						"\t\tValue: " << dynamic_cast<RenderResultNumericalBase*>(result.get())->getValueAsString() << "\n";
				}

				if (!(*outite)->mDesc.mXMP.empty())
					std::cout << "\t\tXMP:\n" << (*outite)->mDesc.mXMP << "\n";
			}
			// Buffer destroyed (auto pointer deletion at scope exit)
		}
	}

	std::cout << "Done.\n";

	return 0;
}
