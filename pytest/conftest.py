""" pytest configuration """
import os
import sys
import time
import pytest

def pytest_addoption(parser):
    """ Add command line options """
    parser.addoption("--lm",
                     action="store",
                     #required=True,
                     help="Binary directory of Lightmetrica")
    parser.addoption("--attach",
                     action="store_true",
                     help="Wait some seconds for being attached by a debugger")

def pytest_configure(config):
    """ Configure pytest """
    pass

    # Add Lightmetrica's binary directory to sys.path
    #bin_dir = config.getoption("--lm")
    #sys.path.insert(0, bin_dir)

    # Inject binary directory inside pytest module
    # so that it can be accessed from any tests.
    #pytest.lm_binary_dir = os.path.abspath(bin_dir)

    # Wait for being attached
    # attach = config.getoption("--attach")
    # if attach:
    #     wait_duration = 10
    #     for i in reversed(range(1,wait_duration+1)):
    #         print('Waiting for being attached in %d seconds (Process ID: %d)' % (i, os.getpid()))
    #         sys.stdout.flush()
    #         time.sleep(1)
