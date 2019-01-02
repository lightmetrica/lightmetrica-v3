"""Run all examples"""
import os
import sys
import subprocess as sp
import argparse

# Parse command line arguments
parser = argparse.ArgumentParser(
    description='Execute all examples')
parser.add_argument(
    '--lm', type=str, required=True, help='Lightmetrica executable directory')
parser.add_argument(
    '--scene', type=str, required=True, help='Scene directory')
args = parser.parse_args()

def execute_example(ex, params):
    print('Executing example [name=%s]' % ex)
    sys.stdout.flush()
    # Execute the example executable
    p = sp.Popen([
        os.path.join(args.lm, ex)
    ] + [str(v) for v in params])
    if p.wait() == 0:
        print('Success')
    else:
        print('Failure')

# Number of samples per pixel
spp = 1000

# Run all examples
execute_example('blank', [])
execute_example('quad', [])
execute_example('raycast', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'raycast_fireplace_room.pfm',
    1920, 1080,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('raycast', [
    os.path.join(args.scene, 'cornell_box/CornellBox-Sphere.obj'),
    'raycast_cornell_box.pfm',
    1920, 1080,
    0, 1, 5,
    0, 1, 0,
    30
])
execute_example('pt', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'pt_fireplace_room.pfm',
    spp, 20, 1920, 1080,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('pt', [
    os.path.join(args.scene, 'cornell_box/CornellBox-Sphere.obj'),
    'pt_cornell_box.pfm',
    spp, 20, 1920, 1080,
    0, 1, 5,
    0, 1, 0,
    30
])
execute_example('custom_material', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'custom_material.pfm',
    1920, 1080,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('custom_renderer', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'custom_renderer.pfm',
    spp, 1920, 1080,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
