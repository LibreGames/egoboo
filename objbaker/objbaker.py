#!/usr/bin/env python

# ObjBaker in wx form

import wx
import ObjBaker

class ObjBakerApp(wx.App):
    def OnInit(self):
	frame = ObjBaker.MainWindow()
	frame.Show()
	self.SetTopWindow(frame)
	return True

if __name__ == '__main__':
    app = ObjBakerApp()
    app.MainLoop()