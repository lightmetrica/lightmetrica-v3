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

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_custom_renderer:
#
# Rendering with custom renderer
# ================================
#
# This example demostrates how to create user-defined renderer by implementing :cpp:class:`lm::Renderer` interface. The implementation is defined in ``renderer_ao.cpp`` (`code <https://github.com/hi2p-perim/lightmetrica-v3/tree/master/functest/renderer_ao.cpp>`__):
#
# .. literalinclude:: ../../functest/renderer_ao.cpp
#     :language: cpp
#     :lines: 6-
#
# In this time, we implemented two functions: :cpp:func:`lm::Component::construct` and :cpp:func:`lm::Renderer::render`. :cpp:func:`lm::Component::construct` function provides a type-agnostic way to initialize the instance with JSON values. You want to implement main logic of the renderer inside the :cpp:func:`lm::Renderer::render` function. We will not explain the detail here, but this renderer implements a simple ambient occlusion. As for the usage of APIs, please refer to the :ref:`corresponding references <api_ref>` for detail.
# -

import lmenv
env = lmenv.load('.lmenv')

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

lm.comp.load_plugin(os.path.join(env.bin_path, 'functest_renderer_ao'))

# +
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
lm.asset('camera1', 'camera::pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})

# OBJ model
lm.asset('obj1', 'model::wavefrontobj', {
    'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj')
})

# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

# Create primitives from model asset
lm.primitive(lm.identity(), {
    'model': lm.asset('obj1')
})
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::ao', {
    'output': lm.asset('film1'),
    'spp': 10
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
