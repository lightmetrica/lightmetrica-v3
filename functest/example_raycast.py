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

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Following is the definition of assets. To load an OBJ model, we can use ``model::wavefrontobj`` asset. This asset internally creates meshes and materials by reading the associated MTL file.
#
# .. note::
#     A model asset can be considered as a special asset containing (a part of) the scene graph and assets reference by the structure. 

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
    'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj')
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# We can create primitives from the loaded ``model::wavefrontobj`` asset by using :cpp:func:`lm::primitives` function. 

# +
# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

# Create primitives from model asset
lm.primitive(lm.identity(), {
    'model': lm.asset('obj1')
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Executing the renderer will produce the following image.
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
