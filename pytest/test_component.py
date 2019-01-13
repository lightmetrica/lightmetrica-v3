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
        # without using factory function
        p = m.createA1()
        assert p.f1() == 42
        assert p.f2(1, 2) == 3

        # Use component inside native plugin
        with lm_plugin_scope('lm_test_plugin'):
            p = m.createTestPlugin()
            assert p.f() == 42
            # Free p before unloading the plugin
            del p
            gc.collect()

        # Extend component from python
        class A2(m.A):
            def f1(self):
                return 43
            def f2(self, a, b):
                return a * b
    
        # Instantiate inside python script and use it in python
        p = A2()
        assert p.f1() == 43
        assert p.f2(2, 3) == 6

        # Instantiate inside python script and use it in C++
        assert m.useA(p) == 86

        # Native embeded plugin
        # w/o property
        p = m.A.create('test::comp::a1', '')
        assert p.f1() == 42
        assert p.f2(2, 3) == 5
        # w/ property
        p = m.D.create('test::comp::d1', '', {'v1':42, 'v2':43})
        assert p.f() == 85
        # # w/ parent component
        # d = m.D.create('test::comp::d1', '', {'v1':42, 'v2':43})
        # e = m.E.create('test::comp::e1', d, None)
        # assert e.f() == 86
        # # w/ underlying component of the parent
        # d  = m.D.create('test::comp::d1', None, {'v1':42, 'v2':43})
        # e1 = m.E.create('test::comp::e1', d, None)
        # e2 = m.E.create('test::comp::e2', e1, None)
        # assert e2.f() == 87

        # Native external plugin
        with lm_plugin_scope('lm_test_plugin'):
            # w/o property
            p = m.TestPlugin.create('testplugin::default', '')
            assert p.f() == 42
            # w/ property
            p = m.TestPlugin.create('testplugin::construct', '', {'v1':42, 'v2':43})
            assert p.f() == -1
            del p
            gc.collect()

        # Define and register component implementation
        # Implement component A
        class A4(m.A):
            def f1(self):
                return 44
            def f2(self, a, b):
                return a - b
        # Register A4 to the framework
        m.A.reg(A4, 'test::comp::a4')

        # A5: with properties
        class A5(m.A):
            def construct(self, prop):
                self.v = prop['v']
                return True
            def f1(self):
                return self.v
            def f2(self, a, b):
                return a + b + self.v
        m.A.reg(A5, 'test::comp::a5')

        # Python plugin
        p = m.A.create('test::comp::a4', '')
        assert p.f1() == 44
        assert p.f2(2, 3) == -1

        # Python plugin instantiate from C++
        assert m.createA4AndCallFuncs() == (44, -1)
        assert m.createA5AndCallFuncs() == (7, 10)
