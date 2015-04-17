#ifndef XDPD_CONSTANTS_H_
#define XDPD_CONSTANTS_H_ 1

/**
*	IP address and TCP port to be used to send commands to xDPd
*/
#define XDPD_ADDRESS                   	"127.0.0.1"
#define XDPD_PORT			"2525"

/*
*       Commands to xDPD - Answers from xDPD
*/
#define DISCOVER_PHY_PORTS              "discover-physical-ports"
#define CREATE_LSI                      "create-lsi"
#define CREATE_NF_PORTS                 "create-nfs-ports"
#define CREATE_VLINKS                   "create-virtual-links"
#define DESTROY_LSI                     "destroy-lsi"
#define DESTROY_NF_PORTS                "destroy-nfs-ports"
#define DESTROY_VLINKS                  "destroy-virtual-links"
#define OK                              "ok"
#define XDPD_ERROR                      "error"


#endif //XDPD_CONSTANTS_H
