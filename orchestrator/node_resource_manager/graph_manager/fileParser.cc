#include "fileParser.h"

set<CheckPhysicalPortsIn> FileParser::parsePortsFile(string fileName)
{
	set<CheckPhysicalPortsIn> physicalPorts;

	set<std::string>::iterator it;
	xmlDocPtr schema_doc=NULL;
 	xmlSchemaParserCtxtPtr parser_ctxt=NULL;
	xmlSchemaPtr schema=NULL;
	xmlSchemaValidCtxtPtr valid_ctxt=NULL;
	xmlDocPtr doc=NULL;

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Reading configuration file: %s",fileName.c_str());

	//Validate the configuration file with the schema
	schema_doc = xmlReadFile(PHY_PORTS_XSD, NULL, XML_PARSE_NONET);
	if (schema_doc == NULL) 
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The schema \"%s\" cannot be loaded or is not well-formed.",PHY_PORTS_XSD);
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}
	
	parser_ctxt = xmlSchemaNewDocParserCtxt(schema_doc);
	if (parser_ctxt == NULL) 
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a parser context for the schema \"%s\".",PHY_PORTS_XSD);
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}

	schema = xmlSchemaParse(parser_ctxt);
	if (schema == NULL) 
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "The XML \"%s\" schema is not valid.",PHY_PORTS_XSD);
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}

	valid_ctxt = xmlSchemaNewValidCtxt(schema);
	if (valid_ctxt == NULL) 
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Unable to create a validation context for the XML schema \"%s\".",PHY_PORTS_XSD);
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}
	
	doc = xmlParseFile(fileName.c_str()); /*Parse the XML file*/
	if (doc==NULL)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "XML file '%s' parsing failed.", fileName.c_str());
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}

	if(xmlSchemaValidateDoc(valid_ctxt, doc) != 0)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Configuration file '%s' is not valid", fileName.c_str());
		/*Free the allocated resources*/
		freeXMLResources(parser_ctxt, valid_ctxt, schema_doc, schema, doc);
		throw new FileParserException();
	}
	
	xmlNodePtr root = xmlDocGetRootElement(doc);

	for(xmlNodePtr cur_root_child=root->xmlChildrenNode; cur_root_child!=NULL; cur_root_child=cur_root_child->next) 
	{
		if ((cur_root_child->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur_root_child->name, (const xmlChar*)PORT_ELEMENT)))
		{
			xmlChar* attr_name = xmlGetProp(cur_root_child, (const xmlChar*)NAME_ATTRIBUTE);
			xmlChar* attr_type = xmlGetProp(cur_root_child, (const xmlChar*)TYPE_ATTRIBUTE);
			xmlChar* attr_side = xmlGetProp(cur_root_child, (const xmlChar*)SIDE_ATTRIBUTE);

			assert(attr_name != NULL);
			assert(attr_type != NULL);
			assert(attr_side != NULL);

			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Port: %s - Type: %s - Side: %s",attr_name,attr_type,attr_side);

			string name((const char*)attr_name);
			string type((const char*)attr_type);
			string side((const char*)attr_side);

			physicalPortType_t ptype = (type == "ethernet")? ETHERNET_PORT : WIFI_PORT;
			physicalPortSide_t pside = (side == "core")? CORE : ( (side == "edge")? EDGE : NONE );
	
			CheckPhysicalPortsIn cppi(name,ptype,pside);
			physicalPorts.insert(cppi);
		}
	}
	
	return physicalPorts;
}

void FileParser::freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc)
{
	if(valid_ctxt!=NULL)
		xmlSchemaFreeValidCtxt(valid_ctxt);

	if(schema!=NULL)
		xmlSchemaFree(schema);

	if(parser_ctxt!=NULL)
	    xmlSchemaFreeParserCtxt(parser_ctxt);

	if(schema_doc!=NULL)    
		xmlFreeDoc(schema_doc);

	if(doc!=NULL)
		xmlFreeDoc(doc); 

	xmlCleanupParser();
}

