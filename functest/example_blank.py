# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.1.3
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_blank:
#
# Rendering blank image
# ==========================
#
# Let's start to use Lightmetrica by rendering a blank image.
# We first import the ``lightmetrica`` module, where we use ``lm`` as an alias of ``lightmetrica`` for simplicity.
#
# .. note::
#     Although we recommend to use Python API to organize the experiments, similar APIs can be accessible from C++. See `example directory`_ for the detail.
#
#     .. _example directory: https://github.com/hi2p-perim/lightmetrica-v3/tree/master/example
#
# .. note::
#     ``lmfunctest`` is a simple module to configure Lightmetrica envrionment from a file named ``.lmenv``. You want to create your own ``.lmenv`` file if you want to execute examples or tests by yourself. For detail, see todo.
# -

import lmfunctest as ft

import lightmetrica as lm

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Lightmetrica offers an extension for the Jupyter notebook to support logging or interactive progress reporting inside the notebook. The extension can be loaded by a line magic command as below.
# -

# %load_ext lightmetrica_jupyter

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# After importing the module, you can initialize the framwork by calling :cpp:func:`lm::init` function. You can pass various arguments to configure the framework to the function, but here we keep it empty so that everything is configured to be default.
# -

lm.init()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Logging and progress reporting in Jupyter notebook can be enabled by :cpp:func:`lm::log::init` and :cpp:func:`lm::progress::init` functions with corresponding types.
# -

lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Once the framework has been initialized properly, you can get an splash message using :cpp:func:`lm::info()` function.
# -

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

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# You can also visualize the film directly in Jupyter notebook. :cpp:func:`lm::buffer` gets the internal image data as numpy array which you can visualize using matplotlib. We rendered a white blank image thus the following image is as we expected.
# -

import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt

img = np.copy(lm.buffer(lm.asset('film')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Finally, we gracefully shutdown the framework with :cpp:func:`lm::shutdown` function. Usually you don't want to explicitly call shutdown function.
# -

lm.shutdown()
