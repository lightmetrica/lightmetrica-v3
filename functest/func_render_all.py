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
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

if not lm.Release:
    lm.debug.attach_to_debugger()

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))
lm.comp.load_plugin(os.path.join(env.bin_path, 'objloader_tinyobjloader'))

lm.objloader.init('tinyobjloader')

scene_names = lmscene.scenes_small()

for scene_name in scene_names:
    lm.reset()
    
    # Load scene
    accel = lm.load_accel('accel', 'embree', {})
    scene = lm.load_scene('scene', 'default', {
        'accel': accel.loc()
    })
    lmscene.load(scene, env.scene_path, scene_name)
    scene.build()
    
    # Render
    film = lm.load_film('film_output', 'bitmap', {
        'w': 1920,
        'h': 1080
    })
    renderer = lm.load_renderer('renderer', 'raycast', {
        'scene': scene.loc(),
        'output': film.loc()
    })
    renderer.render()
    
    # Visualize
    img = np.copy(film.buffer())
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
    ax.set_title(scene)
    plt.show()


