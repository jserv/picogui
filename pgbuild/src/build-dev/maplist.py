#
#
# MapList - A container class that offers Dictonary, List and some Set behavior.
#
# author: 	thanos vassilakis
# 		thanos at 0x01.com
#
# date:		12 August 2001
#
#	(c) thanos vassilakis, 2001
#
# licence: GPL
#
# Liceince
#
#	funcionality:
#		Dictionary - A ListMap object has the full functionality of a Python dictionary, plus its class can be subclassed.
#		List - ListMap has most of the python list functionality. The exceptions are
#			__setitem__ is setByIndex
#			__getitem__ is getByIndex
#			__delietm__ is delByIndex
#
#		Set -  ListMap give you +,-,*, &, |, ^ (xor) operators.
#
#		Other extras:
#			keys, values, items - optionally take a list of keys.
#			sort, reverse
#			slicing, swap, filter, map
#			
#
#	Performance:
#		Obviouly MapList is normally slower than a python dictionary or list, UserDict and UserList, but in some cases can be a lot faster,
#		especially if you use items().
#
#	Timing Tests on a WIN NT Pentium II  130Meg
#
#for 5000 iterations
#doing:
#    for x in xrange(n):
#            o[str(x)] = x
#            tt = o[str(x)]
#            kk =o.keys()
#            vv = o.values()
#            ii = o.items()
#    
#Python Dictionary
#         3 function calls in 65.066 CPU seconds
#
#   Ordered by: standard name
#
#   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
#        1    0.007    0.007   65.065   65.065 <string>:1(?)
#        0    0.000             0.000          profile:0(profiler)
#        1    0.001    0.001   65.066   65.066 profile:0(test(o, n))
#        1   65.058   65.058   65.058   65.058 testod.py:7(test)
#
#
#UserDict
#         25003 function calls in 70.038 CPU seconds
#
#   Ordered by: standard name
#
#   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
#        1    0.007    0.007   70.037   70.037 <string>:1(?)
#     5000    0.215    0.000    0.215    0.000 UserDict.py:14(__getitem__)
#     5000    0.209    0.000    0.209    0.000 UserDict.py:15(__setitem__)
#     5000    3.888    0.001    3.888    0.001 UserDict.py:23(keys)
#     5000   48.845    0.010   48.845    0.010 UserDict.py:24(items)
#     5000    2.815    0.001    2.815    0.001 UserDict.py:25(values)
#        0    0.000             0.000          profile:0(profiler)
#        1    0.001    0.001   70.038   70.038 profile:0(test(o, n))
#        1   14.059   14.059   70.031   70.031 testod.py:7(test)
#
#
#Thanos' MapList
#         25003 function calls in 30.517 CPU seconds
#
#   Ordered by: standard name
#
#   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
#        1    0.000    0.000   30.516   30.516 <string>:1(?)
#     5000    0.350    0.000    0.350    0.000 od1.py:22(__setitem__)
#     5000    0.206    0.000    0.206    0.000 od1.py:30(__getitem__)
#     5000    0.187    0.000    0.187    0.000 od1.py:43(keys)
#     5000   27.109    0.005   27.109    0.005 od1.py:52(values)
#     5000    0.184    0.000    0.184    0.000 od1.py:63(items)
#        0    0.000             0.000          profile:0(profiler)
#        1    0.001    0.001   30.517   30.517 profile:0(test(o, n))
#        1    2.481    2.481   30.516   30.516 testod.py:7(test)
#
#
#Skips seqdict
#         30003 function calls in 228.620 CPU seconds
#
#   Ordered by: standard name
#
#   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
#        1    0.006    0.006  228.619  228.619 <string>:1(?)
#     5000   52.676    0.011  133.975    0.027 ndict.py:126(items)
#     5000    0.168    0.000    0.168    0.000 ndict.py:128(keys)
#    10000  164.001    0.016  164.001    0.016 ndict.py:131(values)
#     5000    0.276    0.000    0.276    0.000 ndict.py:51(__getitem__)
#     5000    0.318    0.000    0.318    0.000 ndict.py:58(__setitem__)
#        0    0.000             0.000          profile:0(profiler)
#        1    0.001    0.001  228.620  228.620 profile:0(test(o, n))
#        1   11.173   11.173  228.613  228.613 testod.py:7(test)
#
##


from operator import getitem, setitem, contains, delitem
from types import TupleType, ListType

SeqTypes = (ListType, TupleType)

