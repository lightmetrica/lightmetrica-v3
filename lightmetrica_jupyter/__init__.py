"""IPython module for Lightmetrica"""
import os
import sys
import shutil
import filecmp
from tqdm import tqdm_notebook
import numpy as np
import matplotlib.pyplot as plt
import json
import platform

def lmenv():
    """Obtains environment settings"""
    with open('.lmenv') as f:
        env_params = json.load(f)
        return env_params[platform.node()]

def jupyter_init_config():
    """ init() configuration for jupyter notebook extension """
    return {
        # Configure logger for jupyter notebook
        'logger': 'logger::jupyter',
        
        # Configure progress reporter for jupyter notebook
        # We can utilize multiple instance of progress reporters.
        # In this example, we used progress bar UI (progress::jupyter)
        # and progress image reporter (progress::film_jupyter)
        'progress': {
            'progress::mux': [
                {
                    'progress::jupyter': {}
                },
                {
                    'progress::delay': {
                        'delay': 1000,
                        'progress': {
                            'progress::film_jupyter': {
                                'film': 'film1',
                                'size': [10,5]
                            }
                        }
                    }
                }
            ]
        }
    }

def update_lm_modules(configuration):
    """Copy Lightmetrica modules"""

    # Module directory
    if configuration is None:
        configuration = "Release"
    module_dir = lmenv()["module_dir"][configuration]

    # Temporary module directory
    temp_module_dir = os.path.join(os.getcwd(), 'temp')
    if not os.path.exists(temp_module_dir):
        os.makedirs(temp_module_dir)

    # Add temporary module path to sys.path
    sys.path.insert(0, temp_module_dir)

    # Copy required modules
    try:
        if sys.platform == 'win32':
            files = ['pylm.cp36-win_amd64.pyd', 'liblm.dll']
            if configuration == "Debug":
                files += ['pylm.pdb', 'liblm.pdb']
        elif sys.platform == 'linux':
            files = ['pylm.cpython-36m-x86_64-linux-gnu.so', 'liblm.so']
        for file in files:
            # Copy if the file is not up-to-date
            src = os.path.join(module_dir, file)
            dst = os.path.join(temp_module_dir, file)
            if not os.path.isfile(dst) or (os.path.isfile(dst) and not filecmp.cmp(src, dst)):
                print('Copying... [%s]' % file)
                shutil.copy(src, dst)
    except PermissionError as e:
        print('Requested module must be updated but hold by the running kernel. Restart the kernel to resolve this issue.')
        raise

    # Register some extensions for jupyter notebook
    import lightmetrica as lm

    @lm.pylm_component('logger::jupyter')
    class JupyterLogger(lm.log.LoggerContext):
        """Logger for jupyter notebook"""
        def construct(self, prop):
            self.severity = 0
            return True
        def log(self, level, severity, filename, line, message):
            if self.severity > severity:
                return
            print(message)
        def updateIndentation(self, n):
            pass
        def setSeverity(self, severity):
            self.severity = severity
        
    @lm.pylm_component('progress::jupyter')
    class JupyterProgress(lm.progress.ProgressContext):
        """Progress reporter for jupyter notebook"""
        def construct(self, prop):
            return True
        def start(self, total):
            self.pbar = tqdm_notebook(total=total)
        def update(self, processed):
            self.pbar.update(max(0, processed - self.pbar.n))
        def end(self):
            self.pbar.update(max(0, self.pbar.total - self.pbar.n))
            self.pbar.close()

    @lm.pylm_component('progress::film_jupyter')
    class JupyterFilmProgress(lm.progress.ProgressContext):
        """Progress reporter for film"""
        def construct(self, prop):
            self.film = prop['film']
            self.size = prop['size']
            return True
        def start(self, total):
            self.total = total
            self.fig = plt.figure(figsize=self.size)
            self.ax = self.fig.add_subplot(111)
            plt.ion()
            plt.tight_layout()
            self.fig.show()
            self.fig.canvas.draw()
        def update(self, processed):
            self.ax.clear()
            img = np.flip(lm.buffer(self.film), axis=0)
            self.ax.imshow(np.clip(np.power(img,1/2.2),0,1))
            self.fig.canvas.draw()
        def end(self):
            self.update(self.total)
            import time
            time.sleep(1)
            plt.show()
            plt.close()

def widen(arg):
    from IPython.core.display import display, HTML
    display(HTML("<style>.container { width:100% !important; }</style>"))

def load_ipython_extension(ip):
    """Register as IPython extension"""
    # Register line magic function
    ip.register_magic_function(update_lm_modules)
    ip.register_magic_function(widen)
