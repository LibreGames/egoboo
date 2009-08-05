## An error for ObjBaker

class Error(Exception):
    def __init__(self, value):
	self.mess = value
    def __str__(self):
	return repr(self.mess)