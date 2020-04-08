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

def scenes_small():
    return [
        'fireplace_room',
        'cornell_box_sphere',
        'cube',
    ]

def load(scene, scene_path, name):
    return globals()[name](scene, scene_path)

def plane_emitter(scene, scene_path):
    """
    A scene containing only a single area light.
    The scene is useful to test the most simpelst configuration.
    """
    camera = lm.load_camera('camera', 'pinhole', {
        'position': [0,0,5],
        'center': [0,0,0],
        'up': [0,1,0],
        'vfov': 30,
        'aspect': 16/9
    })
    mesh = lm.load_mesh('mesh', 'raw', {
        'ps': [-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1],
        'ns': [0,0,1],
        'ts': [0,0,1,0,1,1,0,1],
        'fs': {
            'p': [0,1,2,0,2,3],
            'n': [0,0,0,0,0,0],
            't': [0,1,2,0,2,3]
        }
    })
    material = lm.load_material('mat_black', 'diffuse', {
        'Kd': [0,0,0]
    })
    light = lm.load_light('light', 'area', {
        'Ke': [1,1,1],
        'mesh': mesh.loc()
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    scene.add_primitive({
        'mesh': mesh.loc(),
        'material': material.loc(),
        'light': light.loc()
    })

def fireplace_room(scene, scene_path):
    """
    Fireplace room by Wig42.
    """
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [5.101118, 1.083746, -2.756308],
        'center': [4.167568, 1.078925, -2.397892],
        'up': [0,1,0],
        'vfov': 43.001194,
        'aspect': 16/9
    })
    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'fireplace_room/fireplace_room.obj')
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    scene.add_primitive({
        'model': model.loc()
    })

def cornell_box_empty_white(scene, scene_path):
    """
    Cornell box only with white walls.
    """
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [0,1,5],
        'center': [0,1,0],
        'up': [0,1,0],
        'vfov': 30,
        'aspect': 16/9
    })
    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'cornell_box/CornellBox-Empty-White.obj')
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    scene.add_primitive({
        'model': model.loc()
    })

def cornell_box_sphere(scene, scene_path):
    """
    Cornell box with specular spheres.
    """
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [0,1,5],
        'center': [0,1,0],
        'up': [0,1,0],
        'vfov': 30,
        'aspect': 16/9
    })
    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'cornell_box/CornellBox-Sphere.obj')
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    scene.add_primitive({
        'model': model.loc()
    })

# def bedroom(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [22.634083, -19.868534, 24.155586],
#         'center': [22.027370, -19.116102, 23.899178],
#         'up': [0,0,1],
#         'vfov': 57.978600
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'bedroom/iscv2.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def vokselia_spawn(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [1.525444, 0.983225, 0.731648],
#         'center': [0.780862, 0.377208, 0.451751],
#         'up': [0,1,0],
#         'vfov': 49.857803
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'vokselia_spawn/vokselia_spawn.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def breakfast_room(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [0.195303, 2.751188, 7.619322],
#         'center': [0.139881, 2.681201, 6.623315],
#         'up': [0,1,0],
#         'vfov': 39.384486
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'breakfast_room/breakfast_room.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def buddha(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [0.027255, 0.941126, -2.217943],
#         'center': [0.001645, 0.631268, -1.267505],
#         'up': [0,1,0],
#         'vfov': 19.297509
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'buddha/buddha.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def bunny(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [-0.191925, 2.961061, 4.171464],
#         'center': [-0.185709, 2.478091, 3.295850],
#         'up': [0,1,0],
#         'vfov': 28.841546
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'bunny/bunny.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def cloud(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [22.288721, 32.486145, 85.542023],
#         'center': [22.218456, 32.084743, 84.628822],
#         'up': [0,1,0],
#         'vfov': 53.023230
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'cloud/altostratus00.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

# def conference(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [926.391602, 329.094299, 141.714554],
#         'center': [925.628784, 328.858490, 141.112488],
#         'up': [0,1,0],
#         'vfov': 50.717898
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'conference/conference.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

def cube(scene, scene_path):
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [2, 2, 2],
        'center': [0, 0, 0],
        'up': [0,1,0],
        'vfov': 30,
        'aspect': 16/9
    })
    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'cube/cube.obj')
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    scene.add_primitive({
        'model': model.loc()
    })

# def powerplant(scene_path):
#     lm.asset('camera_main', 'camera::pinhole', {
#         'position': [-200000, 200000, 200000],
#         'center': [0, 0, 0],
#         'up': [0,1,0],
#         'vfov': 30
#     })
#     lm.asset('model_obj', 'model::wavefrontobj', {
#         'path': os.path.join(scene_path, 'powerplant/powerplant.obj')
#     })
#     lm.primitive(lm.identity(), {
#         'camera': lm.asset('camera_main')
#     })
#     lm.primitive(lm.identity(), {
#         'model': lm.asset('model_obj')
#     })

def mitsuba_knob_base(scene, scene_path, **kwargs):
    # Camera
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [0,4,5],
        'center': [0,0,-1],
        'up': [0,1,0],
        'vfov': 30,
        'aspect': 16/9
        #'vfov': 100
    })
    scene.add_primitive({
        'camera': camera.loc()
    })
    
    # Model
    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'mitsuba_knob', 'mitsuba.obj')
    })
    mat_diffuse_white = lm.load_material('mat_diffuse_white', 'diffuse', {
        'Kd': [.8,.8,.8]
    })
    scene.add_primitive({
        'mesh': model.make_loc('mesh_4'),
        #'material': mat_diffuse_white.loc()
        'material': model.make_loc('backdrop')
    })
    if 'mat_knob' in kwargs:
        scene.add_primitive({
            'mesh': model.make_loc('mesh_5'),
            'material': kwargs['mat_inside'] if 'mat_inside' in kwargs else mat_diffuse_white.loc()
        })
        scene.add_primitive({
            'mesh': model.make_loc('mesh_6'),
            'material': kwargs['mat_knob']
        })

