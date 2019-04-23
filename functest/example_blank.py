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
# Rendering blank image
# ==========================
#
# Let's start to use Lightmetrica by rendering a blank image.
# For C++, the first line includes everything necessary to use Lightmetrica.
# For Python, we import the ``lightmetrica`` module, where we use ``lm`` as an alias of ``lightmetrica`` for simplicity.
#
# .. note::
#    You can ignore the comments starting from ``_begin_*`` and ``_end_*``. These comments are only used for the documentation purpose.
# -

import lmfunctest as ft

import lightmetrica as lm

# %load_ext lightmetrica_jupyter

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# After including header, you can initialize the framwork by calling :cpp:func:`lm::init` function. You can pass various arguments to configure the framework to the funciton, but here we keep it empty so that everything is configured to be default.
# -

lm.init()

lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')

lm.info()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Next we define `assets` necessary to dispatch renderer, like materials, meshes, etc. In this example, we only need a `film` to which the renderer outputs the image. We can define assets by :cpp:func:`lm::asset` function. The first argument (``film``) specifies the name of the asset to be referenced. The second argument (``film::bitmap``) is given as the type of the assets formatted as ``<type of asset>::<implementation>`` where the last argument (``{...}``) specifies the parameters passed to the instance. ``film::bitmap`` takes two parameters ``w`` and ``h`` which respectively specify width and height of the film.
# -

lm.asset('film', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Now we are ready for rendering. :cpp:func:`lm::render` function dispatches rendering where we speficy type of the renderer and the parameters as arguments. ``renderer::blank`` is a toy renderer that only produces a blank image to the film specifies by ``film`` parameter where we can use predefined ID of the asset. Also, we can specify the background color by ``color`` parameter.
# -

lm.render('renderer::blank', {
    'output': lm.asset('film'),
    'color': [1,1,1]
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# After rendering, the generated image is kept in ``film``. :cpp:func:`lm::save` function outputs this film to the disk as an image.
# -

lm.save(lm.asset('film'), 'blank.png')

import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt

img = imageio.imread('blank.png')
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()

img = np.flip(np.copy(lm.buffer(lm.asset('film'))), axis=0)
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Finally, we gracefully shutdown the framework with :cpp:func:`lm::shutdown` function.
# -

lm.shutdown()
