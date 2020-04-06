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

# # Lights
#
# This test showcases various light sources supported by Lightmetrica. We render images using `renderer::pt`.

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
        'max_verts': 20,
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

accel = lm.load_accel('accel', 'embree', {})
scene = lm.load_scene('scene', 'default', {'accel': accel.loc()})
mat = lm.load_material('mat_ut', 'glossy', {
    'Ks': [.8,.2,.2],
    'ax': 0.1,
    'ay': 0.1
})

# ## Rendering

# ### Area light
#
# ``light::area``

scene.reset()
lmscene.mitsuba_knob_with_area_light(scene, env.scene_path, mat_knob=mat.loc())
scene.build()
img = render(scene, 'pt')
display_image(img)

# ### Environment light
#
# `light::env` `light::envconst`

scene.reset()
lmscene.mitsuba_knob_with_env_light(
    scene, env.scene_path,
    mat_knob=mat.loc(),
    path=os.path.join(env.scene_path, 'flower_road_1k.hdr'),
    rot=90,
    flip=False,
    scale=0.1)
scene.build()
img = render(scene, 'pt')
display_image(img)

# ### Point light
#
# `light::point`

scene.reset()
lmscene.mitsuba_knob_with_point_light(scene, env.scene_path, mat_knob=mat.loc())
scene.build()
img = render(scene, 'pt')
display_image(img)

# ### Directional light
#
# `light::directional`

scene.reset()
lmscene.mitsuba_knob_with_directional_light(scene, env.scene_path, mat_knob=mat.loc())
scene.build()
img = render(scene, 'pt')
display_image(img)


