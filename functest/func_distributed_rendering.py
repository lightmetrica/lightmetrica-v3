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

# ## Distributed rendering
#
# This test demonstrates distributed rendering feature of Lightmetrica.

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

# + {"magic_args": "_run_worker_process.py"}
# %%writefile _run_worker_process.py
import os
import uuid
import traceback
import lightmetrica as lm
def run_worker_process():
    try:
        lm.init('user::default', {})
        lm.info()
        lm.log.setSeverity(1000)
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

lm.init()
lm.log.init('logger::jupyter', {})
lm.progress.init('progress::jupyter', {})
lm.distributed.master.init({
    'port': 5000
})

lm.distributed.master.printWorkerInfo()

lmscene.load(env.scene_path, 'fireplace_room')
lm.build('accel::sahbvh', {})
lm.asset('film_output', 'film::bitmap', {'w': 1920, 'h': 1080})
lm.renderer('renderer::raycast', {
    'output': lm.asset('film_output')
})

lm.distributed.master.allowWorkerConnection(False)
lm.distributed.master.sync()
lm.render()
lm.distributed.master.gatherFilm(lm.asset('film_output'))
lm.distributed.master.allowWorkerConnection(True)

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


