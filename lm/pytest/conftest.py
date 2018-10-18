import pytest
import sys

def pytest_addoption(parser):
    parser.addoption("--lm", action="store", help="Binary directory of Lightmetrica")

def pytest_configure(config):
    # Add Lightmetrica's binary directory to sys.path
    bin_dir = config.getoption("--lm")
    if bin_dir:
        sys.path.insert(0, bin_dir)