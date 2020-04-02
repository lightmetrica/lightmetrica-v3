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

# ## Performance testing of acceleration structures
#
# This test checks performance of acceleration structure implemented in the framework for various scenes.

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
import timeit
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_nanort'))
lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))

accel_names = [
    'sahbvh',
    'nanort',
    'embree',
    'embreeinstanced'
]
scene_names = lmscene.scenes_small()

# +
build_time_df = pd.DataFrame(columns=accel_names, index=scene_names)
render_time_df = pd.DataFrame(columns=accel_names, index=scene_names)

film = lm.load_film('film_output', 'bitmap', {
    'w': 1920,
    'h': 1080
})

for scene_name in scene_names:
    # Create scene w/o accel
    scene = lm.load_scene('scene', 'default', {})
    lmscene.load(scene, env.scene_path, scene_name)    
    renderer = lm.load_renderer('renderer', 'raycast', {
        'scene': scene.loc(),
        'output': film.loc()
    })
        
    for accel_name in accel_names:
        accel = lm.load_accel('accel', accel_name, {})
        scene.set_accel(accel.loc())
        def build():
            scene.build()
        build_time = timeit.timeit(stmt=build, number=1)
        build_time_df[accel_name][scene_name] = build_time

        def render():
            renderer.render()
        render_time = timeit.timeit(stmt=render, number=1)
        render_time_df[accel_name][scene_name] = render_time
# -

build_time_df

render_time_df
