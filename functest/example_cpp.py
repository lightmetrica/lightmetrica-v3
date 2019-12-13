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

# ## C++ examples
#
# This example executes the examples written with C++ API.

import lmenv
env = lmenv.load('.lmenv')

import os
import sys
import subprocess as sp
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt

outdir = './output'
width = 1920
height = 1080


# +
def run_example(ex, params):
    print('Executing example [name=%s]' % ex)
    sys.stdout.flush()
    # Execute the example executable
    # We convert backslashes as path separator in Windows environment
    # to slashes because subprocess might pass unescaped backslash.
    sp.call([
        os.path.join(env.bin_path, ex)
    ] + [str(v).replace('\\', '/') for v in params])
    
def visualize(out):
    img = imageio.imread(out)
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
    plt.show()


# -

# ### pt.cpp

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Corresponding Python example: :ref:`example_pt`
# -

out = os.path.join(outdir, 'pt.pfm')
run_example('pt', [
    os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj'),
    out,
    10, 20,
    width, height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
visualize(out)

out = os.path.join(outdir, 'pt2.pfm')
run_example('pt', [
    os.path.join(env.scene_path, 'cornell_box/CornellBox-Sphere.obj'),
    out,
    10, 20,
    width, height,
    0, 1, 5,
    0, 1, 0,
    30
])
visualize(out)

# ### custom_renderer.cpp

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Corresponding Python example: :ref:`example_custom_renderer`
# -

out = os.path.join(outdir, 'custom_renderer.pfm')
run_example('custom_renderer', [
    os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj'),
    out,
    10,
    width, height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
visualize(out)
