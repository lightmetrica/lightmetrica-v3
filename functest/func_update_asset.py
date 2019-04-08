# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.3'
#       jupytext_version: 1.0.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Updating already-loaded material
#
# This test shows an example of updating already-loaded material.

import os
import imageio
import pandas as pd
import numpy as np
import timeit
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

lm.init('user::default', {
    'numThreads': -1,
    'logger': 'logger::jupyter'
})

# +
lm.asset('film_output', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})
lm.asset('camera_main', 'camera::pinhole', {
    'film': 'film_output',
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})

lm.asset('obj_base_mat', 'material::diffuse', {
    'Kd': [.8,.2,.2]
})

lm.asset('model_obj', 'model::wavefrontobj', {
    'path': os.path.join(ft.env.scene_path, 'fireplace_room/fireplace_room.obj'),
    'base_material': 'obj_base_mat'
})

lm.primitive(lm.identity(), {'camera': 'camera_main'})
lm.primitives(lm.identity(), 'model_obj')
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::raycast', {
    'output': 'film_output'
})

# %matplotlib inline
img = np.flip(lm.buffer('film_output'), axis=0)
f = plt.figure(figsize=(10,10))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
ax.set_title('orig')

# Replace `obj_base_mat` with different color
# Note that this is not trivial, because `model::wavefrontobj`
# already holds a reference to the original material.
lm.asset('obj_base_mat', 'material::diffuse', {
    'Kd': [.2,.8,.2]
})
lm.render('renderer::raycast', {
    'output': 'film_output'
})

# %matplotlib inline
img = np.flip(lm.buffer('film_output'), axis=0)
f = plt.figure(figsize=(10,10))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
ax.set_title('updated')


