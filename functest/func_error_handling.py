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

# ## Error handling of Lightmetrica
#
# This test covers typical runtime error outputs from Lightmetrica.

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
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

# ### Calling Lightmetrica API without initialization
#
# Calling Lightmetrica API without initialization causes an exception.

# This causes an exception
lm.info()

# Initializing Lightmetrica fixes the problem
lm.init('user::default', {})

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

# Wrong: film:bitmap
lm.asset('film1', 'film:bitmap', {'w': 1920, 'h': 1080})

# Correct: film::bitmap
lm.asset('film1', 'film::bitmap', {'w': 1920, 'h': 1080})

# ### Invalid parameter
#
# The framework will cause an exception if you try to create an asset with invalid parameters.
# The framework will *not* generate an error for the unnecessasry parameters.

# vfov is missing
lm.asset('camera1', 'camera::pinhole', {
    'position': [0,0,5],
    'center': [0,0,0],
    'up': [0,1,0]
})

lm.asset('camera1', 'camera::pinhole', {
    # Parameter type is wrong. position must be an array.
    'position': 5,
    'center': [0,0,0],
    'up': [0,1,0],
    'fov': 30
})

# This is correct
lm.asset('camera1', 'camera::pinhole', {
    # Parameter type is wrong. position must be an array.
    'position': [0,0,5],
    'center': [0,0,0],
    'up': [0,1,0],
    'fov': 30
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


