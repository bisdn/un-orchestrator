#include "ovsdb_manager.h"
#include "ovsdb_constants.h"

/*Socket*/
struct sockaddr_in saddr1;/* sturct addr server*/
struct in_addr sIPaddr1;/* struct IP addr server*/

int rnumber = 1, rnumber_new = 1;
uint64_t dnumber = 1;
int pnumber = 1, nfnumber = 0;

int id = 0;

/*map use to obtain name of switch from id*/
map<uint64_t, string> switch_id;
/*map use to obtain uuid of switch from name*/
map<uint64_t, string> switch_uuid;
/*map use to obtain vport peer of switch from vport name*/
map<string, string> peer_n;
/*map use to obtain list of ports from bridge-id*/
map<uint64_t, list<string> > port_l;
/*map use to obtain list of vports from bridge-id*/
map<uint64_t, list<string> > vport_l;
/*map use to obtain list of port uuid from bridge-id*/
map<uint64_t, list<string> > port_uuid;
/*map use to obtain list of vport uuid from bridge-id*/
map<uint64_t, list<string> > vport_uuid;
/*map use to obtain virtual link id from bridge name*/
map<string, list<uint64_t> > virtual_link_id;
/*map use to obtain name of port from port_id*/
map<uint64_t, string> port_id;
/*map use to id of switch from vport_id*/
map<uint64_t, uint64_t> vl_id;

//Constructor
commands::commands(){
}

//Destroyer
commands::~commands(){
}

/*connect to a ovs server using Socket*/
int commands::cmd_connect() {
	uint16_t tport_n, tport_h;
	int	result, s;
	
	/*Read ip and port by the server*/
	result = inet_aton(SOCKET_IP, &sIPaddr1);
	if (!result){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Invalid IP address.");
		throw commandsException();
	}
	
	if (sscanf(SOCKET_PORT, "%" SCNu16, &tport_h)!=1){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Invalid number of port.");
		throw commandsException();
	}
  	tport_n = htons(tport_h);

	/*creating Socket */
    logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Creating Socket.");
	if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Creation of Socket failed.");
		throw commandsException();
	}
	
	/*preparing addr struct*/
    saddr1.sin_family = AF_INET;
	saddr1.sin_port   = tport_n;
	saddr1.sin_addr   = sIPaddr1;

	if(connect(s, (struct sockaddr *) &saddr1, sizeof(saddr1)) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Connection to the server failed.");
		throw commandsException();
	}
	
	return s;
}

int commands::cmd_disconnect(int s){
	if(close(s) != 0){
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Closing socket failed.");
		throw commandsException();
	}
	
	return EXIT_SUCCESS;
}

