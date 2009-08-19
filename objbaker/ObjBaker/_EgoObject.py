## An Egoboo Object

import wx
import os
import ObjBaker

newObjDir = 'new.obj'

class EgoObject:
    
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
    def getPanels(self, parent=None):
	if self.panels is not None:
	    return self.panels
	panels = {}
	panels['data.txt'] = self.data.getPanel(parent)
	self.panels = panels
	return self.panels
    
    def isNewObject(self):
	return self.dirName is None
    
    def wasEdited(self):
	if self.data.wasEdited():
	    return True
	#elif self.whatnot.edited():
	return False

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
    #print "file pos moved to %d" % fh.tell()

def getFirstChar(fh, skipColon=False):
    if not skipColon:
	gotoColon(fh)
    ch = ' '
    while ch.isspace():
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    #print "Moved file pos to %d" % fh.tell()
    return ch

def getString(fh, skipColon=False):
    ch = getFirstChar(fh, skipColon)
    string = ''
    while not ch.isspace():
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    #print "getString got %s" % string
    return string

def getInt(fh, skipColon=False):
    ch = getFirstChar(fh, skipColon)
    string = ''
    while ch.isdigit() or ch == '-':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    #print "getInt got %s" % string
    return int(string)

def getFloat(fh, skipColon=False):
    ch = getFirstChar(fh, skipColon)
    string = ''
    while ch.isdigit() or ch == '-' or ch == '.':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    #print "getFloat got %s" % string
    return float(string)

def getBool(fh, skipColon=False):
    ch = getFirstChar(fh, skipColon)
    #print "getBool got %s" % ch
    if ch == 'T':
	return 'TRUE'
    return 'FALSE'

def getIDSZ(fh, skipColon=False):
    ch = getFirstChar(fh, skipColon)
    while ch != '[':
	ch = fh.read(1)
	if len(ch) == 0: 
	    raise ObjBaker.Error("Hit end of file!")
    idsz = fh.read(4)
    ch = fh.read(1)
    if len(idsz) != 4 or len(ch) == 0:
	raise ObjBaker.Error("Hit end of file (got %d chars, expected 5)" % len(idsz)+len(ch))
    if ch != ']':
	raise ObjBaker.Error("Error, got a [%s with no ]" % idsz)
    #print "getIDSZ got [%s]" % idsz
    return idsz

def getPair(fh, skipColon=False):
    ch = getFirstChar(fh)
    string = ''
    while ch.isdigit() or ch == '-' or ch == '.':
	string += ch
	ch = fh.read(1)
	if len(ch) == 0:
	    raise ObjBaker.Error("Hit end of file")
    #print "getPair got %s" % string
    splittedString = string.split('-')
    if len(splittedString) == 1:
	splittedString.append(splittedString[0])
    return [float(splittedString[0]), float(splittedString[1])]

def getExpansion(fh):
    idsz = getIDSZ(fh)
    value = None
    if idsz == "LIFE" or idsz == "MANA":
	value = getFloat(fh, True)
    else:
	value = getInt(fh, True)
    #print "getExpansion got [%s] %f" % (idsz, value)
    return (idsz, value)

def get4Int(fh, skipColon=False):
    retValue = []
    retValue.append(getInt(fh, skipColon))
    retValue.append(getInt(fh, True))
    retValue.append(getInt(fh, True))
    retValue.append(getInt(fh, True))
    return retValue

def get4Char(fh, skipColon=False):
    retValue = []
    retValue.append(getFirstChar(fh, skipColon))
    retValue.append(getFirstChar(fh, True))
    retValue.append(getFirstChar(fh, True))
    retValue.append(getFirstChar(fh, True))
    return retValue

def undoPair(pair):
    string0 = "%.3f" % pair[0]
    string0 = string0.rstrip('0').rstrip('.')
    string1 = "%.3f" % pair[1]
    string1 = string1.rstrip('0').rstrip('.')
    if string0 == string1:
	return string0
    return "%s-%s" % (string0, string1)

# func params are (newValue, pairNum, ctrl)

def addIntControl(panel, sizer, label, func, initial=0, minn=0, maxx=100):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    tmpCtrl = wx.SpinCtrl(panel, min=minn, max=maxx, initial=initial)
    panel.Bind(wx.EVT_SPINCTRL, lambda evt: func(evt.GetInt(), -1, tmpCtrl),
	tmpCtrl)
    sizer.Add(tmpCtrl)

def addStringControl(panel, sizer, label, func, initial=''):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    tmpCtrl = wx.TextCtrl(panel, -1, initial, size=(150,27))
    panel.Bind(wx.EVT_TEXT, lambda evt: func(evt.GetString(), -1, tmpCtrl),
	tmpCtrl)
    sizer.Add(tmpCtrl)

def addBoolControl(panel, sizer, label, func, initial=False):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    tmpCtrl = wx.CheckBox(panel, -1)
    tmpCtrl.SetValue(initial == 'TRUE')
    panel.Bind(wx.EVT_CHECKBOX, lambda evt: func(
	evt.IsChecked() and 'TRUE' or 'FALSE', -1, tmpCtrl), tmpCtrl)
    sizer.Add(tmpCtrl)

def addChoiceControl(panel, sizer, label, func, theList, initial=0):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    tmpCtrl = wx.Choice(panel, -1, choices=theList)
    tmpCtrl.SetSelection(initial)
    panel.Bind(wx.EVT_CHOICE, lambda evt: func(theList[evt.GetSelection()],
	-1, tmpCtrl), tmpCtrl)
    sizer.Add(tmpCtrl)

def addPairControl(panel, sizer, label, func, initial=[0,0]):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    tmpSizer = wx.BoxSizer(wx.HORIZONTAL)
    leftSide = wx.TextCtrl(panel, -1, str(initial[0]))
    panel.Bind(wx.EVT_TEXT, lambda evt: func(evt.GetString(), 0, leftSide),
	leftSide)
    tmpSizer.Add(leftSide)
    tmpSizer.Add(wx.StaticText(panel, -1, " - "), flag=wx.ALIGN_CENTER)
    rightSide = wx.TextCtrl(panel, -1, str(initial[1]))
    panel.Bind(wx.EVT_TEXT, lambda evt: func(evt.GetString(), 1, rightSide),
	rightSide)
    tmpSizer.Add(rightSide)
    sizer.Add(tmpSizer)

def addFloatControl(panel, sizer, label, func, initial=0):
    sizer.Add(wx.StaticText(panel, -1, label),
	flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    leftSide = wx.TextCtrl(panel, -1, str(initial))
    panel.Bind(wx.EVT_TEXT, lambda evt: func(evt.GetString(), -1, leftSide),
	leftSide)
    sizer.Add(leftSide)