# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.1.2
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Debugging volume rendering support
#
# This notebooks debugs volumetric rendering support of Lightmetrica.

import sys
import json
with open('.lmenv') as f:
    config = json.load(f)
    if config['path'] not in sys.path:
        sys.path.insert(0, config['path'])
    if config['bin_path'] not in sys.path:
        sys.path.insert(0, config['bin_path'])

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lightmetrica as lm
# %load_ext lightmetrica_jupyter
import lmscene

if lm.Debug:
    lm.attachToDebugger()

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
if lm.Debug:
    lm.parallel.init('parallel::openmp', {
        'numThreads': 1
    })
lm.info()

# +
#lmscene.load(config['scene_path'], 'fireplace_room')

# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1', 'camera::pinhole', {
        'position': [0,0,5],
        'center': [0,0,0],
        'up': [0,1,0],
        'vfov': 30
    })
})

# Mesh
lm.primitive(lm.identity(), {
    'mesh': lm.asset('mesh1', 'mesh::raw', {
        'ps': [-1,1,-1,1,1,-1,1,1,1,-1,1,1],
        'ns': [0,-1,0],
        #'ps': [-3,-3,0,3,-3,0,3,3,0,-3,3,0],
        #'ns': [0,0,1],
        'ts': [0,0,1,0,1,1,0,1],
        'fs': {
            'p': [0,1,2,0,2,3],
            'n': [0,0,0,0,0,0],
            't': [0,1,2,0,2,3]
        }
    }),
    'material': lm.asset('material1', 'material::diffuse', {
        'Kd': [0,0,0]
    }),
    'light': lm.asset('light1', 'light::area', {
        'Ke': [1,1,1],
        'mesh': lm.asset('mesh1')
    })
})

# Medium
lm.primitive(lm.identity(), {
    'medium': lm.asset('medium1', 'medium::homogeneous', {
        'muS': 0.5,
        'muA': 0,
        'phase': lm.asset('phase_iso', 'phase::isotropic', {})
    })
})
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::volpt', {
    'output': lm.asset('film1', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    }),
    'spp': 10,
    'maxLength': 2
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()

f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
im = ax.imshow(img[:,:,0], origin='lower')
divider = make_axes_locatable(ax)
cax = divider.append_axes("right", size="5%", pad=0.05)
plt.colorbar(im, cax=cax)
plt.show()


