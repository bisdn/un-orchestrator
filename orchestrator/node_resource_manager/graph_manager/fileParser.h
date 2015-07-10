#ifndef FileParser_H_
#define FileParser_ 1

#pragma once

#ifdef UNIFY_NFFG
	#include <Python.h>
	#include "../virtualizer/virtualizer.h"
#endif	

#include "../../network_controller/switch_manager/checkPhysicalPorts_in.h"
#include "../../utils/constants.h"
#include "../../utils/logger.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <set>

using namespace std;

class FileParser
{
private:
	static void freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc);

public:

	/**
	*	@brief: parse the configuration file of the Universal Node, and retrieves information
	*		related to the physical ports to be connected to the node itself
	*/
	static set<CheckPhysicalPortsIn> parseConfigurationFile(string fileName);
	
};

class FileParserException: public exception
{
public:
	virtual const char* what() const throw()
	{
		return "FileParserException";
	}
};


#endif //FileParser_H_
