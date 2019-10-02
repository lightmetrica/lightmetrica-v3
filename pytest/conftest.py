""" pytest configuration """
import os
import sys
import time
import json
from argparse import Namespace
import pytest

def pytest_addoption(parser):
    """Add command line options"""
    parser.addoption("--lmenv",
                     action="store",
                     required=True,
                     help="Path to .lmenv file")

def pytest_configure(config):
    """Configure pytest"""
    # Read .lmeenv file
    lmenv_path = config.getoption("--lmenv")
    with open(lmenv_path) as f:
        config = json.load(f)

    # Add root directory and binary directory to sys.path
    if config['path'] not in sys.path:
        sys.path.insert(0, config['path'])
    if config['bin_path'] not in sys.path:
        sys.path.insert(0, config['bin_path'])

    # Make lmenv accessible from the test code
    pytest.lmenv = Namespace(**config)
