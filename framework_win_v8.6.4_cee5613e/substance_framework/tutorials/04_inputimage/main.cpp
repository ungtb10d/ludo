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
//! Fill all image inputs (tweaks) with a loaded JPEG image
//! (or checkboard bitmap if not provided). 
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
	// expect .sbsar (Archive) and JPEG (optional) filepaths
	if (argc<2) 
	{
		std::cerr<<"Usage: "<<argv[0]<<" <sbsar filepath> <jpeg filepath>\n";
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
	
	// Load .jpg file
	std::vector<unsigned char> jpegData;
	if (argc==3)
	{
		Framework::Test::loadFile(jpegData,argv[2],true);
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
	
	// Create image from jpeg or default checkboard if not provided
	InputImage::SPtr inputImage;
	if (jpegData.empty())
	{
		// Checkboard
		{
			// Texture description
			SubstanceTexture texture = {
				nullptr, // No buffer (content will be set later for demo purposes)
				256,  // Width;
				256,  // Height
				Substance_PF_RGBA,
				Substance_ChanOrder_RGBA,
				1};   // One mipmap

			// Create InputImage object from texture description
			inputImage = InputImage::create(texture);
		}
		
		{
			// Fill image
			InputImage::ScopedAccess scopedmod(inputImage);
			for (size_t k=0;k<256*256;++k)  
			{
				// 16x16 black/white checkboard
				((unsigned int*)scopedmod->buffer)[k] = 
					((k&0x1F)<0x10)^(((k>>8)&0x1F)<0x10) ? 0xFFFFFFFFu : 0;
			}
		}
	}
	else
	{
		// JPEG data
		SubstanceTexture texture = {
			&jpegData[0], // JPEG data
			0,            // Width read from header
			0,            // Height read from header
			Substance_PF_JPEG,
			Substance_ChanOrder_NC,
			1};   // One mipmap
		
		// Create InputImage object from texture description
		// JPEG data buffer size is required 
		inputImage = InputImage::create(texture,jpegData.size());
	}
	
	// Instantiate all graphs of the package
	GraphInstances instances;
	instantiate(instances,packagedesc);
	
	// Create renderer object
	Renderer renderer;
	
	// Set inputImage for all image tweaks
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
			if (input->mDesc.isImage())
			{
				static_cast<InputInstanceImage*>(input)->setImage(inputImage);
			}
		}
	}
	
	// Push graphs instances into renderer
	renderer.push(instances);

	// Render synchronous
	// Warning: no ImageInput::ScopedAccess instances should be still alive
	renderer.run();

	// Grab all outputs of all graphs
	Framework::Test::saveAllOutputsAsDDS(instances,"output_");

	std::cout << "Done.\n";
	
	return 0;
}
