
import sys

import pyglet
from pyglet.gl import *
ScreenWidth = 800
ScreenHeight = 600
View = 10

class TerrainMap:
    def SetDimmensions(self, w, h):
        self.Map = [1] * w * h
        self.Width = w
        self.Height = h   
        
    def SetTileSet(self, tileSet):
        self.TileSet = tileSet                                                                     
                
        
class Tile:
    def __init__(self, filename, w, h):
        self.Image = pyglet.image.load(filename).get_texture(rectangle=True)

        #self.Sprite = sf.Sprite(self.Image) 
        #self.Sprite.SetBlendMode(sf.Blend.Alpha)               
        #self.Sprite.SetCenter(w//2, h - 40)   

        self.Width = w
        self.Height = h    
        

def TileToScreen(x, y):
    return ((y-x)*80, (x+y)*40)

def ScreenToTile(x, y):
    return (int(round(y/80.0 - x/160.0)), int(round(x/160.0+y/80.0)))              
    
def DrawMap(map, camX, camY): 
    (tcamX, tcamY) = ScreenToTile(camX, camY)
    (centerX, centerY) = TileToScreen(tcamX, tcamY)
    (offX, offY) = (centerX - camX, centerY - camY)        
    for i in range(0, View*2+1):
        for j in range(0,View+1-(i % 2)):
                            
            tileX = tcamX + i//2 - j
            tileY = tcamY + j + i//2 + (i % 2) - View
            
            if tileX > 0 and tileY > 0 and tileX < map.Width and tileY < map.Height:                
                tile = map.TileSet[map.Map[map.Width * tileY + tileX]]
                
                (px, py) = TileToScreen(tileX - tcamX, tileY - tcamY)


                tile.Image.blit(offX + px+ScreenWidth//2, offY + py+ScreenHeight//2, 0)

               # tile.Sprite.SetPosition(, )                                                                                                
                

#window = sf.RenderWindow(sf.VideoMode(ScreenWidth, ScreenHeight), "Commander Luke")
#window.SetFramerateLimit(25)
        
tileSet = []
tileSet.append(Tile('data/tiles/template.png', 160, 80))
tileSet.append(Tile('data/tiles/grass1.png', 160, 80))

# img = 
# img.anchor_x = img.width // 2
# img.anchor_y = img.height // 2

terrainMap = TerrainMap()
terrainMap.SetDimmensions(100, 100)
terrainMap.SetTileSet(tileSet)
terrainMap.Map[11 * terrainMap.Width + 10] = 1
terrainMap.Map[11 * terrainMap.Width + 11] = 1
terrainMap.Map[12 * terrainMap.Width + 10] = 1
terrainMap.Map[12 * terrainMap.Width + 11] = 1


camX = 0; camY = 1000
camDx = 0; camDy = 0

#running = True
#while running:
#    event = sf.Event()    
#    while window.GetEvent(event):
#        if event.Type == sf.Event.Closed:
#            running = False
#        
#        if event.Type == sf.Event.KeyPressed and event.Key.Code == sf.Key.Right:
#            camDx += (60.0 - camDx) * 0.01            
#        if event.Type == sf.Event.KeyPressed and event.Key.Code == sf.Key.Left:
#            camDx += (-60.0 - camDx) * 0.01            
#        if event.Type == sf.Event.KeyPressed and event.Key.Code == sf.Key.Down:
#            camDy += (60.0 - camDy) * 0.01            
#        if event.Type == sf.Event.KeyPressed and event.Key.Code == sf.Key.Up:
#            camDy += (-60.0 - camDy) * 0.01  
#            
#        if event.Type == sf.Event.MouseButtonPressed:
#            (tx, ty) = ScreenToTile(camX - 400 + window.GetInput().GetMouseX(), camY - 300 + window.GetInput().GetMouseY())
#            terrainMap.Map[ty * terrainMap.Width + tx] = 1
#            print("ok")
#                  
#            
#    window.Clear(sf.Color.Blue)
#    
#    camX += camDx; camY += camDy        
#   
#    camDx *= 0.9; camDy *= 0.9
#    window.Draw(text)    
#
#    window.Display()


import sys

import pyglet
from pyglet.gl import *

window = pyglet.window.Window(visible=False, resizable=True)

@window.event
def on_draw():
    #background.blit_tiled(0, 0, 0, window.width, window.height)
    DrawMap(terrainMap, camX, camY)

        

if __name__ == '__main__':

#    filename = sys.argv[1]


#    checks = pyglet.image.create(32, 32, pyglet.image.CheckerImagePattern())
#    background = pyglet.image.TileableTexture.create_for_image(checks)

    # Enable alpha blending, required for image.blit.
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    window.width = ScreenWidth 
    window.height = ScreenHeight
    window.set_visible()

    pyglet.app.run()

