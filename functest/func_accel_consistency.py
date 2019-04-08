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

# ## Checking consistency of acceleration structures
#
# This test checks consistencies between different acceleration structures. We render the images with `renderer::raycast` with different accelleration structures for various scenes, and compute differences among them. If the implementations are correct, all difference images should be blank because ray-triangle intersection is a deterministic process.

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
lm.log.init('logger::jupyter', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.info()
lm.log.setSeverity(lm.log.LogLevel.Warn)
lm.comp.detail.loadPlugin(os.path.join(ft.env.bin_path, 'accel_nanort'))


def build_and_render(scene, accel):
    lm.reset()
    lmscene.load(ft.env.scene_path, scene)
    lm.build('accel::' + accel, {})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output')
    })
    return np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)


# ### Difference images (pixelwised RMSE)
#
# Correct if the difference images are blank.

# Execute rendering for each scene and accel
rmse_df = pd.DataFrame(columns=ft.accels(), index=lmscene.scenes())
rmse_df.drop(columns='sahbvh', inplace=True)
for scene in lmscene.scenes():
    # Use the image for 'accel::sahbvh' as reference
    ref = build_and_render(scene, 'sahbvh')
    
    # Check consistency for other accels
    for accel in ft.accels():
        if accel == 'sahbvh':
            continue
            
        # Render and compute a different image
        img = build_and_render(scene, accel)
        diff = ft.rmse_pixelwised(ref, img)
        
        # Record rmse
        rmse = ft.rmse(ref, img)
        rmse_df[accel][scene] = rmse
    
        # Visualize the difference image
        f = plt.figure(figsize=(10,10))
        ax = f.add_subplot(111)
        im = ax.imshow(diff)
        divider = make_axes_locatable(ax)
        cax = divider.append_axes("right", size="5%", pad=0.05)
        plt.colorbar(im, cax=cax)
        ax.set_title('{}, sahbvh vs. {}'.format(scene, accel))
        plt.show()

# ### RMSE
#
# Correct if the values are near zero.

rmse_df
