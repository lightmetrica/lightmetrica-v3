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

if lm.BuildConfig != lm.ConfigType.Release:
    lm.attachToDebugger()

lm.init()
if lm.BuildConfig != lm.ConfigType.Release:
    lm.parallel.init('parallel::openmp', {'numThreads': 1})
lm.log.init('logger::jupyter', {})
lm.progress.init('progress::jupyter', {})
lm.info()

lm.comp.loadPlugin(os.path.join(env.bin_path, 'accel_embree'))

lmscene.load(env.scene_path, 'fireplace_room')
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})
lm.build('accel::embree', {})

shared_renderer_params = {
    'output': lm.asset('film1'),
    'image_sample_mode': 'image',
    'max_length': 20,
}

# ### w/ sample-based scheduler

lm.render('renderer::pt', {
    **shared_renderer_params,
    'scheduler': 'sample',
    'spp': 1,
    'num_samples': 10000000
})

img1 = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img1,1/2.2),0,1), origin='lower')
plt.show()

# ### w/ time-based scheduler

lm.render('renderer::pt', {
    **shared_renderer_params,
    'scheduler': 'time',
    'render_time': 5,
})

img2 = np.copy(lm.buffer(lm.asset('film1')))
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
