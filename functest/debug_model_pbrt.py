# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.1.3
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lmfunctest as ft
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

if lm.Debug:
    lm.attachToDebugger()

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'model_pbrt'))

scene_path = r'C:\Users\rei\projects\pbrt-scenes'

# +
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
# lm.asset('camera1', 'camera::pinhole', {
#     'position': [0, 23, 30],
#     'center': [10.2, 5, 0],
#     'up': [0, 1, 0],
#     'vfov': 50
# })

# PBRT scene model
lm.asset('obj1', 'model::pbrt', {
    'path': os.path.join(scene_path, 'bathroom', 'bathroom.pbrt')
})

# +
# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

# Create primitives from model asset
lm.primitive(lm.identity(), {
    'model': lm.asset('obj1')
})
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::raycast', {
    'output': lm.asset('film1'),
    'color': [0,0,0]
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
