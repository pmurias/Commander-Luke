import pyglet
from pyglet.window import key


window = pyglet.window.Window(visible=False, fullscreen=True)
View = 13


class TerrainMap:
    def __init__(self,width,height,tileSet):
        self.tileSet = tileSet
        self.width = width
        self.height = height
        self.map = [0] * width * height

class Tile:
    def __init__(self, filename, w, h):
        self.Image = pyglet.resource.image(filename) 
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
                sprite.x = offX + px+screenWidth//2 - 80
                sprite.y = offY + py+screenHeight//2 - 40
                sprites.append(sprite)
    return sprites

class Camera():
    def __init__(self,x=0,y=0,dX=0,dY=0):
        self.x = x
        self.y = y
        self.dX = dX
        self.dY = dY
    

@window.event
def on_key_press(symbol,modifiers):
    if symbol == key.X:
        exit()
        
@window.event
def on_mouse_press(x, y, button, modifiers):
    (tx, ty) = ScreenToTile(camera.x - screenWidth//2 + x, camera.y - screenHeight//2 + y)
    terrainMap.map[ty * terrainMap.width + tx] = 1
    

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
    
    
pyglet.resource.path.append('../data/tiles')
pyglet.resource.reindex();

tileSet = []
tileSet.append(Tile('template.png', 160, 80))
tileSet.append(Tile('grass1.png', 160, 80))


terrainMap = TerrainMap(100,100,tileSet)

camera = Camera()
       

screenWidth = window.width
screenHeight = window.height


keys = key.KeyStateHandler()
window.push_handlers(keys)

tiles_batch = pyglet.graphics.Batch()

fps_display = pyglet.clock.ClockDisplay()
mapSprites = []


pyglet.clock.schedule_interval(update_camera, 1/25.)


window.set_visible()

pyglet.app.run()

