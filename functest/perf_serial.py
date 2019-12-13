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

# ## Performance testing of serialization
#
# This test checks the performance improvement of scene setup with serialization feature.

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

scene_names = lmscene.scenes_small()

scene_setup_time_df = pd.DataFrame(
    columns=['scene loading', 'serialization', 'deserialization'],
    index=scene_names)
for scene_name in scene_names:
    lm.reset()
    
    lm.load_film('film_output', 'bitmap', {
        'w': 1920,
        'h': 1080
    })
    
    # Load the scene without serialization
    def load_scene():
        accel = lm.load_accel('accel', 'sahbvh', {})
        scene = lm.load_scene('scene', 'default', {
            'accel': accel.loc()
        })
        lmscene.load(scene, env.scene_path, scene_name)
    loading_time_without_serialization = timeit.timeit(stmt=load_scene, number=1)
    scene_setup_time_df['scene loading'][scene_name] = loading_time_without_serialization
    
    # Export the internal state to a file
    def serialize_scene():
        lm.save_state_to_file('lm.serialized')
    serialization_time = timeit.timeit(stmt=serialize_scene, number=1)
    scene_setup_time_df['serialization'][scene_name] = serialization_time
    
    # Import the internal state from the serialized file
    lm.reset()
    def deserialize_scene():
        lm.load_state_from_file('lm.serialized')
    deserialization_time = timeit.timeit(stmt=deserialize_scene, number=1)
    scene_setup_time_df['deserialization'][scene_name] = deserialization_time

scene_setup_time_df
