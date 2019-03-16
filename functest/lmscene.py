import os
import lightmetrica as lm

def scenes():
    return [
        'fireplace_room',
        'cornell_box_sphere',
        'bedroom',
        'vokselia_spawn',
        'breakfast_room',
        'buddha',
        'bunny',
        'cloud',
        'conference',
        'cube',
        'powerplant'
    ]

def load(scene_path, name):
    return globals()[name](scene_path)

def fireplace_room(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [5.101118, 1.083746, -2.756308],
        'center': [4.167568, 1.078925, -2.397892],
        'up': [0,1,0],
        'vfov': 43.001194
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'fireplace_room/fireplace_room.obj'),
        'loader': 'objloader::tinyobjloader'
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

def bedroom(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [22.634083, -19.868534, 24.155586],
        'center': [22.027370, -19.116102, 23.899178],
        'up': [0,0,1],
        'vfov': 57.978600
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'bedroom/iscv2.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def vokselia_spawn(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [1.525444, 0.983225, 0.731648],
        'center': [0.780862, 0.377208, 0.451751],
        'up': [0,1,0],
        'vfov': 49.857803
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'vokselia_spawn/vokselia_spawn.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def breakfast_room(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [0.195303, 2.751188, 7.619322],
        'center': [0.139881, 2.681201, 6.623315],
        'up': [0,1,0],
        'vfov': 39.384486
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'breakfast_room/breakfast_room.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def buddha(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [0.027255, 0.941126, -2.217943],
        'center': [0.001645, 0.631268, -1.267505],
        'up': [0,1,0],
        'vfov': 19.297509
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'buddha/buddha.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def bunny(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [-0.191925, 2.961061, 4.171464],
        'center': [-0.185709, 2.478091, 3.295850],
        'up': [0,1,0],
        'vfov': 28.841546
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'bunny/bunny.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def cloud(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [22.288721, 32.486145, 85.542023],
        'center': [22.218456, 32.084743, 84.628822],
        'up': [0,1,0],
        'vfov': 53.023230
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'cloud/altostratus00.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def conference(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [926.391602, 329.094299, 141.714554],
        'center': [925.628784, 328.858490, 141.112488],
        'up': [0,1,0],
        'vfov': 50.717898
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'conference/conference.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def cube(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [2, 2, 2],
        'center': [0, 0, 0],
        'up': [0,1,0],
        'vfov': 30
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'cube/cube.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })

def powerplant(scene_path):
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [-200000, 200000, 200000],
        'center': [0, 0, 0],
        'up': [0,1,0],
        'vfov': 30
    })
    lm.asset('model_obj', 'model::wavefrontobj', {
        'path': os.path.join(scene_path, 'powerplant/powerplant.obj')
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })
    lm.primitive(lm.identity(), {
        'model': lm.asset('model_obj')
    })