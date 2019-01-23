"""Rendering blank image

Example:
$ PYTHONPATH="../;../build/bin/Release" python blank.py
"""

# _begin_import
import lightmetrica as lm
# _end_import
import argparse

def run(**kwargs):
    # _begin_init
    # Initialize the framework
    lm.init('user::default', {})
    # _end_init

    # _begin_assets
    # Define assets
    # Film for the rendered image
    lm.asset('film', 'film::bitmap', {
        'w': kwargs['width'],
        'h': kwargs['height']
    })
    # _end_assets

    # _begin_render
    # Render an image
    lm.render('renderer::blank', {
        'output': 'film',
        'color': [1,1,1]
    })
    # _end_render

    # _begin_save
    # Save rendered image
    lm.save('film', kwargs['out'])
    # _end_save

    # _begin_shutdown
    # Shutdown the framework
    lm.shutdown()
    # _end_shutdown

if __name__ == '__main__':
    # _begin_parse_command_line
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=str, required=True)
    parser.add_argument('--width', type=int, required=True)
    parser.add_argument('--height', type=int, required=True)
    args = parser.parse_args()
    # _end_parse_command_line

    run(**vars(args))