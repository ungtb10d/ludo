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

#include "detailsutils.h"
#include "detailsxml.h"

#include <substance/pixelformat.h>

#include <algorithm>
#include <iterator>


#include <memory>


namespace SubstanceAir
{
namespace Details
{


void getStringAttribute(string& dst,XmlElement* element,const char* name)
{
	const char* attr = element->Attribute(name);
	if (attr)
	{
		dst = attr;
	}
}

typedef struct
{
	const char*      componentString;
	ChannelComponent component;
} ChannelComponentAssociation;

static const ChannelComponentAssociation componentAssociations[] =
{
	{ "RGB", ChannelComponent_RGB },
	{ "R"  , ChannelComponent_R   },
	{ "G"  , ChannelComponent_G   },
	{ "B"  , ChannelComponent_B   },
	{ "A"  , ChannelComponent_A   },
};
static const int componentAssociationCount = sizeof(componentAssociations) / sizeof(componentAssociations[0]);

template <typename T>
struct InputNumericTraits;

template <>
struct InputNumericTraits<float>
{
	static float One() { return 1.f; }
};

template <>
struct InputNumericTraits<int>
{
	static int One() { return 1; }
};

template <typename T>
struct InputNumericTraits
{
	static T One()
	{
		T res;
		for (int i=0; i<T::Size; ++i)
			res[i]=(typename T::Type)1;
		return res;
	}
};


template <typename T>
unique_ptr<InputDescBase> parseInputNumerical(XmlElement* inputnode)
{
	auto input = make_unique<InputDescNumerical<T>>();

	input->mDefaultValue = checkedCast<T>(inputnode->Attribute("default"));

	// Get minimum and maximum values if present
	T zeroValue;
	memset(&zeroValue,0,sizeof(zeroValue));
	input->mMaxValue = InputNumericTraits<T>::One();
	input->mMinValue = zeroValue;
	input->mSliderStep = 0.01f;
	input->mSliderClamp = false;
	input->mSpotColorInfo = "";

	XmlElement *inputguinode = inputnode->FirstChildElement("inputgui");
	if (inputguinode)
	{
		// Get min/max and other input characteristics if present
		XmlElement *inputsubguinode = inputguinode->FirstChildElement();
		if (inputsubguinode)
		{
			input->mMaxValue   = checkedCast<T>(inputsubguinode->Attribute("max"));
			input->mMinValue   = checkedCast<T>(inputsubguinode->Attribute("min"));
			input->mSliderStep = checkedCast<float>(inputsubguinode->Attribute("step"));

			getStringAttribute(input->mLabelFalse, inputsubguinode, "label0");
			getStringAttribute(input->mLabelTrue,  inputsubguinode, "label1");

			string clamp;
			getStringAttribute(clamp, inputsubguinode, "clamp");
			input->mSliderClamp = (clamp == "on") ? true : false;

			// Get enumeration values if present
			AIR_XMLFOREACH (cmbitem,inputsubguinode,"guicomboboxitem")
			{
				input->mEnumValues.resize(input->mEnumValues.size()+1);
				getStringAttribute(input->mEnumValues.back().second,cmbitem,"text");
				input->mEnumValues.back().first = checkedCast<T>(cmbitem->Attribute("value"));
			}

			// Get per channel labels if present
			const char* veclabels[4] = {"label0","label1","label2","label3"};
			for (size_t k=0;k<4;++k)
			{
				string label;
				getStringAttribute(label,inputsubguinode,veclabels[k]);
				if (!label.empty())
				{
					input->mGuiVecLabels.resize(k+1);
					input->mGuiVecLabels[k] = label;
				}
			}
		}
	}

	return static_unique_ptr_cast<InputDescBase>(std::move(input));
}


} // namespace Details
} // namespace SubstanceAir


