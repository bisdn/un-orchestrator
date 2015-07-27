#include "fileParser.h"

set<CheckPhysicalPortsIn> FileParser::parseConfigurationFile(string fileName)
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
#ifdef UNIFY_NFFG
		if ((cur_root_child->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur_root_child->name, (const xmlChar*)RESOURCES_ELEMENT)))
		{
			//Element <resources>
			
			int cpu, memory, storage;
			char *memory_unit, *storage_unit;
			
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "RESOURCES");
			xmlNodePtr resources = cur_root_child;
			for(xmlNodePtr resource = resources->xmlChildrenNode; resource != NULL; resource = resource->next) 
			{
				if ((resource->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(resource->name, (const xmlChar*)CPU_ELEMENT)))
				{
					//Element <cpu>					
					xmlChar* attr_amount = xmlGetProp(resource, (const xmlChar*)AMOUNT_ATTRIBUTE);
					assert(attr_amount != NULL);
					sscanf((char*)attr_amount,"%d",&cpu);
				}
				else if ((resource->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(resource->name, (const xmlChar*)MEMORY_ELEMENT)))
				{
					//Element <memory>
					xmlChar* attr_amount = xmlGetProp(resource, (const xmlChar*)AMOUNT_ATTRIBUTE);
					assert(attr_amount != NULL);
					sscanf((char*)attr_amount,"%d",&memory);
					
					xmlChar* attr_unit = xmlGetProp(resource, (const xmlChar*)UNIT_ATTRIBUTE);
					assert(attr_unit != NULL);
					memory_unit  = (char*)malloc(sizeof(char) * (strlen((const char*)attr_unit) + 1));
					memcpy(memory_unit, attr_unit, strlen((const char*)attr_unit));
					memory_unit[strlen((const char*)attr_unit)] = '\0';
				}
				else if ((resource->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(resource->name, (const xmlChar*)STORAGE_ELEMENT)))
				{
					//Element <storage>
					xmlChar* attr_amount = xmlGetProp(resource, (const xmlChar*)AMOUNT_ATTRIBUTE);
					assert(attr_amount != NULL);
					sscanf((char*)attr_amount,"%d",&storage);
					
					xmlChar* attr_unit = xmlGetProp(resource, (const xmlChar*)UNIT_ATTRIBUTE);
					assert(attr_unit != NULL);
					storage_unit  = (char*)malloc(sizeof(char) * (strlen((const char*)attr_unit) + 1));
					memcpy(storage_unit, attr_unit, strlen((const char*)attr_unit));
					storage_unit[strlen((const char*)attr_unit)] = '\0';
				}
			
			}
			
			logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Resources of the node: CPU: %d - memory: %d %s - storage: %d %s",cpu,memory,memory_unit,storage,storage_unit);
			
			if(!Virtualizer::addResources(cpu,memory,memory_unit,storage,storage_unit))
			{
				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Error while setting the resources in the virtualizer");
				throw new FileParserException();
			}
		}//end <resources>
	
		else
#endif		
		if ((cur_root_child->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur_root_child->name, (const xmlChar*)PORTS_ELEMENT)))
		{		
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "PORTS");
			xmlNodePtr ports = cur_root_child;
			for(xmlNodePtr port = ports->xmlChildrenNode; port != NULL; port = port->next) 
			{
				if ((port->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(port->name, (const xmlChar*)PORT_ELEMENT)))
				{
					//Element <port>
		
					xmlChar* attr_name = xmlGetProp(port, (const xmlChar*)NAME_ATTRIBUTE);
					xmlChar* attr_type = xmlGetProp(port, (const xmlChar*)TYPE_ATTRIBUTE);
					xmlChar* attr_side = xmlGetProp(port, (const xmlChar*)SIDE_ATTRIBUTE);

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
					
#ifdef UNIFY_NFFG
					xmlNodePtr virtualizers = port;
					
					for(xmlNodePtr virtualized = virtualizers->xmlChildrenNode; virtualized != NULL; virtualized = virtualized->next) 
					{
						if ((virtualized->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(virtualized->name, (const xmlChar*)VIRTUALIZED_ELEMENT)))
						{
							//Element <virtualized>
							xmlChar* attr_as = xmlGetProp(virtualized, (const xmlChar*)AS_ATTRIBUTE);
							xmlChar* attr_type = xmlGetProp(virtualized, (const xmlChar*)PORT_TYPE_ATTRIBUTE);
							
							logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Virtualized as: name: '%s' - type: '%s'",attr_as,attr_type);
			
							if(!Virtualizer::addPort((char*)attr_name,(char*)attr_as,(char*)attr_type))
							{
								logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Error while adding a port in the virtualizer");
								throw new FileParserException();
							}
						}			
					}
#endif		
				}			
			}
		}//end <ports>
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

