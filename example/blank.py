"""Rendering blank image

Example:
$ PYTHONPATH="../;../build/bin/Release" python blank.py
"""

# _begin_example
import lightmetrica as lm
import argparse

def run(**kwargs):
    # Initialize the framework
    lm.init('user::default', {})

    # Define assets
    # Film for the rendered image
    lm.asset('film', 'film::bitmap', {
        'w': kwargs['width'],
        'h': kwargs['height']
    })

    # Render an image
    lm.render('renderer::blank', {
        'output': 'film',
        'color': [1,1,1]
    })

    # Save rendered image
    lm.save('film', kwargs['out'])

    # Shutdown the framework
    lm.shutdown()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=str, required=True)
    parser.add_argument('--width', type=int, required=True)
    parser.add_argument('--height', type=int, required=True)
    args = parser.parse_args()

    run(**vars(args))
# _end_example