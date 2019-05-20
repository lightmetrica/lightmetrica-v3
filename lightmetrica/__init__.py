"""Entry point for Lightmetrica's Python binding"""

import os
import numpy as np

if 'LM_DEBUG' in os.environ and os.environ['LM_DEBUG'] == '1':
    from .pylm_debug import *
else:
    from .pylm import *

def pylm_component(name):
    """Decorator for registering a class to lightmetrica"""
    def pylm_component_(object):
        # Get base class
        base = object.__bases__[0]
        base.reg(object, name)
        return object
    return pylm_component_

def array(*args, **kwargs):
    """Numpy array with default floating point type of lightmetrica"""
    kwargs.setdefault('dtype', np.float32)
    return np.array(*args, **kwargs)