/*
*	@cli = struct CreateLsiIn
*	@FIXME = without virtual Link
*/
CreateLsiOut* commands::cmd_editconfig_lsi (CreateLsiIn cli, int s){

    unsigned int i=0;	
    int len = 0;
    size_t  nleft = 0;
    ssize_t nwritten = 0;
    
    ssize_t r = 0;
	
	const char *ptr = "", *json = "";
	char read_buf[4096];
	
	strcpy(read_buf, "");
	
	/*Information for CreateLsiOut to return*/
	CreateLsiOut *clo = NULL;
	map<string,unsigned int> physical_ports;
	map<string,map<string, unsigned int> >  network_functions_ports;
	list<pair<unsigned int, unsigned int> > virtual_links;
	
    int dnumber_new = 0, nfnumber_old = 0;
    
    //list of ports
	list<string> ports = cli.getPhysicalPortsName();

	//list of nf	
	set<string> nfs = cli.getNetworkFunctionsName();

	//list of nft	
	map<string,nf_t> nf_type = cli.getNetworkFunctionsType(); 	

	//list of remote LSI
	list<uint64_t> vport = cli.getVirtualLinksRemoteLSI();
	
	//list of remote LSI
	char rp[2048][64];
	
	//Local variables
	const char *peer_name = "";

	uint64_t port_id_1 = 0, port_id_2 = 0;
	
	unsigned int nf_size = 0;

	char sw[64] = "Bridge", tcp_s[64] = "tcp:", ctr[64] = "ctrl", vrt[64] = "VirtualPort", trv[64] = "vport";

	char temp[64] = "", tmp[64] = "", of_version[64] = "";
    
    /*set the OpenFlow version*/
	switch(OFP_VERSION)
	{
		case OFP_10:
			strcpy(of_version, "OpenFlow10");
			break;
		case OFP_12:
			strcpy(of_version, "OpenFlow12");
			break;
		case OFP_13:
			strcpy(of_version, "OpenFlow13");
			break;
	}
    
    //connect socket
    s = cmd_connect();
    
    /*Root object contained three object [method, params, id]*/
    Jzon::Object root;
    root.Add("method", "transact");

    Jzon::Array params;
    params.Add("Open_vSwitch");
    
    Jzon::Object first_obj;
    Jzon::Object row;
    Jzon::Array iface;
    Jzon::Array iface1;
    Jzon::Array iface2;
	
	//Create Bridge
    /*create current name of a bridge*/
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(sw, temp);
	
	/*fill the map switch_id*/
	switch_id[dnumber] = string(sw);
	
	Jzon::Array peer;
    Jzon::Array peer1;
    Jzon::Array peer2;
    
    int l = 0;
    
    //Create controller
    /*Create the current target of a controller*/
	strcat(tcp_s, cli.getControllerAddress().c_str());
	strcat(tcp_s, ":");
	strcat(tcp_s, cli.getControllerPort().c_str());
	strcpy(temp, tcp_s);
	
	first_obj.Add("op", "insert");
    first_obj.Add("table", "Controller");
    
    /*Insert a Controller*/
    row.Add("target", temp);
    row.Add("local_ip", "127.0.0.1");
    row.Add("is_connected", true);
    
    first_obj.Add("row", row);
    
    //Create the current name of a controller
	sprintf(temp, "%" PRIu64, dnumber);
	strcat(ctr, temp);
	strcpy(temp, ctr);
	strcpy(ctr, "ctrl");
    
    first_obj.Add("uuid-name", temp);
    
    params.Add(first_obj);
    
    row.Clear();
    first_obj.Clear();
    
	first_obj.Add("op", "insert");
    first_obj.Add("table", "Bridge");
    
    /*Insert a bridge*/
    row.Add("name", sw);
    
    Jzon::Array port;
	Jzon::Array port1;
	Jzon::Array port2;
	
	port.Add("set");
    
	port.Add(port1);
	
	row.Add("ports", port);
	
	port1.Clear();
	port.Clear();
    
    Jzon::Array ctrl;
    ctrl.Add("set");

	Jzon::Array ctrl1;
	
	Jzon::Array ctrl2;

	//Create the current name of a controller
	sprintf(tmp, "%" PRIu64, dnumber);
	strcat(ctr, tmp);
	strcpy(tmp, ctr);
	
	ctrl2.Add("named-uuid");
	ctrl2.Add(tmp);
	
	ctrl1.Add(ctrl2);
	
	ctrl.Add(ctrl1);
    
    row.Add("controller", ctrl);
    
    //Add protocols
    row.Add("protocols", of_version);
    
    first_obj.Add("row", row);
    
    first_obj.Add("uuid-name", sw);
    
    params.Add(first_obj);
    
    row.Clear();
    first_obj.Clear();
    port.Clear();
    port1.Clear();
    port2.Clear();
    ctrl.Clear();
    ctrl1.Clear();
    ctrl2.Clear();
    
    dnumber_new = dnumber;
    
    /*Object with four items [op, table, where, mutations]*/
    Jzon::Object second_obj;
    second_obj.Add("op", "mutate");
    second_obj.Add("table", "Open_vSwitch");
    
    /*Empty array [where]*/
    Jzon::Array where;
    second_obj.Add("where", where);
    
    /*Array with one element*/
    Jzon::Array w_array;
    
    /*Array  with three elements*/
    Jzon::Array m_array;
    m_array.Add("bridges");
    m_array.Add("insert");
    
    /*Array with two elements*/
    Jzon::Array i_array;
    i_array.Add("set");
    
    /*Array with one element*/
    Jzon::Array s_array;
    
    /*Array with two element*/
    Jzon::Array a_array;
    a_array.Add("named-uuid");
    a_array.Add(sw);
    
    s_array.Add(a_array);
    
    i_array.Add(s_array);
    
    m_array.Add(i_array);
    
    w_array.Add(m_array);
    
    second_obj.Add("mutations", w_array);
    
    params.Add(second_obj);
    
    root.Add("params", params);

	root.Add("id", id);

	w_array.Clear();
	m_array.Clear();
	i_array.Clear();
	s_array.Clear();
	a_array.Clear();
	
	//Increment transaction id
	id++;

	Jzon::Writer writer(root, Jzon::StandardFormat);
    writer.Write();
    std::string result = writer.GetResult();
    
    len = result.size();
    
    json = result.c_str();
    
    ptr = result.c_str();
    
    Jzon::Object rootNode;
    ofstream myfile;
    string *strr = new string[256];
    
    for (nleft=len; nleft > 0; )
    {
		nwritten = send(s, ptr, nleft, 0);
		if (nwritten >0)
		{
	    	nleft -= nwritten;
	    	ptr += nwritten;   
		}
    }
    
    strcpy(read_buf, "");
    
    unsigned kkkk = 0;
    
    r = 0;
    
    r = read(s, read_buf, 4096);
    
    read_buf[r-1] = '\0';
    
    string myString(read_buf);
    
  	myfile.open ("file1.json");
  	myfile << read_buf;
  	myfile.close();

	Jzon::FileReader::ReadFile("file1.json", rootNode);

    for (Jzon::Object::iterator it = rootNode.begin(); it != rootNode.end(); ++it)
    {
    	std::string name = (*it).first;
        Jzon::Node &node = (*it).second;

        /*find array [result]*/
        if (node.IsArray())
        {
        	/*convert Node in an Array*/
     		const Jzon::Array &result = node.AsArray();
     	
     		kkkk = result.GetCount();
     	
     		for(i=0;i<result.GetCount();i++){
     			/*retrieve i-node*/
		 		Jzon::Node &node1 = result.Get(i);
		 		
		 		/*convert it in a Object*/
		 		Jzon::Object uuidNode = node1.AsObject();
		 		
		 		if (node1.IsObject()){
		 			/*iterate in this Object*/
		 			for (Jzon::Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
					{
						std::string name1 = (*it1).first;
		    			Jzon::Node &node1 = (*it1).second;
		    			
		    			/*search a uuid array*/
						if(node1.IsArray()){
							const Jzon::Array &stuff1 = node1.AsArray();
							/*retriere the second element of this array*/
		 					Jzon::Node &node2 = stuff1.Get(1);
		 					strr[i] = node2.ToString();
						}
					}
		 		}		
     		}	
        }
	} 
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
    
    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
    
    if(kkkk!=0){
		//store the switch-uuid
    	switch_uuid[dnumber] = strr[i-2];
	}

	/*create ports*/
	if(ports.size() !=0){
	
		ports.reverse();
	
		for(list<string>::iterator p = ports.begin(); p != ports.end(); p++)
		{
			add_ports(rnumber, (*p), dnumber, 0, s);
			
			port_l[dnumber].push_back((*p).c_str());
			physical_ports[(*p)] = rnumber_new;
			
			rnumber++;
			rnumber_new++;
		}
	}
	
	/*Create interfaces related by the nf ports*/
    if(nfs.size() != 0){    		
		/*for each network function name in the list of nfs*/
		for(set<string>::iterator nf = nfs.begin(); nf != nfs.end(); nf++)
		{
			list<string> nfs_ports = cli.getNetworkFunctionsPortNames(*nf);
			
			map<string,unsigned int> n_ports_1;
			
			/*for each network function port in the list of nfs_ports*/
			for(list<string>::iterator nfp = nfs_ports.begin(); nfp != nfs_ports.end(); nfp++){
				add_ports(rnumber, (*nfp), dnumber, 1, s);
				
				locale loc;	
				port2.Add("named-uuid");
				string str = (*nfp);
				
				for (string::size_type i=0; i<str.length(); ++i)
    				str[i] = tolower(str[i], loc);
    				
				strcpy(tmp, (char *)(str).c_str());
				sprintf(temp, "%" PRIu64, dnumber);
				strcat(tmp, "b");
				strcat(tmp, temp);
				strcpy(temp, tmp);
				
				for(unsigned int j=0;j<strlen(temp);j++){
					if(temp[j] == '_')
						temp[j] = 'p';
				}
				
				//insert this port name into port_n
				port_l[dnumber].push_back(temp);
				
				/*fill the map ports*/
				n_ports_1[(*nfp)] = rnumber_new;

				rnumber++;
				rnumber_new++;
				
				nf_size++;
			}
			
			/*fill the network_functions_ports*/
			network_functions_ports[(*nf)] = n_ports_1;
		}
	}
	
	/*Create interfaces related by the vlink ports*/
	if(vport.size() != 0)
	{	
		nfnumber_old = pnumber;
	
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
	    	char ifac[64] = "iface";
	    	
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
			
			port_id_1 = pnumber;
			
			pnumber++;
			
			port_id_2 = pnumber;
			
			sprintf(temp, "%d", pnumber);
			strcat(trv, temp);

			peer_n[trv] = vrt;
				
			strcpy(rp[l], trv);	
			
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			cmd_add_virtual_link(vrt, trv, ifac, dnumber, s);
			
			port_id[port_id_1] = vrt;
			port_id[port_id_2] = trv;
			
			virtual_link_id[sw].push_back(port_id_2);
			
			vport_l[dnumber].push_back(vrt);
				
			vl_id[port_id_1] = dnumber;
			vl_id[port_id_2] = (*nf);
				
			rnumber++;
			pnumber++;
			
			l++;
		}
	}
	
    //increment switch number
    dnumber++;
    
    root.Clear();
    params.Clear();
    row.Clear();
    first_obj.Clear();
    port2.Clear();
    port1.Clear();
    port.Clear();
    peer.Clear();
    peer1.Clear();
    peer2.Clear();
    
    pnumber = nfnumber_old;
    
    uint64_t pi = 0;
    
    Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;
	
	//disconnect socket
    cmd_disconnect(s);
	
	//int nn = l;
	
	l = 0;
	
    if(vport.size() != 0)
	{
		//rnumber_old = rnumber;
	
		for(list<uint64_t>::iterator nf = vport.begin(); nf != vport.end(); nf++)
		{
			//connect socket
			s = cmd_connect();
		
			root.Add("method", "transact");

			params.Add("Open_vSwitch");
		
			pi = (*nf);
			
			strcpy(vrt, "vport");
			strcpy(trv, "vport");
			char ifac[64] = "iface";
				
			sprintf(temp, "%d", pnumber);
			strcat(vrt, temp);
				
			pnumber++;
				
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			//store this vport
			vport_l[pi].push_back(rp[l]);
				
			peer_name = peer_n[rp[l]].c_str();
		
			cmd_add_virtual_link(rp[l], peer_name, ifac, pi, s);
				
			pnumber++;
			rnumber++;
			
			root.Clear();
			params.Clear();
			
			//Increment transaction id
			id++;
			
			l++;
			
			virtual_links.push_back(make_pair(nf_size+ports.size() + l, port_uuid[pi].size() + l));
			
			//disconnect socket
			cmd_disconnect(s);
		}
	}
    	
    rnumber_new = 1;
  
    clo = new CreateLsiOut(dnumber_new, physical_ports, network_functions_ports, virtual_links);
    	
	return clo;
}

