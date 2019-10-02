"""Helper script to run all unit tests"""
import os
import sys
import json
import argparse
import subprocess as sp
from colorama import Fore, Back, Style
import pytest
from functest.run_all import run_functests

def add_bool_arg(parser, name, default):
    """Add boolean option with mutually exclusive group"""
    # cf. https://stackoverflow.com/a/31347222/3127098
    dest = name.replace('-','_')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('--' + name, dest=dest, action='store_true')
    group.add_argument('--no-' + name, dest=dest, action='store_false')
    parser.set_defaults(**{dest:default})

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Execute all unit tests')
    parser.add_argument('--lmenv', type=str, help='Path to .lmenv file')
    parser.add_argument('--output-dir', nargs='?', type=str, default='executed_functest', help='Output directory of executed notebooks')
    add_bool_arg(parser, 'cpp-unit', True)
    add_bool_arg(parser, 'python-unit', True)
    add_bool_arg(parser, 'functest', False)
    args = parser.parse_args()

    # Execute C++ tests
    if args.cpp_unit:
        # Read .lmeenv file
        with open(args.lmenv) as f:
            config = json.load(f)
        print(Fore.GREEN + "------------------------" + Style.RESET_ALL)
        print(Fore.GREEN + "Executing C++ unit tests" + Style.RESET_ALL)
        print(Fore.GREEN + "------------------------" + Style.RESET_ALL, flush=True)
        sp.call([
            os.path.join(config['bin_path'], 'lm_test')
        ])

    # Execute python tests
    if args.python_unit:
        print(Fore.GREEN + "---------------------------" + Style.RESET_ALL)
        print(Fore.GREEN + "Executing Python unit tests" + Style.RESET_ALL)
        print(Fore.GREEN + "---------------------------" + Style.RESET_ALL, flush=True)
        base_path = os.path.dirname(os.path.realpath(__file__))
        pytest.main([
            os.path.join(base_path, 'pytest'),
            '--lmenv', args.lmenv
        ])

    # Execute functional tests
    if args.functest:
        print(Fore.GREEN + "--------------------------" + Style.RESET_ALL)
        print(Fore.GREEN + "Executing functional tests" + Style.RESET_ALL)
        print(Fore.GREEN + "--------------------------" + Style.RESET_ALL, flush=True)
        run_functests(args.output_dir, args.lmenv)