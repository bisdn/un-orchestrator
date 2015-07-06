import nffglib

def handle_request(method, url, content = None):
	print "Method: " + method
	print "Url: " + url
	
	if not content:
		print "This message has empty body!"
	else:
		print "Content: " + content
		
	
	return "AAAAAAA"

