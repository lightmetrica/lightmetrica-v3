# ---
# jupyter:
#   jupytext:
#     cell_metadata_json: true
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.3.3
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_raycast:
#
# Raycasting a scene with OBJ models
# =======================================
#
# This example demonstrates how to render a scene with OBJ models using raycasting.
# -

import lmenv
env = lmenv.load('.lmenv')

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

# + {"nbsphinx": "hidden"}
if not lm.Release:
    lm.debug.attach_to_debugger()
# -

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Following is the definition of assets. To load an OBJ model, we can use ``model::wavefrontobj`` asset. This asset internally creates meshes and materials by reading the associated MTL file.
#
# .. note::
#     A model asset can be considered as a special asset containing (a part of) the scene graph and assets reference by the structure. 

# +
# Film for the rendered image
film = lm.load_film('film', 'bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
camera = lm.load_camera('camera', 'pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194,
    'aspect': 16/9
})

# OBJ model
model = lm.load_model('model', 'wavefrontobj', {
    'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj')
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# We can create primitives from the loaded mode using ``model` parameter for the :cpp:func:`lm::Scene::add_primitive` function.
# -

accel = lm.load_accel('accel', 'sahbvh', {})
scene = lm.load_scene('scene', 'default', {
    'accel': accel.loc()
})
scene.add_primitive({
    'camera': camera.loc()
})
scene.add_primitive({
    'model': model.loc()
})
scene.build()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Executing the renderer will produce the following image.
# -

renderer = lm.load_renderer('renderer', 'raycast', {
    'scene': scene.loc(),
    'output': film.loc(),
    'bg_color': [0,0,0]
})
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
