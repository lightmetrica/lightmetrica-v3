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

# ## Rendering with serialized assets

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
lm.log.init('jupyter')
lm.progress.init('jupyter')
lm.info()

# ### Asset group
#
# *Asset group* is a special type of asset that can hold multiple instance of assets. By means of the asset group, we can hierarchcally manage the assets. Asset group can be created by ``lm.load_asset_group()`` function.

g = lm.load_asset_group('fireplace_room', '', {})

# An another asset can be loaded as a child of the asset group by calling ``lm.AssetGroup.load_*()`` member functions. The arguments are same as ``lm.load_*()`` functions. Note that the locator of the asset includes the id of the group.

camera = g.load_camera('camera1', 'pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})
model = g.load_model('model', 'wavefrontobj', {
    'path': os.path.join(env.scene_path, 'fireplace_room/fireplace_room.obj')
})
accel = g.load_accel('accel', 'sahbvh', {})
scene = g.load_scene('scene', 'default', {
    'accel': accel.loc()
})
scene.add_primitive({
    'camera': camera.loc()
})
scene.add_primitive({
    'model': model.loc()
})
scene.build()

print(scene.loc())

# Display a tree structure of the assets


# ### Serialization of asset

# An asset can be serialized into a disk as a binary stream. For instance, it is useful to accelerate the loading time of the assets in debug mode or in the repetitive experiments, since we can skip the precomputation along with loading of the asset.
#
# Serialization to a file can be done by ``lm.Component.serialize_to_file()`` function, where we give the path to the output file as an argument.

g.serialize_to_file('fireplace_room.serialized')

# Reset the internal state
lm.reset()

# The serialized asset can be loaded by ``lm.load_serialized_asset()`` funcction, where the first argument specifies the id of the asset and the second argument specifies the path to the serialized asset. Note that the id of the asset can be not always the same from the original asset before serialization.

lm.load_serialized_asset('fireplace_room', 'fireplace_room.serialized')

# ### Rendering with serialized asset
#
# We can render the image using the serializaed asset. Here we are using a locator directly instead of ``.loc()`` function, since the previously obtained reference (``scene``) became invalid.

# Rendering
film = g.load_film('film', 'bitmap', {
    'w': 1920,
    'h': 1080
})
renderer = lm.load_renderer('renderer', 'pt', {
    'scene': '$.assets.fireplace_room.scene',
    'output': film.loc(),
    'scheduler': 'sample',
    'spp': 1,
    'max_length': 20
})
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
