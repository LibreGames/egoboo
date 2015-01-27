"""
    ObjBaker - Object Editor for Egoboo
    Copyright (C) 2009  Sean Baker

    This file is part of ObjBaker.

    ObjBaker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ObjBaker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ObjBaker.  If not, see <http://www.gnu.org/licenses/>.
"""

def strippedtext():
    f = open('EgoScript.txt', 'r')
    p = []
    for i in f.readlines():
        p.append(i.replace('\n', ''))
    return p
                 
def load_EgoScript():
    EgoScriptFile = open('EgoScript.txt', 'r')
    Set = 'functions'
    EgoScript = {'functions':[] ,'constants':[] ,'variables':[]}
    for i in strippedtext() :
        if i == '[functions]' :
            Set = 'functions'
        elif i == '[constants]' :
            Set = 'constants'
        elif i == '[variables]' :
            Set = 'variables'
        else :
            EgoScript[Set].append(i)
    return EgoScript
                 
if __name__ == '__main__' :
    print load_EgoScript()
