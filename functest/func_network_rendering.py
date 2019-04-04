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

# ## Network rendering
#
# This test demonstrates network rendering feature of Lightmetrica.

# %load_ext autoreload
# %autoreload 2

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm

os.getpid()

# %load_ext lightmetrica_jupyter

lm.init('user::default', {})
lm.log.init('logger::jupyter', {})
lm.progress.init('progress::jupyter', {})

lm.dist.init('dist::master::default', {
    'port': 5000
})

lm.dist.printWorkerInfo()

lmscene.load(ft.env.scene_path, 'fireplace_room')

lm.build('accel::sahbvh', {})

lm.asset('film_output', 'film::bitmap', {'w': 1920, 'h': 1080})
lm.renderer('renderer::raycast', {
    'output': lm.asset('film_output')
})

lm.dist.sync()

lm.render()

lm.dist.gatherFilm(lm.asset('film_output'))

img = np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()


