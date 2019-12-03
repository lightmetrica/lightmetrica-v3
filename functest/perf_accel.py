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
lm.parallel.init('parallel::openmp', {
    'num_threads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_nanort'))
lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))

accels = [
    'accel::sahbvh',
    'accel::nanort',
    'accel::embree',
    'accel::embreeinstanced'
]
scenes = lmscene.scenes_small()

build_time_df = pd.DataFrame(columns=accels, index=scenes)
render_time_df = pd.DataFrame(columns=accels, index=scenes)
for scene in scenes:
    lm.reset()
    lmscene.load(env.scene_path, scene)
    for accel in accels:
        lm.asset('film_output', 'film::bitmap', {
            'w': 1920,
            'h': 1080
        })
        
        def build():
            lm.build(accel, {})
        build_time = timeit.timeit(stmt=build, number=1)
        build_time_df[accel][scene] = build_time

        def render():
            lm.render('renderer::raycast', {
                'output': lm.asset('film_output')
            })
        render_time = timeit.timeit(stmt=render, number=1)
        render_time_df[accel][scene] = render_time

build_time_df

render_time_df
