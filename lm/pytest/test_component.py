"""Component tests"""
import os
import pytest
import gc
from contextlib import contextmanager
import pylm as lm
from pylm_test import component as m

@contextmanager
def lm_logger_scope():
    """Enables logger in the context"""
    try:
        lm.log.init('logger::default', {})
        yield
    finally:
        lm.log.shutdown()

@contextmanager
def lm_plugin_scope(name):
    """Enables plugin in the context"""
    try:
        lm.comp.detail.loadPlugin(os.path.join(pytest.lm_binary_dir, name))
        yield
    finally:
        lm.comp.detail.unloadPlugins()

def test_construction():
    """Construction of instances"""
    with lm_logger_scope():
        # Create and use component inside python script
        p = m.createA1()
        assert p.f1() == 42
        assert p.f2(1, 2) == 3

        # Create component instance
        p = m.D.create('test::comp::d1', None, {'v1':42, 'v2':43})
        assert p.f() == 85

        # Use component inside native plugin
        with lm_plugin_scope('lm_test_plugin'):
            #p = m.createTestPlugin()
            #assert p.f() == 42
            p = m.TestPlugin.create('testplugin::construct', None, {'v1':42, 'v2':43})
            assert p.f() == -1
            # Free p before unloading the plugin
            del p
            gc.collect()

        # Extend component from python
        class A2(m.A):
            def f1(self):
                return 43
            def f2(self, a, b):
                return a * b

        class A3(m.A):
            def __init__(self, v):
                # We need explicit initialization (not super()) of the base class
                # See https://pybind11.readthedocs.io/en/stable/advanced/classes.html
                test_comp.A.__init__(self)
                self.v = v
            def f1(self):
                return self.v
            def f2(self, a, b):
                return self.v * self.v
    
        # Instantiate inside python script and use it in python
        p = A2()
        assert p.f1() == 43
        assert p.f2(2, 3) == 6
