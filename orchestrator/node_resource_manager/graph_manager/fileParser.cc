#include "fileParser.h"

#ifndef UNIFY_NFFG
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
#else
set<CheckPhysicalPortsIn> FileParser::parsePortsFile(string fileName)
{
	//The configuration file has the format required by the NF-FG library defined in Unify.
	//Then, let's exploit the NF-FG library to parse it and retrieve the ports!
	
	set<CheckPhysicalPortsIn> physicalPorts;
	
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL) 
	{ 
    	PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_INIT_ORCH);
    	if (pythonFunction && PyCallable_Check(pythonFunction)) 
    	{
			PyObject *pythonArgs = PyTuple_New(1);	
	    	PyObject *pythonValue = PyString_FromString(fileName.c_str());
	    	if(!pythonValue)
	    	{
	    		Py_DECREF(pythonArgs);
				Py_DECREF(pythonFile);
				//TODO: put an error message
				throw new FileParserException();		
	    	}
	    	PyTuple_SetItem(pythonArgs, 0, pythonValue);
	    	
	    	PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);
                     
			assert(PyList_Check(pythonRetVal));
            
            int count = (int) PyList_Size(pythonRetVal);
            assert(count%2 == 0);
		    for (int i = 0 ; i < count ; i+=2 )
		    {
		        PyObject *id = PyList_GetItem(pythonRetVal,i);
		        PyObject *name = PyList_GetItem(pythonRetVal,i+1);
		        logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "%s - %s",PyString_AsString(id),PyString_AsString(name));
		        
		        //TODO: handle the ID!
		        physicalPortType_t ptype = ETHERNET_PORT; //FIXME: probably this is not always correct
				physicalPortSide_t pside = NONE;	//FIXME: is this information important? I can completely remove it from the code
				string aux = PyString_AsString(name);
				CheckPhysicalPortsIn cppi(aux,ptype,pside);
				physicalPorts.insert(cppi);
		    }
            
            Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
    	}
    	else 
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_INIT_ORCH);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        throw new FileParserException();		
		}
	}
	else
    {
	   	PyErr_Print();
	   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
	   	throw new FileParserException();
	}

	return physicalPorts;
}
#endif

#ifndef UNIFY_NFFG
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
#endif

