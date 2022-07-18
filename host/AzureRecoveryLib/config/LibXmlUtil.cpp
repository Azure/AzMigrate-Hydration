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
#include "LibXmlUtil.h"
#include "HostInfoDefs.h"

#include <boost/assert.hpp>

namespace AzureRecovery
{

/*
Method      : LibXmlUtil::xGetChildEleNode

Description : Retrieves the first child element node of current node.

Parameters  : [in] node : curremt node ptr

Return Code : Return the first child element node if exist, otherwise NULL
*/
xmlNodePtr LibXmlUtil::xGetChildEleNode(xmlNodePtr node)
{
	xmlNodePtr curr_node = node ? node->children : NULL;

	while (curr_node &&
		(XML_ELEMENT_NODE != curr_node->type)
		)
		curr_node = curr_node->next;

	return curr_node;
}

/*
Method      : LibXmlUtil::xGetNextEleNode

Description : Retrieves the next element node of current node.

Parameters  : [in] node : curremt node ptr

Return Code : Return the next element node if exist, otherwise NULL
*/
xmlNodePtr LibXmlUtil::xGetNextEleNode(xmlNodePtr node)
{
	xmlNodePtr curr_node = node ? node->next : NULL;

	while (curr_node &&
		(XML_ELEMENT_NODE != curr_node->type)
		)
		curr_node = curr_node->next;

	return curr_node;
}

/*
Method      : LibXmlUtil::xGetProp

Description : Retrieves the specified property value from a given node.

Parameters  : [in] node: current node ptr
              [in] prop_name: property name

Return Code : returns specified property value from the node if property exist, 
              otherwise empty string.
*/
std::string LibXmlUtil::xGetProp(xmlNodePtr node, const std::string & prop_name)
{
	BOOST_ASSERT(NULL != node);

	std::string propVal;
	if (!node || prop_name.empty()) return propVal;

	xmlChar *pProp = xmlGetProp(node, (const xmlChar*)prop_name.c_str());

	if (pProp)
	{
		propVal = (const char*)pProp;
		xmlFree(pProp);
	}

	return propVal;
}

/*
Method      : LibXmlUtil::xGetChildNodeWithName

Description : Looks for a child node with specified name.

Parameters  : [in] node : current node
              [in] child_node: child node name

Return Code : Returns child node if exist with specified node name, otherwise NULL
*/
xmlNodePtr LibXmlUtil::xGetChildNodeWithName(xmlNodePtr node, const std::string& child_node)
{
	xmlNodePtr curr_child = xGetChildEleNode(node);

	while (curr_child)
	{
		if (0 == xmlStrcasecmp(curr_child->name, (const xmlChar*)child_node.c_str()))
			break;

		curr_child = xGetNextEleNode(curr_child);
	}

	return curr_child;
}

/*
Method      : LibXmlUtil::xGetNextNodeWithName

Description : Looks for a next node with specified name.

Parameters  : [in] node : current node
              [in] next_node_name: child node name

Return Code : Returns next node if exist with specified node name, otherwise NULL
*/
xmlNodePtr LibXmlUtil::xGetNextNodeWithName(xmlNodePtr node, const std::string& next_node_name)
{
	xmlNodePtr next_node = xGetNextEleNode(node);
	while (next_node)
	{
		if (0 == xmlStrcasecmp(next_node->name, (const xmlChar*)next_node_name.c_str()))
			break;

		next_node = xGetNextEleNode(next_node);
	}

	return next_node;
}

/*
Method      : LibXmlUtil::xGetParamNode_Name_Value_Attrs

Description : Retreives the Name, Value properties from the given xml node.

Parameters  : [in] node : current node
              [out] Name : outparam for Name property value
			  [out] Value : outparam for Value property value

Return Code : None
If those properties are not available then the value for Name & Value are set with empty strings.
*/
void LibXmlUtil::xGetParamNode_Name_Value_Attrs(xmlNodePtr node, std::string& Name, std::string& Value)
{
	if (NULL == node) return;

	Name = xGetProp(node, HostInfoXmlNode::ATTRIBUTE_NAME);
	Value = xGetProp(node, HostInfoXmlNode::ATTRIBUTE_VALUE);
}

/*
Method      : LibXmlUtil::xGetParamGrp_Id

Description : Retreives the Id property from given xml node.

Parameters  : [in] node : current node

Return Code : returns the Id property value of given node, an empty string if property not found
*/
std::string LibXmlUtil::xGetParamGrp_Id(xmlNodePtr node)
{
	return xGetProp(node, HostInfoXmlNode::ATTRIBUTE_ID);
}

/*
Method      : LibXmlUtil::xGetNextParamGrp
              LibXmlUtil::xGetNextParam

Description : 

Parameters  : None

Return Code : Returns next parameter or parametergroup nodes
*/
xmlNodePtr LibXmlUtil::xGetNextParamGrp(xmlNodePtr node)
{
	return xGetNextNodeWithName(node, HostInfoXmlNode::PARAMETER_GROUP);
}

xmlNodePtr LibXmlUtil::xGetNextParam(xmlNodePtr node)
{
	return xGetNextNodeWithName(node, HostInfoXmlNode::PARAMETER);
}

/*
Method      : LibXmlUtil::xGetNextParamGrpWithId

Description : Gets next parameter grup with given Id as its property. The id comparision is not case sensitive.
              If nCmp is set to 'true' then strncmp will happen with next node Id propeties, it nCmp is 'false' 
			  then strcmp will happen with next node lookup.

Parameters  : [in] node: current node ptr
              [in] id: id property value to be compared
			  [in] nCmp: if true then strncmp with Id propety orthwise strcmp

Return Code : return next parametergroup node ptr with give id value, otherwise NULL will be returned
*/
xmlNodePtr LibXmlUtil::xGetNextParamGrpWithId(xmlNodePtr node, const std::string& id, bool nCmp)
{
	xmlNodePtr paramGrpNode = xGetNextNodeWithName(node, HostInfoXmlNode::PARAMETER_GROUP);
	while (paramGrpNode)
	{
		if (nCmp)
		{
			if (0 == xmlStrncasecmp((const xmlChar*)xGetParamGrp_Id(paramGrpNode).c_str(),
				(const xmlChar*)id.c_str(),
				id.length()))
				break;
		}
		else
		{
			if (0 == xmlStrcasecmp((const xmlChar*)xGetParamGrp_Id(paramGrpNode).c_str(),
				(const xmlChar*)id.c_str()))
				break;
		}

		paramGrpNode = xGetNextNodeWithName(paramGrpNode, HostInfoXmlNode::PARAMETER_GROUP);
	}

	return paramGrpNode;
}

/*
Method      : LibXmlUtil::xGetChildParamGrpWithId

Description : Gets first child parameter grup with given Id as its property. The id comparision is not case sensitive.
              If nCmp is set to 'true' then strncmp will happen with child node Id propeties, it nCmp is 'false'
              then strcmp will happen with child node lookup.

Parameters  : [in] node: current node ptr
              [in] id: id property value to be compared
              [in] nCmp: if true then strncmp with Id propety orthwise strcmp

Return Code : return first child parametergroup node ptr with give id value, otherwise NULL will be returned
*/
xmlNodePtr LibXmlUtil::xGetChildParamGrpWithId(xmlNodePtr node, const std::string& id, bool nCmp)
{
	xmlNodePtr paramGrpNode = xGetChildNodeWithName(node, HostInfoXmlNode::PARAMETER_GROUP);
	while (paramGrpNode)
	{
		if (nCmp)
		{
			if (0 == xmlStrncasecmp((const xmlChar*)xGetParamGrp_Id(paramGrpNode).c_str(), 
				(const xmlChar*)id.c_str(), 
				id.length()))
				break;
		}
		else
		{
			if (0 == xmlStrcasecmp((const xmlChar*)xGetParamGrp_Id(paramGrpNode).c_str(),
				(const xmlChar*)id.c_str()))
				break;
		}

		paramGrpNode = xGetNextNodeWithName(paramGrpNode, HostInfoXmlNode::PARAMETER_GROUP);
	}

	return paramGrpNode;
}

}
