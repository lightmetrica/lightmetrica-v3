"""Component tests"""
import os
import pytest
import gc
import pickle
from contextlib import contextmanager
import lightmetrica as lm
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
        lm.comp.loadPlugin(os.path.join(pytest.lmenv.bin_path, name))
        yield
    finally:
        lm.comp.unloadPlugins()

def test_construction():
    """Construction of instances"""
    with lm_logger_scope():
        # Create and use component inside python script
        # without using factory function
        p = m.createA1()
        assert p.f1() == 42
        assert p.f2(1, 2) == 3

        # -----------------------------------------------------------------------------------------

        # Use component inside native plugin
        with lm_plugin_scope('lm_test_plugin'):
            p = m.createTestPlugin()
            assert p.f() == 42
            # Free p before unloading the plugin
            del p
            gc.collect()

        # -----------------------------------------------------------------------------------------

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

        # -----------------------------------------------------------------------------------------

        # Native embeded plugin
        # w/o property
        p = m.A.create('test::comp::a1', '')
        assert p.f1() == 42
        assert p.f2(2, 3) == 5
        # w/ property
        p = m.D.create('test::comp::d1', '', {'v1':42, 'v2':43})
        assert p.f() == 85

        # -----------------------------------------------------------------------------------------

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

        # -----------------------------------------------------------------------------------------

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

def test_serialization():
    """Serialization of component instances"""
    with lm_logger_scope():
        # Component with serialization support
        class A_Serializable(m.A):
            def construct(self, prop):
                self.v = prop['v']
                return True
            def save(self):
                return str(self.v)
            def load(self, s):
                self.v = int(s)
            def f1(self):
                return self.v
        m.A.reg(A_Serializable, 'test::comp::serializable')
        
        # Create instance
        p = m.A.create('test::comp::serializable', '', {'v': 22})
        assert p.f1() == 22

        # Serialize it
        serialized = p.save()
        
        # Create another instance, deserialize it
        p2 = m.A.create('test::comp::serializable', '')
        p2.load(serialized)
        assert p2.f1() == 22

        # Same test with the instance created in C++
        assert m.roundTripSerializedA() == 23

        # Same test with lm.serial.save / load functions in C++
        assert m.roundTripSerializedA_UseSerial() == 23

        # -----------------------------------------------------------------------------------------

        # Using pickle for serializing member variables
        class A_SerializableWithPickle(m.A):
            def construct(self, prop):
                self.v1 = prop['v1']
                self.v2 = prop['v2']
                return True
            def save(self):
                return pickle.dumps((self.v1, self.v2))
            def load(self, s):
                self.v1, self.v2 = pickle.loads(s)
            def f1(self):
                return self.v1 + self.v2
        m.A.reg(A_SerializableWithPickle, 'test::comp::serializable_with_pickle')
        
        # Create instance
        p = m.A.create('test::comp::serializable_with_pickle', '', {'v1': 4, 'v2': 13})
        assert p.f1() == 17

        # Serialize it
        serialized = p.save()
        
        # Create another instance, deserialize it
        p2 = m.A.create('test::comp::serializable_with_pickle', '')
        p2.load(serialized)
        assert p2.f1() == 17

        # Same test with the instance created in C++
        assert m.roundTripSerializedA_WithPickle() == 48