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
lm.log.init('logger::jupyter', {})

lm.info()

# ### Wrong asset name
#
# If you specify the wrong asset name, the framework will rause an exception.

try:
    # Wrong: film:bitmap
    lm.asset('film1', 'film:bitmap', {'w': 1920, 'h': 1080})
except Exception:
    traceback.print_exc()

# Correct: film::bitmap
lm.asset('film1', 'film::bitmap', {'w': 1920, 'h': 1080})

# ### Invalid parameter
#
# The framework will cause an exception if you try to create an asset with invalid parameters.
# The framework will *not* generate an error for the unnecessasry parameters.

try:
    # vfov is missing
    lm.asset('camera1', 'camera::pinhole', {
        'position': [0,0,5],
        'center': [0,0,0],
        'up': [0,1,0]
    })
except Exception:
    traceback.print_exc()

try:
    lm.asset('camera1', 'camera::pinhole', {
        # Parameter type is wrong. position must be an array.
        'position': 5,
        'center': [0,0,0],
        'up': [0,1,0],
        'fov': 30
    })
except Exception:
    traceback.print_exc()

# This is correct
lm.asset('camera1', 'camera::pinhole', {
    # Parameter type is wrong. position must be an array.
    'position': [0,0,5],
    'center': [0,0,0],
    'up': [0,1,0],
    'vfov': 30
})

# ### Missing reference

lm.asset('mesh1', 'mesh::raw', {
    'ps': [-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1],
    'ns': [0,0,1],
    'ts': [0,0,1,0,1,1,0,1],
    'fs': {
        'p': [0,1,2,0,2,3],
        'n': [0,0,0,0,0,0],
        't': [0,1,2,0,2,3]
    }
})

# material1 is undefined
lm.primitive(lm.identity(), {
    'mesh': lm.asset('mesh1'),
    'material': lm.asset('material1')
})

# Define a missing asset
lm.asset('material1', 'material::diffuse', {
    'Kd': [1,1,1]
})

# 'material1' is not a valid locator use lm.asset()
lm.primitive(lm.identity(), {
    'mesh': lm.asset('mesh1'),
    'material': 'material1'
})

# This is correct
lm.primitive(lm.identity(), {
    'mesh': lm.asset('mesh1'),
    'material': lm.asset('material1')
})

# ### Rendering with invalid scene

try:
    # Without camera
    lm.render('renderer::raycast', {
        'output': lm.asset('film1'),
        'color': [0,0,0]
    })
except Exception:
    traceback.print_exc()

lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

try:
    # Without acceleration structure
    lm.render('renderer::raycast', {
        'output': lm.asset('film1'),
        'color': [0,0,0]
    })
except Exception:
    traceback.print_exc()

lm.build('accel::sahbvh', {})

lm.render('renderer::raycast', {
    'output': lm.asset('film1'),
    'color': [0,0,0]
})
