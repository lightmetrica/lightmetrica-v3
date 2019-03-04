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

# ## Checking consistency of serialization
#
# This test checks the consistency of the internal states between before/after serialization. We render two images. One with normal configuration and the other with deserialized states.

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
lm.log.setSeverity(lm.log.LogLevel.Warn)

rmse_series = pd.Series(index=lmscene.scenes())
for scene in lmscene.scenes():
    lm.reset()
    
    # Load scene and render
    lmscene.load(ft.env.scene_path, scene)
    lm.build('accel::sahbvh', {})
    lm.render('renderer::raycast', {
        'output': 'film_output'
    })
    img_orig = np.flip(np.copy(lm.buffer('film_output')), axis=0)
    
    # Serialize, reset, deserialize, and render
    lm.serialize('lm.serialized')
    lm.reset()
    lm.deserialize('lm.serialized')
    lm.render('renderer::raycast', {
        'output': 'film_output'
    })
    img_serial = np.flip(np.copy(lm.buffer('film_output')), axis=0)
    
    # Compare two images
    rmse = ft.rmse(img_orig, img_serial)
    rmse_series[scene] = rmse

rmse_series
