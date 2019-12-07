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

# ## Scheduler for rendering

import lmenv
env = lmenv.load('.lmenv')

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lightmetrica as lm
# %load_ext lightmetrica_jupyter
import lmscene

if not lm.Release:
    lm.attach_to_debugger()

lm.init()
if not lm.Release:
    lm.parallel.init('openmp', {'num_threads': 1})
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))

accel = lm.load_accel('accel', 'embree', {})
scene = lm.load_scene('scene', 'default', {
    'accel': accel.loc()
})
lmscene.load(scene, env.scene_path, 'fireplace_room')
scene.build()
film = lm.load_film('film_output', 'bitmap', {
    'w': 1920,
    'h': 1080
})

shared_renderer_params = {
    'scene': scene.loc(),
    'output': film.loc(),
    'image_sample_mode': 'image',
    'max_length': 20,
}

# ### w/ sample-based scheduler

renderer = lm.load_renderer('renderer', 'pt', {
    **shared_renderer_params,
    'scheduler': 'sample',
    'spp': 1,
    'num_samples': 10000000
})
renderer.render()

img1 = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img1,1/2.2),0,1), origin='lower')
plt.show()

# ### w/ time-based scheduler

renderer = lm.load_renderer('renderer', 'pt', {
    **shared_renderer_params,
    'scheduler': 'time',
    'render_time': 5,
})
renderer.render()

img2 = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img2,1/2.2),0,1), origin='lower')
plt.show()

# ### Diff

from scipy.ndimage import gaussian_filter
diff_gauss = np.abs(gaussian_filter(img1 - img2, sigma=3))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(diff_gauss,1/2.2),0,1), origin='lower')
plt.show()