def isListOfLists( obj):
	if type(obj) in SeqTypes:
		if obj and type(obj[0]) in (SeqTypes):
			return 1
	return 0

class MapList:
	def __init__(self, other=None):
		self.init()
		if other:
			self.update(other)
# standard Dict methods

	def __len__(self):
		"""Returns number of values"""
		return len(self._keys)

	def toDict(self):
		"""Returns a new dictionary of the items"""
		d={}
		map(lambda (k,v), d=d: setitem(d, k, v), self._items)
		return d

	def __setitem__(self, key, value):
		"""Set a value by key"""
		if not self.dict.has_key(key):
			obj = [key,value]
			self._items.append(obj)
			self._keys.append(key)
			self.dict[key]=obj
		else:
			self.dict[key][1]=value

	def __getitem__(self, key):
		"""get a value by key"""
		return self.dict[key][1]
	
	def __delitem__(self, key):
		"""delete a value by key"""
		obj = self.dict[key]
		self._items.remove(obj)
		self._keys.remove(obj[0])
		del self.dict[key]

	def __cmp__(self,other):
		"""cmp list of items"""
		return cmp(self.items(),other.items())
	def __repr__(self):
		body = map(lambda (k,v):  ":".join((repr(k),repr(v))), self._items)
		return "<%s %s >" % (self.__class__.__name__, ",".join(body))

	def keys(self, keysRequested=(), ordered=1):
		"""return keys of MapList

		if sequence keysRequested is given only self.keys() & keysRequested is returned in the order stored.
		if ordered=0 keys are returned in the order of keysRequested.
		Invalid  keys in keysRequested are ignored.
		"""
		
		if keysRequested:
			if ordered:
				return filter(lambda x, keys = keysRequested: x in keys, self._keys)
			else:
				has=self.dict.has_key
				return filter(has, keysRequested)
		return self._keys

	def values(self, keysRequested=(),ordered=1):
		"""return values of MapList

		if sequence keysRequested is given only self.values() for keys in  keysRequested are returned in the order stored.
		if ordered=0 values are returned in the order of keysRequested.
		Invalid  keys in keysRequested are ignored.
		"""

		items = self._items
		if keysRequested:
			if ordered:
				items =  filter(lambda (k,v), keys = keysRequested: k in keys, self._items)
			else:
				has=self.dict.has_key
				get = self.dict.get
				return map(get,  filter(has, keysRequested))
		return map(getitem, items,(1,)*len(items))

	def items(self, keys=(), ordered=1):
		"""return items of MapList

		if sequence keysRequested is given only self.items() for keys in  keysRequested are returned in the order stored.
		if ordered=0 items are returned in the order of keysRequested.
		Invalid  keys in keysRequested are ignored.
		"""

		if keys:
#			if ordered:
				return filter(lambda (k,v), keys = keys: k in keys, self._items)
#			else:
#				return filter(lambda (k,v), keys= keys: 
		return self._items

	def has_key(self, key):
		return self.dict.has_key(key)

	def get(self, key, default=None):
		return self.dict.get(key,  (None,default))[1]
		

	def setdefault(self, key, default=None):
		if self.dict.has_key(key):
			return self.dict[key][1]
		self[key] = default
		return default

	def popitem(self, i=-1):
		obj = self._items.pop(i)
		del self.dict[obj[0]]
		self._keys.pop(i)
		return obj

	def clear(self):
		self.init()

	def copy(self):
		if self.__class__ is MapList:
			return self.__class__(self._items)
		import copy
		return copy.copy(self)

	def update(self, other):
		"""updates MapList with a MapList, Dictionary, or sequence
		of sequences that are treated as list of items"""
		if isinstance(other, MapList):
			set = self.tupleSet
			map(set, other._items)
	       	elif isinstance(other, type(self.dict)):
			set = self.tupleSet
			map(set, other.items())
		elif isinstance(other, type(self._items)):
			set = self.tupleSet
			map(set, other)
        	else:
			set = self.tupleSet
			map(set, list(other))
		return self


	def init(self):
		self.dict={}
		self._items=[]
		self._keys=[]




# Non standard Dict methods
	set = __setitem__

	def tupleSet(self, tup):
		obj = list(tup)
		key = tup[0]
		if not self.dict.has_key(key):
		    self._items.append(obj)
		    self._keys.append(key)
		self.dict[key]=obj

