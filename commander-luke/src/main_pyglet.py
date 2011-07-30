
import sys


import pyglet
from pyglet.gl import *
from pyglet.window import key
#ScreenWidth = 1024
#ScreenHeight = 800
View = 20

pyglet.resource.path.append('../data/tiles')
pyglet.resource.reindex();



class TerrainMap:
    def __init__(self,width,height,tileSet):
        self.tileSet = tileSet
        self.width = width
        self.height = height
        self.map = [1] * width * height

class Tile:
    def __init__(self, filename, w, h):
        self.Image = pyglet.resource.image(filename) #pyglet.image.load(filename).get_texture(rectangle=True)

        #self.Sprite = sf.Sprite(self.Image) 
        #self.Sprite.SetBlendMode(sf.Blend.Alpha)               
        #self.Sprite.SetCenter(w//2, h - 40)   

        self.width = w
        self.height = h    
        

def TileToScreen(x, y):
    return ((y-x)*80, (x+y)*40)

def ScreenToTile(x, y):
    return (int(round(y/80.0 - x/160.0)), int(round(x/160.0+y/80.0)))              
    
def draw_map(map, batch, camera): 
    sprites = []
    (tcamX, tcamY) = ScreenToTile(camera.x, camera.y)
    (centerX, centerY) = TileToScreen(tcamX, tcamY)
    (offX, offY) = (centerX - camera.x, centerY - camera.y)        
    for i in range(0, View*2+1):
        for j in range(0,View+1-(i % 2)):
                            
            tileX = tcamX + i//2 - j
            tileY = tcamY + j + i//2 + (i % 2) - View
            
            if tileX > 0 and tileY > 0 and tileX < map.width and tileY < map.height:                
                tile = map.tileSet[map.map[map.width * tileY + tileX]]
                
                (px, py) = TileToScreen(tileX - tcamX, tileY - tcamY)

                sprite = pyglet.sprite.Sprite(tile.Image,batch=batch)
                sprite.x = offX + px+ScreenWidth//2
                sprite.y = offY + py+ScreenHeight//2
                sprites.append(sprite)
    return sprites

class Camera():
    def __init__(self,x=0,y=0,dX=0,dY=0):
        self.x = x
        self.y = y
        self.dX = dX
        self.dY = dY

        
tileSet = []
tileSet.append(Tile('template.png', 160, 80))
tileSet.append(Tile('grass1.png', 160, 80))


terrainMap = TerrainMap(100,100,tileSet)

#terrainMap.Map[11 * terrainMap.Width + 10] = 1
#terrainMap.Map[11 * terrainMap.Width + 11] = 1
#terrainMap.Map[12 * terrainMap.Width + 10] = 1
#terrainMap.Map[12 * terrainMap.Width + 11] = 1


camera = Camera()

#            
#        if event.Type == sf.Event.MouseButtonPressed:
#            (tx, ty) = ScreenToTile(camX - 400 + window.GetInput().GetMouseX(), camY - 300 + window.GetInput().GetMouseY())
#            terrainMap.Map[ty * terrainMap.Width + tx] = 1
#            print("ok")
#                  
#            


import sys

import pyglet
from pyglet.gl import *

window = pyglet.window.Window(visible=False, fullscreen=True)


@window.event
def on_key_press(symbol,modifiers):
    if symbol == key.X:
        exit()

keys = key.KeyStateHandler()
window.push_handlers(keys)

tiles_batch = pyglet.graphics.Batch()

fps_display = pyglet.clock.ClockDisplay()
mapSprites = []

@window.event
def on_draw():
    global mapSprites
    window.clear()

    for sprite in mapSprites:
        sprite.delete()

    mapSprites = draw_map(terrainMap,tiles_batch,camera)
    tiles_batch.draw()
    fps_display.draw()


def update_camera(dt):
    camera.x += camera.dX
    camera.y += camera.dY        

    if keys[key.RIGHT]:
        camera.dX += (60.0 - camera.dX) * 0.01            
    if keys[key.LEFT]:
        camera.dX += (-60.0 - camera.dX) * 0.01            

    if keys[key.DOWN]:
        camera.dY += (-60.0 - camera.dY) * 0.01  

    if keys[key.UP]:
        camera.dY += (60.0 - camera.dY) * 0.01            

    camera.dX *= 0.9
    camera.dY *= 0.9


pyglet.clock.schedule_interval(update_camera, 1/25.)


window.set_visible()

pyglet.app.run()

