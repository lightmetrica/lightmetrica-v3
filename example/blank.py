"""Rendering blank image

Example:
$ PYTHONPATH="../;../build/bin/Release" python blank.py
"""

# _begin_example
import lightmetrica as lm

def run():
    # Initialize the framework
    lm.init('user::default', {})

    # Define assets
    # Film for the rendered image
    lm.asset('film', 'film::bitmap', {'w': 1920, 'h': 1080})

    # Render an image
    lm.render('renderer::blank', {
        'output': 'film',
        'color': [1,1,1]
    })

    # Save rendered image
    lm.save('film', 'blank_py.pfm')

if __name__ == '__main__':
    run()
# _end_example