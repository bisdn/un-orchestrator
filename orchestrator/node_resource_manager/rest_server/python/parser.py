#!/usr/bin/env python
__author__ = 'Ivano Cerrato'

#NF-FG library
import nffglib
#Constants used by the parser
import constants

import os


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
			
			LOG.debug("Returning: %s",answer)
			
			return answer
	elif method == 'POST':
		LOG.debug("POST!") 
		if url == '/ping':
			return 'OK'
		elif url == '/get-config':
			#answer = get_config()
			return get_config()
#			return "BBBBBBBBBB"
		elif url == '/edit-config':
			#TODO
			return
		else:
			#TODO: this should not happen
			return
	else:
		#TODO: this should not happen
		return


def get_config():
	'''
	Return the current configuration of the node
	'''
	
	print os.path.dirname(__file__)
	
	try:
		LOG.debug("Reading file: %s",constants.TMP_FILE)
		config = nffglib.Virtualizer.parse(file=constants.TMP_FILE)
	except IOError as e:
		#TODO
		print "I/O error({0}): {1}".format(e.errno, e.strerror)
		return "AAAA"
	
	config_xml = config.xml()
	LOG.debug("File correctly read!")
	LOG.debug("%s",config_xml)
	return config_xml
	
