# Image handler for ObjBaker

import wx
import os

images = {}

def loadImages():
    images['new'] = wx.Bitmap(os.path.join('icons', 'document-new.png'))
    images['open'] = wx.Bitmap(os.path.join('icons', 'document-open.png'))
    images['save'] = wx.Bitmap(os.path.join('icons', 'document-save.png'))
    images['saveas'] = wx.Bitmap(os.path.join('icons', 'document-save-as.png'))
    images['quit'] = wx.Bitmap(os.path.join('icons', 'application-exit.png'))
    images['help'] = wx.Bitmap(os.path.join('icons', 'help-contents.png'))
    images['about'] = wx.Bitmap(os.path.join('icons', 'help-about.png'))

def get(key):
    return images[key]