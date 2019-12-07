"""Run all functional tests"""
import os
import sys
from colorama import Fore, Back, Style
import nbformat
from nbconvert.preprocessors import ExecutePreprocessor
import argparse
import jupytext
import nbmerge
from shutil import copyfile

def run_functests(output_dir, lmenv_path):
    # Output directory of executed notebooks
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    # Base directory of the functional tests.
    # That is, the directory where this script is located.
    base_path = os.path.dirname(os.path.realpath(__file__))

    # Copy .lmenv file to the functest directory
    copyfile(lmenv_path, os.path.join(base_path, '.lmenv'))

    # Tests
    tests = [
        #'example_blank',
        # 'example_quad',
        # 'example_raycast',
        # 'example_pt',
        'example_custom_material',
        'example_custom_renderer',
        'func_render_all',
        'func_render_instancing',
        'func_accel_consistency',
        'func_py_custom_material',
        'func_py_custom_renderer',
        'func_distributed_rendering',
        'func_distributed_rendering_ext',
        'func_error_handling',
        'func_obj_loader_consistency',
        'func_serial_consistency',
        'func_update_asset',
        'func_scheduler',
        'perf_accel',
        'perf_obj_loader',
        'perf_serial'
    ]

    # Execute tests
    for test in tests:
        print(Fore.GREEN + "Running test [name='{}']".format(test) + Style.RESET_ALL, flush=True)
        
        # Read the requested notebook
        nb = jupytext.read(os.path.join(base_path, test + '.py'))

        # Execute the notebook
        ep = ExecutePreprocessor(timeout=600)
        ep.preprocess(nb, {'metadata': {'path': base_path}})

        # Write result
        with open(os.path.join(output_dir, test + '.ipynb'), mode='w', encoding='utf-8') as f:
            nbformat.write(nb, f)

    # Merge executed notebooks
    print(Fore.GREEN + "Merging notebooks" + Style.RESET_ALL)
    notebook_paths = [os.path.join(output_dir, test + '.ipynb') for test in tests]
    nb = nbmerge.merge_notebooks(os.getcwd(), notebook_paths)
    with open(os.path.join(output_dir, 'merged.ipynb'), mode='w', encoding='utf-8') as f:
        nbformat.write(nb, f)

    # Notify success
    print(Fore.GREEN + "All tests have been executed successfully" + Style.RESET_ALL)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Execute all functional tests')
    parser.add_argument('--lmenv', type=str, help='Path to .lmenv file')
    parser.add_argument('--output-dir', nargs='?', type=str, default='executed_functest', help='Output directory of executed notebooks')
    args = parser.parse_args()
    run_functests(output_dir, lmenv)