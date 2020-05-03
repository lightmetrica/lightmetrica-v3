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

# # Materials
#
# This test showcases rendering with various materials provided by Lightmetrica. We render the images using ``renderer::pt``.

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
    lm.parallel.init('openmp', num_threads=1)
    lm.debug.attach_to_debugger()


# +
def render(scene, name, **kwargs):
    w = 854
    h = 480
    film = lm.load_film('film', 'bitmap', w=w, h=h)
    renderer = lm.load_renderer('renderer', name,
        scene=scene,
        output=film,
        max_verts=20,
        scheduler='time',
        render_time=30,
        **kwargs)
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
accel = lm.load_accel('accel', 'embree')
scene = lm.load_scene('scene', 'default', accel=accel)
mat = lm.load_material('mat_ut', 'diffuse', Kd=[1,1,1])
lmscene.bunny_with_area_light(scene, env.scene_path, mat_knob=mat)
scene.build()

# ## Rendering

# ### Diffse material
#
# `material::diffuse`

lm.load_material('mat_ut', 'diffuse', Kd=[.8,.2,.2])
img = render(scene, 'pt')
display_image(img)

# ### Glossy material
#
# `material::glossy`

lm.load_material('mat_ut', 'glossy', Ks=[.8,.2,.2], ax=0.2, ay=0.2)
img = render(scene, 'pt')
display_image(img)

# ### Perfect specular reflection
#
# `material::mirror`

lm.load_material('mat_ut', 'mirror')
img = render(scene, 'pt')
display_image(img)

# ### Fresnel reflection / refraction
#
# `material::fresnel`

lm.load_material('mat_ut', 'glass', Ni=1.5)
img = render(scene, 'pt')
display_image(img)

# ### Mixture material with constant weights using RR
#
# `material::constant_weight_mixture_rr`

mat_diffuse = lm.load_material('mat_diffuse', 'diffuse', Kd=[.1,.8,.1])
mat_glossy = lm.load_material('mat_glossy', 'glossy', Ks=[.8,.1,.1], ax=0.2, ay=0.2)
mat_mirror = lm.load_material('mat_mirror', 'mirror')
mat = lm.load_material('mat_ut', 'constant_weight_mixture_rr', [
    {'material': mat_diffuse.loc(), 'weight': 0.2},
    {'material': mat_glossy.loc(), 'weight': 0.4},
    {'material': mat_mirror.loc(), 'weight': 0.4}
])
img = render(scene, 'pt')
display_image(img)

# ### Mixture material with constant weights using marginalization 
#
# `material::constant_weight_mixture_marginalized`

mat = lm.load_material('mat_ut', 'constant_weight_mixture_marginalized', [
    {'material': mat_diffuse.loc(), 'weight': 0.2},
    {'material': mat_glossy.loc(), 'weight': 0.4},
    {'material': mat_mirror.loc(), 'weight': 0.4}
])
img = render(scene, 'pt')
display_image(img)

# ### Mixture material with alpha texture
#
# `material::mixture_wavefrontobj`
#
# This material is the default material converted from MTL format of Wavefront OBJ.

tex = lm.load_texture('tex', 'bitmap',
    path=os.path.join(env.scene_path, 'fireplace_room', 'textures', 'leaf.png'))
lm.load_material('mat_ut', 'mixture_wavefrontobj',
    Kd=[.8,.8,.8],
    mapKd=tex,
    Ks=[0,0,0],
    ax=0.2,
    ay=0.2,
    no_alpha_mask=False)
img = render(scene, 'pt')
display_image(img)
