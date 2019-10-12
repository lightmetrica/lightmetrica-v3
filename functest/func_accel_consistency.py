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

# ## Checking consistency of acceleration structures
#
# This test checks consistencies between different acceleration structures. We render the images with `renderer::raycast` with different accelleration structures for various scenes, and compute differences among them. If the implementations are correct, all difference images should be blank because ray-triangle intersection is a deterministic process.

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

# + {"code_folding": [0]}
# Initialize Lightmetrica
lm.init()
lm.log.init('logger::jupyter', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.info()
lm.comp.loadPlugin(os.path.join(env.bin_path, 'accel_nanort'))
lm.comp.loadPlugin(os.path.join(env.bin_path, 'accel_embree'))


# -

# ### Difference images (pixelwised RMSE)
#
# Correct if the difference images are blank.

# + {"code_folding": [0]}
# Function to build and render the image
def build_and_render(accel):
    lm.build(accel, {})
    lm.asset('film_output', 'film::bitmap', {'w': 1920, 'h': 1080})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    return np.copy(lm.buffer(lm.asset('film_output')))


# + {"code_folding": []}
# Accels and scenes
accels = ['accel::nanort', 'accel::embree', 'accel::embreeinstanced']
scenes = lmscene.scenes_small()


# +
def rmse(img1, img2):
    return np.sqrt(np.mean((img1 - img2) ** 2))

def rmse_pixelwised(img1, img2):
    return np.sqrt(np.sum((img1 - img2) ** 2, axis=2) / 3)


# -

# Execute rendering for each scene and accel
rmse_df = pd.DataFrame(columns=accels, index=scenes)
for scene in scenes:
    print("Rendering [scene='{}']".format(scene))
    
    # Load scene
    lm.reset()
    lmscene.load(env.scene_path, scene)
    
    # Use the image for 'accel::sahbvh' as reference
    ref = build_and_render('accel::sahbvh')
    
    # Check consistency for other accels
    for accel in accels:
        # Render and compute a different image
        img = build_and_render(accel)
        diff = rmse_pixelwised(ref, img)
        
        # Record rmse
        e = rmse(ref, img)
        rmse_df[accel][scene] = e
    
        # Visualize the difference image
        f = plt.figure(figsize=(10,10))
        ax = f.add_subplot(111)
        im = ax.imshow(diff, origin='lower')
        divider = make_axes_locatable(ax)
        cax = divider.append_axes("right", size="5%", pad=0.05)
        plt.colorbar(im, cax=cax)
        ax.set_title('{}, accel::sahbvh vs. {}'.format(scene, accel))
        plt.show()

# ### RMSE
#
# Correct if the values are near zero.

rmse_df
