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

#include "details/detailsxml.h"
#include "details/detailslinkdata.h"
#include "details/detailsrendererimpl.h"
#include "details/detailsutils.h"
#include "details/detailsstringutils.h"

#include <substance/framework/package.h>

#include <substance/pixelformat.h>
#include <substance/version.h>
#include <substance/linker/linker.h>

#include <iterator>

#include <assert.h>


namespace SubstanceAir
{

//! @brief  Structure used to receipt SBSAR introspection result
struct LinkerArchiveContent
{
	string xml;
	string assembly;
	map<u16string, GraphDesc::Thumbnail> thumbnails;
	map<u16string, vector<uint8_t> > extraFiles;
};


extern "C" void SUBSTANCE_CALLBACK linkerCallbackArchiveXml(
	SubstanceLinkerHandle *handle,
	const unsigned short*,
	const char* xmlContent)
{
	LinkerArchiveContent *content;
	unsigned int res;
	res = substanceLinkerHandleGetUserData(handle,(size_t*)&content);
	(void)res;
	assert(res==0);

	content->xml = xmlContent;
}


extern "C" void SUBSTANCE_CALLBACK linkerCallbackArchiveAsm(
	SubstanceLinkerHandle *handle,
	const unsigned short*,
	const char* asmContent,
	size_t asmSize)
{
	LinkerArchiveContent *content;
	unsigned int res;
	res = substanceLinkerHandleGetUserData(handle,(size_t*)&content);
	(void)res;
	assert(res==0);

	content->assembly.assign((char*)asmContent,asmSize);
}

extern "C" void SUBSTANCE_CALLBACK linkerCallbackArchiveExtra(
	SubstanceLinkerHandle *handle,
	const unsigned short *fullname,
	const void *extraContent,
	size_t extraSize)
{
	LinkerArchiveContent *content = nullptr;
	unsigned int res = 0u;
	res = substanceLinkerHandleGetUserData(handle, (size_t*)&content);
	(void)res;
	assert(res==0);

	vector<uint8_t> fileContent;
	fileContent.resize(extraSize);
	memcpy(fileContent.data(), extraContent, extraSize);

	u16string path = reinterpret_cast<const Utf16Char*>(fullname);
	// Windows must use wchar_t strings instead of char16_t strings
#ifdef _WIN32
	u16string ending = L".png";
#else
	u16string ending = u".png";
#endif

	// Files 1) whose name ends in '.png' and 2) that are right in the 'assemblies/content/0000/'
	// folder (not in a subfolder) are considered graph thumbnails
	// Other files are stored in a separate map
	map<u16string, vector<uint8_t> >& extraMap =
		((std::find(path.begin(), path.end(), (Utf16Char) '/') == path.end()) &&
		(Details::StringUtils::stringEndsWith<u16string>(path, ending))) ?
			content->thumbnails :
			content->extraFiles;

	assert(extraMap.count(path) == 0);
	extraMap[path] = std::move(fileContent);
}

}  // namespace SubstanceAir



//! @brief Construct from Substance archive (.sbsar) file
//! @param archivePtr Pointer on archive data (this buffer can be
//!		deleted just after).
//! @param archiveSize Size of buffer referenced by archivePtr
SubstanceAir::PackageDesc::PackageDesc(
		const void *archivePtr,
		size_t archiveSize,
		const OutputOptions& outputOptions) :
	mUserData(0),
	mUid(0),
	mInstancesCount(0),
	mLastSBSASMVersion(0)
{
	// Use linker to open archive

	// Create linker context
	SubstanceLinkerContext *context;
	unsigned int res;

	res = substanceLinkerContextInit(
		&context,
		SUBSTANCE_LINKER_API_VERSION,
		Substance_EngineID_BLEND);  // Engine ID not used, no link operation
	assert(res==0);

	res = substanceLinkerContextSetCallback(
		context,
		Substance_Linker_Callback_ArchiveXml,
		(void*)SubstanceAir::linkerCallbackArchiveXml);
	assert(res==0);

	res = substanceLinkerContextSetCallback(
		context,
		Substance_Linker_Callback_ArchiveAsm,
		(void*)SubstanceAir::linkerCallbackArchiveAsm);
	assert(res==0);

	res = substanceLinkerContextSetCallback(
		context,
		Substance_Linker_Callback_ArchiveExtraFullPath,
		(void*)SubstanceAir::linkerCallbackArchiveExtra);
	assert(res==0);

	SubstanceLinkerHandle *handle;
	res = substanceLinkerHandleInit(&handle,context);
	assert(res==0);

	// Push archive, callbacks called w/ SBSASM and XML
	LinkerArchiveContent content;

	res = substanceLinkerHandleSetUserData(
		handle,
		(size_t)&content);
	assert(res==0);

	if (substanceLinkerHandlePushAssemblyMemory(
		handle,
		(const char*)archivePtr,
		archiveSize)==Substance_Linker_Error_None)
	{
		// Succeed, grab assembly
		mLinkData = SubstanceAir::make_shared<Details::LinkDataAssembly>(content.assembly);
	}

	// Grab the SBSASM version number
	res = substanceLinkerHandleGetLastSBSASMVersion(handle, &mLastSBSASMVersion);
	assert(res == 0);

	// Release
	res = substanceLinkerHandleRelease(handle);
	assert(res==0);
	res = substanceLinkerContextRelease(context);
	assert(res==0);

	if (mLinkData.get()!=nullptr &&
		!content.xml.empty())
	{
		// Acquire the thumbnail map
		if (!content.thumbnails.empty())
		{
			mThumbnailMap = std::move(content.thumbnails);
		}

		// Grab the extra files (if any)
		if (!content.extraFiles.empty())
		{
			mExtraMap = std::move(content.extraFiles);
		}

		// Initialize from XML
		mXmlString.swap(content.xml);
		init(outputOptions);
	}
	else
	{
		// Error: invalid archive
	}
}


