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

# ## Performance testing of serialization
#
# This test checks the performance improvement of scene setup with serialization feature.

import os
import imageio
import pandas as pd
import numpy as np
import timeit
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm

# %load_ext lightmetrica_jupyter

lm.init('user::default', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

scenes = lmscene.scenes_small()

scene_setup_time_df = pd.DataFrame(
    columns=['scene loading', 'serialization', 'deserialization'],
    index=scenes)
for scene in scenes:
    lm.reset()
    
    lm.asset('film_output', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })
    
    # Load the scene without serialization
    def load_scene():
        lmscene.load(ft.env.scene_path, scene)
        lm.build('accel::sahbvh', {})
    loading_time_without_serialization = timeit.timeit(stmt=load_scene, number=1)
    scene_setup_time_df['scene loading'][scene] = loading_time_without_serialization
    
    # Export the internal state to a file
    def serialize_scene():
        lm.serialize('lm.serialized')
    serialization_time = timeit.timeit(stmt=serialize_scene, number=1)
    scene_setup_time_df['serialization'][scene] = serialization_time
    
    # Import the internal state from the serialized file
    lm.reset()
    def deserialize_scene():
        lm.deserialize('lm.serialized')
    deserialization_time = timeit.timeit(stmt=deserialize_scene, number=1)
    scene_setup_time_df['deserialization'][scene] = deserialization_time

scene_setup_time_df
