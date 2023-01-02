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

#ifndef _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSXMLHELPERS_H__
#define _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSXMLHELPERS_H__

#include <substance/framework/graph.h>

// TinyXML custom namespace definition
#ifndef TINYXML_CUSTOM_NAMESPACE
#define TINYXML_CUSTOM_NAMESPACE
#endif

#include <tinyxml.h>





namespace SubstanceAir
{
namespace Details
{

typedef TINYXML_CUSTOM_NAMESPACE TiXmlDocument XmlDocument;
typedef TINYXML_CUSTOM_NAMESPACE TiXmlElement XmlElement;


//! @brief Xml elements foreach
//! @param childvar Child element variable
//! @param parentnode Parent XML node, skip if nullptr
//! @param nodelabel Label of nodes to iterate
#define AIR_XMLFOREACH(childvar,parentnode,label) \
	for (Details::XmlElement *pnode =(parentnode), \
			*childvar = pnode!=nullptr ? pnode->FirstChildElement(label) : nullptr; \
		childvar; \
		childvar = childvar->NextSiblingElement(label))


//! @brief Parse an output node from the Substance Air xml desc file
//! @note Not reading the default size as we do not use it
bool parseOutput(
	XmlElement* outputnode,
	OutputDesc& output);

//! @brief Parse an input node from the substance xml companion file
InputDescBase* parseInput(
	XmlElement* inputnode,
	GraphDesc& graph);

//! @brief Parse a preset
void parsePreset(
	Preset &preset,
	XmlElement* sbspresetnode,
	const GraphDesc* graphDesc);

//! @brief Parse a list of presets
void parsePresets(
	Presets &presets,
	XmlElement* sbspresetsnode,
	const GraphDesc* graphDesc);

//! @brief Parse channel if present in XML node
void parseChannels(
	vector<ChannelFullDesc>& channelsFullDescs,
	vector<ChannelUse>& channels,
	vector<string>& channelsStr,
	XmlElement* parentnode);

//! @brief Return XMP packet as string if exists
string getXmpPacket(XmlElement* node);


void getStringAttribute(string& dst,XmlElement* element,const char* name);

template <typename T>
T checkedCast(const char* str)
{
	T value;
	memset(&value,0,sizeof(value));

	if (str)
	{
		string strtrimmed(str);
		while (!strtrimmed.empty() &&
			(*strtrimmed.rbegin()==' ' || *strtrimmed.rbegin()=='\t'))
		{
			strtrimmed.resize(strtrimmed.size()-1);
		}
		stringstream sstr(strtrimmed);
		sstr >> value;
	}

	return value;
}

template <typename T>
void getNumericAttribute(T& dst,XmlElement* element,const char* name)
{
	dst = checkedCast<T>(element->Attribute(name));
}

string encodeToXmlEntities(const string&);


} // namespace Details
} // namespace SubstanceAir

#endif // _SUBSTANCE_AIR_FRAMEWORK_DETAILS_DETAILSXMLHELPERS_H__
