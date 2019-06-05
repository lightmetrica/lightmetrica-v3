"""Entry point for Lightmetrica's Python binding"""

import os
import numpy as np

try:
    # Import lightmetrica module on development.
    # sys.path is assumed to include module and binary directories.
    from pylm import *
except:
    # Otherwise try to import from current directory
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