#Some standerd List methods
	def __contains__(self, key):
		"""same as has_key(key)"""
		return key in self._keys

	def __getslice__(self, i, j):
		"""return new MapList with items sliced between indices i and j"""
		i = max(i, 0); j = max(j, 0)
		return self.__class__(self._items[i:j])
		
	def __setslice__(self, i, j, other):
		"""set  items of MapList between indices i and j with items form other MapList, map, or sequence of items"""
		i = max(i, 0); j = max(j, 0)
		doDel = 1
		if isinstance(other, MapList):
			self._items[i:j] = other._items
			self._keys[i:j] = other._keys
		elif isinstance(other, type(self._items)):
			self._items[i:j] = other
			self._keys[i:j] = map(getitem, (other,)*len(other), (0,)*len(other))
		elif isinstance(other, type(self.dict)):
			self._items[i:j] = other.items()
			self._items[i:j] = other.keys()
		elif isListOfLists(other):
			self._items[i:j] = other
			self._keys[i:j] = map(getitem, (other,)*len(other), (0,)*len(other))	
		elif other in SeqTypes:
			self._items[i:j] = map(list, zip(other, other))
			self._keys[i:j] = other
		else :
			other = (other,) * (j-i)
			self._items[i:j] = map(list, zip(self._keys[i:j], other))
			doDel =0
		if doDel:
			map(delitem, (self.dict,)*len(other), self._keys[i:j])
		map(setitem, (self.dict,)*len(other), self._keys[i:j], self._items[i:j])
		
		

	def __delslice__(self, i, j):
		"""delete items form MapList between indices i and j"""
		i = max(i, 0); j = max(j, 0)
		deli = self.__delitem__
		map(deli, self._keys[i:j])

	def getByIndex(self, index):
		"""return value by index"""
		return self._items[index]

	def setByIndex(self, index, keyNvalue):
		"""set value by index"""
		keyNvalue = list(keyNvalue)
		prevkey = self._keys[index]
		key = keyNvalue[0]
		self._keys[index] = key
		self._items[index] = keyNvalue
		del self.dict[prevkey]
		if key != prevkey and self.dict.has_key(key):
				self[prevkey]
		self.dict[key]= keyNvalue[0]

	def delByIndex(self, index):
		"""delete item by index"""
		key = self._keys[index]
		del self[key]
		
	def append(self, key, value):
		"""append item to MapList"""
		self[key] =value
		"""
		obj = [key,value]
		if self.dict.has_key(key):
		    oldobj = self.dict[key]
		    i = self._items.index(oldobj)
		    self._items[i] = obj
		self.dict[key]=obj
		"""

	def insert(self, i, key, value):
		"""insert item into MapList"""
		obj=[key,value]
		if self.dict.has_key(key):
			i = self._keys.index[key]
			del self._keys[i]
			del self._items[i]
		else:
			self.dict[key] = obj
		self._keys.insert(i, key)
		self._items.insert(i, obj)
	
	def pop(self, i=-1):
		"""pop item from MapList, if i given from index i"""
		return self.popitem(i)
				
	def count(self,value):
		"""return number of times value is found in MapList"""
    		return self.values().count(value)