void commands::add_ports(int rnumber, string p, uint64_t dnumber, int nf, int s){
	
	char temp[64] = "", tmp[64] = "";
	
	int len;
    size_t  nleft;
    ssize_t nwritten;
	
	const char *ptr, *json;
	char read_buf[4096];
	
	locale loc;	
	
	bool flag = false;
	
	map<string, unsigned int> ports;
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	//connect socket
    s = cmd_connect();

	char ifac[64] = "iface";
		
	//Create the current name of a interface
	if(nf == 0){
		sprintf(temp, "%d", rnumber);
		strcat(ifac, temp);
		
		strcpy(temp, p.c_str());
	}else{
		string str = p;
		
		sprintf(temp, "%d", rnumber);
		strcat(ifac, temp);
				
		//Create port name to lower case
		for (string::size_type i=0; i<str.length(); ++i)
    		str[i] = tolower(str[i], loc);
    				
    	//Create the current port name
		strcpy(tmp, (char *)(str).c_str());
		sprintf(temp, "%" PRIu64, dnumber);
		strcat(tmp, "b");
		strcat(tmp, temp);
		strcpy(temp, tmp);
				
		for(unsigned int j=0;j<strlen(temp);j++){
			if(temp[j] == '_')
				temp[j] = 'p';
		}
	}
	
	root.Add("method", "transact");

	params.Add("Open_vSwitch");
		
	first_obj.Add("op", "insert");
	first_obj.Add("table", "Interface");
		
	/*Insert an Interface*/
	row.Add("name", temp);
	if(nf != 0)
		row.Add("type", "internal");
		
	first_obj.Add("row", row);
		
	first_obj.Add("uuid-name", ifac);
		
	params.Add(first_obj);
		
	row.Clear();
	first_obj.Clear();
			
	first_obj.Add("op", "insert");
	first_obj.Add("table", "Port");
		
	/*Insert a port*/
	row.Add("name", temp);
		
	iface.Add("set");
	
	iface2.Add("named-uuid");
	iface2.Add(ifac);
	
	iface1.Add(iface2);
	iface.Add(iface1);
		
	row.Add("interfaces", iface);
		
	first_obj.Add("row", row);
		
	first_obj.Add("uuid-name", temp);
		
	params.Add(first_obj);
		
	//insert this port name into port_l
	port_l[dnumber].push_back(temp);
		
	row.Clear();
	first_obj.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
		
	first_obj.Add("op", "update");
	first_obj.Add("table", "Bridge");
			   
	third_object.Add("_uuid");
	third_object.Add("==");
		
	fourth_object.Add("uuid");
	fourth_object.Add(switch_uuid[dnumber].c_str());
		
	third_object.Add(fourth_object);
	where.Add(third_object);
		
	first_obj.Add("where", where);
	
	where.Clear();
	
	for(list<string>::iterator u = port_uuid[dnumber].begin(); u != port_uuid[dnumber].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
		
	for(list<string>::iterator u = vport_uuid[dnumber].begin(); u != vport_uuid[dnumber].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
		
	port2.Add("named-uuid");
	port2.Add(temp);
	
	port1.Add(port2);
	
	port2.Clear();
		
	/*Array with two elements*/
	i_array.Add("set");
		   
	i_array.Add(port1);
		
	row.Add("ports", i_array);
		
	first_obj.Add("row", row);
		
	params.Add(first_obj);
		
	second_obj.Clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj.Add("op", "mutate");
	second_obj.Add("table", "Open_vSwitch");
		
	/*Empty array [where]*/
	second_obj.Add("where", where);
		
	/*Array with two element*/
	maa.Add("next_cfg");
	maa.Add("+=");
	maa.Add(1);
			
	ma.Add(maa);
		
	second_obj.Add("mutations", ma);
			
	params.Add(second_obj);
		
	ma.Clear();
	maa.Clear();
	row.Clear();
	where.Clear();
	first_obj.Clear();
	second_obj.Clear();
	third_object.Clear();
	fourth_object.Clear();
	i_array.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
	port1.Clear();
	port2.Clear();
	root.Add("params", params);

	root.Add("id", id);

	Jzon::Writer writer1(root, Jzon::StandardFormat);
	writer1.Write();
	std::string result1 = writer1.GetResult();
		
	len = result1.size();
		
	json = result1.c_str();
		
	ptr = result1.c_str();
		
	for (nleft=len; nleft > 0; )
	{
		nwritten = send(s, ptr, nleft, 0);
		if (nwritten >0)
		{
			nleft -= nwritten;
			ptr += nwritten;   
		}
	}
			
	read(s, read_buf, 4096);
		
	string myString1(read_buf);
		
	ofstream myfile;
	myfile.open ("file1.json");
	myfile << read_buf;
	myfile.close();
		
	Jzon::Object rootNode;  	
	Jzon::FileReader::ReadFile("file1.json", rootNode);

	for (Jzon::Object::iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		//Search the first object related by updated Bridge
		if(flag){
			std::string name = (*it).first;
			Jzon::Node &node = (*it).second;
			if (node.IsArray())
				{
				 	const Jzon::Array &result = node.AsArray();
					for(unsigned i=0;i<result.GetCount();i++)
					{
						Jzon::Node &node1 = result.Get(i);

						Jzon::Object uuidNode = node1.AsObject();
							 		
						if (node1.IsObject())
						{

							 for (Jzon::Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
							 {
								std::string name1 = (*it1).first;
								Jzon::Node &node1 = (*it1).second;
							
								if(node1.IsArray()){
									const Jzon::Array &stuff1 = node1.AsArray();
										
								 	Jzon::Node &node2 = stuff1.Get(1);
								 	//save the second element, the new port created
						 			if(i==1){
						 				//insert in a port_uuid the list of port-uuid for this switch
						 				port_uuid[dnumber].push_back(node2.ToString());
						 			}
								}
							}
						}		
					}
				}
			}
			flag = !flag;
	}
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
	root.Clear();
	params.Clear();
			
	//Increment transaction id
	id++;

	//disconnect socket
    cmd_disconnect(s);
}

void commands::cmd_editconfig_lsi_delete(uint64_t dpi, int s){
	
	int len = 0;
    size_t  nleft = 0;
    ssize_t nwritten = 0;
	
	ssize_t r = 0;
	
	const char *ptr = "", *json = "";
	char read_buf[4096] = "";
	
	locale loc;	
	
	//bool flag = false;
	
	map<string, unsigned int> ports;
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	/*for all virtual link ports, destroy it*/
	for(list<uint64_t>::iterator i = virtual_link_id[switch_id[dpi]].begin(); i != virtual_link_id[switch_id[dpi]].end(); i++){
		cmd_delete_virtual_link(vl_id[(*i)], (*i), s);
	}

	port_l.erase(dpi);
	vport_l.erase(dpi);
	virtual_link_id[switch_id[dpi]].clear();

	//connect socket
    s = cmd_connect();

	root.Add("method", "transact");

	params.Add("Open_vSwitch");
				
	first_obj.Add("op", "update");
	first_obj.Add("table", "Open_vSwitch");
			   
	first_obj.Add("where", where);
	
	switch_uuid.erase(dpi);
	
	for(map<uint64_t, string>::iterator sww = switch_uuid.begin(); sww != switch_uuid.end(); sww++)
	{
		port2.Add("uuid");
			
		port2.Add(sww->second);
	
		port1.Add(port2);
	
		port2.Clear();
	}
		
	/*Array with two elements*/
	i_array.Add("set");
		   
	i_array.Add(port1);
		
	row.Add("bridges", i_array);
		
	first_obj.Add("row", row);
		
	params.Add(first_obj);
		
	second_obj.Clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj.Add("op", "mutate");
	second_obj.Add("table", "Open_vSwitch");
		
	/*Empty array [where]*/
	second_obj.Add("where", where);
			
	/*Array with two element*/
	maa.Add("next_cfg");
	maa.Add("+=");
	maa.Add(1);
			
	ma.Add(maa);
		
	second_obj.Add("mutations", ma);
			
	params.Add(second_obj);
	
	ma.Clear();
	maa.Clear();
	row.Clear();
	where.Clear();
	first_obj.Clear();
	second_obj.Clear();
	third_object.Clear();
	fourth_object.Clear();
	i_array.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
	port1.Clear();
	port2.Clear();
	root.Add("params", params);

	root.Add("id", id);

	Jzon::Writer writer1(root, Jzon::StandardFormat);
	writer1.Write();
	std::string result1 = writer1.GetResult();
		
	len = result1.size();
		
	json = result1.c_str();
		
	ptr = result1.c_str();
	
	ofstream myfile;
	
	Jzon::Object rootNode;  	
	
	for (nleft=len; nleft > 0; )
	{
		nwritten = send(s, ptr, nleft, 0);
		if (nwritten >0)
		{
			nleft -= nwritten;
			ptr += nwritten;   
		}
	}
		
	strcpy(read_buf, "");
			
	r = read(s, read_buf, 4096);
	
	read_buf[r-1] = '\0';
		
	string myString1(read_buf);
		
	myfile.open ("file1.json");
	myfile << read_buf;
	myfile.close();
	
	Jzon::FileReader::ReadFile("file1.json", rootNode);
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);

	root.Clear();
	params.Clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}

AddNFportsOut *commands::cmd_editconfig_NFPorts(AddNFportsIn anpi, int s){

	AddNFportsOut *anf = NULL;
	
	char temp[64] = "", tmp[64] = "";
	
	int len;
    size_t  nleft;
    ssize_t nwritten;
	
	const char *ptr, *json;
	char read_buf[4096];
	
	locale loc;	
	
	bool flag = false;
	
	map<string, unsigned int> ports;

	list<string> nfp = anpi.getNetworkFunctionsPorts();
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	//connect socket
    s = cmd_connect();

	if(nfp.size() != 0){
		
		root.Add("method", "transact");

		params.Add("Open_vSwitch");
		
		/*for each network function name in the list of nfs*/
		for(list<string>::iterator nf = nfp.begin(); nf != nfp.end(); nf++)
		{
			char ifac[64] = "iface";
				
			//Create the current name of a interface
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
			
			string str = (*nf);
				
			//Create port name to lower case
			for (string::size_type i=0; i<str.length(); ++i)
    			str[i] = tolower(str[i], loc);
    				
    		//Create the current port name
			strcpy(tmp, (char *)(str).c_str());
			sprintf(temp, "%" PRIu64, anpi.getDpid());
			strcat(tmp, "b");
			strcat(tmp, temp);
			strcpy(temp, tmp);
				
			for(unsigned int j=0;j<strlen(temp);j++){
				if(temp[j] == '_')
					temp[j] = 'p';
			}
				
			sprintf(temp, "%d", rnumber);
			strcat(ifac, temp);
		
			first_obj.Add("op", "insert");
			first_obj.Add("table", "Interface");
		
			row.Add("name", temp);
			row.Add("type", "internal");
		
			first_obj.Add("row", row);
		
			first_obj.Add("uuid-name", ifac);
		
			params.Add(first_obj);
		
			row.Clear();
			first_obj.Clear();
				
			first_obj.Add("op", "insert");
			first_obj.Add("table", "Port");
		
			row.Add("name", temp);
		
			iface.Add("set");
	
			iface2.Add("named-uuid");
			iface2.Add(ifac);
	
			iface1.Add(iface2);
			iface.Add(iface1);
		
			row.Add("interfaces", iface);
		
			first_obj.Add("row", row);
		
			first_obj.Add("uuid-name", temp);
		
			params.Add(first_obj);
		
			row.Clear();
			first_obj.Clear();
			iface.Clear();
			iface1.Clear();
			iface2.Clear();
				
			//insert this port name into port_n
			port_l[anpi.getDpid()].push_back(temp);
				
			ports[(*nf)] = rnumber;	
				
			rnumber++;
		}
		
		first_obj.Add("op", "update");
		first_obj.Add("table", "Bridge");
			   
		third_object.Add("_uuid");
		third_object.Add("==");
		
		fourth_object.Add("uuid");
		fourth_object.Add(switch_uuid[anpi.getDpid()].c_str());
		
		third_object.Add(fourth_object);
		where.Add(third_object);
		
		first_obj.Add("where", where);
	
		where.Clear();
	
		for(list<string>::iterator u = port_uuid[anpi.getDpid()].begin(); u != port_uuid[anpi.getDpid()].end(); u++)
		{
			port2.Add("uuid");
				
			port2.Add((*u));
	
			port1.Add(port2);
	
			port2.Clear();
		}
		
		for(list<string>::iterator u = vport_uuid[anpi.getDpid()].begin(); u != vport_uuid[anpi.getDpid()].end(); u++)
		{
			port2.Add("uuid");
			
			port2.Add((*u));
	
			port1.Add(port2);
	
			port2.Clear();
		}
	
		for(list<string>::iterator nff = nfp.begin(); nff != nfp.end(); nff++)
		{
			string str = (*nff);
				
			//Create port name to lower case
			for (string::size_type i=0; i<str.length(); ++i)
    			str[i] = tolower(str[i], loc);
    				
			strcpy(tmp, (char *)(str).c_str());
			sprintf(temp, "%" PRIu64, anpi.getDpid());
			strcat(tmp, "b");
			strcat(tmp, temp);
			strcpy(temp, tmp);
				
			for(unsigned int j=0;j<strlen(temp);j++){
				if(temp[j] == '_')
					temp[j] = 'p';
			}
		
			port2.Add("named-uuid");
			port2.Add(temp);
	
			port1.Add(port2);
	
			port2.Clear();
		}
		
		/*Array with two elements*/
		i_array.Add("set");
		   
		i_array.Add(port1);
		
		row.Add("ports", i_array);
		
		first_obj.Add("row", row);
		
		params.Add(first_obj);
		
		second_obj.Clear();
		
		/*Object with four items [op, table, where, mutations]*/
		second_obj.Add("op", "mutate");
		second_obj.Add("table", "Open_vSwitch");
		
		/*Empty array [where]*/
		second_obj.Add("where", where);
			
		/*Array with two element*/
		maa.Add("next_cfg");
		maa.Add("+=");
		maa.Add(1);
			
		ma.Add(maa);
		
		second_obj.Add("mutations", ma);
			
		params.Add(second_obj);
		
		ma.Clear();
		maa.Clear();
		row.Clear();
		where.Clear();
		first_obj.Clear();
		second_obj.Clear();
		third_object.Clear();
		fourth_object.Clear();
		i_array.Clear();
		iface.Clear();
		iface1.Clear();
		iface2.Clear();
		port1.Clear();
		port2.Clear();
		root.Add("params", params);

		root.Add("id", id);

		Jzon::Writer writer1(root, Jzon::StandardFormat);
		writer1.Write();
		std::string result1 = writer1.GetResult();
		
		len = result1.size();
		
		json = result1.c_str();
		
		ptr = result1.c_str();
		
		for (nleft=len; nleft > 0; )
		{
			nwritten = send(s, ptr, nleft, 0);
			if (nwritten >0)
			{
				nleft -= nwritten;
				ptr += nwritten;   
			}
		}
			
		read(s, read_buf, 4096);
		
		string myString1(read_buf);
		
		ofstream myfile;
		myfile.open ("file1.json");
		myfile << read_buf;
		myfile.close();
		
		Jzon::Object rootNode;  	
		Jzon::FileReader::ReadFile("file1.json", rootNode);

		for (Jzon::Object::iterator it = rootNode.begin(); it != rootNode.end(); ++it)
		{
			//Search the first object related by updated Bridge
			if(flag){
				std::string name = (*it).first;
				Jzon::Node &node = (*it).second;
					if (node.IsArray())
					{
					 	const Jzon::Array &result = node.AsArray();
						for(unsigned i=0;i<result.GetCount();i++){

							Jzon::Node &node1 = result.Get(i);

							Jzon::Object uuidNode = node1.AsObject();
							 		
							if (node1.IsObject()){

							 	for (Jzon::Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
								{
									std::string name1 = (*it1).first;
									Jzon::Node &node1 = (*it1).second;
							
									if(node1.IsArray()){
										const Jzon::Array &stuff1 = node1.AsArray();
										
								 		Jzon::Node &node2 = stuff1.Get(1);
								 		//save the second element, the new port created
						 				if(i==1){
						 					//insert in a port_uuid the list of port-uuid for this switch
						 					port_uuid[anpi.getDpid()].push_back(node2.ToString());
						 				}
									}
								}
							 }		
						 }
					}
			}
			flag = !flag;
		}
			
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
		root.Clear();
		params.Clear();
			
		//Increment transaction id
		id++;
	}

	//disconnect socket
    cmd_disconnect(s);

    anf = new AddNFportsOut(anpi.getNFname(), ports);

	return anf;
}

void commands::cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s){
	
	int len;
    size_t  nleft;
    ssize_t nwritten;
	
	const char *ptr, *json;
	char read_buf[4096];
	
	locale loc;	
	
	map<string, unsigned int> ports;

	set<string> nfp = dnpi.getNFports();
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	//connect socket
    s = cmd_connect();

	if(nfp.size() != 0){
		
		root.Add("method", "transact");

		params.Add("Open_vSwitch");
		
		first_obj.Add("op", "update");
		first_obj.Add("table", "Bridge");
			   
		third_object.Add("_uuid");
		third_object.Add("==");
		
		fourth_object.Add("uuid");
		fourth_object.Add(switch_uuid[dnpi.getDpid()].c_str());
		
		third_object.Add(fourth_object);
		where.Add(third_object);
		
		first_obj.Add("where", where);
	
		where.Clear();
		
		list<string>::iterator uu = port_uuid[dnpi.getDpid()].begin();
		
		/*for each port in the list of the getNetworkFunctionsPorts*/
		for(set<string>::iterator p = nfp.begin(); p != nfp.end(); p++){
			string sss = (*p);
			uu = port_uuid[dnpi.getDpid()].begin();
			//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
			for(list<string>::iterator u = port_l[dnpi.getDpid()].begin(); u != port_l[dnpi.getDpid()].end(); u++){
				string s = (*u);
				if(s.compare(sss) == 0){
					port_uuid[dnpi.getDpid()].remove((*uu));
					break;	
				}
				uu++;
			}
		}
	
		for(list<string>::iterator u = port_uuid[dnpi.getDpid()].begin(); u != port_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.Add("uuid");
				
			port2.Add((*u));
	
			port1.Add(port2);
	
			port2.Clear();
		}
		
		for(list<string>::iterator u = vport_uuid[dnpi.getDpid()].begin(); u != vport_uuid[dnpi.getDpid()].end(); u++)
		{
			port2.Add("uuid");
			
			port2.Add((*u));
	
			port1.Add(port2);
	
			port2.Clear();
		}
		
		/*Array with two elements*/
		i_array.Add("set");
		   
		i_array.Add(port1);
		
		row.Add("ports", i_array);
		
		first_obj.Add("row", row);
		
		params.Add(first_obj);
		
		second_obj.Clear();
		
		/*Object with four items [op, table, where, mutations]*/
		second_obj.Add("op", "mutate");
		second_obj.Add("table", "Open_vSwitch");
		
		/*Empty array [where]*/
		second_obj.Add("where", where);
			
		/*Array with two element*/
		maa.Add("next_cfg");
		maa.Add("+=");
		maa.Add(1);
			
		ma.Add(maa);
		
		second_obj.Add("mutations", ma);
			
		params.Add(second_obj);
		
		ma.Clear();
		maa.Clear();
		row.Clear();
		where.Clear();
		first_obj.Clear();
		second_obj.Clear();
		third_object.Clear();
		fourth_object.Clear();
		i_array.Clear();
		iface.Clear();
		iface1.Clear();
		iface2.Clear();
		port1.Clear();
		port2.Clear();
		root.Add("params", params);

		root.Add("id", id);

		Jzon::Writer writer1(root, Jzon::StandardFormat);
		writer1.Write();
		std::string result1 = writer1.GetResult();
		
		len = result1.size();
		
		json = result1.c_str();
		
		ptr = result1.c_str();
		
		for (nleft=len; nleft > 0; )
		{
			nwritten = send(s, ptr, nleft, 0);
			if (nwritten >0)
			{
				nleft -= nwritten;
				ptr += nwritten;   
			}
		}
			
		read(s, read_buf, 4096);
		
		string myString1(read_buf);
		
		ofstream myfile;
		myfile.open ("file1.json");
		myfile << read_buf;
		myfile.close();
			
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
		root.Clear();
		params.Clear();
			
		//Increment transaction id
		id++;
		
		//disconnect socket
    	cmd_disconnect(s);
	}
}

AddVirtualLinkOut *commands::cmd_addVirtualLink(AddVirtualLinkIn avli, int s){
	/*struct to return*/
	AddVirtualLinkOut *avlo = NULL;
	
	uint64_t port_id_1 = 0, port_id_2 = 0;
	char temp[64] = "", vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;
	
	strcpy(vrt, "vport");
	char ifac[64] = "iface";
	    	
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(vrt, temp);
			
	port_id_1 = pnumber;
		
	pnumber++;
			
	port_id_2 = pnumber;
			
	/*create virtual link*/
	sprintf(temp, "%d", pnumber);
	strcat(trv, temp);
		
	//Create the current name of a interface
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);

	rnumber++;

	//create first endpoint
	cmd_add_virtual_link(vrt, trv, ifac, avli.getDpidA(), s);
	
	//Create the current name of a interface
	strcpy(ifac, "iface");
	sprintf(temp, "%d", rnumber);
	strcat(ifac, temp);
	
	//create second endpoint
	cmd_add_virtual_link(trv, vrt, ifac, avli.getDpidB(), s);

	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidA()]].push_back(port_id_2);
	
	/*store the information [bridge name, list of peer port]*/
	virtual_link_id[switch_id[avli.getDpidB()]].push_back(port_id_1);

	port_id[port_id_1] = vrt;
	port_id[port_id_2] = trv;

	/*store the information [switch_id, port_id]*/
	vl_id[port_id_1] = avli.getDpidA();
	vl_id[port_id_2] = avli.getDpidB();
	
	//insert this port name into port_n
	vport_l[avli.getDpidA()].push_back(vrt);
	//insert this port name into port_n
	vport_l[avli.getDpidB()].push_back(trv);
	
	/*peer name of the bridge name vrt is trv*/
	peer_n[trv] = vrt;
	
	/*peer name of the bridge name trv is vrt*/
	peer_n[vrt] = trv;
				
	/*store the information of virtual link*/
	virtual_links.push_back(make_pair(port_id_1, port_id_2));
	
	/*store the information of virtual link*/
	virtual_links.push_back(make_pair(port_id_2, port_id_1));

	avlo = new AddVirtualLinkOut(port_uuid[avli.getDpidA()].size() + vport_uuid[avli.getDpidA()].size(), port_uuid[avli.getDpidB()].size() + vport_uuid[avli.getDpidB()].size());

	return avlo;
}

