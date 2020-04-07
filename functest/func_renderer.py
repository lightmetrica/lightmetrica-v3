# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.3.0
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# # Testing renderers
#
# This test renders a same scene using different rendering techniques implemented in the framework.

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter
import lmscene
from util import *

# ## Setup framework

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()
lm.comp.load_plugin(os.path.join(env.bin_path, 'accel_embree'))
if not lm.Release:
    lm.parallel.init('openmp', {'num_threads': 1})
    lm.debug.attach_to_debugger()

# ## Scene setup

accel = lm.load_accel('accel', 'sahbvh', {})
scene = lm.load_scene('scene', 'default', {'accel': accel.loc()})
#lmscene.plane_emitter(scene, env.scene_path)
lmscene.fireplace_room(scene, env.scene_path)
#lmscene.cornell_box_empty_white(scene, env.scene_path)
#lmscene.cornell_box_sphere(scene, env.scene_path)
scene.build()

# ## Rendering

fig_size = 10

# Resolution is fixed to 480p
film = lm.load_film('film', 'bitmap', {
    'w': 854,
    'h': 480
})
common_params = {
    'scene': scene.loc(),
    'output': film.loc(),
    'scheduler': 'time',
    'render_time': 5,
    'max_verts': 10
}


# Render reference image
def render_ref():
    renderer = lm.load_renderer('renderer', 'pt', {
        **common_params,
        'sampling_mode': 'naive'
    })
    renderer.render()
    return np.copy(film.buffer())
img_ref = render_ref()
display_image(img_ref, 'ref', fig_size)

# +
# Compute normalization factor used for MCMC based renderers
norm = normalization(img_ref)

# Common parameters for MCMC based renderers
mlt_common_params = {
    'normalization': norm,
    's1': 0.001,
    's2': 0.1
}
# -

# Execute rendering with different renderers and compare against reference
# List of (renderer_name, additional options)
renderers = [
    ('raycast', {'visualize_normal': True}),
    ('pt', {'sampling_mode': 'naive'}),
    ('pt', {'sampling_mode': 'nee'}),
    ('pt', {'sampling_mode': 'mis'}),
    ('lt', {}),
    ('pt_naive_path', {}),
    ('pt_nee_path', {}),
    ('lt_nee_path', {}),
    ('bdpt', {}),
    ('pssmlt', {
        **mlt_common_params,
        'large_step_prob': .5
    }),
    ('mlt', {
        **mlt_common_params,
        'mut_weights': {
            'bidir': 1,
            'lens': 1
        }
    })
]
for renderer_name, opt in renderers:
    # Execute rendering
    renderer = lm.load_renderer('renderer', renderer_name, {
        **common_params,
        **opt
    })
    renderer.render()
    img = np.copy(film.buffer())
    
    # Display rendered result
    display_image(img, renderer_name + ' ' + str(opt), fig_size)
        
    # Compare against reference
    if renderer_name != 'raycast':
        display_diff_gauss(img, img_ref, fig_size, 5)
