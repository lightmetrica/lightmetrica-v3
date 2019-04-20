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

# ## Rendering with instanced geometries
#
# This test demonstrates rendering with multi-level instancing.

# %load_ext autoreload
# %autoreload 2

# + {"code_folding": []}
# Imports
import os
import traceback
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm
# -

os.getpid()

# %load_ext lightmetrica_jupyter

# + {"code_folding": []}
# Initialize Lightmetrica
lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
# -

lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'accel_embree'))

# + {"code_folding": []}
# Create a sphere geometry with triangle mesh
r = 1
numTheta = 10
numPhi = 2*numTheta
vs = np.zeros((numPhi*(numTheta+1), 3))
ns = np.zeros((numPhi*(numTheta+1), 3))
ts = np.zeros((numPhi*(numTheta+1), 2))
for i in range(numTheta+1):
    for j in range(numPhi):
        theta = i*np.pi/numTheta
        phi = j*2*np.pi/numPhi
        idx = i*numPhi+j
        ns[idx,0] = np.sin(theta)*np.sin(phi)
        ns[idx,1] = np.cos(theta)
        ns[idx,2] = np.sin(theta)*np.cos(phi)
        vs[idx,0] = r*ns[idx,0]
        vs[idx,1] = r*ns[idx,1]
        vs[idx,2] = r*ns[idx,2]

fs = np.zeros((2*numPhi*(numTheta-1), 3), dtype=np.int32)
idx = 0
for i in range(1,numTheta+1):
    for j in range(1,numPhi+1):
        p00 = (i-1)*numPhi+j-1
        p01 = (i-1)*numPhi+j%numPhi
        p10 = i*numPhi+j-1
        p11 = i*numPhi+j%numPhi
        if i > 1:
            fs[idx,:] = np.array([p10,p01,p00])
            idx += 1
        if i < numTheta:
            fs[idx,:] = np.array([p11,p01,p10])
            idx += 1


# + {"code_folding": []}
# Scene setup
def scene_setup():
    lm.reset()
#     lm.asset('mesh_sphere', 'mesh::raw',
#         ps = vs.flatten().tolist(),
#         ns = ns.flatten().tolist(),
#         ts = ts.flatten().tolist(),
#         fs_p = fs.flatten().tolist(),
#         fs_t = fs.flatten().tolist(),
#         fs_n = fs.flatten().tolist()
#     )
#     lm.asset('camera_main', 'camera::pinhole',
#         position = [0,0,50],
#         center = [0,0,0],
#         up = [0,1,0],
#         vfov = 30
#     )
#     lm.asset('material_white', 'material::diffuse',
#          Kd = [1,1,1]
#     )
#     lm.primitive(
#         camera = lm.asset('camera_main')
#     )
    lm.asset('mesh_sphere', 'mesh::raw', {
        'ps': vs.flatten().tolist(),
        'ns': ns.flatten().tolist(),
        'ts': ts.flatten().tolist(),
        'fs': {
            'p': fs.flatten().tolist(),
            't': fs.flatten().tolist(),
            'n': fs.flatten().tolist()
        }
    })
    lm.asset('camera_main', 'camera::pinhole', {
        'position': [0,0,50],
        'center': [0,0,0],
        'up': [0,1,0],
        'vfov': 30
    })
    lm.asset('material_white', 'material::diffuse', {
        'Kd': [1,1,1]
    })
    lm.primitive(lm.identity(), {
        'camera': lm.asset('camera_main')
    })


# -

# Rendering
def render_and_visualize():
    lm.build('accel::embree', {})
#     lm.asset('film_output', 'film::bitmap',
#         w = 1920,
#         h = 1080
#     )
#     lm.render('renderer::raycast',
#         output = lm.asset('film_output'),
#         visualize_normal = True,
#         bg_color = [1,1,1]
#     )
    lm.asset('film_output', 'film::bitmap', {'w': 1920, 'h': 1080})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output'),
        'visualize_normal': True,
        'bg_color': [1,1,1]
    })
    img = np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1))
    plt.show()


# ### Without instancing

scene_setup()
for y in np.linspace(-10,10,10):
    for x in np.linspace(-10,10,10):
        p = lm.primitivenode({
            'mesh': lm.asset('mesh_sphere'),
            'material': lm.asset('material_white')
        })
        t = lm.transformnode(lm.translate(np.array([x,y,0])))
        lm.addchild(t, p)
        lm.addchild(lm.rootnode(), t)

render_and_visualize()

# ### Single-level instancing

# +
scene_setup()

# Instance group
g = lm.instancegroupnode()
lm.addchild(g, lm.primitivenode({
    'mesh': lm.asset('mesh_sphere'),
    'material': lm.asset('material_white')
}))

# Transformed instanced group
for y in np.linspace(-10,10,10):
    for x in np.linspace(-10,10,10):
        t = lm.transformnode(lm.translate(np.array([x,y,0])))
        lm.addchild(t, g)
        lm.addchild(lm.rootnode(), t)
# -

render_and_visualize()

# ### Multi-level instancing

# +
scene_setup()

# Initial group
g1 = lm.instancegroupnode()
lm.addchild(g1, lm.primitivenode({
    'mesh': lm.asset('mesh_sphere'),
    'material': lm.asset('material_white')
}))

# Second group using initial group as chilren
g2 = lm.instancegroupnode()
for y in np.linspace(-10,10,10):
    t = lm.transformnode(lm.translate(np.array([0,y,0])))
    lm.addchild(t, g)
    lm.addchild(g2, t)
    
# Add transformed second group to the root node
for x in np.linspace(-10,10,10):
    t = lm.transformnode(lm.translate(np.array([x,0,0])))
    lm.addchild(t, g2)
    lm.addchild(lm.rootnode(), t)
# -

render_and_visualize()

# ### Detecting invalid group

# +
# # Scene containing cyclic dependency is invalid
# scene_setup()
# try:
#     g = lm.group()
#     lm.node.add(g, g)
# except Exception:
#     traceback.print_exc()
# -


