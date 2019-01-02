"""Example: Rendering blank image"""

# _begin_example
import pylm as lm

# Initialize the framework
lm.init()

# Define assets
# Film for the rendered image
lm.asset('film', 'film::bitmap', {'w': 1920, 'h': 1080})

# Render an image
lm.render('renderer::blank', {
    'output': 'film',
    'color': [1,1,1]
})

# Save rendered image
lm.save('film', 'blank.pfm')
# _end_example