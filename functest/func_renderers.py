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

# # Renderers
#
# This test showcases rendering with various renderer approaches.

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import pickle
import json
import numpy as np
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter
import lmscene

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()
lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))
if not lm.Release:
    lm.parallel.init('openmp', {'num_threads': 1})
    lm.debug.attach_to_debugger()


# +
def render(scene, name, params = {}):
    w = 854
    h = 480
    film = lm.load_film('film', 'bitmap', {'w': w, 'h': h})
    renderer = lm.load_renderer('renderer', name, {
        'scene': scene.loc(),
        'output': film.loc(),
        'max_verts': 10,
        'scheduler': 'time',
        'render_time': 30,
        **params
    })
    renderer.render()
    return np.copy(film.buffer())

def display_image(img, fig_size=15, scale=1):
    f = plt.figure(figsize=(fig_size,fig_size))
    ax = f.add_subplot(111)                                                                                                                                                
    ax.imshow(np.clip(np.power(img*scale,1/2.2),0,1), origin='lower')
    ax.axis('off')
    plt.show()


# -

# ## Scene setup

# Create scene
accel = lm.load_accel('accel', 'embree', {})
scene = lm.load_scene('scene', 'default', {'accel': accel.loc()})
lmscene.cornell_box_sphere(scene, env.scene_path)
scene.build()

# ## Rendering

# ### Path tracing (naive)
#
# `renderer::pt` with `sampling_mode = naive`. For comparison, the primary rays are sampling from the entire image (`primary_ray_sampling_mode = image`).

img = render(scene, 'pt', {
    'sampling_mode': 'naive',
    'primary_ray_sampling_mode': 'image'
})
display_image(img)

# ### Path tracing (next event estimation)
#
# `renderer::pt` with `sampling_mode = nee`

img = render(scene, 'pt', {
    'sampling_mode': 'nee',
    'primary_ray_sampling_mode': 'image'
})
display_image(img)

# ### Path tracing (multiple importance sampling)
#
# `renderer::pt` with `sampling_mode = mis`

img = render(scene, 'pt', {
    'sampling_mode': 'mis',
    'primary_ray_sampling_mode': 'image'
})
display_image(img)

# ### Light tracing
#
# `renderer::lt`

img = render(scene, 'lt')
display_image(img)

# ### Bidirectional path tracing
#
# `renderer::bdpt` (unoptimized, bad absolute performance)

img = render(scene, 'bdpt')
display_image(img)


