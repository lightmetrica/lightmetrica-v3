# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.1.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_pt:
#
# Rendering with path tracing
# ===========================
#
# This example describes how to render the scene with path tracing. Path tracing is a rendering technique based on Monte Carlo method and notably one of the most basic (yet practical) rendering algorithms taking global illumination into account. Our framework implements path tracing as ``renderer::pt`` renderer. 
#
# The use of the renderer is straightforward; we just need to specify ``renderer::pt`` with :cpp:func:`lm::render` function with some renderer-specific parameters. Thanks to the modular design of the framework, the most of the code can be the same as :ref:`example_raycast`.
# -

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lmfunctest as ft
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

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
    'path': os.path.join(ft.env.scene_path, 'fireplace_room/fireplace_room.obj')
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
lm.render('renderer::pt', {
    'output': lm.asset('film1'),
    'spp': 10,
    'maxLength': 20
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
