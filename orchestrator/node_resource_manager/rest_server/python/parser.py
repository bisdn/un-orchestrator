#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#NF-FG library
import nffglib
#Constants used by the parser
import constants

import logging

#Set the logger
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.DEBUG)	#Change here the logging level
LOG.propagate = False
sh = logging.StreamHandler()
sh.setLevel(logging.DEBUG)
f = logging.Formatter('[%(asctime)s] [Local-Orchestrator] [%(levelname)s] %(message)s')
sh.setFormatter(f)
LOG.addHandler(sh)

def handle_request(method, url, content = None):
	'''
	Handles HTTP commands coming from the network
	'''

	LOG.debug("Method: %s",method)
	LOG.debug("Url: %s",url)
	
	if not content:
		LOG.debug("This message has empty body!")
	else:
		LOG.debug("Content: %s",content)

	status = 1

	if method == 'GET':
		if url == '/ping':
			return 'OK'
		else:
			a = 'usage:\n'
			b = 'get http://hostip:tcpport - this help message\n'
			c = 'get http://hostip:tcpport/ping - test webserver aliveness\n'
			d = 'post http://hostip:tcpport/get-config - query NF-FG\n'
			e = 'post http://hostip:tcpport/edit-config - send NF-FG request in the post body'
			
			answer = a + b + c + d + e
			
			LOG.debug("Returning: ")
			LOG.debug("%s",answer)
			
			return answer
			
	elif method == 'POST':
		if url == '/ping':
			return 'OK'
			
		elif url == '/get-config':
			return get_config(constants.TMP_FILE)
			
		elif url == '/edit-config':
			#TODO
			return
			
		else:
			#TODO: this should not happen
			return
	else:
		#TODO: this should not happen
		return

def get_config(fileName):
	'''
	Return the current configuration of the node
	'''
	
	try:
		LOG.debug("Reading file: %s",fileName)
		config = nffglib.Virtualizer.parse(file=fileName)
	except:
	#(IOError,InvalidXML) as e:
		#TODO
#		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		print "EXCEPTIONNNNNNNNNNNNNNNNNNNNNNNNNNNNNNn"
		return "AAAA"
		
	LOG.debug("Parsing the file content")
	try:
		config_xml = config.xml()
	except:
		print "----"
	LOG.debug("File correctly managed!")
	LOG.debug("%s",config_xml)
	return config_xml

	
def init_orchestrator(fileName):
	'''
	Read the configuration file of the orchestrator and retrieves information
	related to the ports of the Universal Node
	
	In the file, there must be one and only one node in the infrastructure.
	I don't do any check on this statement.
	'''

	infrastructure_xml = get_config(fileName)
	
	LOG.debug('Parsing the physical infrastructure...')
	
	infrastructure = nffglib.Virtualizer.parse(text=infrastructure_xml)
	
	universal_node = infrastructure.c_nodes.list_node[0]
	
	LOG.debug("Infrastructure node: ")
	LOG.debug("\t %s",universal_node.g_node.g_idNameType.g_idName.l_id)
	LOG.debug("\t %s",universal_node.g_node.g_idNameType.g_idName.l_name)
	
	LOG.debug("Available ports: ")
	toBeReturned = []
	ports = universal_node.g_node.c_ports.list_port;
	for port in ports:
		LOG.debug("\t %s - %s",port.g_idName.l_id,port.g_idName.l_name)
		toBeReturned.append(port.g_idName.l_id)
		toBeReturned.append(port.g_idName.l_name)
		
	''' 
		Save the configuration file in the tmp file that will be used for the rest of the
		execution to maintain the current configuration of the node
	'''
		
	tmpFile = open(constants.TMP_FILE, "w")
	tmpFile.write(infrastructure_xml)
	tmpFile.close()
	
	return toBeReturned

################################

def main():
	'''
	Only used for debug purposes
	'''
	
	fileName = "/home/kvmuser/Desktop/un-orchestrator/orchestrator/config/infra_domain.xml"
	
	print "First execution"
	init_orchestrator(fileName)
	print "Done"
	
	print "Second execution"
	init_orchestrator(fileName)
	print "Done"
	
if __name__ == '__main__':
	'''
	Only used for debug purposes
	'''
	main()


