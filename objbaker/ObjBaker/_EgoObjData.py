## An Egoboo Object

import wx
import wx.lib.scrolledpanel as scrolled
import os
import ObjBaker
import ObjBaker._EgoObject as EgoObj

genders = ['male', 'female', 'random', 'other']
barColors = ["empty", "red", "yellow", "green", "blue", "purple"]

class EgoObjData:
    
    def __init__(self, dirName):
	self.panel = None
	self.edited = False
	fileName = os.path.join(dirName, 'data.txt')
	fh = None
	try:
	    fh = open(fileName, 'r')
	except IOError, e:
	    raise ObjBaker.Error(e)
	
	data = {}
	# Real general data
	data['slot'] = EgoObj.getInt(fh)
	data['class'] = EgoObj.removeUnderlines(EgoObj.getString(fh))
	data['uniformlylit'] = EgoObj.getBool(fh)
	data['maxammo'] = EgoObj.getInt(fh)
	data['currammo'] = EgoObj.getInt(fh)
	data['gender'] = EgoObj.getString(fh) ## MALE FEMALE RANDOM OTHER
	# Object stats
	data['lifecolor'] = EgoObj.getInt(fh)
	data['manacolor'] = EgoObj.getInt(fh)
	data['lifepoints'] = EgoObj.getPair(fh)
	data['lifepointsperlevel'] = EgoObj.getPair(fh)
	data['manapoints'] = EgoObj.getPair(fh)
	data['manapointsperlevel'] = EgoObj.getPair(fh)
	data['manareturn'] = EgoObj.getPair(fh)
	data['manareturnperlevel'] = EgoObj.getPair(fh)
	data['manaflow'] = EgoObj.getPair(fh)
	data['manaflowperlevel'] = EgoObj.getPair(fh)
	data['strength'] = EgoObj.getPair(fh)
	data['strengthperlevel'] = EgoObj.getPair(fh)
	data['wisdom'] = EgoObj.getPair(fh)
	data['wisdomperlevel'] = EgoObj.getPair(fh)
	data['intelligence'] = EgoObj.getPair(fh)
	data['intelligenceperlevel'] = EgoObj.getPair(fh)
	data['dexterity'] = EgoObj.getPair(fh)
	data['dexterityperlevel'] = EgoObj.getPair(fh)
	# More physical attributes
	data['size'] = EgoObj.getFloat(fh)
	data['sizeperlevel'] = EgoObj.getFloat(fh)
	data['shadowsize'] = EgoObj.getInt(fh)
	data['bumpsize'] = EgoObj.getInt(fh)
	data['bumpheight'] = EgoObj.getInt(fh)
	data['bumpdampen'] = EgoObj.getFloat(fh)
	data['weight'] = EgoObj.getInt(fh)
	data['jumppower'] = EgoObj.getFloat(fh)
	data['numjumps'] = EgoObj.getInt(fh)
	data['sneakspeed'] = EgoObj.getInt(fh)
	data['walkspeed'] = EgoObj.getInt(fh)
	data['runspeed'] = EgoObj.getInt(fh)
	data['flytoheight'] = EgoObj.getInt(fh)
	data['flashingand'] = EgoObj.getInt(fh)
	data['transparencyblending'] = EgoObj.getInt(fh)
	data['lightblending'] = EgoObj.getInt(fh)
	data['transferblendingtoweapons'] = EgoObj.getBool(fh)
	data['sheen'] = EgoObj.getInt(fh)
	data['phongmapping'] = EgoObj.getBool(fh)
	data['texturexmovement'] = EgoObj.getFloat(fh)
	data['textureymovement'] = EgoObj.getFloat(fh)
	data['conformhillschair'] = EgoObj.getBool(fh)
	# Invulnerability data
	data['isinvincible'] = EgoObj.getBool(fh)
	data['noniframefacing'] = EgoObj.getInt(fh)
	data['noniframeangle'] = EgoObj.getInt(fh)
	data['iframefacing'] = EgoObj.getInt(fh)
	data['iframeangle'] = EgoObj.getInt(fh)
	# Skin defenses
	data['basedef'] = EgoObj.get4Int(fh)
	data['slashdef'] = EgoObj.get4Int(fh)
	data['crushdef'] = EgoObj.get4Int(fh)
	data['pokedef'] = EgoObj.get4Int(fh)
	data['holydef'] = EgoObj.get4Int(fh)
	data['evildef'] = EgoObj.get4Int(fh)
	data['firedef'] = EgoObj.get4Int(fh)
	data['icedef'] = EgoObj.get4Int(fh)
	data['zapdef'] = EgoObj.get4Int(fh)
	data['slashinv'] = EgoObj.get4Char(fh)
	data['crushinv'] = EgoObj.get4Char(fh)
	data['pokeinv'] = EgoObj.get4Char(fh)
	data['holyinv'] = EgoObj.get4Char(fh)
	data['evilinv'] = EgoObj.get4Char(fh)
	data['fireinv'] = EgoObj.get4Char(fh)
	data['iceinv'] = EgoObj.get4Char(fh)
	data['zapinv'] = EgoObj.get4Char(fh)
	data['accelrate'] = EgoObj.get4Int(fh)
	# Experience and level data
	data['expforlvl1'] = EgoObj.getInt(fh)
	data['expforlvl2'] = EgoObj.getInt(fh)
	data['expforlvl3'] = EgoObj.getInt(fh)
	data['expforlvl4'] = EgoObj.getInt(fh)
	data['expforlvl5'] = EgoObj.getInt(fh)
	data['currexp'] = EgoObj.getPair(fh)
	data['expworth'] = EgoObj.getInt(fh)
	data['expexchange'] = EgoObj.getFloat(fh)
	data['expratesecret'] = EgoObj.getFloat(fh)
	data['expratequest'] = EgoObj.getFloat(fh)
	data['exprateunknown'] = EgoObj.getFloat(fh)
	data['exprateenemy'] = EgoObj.getFloat(fh)
	data['expratesleepenemy'] = EgoObj.getFloat(fh)
	data['expratehateenemy'] = EgoObj.getFloat(fh)
	data['exprateteam'] = EgoObj.getFloat(fh)
	data['expratetalk'] = EgoObj.getFloat(fh)
	# IDSZ Identification tags ( [NONE] is valid )
	data['parentid'] = EgoObj.getIDSZ(fh)
	data['typeid'] = EgoObj.getIDSZ(fh)
	data['skillid'] = EgoObj.getIDSZ(fh)
	data['specialid'] = EgoObj.getIDSZ(fh)
	data['hateid'] = EgoObj.getIDSZ(fh)
	data['vulnerabilityid'] = EgoObj.getIDSZ(fh)
	# Item and damage flags
	data['isitem'] = EgoObj.getBool(fh)
	data['ismount'] = EgoObj.getBool(fh)
	data['isstackable'] = EgoObj.getBool(fh)
	data['nameknown'] = EgoObj.getBool(fh)
	data['usageknown'] = EgoObj.getBool(fh)
	data['exportable'] = EgoObj.getBool(fh)
	data['needsskillid'] = EgoObj.getBool(fh)
	data['isplatform'] = EgoObj.getBool(fh)
	data['getsmoney'] = EgoObj.getBool(fh)
	data['canopen'] = EgoObj.getBool(fh)
	#Other item and damage stuff
	data['damagetype'] = EgoObj.getString(fh)
	data['attacktype'] = EgoObj.getString(fh)
	# Particle attachments
	data['numparticlesattach'] = EgoObj.getInt(fh)
	data['reaffirmattachments'] = EgoObj.getString(fh)
	data['particletypeattach'] = EgoObj.getInt(fh)
	# Character hands
	data['leftgripvalid'] = EgoObj.getBool(fh)
	data['rightgripvalid'] = EgoObj.getBool(fh)
	# Particle spawning on attack order ( for weapon characters )
	data['particleattachweapon'] = EgoObj.getBool(fh)
	data['particletypeweapon'] = EgoObj.getInt(fh)
	# Particle spawning for GoPoof
	data['numparticlespoof'] = EgoObj.getInt(fh)
	data['facingadd'] = EgoObj.getInt(fh)
	data['particletypepoof'] = EgoObj.getInt(fh)
	# Particle spawning for Blud ( If you want it, you put it in... )
	data['bludvalid'] = EgoObj.getString(fh) # TRUE FALSE ULTRA
	data['particletypeblud'] = EgoObj.getInt(fh)
	# Extra stuff I forgot
	data['waterwalk'] = EgoObj.getBool(fh)
	data['bounciness'] = EgoObj.getFloat(fh)
	# More stuff I forgot
	data['lifeheal'] = EgoObj.getFloat(fh)
	data['manacost'] = EgoObj.getFloat(fh)
	data['lifereturn'] = EgoObj.getInt(fh)
	data['stoppedby'] = EgoObj.getInt(fh)
	data['skinname0'] = EgoObj.removeUnderlines(EgoObj.getString(fh))
	data['skinname1'] = EgoObj.removeUnderlines(EgoObj.getString(fh))
	data['skinname2'] = EgoObj.removeUnderlines(EgoObj.getString(fh))
	data['skinname3'] = EgoObj.removeUnderlines(EgoObj.getString(fh))
	data['skincost0'] = EgoObj.getInt(fh)
	data['skincost1'] = EgoObj.getInt(fh)
	data['skincost2'] = EgoObj.getInt(fh)
	data['skincost3'] = EgoObj.getInt(fh)
	data['strengthdampen'] = EgoObj.getFloat(fh)
	# Another memory lapse
	data['ridernoattack'] = EgoObj.getBool(fh)
	data['canbedazed'] = EgoObj.getBool(fh)
	data['canbegrogged'] = EgoObj.getBool(fh)
	data['permanentlife'] = EgoObj.getInt(fh)
	data['permanentmana'] = EgoObj.getInt(fh)
	data['canseeinvisible'] = EgoObj.getBool(fh)
	data['cursechance'] = EgoObj.getInt(fh)
	data['footfallsound'] = EgoObj.getInt(fh)
	data['jumpsound'] = EgoObj.getInt(fh)
	# Expansions
	data['expansions'] = {}
	try:
	    while True:
		tmpExp = EgoObj.getExpansion(fh)
		if tmpExp[0] == 'DRES':
		    if not data['expansions'].has_key('DRES'):
			data['expansions']['DRES'] = 0
		    data['expansions']['DRES'] |= 1 << int(tmpExp[1])
		else:
		    data['expansions'][tmpExp[0]] = tmpExp[1]
	except ObjBaker.Error:
	    pass
	# end of __init__
	self.data = data
    
    def save(self, dirName):
	fileName = os.path.join(dirName, 'data.txt')
	fh = None
	try:
	    fh = open(fileName, 'w')
	except IOError, e:
	    raise ObjBaker.Error(e)
	tmpl = ObjBaker.Templater(fh, "data.txt")
	data = self.data
	# Real general data
	tmpl.write(data['slot'])
	tmpl.write(EgoObj.addUnderlines(data['class']))
	tmpl.write(data['uniformlylit'])
	tmpl.write(data['maxammo'])
	tmpl.write(data['currammo'])
	tmpl.write(data['gender'])
	# Object stats
	tmpl.write(data['lifecolor'])
	tmpl.write(data['manacolor'])
	tmpl.write(EgoObj.undoPair(data['lifepoints']))
	tmpl.write(EgoObj.undoPair(data['lifepointsperlevel']))
	tmpl.write(EgoObj.undoPair(data['manapoints']))
	tmpl.write(EgoObj.undoPair(data['manapointsperlevel']))
	tmpl.write(EgoObj.undoPair(data['manareturn']))
	tmpl.write(EgoObj.undoPair(data['manareturnperlevel']))
	tmpl.write(EgoObj.undoPair(data['manaflow']))
	tmpl.write(EgoObj.undoPair(data['manaflowperlevel']))
	tmpl.write(EgoObj.undoPair(data['strength']))
	tmpl.write(EgoObj.undoPair(data['strengthperlevel']))
	tmpl.write(EgoObj.undoPair(data['wisdom']))
	tmpl.write(EgoObj.undoPair(data['wisdomperlevel']))
	tmpl.write(EgoObj.undoPair(data['intelligence']))
	tmpl.write(EgoObj.undoPair(data['intelligenceperlevel']))
	tmpl.write(EgoObj.undoPair(data['dexterity']))
	tmpl.write(EgoObj.undoPair(data['dexterityperlevel']))
	# More phyiscal attributes
	tmpl.write(data['size'])
	tmpl.write(data['sizeperlevel'])
	tmpl.write(data['shadowsize'])
	tmpl.write(data['bumpsize'])
	tmpl.write(data['bumpheight'])
	tmpl.write(data['bumpdampen'])
	tmpl.write(data['weight'])
	tmpl.write(data['jumppower'])
	tmpl.write(data['numjumps'])
	tmpl.write(data['sneakspeed'])
	tmpl.write(data['walkspeed'])
	tmpl.write(data['runspeed'])
	tmpl.write(data['flytoheight'])
	tmpl.write(data['flashingand'])
	tmpl.write(data['transparencyblending'])
	tmpl.write(data['lightblending'])
	tmpl.write(data['transferblendingtoweapons'])
	tmpl.write(data['sheen'])
	tmpl.write(data['phongmapping'])
	tmpl.write(data['texturexmovement'])
	tmpl.write(data['textureymovement'])
	tmpl.write(data['conformhillschair'])
	# Invulnerability data
	tmpl.write(data['isinvincible'])
	tmpl.write(data['noniframefacing'])
	tmpl.write(data['noniframeangle'])
	tmpl.write(data['iframefacing'])
	tmpl.write(data['iframeangle'])
	# Skin defenses
	tmpl.write(tuple(data['basedef']))
	tmpl.write(tuple(data['slashdef']))
	tmpl.write(tuple(data['crushdef']))
	tmpl.write(tuple(data['pokedef']))
	tmpl.write(tuple(data['holydef']))
	tmpl.write(tuple(data['evildef']))
	tmpl.write(tuple(data['firedef']))
	tmpl.write(tuple(data['icedef']))
	tmpl.write(tuple(data['zapdef']))
	tmpl.write(tuple(map(lambda string: string[0], data['slashinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['crushinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['pokeinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['holyinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['evilinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['fireinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['iceinv'])))
	tmpl.write(tuple(map(lambda string: string[0], data['zapinv'])))
	tmpl.write(tuple(data['accelrate']))
	# Experience and level data
	tmpl.write(data['expforlvl1'])
	tmpl.write(data['expforlvl2'])
	tmpl.write(data['expforlvl3'])
	tmpl.write(data['expforlvl4'])
	tmpl.write(data['expforlvl5'])
	tmpl.write(EgoObj.undoPair(data['currexp']))
	tmpl.write(data['expworth'])
	tmpl.write(data['expexchange'])
	tmpl.write(data['expratesecret'])
	tmpl.write(data['expratequest'])
	tmpl.write(data['exprateunknown'])
	tmpl.write(data['exprateenemy'])
	tmpl.write(data['expratesleepenemy'])
	tmpl.write(data['expratehateenemy'])
	tmpl.write(data['exprateteam'])
	tmpl.write(data['expratetalk'])
	# IDSZ Identification tags ( [NONE] is valid )
	tmpl.write(data['parentid'])
	tmpl.write(data['typeid'])
	tmpl.write(data['skillid'])
	tmpl.write(data['specialid'])
	tmpl.write(data['hateid'])
	tmpl.write(data['vulnerabilityid'])
	# Item and damage flags
	tmpl.write(data['isitem'])
	tmpl.write(data['ismount'])
	tmpl.write(data['isstackable'])
	tmpl.write(data['nameknown'])
	tmpl.write(data['usageknown'])
	tmpl.write(data['exportable'])
	tmpl.write(data['needsskillid'])
	tmpl.write(data['isplatform'])
	tmpl.write(data['getsmoney'])
	tmpl.write(data['canopen'])
	# Other item and damage stuff
	tmpl.write(data['damagetype'])
	tmpl.write(data['attacktype'])
	# Particle attachments
	tmpl.write(data['numparticlesattach'])
	tmpl.write(data['reaffirmattachments'])
	tmpl.write(data['particletypeattach'])
	# Character hands
	tmpl.write(data['leftgripvalid'])
	tmpl.write(data['rightgripvalid'])
	# Particle spawning on attack order ( for weapon characters )
	tmpl.write(data['particleattachweapon'])
	tmpl.write(data['particletypeweapon'])
	# Particle spawning for GoPoof
	tmpl.write(data['numparticlespoof'])
	tmpl.write(data['facingadd'])
	tmpl.write(data['particletypepoof'])
	# Particle spawning for Blud ( If you want it, you put it in... )
	tmpl.write(data['bludvalid'])
	tmpl.write(data['particletypeblud'])
	# Extra stuff I forgot
	tmpl.write(data['waterwalk'])
	tmpl.write(data['bounciness'])
	# More stuff I forgot
	tmpl.write(data['lifeheal'])
	tmpl.write(data['manacost'])
	tmpl.write(data['lifereturn'])
	tmpl.write(data['stoppedby'])
	tmpl.write(EgoObj.addUnderlines(data['skinname0']))
	tmpl.write(EgoObj.addUnderlines(data['skinname1']))
	tmpl.write(EgoObj.addUnderlines(data['skinname2']))
	tmpl.write(EgoObj.addUnderlines(data['skinname3']))
	tmpl.write(data['skincost0'])
	tmpl.write(data['skincost1'])
	tmpl.write(data['skincost2'])
	tmpl.write(data['skincost3'])
	tmpl.write(data['strengthdampen'])
	# Another memory lapse
	tmpl.write(data['ridernoattack'])
	tmpl.write(data['canbedazed'])
	tmpl.write(data['canbegrogged'])
	tmpl.write(data['permanentlife'])
	tmpl.write(data['permanentmana'])
	tmpl.write(data['canseeinvisible'])
	tmpl.write(data['cursechance'])
	tmpl.write(data['footfallsound'])
	tmpl.write(data['jumpsound'])
	tmpl.writeEnd()
	if 'DRES' in data['expansions']:
	    for i in range(0, 3):
		if data['expansions']['DRES'] & 1 << i == 1 << i:
		    fh.write(':[DRES] %d\n' % i)
	for key in data['expansions']:
	    if key == 'DRES':
		continue
	    if key == 'LIFE' or key == 'MANA':
		string0 = str(data['expansions'][key]).rstrip('0')
		if string0[-1] == '.':
		    string0 += '0'
		fh.write(':[%s] %s\n' % (key, string0))
	    else:
		fh.write(':[%s] %d\n' % (key, data['expansions'][key]))
    
    def loadPanel(self, parent=None):
	self.panel = panel = scrolled.ScrolledPanel(parent)
	sizer = wx.FlexGridSizer(0, 2, 10, 25)
	EgoObj.addIntControl(panel, sizer, "Object slot number",
	    lambda v,p,c: self.change('slot', v), maxx=512, 
	    initial=self.data['slot'])
	EgoObj.addStringControl(panel, sizer, "Object class name",
	    lambda v,p,c: self.change('class', v), self.data['class'])
	EgoObj.addBoolControl(panel, sizer, "Object is lit uniformly",
	    lambda v,p,c: self.change('uniformlylit', v),
	    self.data['uniformlylit'])
	EgoObj.addIntControl(panel, sizer, "Maximum ammo", 
	    lambda v,p,c: self.change('maxammo', v), maxx=255,
	    initial=self.data['maxammo'])
	EgoObj.addIntControl(panel, sizer, "Current ammo", 
	    lambda v,p,c: self.change('currammo', v), maxx=255,
	    initial=self.data['currammo'])
	EgoObj.addChoiceControl(panel, sizer, "Gender", 
	    lambda v,p,c: self.change('gender', v.upper()), genders,
	    genders.index(self.data['gender'].lower()))
	
	EgoObj.addChoiceControl(panel, sizer, "Life bar color", 
	    lambda v,p,c: self.change('lifecolor', barColors.index(v)),
	    barColors, self.data['lifecolor'])
	EgoObj.addChoiceControl(panel, sizer, "Mana bar color", 
	    lambda v,p,c: self.change('manacolor', barColors.index(v)),
	    barColors, self.data['manacolor'])
	EgoObj.addPairControl(panel, sizer, "Life points",
	    lambda v,p,c: self.change('lifepoints', v, p, True, c),
	    self.data['lifepoints'])
	EgoObj.addPairControl(panel, sizer, "Life increase per level",
	    lambda v,p,c: self.change('lifepointsperlevel', v, p, True, c),
	    self.data['lifepointsperlevel'])
	EgoObj.addPairControl(panel, sizer, "Mana points",
	    lambda v,p,c: self.change('manapoints', v, p, True, c),
	    self.data['manapoints'])
	EgoObj.addPairControl(panel, sizer, "Mana increase per level",
	    lambda v,p,c: self.change('manapointsperlevel', v, p, True, c),
	    self.data['manapointsperlevel'])
	EgoObj.addPairControl(panel, sizer, "Mana return",
	    lambda v,p,c: self.change('manareturn', v, p, True, c),
	    self.data['manareturn'])
	EgoObj.addPairControl(panel, sizer, "Mana return increase per level",
	    lambda v,p,c: self.change('manareturnperlevel', v, p, True, c),
	    self.data['manareturnperlevel'])
	EgoObj.addPairControl(panel, sizer, "Mana flow",
	    lambda v,p,c: self.change('manaflow', v, p, True, c),
	    self.data['manaflow'])
	EgoObj.addPairControl(panel, sizer, "Mana flow increase per level",
	    lambda v,p,c: self.change('manaflowperlevel', v, p, True, c),
	    self.data['manaflowperlevel'])
	EgoObj.addPairControl(panel, sizer, "Strength",
	    lambda v,p,c: self.change('strength', v, p, True, c),
	    self.data['strength'])
	EgoObj.addPairControl(panel, sizer, "Strength increase per level",
	    lambda v,p,c: self.change('strengthperlevel', v, p, True, c),
	    self.data['strengthperlevel'])
	EgoObj.addPairControl(panel, sizer, "Wisdom",
	    lambda v,p,c: self.change('wisdom', v, p, True, c),
	    self.data['wisdom'])
	EgoObj.addPairControl(panel, sizer, "Wisdom increase per level",
	    lambda v,p,c: self.change('wisdomperlevel', v, p, True, c),
	    self.data['wisdomperlevel'])
	EgoObj.addPairControl(panel, sizer, "Intelligence",
	    lambda v,p,c: self.change('intelligence', v, p, True, c),
	    self.data['intelligence'])
	EgoObj.addPairControl(panel, sizer, "Intelligence increase per level",
	    lambda v,p,c: self.change('intelligenceperlevel', v, p, True, c),
	    self.data['intelligenceperlevel'])
	EgoObj.addPairControl(panel, sizer, "Dexterity",
	    lambda v,p,c: self.change('dexterity', v, p, True, c),
	    self.data['dexterity'])
	EgoObj.addPairControl(panel, sizer, "Dexterity increase per level",
	    lambda v,p,c: self.change('dexterityperlevel', v, p, True, c),
	    self.data['dexterityperlevel'])
	
	EgoObj.addFloatControl(panel, sizer, "Size",
	    lambda v,p,c: self.change('size', v, -1, True, c), self.data['size'])
	EgoObj.addFloatControl(panel, sizer, "Size increase per level",
	    lambda v,p,c: self.change('sizeperlevel', v, -1, True, c),
	    self.data['sizeperlevel'])
	EgoObj.addIntControl(panel, sizer, "Shadow size", 
	    lambda v,p,c: self.change('shadowsize', v), maxx=1000000000,
	    initial=self.data['shadowsize'])
	EgoObj.addIntControl(panel, sizer, "Bump size", 
	    lambda v,p,c: self.change('bumpsize', v), maxx=1000000000,
	    initial=self.data['bumpsize'])
	EgoObj.addIntControl(panel, sizer, "Bump height", 
	    lambda v,p,c: self.change('bumpheight', v), maxx=1000000000,
	    initial=self.data['bumpheight'])
	EgoObj.addFloatControl(panel, sizer, "Bump dampen",
	    lambda v,p,c: self.change('bumpdampen', v, -1, True, c),
	    self.data['bumpdampen'])
	EgoObj.addIntControl(panel, sizer, "Weight", 
	    lambda v,p,c: self.change('weight', v), maxx=255,
	    initial=self.data['weight'])
	EgoObj.addFloatControl(panel, sizer, "Jump power",
	    lambda v,p,c: self.change('jumppower', v, -1, True, c),
	    self.data['jumppower'])
	EgoObj.addIntControl(panel, sizer, "Number of jumps", 
	    lambda v,p,c: self.change('numjumps', v), maxx=4,
	    initial=self.data['numjumps'])
	EgoObj.addIntControl(panel, sizer, "Sneak Speed", 
	    lambda v,p,c: self.change('sneakspeed', v), maxx=1000000000,
	    initial=self.data['sneakspeed'])
	EgoObj.addIntControl(panel, sizer, "Walk Speed", 
	    lambda v,p,c: self.change('walkspeed', v), maxx=1000000000,
	    initial=self.data['walkspeed'])
	EgoObj.addIntControl(panel, sizer, "Run Speed", 
	    lambda v,p,c: self.change('runspeed', v), maxx=1000000000,
	    initial=self.data['runspeed'])
	EgoObj.addIntControl(panel, sizer, "Fly to height", 
	    lambda v,p,c: self.change('flytoheight', v), maxx=1000000000,
	    initial=self.data['flytoheight'])
	EgoObj.addIntControl(panel, sizer, "Flashing AND", 
	    lambda v,p,c: self.change('flashingand', v), minn=1, maxx=255,
	    initial=self.data['flashingand'])
	EgoObj.addIntControl(panel, sizer, "Transparency blending", 
	    lambda v,p,c: self.change('transparencyblending', v), maxx=255,
	    initial=self.data['transparencyblending'])
	EgoObj.addIntControl(panel, sizer, "Light blending", 
	    lambda v,p,c: self.change('lightblending', v), maxx=255,
	    initial=self.data['lightblending'])
	EgoObj.addBoolControl(panel, sizer, "Transfer blending to weapons",
	    lambda v,p,c: self.change('transferblendingtoweapons', v),
	    self.data['transferblendingtoweapons'])
	EgoObj.addIntControl(panel, sizer, "Sheen", 
	    lambda v,p,c: self.change('sheen', v), minn=0, maxx=15,
	    initial=self.data['sheen'])
	EgoObj.addBoolControl(panel, sizer, "Phong mapping",
	    lambda v,p,c: self.change('phongmapping', v),
	    self.data['phongmapping'])
	EgoObj.addFloatControl(panel, sizer, "Texture X movement rate",
	    lambda v,p,c: self.change('texturexmovement', v, -1, True, c),
	    self.data['texturexmovement'])
	EgoObj.addFloatControl(panel, sizer, "Texture Y movement rate",
	    lambda v,p,c: self.change('textureymovement', v, -1, True, c),
	    self.data['textureymovement'])
	EgoObj.addBoolControl(panel, sizer, "Conform to hills like a chair",
	    lambda v,p,c: self.change('conformhillschair', v),
	    self.data['conformhillschair'])
	
	EgoObj.addBoolControl(panel, sizer, "Object is totally invincible",
	    lambda v,p,c: self.change('isinvincible', v),
	    self.data['isinvincible'])
	EgoObj.addIntControl(panel, sizer, "NonIFrame Invulnerability facing", 
	    lambda v,p,c: self.change('noniframefacing', v), minn=0, maxx=65535,
	    initial=self.data['noniframefacing'])
	EgoObj.addIntControl(panel, sizer, "NonIFrame Invulnerability angle", 
	    lambda v,p,c: self.change('notiframeangle', v), minn=0, maxx=32768,
	    initial=self.data['noniframeangle'])
	EgoObj.addIntControl(panel, sizer, "IFrame Invulnerability facing", 
	    lambda v,p,c: self.change('iframefacing', v), minn=0, maxx=65535,
	    initial=self.data['iframefacing'])
	EgoObj.addIntControl(panel, sizer, "IFrame Invulnerability angle", 
	    lambda v,p,c: self.change('iframeangle', v), minn=0, maxx=32768,
	    initial=self.data['iframeangle'])
	
	panel.SetSizer(sizer)
	panel.SetAutoLayout(1)
	panel.SetupScrolling()
    
    def getPanel(self, parent=None):
	if self.panel is None:
	    self.loadPanel(parent)
	return self.panel
    
    def change(self, key, value, pair=-1, doFloat=False, textCtrl=None):
	if doFloat or pair != -1:
	    if pair != -1:
		self.changePair(key, pair, value, textCtrl)
	    else:
		self.changeFloat(key, value, textCtrl)
	    return
	print "Changing %s from %s to %s in change" % (key, self.data[key], value)
	self.data[key] = value
	self.edited = True
    
    def changePair(self, key, part, value, textCtrl):
	print "Changing %s[%d] from %s to %s in changePair..." % \
		(key, part, self.data[key][part], value),
	try:
	    self.data[key][part] = float(value)
	    print "ok"
	    self.edited = True
	except ValueError:
	    if value == '':
		self.data[key][part] = 0
		print "ok"
		self.edited = True
	    else:
		print "failed!"
		textCtrl.ChangeValue(str(self.data[key][part]))
    
    def changeFloat(self, key, value, textCtrl):
	print "Changing %s from %s to %s in changeFloat..." % (key, self.data[key], value),
	try:
	    self.data[key] = float(value)
	    print "ok"
	    self.edited = True
	except ValueError:
	    if value == '':
		self.data[key] = 0
		print "ok"
		self.edited = True
	    else:
		print "failed!"
		textCtrl.ChangeValue(str(self.data[key]))
    
    def wasEdited(self):
	return self.edited