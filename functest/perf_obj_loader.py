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
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

lm.comp.load_plugin(os.path.join(env.bin_path, 'objloader_tinyobjloader'))

objloader_names = ['simple', 'tinyobjloader']
scene_names = lmscene.scenes_small()

loading_time_df = pd.DataFrame(columns=objloader_names, index=scene_names)
for scene_name in scene_names:
    # Check consistency with other loaders
    for objloader_name in objloader_names:
        # Load the scene with selected obj loader
        lm.objloader.init(objloader_name)
        lm.reset()
        
        def load_model():
            lm.load_model('model_obj', 'wavefrontobj', {
                'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj')
            })
            
        loading_time = timeit.timeit(stmt=load_model, number=1)
        loading_time_df[objloader_name][scene_name] = loading_time

loading_time_df
