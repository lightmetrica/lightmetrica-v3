"""Compress all images inside a directory to JPG"""
import os
import imageio
import numpy as np
import argparse
import glob

# Parse command line arguments
parser = argparse.ArgumentParser(
    description='Compress all images inside a directory to JPG')
parser.add_argument(
    '--dir', type=str, required=True, help='Target directory')
args = parser.parse_args()

def gamma_correction(img):
    return np.clip(np.power(img,1.0/2.2),0,1)

# Enumerate all files inside a given directory
for file in glob.glob(os.path.join(args.dir, '*.pfm')):
    # Covert .pfm to .jpg
    print('Loading ', file)
    img = imageio.imread(file)
    img = np.flipud(gamma_correction(img) * 255).astype(np.uint8)
    # Save
    out = os.path.splitext(file)[0] + '.jpg'
    print('Saving ', out)
    imageio.imwrite(out, img)
