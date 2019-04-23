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

# ## Custom material
#
# This test demostrates how to create an custom material from python side. Due to GIL, the execution of the python function is limited to a single thread. 

# %load_ext autoreload
# %autoreload 2

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm

os.getpid()


# %load_ext lightmetrica_jupyter

@lm.pylm_component('material::visualize_normal')
class Material_VisualizeNormal(lm.Material):
    def construct(self, prop):
        return True
    def isSpecular(self, geom, comp):
        return False
    def sample(self, geom, wi):
        return None
    def reflectance(self, geom, comp):
        return np.abs(geom.n)
    def pdf(self, geom, comp, wi, wo):
        return 0
    def eval(self, geom, comp, wi, wo):
        return np.zeros(3)


lm.init('user::default', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': 1
})
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

# +
# Original material
lm.asset('mat_vis_normal', 'material::visualize_normal', {})

# Scene
lm.asset('camera_main', 'camera::pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})
lm.asset('model_obj', 'model::wavefrontobj', {
    'path': os.path.join(ft.env.scene_path, 'fireplace_room/fireplace_room.obj'),
    'base_material': lm.asset('mat_vis_normal')
})
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera_main')
})
lm.primitive(lm.identity(), {
    'model': lm.asset('model_obj')
})
# -

lm.build('accel::sahbvh', {})

# +
# Render
lm.asset('film_output', 'film::bitmap', {
    'w': 640,
    'h': 360
})
lm.render('renderer::raycast', {
    'output': lm.asset('film_output')
})
img = np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)

# Visualize
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()
