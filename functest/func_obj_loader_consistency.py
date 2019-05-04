# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.1.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Checking consistency of OBJ loader

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

# %load_ext lightmetrica_jupyter

lm.init('user::default', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'accel_nanort'))
lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'objloader_tinyobjloader'))


def build_and_render(scene):
    lm.reset()
    lmscene.load(ft.env.scene_path, scene)
    lm.build('accel::nanort', {})
    lm.asset('film_output', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    return np.copy(lm.buffer(lm.asset('film_output')))


objloaders = ['objloader::tinyobjloader']
scenes = lmscene.scenes_small()

for scene in scenes:
    # Reference
    lm.objloader.init('objloader::simple', {})
    ref = build_and_render(scene)
    
    # Visualize reference
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(ref,1/2.2),0,1), origin='lower')
    ax.set_title('{}, objloader::simple'.format(scene))
    plt.show()
    
    # Check consistency with other loaders
    for objloader in objloaders:
        # Render
        lm.objloader.init(objloader, {})
        img = build_and_render(scene)
        diff = ft.rmse_pixelwised(ref, img)
    
        # Visualize
        f = plt.figure(figsize=(15,15))
        ax = f.add_subplot(111)
        ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
        ax.set_title('{}, {}'.format(scene, objloader))
        plt.show()
    
        # Visualize the difference image
        f = plt.figure(figsize=(15,15))
        ax = f.add_subplot(111)
        im = ax.imshow(diff, origin='lower')
        divider = make_axes_locatable(ax)
        cax = divider.append_axes("right", size="5%", pad=0.05)
        plt.colorbar(im, cax=cax)
        ax.set_title('{}, objloader::simple vs. {}'.format(scene, objloader))
        plt.show()
