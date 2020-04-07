# ---
# jupyter:
#   jupytext:
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

# ## Updating already-loaded material
#
# This test shows an example of updating already-loaded material.

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
import timeit
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

camera = lm.load_camera('camera_main', 'pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194,
    'aspect': 16/9
})
material = lm.load_material('obj_base_mat', 'diffuse', {
    'Kd': [.8,.2,.2]
})
model = lm.load_model('model_obj', 'wavefrontobj', {
    'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj'),
    'base_material': material.loc()
})
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

film = lm.load_film('film_output', 'bitmap', {'w': 1920, 'h': 1080})
renderer = lm.load_renderer('renderer', 'raycast', {
    'scene': scene.loc(),
    'output': film.loc()
})
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(10,10))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
ax.set_title('orig')

# Replace `obj_base_mat` with different color
# Note that this is not trivial, because `model::wavefrontobj`
# already holds a reference to the original material.
lm.load_material('obj_base_mat', 'diffuse', {
    'Kd': [.2,.8,.2]
})

renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(10,10))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
ax.set_title('modified')
