import struct

def pack(fmt, *args):
	# safe (type-casting) wrapper for struct.pack
	#print 'converting %s for "%s"' % (args, fmt)
	processed_args = []
	for spec in fmt:
		if spec in ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'x']:
			continue
		a = args[len(processed_args)]
		# simple way of supporting Widget objects
		if hasattr(a, 'handle'):
			a = a.handle
		if spec == 'L':
			conv = long
		elif spec == 's':
			conv = str
		else:
			conv = int
		#print 'converting %s to %s for "%s"' % (`a`, conv.__name__, spec)
		processed_args.append(conv(a))
	return struct.pack('!' + fmt, *processed_args)

def request(reqtype, data='', id=None):
	data = str(data)
	if id is None:
		id = reqtype
	return pack('LLHxx', id, len(data), reqtype) + data

# specific requests

def attachwidget(parent_id, child_id, relationship, id=None):
	return request(47, pack('LLHxx', parent_id, child_id, relationship), id=id)

def batch(req_list, id=None):
	# example code:
	# l = []
	# l.append(requests.somerequest())
	# l.append(requests.somerequest())
	# l.append(requests.somerequest())
	# connection.send(requests.batch(l))
	#
	# well, in fact you can pass a string too
	return request(18, ''.join(req_list), id=id)

def chcontext(obj_id, delta, id=None):
	return request(30, pack('LHxx', obj_id, delta), id=id)
	
def checkevent(id=None):
	return request(43, id=id)

def createwidget(wtype, id=None):
	return request(46, pack('Hxx', wtype), id=id)

def drivermsg(message, param, id=None):
	return request(39, pack('LL', message, param), id=id)
	
def dup(obj_id, id=None):
	return request(27, pack('L', obj_id), id=id)
	
def findwidget(name, id=None):
	return request(42, name + '\x00', id=id)
	
def focus(obj_id, id=None):
	return request (25, pack('L', obj_id), id=id)

def free(obj_id, id=None):
	return request (6, pack('L', obj_id), id=id)
	
def get(widget_id, property, id=None):
	return request(8, pack('LHxx', widget_id, property), id=id)

def getfstyle(index, id=None):
	return request(41, pack('Hxx', index), id=id)
	
def getinactive (id=None):
	return request(37, id=id)
	
def getmode(id=None):
	return request(22, id=id)
	
def getpayload(obj_id, id=None):
	return request (29, pack('L', obj_id), id=id)
	
def getstring(obj_id, id=None):
	return request(26, pack('L', obj_id), id=id)
	
def inputkey(type, key, mods, id=None):
	return request(10, pack('LHH', type, keys,mods), id=id)
	
def inputpoint(type, x, y, btn, id=None):
	return request(11,  pack('LHHHxx', type, x,  y, btn), id=id)
	
def loaddriver(name,  id=None):
	return request(40, name + '\x00', id=id)

def mkarray(data,  id=None):
	return request(33, data + '\x00', id=id)
	
def mkbitmap(image, id=None):
	return request(3, image, id=id)
	
def mkcontext(id=None):
	return request(23, id=id)
	
def mkfillstyle(id=None):
	return request(14, id=id)

def mkfont(name, style, size, id=None):
	return request(4, pack('40sLHxx', name, style, size), id=id)

def mkpopup(x, y, width, height, id=None):
	return request(16, pack('HHHH', x, y, width, height), id=id)

def mkstring(s, id=None):
	return request(5, s + '\x00', id=id)

def mktheme(data, id=None):
	return request(9, data + '\x00', id=id)
	
def mkwidget(relationship, type, parent, id=None):
	return request(2, pack('HHL', relationship,  type, parent), id=id)
	
def newbitmap(width, height, id=None):
	return request(35, pack('HH', width, height), id=id)
	
def ping(id=None):
	return request(0, id=id)
	
def own(res, id=None):
	return request(19, pack('H', res), id=id)
	
def render(dest, groptype, id=None):
	return request(34, pack('LL', dest, groptype), id=id)
	
def rmcontext(id=None):
	return request(24, id=id)

def set(widget_id, property, value, id=None):
	return request(7, pack('LLHxx', widget_id, value, property), id=id)

def setinactive(time, id=None):
	return request(38, pack('L', time), id=id)
	
def setmode(xres, yres, bpp, flagmode, flags, id=None):
	return request(21, pack('HHHHL', xres, yres, bpp, flagmode, flags), id=id)
	
def setpayload(h, payload, id=None):
	return request(28, pack('LL', h, payload), id=id)
	
def sizebitmap(widget_id, property, value, id=None):
	return request(44, pack('LLHxx', widget_id, value, property), id=id)

def sizetext(text, font, id=None):
	return request(17, pack('LL', text, font), id=id)
	
def thlookup(object, property, id=None):
	return request(36, pack('HH', object, property), id=id)
	
def traversewidget(widget, direction, count, id=None):
	return request(49, pack('LHH', widget, direction, count), id=id)
	
def disown(res, id=None):
	return request(20, pack('H', res), id=id)
	
def updatepart(widget_id, id=None):
	return request(32, pack('L', widget_id), id=id)

def update(id=None):
	return request(1, id=id)
		
def register(name_id, apptype=1, id=None):
	# the id of the string for the app name, and 1 for PG_APP_NORMAL
	# (could be 2 for PG_APP_TOOLBAR)
	return request(15, pack('LHxx', name_id, apptype), id=id)

def wait(id=None):
	return request(13, id=id)

def writeto(widget_id, property, value, id=None):
	return request(8, pack('LLHxx', widget_id, value, property), id=id)
