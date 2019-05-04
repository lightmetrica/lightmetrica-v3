"""Run all functional tests"""
import os
import sys
import subprocess as sp

def run_single(name):
    # Convert .py to .ipynb
    sp.call([
        'jupytext',
        '--to', 'notebook',
        name+'.py'
    ])

    # Execute the notebook
    sp.call([
        'jupyter', 'nbconvert',
        '--ExecutePreprocessor.timeout=600',
        '--to', 'notebook',
        '--execute', name+'.ipynb',
        '--allow-errors',
        '--output-dir=./executed_functest'
    ])

    # Override the original python file
    # and update timestamp to prevent error of jupytext
    sp.call([
        'jupytext',
        '--from', 'ipynb',
        '--to', 'py:light',
        name+'.ipynb'
    ])

tests = [
    'example_blank',
    'example_quad',
    'example_raycast',
    'example_pt',
    'example_custom_material',
    'example_custom_renderer',
    'example_cpp',
    'func_render_all',
    'func_render_instancing',
    'func_accel_consistency',
    'func_py_custom_material',
    'func_py_custom_renderer',
    'func_distributed_rendering',
    'func_distributed_rendering_ext',
    'func_error_handling',
    'func_obj_loader_consistency',
    'func_render_instancing',
    'func_serial_consistency',
    'func_update_asset',
    'perf_accel',
    'perf_obj_loader',
    'perf_serial'
]
for test in tests:
    print("Running test [name='{}']".format(test), flush=True)
    run_single(test)
