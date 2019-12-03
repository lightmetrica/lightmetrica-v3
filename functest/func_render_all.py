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

# ## Rendering all scenes
#
# This test checks scene loading and basic rendering for all test scenes.

# %load_ext autoreload
# %autoreload 2

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

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))
lm.comp.load_plugin(os.path.join(env.bin_path, 'objloader_tinyobjloader'))

lm.objloader.init('objloader::tinyobjloader', {})

scenes = lmscene.scenes_small()

for scene in scenes:
    lm.reset()
    
    # Film
    lm.asset('film_output', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })
    
    # Render
    lmscene.load(env.scene_path, scene)
    lm.build('accel::embree', {})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    img = np.copy(lm.buffer(lm.asset('film_output')))
    
    # Visualize
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
    ax.set_title(scene)
    plt.show()
