# ---
# jupyter:
#   jupytext:
#     cell_metadata_json: true
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

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_quad:
#
# Rendering quad
# ==========================
#
# This example describes how to render a simple scene containing a quad represented by two triangles. The code starts again with :cpp:func:`lm::init` function.
# -

import lmenv
lmenv.load('.lmenv')

import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

# + {"nbsphinx": "hidden"}
if not lm.Release:
    lm.debug.attach_to_debugger()
    lm.parallel.init('openmp', {
        'numThread': 1
    })

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Similarly we define the assets. In addition to ``film``, we define ``camera``, ``mesh``, and ``material``. Although the types of assets are different, we can use consistent interface to define the assets: ``lm::load_*()`` functions. Here we prepare for a pinhole camera (``camera::pinhole``), a raw mesh (``mesh::raw``), and a diffuse material (``material::diffuse``) with the corrsponding parameters. Please refer to :ref:`component_ref` for the detailed description of the parameters.

# +
# Film for the rendered image
film = lm.load_film('film', 'bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
camera = lm.load_camera('camera', 'pinhole', {
    'position': [0,0,5],
    'center': [0,0,0],
    'up': [0,1,0],
    'vfov': 30,
    'aspect': 16/9
})

# Load mesh with raw vertex data
mesh = lm.load_mesh('mesh', 'raw', {
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
material = lm.load_material('material', 'diffuse', {
    'Kd': [1,1,1]
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Next we will create a `scene` asset. The scene asset can also be created :cpp:func:`load_scene` function. Here, we will create ``scene::default`` asset.
# A scene internally `uses acceleration structure` for ray-scene intersections, which can be specified by ``accel`` parameter.
# -

accel = lm.load_accel('accel', 'sahbvh', {})
scene = lm.load_scene('scene', 'default', {
    'accel': accel.loc()
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# The scene of Lightmetrica is defined by a set of ``primitives``. A primitive specifies an object inside the scene by associating geometries and materials. We can define a primitive by :cpp:func:`lm::Scene::add_primitive` function.
#
# .. note::
#     If you need transformation applied to the geometry, you can use :cpp:func:`lm::Scene::add_transformed_primitive` function. The transformation is given by 4x4 matrix. 
#
# In this example we define two pritimives; one for camera and the other for quad mesh with diffuse material. We don't apply any transformation to the geometry, so we use :cpp:func:`lm::Scene::add_primitive` function.
#
# .. note::
#     Specifically, the scene is represented by a *scene graph*, a directed acyclic graph representing spatial structure and attributes of the scene. Each node of the scene graph describes either a primitive or a pritmive group. We provide a set of APIs to manipulate the structure of scene graph for advanced usage like instancing.
# -

scene.add_primitive({
    'camera': camera.loc()
})
scene.add_primitive({
    'mesh': mesh.loc(),
    'material': material.loc()
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# After the configuration of the primitives, we must `build` the scene, which can be done by :cpp:func:`lm::Scene::build` function.
# -

scene.build()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Nowe we are ready for rendering. Here we will use ``renderer::raycast`` asset, which takes ``scene`` as a parameter. The rendered image will out written in the film asset specified by ``output`` parameter. We can also configure the background color with ``bg_color`` parameter.
# -

renderer = lm.load_renderer('renderer', 'raycast', {
    'scene': scene.loc(),
    'output': film.loc(),
    'bg_color': [0,0,0]
})
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