void commands::cmd_add_virtual_link(string vrt, string trv, char ifac[64], uint64_t dpi, int s){
	
	int len;
    size_t  nleft;
    ssize_t nwritten;
	
	const char *ptr, *json;
	char read_buf[4096];
	
	locale loc;	
	
	bool flag = false;
	
	map<string, unsigned int> ports;
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	//connect socket
    s = cmd_connect();

	root.Add("method", "transact");

	params.Add("Open_vSwitch");
			
	first_obj.Add("op", "insert");
	first_obj.Add("table", "Interface");
		
	/*Insert an Interface*/
	row.Add("name", vrt);
	row.Add("type", "patch");
		
	/*Add options peer*/
	peer.Add("map");
	
	peer2.Add("peer");
	peer2.Add(trv);
			
	peer1.Add(peer2);
	peer.Add(peer1);
    
    row.Add("options", peer);
		
	first_obj.Add("row", row);
		
	first_obj.Add("uuid-name", ifac);
		
	params.Add(first_obj);
		
	row.Clear();
	first_obj.Clear();
				
	first_obj.Add("op", "insert");
	first_obj.Add("table", "Port");
		
	/*Insert a port*/
	row.Add("name", vrt);
		
	iface.Add("set");
	
	iface2.Add("named-uuid");
	iface2.Add(ifac);
	
	iface1.Add(iface2);
	iface.Add(iface1);
		
	row.Add("interfaces", iface);
		
	first_obj.Add("row", row);
		
	first_obj.Add("uuid-name", vrt);
		
	params.Add(first_obj);
		
	row.Clear();
	first_obj.Clear();
	peer.Clear();
	peer1.Clear();
	peer2.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
		
	first_obj.Add("op", "update");
	first_obj.Add("table", "Bridge");
			   
	third_object.Add("_uuid");
	third_object.Add("==");
		
	fourth_object.Add("uuid");
	fourth_object.Add(switch_uuid[dpi].c_str());
		
	third_object.Add(fourth_object);
	where.Add(third_object);
		
	first_obj.Add("where", where);
	
	where.Clear();
	
	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
	
	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
	
	port2.Add("named-uuid");
	port2.Add(vrt);
	
	port1.Add(port2);
	
	port2.Clear();
		
	/*Array with two elements*/
	i_array.Add("set");
		   
	i_array.Add(port1);
		
	row.Add("ports", i_array);
		
	first_obj.Add("row", row);
		
	params.Add(first_obj);
		
	second_obj.Clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj.Add("op", "mutate");
	second_obj.Add("table", "Open_vSwitch");
		
	/*Empty array [where]*/
	second_obj.Add("where", where);
			
	/*Array with two element*/
	maa.Add("next_cfg");
	maa.Add("+=");
	maa.Add(1);
			
	ma.Add(maa);
		
	second_obj.Add("mutations", ma);
			
	params.Add(second_obj);
	
	ma.Clear();
	maa.Clear();
	row.Clear();
	where.Clear();
	first_obj.Clear();
	second_obj.Clear();
	third_object.Clear();
	fourth_object.Clear();
	i_array.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
	port1.Clear();
	port2.Clear();
	root.Add("params", params);

	root.Add("id", id);

	Jzon::Writer writer1(root, Jzon::StandardFormat);
	writer1.Write();
	std::string result1 = writer1.GetResult();
		
	len = result1.size();
		
	json = result1.c_str();
		
	ptr = result1.c_str();
	
	for (nleft=len; nleft > 0; )
	{
		nwritten = send(s, ptr, nleft, 0);
		if (nwritten >0)
		{
			nleft -= nwritten;
			ptr += nwritten;   
		}
	}
			
	read(s, read_buf, 4096);
		
	string myString1(read_buf);
		
	ofstream myfile;
	myfile.open ("file1.json");
	myfile << read_buf;
	myfile.close();
	
	Jzon::Object rootNode;  	
	Jzon::FileReader::ReadFile("file1.json", rootNode);

	for (Jzon::Object::iterator it = rootNode.begin(); it != rootNode.end(); ++it)
	{
		//Search the first object related by updated Bridge
		if(flag){
			std::string name = (*it).first;
			Jzon::Node &node = (*it).second;
			if (node.IsArray())
			{
			 	const Jzon::Array &result = node.AsArray();
				for(unsigned i=0;i<result.GetCount();i++){

					Jzon::Node &node1 = result.Get(i);

					Jzon::Object uuidNode = node1.AsObject();
							 		
	    			if (node1.IsObject()){

					 	for (Jzon::Object::iterator it1 = uuidNode.begin(); it1 != uuidNode.end(); ++it1)
						{
							std::string name1 = (*it1).first;
							Jzon::Node &node1 = (*it1).second;
							
		     				if(node1.IsArray()){
								const Jzon::Array &stuff1 = node1.AsArray();
										
						 		Jzon::Node &node2 = stuff1.Get(1);
						 		//save the second element, the new port created
				 				if(i==1){
				 					//insert in a port_uuid the list of vport-uuid for this switch
				 					vport_uuid[dpi].push_back(node2.ToString());
				 				}
							}
						}
					 }		
				 }
			}
		}
		
		flag = !flag;
	}
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
	root.Clear();
	params.Clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}

