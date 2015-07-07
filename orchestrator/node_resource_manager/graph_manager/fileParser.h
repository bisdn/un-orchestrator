#ifndef FileParser_H_
#define LSI_H_ 1

#pragma once

#ifdef UNIFY_NFFG
	#include <Python.h>
#endif	

#include "../../network_controller/switch_manager/checkPhysicalPorts_in.h"
#include "../../utils/constants.h"
#include "../../utils/logger.h"

#ifndef UNIFY_NFFG
	#include <libxml/parser.h>
	#include <libxml/tree.h>
	#include <libxml/xmlschemas.h>
#endif

#include <set>

using namespace std;

class FileParser
{
private:
#ifndef UNIFY_NFFG
	static void freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc);
#endif

public:

	/**
	*	@brief: parse the configuration file of the Universal Node, and retrieves information
	*		related to the physical ports to be connected to the node itself
	*/
	static set<CheckPhysicalPortsIn> parseConfigurationFile(string fileName);
	
#ifdef UNIFY_NFFG
	/**
	*	@brief: In the tmp file of the Universal Node, replaces the ID of a given port with the ID provided
	*
	*	@param: portName	Name of the port whose ID has to be replaced
	*	@param: ID			New ID to be associated with the port
	*/
	static void EditPortID(string portName, unsigned int ID);
#endif	
	
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
