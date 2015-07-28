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
*
*	@messages:
*		PUT /graph/graph_id
*			Create a new graph with ID graph_id if it is does not exist yet;
*			otherwise, the graph is updated.
*			The graph is described into the body of the message.
*		GET /graph/graph_id
*			Retrieve the description of the graph with ID graph_id
*		DELETE /graph/graph_id
*			Delete the graph with ID graph_id
*		DELETE /garph/graph_id/flow_id
*			Remove the flow with ID flow_id from the graph with ID graph_id
*
*		GET /interfaces
*			Retrieve information on the physical interfaces available on the
*			node
*/


#pragma once

#define __STDC_FORMAT_MACROS

#include <microhttpd.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sstream>

#include "../graph_manager/graph_manager.h"
#include "../../utils/constants.h"
#include "../graph/high_level_graph/high_level_action_port.h"
#include "../graph/high_level_graph/high_level_action_endpoint.h"
#include "match_parser.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <sstream>
#include <fstream>

class GraphManager;
			
class RestServer
{
private:

	static GraphManager *gm;

#ifdef UNIFY_NFFG
	static bool firstTime;
#endif
	

	struct connection_info_struct
	{
		char *message;
		size_t length;
	};

	static int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

	static int doGet(struct MHD_Connection *connection,const char *url);
	static int doGetGraph(struct MHD_Connection *connection,char *graphID);
	static int doGetInterfaces(struct MHD_Connection *connection);
	
	static int doPut(struct MHD_Connection *connection, const char *url, void **con_cls);
	static bool parsePutBody(struct connection_info_struct &con_info,highlevel::Graph &graph, bool newGraph);
	
	static int doDelete(struct MHD_Connection *connection,const char *url, void **con_cls);
	
	static int createGraphFromFile(string toBeCreated);
	static bool parseGraphFromFile(string toBeCreated,highlevel::Graph &graph, bool newGraph);
	
	static bool parseGraph(Value value, highlevel::Graph &graph, bool newGraph);
	
	static bool readGraphFromFile(char *nffg_filename);
#ifdef UNIFY_NFFG
	static bool readRulesToBeRemovedFromFile(char *filename);
#endif
	
public:
	static bool init(char *nffg_filename,int core_mask, char *ports_file_name);
	
	static void terminate();

	static int answer_to_connection (void *cls, struct MHD_Connection *connection,
						const char *url, const char *method, const char *version,
						const char *upload_data,size_t *upload_data_size, void **con_cls);
						
	static void request_completed (void *cls, struct MHD_Connection *connection, void **con_cls,
						enum MHD_RequestTerminationCode toe);						
};

#endif //REST_SERVER_H_
