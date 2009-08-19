# Template Manager for ObjBaker

import os
import ObjBaker

class Templater:
    
    def __init__(self, fh, fileName):
	tmplName = os.path.join('templates', '%s.tmpl' % fileName)
	try:
	    self.tmplfh = open(tmplName, 'r')
	except IOError, e:
	    raise ObjBaker.Error(e)
	self.outfh = fh
	self.lineNum = 0
    
    def write(self, data):
	line = self.tmplfh.readline()
	if len(line) == 0:
	    raise ObjBaker.Error("Hit end of file!")
	self.lineNum += 1
	while line.find('%') == -1:
	    self.outfh.write(line)
	    line = self.tmplfh.readline()
	    if len(line) == 0:
		raise ObjBaker.Error("Hit end of file!")
	    self.lineNum += 1
	#print "line", self.lineNum, data
	if line.find('%f') != -1:
	    line = line.replace('%f', '%s')
	    data = str(data).rstrip('0')
	    if data[-1] == '.':
		data += '0'
	self.outfh.write(line % data)
    
    def writeEnd(self):
	line = self.tmplfh.readline()
	while len(line) != 0:
	    self.outfh.write(line)
	    line = self.tmplfh.readline()
	    self.lineNum += 1
	#print "ending line", self.lineNum
	self.tmplfh = None