void commands::cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s){
	
	uint64_t port_id_1 = 0, port_id_2 = 0;
	char vrt[64] = "", trv[64] = "";
	
	list<pair<unsigned int, unsigned int> > virtual_links;

	//destroy first endpoint
	cmd_delete_virtual_link(dvli.getDpidA(), dvli.getIdA(), s);
	
	//destroy second endpoint
	cmd_delete_virtual_link(dvli.getDpidB(), dvli.getIdB(), s);

	/*store the information [switch_id, port_id]*/
	vl_id.erase(port_id_1);
	vl_id.erase(port_id_2);
	
	//remove this port name into port_n
	vport_l[dvli.getDpidA()].remove(vrt);
	//insert this port name into port_n
	vport_l[dvli.getDpidB()].remove(trv);
				
	/*store the information of virtual link*/
	virtual_links.remove(make_pair(dvli.getDpidA(), dvli.getDpidB()));
	
	/*store the information of virtual link*/
	virtual_links.remove(make_pair(dvli.getDpidB(), dvli.getDpidA()));
}

void commands::cmd_delete_virtual_link(uint64_t dpi, uint64_t idp, int s){
	
	int len;
    size_t  nleft;
    ssize_t nwritten;
	
	const char *ptr, *json;
	char read_buf[4096];
	
	locale loc;	
	
	//bool flag = false;
	
	map<string, unsigned int> ports;
	
	Jzon::Object root, first_obj, second_obj, row;
	Jzon::Array params, iface, iface1, iface2, where, port1, port2, i_array, peer, peer1, peer2;

	Jzon::Array ma;
	Jzon::Array maa;
	
	Jzon::Array third_object;
	Jzon::Array fourth_object;

	//connect socket
    s = cmd_connect();

	root.Add("method", "transact");

	params.Add("Open_vSwitch");
				
	first_obj.Add("op", "update");
	first_obj.Add("table", "Bridge");
			   
	third_object.Add("_uuid");
	third_object.Add("==");
		
	fourth_object.Add("uuid");
	fourth_object.Add(switch_uuid[dpi].c_str());
		
	third_object.Add(fourth_object);
	where.Add(third_object);
		
	first_obj.Add("where", where);
	
	where.Clear();
	
	//search the port-id and after erase this port-uuid
	list<string>::iterator uu = vport_uuid[dpi].begin();
	
	//should be search in port_l, p....if find it take the index and remove it from the set port-uuid[pi]
	for(list<string>::iterator u = vport_l[dpi].begin(); u != vport_l[dpi].end(); u++){
		string sss = (*u);
		if(port_id[idp].compare(sss) == 0){
			vport_uuid[dpi].remove(*uu);
			vport_l[dpi].remove(*u);
			break;	
		}
		uu++;
	}
	
	//insert normal ports
	for(list<string>::iterator u = port_uuid[dpi].begin(); u != port_uuid[dpi].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
	
	//insert vports
	for(list<string>::iterator u = vport_uuid[dpi].begin(); u != vport_uuid[dpi].end(); u++)
	{
		port2.Add("uuid");
			
		port2.Add((*u));
	
		port1.Add(port2);
	
		port2.Clear();
	}
		
	/*Array with two elements*/
	i_array.Add("set");
		   
	i_array.Add(port1);
		
	row.Add("ports", i_array);
		
	first_obj.Add("row", row);
		
	params.Add(first_obj);
		
	second_obj.Clear();
		
	/*Object with four items [op, table, where, mutations]*/
	second_obj.Add("op", "mutate");
	second_obj.Add("table", "Open_vSwitch");
		
	/*Empty array [where]*/
	second_obj.Add("where", where);
			
	/*Array with two element*/
	maa.Add("next_cfg");
	maa.Add("+=");
	maa.Add(1);
			
	ma.Add(maa);
		
	second_obj.Add("mutations", ma);
			
	params.Add(second_obj);
	
	ma.Clear();
	maa.Clear();
	row.Clear();
	where.Clear();
	first_obj.Clear();
	second_obj.Clear();
	third_object.Clear();
	fourth_object.Clear();
	i_array.Clear();
	iface.Clear();
	iface1.Clear();
	iface2.Clear();
	port1.Clear();
	port2.Clear();
	root.Add("params", params);

	root.Add("id", id);

	Jzon::Writer writer1(root, Jzon::StandardFormat);
	writer1.Write();
	std::string result1 = writer1.GetResult();
		
	len = result1.size();
		
	json = result1.c_str();
		
	ptr = result1.c_str();
	
	for (nleft=len; nleft > 0; )
	{
		nwritten = send(s, ptr, nleft, 0);
		if (nwritten >0)
		{
			nleft -= nwritten;
			ptr += nwritten;   
		}
	}
			
	read(s, read_buf, 4096);
		
	string myString1(read_buf);
		
	ofstream myfile;
	myfile.open ("file1.json");
	myfile << read_buf;
	myfile.close();
	
	Jzon::Object rootNode;  	
	Jzon::FileReader::ReadFile("file1.json", rootNode);
			
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Result of query: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, json);
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Response json: ");
		
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, read_buf);
			
	root.Clear();
	params.Clear();
			
	//Increment transaction id
	id++;
	
	//disconnect socket
    cmd_disconnect(s);
}
