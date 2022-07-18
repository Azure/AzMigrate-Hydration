//declared common protocol to be used by VACP Client, Configurator and VACP server for Fabric

#ifndef VACP_TAG_INFO_H
#define VACP_TAG_INFO_H


typedef struct _TAG_IDENTIFIER_AND_DATA_
{
	unsigned short tagIdentifier;
	std::string strTagName;
}TAG_IDENTIFIER_AND_DATA;

typedef struct _VACP_SERVER_CMD_ARG_
{
	bool bRevocationTag;
	unsigned int tagFlag;
    std::vector<std::string> vDeviceId;
    std::vector<TAG_IDENTIFIER_AND_DATA> vTagIdAndData;
}VACP_SERVER_CMD_ARG;

#endif //end