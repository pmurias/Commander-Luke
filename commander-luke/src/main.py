# Include the PySFML extension
from PySFML import *

# Create the main window
window = sf.RenderWindow(sf.VideoMode(800, 600), "Commander Luke")

# Create a graphical string to display
text = sf.String("Game Over")

# Start the game loop
running = True
while running:
    event = sf.Event()
    while window.GetEvent(event):
        if event.Type == sf.Event.Closed:
            running = False

    # Draw the text, and update the window
    window.Draw(text)
    window.Display()
