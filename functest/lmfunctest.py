import sys
import json
import numpy as np
import imageio
from argparse import Namespace

def loadenv(config_path):
    """Load configuration file of Lightmetrica environment"""

    # Open config file
    with open(config_path) as f:
        config = json.load(f)

    # Add root directory and binary directory to sys.path
    if config['path'] not in sys.path:
        sys.path.insert(0, config['path'])
    if config['bin_path'] not in sys.path:
        sys.path.insert(0, config['bin_path'])

    return Namespace(**config)

# Environment configuration
env = loadenv('.lmenv')

def accels():
    """Names of acceleration structures"""
    return ['sahbvh', 'nanort']

def save(path, img):
    """Save image"""
    imageio.imwrite(path, np.clip(np.power(img, 1/2.2) * 256, 0, 255).astype(np.uint8))

def rmse(img1, img2):
    return np.sqrt(np.mean((img1 - img2) ** 2))

def rmse_pixelwised(img1, img2):
    return np.sqrt(np.sum((img1 - img2) ** 2, axis=2) / 3)