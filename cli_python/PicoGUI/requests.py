from struct import pack

def request(reqtype, data='', id=None):
	data = str(data)
	if id is None:
		id = reqtype
	return pack('!LLHxx', id, len(data), reqtype) + data

# specific requests

def attachwidget(parent_id, child_id, relationship, id=None):
	return request(47, pack('!LLHxx', parent_id, child_id, relationship), id=id)

def createwidget(wtype, id=None):
	return request(46, pack('!Hxx', wtype), id=id)

def mkfont(name, style, size, id=None):
	return request(4, pack('!40sLHxx', name, style, size), id=id)

def mkstring(s, id=None):
	s = s + '\x00'
	return request(5, s, id=id)

def set(widget_id, property, value, id=None):
	return request(7, pack('!LLHxx', widget_id, value, property), id=id)

def register(name_id, id=None):
	# the id of the string for the app name, and 1 for PG_APP_NORMAL
	# (could be 2 for PG_APP_TOOLBAR)
	return request(15, pack('!LHxx', name_id, 1), id=id)

def update(id=None):
	return request(1, id=id)

def wait(id=None):
	return request(13, id=id)
