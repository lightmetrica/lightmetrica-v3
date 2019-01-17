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
parser.add_argument(
    '--spp', type=int, default=10, help='Number of samples per pixel')
parser.add_argument(
    '--width', type=int, default=1920, help='Width of rendererd images if configuable')
parser.add_argument(
    '--height', type=int, default=1080, help='Height of rendererd images if configuable')
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

# Run all examples
execute_example('blank', [])
execute_example('quad', [])
execute_example('raycast', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'raycast_fireplace_room.pfm',
    args.width, args.height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('raycast', [
    os.path.join(args.scene, 'cornell_box/CornellBox-Sphere.obj'),
    'raycast_cornell_box.pfm',
    args.width, args.height,
    0, 1, 5,
    0, 1, 0,
    30
])
execute_example('pt', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'pt_fireplace_room.pfm',
    args.spp, 20, args.width, args.height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('pt', [
    os.path.join(args.scene, 'cornell_box/CornellBox-Sphere.obj'),
    'pt_cornell_box.pfm',
    args.spp, 20, args.width, args.height,
    0, 1, 5,
    0, 1, 0,
    30
])
execute_example('custom_material', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'custom_material.pfm',
    args.width, args.height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('custom_renderer', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'custom_renderer.pfm',
    args.spp, args.width, args.height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('serialization', [
    os.path.join(args.scene, 'fireplace_room/fireplace_room.obj'),
    'serialization_fireplace_room.pfm',
    args.width, args.height,
    5.101118, 1.083746, -2.756308,
    4.167568, 1.078925, -2.397892,
    43.001194
])
execute_example('serialization', [
    os.path.join(args.scene, 'cornell_box/CornellBox-Sphere.obj'),
    'serialization_cornell_box.pfm',
    args.width, args.height,
    0, 1, 5,
    0, 1, 0,
    30
])