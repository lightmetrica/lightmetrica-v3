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

# ## Error handling
#
# This test covers typical runtime error outputs from Lightmetrica.

# %load_ext autoreload
# %autoreload 2

import lmenv
lmenv.load('.lmenv')

import os
import traceback
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

# ### Calling Lightmetrica API without initialization
#
# Calling Lightmetrica API without initialization causes an exception.

try:
    # This causes an exception
    lm.info()
except Exception:
    traceback.print_exc()

# Initializing Lightmetrica fixes the problem
lm.init()

# ### No outputs in Jupyter notebook
#
# Lightmetrica initializes default logger that outputs messages into standard output of the console. If you are using Jupyter notebook, you need to initialize the logger using `logger::jupyter`.

# No output in Jupyter notebook
lm.info()

# Initialize the logger with logger::jupyter
lm.log.init('jupyter')

lm.info()

# ### Wrong asset name
#
# If you specify the wrong asset name, the framework will rause an exception.

try:
    # Wrong: bitmapp
    lm.load_film('film1', 'bitmapp', w=1920, h=1080)
except Exception:
    traceback.print_exc()

# Correct: bitmap
lm.load_film('film1', 'bitmap', w=1920, h=1080)

# ### Invalid parameter
#
# The framework will cause an exception if you try to create an asset with invalid parameters.
# The framework will *not* generate an error for the unnecessasry parameters.

try:
    # vfov is missing
    lm.load_camera('camera1', 'pinhole',
        position=[0,0,5],
        center=[0,0,0],
        up=[0,1,0],
        aspect=16/9)
except Exception:
    traceback.print_exc()

try:
    lm.load_camera('camera1', 'pinhole',
        # Parameter type is wrong. position must be an array.
        position=5,
        center=[0,0,0],
        up=[0,1,0],
        aspect=16/9,
        fov=30)
except Exception:
    traceback.print_exc()

# This is correct
lm.load_camera('camera1', 'pinhole',
    # Parameter type is wrong. position must be an array.
    position=[0,0,5],
    center=[0,0,0],
    up=[0,1,0],
    aspect=16/9,
    vfov=30)

# ### Missing reference

lm.load_mesh('mesh1', 'raw',
    ps=[-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1],
    ns=[0,0,1],
    ts=[0,0,1,0,1,1,0,1],
    fs={
        'p': [0,1,2,0,2,3],
        'n': [0,0,0,0,0,0],
        't': [0,1,2,0,2,3]
    })

accel = lm.load_accel('accel', 'sahbvh')
scene = lm.load_scene('scene', 'default', accel=accel)

try:
    # material1 is undefined
    scene.add_primitive(mesh='$.assets.mesh1', material='$.assets.material1')
except Exception:
    traceback.print_exc()

# Define a missing asset
lm.load_material('material1', 'diffuse', Kd=[1,1,1])

try:
    # 'material1' is not a valid locator
    scene.add_primitive(mesh='$.assets.mesh1', material='material1')
except Exception:
    traceback.print_exc()

# This is correct
scene.add_primitive(mesh='$.assets.mesh1', material='$.assets.material1')

# ### Rendering with invalid scene

renderer = lm.load_renderer('renderer', 'raycast',
    scene=scene,
    output='$.assets.film1',
    color=[0,0,0])

try:
    # Without camera
    renderer.render()
except Exception:
    traceback.print_exc()

# Add camera primitive
scene.add_primitive(camera='$.assets.camera1')

try:
    # With camera, but without build()
    renderer.render()
except Exception:
    traceback.print_exc()

scene.build()

# OK
renderer.render()
