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

# ## Validating component tree

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

lm.init('user::default', {
    'numThreads': -1,
    'logger': 'logger::jupyter'
})

for scene in lmscene.scenes():
    lm.reset()
    
    lmscene.load(ft.env.scene_path, scene)
    lm.build('accel::sahbvh', {})
    
    lm.validate()


