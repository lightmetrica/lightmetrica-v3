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

# ## Distributed rendering with extension
#
# This test demonstrates distributed rendering with user-defined component.

# %load_ext autoreload
# %autoreload 2

import lmenv
env = lmenv.load('.lmenv')

import os
import imageio
import pandas as pd
import numpy as np
import multiprocessing as mp
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmscene
import lightmetrica as lm

os.getpid()

# %load_ext lightmetrica_jupyter

# ### Worker process
#
# To create an process on Jupyter notebook in Windows, we need to separate the function to be processed in a different file and add the invocation of the process must be enclosed by `if __name__ == '__main__'` clause.

# + {"magic_args": "_lm_renderer_ao.py"}
# %%writefile _lm_renderer_ao.py
import lightmetrica as lm
import pickle
import numpy as np

@lm.pylm_component('renderer::ao')
class Renderer_AO(lm.Renderer):
    """Simple ambient occlusion renderer"""

    def construct(self, prop):
        self.film = lm.Film.cast_from(lm.comp.get(prop['output']))
        if self.film is None:
            return False
        self.spp = prop['spp']
        return True
    
    def save(self):
        return pickle.dumps((self.film.loc(), self.spp))
    
    def load(self, s):
        loc, self.spp = pickle.loads(s)
        self.film = lm.Film.cast_from(lm.comp.get(loc))

    def render(self, scene):
        w = self.film.size().w
        h = self.film.size().h
        rng = lm.Rng(42)
        def process(index, threadid):
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
        lm.parallel.foreach(w*h, process)


# + {"magic_args": "_run_worker_process.py"}
# %%writefile _run_worker_process.py
import os
import uuid
import traceback
import lightmetrica as lm
import _lm_renderer_ao
def run_worker_process():
    try:
        lm.init()
        lm.info()
        lm.log.set_severity(1000)
        lm.log.log(lm.log.LogLevel.Err, lm.log.LogLevel.Info, '', 0, 'pid={}'.format(os.getpid()))
        lm.distributed.worker.init({
            'name': uuid.uuid4().hex,
            'address': 'localhost',
            'port': 5000,
            'numThreads': 1
        })
        lm.distributed.worker.run()
        lm.distributed.shutdown()
        lm.shutdown()
    except Exception:
        tr = traceback.print_exc()
        lm.log.log(lm.log.LogLevel.Err, lm.log.LogLevel.Info, '', 0, str(tr))


# -

from _run_worker_process import *
if __name__ == '__main__':
    pool = mp.Pool(2, run_worker_process)

# ### Master process

import _lm_renderer_ao

lm.init()
lm.log.init('logger::jupyter', {})
lm.progress.init('progress::jupyter', {})
lm.distributed.master.init({
    'port': 5000
})
lm.distributed.master.print_worker_info()

lmscene.load(env.scene_path, 'fireplace_room')
lm.build('accel::sahbvh', {})
lm.asset('film_output', 'film::bitmap', {'w': 320, 'h': 180})
lm.renderer('renderer::ao', {
    'output': lm.asset('film_output'),
    'spp': 3
})

lm.distributed.master.allow_worker_connection(False)
lm.distributed.master.sync()
lm.render()
lm.distributed.master.gather_film(lm.asset('film_output'))
lm.distributed.master.allow_worker_connection(True)

img = np.copy(lm.buffer(lm.asset('film_output')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()

# Termination of the worker process is necessary for Windows
# because fork() is not supported in Windows.
# cf. https://docs.python.org/3/library/multiprocessing.html#contexts-and-start-methods
pool.terminate()
pool.join()