//! @brief Parse an output node from the Substance Air xml desc file
//! @note Not reading the default size as we do not use it
bool SubstanceAir::Details::parseOutput(
	XmlElement* outputnode,
	OutputDesc& output)
{
	getNumericAttribute<UInt>(output.mUid,outputnode,"uid");

	getStringAttribute(output.mIdentifier,outputnode,"identifier");

	output.mFormat = 0;
	outputnode->QueryIntAttribute("format",&output.mFormat);

	{
		int type = Substance_IOType_Image;
		outputnode->QueryIntAttribute("type",&type);
		output.mType = (SubstanceIOType)type;
	}

	if (output.isImage())
	{
		// Image output

		getNumericAttribute<UInt>(output.mMipmaps,outputnode,"mipmaps");

		{
			string dynamicSize;
			getStringAttribute(dynamicSize,outputnode,"dynamicSize");
			output.mSizeSpace = dynamicSize=="yes" ? OutputDesc::SizeSpace::Relative : OutputDesc::SizeSpace::Absolute;
		}

		{
			UInt width,height;
			getNumericAttribute<UInt>(width,outputnode,"width");
			getNumericAttribute<UInt>(height,outputnode,"height");

			output.mWidth = Utils::log2(width);
			output.mHeight = Utils::log2(height);

			// In relative space, compute the distance of width and height from the default size
			// used by the cooker.
			if (output.mSizeSpace==OutputDesc::SizeSpace::Relative) {
				output.mWidth -= 8;
				output.mHeight -= 8;
			}
		}

		output.mGuiType = OutputGuiType_Image;
	}
	else
	{
		// Numerical/string output
		output.mSizeSpace = OutputDesc::SizeSpace::Absolute;
		output.mMipmaps = 0u;
		output.mFormat = ~0;
		output.mWidth = 1;
		output.mHeight = 1;

		switch (output.mType)
		{
			case Substance_IOType_Integer:
			case Substance_IOType_Integer2:
			case Substance_IOType_Integer3:
			case Substance_IOType_Integer4:
				output.mGuiType = OutputGuiType_Integer;
			break;

			default:
				output.mGuiType = OutputGuiType_Float;
		}

	}

	getStringAttribute(output.mUserTag,outputnode,"usertag");

	XmlElement* outputguinode = outputnode->FirstChildElement("outputgui");

	if (outputguinode)
	{
		getStringAttribute(output.mLabel,outputguinode,"label");
		getStringAttribute(output.mGuiVisibleIf,outputguinode,"visibleif");
		getStringAttribute(output.mGroup,outputguinode,"group");
		getStringAttribute(output.mGuiDescription,outputguinode,"description");

		// try to read the channel from the GUI info
		parseChannels(output.mChannelsFull, output.mChannels,output.mChannelsStr,outputguinode);

		if (output.mType==Substance_IOType_Integer)
		{
			string typegui;
			getStringAttribute(typegui,outputguinode,"typegui");
			if (typegui=="boolean")
			{
				output.mGuiType = OutputGuiType_Boolean;
			}
		}
	}

	output.mXMP = getXmpPacket(outputnode);

	return true;
}


//! @brief Parse channel if present in XML node
void SubstanceAir::Details::parseChannels(
	vector<ChannelFullDesc>& channelsFullDescs,
	vector<ChannelUse>& channels,
	vector<string>& channelsStr,
	XmlElement* parentnode)
{
	channels.clear();
	channelsStr.clear();

	XmlElement* channelsnode = parentnode->FirstChildElement("channels");

	if (channelsnode)
	{
		for (XmlElement* channelnode = channelsnode->FirstChildElement("channel");
		     channelnode;
		     channelnode = channelnode->NextSiblingElement("channel"))
		{
			ChannelFullDesc fullDesc;

			fullDesc.mComponent = ChannelComponent_RGBA;
			string componentsStr;
			getStringAttribute(componentsStr, channelnode, "components");
			if (!componentsStr.empty())
			{
				const char* componentsCStr = componentsStr.c_str();
				for (int i = 0; i < componentAssociationCount; i++)
				{
					if (strcmp(componentsCStr, componentAssociations[i].componentString) == 0)
					{
						fullDesc.mComponent = componentAssociations[i].component;
						break;
					}
				}
			}

			string channelStr;
			getStringAttribute(channelStr,channelnode,"names");
			const char* const* channelNameEnd = getChannelNames()+Channel_INTERNAL_COUNT;
			const char* const* channelFound = std::find(getChannelNames(),channelNameEnd,channelStr);
			ChannelUse channel = Channel_UNKNOWN;
			if (channelFound!=channelNameEnd)
			{
				channel = (ChannelUse) std::distance(getChannelNames(),channelFound);
			}
			fullDesc.mUsageStr = channelStr;
			fullDesc.mUsage = channel;

			string colorSpaceStr;
			getStringAttribute(colorSpaceStr, channelnode, "colorspace");
			const char* const* colorSpaceNameEnd = getColorSpaceNames() + ColorSpace_INTERNAL_COUNT;
			const char* const* colorSpaceFound = std::find(getColorSpaceNames(), colorSpaceNameEnd, colorSpaceStr);
			ColorSpace colorSpace = ColorSpace_UNKNOWN;
			if (colorSpaceFound != colorSpaceNameEnd)
			{
				colorSpace = (ColorSpace) std::distance(getColorSpaceNames(), colorSpaceFound);
			}
			fullDesc.mColorSpaceStr = colorSpaceStr;
			fullDesc.mColorSpace = colorSpace;

			channelsStr.push_back(channelStr);
			channels.push_back(channel);
			channelsFullDescs.push_back(fullDesc);
		}
	}
}


