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

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Rendering quad
# ==========================
#
# This section describes how to render a simple scene containing a quad represented by two triangles. From this example we do not provide the full source code inside this page. Please follow the files inside ``example`` directory if necessary.
#
# The code starts again with :cpp:func:`lm::init` function. Yet in this time, we specify the number of threads used for parallel processes by ``numThread`` parameter. The negative number configures the number of threads deducted from the maximum number of threads. For instance, if we specify ``-1`` on the machine with the maximum number of threads ``32``, the function configures the number of threads by ``31``. 
# -

import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lmfunctest as ft
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Next we define the assets. In addition to ``film``, we define ``camera``, ``mesh``, and ``material``. Although the types of assets are different, we can use consistent interface to define the assets. Here we prepare for a pinhole camera (``camera::pinhole``), a raw mesh (``mesh::raw``), and a diffuse material (``material::diffuse``) with the corrsponding parameters. Please refer to the corresponding pages for the detailed description of the parameters.

# +
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
lm.asset('camera1', 'camera::pinhole', {
    'position': [0,0,5],
    'center': [0,0,0],
    'up': [0,1,0],
    'vfov': 30
})

# Load mesh with raw vertex data
lm.asset('mesh1', 'mesh::raw', {
    'ps': [-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1],
    'ns': [0,0,1],
    'ts': [0,0,1,0,1,1,0,1],
    'fs': {
        'p': [0,1,2,0,2,3],
        'n': [0,0,0,0,0,0],
        't': [0,1,2,0,2,3]
    }
})

# Material
lm.asset('material1', 'material::diffuse', {
    'Kd': [1,1,1]
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# The scene of Lightmetrica is defined by a set of ``primitives``. A primitive specifies an object inside the scene by associating geometries and materials with transformation. We can define a primitive by :cpp:func:`lm::primitive` function where we specifies transformation matrix and associating assets as arguments.
# In this example we define two pritimives; one for camera and the other for quad mesh with diffuse material. Transformation is given by 4x4 matrix. Here we specified identify matrix meaning no transformation.

# +
# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

# Mesh
lm.primitive(lm.identity(), {
    'mesh': lm.asset('mesh1'),
    'material': lm.asset('material1')
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# For this example we used ``renderer::raycast`` for rendering. 
# This renderer internally uses acceleration structure for ray-scene intersections. 
# The acceleration structure can be given by :cpp:func:`lm::build` function. In this example we used ``accel::sahbvh``.
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::raycast', {
    'output': lm.asset('film1'),
    'color': [0,0,0]
})

img = np.flip(np.copy(lm.buffer(lm.asset('film1'))), axis=0)
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()
