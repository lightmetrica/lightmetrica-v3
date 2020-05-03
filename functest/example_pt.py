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
# .. _example_pt:
#
# Rendering with path tracing
# ===========================
#
# This example describes how to render the scene with path tracing. Path tracing is a rendering technique based on Monte Carlo method and notably one of the most basic (yet practical) rendering algorithms taking global illumination into account. Our framework implements path tracing as ``renderer::pt`` renderer. 
#
# The use of the renderer is straightforward; we just need to create ``renderer::pt`` renderer and call :cpp:func:`lm::Renderer::render` function with some renderer-specific parameters. Thanks to the modular design of the framework, the most of the code can be the same as :ref:`example_raycast`.
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
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

# +
# Film for the rendered image
film = lm.load_film('film', 'bitmap', w=1920, h=1080)

# Pinhole camera
camera = lm.load_camera('camera1', 'pinhole',
    position=[5.101118, 1.083746, -2.756308],
    center=[4.167568, 1.078925, -2.397892],
    up=[0,1,0],
    vfov=43.001194,
    aspect=16/9)

# OBJ model
model = lm.load_model('model', 'wavefrontobj',
    path=os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj'))
# -

accel = lm.load_accel('accel', 'sahbvh')
scene = lm.load_scene('scene', 'default', accel=accel)
scene.add_primitive(camera=camera)
scene.add_primitive(model=model)
scene.build()

renderer = lm.load_renderer('renderer', 'pt',
    scene=scene,
    output=film,
    scheduler='sample',
    spp=5,
    max_verts=20)
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
