## An Egoboo Object

import wx
import os
import ObjBaker
import ObjBaker._EgoObject as EgoObj

class EgoObjData:
    
    barColors = ["empty", "red", "yellow", "green", "blue", "purple"]
    
    def __init__(self, dirName):
	self.panel = None
	self.loadPanel()
	fileName = os.path.join(dirName, 'data.txt')
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
	# TODO: Skin defenses
	#data['skindef'] = EgoObj.getSkinDef(fh)
	for i in range(0,18): #Advance past the skin defenses
	    EgoObj.gotoColon(fh)
	# Experience and level data
	data['expforlvl1'] = EgoObj.getInt(fh)
	data['expforlvl2'] = EgoObj.getInt(fh)
	data['expforlvl3'] = EgoObj.getInt(fh)
	data['expforlvl4'] = EgoObj.getInt(fh)
	data['expforlvl5'] = EgoObj.getInt(fh)
	data['currexp'] = EgoObj.getInt(fh)
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
	## TODO!!!
	# end of __init__
	self.data = data
    
    def loadPanel(self):
	pass