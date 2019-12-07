# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.2.4
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Custom material in Python
#
# This test demostrates how to create an custom material using component extension in Python. Due to GIL, the execution of the python function is limited to a single thread. 

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm


# %load_ext lightmetrica_jupyter

@lm.pylm_component('material::visualize_normal')
class Material_VisualizeNormal(lm.Material):
    def construct(self, prop):
        return True
    def is_specular(self, geom, comp):
        return False
    def sample(self, geom, wi):
        return None
    def reflectance(self, geom, comp):
        return np.abs(geom.n)
    def pdf(self, geom, comp, wi, wo):
        return 0
    def eval(self, geom, comp, wi, wo):
        return np.zeros(3)


lm.init()
lm.parallel.init('openmp', {'num_threads': 1})
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

# +
# Original material
material = lm.load_material('mat_vis_normal', 'visualize_normal', {})

# Scene
camera = lm.load_camera('camera_main', 'pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
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

# +
# Render
film = lm.load_film('film_output', 'bitmap', {
    'w': 640,
    'h': 360
})
renderer = lm.load_renderer('renderer', 'raycast', {
    'scene': scene.loc(),
    'output': film.loc()
})
renderer.render()

# Visualize
img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
