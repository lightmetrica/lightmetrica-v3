import os
import lightmetrica as lm

def scenes():
    return [
        'fireplace_room',
        'cornell_box_sphere'
    ]

def load(scene_path, name):
    return globals()[name](scene_path)

def bedroom(scene_path):
    pass

def fireplace_room(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [5.101118, 1.083746, -2.756308],
        'center': [4.167568, 1.078925, -2.397892],
        'up': [0,1,0],
        'vfov': 43.001194
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'fireplace_room/fireplace_room.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def cornell_box_sphere(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [0,1,5],
        'center': [0,1,0],
        'up': [0,1,0],
        'vfov': 30
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'cornell_box/CornellBox-Sphere.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })