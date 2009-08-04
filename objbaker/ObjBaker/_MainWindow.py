# The main window for ObjBaker in wx

import wx
import os
import ObjBaker.About
import ObjBaker.Images as images

class MainWindow(wx.Frame):
    
    def __init__(self):
	wx.Frame.__init__(self, None, title="ObjBaker")
	## Blah, Windows :(
	self.SetIcon(wx.Icon(os.path.join('icons', 'icon.ico'), wx.BITMAP_TYPE_ICO))
	images.loadImages()
	self.loadMenuBar()
	self.loadToolBar()
	self.loadStatusBar()
    
    def loadMenuBar(self):
	self.menuBar = menuBar = wx.MenuBar()
	
	fileMenu = wx.Menu()
	
	newObjectMenu = wx.MenuItem(fileMenu, -1, "&New Object\tCtrl+N",
	    "Make a new object")
	newObjectMenu.SetBitmap(images.get('new'))
	fileMenu.AppendItem(newObjectMenu)
	self.Bind(wx.EVT_MENU, self.doNewObject, newObjectMenu)
	
	openObjectMenu = wx.MenuItem(fileMenu, -1, "&Open Object\tCtrl+O",
	    "Open an existing object")
	openObjectMenu.SetBitmap(images.get('open'))
	fileMenu.AppendItem(openObjectMenu)
	self.Bind(wx.EVT_MENU, self.doOpenObject, openObjectMenu)
	
	saveObjectMenu = wx.MenuItem(fileMenu, -1, "&Save Object\tCtrl+S",
	    "Save the object")
	saveObjectMenu.SetBitmap(images.get('save'))
	fileMenu.AppendItem(saveObjectMenu)
	self.Bind(wx.EVT_MENU, self.doSaveObject, saveObjectMenu)
	
	saveAsObjectMenu = wx.MenuItem(fileMenu, -1, "Save Object &As\tCtrl+Shift+S",
	    "Save the object as another object")
	saveAsObjectMenu.SetBitmap(images.get('saveas'))
	fileMenu.AppendItem(saveAsObjectMenu)
	self.Bind(wx.EVT_MENU, self.doSaveAsObject, saveAsObjectMenu)
	
	fileMenu.AppendSeparator()
	
	quitMenu = wx.MenuItem(fileMenu, -1, "&Quit\tCtrl+Q", "Quit")
	quitMenu.SetBitmap(images.get('quit'))
	fileMenu.AppendItem(quitMenu)
	self.Bind(wx.EVT_MENU, self.doQuit, quitMenu)
	
	helpMenu = wx.Menu()
	
	helpItemMenu = wx.MenuItem(helpMenu, -1, "&Help\tCtrl+?", "Help me!")
	helpItemMenu.SetBitmap(images.get('help'))
	helpMenu.AppendItem(helpItemMenu)
	self.Bind(wx.EVT_MENU, self.doHelp, helpItemMenu)
	
	helpMenu.AppendSeparator()
	
	aboutMenu = wx.MenuItem(helpMenu, -1, "&About", "About ObjBaker")
	aboutMenu.SetBitmap(images.get('about'))
	helpMenu.AppendItem(aboutMenu)
	self.Bind(wx.EVT_MENU, self.doAbout, aboutMenu)
	
	menuBar.Append(fileMenu, "&File")
	menuBar.Append(helpMenu, "&Help")
	
	self.SetMenuBar(menuBar)
    
    def loadToolBar(self):
	self.toolBar = self.CreateToolBar()
	
	self.toolBar.AddLabelTool(300, "", images.get('new'),
	    shortHelp="Make a new object")
	self.Bind(wx.EVT_TOOL, self.doNewObject, id=300)
	
	self.toolBar.AddLabelTool(301, "", images.get('open'),
	    shortHelp="Open an existing object")
	self.Bind(wx.EVT_TOOL, self.doOpenObject, id=301)
	
	self.toolBar.AddLabelTool(302, "", images.get('save'),
	    shortHelp="Save the object")
	self.Bind(wx.EVT_TOOL, self.doSaveObject, id=302)
	
	self.toolBar.AddLabelTool(303, "", images.get('saveas'),
	    shortHelp="Save the object as another object")
	self.Bind(wx.EVT_TOOL, self.doSaveAsObject, id=303)
	
	self.toolBar.AddSeparator()
	
	self.toolBar.AddLabelTool(305, "", images.get('help'),
	     shortHelp="Help me!")
	self.Bind(wx.EVT_TOOL, self.doHelp, id=305)
	
	self.toolBar.AddLabelTool(304, "", images.get('quit'), shortHelp="Quit")
	self.Bind(wx.EVT_TOOL, self.doQuit, id=304)
	
	self.toolBar.Realize()
    
    def loadStatusBar(self):
	self.statusBar = statusBar = self.CreateStatusBar()
    
    def doNewObject(self, event):
	print "MainWindow.doNewObject called!"
	self.statusBar.PushStatusText("MainWindow.doNewObject called!")
    
    def doOpenObject(self, event):
	print "MainWindow.doOpenObject called!"
	self.statusBar.PushStatusText("MainWindow.doOpenObject called!")
    
    def doSaveObject(self, event):
	print "MainWindow.doSaveObject called!"
	self.statusBar.PushStatusText("MainWindow.doSaveObject called!")
    
    def doSaveAsObject(self, event):
	print "MainWindow.doSaveAsObject called!"
	self.statusBar.PushStatusText("MainWindow.doSaveAsObject called!")
    
    def doQuit(self, event):
	print "MainWindow.doQuit called!"
	self.Close()
    
    def doHelp(self, event):
	print "MainWindow.doHelp called!"
	self.statusBar.PushStatusText("MainWindow.doHelp called!")
    
    def doAbout(self, event):
	print "MainWindow.doAbout called!"
	self.statusBar.PushStatusText("MainWindow.doAbout called!")
	ObjBaker.About.doAbout(self)