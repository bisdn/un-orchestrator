#ifndef REST_SERVER_H_
#define REST_SERVER_H_ 1

/**
*	@brief: the REST server is based on the microhttpd library:
*				www.gnu.org/software/libmicrohttpd/
*
*	Documentation on HTTP return values can be found at:
*		http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
*
*	Documentation on HTTP headers can be found at:
*		http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
*/


#pragma once

#define __STDC_FORMAT_MACROS

#include <microhttpd.h>
#include <string.h>
#include <assert.h>

#include <set>
#include <string>

#include "nf.h"


#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "constants.h"
#include "logger.h"
			
using namespace json_spirit;			
using namespace std;			
			
class RestServer
{
private:	 
	 
	struct connection_info_struct
	{
	};
	
	static void freeXMLResources(xmlSchemaParserCtxtPtr parser_ctxt, xmlSchemaValidCtxtPtr valid_ctxt, xmlDocPtr schema_doc, xmlSchemaPtr schema, xmlDocPtr doc);
	
	/**
	*	Names of the available NFs
	**/
	static set<NF*> nfs;



	static int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

	static int doGet(struct MHD_Connection *connection,const char *url);
	
public:
	static bool init(string nf_file_description);

	static int answer_to_connection (void *cls, struct MHD_Connection *connection,
						const char *url, const char *method, const char *version,
						const char *upload_data,size_t *upload_data_size, void **con_cls);
						
	static void request_completed (void *cls, struct MHD_Connection *connection, void **con_cls,
						enum MHD_RequestTerminationCode toe);						
};

#endif //REST_SERVER_H_
