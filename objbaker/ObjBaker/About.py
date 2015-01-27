# About box for ObjBaker

import wx
import ObjBaker.version

def doAbout(frame):
    info = wx.AboutDialogInfo()
    info.Name = "ObjBaker"
    info.Version = ObjBaker.version.VERSION
    info.Copyright = "(c) 2009 pf5234"
    info.Description = "ObjBaker helps create and modify objects for Egoboo"
    info.WebSite = ('http://egoboo.sourceforge.net/', "Egoboo's homepage")
    info.Developers = ["pf5234"]
    info.License = ''.join(open("LICENSE.txt").readlines())
    wx.AboutBox(info)