//! @brief Construct w/ xml description and its binary content
//! @param assemblyPtr Pointer on assembly data
//! @param assemblySize Size of buffer referenced by assemblyPtr
//! @param xmlString Pointer on UTF8 Null terminated string that contains
//!		XML description of assemblyBinary I/O bindings
SubstanceAir::PackageDesc::PackageDesc(
		const void *assemblyPtr,
		size_t assemblySize,
		const char *xmlString,
		const OutputOptions& outputOptions) :
	mUserData(0),
	mUid(0),
	mXmlString(xmlString),
	mInstancesCount(0)
{
	mLinkData = make_shared<Details::LinkDataAssembly>(
	                (const char*)assemblyPtr,
	                assemblySize);

	// Here the linker is bypassed, so we directly fetch the SBSASM version number from the buffer itself
	mLastSBSASMVersion = assemblySize > 7 ? (UInt) (((unsigned short*) assemblyPtr)[3]) : 0;

	init(outputOptions);
}


SubstanceAir::PackageDesc::PackageDesc(
		const PackageDesc &sourcePackage,
		const OutputOptions& outputOptions) :
	mUserData(0),
	mUid(0),
	mXmlString(sourcePackage.mXmlString),
	mLinkData(sourcePackage.mLinkData),
	mInstancesCount(0),
	mLastSBSASMVersion(sourcePackage.mLastSBSASMVersion),
	mThumbnailMap(sourcePackage.mThumbnailMap),
	mExtraMap(sourcePackage.mExtraMap)
{
	init(outputOptions);
}


void SubstanceAir::PackageDesc::init(
	const OutputOptions& outputOptions)
{
	// try to parse the XML description file
	if (parseXml(mXmlString.c_str()))
	{
		// Force output options
		int allowedfmt = outputOptions.mAllowedFormats;
		if ((allowedfmt&Format_Mask_ALL)==0)
		{
//			Warning: invalid format selection
		}

		for (auto& graph : mGraphs)
		{
			for (auto& output : graph.mOutputs)
			{
				if (output.isImage())
				{
					// Format
					const int initialFormat = output.mFormat;
					fixFormat(output.mFormat,outputOptions.mAllowedFormats);

					// Mipmap
					const unsigned int initialMipmap = output.mMipmaps;
					if (outputOptions.mMipmap!=Mipmap_Default)
					{
						output.mMipmaps = outputOptions.mMipmap==Mipmap_ForceNone ?
							1 :
							0;
					}

					output.mFmtOverridden =
						initialFormat!=output.mFormat ||
						initialMipmap!=output.mMipmaps;
				}
				else
				{
					output.mFmtOverridden = false;
				}
			}
		}
	}
	else
	{
		mGraphs.clear();
	}
}


