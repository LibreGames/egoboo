## An Egoboo Object

import wx
import os

class EgoObject:
    
    newObjDir = 'new.obj'
    
    def __init__(self, dirName=None):
	tmpDir = dirName
	if tmpDir is None:
	    tmpDir = newObjDir
	self.panels = None
	self.data = ObjBaker.EgoObjData(tmpDir)
	self.dirName = dirName
    
    def save(self, dirName=None):
	if dirName is None and self.dirName is None:
	    raise ObjBaker.Error("dirName is None and self.dirName is None")
	self.data.save(dirName)
	if dirName is not None:
	    self.dirName = dirName
    
    ## Returns a dictionary of ("Tab Name", wx.Panel)
    def getwxPanels(self):
	if self.panels is not None:
	    return self.panels
	panels = {}
	panels["data.txt"] = self.data.getwxPanel()
	self.panels = panels
	return self.panels
    
    def isNewObject(self):
	return self.dirName is None

def removeUnderlines(string):
    return string.replace('_', ' ')

def addUnderlines(string):
    return string.replace(' ', '_')

def gotoColon(fh):
    ch = ''
    while ch != ':':
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("No more : in file!")
    print "file pos moved to %d" % fh.tell()

def getFirstChar(fh):
    gotoColon(fh)
    ch = ' '
    while ch.isspace():
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    print "Moved file pos to %d" % fh.tell()
    return ch

def getString(fh):
    ch = getFirstChar(fh)
    string = ''
    while not ch.isspace():
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    print "getString got %s" % string
    return string

def getInt(fh):
    ch = getFirstChar(fh)
    string = ''
    while ch.isdigit() or ch == '-':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    print "getInt got %s" % string
    return int(string)

def getFloat(fh):
    ch = getFirstChar(fh)
    string = ''
    while ch.isdigit() or ch == '-' or ch == '.':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    print "getFloat got %s" % string
    return float(string)

def getBool(fh):
    ch = getFirstChar(fh)
    print "getBool got %s" % ch
    if ch == 'T':
	return 'TRUE'
    return 'FALSE'

def getIDSZ(fh):
    ch = getFirstChar(fh)
    idsz = fh.read(4)
    if len(idsz) != 4:
	raise ObjBaker.Error("Hit end of file (got %d chars, expected 4)" % len(idsz))
    print "getIDSZ got [%s]" % idsz
    return idsz

def getPair(fh):
    ch = getFirstChar(fh)
    string = ''
    while ch.isdigit() or ch == '-':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    print "getPair got %s" % string
    return string.split('-')