//! @brief Parse an input node from the substance xml companion file
SubstanceAir::InputDescBase* SubstanceAir::Details::parseInput(
	XmlElement* inputnode,
	GraphDesc& graph)
{
	unique_ptr<InputDescBase> input;
	InputDescImage* inputImg = nullptr;

	{
		int type = -1;
		inputnode->QueryIntAttribute("type",&type);

		switch (type)
		{
			case Substance_IOType_Float:
				input = parseInputNumerical<float>(inputnode); break;
			case Substance_IOType_Float2:
				input = parseInputNumerical<Vec2Float>(inputnode); break;
			case Substance_IOType_Float3:
				input = parseInputNumerical<Vec3Float>(inputnode); break;
			case Substance_IOType_Float4:
				input = parseInputNumerical<Vec4Float>(inputnode); break;
			case Substance_IOType_Integer:
				input = parseInputNumerical<int>(inputnode); break;
			case Substance_IOType_Integer2:
				input = parseInputNumerical<Vec2Int>(inputnode); break;
			case Substance_IOType_Integer3:
				input = parseInputNumerical<Vec3Int>(inputnode); break;
			case Substance_IOType_Integer4:
				input = parseInputNumerical<Vec4Int>(inputnode); break;
			case Substance_IOType_Image:
			{
				input = static_unique_ptr_cast<InputDescBase>(make_unique<InputDescImage>());
				inputImg = static_cast<InputDescImage*>(input.get());
				break;
			}
			case Substance_IOType_String:
				input = static_unique_ptr_cast<InputDescBase>(make_unique<InputDescString>(inputnode->Attribute("default"))); break;
			default:
				return nullptr;
		}

		input->mType = (SubstanceIOType)type;
	}

	getStringAttribute(input->mIdentifier,inputnode,"identifier");

	// $outputsize and $randomseed are heavyduty input
	input->mIsDefaultHeavyDuty = (input->mIdentifier[0] == '$');
	getNumericAttribute<UInt>(input->mUid,inputnode,"uid");

	getStringAttribute(input->mUserTag,inputnode,"usertag");

	{
		// Parse alter outputs list and match uid with outputs
		string alteroutputs;
		getStringAttribute(alteroutputs,inputnode,"alteroutputs");
		stringstream sstream(alteroutputs);
		string token;
		while (std::getline(sstream,token,','))
		{
			const UInt uid = Details::checkedCast<UInt>(token.c_str());
			const size_t outind = graph.findOutput(uid);
			if (outind<graph.mOutputs.size())
			{
				// push the output in the list of outputs altered by the input
				input->mAlteredOutputUids.push_back(uid);

				// and the inverse list
				graph.mOutputs[outind].mAlteringInputUids.push_back(input->mUid);
			}
			else
			{
				// TODO Warning: unknown output IF not unused $time / $normalformat
			}
		}

		std::sort(input->mAlteredOutputUids.begin(),input->mAlteredOutputUids.end());
	}

	// Store <inputgui> Xml element
	{
		XmlElement *inpguinode = inputnode->FirstChildElement("inputgui");
		if (inpguinode)
		{
			// Get label
			getStringAttribute(input->mLabel,inpguinode,"label");

			// Get display mode
			{
				SubstanceAir::string showas;
				getStringAttribute(showas,inpguinode,"showas");
				input->mShowAsPin = showas.empty() ?
					inputImg!=nullptr :               // If not set, input image are shown as pin by default
					showas=="pin";
			}

			// Get visibleif expression
			getStringAttribute(input->mGuiVisibleIf,inpguinode,"visibleif");

			// Parse channels
			parseChannels(input->mChannelsFull, input->mChannels,input->mChannelsStr,inpguinode);

			{
				// Get widget type
				string widget;
				getStringAttribute(widget,inpguinode,"widget");
				static const char* const widgetNames[] = { "",
					"slider","angle","color","togglebutton","enumbuttons","combobox","image","position" };
				const char* const* widgetNameEnd = widgetNames+Input_INTERNAL_COUNT;
				const char* const* widgetName = std::find(
					widgetNames,widgetNameEnd,widget);
				input->mGuiWidget = (InputWidget)(
					std::distance(widgetNames,widgetName)%Input_INTERNAL_COUNT);

				// Parse spot color info if available
				if (input->mGuiWidget == Input_Color &&
				    (input->mType == Substance_IOType_Float3 || input->mType == Substance_IOType_Float4))
				{
					string spotColorInfo;
					getStringAttribute(spotColorInfo, inputnode, "spotcolorinfo");
					static_cast<InputDescNumericalBase*>(input.get())->mSpotColorInfo = spotColorInfo;
				}
			}

			// Image color flag / format / def. color / channels
			if (inputImg!=nullptr)
			{
				XmlElement *guiimgnode = inpguinode->FirstChildElement("guiimage");
				if (guiimgnode)
				{
					{
						string colortype;
						getStringAttribute(colortype,guiimgnode,"colortype");
						inputImg->mIsColor = colortype=="color";
					}
					{
						// Parse default colors for image inputs (new in engine v8)
						const char* defAttribute = inputnode->Attribute("default");
						if (defAttribute)
						{
							if (inputImg->mIsColor)
							{
								inputImg->mDefaultColor = checkedCast<Vec4Float>(defAttribute);
							}
							else
							{
								float lum = checkedCast<float>(defAttribute);
								inputImg->mDefaultColor = Vec4Float(lum, lum, lum, 1.0f);
							}
						}
					}
					{
						string format;
						getStringAttribute(format,guiimgnode,"format");
						inputImg->mIsFPFormat = format=="float";
					}

					// For compatibility sake
					if (input->mChannels.empty())
					{
						parseChannels(input->mChannelsFull, input->mChannels,input->mChannelsStr,guiimgnode);
					}
				}
			}

			// Get group (identifier, will be replaced by label later)
			getStringAttribute(input->mGuiGroup,inpguinode,"group");

			// Get description
			getStringAttribute(input->mGuiDescription,inpguinode,"description");
		}
		else
		{
			input->mLabel = input->mIdentifier;		// use identifier instead
			input->mGuiWidget = Input_NoWidget;
			input->mShowAsPin = false;
		}
	}

	input->mXMP = getXmpPacket(inputnode);

	return input.release();
}


