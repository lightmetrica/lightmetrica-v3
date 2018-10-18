""" pytest configuration """
import sys

def pytest_addoption(parser):
    """ Add command line options """
    parser.addoption("--lm", action="store", help="Binary directory of Lightmetrica")

def pytest_configure(config):
    """ Configure pytest """
    # Add Lightmetrica's binary directory to sys.path
    bin_dir = config.getoption("--lm")
    if bin_dir:
        sys.path.insert(0, bin_dir)
        