#!/usr/bin/env python

# how to run: python datatest.py path/to/the.obj

import ObjBaker
import sys

if __name__ == '__main__':
    data = ObjBaker.EgoObjData(sys.argv[1])
    for key in data.data:
	print "%s: %s" % (key, data.data[key])
    data.save('.')