//! @brief Parse a preset element
void SubstanceAir::Details::parsePreset(
	Preset &preset,
	XmlElement* sbspresetnode,
	const GraphDesc* graphDesc)
{
	// Parse preset
	getStringAttribute(preset.mPackageUrl,sbspresetnode,"pkgurl");
	preset.mPackageUrl = graphDesc!=nullptr ?
		graphDesc->mPackageUrl :
		preset.mPackageUrl;
	getStringAttribute(preset.mLabel,sbspresetnode,"label");
	getStringAttribute(preset.mDescription,sbspresetnode,"description");

	// Loop on preset inputs
	AIR_XMLFOREACH (presetinputnode,sbspresetnode,"presetinput")
	{
		preset.mInputValues.resize(preset.mInputValues.size()+1);
		Preset::InputValue &inpvalue = preset.mInputValues.back();

		inpvalue.mUid = 0;
		getNumericAttribute<UInt>(inpvalue.mUid,presetinputnode,"uid");

		int type = 0xFFFFu;

		if (graphDesc)
		{
			// Internal preset
			size_t inputIndex = graphDesc->findInput(inpvalue.mUid);
			if (inputIndex<graphDesc->mInputs.size())
			{
				const InputDescBase *inpdesc = graphDesc->mInputs[inputIndex];
				type = inpdesc->mType;
				inpvalue.mIdentifier = inpdesc->mIdentifier;
			}
			else
			{
				// TODO Warning: cannot match internal preset
				inpvalue.mType = (SubstanceIOType)0xFFFF;
			}
		}
		else
		{
			// External preset
			presetinputnode->QueryIntAttribute("type",&type);
			getStringAttribute(inpvalue.mIdentifier,presetinputnode,"identifier");
		}

		inpvalue.mType = (SubstanceIOType)type;

		getStringAttribute(
			inpvalue.mValue,
			presetinputnode,
			inpvalue.mType==Substance_IOType_Image ? "filepath" : "value");
	}
}


//! @brief Parse a list of presets
void SubstanceAir::Details::parsePresets(
	Presets &presets,
	XmlElement* sbspresetsnode,
	const GraphDesc* graphDesc)
{
	// Reserve presets array
	{
		size_t presetsCount = 0;
		AIR_XMLFOREACH (sbspresetnode,sbspresetsnode,"sbspreset")
		{
			++presetsCount;
		}
		presets.reserve(presetsCount+presets.size());
	}

	AIR_XMLFOREACH (sbspresetnode,sbspresetsnode,"sbspreset")
	{
		presets.resize(presets.size()+1);
		Preset &preset = presets.back();
		parsePreset(preset,sbspresetnode,graphDesc);
	}
}


SubstanceAir::string SubstanceAir::Details::encodeToXmlEntities(const SubstanceAir::string& str)
{
	TIXML_STRING res;
	TINYXML_CUSTOM_NAMESPACE TiXmlBase::EncodeString(
		TIXML_STRING(str.c_str()),
		&res);
	return string(res.c_str());
}


SubstanceAir::string SubstanceAir::Details::getXmpPacket(XmlElement* node)
{
	XmlElement *xpacket = node->FirstChildElement("rdf:RDF");
	if (xpacket)
	{
		TiXmlPrinter printer;
		xpacket->Accept(&printer);
		return string(printer.CStr(),printer.Size());
	}
	return string();
}
