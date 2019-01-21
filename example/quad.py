"""Rendering quad"""

import lightmetrica as lm
import argparse

def run(**kwargs):
    # Initialize the framework
    # _begin_init
    lm.init('user::default', {
        'numThreads': -1
    })
    # _end_init

    # Define assets
    # _begin_assets
    # Film for the rendered image
    lm.asset('film1', 'film::bitmap', {
        'w': kwargs['width'],
        'h': kwargs['height']
    })

    # Pinhole camera
    lm.asset('camera1', 'camera::pinhole', {
        'film': 'film1',
        'position': [0,0,5],
        'center': [0,0,0],
        'up': [0,1,0],
        'vfov': 30
    })

    # Load mesh with raw vertex data
    lm.asset('mesh1', 'mesh::raw', {
        'ps': [-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1],
        'ns': [0,0,1],
        'ts': [0,0,1,0,1,1,0,1],
        'fs': {
            'p': [0,1,2,0,2,3],
            'n': [0,0,0,0,0,0],
            't': [0,1,2,0,2,3]
        }
    })

    # Material
    lm.asset('material1', 'material::diffuse', {
        'Kd': [1,1,1]
    })
    # _end_assets

    # Define scene primitives
    # _begin_primitive
    # Camera
    lm.primitive(lm.identity(), {'camera': 'camera1'})

    # Mesh
    lm.primitive(lm.identity(), {
        'mesh': 'mesh1',
        'material': 'material1'
    })
    # _end_primitive

    # Render an image
    # _begin_render
    lm.build('accel::sahbvh', {})
    lm.render('renderer::raycast', {
        'output': 'film1',
        'color': [0,0,0]
    })
    # _end_render

    # Save rendered image
    lm.save('film1', kwargs['out'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=str, required=True)
    parser.add_argument('--width', type=int, required=True)
    parser.add_argument('--height', type=int, required=True)
    args = parser.parse_args()

    run(**vars(args))