void SubstanceAir::PackageDesc::fixFormat(int& fmt,unsigned int allowed)
{
	static const unsigned int fmtList[] = {
		Substance_PF_RGBA,
		Substance_PF_L,
		Substance_PF_RGBA | Substance_PF_16I,
		Substance_PF_L    | Substance_PF_16I,
		Substance_PF_BC1,
		Substance_PF_BC2,
		Substance_PF_BC3,
		Substance_PF_RGBA | Substance_PF_16F,
		Substance_PF_L    | Substance_PF_16F,
		Substance_PF_RGBA | Substance_PF_32F,
		Substance_PF_L    | Substance_PF_32F };
	static const int A = 0xA;
	static const unsigned int fmtCount = 11u;
	static const int matrixFmt[fmtCount][fmtCount] = {
		{ 0,6,5,4,2,7,9,1,3,8,A },  // 0: RGBA8
		{ 1,0,3,8,A,4,6,5,2,7,9 },  // 1: L8
		{ 2,9,7,0,6,5,4,3,A,8,1 },  // 2: RGBA16
		{ 3,A,8,2,9,7,1,0,4,6,5 },  // 3: L16
		{ 4,6,5,0,2,7,9,1,3,8,A },  // 4: BC1
		{ 5,6,4,0,2,7,9,1,3,8,A },  // 5: BC2
		{ 6,5,4,0,2,7,9,1,3,8,A },  // 6: BC3
		{ 7,9,2,0,6,5,4,8,A,3,1 },  // 7: RGBA16F
		{ 8,A,7,9,3,2,1,0,4,6,5 },  // 8: L16F
		{ 9,7,2,0,6,5,4,A,8,3,1 },  // 9: RGBA32F
		{ A,9,8,7,3,2,1,0,4,6,5 }   // A: L32F
	};

	const int* chain = nullptr;

	switch (fmt)
	{
		case Substance_PF_RGBA:
		case Substance_PF_RGBx:
		case Substance_PF_RGB: chain = matrixFmt[0]; break;
		case Substance_PF_L:   chain = matrixFmt[1]; break;

		case Substance_PF_RGBA | Substance_PF_16I:
		case Substance_PF_RGBx | Substance_PF_16I:
		case Substance_PF_RGB  | Substance_PF_16I: chain = matrixFmt[2]; break;
		case Substance_PF_L    | Substance_PF_16I: chain = matrixFmt[3]; break;

		case Substance_PF_RGBA | Substance_PF_16F:
		case Substance_PF_RGBx | Substance_PF_16F:
		case Substance_PF_RGB  | Substance_PF_16F: chain = matrixFmt[7]; break;
		case Substance_PF_L    | Substance_PF_16F: chain = matrixFmt[8]; break;

		case Substance_PF_RGBA | Substance_PF_32F:
		case Substance_PF_RGBx | Substance_PF_32F:
		case Substance_PF_RGB  | Substance_PF_32F: chain = matrixFmt[9]; break;
		case Substance_PF_L    | Substance_PF_32F: chain = matrixFmt[10]; break;

		case Substance_PF_DXT1: chain = matrixFmt[4]; break;
		case Substance_PF_DXT2:
		case Substance_PF_DXT3: chain = matrixFmt[5]; break;
		case Substance_PF_DXT4:
		case Substance_PF_DXT5: chain = matrixFmt[6]; break;
	}

	if (chain!=nullptr && ((1<<chain[0])&allowed)==0)
	{
		// Not allowed found alternative
		for (size_t k=1;k<fmtCount;++k)
		{
			if (((1<<chain[k])&allowed)!=0)
			{
				fmt = fmtList[chain[k]];
				break;
			}
		}
	}
}


//! @brief Destructor
SubstanceAir::PackageDesc::~PackageDesc()
{
	// No more graph instances of graph desc of this package
	assert(mInstancesCount==0);
}


const SubstanceAir::string* SubstanceAir::PackageDesc::getAssemblyData() const
{
	return mLinkData.get()!=nullptr ? mLinkData->getAssembly() : nullptr;
}


const SubstanceAir::string& SubstanceAir::PackageDesc::getXmlString() const
{
	return mXmlString;
}


