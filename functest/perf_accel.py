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

# ## Performance testing of acceleration structures
#
# This test checks performance of acceleration structure implemented in the framework for various scenes.

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

lm.init('user::default', {
    'numThreads': -1,
    'logger': 'logger::jupyter'
})
lm.log.setSeverity(lm.log.LogLevel.Warn)
lm.comp.detail.loadPlugin(os.path.join(ft.env.bin_path, 'accel_nanort'))

build_time_df = pd.DataFrame(columns=ft.accels(), index=lmscene.scenes())
render_time_df = pd.DataFrame(columns=ft.accels(), index=lmscene.scenes())
for scene in lmscene.scenes():
    for accel in ft.accels():
        lm.reset()

        lmscene.load(ft.env.scene_path, name)
        def build():
            lm.build('accel::' + accel, {})
        build_time = timeit.timeit(stmt=build, number=1)
        build_time_df[accel][scene] = build_time

        def render():
            lm.render('renderer::raycast', {
                'output': 'film_output'
            })
        render_time = timeit.timeit(stmt=render, number=1)
        render_time_df[accel][scene] = render_time

build_time_df

render_time_df


