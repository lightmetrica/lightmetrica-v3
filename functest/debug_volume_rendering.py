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

# ## Debugging volume rendering support
#
# This notebooks debugs volumetric rendering support of Lightmetrica.

import sys
import json
with open('.lmenv') as f:
    config = json.load(f)
    if config['path'] not in sys.path:
        sys.path.insert(0, config['path'])
    if config['bin_path'] not in sys.path:
        sys.path.insert(0, config['bin_path'])

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

if lm.Debug:
    lm.attachToDebugger()

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

# +
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
lm.asset('camera1', 'camera::pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})

# OBJ model
lm.asset('obj1', 'model::wavefrontobj', {
    'path': os.path.join(config['scene_path'], 'fireplace_room/fireplace_room.obj')
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
lm.render('renderer::volpt_naive', {
    'output': lm.asset('film1'),
    'spp': 1000,
    'maxLength': 20
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
