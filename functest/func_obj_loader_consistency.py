# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.3.3
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Checking consistency of OBJ loader

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
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

lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))
lm.comp.load_plugin(os.path.join(env.bin_path, 'objloader_tinyobjloader'))


def build_and_render(scene_name):
    lm.reset()
    accel = lm.load_accel('accel', 'embree')
    scene = lm.load_scene('scene', 'default', accel=accel)
    lmscene.load(scene, env.scene_path, scene_name)
    scene.build()
    film = lm.load_film('film_output', 'bitmap', w=1920, h=1080)
    renderer = lm.load_renderer('renderer', 'raycast', scene=scene, output=film)
    renderer.render()
    return np.copy(film.buffer())


objloaders = ['tinyobjloader']
scene_names = lmscene.scenes_small()


def rmse_pixelwised(img1, img2):
    return np.sqrt(np.sum((img1 - img2) ** 2, axis=2) / 3)


for scene_name in scene_names:
    # Reference
    lm.objloader.init('simple')
    ref = build_and_render(scene_name)
    
    # Visualize reference
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(ref,1/2.2),0,1), origin='lower')
    ax.set_title('{}, simple'.format(scene_name))
    plt.show()
    
    # Check consistency with other loaders
    for objloader in objloaders:
        # Render
        lm.objloader.init(objloader, {})
        img = build_and_render(scene_name)
        diff = rmse_pixelwised(ref, img)
    
        # Visualize
        f = plt.figure(figsize=(15,15))
        ax = f.add_subplot(111)
        ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
        ax.set_title('{}, {}'.format(scene_name, objloader))
        plt.show()
    
        # Visualize the difference image
        f = plt.figure(figsize=(15,15))
        ax = f.add_subplot(111)
        im = ax.imshow(diff, origin='lower')
        divider = make_axes_locatable(ax)
        cax = divider.append_axes("right", size="5%", pad=0.05)
        plt.colorbar(im, cax=cax)
        ax.set_title('{}, simple vs. {}'.format(scene_name, objloader))
        plt.show()
