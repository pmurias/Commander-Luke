# script for automated rendering of isometric sprites and animations
# - set the number of directions you want to render sprite from at the 'directions' variable
# - by default, script renders all availible actions. if you want to render only some of the actions
#   put the actions names at the 'this_actions_only' list
# - sprites will be saved at subdirectory of .blend file directory named after Mesh object name. 
#   each animation will have its own prefix based on action name, i.e. //MeshName/MeshName.AnimNameframeNo.png
# - each of the selected lights will gain 'AutoTrack' constraint, to track selected model
# - due lack of proper function in API, lights should be manually parented to camera
# - current camera will track the model too

import Blender
from Blender.Camera import *
from Blender.Curve import *
from Blender.Ipo import *
from Blender.Scene import *
from Blender.Armature import NLA
from math import *

this_actions_only = []
directions = 16

#-------------------------------------------------------------------------------------		
scene = Blender.Scene.GetCurrent()
cam = scene.getCurrentCamera()
rend = scene.getRenderingContext()	

#-------------------------------------------------------------------------------------		
def set_cam_ipo(animLen):
	camipo = Blender.Ipo.New('Object','camipo')
	xcurve = camipo.addCurve('LocX')
	ycurve = camipo.addCurve('LocY')

	xcurve.append((0, calc_cam_x(0)))
	ycurve.append((0, calc_cam_y(0)))
	for i in range(0, directions):
		xcurve.append((i*animLen+1, calc_cam_x(i)))
		ycurve.append((i*animLen+1, calc_cam_y(i)))

	xcurve.interpolation = Blender.IpoCurve.InterpTypes.CONST
	xcurve.recalc()
	ycurve.interpolation = Blender.IpoCurve.InterpTypes.CONST
	ycurve.recalc()

	cam.setIpo(camipo)


#-------------------------------------------------------------------------------------		
def duplicate_anim(action):
	frameNumbers = action.getFrameNumbers()
	animLen = frameNumbers[-1]
	allAnimIpos = action.getAllChannelIpos()
	for ch, ipo in allAnimIpos.iteritems():
		for curve in ipo.curves:
			for i in range(1, directions):
				for frame in frameNumbers:
					curve.append((float(i) * float(animLen) + float(frame), curve[frame]))
	return animLen

#-------------------------------------------------------------------------------------		
def clear_anim(action, origLen):
	allAnimIpos = action.getAllChannelIpos()
	for ch, ipo in allAnimIpos.iteritems():
		for curve in ipo.curves:
			delPts = []
			keyframei = 0
			for keyframe in curve.bezierPoints:
				if keyframe.pt[0] > origLen:
					delPts.insert(0, keyframei)				
				keyframei += 1
			for delPt in delPts:
				curve.delBezier(delPt)

#-------------------------------------------------------------------------------------									
def do_rendering(len, name):
	rend.endFrame(len*directions)
	
	set_cam_ipo(len)
	
	scene.render.setRenderPath('//'+obName+'/'+obName+'.'+name+'###')
	scene.render.renderAnim()	
	
#-------------------------------------------------------------------------------------		
def set_autotrack_constr(selOb, obj):
	autotrack = None
	for constr in obj.constraints:
		if constr.name == 'AutoTrack':
			autotrack = constr
			break	
		
	if not autotrack:		
		autotrack = obj.constraints.append(Blender.Constraint.Type.TRACKTO)
		autotrack[Blender.Constraint.Settings.TRACK] = Blender.Constraint.Settings.TRACKNEGZ
		autotrack[Blender.Constraint.Settings.UP] = Blender.Constraint.Settings.UPY
		autotrack.name = 'AutoTrack'
			
	autotrack[Blender.Constraint.Settings.TARGET] = selOb
		
	
	
#--------------------------main-------------------------------------------------------			
if Blender.Object.GetSelected() != []:
	lights = []
	selOb = None
	for obj in Blender.Object.GetSelected():
		if obj.getType() == 'Lamp':
			lights.append(obj)
		elif obj.getType() == 'Mesh':
			selOb = obj
			
	for light in lights:
		print 'LIGHT!'
		set_autotrack_constr(selOb, light)
		
	set_autotrack_constr(selOb, cam)
	cam.LocZ = selOb.LocZ + 6.0
	
	rend.startFrame(1)
			
	if selOb and selOb.getType() == 'Mesh':
		obName = selOb.getName()
		
		armature = None
		for mod in selOb.modifiers:
			if mod.type == Blender.Modifier.Type.ARMATURE:
				armature = mod[Blender.Modifier.Settings.OBJECT]

		xoff = selOb.LocX
		yoff = selOb.LocY

		def calc_cam_x(i):
  			return xoff + 10.392304845*cos((float(i)/(directions))*2*pi - .5*pi)
		def calc_cam_y(i):
			return yoff + 10.392304845*sin((float(i)/(directions))*2*pi - .5*pi)

		if armature:
			for animName in NLA.GetActions():
				if this_actions_only != [] and not animName in this_actions_only:
					continue					
				action = NLA.GetActions()[animName]
				action.setActive(armature)
				animLen = duplicate_anim(action)
				
				do_rendering(animLen, animName)				
										
				clear_anim(action, animLen)
		else:
			do_rendering(1, obName)		
		