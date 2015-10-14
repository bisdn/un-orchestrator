#ifndef OVS_CONSTANTS_H_
#define OVS_CONSTANTS_H_ 1

/**
*	Scripts exploited by OVS plugin
*	(The script path is followed by some %s, exploited by the code to create the proper command line
*/

#define PATH_SCRIPT_VIRTUAL_LINK 	"./network_controller/switch_manager/plugins/ovs/scripts/VirtualLink.sh %s %s %s %s"
#define PATH_SCRIPT_ID_PORT			"./network_controller/switch_manager/plugins/ovs/scripts/IdPort.sh %s %s"
#define PATH_SCRIPT_OF_VERSION		"./network_controller/switch_manager/plugins/ovs/scripts/OFVersion.sh %s %s"

#define TMP_XML_FILE				"file.xml"

#define SOCKET_IP					"127.0.0.1"
#define SOCKET_PORT					"6632"

#endif //OVS_CONSTANTS_H_
