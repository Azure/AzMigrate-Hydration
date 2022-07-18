/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: LibXmlUtil.h

Description	: LibXmlUtil is a wrapper around LibXml C functions, the LibXmlUtil helper 
              function makes it easy to access/get the xml nodes and its properties.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_HOST_INFO_XML_UTIL_H
#define AZURE_RECOVERY_HOST_INFO_XML_UTIL_H

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <string>

namespace AzureRecovery
{
	struct LibXmlUtil
	{
		static xmlNodePtr xGetChildEleNode(xmlNodePtr node);

		static xmlNodePtr xGetNextEleNode(xmlNodePtr node);

		static std::string xGetProp(xmlNodePtr node, const std::string & prop_name);

		static xmlNodePtr xGetChildNodeWithName(xmlNodePtr node, const std::string& child_node_name);

		static xmlNodePtr xGetNextNodeWithName(xmlNodePtr node, const std::string& next_node_name);

		static xmlNodePtr xGetNextParam(xmlNodePtr node);

		static xmlNodePtr xGetNextParamGrp(xmlNodePtr node);

		static xmlNodePtr xGetNextParamGrpWithId(xmlNodePtr node, const std::string& id, bool nCmp = false);

		static xmlNodePtr xGetChildParamGrpWithId(xmlNodePtr node, const std::string& id, bool nCmp = false);

		static void xGetParamNode_Name_Value_Attrs(xmlNodePtr node, std::string& Name, std::string& Value);

		static std::string xGetParamGrp_Id(xmlNodePtr node);
	};
}

#endif//~AZURE_RECOVERY_HOST_INFO_XML_UTIL_H