#
# Set style methods
#	union = MapList.__add__
	
	def __add__(self, other):
		"""MapList returned that is the Union of the set of keys of the operands"""
        	if isinstance(other, MapList):
            		return self.__class__(self._items + other._items)
        	elif isinstance(other, type(self.dict)):
            		return self.__class__(self._items + other.items())
		elif isinstance(other, type(self._items)):
			return self.__class__(self._items+other)
        	else:
        		return self.__class__(self._items + list(other))

	def __radd__(self, other):
        	if isinstance(other, MapList):
            		return self.__class__(other._items + self._items)
        	elif isinstance(other, type(self.dict)):
            		return self.__class__(other.items() + self._items)
		elif isinstance(other, type(self._items)):
			return self.__class__(other + self._items)
        	else:
        		return self.__class__(list(other) + self._items)

    	def __iadd__(self, other):		
		if isinstance(other, MapList):
			set = self.tupleSet
			map(set, other._items)
	       	elif isinstance(other, type(self.dict)):
			set = self.tupleSet
			map(set, other.items())
		elif isinstance(other, type(self._items)):
			set = self.tupleSet
			map(set, other)
        	else:
			set = self.tupleSet
			map(set, list(other))
		return self

	__or__ = __add__
	__ror__ = __radd__
	__ior__ = __iadd__
	
	def safeDel(self, key):
		if self.dict.has_key(key):
			del self[key]

	def __sub__(self, other):
		"a new MapList returned with items in other object excluded"
		newdict = MapList(self)
		delitem = newdict.safeDel
        	if isinstance(other, MapList):
			keys = other._keys
        	elif isinstance(other, type(self.dict)):
            		keys = other.keys()
		elif isinstance(other, type(self._items)):
			keys = map(getitem, other, (1,)*len(other))
        	else:
        		keys = map(getitem, list(other), (1,)*len(list(other)))
		map(delitem, keys)
		return newdict

	def __rsub__(self, other):
		newdict = MapList(other)
		delitem = newdict.__delitem__
 		map(delitem, self._keys)
 		return self

    	def __isub__(self, other):		
		delitem = self.__delitem__
        	if isinstance(other, MapList):
			keys = self._keys
        	elif isinstance(other, type(self.dict)):
            		keys = other.keys()
		elif isinstance(other, type(self._items)):
			keys = map(getitem, other, (1,)*len(other))
        	else:
        		keys = map(getitem, list(other), (1,)*len(list(other)))
		map(delitem, keys)
		return self
	

	def __mul__(self, other):
		"MapList returned that is the Intersection of the set of keys of the operands"
		has = self.dict.has_key
	       	if isinstance(other, MapList):
			keys = filter(has, other._keys)
			if keys:
				items = self.items(keys)
			else:
				items =[]
			return self.__class__(items)
        	if isinstance(other, type(self.dict)):
			keys = filter(has, other.keys())
			return self.__class__(self.items(keys))
		if not isinstance(other, type(self._items)):
			other = list(other)
 		items = filter(lambda (k,v), has=has: has(k), other)
		return self.__class__(items)
	__and__ = __mul__
  	__rmul__ = __mul__

	def __imul__(self, other):
		has = self.dict.has_key
	       	if isinstance(other, MapList):
			keys = filter(has, other._keys)
			items  = self.items(keys)
        	elif isinstance(other, type(self.dict)):
			keys = filter(has, other.keys())
			items = self.items(keys)
		elif  isinstance(other, type(self._items)):
			items = filter(lambda (k,v), has=has: has(k), other)
		else:
			items = filter(lambda (k,v), has=has: has(k), list(other))
 		self.clear()
		self.update(items)
		return self


	def __xor__(self, other):
		"""MapList returned that is the XOR of the set of keys of the
		operands, taht is all items that are not in both operands"""
		res = self & other
		return (self+other) - res

	def filter(self, func):
		"""creats a copy of MapList and then applies func to each item.
			if func returns 0 removes the item.
			func should expect a [key, value] pair"""
		return self.__class__(filter(func, self._items))

	def map(self, func, *args):
		"""Returns a MapList that is the result of appling func to each item.
			func should expect a [key, value] pair"""
		return self.__class__(map(func, self._items, *args))

	def reverse(self):
		""" reverses the order of the keys of the MapList"""
		self._keys.reverse()
		self._items.reverse()
	
	def sort(self,*args):
		""" sorts the keys of the MapList"""
		self._keys.sort( *args)
		self._items.sort( *args)

	def swap(self):
		""" swaps the keys with their values"""
		tmp = self.__class__(map( lambda (k,v): (v,k), self._items))
		self._keys, self.dict, self._items = tmp._keys, tmp.dict, tmp._items		      



def _test():
    import doctest, od1
    reload(od1)
    return doctest.testmod(od1)


