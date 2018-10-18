""" pytest configuration """
import os
import sys
import time

def pytest_addoption(parser):
    """ Add command line options """
    parser.addoption("--lm",
                     action="store",
                     help="Binary directory of Lightmetrica")
    parser.addoption("--attach",
                     action="store_true",
                     help="Wait some seconds for being attached by a debugger")

def pytest_configure(config):
    """ Configure pytest """
    # Add Lightmetrica's binary directory to sys.path
    bin_dir = config.getoption("--lm")
    if bin_dir:
        sys.path.insert(0, bin_dir)

    # Wait for being attached
    attach = config.getoption("--attach")
    if attach:
        wait_duration = 10
        for i in reversed(range(1,wait_duration+1)):
            print('Waiting for being attached in %d seconds (Process ID: %d)' % (i, os.getpid()))
            sys.stdout.flush()
            time.sleep(1)
