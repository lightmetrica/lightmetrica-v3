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

os.getpid()


# %load_ext lightmetrica_jupyter

@lm.pylm_component('renderer::ao')
class Renderer_AO(lm.Renderer):
    """Simple ambient occlusion renderer"""
    
    def construct(self, prop):
        self.film = lm.Film.castFrom(lm.comp.get(prop['output']))
        if self.film is None:
            return False
        self.spp = prop['spp']
        return True
    
    def render(self, scene):
        w = self.film.size().w
        h = self.film.size().h
        rng = lm.Rng(42)
        lm.progress.start(lm.progress.ProgressMode.Samples, w*h, 0)
        def process(index):
            x = index % w
            y = int(index / w)
            rp = np.array([(x+.5)/w, (y+.5)/h])
            ray = scene.primaryRay(rp, self.film.aspectRatio())
            hit = scene.intersect(ray)
            if hit is None:
                return
            V = 0
            for i in range(self.spp):
                n, u, v = hit.geom.orthonormalBasis(-ray.d)
                d = lm.math.sampleCosineWeighted(rng)
                r = lm.Ray(hit.geom.p, np.dot(d, [u,v,n]))
                if scene.intersect(r, lm.Eps, .2) is None:
                    V += 1
            V /= self.spp
            self.film.setPixel(x, y, np.full(3, V))
            lm.progress.update(y*w+x)
        for i in range(w*h):
            process(i)
        lm.progress.end()


lm.init('user::default', {})

lm.parallel.init('parallel::openmp', {
    'numThreads': 1
})

lm.log.init('logger::jupyter', {})

lm.progress.init('progress::jupyter', {})

lm.info()

# Scene
lm.asset('film_output', 'film::bitmap', {
    'w': 640,
    'h': 360
})
lmscene.load(env.scene_path, 'fireplace_room')

lm.build('accel::sahbvh', {})

lm.render('renderer::ao', {
    'output': lm.asset('film_output'),
    'spp': 5
})

img = np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)

f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()
