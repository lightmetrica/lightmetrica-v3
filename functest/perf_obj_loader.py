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

# ## Performance testing of OBJ loader

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
import timeit
import lmscene
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

lm.init()
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

lm.comp.loadPlugin(os.path.join(env.bin_path, 'objloader_tinyobjloader'))

objloaders = ['objloader::simple', 'objloader::tinyobjloader']
scenes = lmscene.scenes_small()

loading_time_df = pd.DataFrame(columns=objloaders, index=scenes)
for scene in scenes:
    # Check consistency with other loaders
    for objloader in objloaders:
        # Load the scene with selected obj loader
        lm.objloader.init(objloader, {})
        lm.reset()
        def load_scene():
            lmscene.load(env.scene_path, scene)
        loading_time = timeit.timeit(stmt=load_scene, number=1)
        loading_time_df[objloader][scene] = loading_time

loading_time_df