//! @brief Parse a Substance Air xml companion file
//! @param[in] xml The content of the xml file, must be valid
bool SubstanceAir::PackageDesc::parseXml(const char *xml)
{
	if (xml==nullptr)
	{
		// abort if invalid
//		GWarn->Log( "#air: error, no xml content to parse" );
		return false;
	}

	Details::XmlDocument document;
	document.Parse(xml);

	if (document.Error())
	{
//		GWarn->Log("#air: error while parsing xml companion file");
		return false;
	}

	Details::XmlElement* rootnode = document.FirstChildElement("sbsdescription");

	if (!rootnode)
	{
//		GWarn->Log("#air: error, invalid xml content (sbsdescription)");
		return false;
	}
	else
	{
		{
			if (rootnode->Attribute("asmuid")==nullptr)
			{
//				GWarn->Log("#air: error, invalid xml content (asmuid)");
//				TODO: generate UID from XML hash
				return false;
			}

			Details::getNumericAttribute(mUid,rootnode,"asmuid");
		}

		Details::XmlElement* graphlistnode = rootnode->FirstChildElement("graphs");
		if (!graphlistnode)
		{
//			GWarn->Log("#air: error, invalid xml content (graphs)");
//			Read linear XMLs
			return false;
		}

		// Get global inputs elements
		typedef vector<Details::XmlElement*> XmlElements;
		XmlElements globalinputnodes;
		{
			Details::XmlElement* globalnode = rootnode->FirstChildElement("global");
			if (globalnode!=nullptr)
			{
				AIR_XMLFOREACH (inputnode,globalnode->FirstChildElement("inputs"),"input")
				{
					string identifier;
					Details::getStringAttribute(identifier,inputnode,"identifier");
					if (identifier=="$time" || identifier=="$normalformat")
					{
						globalinputnodes.push_back(inputnode);
					}
					else
					{
						// TODO2 Not handled global
					}
				}
			}
		}

		int graphcount = 0;
		graphlistnode->QueryIntAttribute("count",&graphcount);
		mGraphs.reserve(graphcount);

		// Parse all the graphs
		for (Details::XmlElement* graphnode = graphlistnode->FirstChildElement("graph");
			graphnode;
			graphnode = graphnode->NextSiblingElement("graph"))
		{
			// Create the graph object
			mGraphs.resize(mGraphs.size()+1);
			GraphDesc &graph = mGraphs.back();

			Details::getStringAttribute(graph.mPackageUrl,graphnode,"pkgurl");
			Details::getStringAttribute(graph.mLabel,graphnode,"label");
			Details::getStringAttribute(graph.mDescription,graphnode,"description");
			Details::getStringAttribute(graph.mCategory,graphnode,"category");
			Details::getStringAttribute(graph.mKeywords,graphnode,"keywords");
			Details::getStringAttribute(graph.mAuthor,graphnode,"author");
			Details::getStringAttribute(graph.mAuthorUrl,graphnode,"authorurl");
			Details::getStringAttribute(graph.mUserTag,graphnode,"usertag");

			Details::getStringAttribute(graph.mTypeStr,graphnode,"graphtype");
			if (!graph.mTypeStr.empty())
			{
				const char* const* typeNameEnd = getGraphTypeNames()+GraphType_INTERNAL_COUNT;
				const char* const* typeNameFound = std::find(
					getGraphTypeNames(),
					typeNameEnd,
					graph.mTypeStr);
				graph.mType = typeNameFound!=typeNameEnd ?
					(GraphType)std::distance(getGraphTypeNames(),typeNameFound) :
					GraphType_UNKNOWN;

				// Both "environment_light" and "light" map to the GraphType_EnvironmentLight type
				if (graph.mType == GraphType_UNKNOWN && strcmp(graph.mTypeStr.c_str(), "environment_light") == 0)
				{
					graph.mType = GraphType_EnvironmentLight;
				}
			}

			string iconPath;
			Details::getStringAttribute(iconPath, graphnode, "icon");

			if (!iconPath.empty())
			{
				u16string iconNameUnicode = Details::StringUtils::utf8ToUtf16(iconPath);

				// Check that a thumbnail has been read in
				if (mThumbnailMap.find(iconNameUnicode) != mThumbnailMap.end())
				{
					// Ensure the thumbnail map is not altered so that another
					// package desc can be instantiated from this one
					graph.mThumbnail = mThumbnailMap[iconNameUnicode];
				}
			}

			// Parse physical size
			string physicalSize;
			Details::getStringAttribute(physicalSize, graphnode, "physicalSize");
			if (!physicalSize.empty())
			{
				std::sscanf(
					physicalSize.c_str(),
					"%f,%f,%f",
					&graph.mPhysicalSize.x,
					&graph.mPhysicalSize.y,
					&graph.mPhysicalSize.z);
			}
			else
			{
				graph.mPhysicalSize = Vec3Float(0.0f, 0.0f, 0.0f);
			}

			graph.mParent = this;

			//! @todo parse the usertags

			// Parse outputs
			{
				Details::XmlElement* outlistnode =
					graphnode->FirstChildElement("outputs");

				if (outlistnode)
				{
					int outputcount;
					outlistnode->QueryIntAttribute("count",&outputcount);

					graph.mOutputs.reserve(outputcount);

					AIR_XMLFOREACH (outputnode,outlistnode,"output")
					{
						const size_t prevsize = graph.mOutputs.size();
						graph.mOutputs.resize(prevsize+1);

						// Parse output
						if (!Details::parseOutput(outputnode,graph.mOutputs.back()))
						{
							graph.mOutputs.resize(prevsize);
							continue;
						}
					}
				}

				if (graph.mOutputs.empty())
				{
					mGraphs.resize(mGraphs.size()-1);
					continue;
				}

				// Sort output indices per UID
				graph.commitOutputs();
			}

			// Try to add $time and $normalformat inputs
			for (auto& inputnode : globalinputnodes)
			{
				InputDescBase *const input = Details::parseInput(inputnode,graph);
				if (input!=nullptr)
				{
					if (input->mAlteredOutputUids.empty())
					{
						AIR_DELETE(input);   // Not useful
					}
					else
					{
						graph.mInputs.push_back(input);
					}
				}
			}

			// Parse group labels
			typedef map<string,string> GroupLabels;
			GroupLabels groupLabels;
			AIR_XMLFOREACH (guigroupnode,graphnode->FirstChildElement("guigroups"),"guigroup")
			{
				string identifier,label;
				Details::getStringAttribute(identifier,guigroupnode,"identifier");
				Details::getStringAttribute(label,guigroupnode,"label");
				groupLabels.insert(GroupLabels::value_type(identifier,label));
			}

			// Parse inputs
			{
				Details::XmlElement* inputlistnode =
					graphnode->FirstChildElement("inputs");

				if (inputlistnode)
				{
					int inputcount;
					inputlistnode->QueryIntAttribute("count",&inputcount);

					graph.mInputs.reserve(inputcount+2);

					AIR_XMLFOREACH (inputnode,inputlistnode,"input")
					{
						// Parse input
						InputDescBase *const input =
							Details::parseInput(inputnode,graph);
						if (input!=nullptr)
						{
							graph.mInputs.push_back(input);
							input->mGuiGroup = groupLabels[input->mGuiGroup];
						}
					}
				}
			}

			// Sort input indices per UID
			graph.commitInputs();

			// Sort altering outputs
			for (auto& output : graph.mOutputs)
			{
				std::sort(
					output.mAlteringInputUids.begin(),
					output.mAlteringInputUids.end());
			}

			// Parse presets
			{
				Details::XmlElement* sbspresetsnode =
					graphnode->FirstChildElement("sbspresets");

				if (sbspresetsnode)
				{
					Details::parsePresets(
						graph.mPresets,
						sbspresetsnode,
						&graph);
				}
			}

			graph.mXMP = Details::getXmpPacket(graphnode);
		}

		mXMP = Details::getXmpPacket(rootnode);
	}
	return true;
}


