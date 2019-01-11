"""Rendering with path tracing

Example:
$ PYTHONPATH="../;../build/bin/Release" python pt.py \
    --obj ./scenes/fireplace_room/fireplace_room.obj \
    --out result.pfm \
    --spp 10 --len 20 \
    --width 1920 --height 1080 \
    --eye 5.101118 1.083746 -2.756308 \
    --lookat 4.167568 1.078925 -2.397892 \
    --vfov 43.001194
"""

import lightmetrica as lm
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--obj', type=str, required=True)
parser.add_argument('--out', type=str, required=True)
parser.add_argument('--spp', type=int, required=True)
parser.add_argument('--len', type=int, required=True)
parser.add_argument('--width', type=int, required=True)
parser.add_argument('--height', type=int, required=True)
parser.add_argument('--eye', nargs=3, type=float, required=True)
parser.add_argument('--lookat', nargs=3, type=float, required=True)
parser.add_argument('--vfov', type=float, required=True)
args = parser.parse_args()

# Initialize the framework
lm.init({
    'numThreads': -1
})

# Define assets
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': args.width,
    'h': args.height
})

# Pinhole camera
lm.asset('camera1', 'camera::pinhole', {
    'film': 'film1',
    'position': args.eye,
    'center': args.lookat,
    'up': [0,1,0],
    'vfov': args.vfov
})

# OBJ model
lm.asset('obj1', 'model::wavefrontobj', {'path': args.obj})

# Define scene primitives
# Camera
lm.primitive(lm.identity(), {'camera': 'camera1'})

# Create primitives from model asset
lm.primitives(lm.identity(), 'obj1')

# Render an image
lm.build('accel::sahbvh', {})
# _begin_render
lm.render('renderer::pt', {
    'output': 'film1',
    'spp': args.spp,
    'maxLength': args.len
})
# _end_render

# Save rendered image
lm.save('film1', args.out)
