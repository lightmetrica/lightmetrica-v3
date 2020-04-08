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
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

if not lm.Release:
    lm.debug.attach_to_debugger()

scene_names = lmscene.scenes_small()


def rmse(img1, img2):
    return np.sqrt(np.mean((img1 - img2) ** 2))


rmse_series = pd.Series(index=scene_names)
for scene_name in scene_names:
    print("Testing [scene='{}']".format(scene_name))
    
    # Load scene and render
    print('w/o serialization')
    lm.reset()
    lm.load_film('film_output', 'bitmap', {
        'w': 1920,
        'h': 1080
    })
    lm.load_accel('accel', 'sahbvh', {})
    scene = lm.load_scene('scene', 'default', {
        'accel': '$.assets.accel'
    })
    lmscene.load(scene, env.scene_path, scene_name)
    scene.build()
    lm.load_renderer('renderer', 'raycast', {
        'scene': '$.assets.scene',
        'output': '$.assets.film_output',
    })
    
    renderer = lm.get_renderer('$.assets.renderer')
    renderer.render()
    film = lm.get_film('$.assets.film_output')
    img_orig = np.copy(film.buffer())
    
    # Visualize
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img_orig,1/2.2),0,1), origin='lower')
    plt.show()
    
    # Serialize, reset, deserialize, and render
    print('w/ serialization')
    lm.save_state_to_file('lm.serialized')
    lm.reset()
    lm.load_state_from_file('lm.serialized')
    
    renderer = lm.get_renderer('$.assets.renderer')
    renderer.render()
    film = lm.get_film('$.assets.film_output')
    img_serial = np.copy(film.buffer())
    
    # Visualize
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img_serial,1/2.2),0,1), origin='lower')
    plt.show()
    
    # Compare two images
    e = rmse(img_orig, img_serial)
    rmse_series[scene_name] = e

rmse_series