bool SubstanceAir::PackageDesc::getExtraResource(const u16string& filename, vector<uint8_t>& extraContent) const
{
#ifdef _WIN32
	u16string sbsarPrefix = L"sbsar://";
#else
	u16string sbsarPrefix = u"sbsar://";
#endif

	// Strip the potential 'sbsar://' part if present
	u16string strippedFilename(filename);
	if (Details::StringUtils::stringStartsWith<u16string>(filename, sbsarPrefix))
	{
		strippedFilename.assign(filename.begin() + sbsarPrefix.size(), filename.end());
	}

	if (mExtraMap.count(strippedFilename))
	{
		extraContent = mExtraMap.at(strippedFilename);
		return true;
	}

	return false;
}


bool SubstanceAir::PackageDesc::getExtraResource(const string& filename, vector<uint8_t>& extraContent) const
{
	u16string u16filename = Details::StringUtils::utf8ToUtf16(filename);
	return getExtraResource(u16filename, extraContent);
}


bool SubstanceAir::PackageDesc::getExtraResource(const char* filename, vector<uint8_t>& extraContent) const
{
	string u8filename(filename);
	u16string u16filename = Details::StringUtils::utf8ToUtf16(u8filename);
	return getExtraResource(u16filename, extraContent);
}


SubstanceAir::PackageDesc::PackageDesc() :
	mInstancesCount(0)
{
}

void SubstanceAir::instantiate(
	GraphInstances& instances,
	const PackageDesc& packageDesc)
{
	for (const auto& graph : packageDesc.getGraphs())
	{
		instances.push_back(make_shared<GraphInstance>(graph));
	}
}

