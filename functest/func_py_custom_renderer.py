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

# ## Custom renderer in Python
#
# This test demostrates how to create an custom renderer using component extension in Python.

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm


# %load_ext lightmetrica_jupyter

@lm.pylm_component('renderer::ao')
class Renderer_AO(lm.Renderer):
    """Simple ambient occlusion renderer"""
    
    def construct(self, prop):
        self.scene = lm.get_scene(prop['scene'])
        self.film = lm.get_film(prop['output'])
        if self.film is None:
            return False
        self.spp = prop['spp']
        return True
    
    def render(self):
        w = self.film.size().w
        h = self.film.size().h
        rng = lm.Rng(42)
        lm.progress.start(lm.progress.ProgressMode.Samples, w*h, 0)
        def process(index):
            x = index % w
            y = int(index / w)
            rp = np.array([(x+.5)/w, (y+.5)/h])
            ray = scene.primary_ray(rp, self.film.aspect_ratio())
            hit = scene.intersect(ray)
            if hit is None:
                return
            V = 0
            for i in range(self.spp):
                n, u, v = hit.geom.orthonormal_basis(-ray.d)
                d = lm.math.sample_cosine_weighted(rng)
                r = lm.Ray(hit.geom.p, np.dot(d, [u,v,n]))
                if scene.intersect(r, lm.Eps, .2) is None:
                    V += 1
            V /= self.spp
            self.film.set_pixel(x, y, np.full(3, V))
            lm.progress.update(y*w+x)
        for i in range(w*h):
            process(i)
        lm.progress.end()


lm.init()
lm.info()
lm.log.init('jupyter', {})
lm.progress.init('jupyter', {})
lm.parallel.init('openmp', {'num_threads': 1})

# Scene
film = lm.load_film('film_output', 'bitmap', {
    'w': 640,
    'h': 360
})
accel = lm.load_accel('accel', 'sahbvh', {})
scene = lm.load_scene('scene', 'default', {
    'accel': accel.loc()
})
lmscene.load(scene, env.scene_path, 'fireplace_room')
scene.build()

renderer = lm.load_renderer('renderer', 'ao', {
    'scene': scene.loc(),
    'output': film.loc(),
    'spp': 5
})
renderer.render()

img = np.flip(np.copy(film.buffer()), axis=0)
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()