def mitsuba_knob_with_area_light(scene, scene_path, **kwargs):
    mitsuba_knob_base(scene, scene_path, **kwargs)
    
    # Area light
    Ke = 10
    model_light = lm.load_model('model_light', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'mitsuba_knob', 'light.obj')
    })
    mat_black = lm.load_material('mat_black', 'diffuse', {'Kd': [0,0,0]})
    light = lm.load_light('light', 'area', {
        'Ke': [Ke,Ke,Ke],
        'mesh': model_light.make_loc('mesh_1')
    })
    scene.add_primitive({
        'mesh':  model_light.make_loc('mesh_1'),
        'material': mat_black.loc(),
        'light': light.loc()
    })

def mitsuba_knob_with_env_light_const(scene, scene_path, **kwargs):
    mitsuba_knob_base(scene, scene_path, **kwargs)
    
    # Environment light
    Le = 1
    light_env = lm.load_light('light_env', 'envconst', {
        'Le': [Le,Le,Le] 
    })
    scene.add_primitive({
        'light': light_env.loc()
    })

def mitsuba_knob_with_env_light(scene, scene_path, **kwargs):
    mitsuba_knob_base(scene, scene_path, **kwargs)

    # Environment light
    light_env = lm.load_light('light_env', 'env', kwargs)
    scene.add_primitive({
        'light': light_env.loc()
    })

def mitsuba_knob_with_point_light(scene, scene_path, **kwargs):
    mitsuba_knob_base(scene, scene_path, **kwargs)

    # Point light
    Le = 100
    light_point = lm.load_light('light_point', 'point', {
        'Le': [Le,Le,Le],
        'position': [5,5,5]
    })
    scene.add_primitive({
        'light': light_point.loc()
    })

def mitsuba_knob_with_directional_light(scene, scene_path, **kwargs):
    mitsuba_knob_base(scene, scene_path, **kwargs)

    # Directional light
    Le = 2
    light_directional = lm.load_light('light_directional', 'directional', {
        'Le': [Le,Le,Le],
        'direction': [-1,-1,-1]
    })
    scene.add_primitive({
        'light': light_directional.loc()
    })

# -------------------------------------------------------------------------------------------------

def bunny_base(scene, scene_path, **kwargs):
    camera = lm.load_camera('camera_main', 'pinhole', {
        'position': [-0.191925, 2.961061, 4.171464],
        'center': [-0.185709, 2.478091, 3.295850],
        'up': [0,1,0],
        'vfov': 28.841546,
        'aspect': 16/9
    })
    scene.add_primitive({
        'camera': camera.loc()
    })

    model = lm.load_model('model_obj', 'wavefrontobj', {
        'path': os.path.join(scene_path, 'bunny', 'bunny_with_planes.obj')
    })
    mat_diffuse_white = lm.load_material('mat_diffuse_white', 'diffuse', {
        'Kd': [.8,.8,.8]
    })
    
    # floor
    tex = lm.load_texture('tex_floor', 'bitmap', {
        'path': os.path.join(scene_path, 'bunny', 'default.png')
    })
    mat_floor = lm.load_material('mat_floor', 'diffuse', {
        'mapKd': tex.loc()
    })
    scene.add_primitive({
        'mesh': model.make_loc('mesh_2'),
        'material': mat_floor.loc()
    })
    # bunny
    if 'mat_knob' in kwargs:
        scene.add_primitive({
            'mesh': model.make_loc('mesh_1'),
            'material': kwargs['mat_knob']
        })

def bunny_with_area_light(scene, scene_path, **kwargs):
    bunny_base(scene, scene_path, **kwargs)

    # Light source
    Ke = 10
    mat_black = lm.load_material('mat_black', 'diffuse', {'Kd': [0,0,0]})
    light = lm.load_light('light', 'area', {
        'Ke': [Ke,Ke,Ke],
        'mesh': '$.assets.model_obj.mesh_3'
    })
    scene.add_primitive({
        'mesh':  '$.assets.model_obj.mesh_3',
        'material': mat_black.loc(),
        'light': light.loc()
    })

def bunny_with_env_light(scene, scene_path, **kwargs):
    bunny_base(scene, scene_path, **kwargs)

    # Environment light
    light_env = lm.load_light('light_env', 'env', kwargs)
    scene.add_primitive({
        'light': light_env.loc()
    })

def bunny_with_point_light(scene, scene_path, **kwargs):
    bunny_base(scene, scene_path, **kwargs)

    # Point light
    Le = 100
    light_point = lm.load_light('light_point', 'point', {
        'Le': [Le,Le,Le],
        'position': [5,5,5]
    })
    scene.add_primitive({
        'light': light_point.loc()
    })

def bunny_with_directional_light(scene, scene_path, **kwargs):
    bunny_base(scene, scene_path, **kwargs)

    # Directional light
    Le = 2
    light_directional = lm.load_light('light_directional', 'directional', {
        'Le': [Le,Le,Le],
        'direction': [-1,-1,-1]
    })
    scene.add_primitive({
        'light': light_directional.loc()
    })