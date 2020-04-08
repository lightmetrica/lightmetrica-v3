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

# ## Rendering with serialized assets
#
# This example explains how to render images with the serialized assets.

import lmenv
env = lmenv.load('.lmenv')

import os
import traceback
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

if not lm.Release:
    lm.debug.attach_to_debugger()

# ### Visualizing asset tree
#
# An asset can hold another assets in the instance. As a result, a created set of asset can constitute of an *asset tree*. We can visualize the structure of the tree using ``lm.debug.print_asset_tree()`` function.

accel = lm.load_accel('accel', 'sahbvh', {})
scene = lm.load_scene('scene', 'default', {
    'accel': accel.loc()
})

lm.debug.print_asset_tree()

# Clear the internal state
lm.reset()
lm.debug.print_asset_tree()

# ### Asset group
#
# *Asset group* is a special type of asset that can hold multiple instance of assets. By means of the asset group, we can hierarchcally manage the assets. Asset group can be created by ``lm.load_asset_group()`` function.

g = lm.load_asset_group('fireplace_room', 'default', {})

# An another asset can be loaded as a child of the asset group by calling ``lm.AssetGroup.load_*()`` member functions. The arguments are same as ``lm.load_*()`` functions. Note that the locator of the asset includes the id of the group.

camera = g.load_camera('camera1', 'pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194,
    'aspect': 16/9
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

lm.debug.print_asset_tree()

# ### Serialization of asset

# An asset can be serialized into a disk as a binary stream. For instance, it is useful to accelerate the loading time of the assets in debug mode or in the repetitive experiments, since we can skip the precomputation along with loading of the asset.
#
# Serialization to a file can be done by ``lm.Component.save_to_file()`` function. We give the path to the output file as an argument.

g.save_to_file('fireplace_room.serialized')

# Reset the internal state
lm.reset()
lm.debug.print_asset_tree()

# Note that serializing aseet group means serializing a subtree of the entire asset tree. The serialization process can fail if an asset being serialized (incl. child assets) contains external reference out of the subtree. 

accel = lm.load_accel('accel', 'sahbvh', {})
g = lm.load_asset_group('fireplace_room', 'default', {})
scene = g.load_scene('scene', 'default', {
    'accel': accel.loc()
})
lm.debug.print_asset_tree()

# Serialization will fail because
# accel is out of the subtree starting from g as a root.
try:
    g.save_to_file('failed.serialized')
except Exception:
    traceback.print_exc()

# Reset the internal state
lm.reset()
lm.debug.print_asset_tree()

# ### Loading serialized asset
#
# The serialized asset can be loaded by ``lm.load_serialized()`` funcction, where the first argument specifies the id of the asset and the second argument specifies the path to the serialized asset. Note that the id of the asset can be not always the same from the original asset before serialization.

lm.load_serialized('fireplace_room', 'fireplace_room.serialized')

lm.debug.print_asset_tree()

# Reset the internal state
lm.reset()
lm.debug.print_asset_tree()

# Also note that the serialized asset can be loaded in a different location in the asset tree, for instance, as a child of the different asset group. 

g = lm.load_asset_group('another_group', 'default', {})
g.load_serialized('fireplace_room', 'fireplace_room.serialized')
lm.debug.print_asset_tree()

# ### Rendering with serialized asset
#
# We can render the image using the serializaed asset. Here we are using a locator directly instead of ``.loc()`` function, since the previously obtained reference (``scene``) became invalid.

# Rendering
film = lm.load_film('film', 'bitmap', {
    'w': 1920,
    'h': 1080
})
renderer = lm.load_renderer('renderer', 'pt', {
    'scene': '$.assets.another_group.fireplace_room.scene',
    'output': film.loc(),
    'scheduler': 'sample',
    'spp': 1,
    'max_verts': 20
})
renderer.render()

img = np.copy(film.buffer())
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