if __name__ == '__main__':
	print "testing __init__"
	o1 = MapList()
	assert o1.items() == [], o1.items()
	o2=MapList(map(None, map(str, range(3)), range(3)))
	res = o2.items(), o2.keys(), o2.values()
	print o2
	assert res == ([['0', 0], ['1', 1], ['2', 2]], ['0', '1', '2'], [0, 1, 2]), res
	o3 = MapList(o2.toDict())
	items = o3.items()
	items.sort()
	assert items == o2.items(), items
	o3 = MapList(o2)
	assert o3.items() == o2.items(), o3.items()


	print "testing __setitem__"
	o = MapList()
	for i in range(5):
		o[str(i)]=i
	o['0']='zero'
	#print o.items()
	assert o.items() == [['0', 'zero'], ['1', 1], ['2', 2], ['3', 3], ['4', 4]], o.items()

	print "testing __getitem__"
	lst = []
	for i in range(5):
		lst.append(o[str(i)])
	assert lst == ['zero', 1, 2, 3, 4], lst

	print "testing __delitem__"
	for i in range(3):
		del o[str(i)]
	assert o.items() == [['3', 3], ['4', 4]], o.items()
	
	print "testing __cmp__"
	assert o2 == o3, (o2, o3)
	o2.popitem()
	assert o2 != o3, (o2, o3)

	print "testing keys"
	keys = map(str, range(3))
	o2=MapList(map(None, keys, range(3)))
	assert o2.keys() == keys, o2.keys()
	del o2['2']; del keys[2]
	assert o2.keys() == keys, o2.keys()
	o2['new'] = 'new'; keys.append('new')
	assert o2.keys() == keys, o2.keys()

	print "testing values"
	keys = map(str, range(3))
	values  =range(3)
	o2=MapList(map(None, keys, values))
	assert o2.values() == values, o2.values()
	del o2['2']; del values[2]
	assert o2.values() == values, o2.values()
	o2['new'] = 'new'; values.append('new')
	assert o2.values() == values, o2.values()

	print "testing items"
	items = map(list, map(None, map(str, range(3)),range(3)))
	o2=MapList( items )
	assert o2.items() == items, o2.items()
	del o2['2']; del items[2]
	assert o2.items() == items, o2.items()
	o2['new'] = 'new'; items.append(['new','new'])
	assert o2.items() == items, o2.items()

	
	print "testing has_key"
	keys = map(str, range(3))
	values  =range(3)
	o2=MapList(map(None, keys, values))
	assert map(o2.has_key, keys) == [1,] * len(keys)
	assert map(MapList().has_key, keys) == [0,] * len(keys)

	print "testing getitem"
	lst = []
	o2=MapList(map(None, map(str, range(3)),range(3)))
	for i in range(15):
		lst.append(o2.get(str(i),i))
	assert lst == range(15), lst


	print "testing setdefault"
	lst = []
	o2=MapList(map(None, map(str, range(3)),range(3)))
	for i in range(15):
		lst.append(o2.setdefault(str(i),i))
	assert lst == range(15) and lst == o2.values(), lst

	print "testing popitem"
	lst = []
	o2=MapList(map(None, map(str, range(3)),range(3)))
	ref = MapList(o2)
	for i in range(len(ref)):
		pi = o2.popitem()
		assert ref.items()[-(i+1)] == pi, pi
	try:
		o2.popitem()
	except IndexError:
		pass
	except:
		assert 0, 'failed pop passed end'
	else:
		assert 0, 'failed pop passed end'


	print "testing count"
	keys = map(str, range(3))
	values  =range(3)
	o2=MapList(map(None, keys, values))
	assert map(o2.count, values) == [1,] * len(values)
	assert map(MapList().count, values) == [0,] * len(values)
	o2['z'] = 0
	assert o2.count(0) == 2


	print "testing __getslice__"
	keys = map(str, range(6))
	values  =range(6)
	o2=MapList(map(None, keys, values))
	assert o2[2:4].values() == range(6)[2:4]
	assert o2[2:-2].values() == range(6)[2:-2]
	
	print "testing __delslice__"
	keys = map(str, range(6))
	values  =range(6)
	o2=MapList(map(None, keys, values))
	del o2[2:4]
	ref = range(6)
	del ref[2:4]
	assert o2.values() == ref
	o2=MapList(map(None, keys, values))
	del o2[2:-2]
	del ref[2:-2]
	assert o2.values() == ref

	
	print "testing append"

	o2=MapList(map(None, map(str, range(3)),range(3)))
	o21=MapList()
	for k, v in o2.items():
		o21.append(k, v)
	assert o21 == o2

	print "testing insert"
	o2=MapList(map(None, map(str, range(3)),range(3)))
	o21=MapList()
	for k, v in o2.items():
		o21.insert(0, k, v)
	o21.reverse()
	assert o21 == o2, (o21, o2)

	print "testing popitem"
	o2=MapList(map(None, map(str, range(3)),range(3)))
	o21 = MapList()
	ref = MapList(o2)
	for i in range(len(ref)):
		apply(o21.append, o2.pop(0))
	assert o21 == ref
	try:
		o21.pop(23)
	except IndexError:
		pass
	except:
		assert 0, 'failed pop passed end'
	else:
		assert 0, 'failed pop passed end'


	
	print "testing add"
	o2=MapList(map(None, map(str, range(3)),range(3)))
	o3=MapList(map(None, map(str, range(2,6)),range(2,6)))
	o4 = o2 + o3
	assert o4.values() == range(6), o4.values()
	o4 = o2 + o2
	assert o4 == o2
	o2 =MapList(map(None, map(str, range(3)),range(3)))
	o3=MapList(map(None, map(str, range(6,8)),range(6,8)))
	o4 = o2 + o3
	ans = range(8)
	del ans[3:6]
	assert o4.values() == ans, (ans, o4.values())

	

	print "testing sub"
	o1=MapList(map(None, map(str, range(3)),range(3)))
	o2=MapList(map(None, map(str, range(2,6)),range(2,6)))
	o3=MapList(map(None, map(str, range(10,16)),range(10,16)))
	o4 = o1 - o2
	assert o4.values() == [0,1], o4
	o4 = o2 - o1
	assert o4.values() == [3,4,5], o4
	o4 = o3 - o1
	assert o4.values()==[10, 11, 12, 13, 14, 15], o4
	o4 = o1 - o3
	assert o4.values() == [0, 1, 2], o4
	o4 = o1 - o1
	assert o4.values() == [], o4

	print "testing union"
	o1=MapList(map(None, map(str, range(3)),range(3))) # [0,1,2]
	o2=MapList(map(None, map(str, range(1,6)),range(1,6))) #[1,2,3,4,5]
	o3=MapList(map(None, map(str, range(10,16)),range(10,16))) #[10,11,12,13,14,15]
	o4 = o1 * o2
	assert o4.values() == [1,2], o4
	o4 = o2 &  o1
	assert o4.values() == [1, 2], o4
	o4 = o3 * o1
	assert o4.values()==[], o4
	o4 = o1 * o3
	assert o4.values() == [], o4
	o4 = o1 * o1
	assert o4 == o1, o4

	print "testing xor"
	o1=MapList(map(None, map(str, range(3)),range(3))) # [0,1,2]
	o2=MapList(map(None, map(str, range(1,6)),range(1,6))) #[1,2,3,4,5]
	o3=MapList(map(None, map(str, range(10,16)),range(10,16))) #[10,11,12,13,14,15]
	o4 = o1 ^ o2
	assert o4.values() == [0,3,4,5], o4
	o4 = o2 ^  o1
	assert o4.values() == [3,4,5,0], o4
	o4 = o3 ^ o1
	assert o4.values() ==[10,11,12,13,14,15,0,1,2], o4
	o4 = o1 ^ o3
	assert o4.values() == [0,1,2,10,11,12,13,14,15], o4
	o4 = o1 ^ o1
	assert not o4, o4

	print "testing filter"
	o1=MapList(map(None, map(str, range(10)),range(10))) 
	o1[3:6]= 'HoHo'
	o4 = o1.filter(lambda (k,v):v=='HoHo')
	assert o4.keys() == o1.keys()[3:6], o4

	print "testing map"
	o1=MapList(map(None, map(str, range(10)),range(10))) 
	o4 = o1.map(lambda (k,v), i:[k,v+i], (1,)*len(o1))
	assert o4.values() == range(1,11), o4

	print "testing reverse"
	o1=MapList(map(None, map(str, range(10)),range(10))) 
	o1.reverse()
	assert o1.values() == range(9,-1,-1),o1

	print "testing sort"
	o1.sort()
	assert o1.values() == range(10),o1

	print "testing swap"
	o1.swap()
	assert o1.keys() == range(10),o1


	
	
	import UserDict
	containers=[({},'Python Dictionary'),(UserDict.UserDict(),'UserDict'),(MapList(),'Thanos\' MapList')]
	try:
		#import avl
		#containers.append((avl.avl(), 'avl'))
		import ndict
		containers.append((ndict.seqdict(), 'Wolfgang Grafen\'s seqdict'))
	except:
		print "Failed to import Wolfgang Grafen's seqdict"

	def test(o, n):
		for x in xrange(n):
			o[x] = x
			tt = o[x]
			kk =o.keys()
			vv = o.values()
			ii = o.items()
	import profile
	import sys
	print "Performance tests"
    	try:
		lengths= map(long,sys.argv[1:])
	except:
		lengths=(50, 100, 500)
	if not lengths:
		lengths=(50, 100, 500)
	for n in lengths:
		print 'for', n, 'iterations'
		print """doing:
		for x in xrange(%d):
            o[str(x)] = x
            tt = o[str(x)]
            kk =o.keys()
            vv = o.values()
            ii = o.items()
    """ % n
		for o, m in containers:
			print m
			profile.run('test(o, n)')
   
	

	
	


