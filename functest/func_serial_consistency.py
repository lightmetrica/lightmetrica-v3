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

# ## Checking consistency of serialization
#
# This test checks the consistency of the internal states between before/after serialization. We render two images. One with normal configuration and the other with deserialized states.

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm

os.getpid()

# %load_ext lightmetrica_jupyter

lm.init()
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

scenes = lmscene.scenes_small()


def rmse(img1, img2):
    return np.sqrt(np.mean((img1 - img2) ** 2))


rmse_series = pd.Series(index=scenes)
for scene in scenes:
    print("Testing [scene='{}']".format(scene))
    
    lm.reset()
    
    lm.asset('film_output', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })
    
    # Load scene and render
    lmscene.load(env.scene_path, scene)
    lm.build('accel::sahbvh', {})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    img_orig = np.copy(lm.buffer(lm.asset('film_output')))
    
    # Serialize, reset, deserialize, and render
    lm.serialize('lm.serialized')
    lm.reset()
    lm.deserialize('lm.serialized')
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    img_serial = np.copy(lm.buffer(lm.asset('film_output')))
    
    # Compare two images
    e = rmse(img_orig, img_serial)
    rmse_series[scene] = e

rmse_series
