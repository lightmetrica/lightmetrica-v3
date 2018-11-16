"""Component tests"""
import pytest
from contextlib import contextmanager
import pylm as lm
from pylm_test import component as m

@contextmanager
def lm_logger():
    try:
        lm.log.init('logger::default', {})
        yield
    finally:
        lm.log.shutdown()


def test_construction():
    """Construction of instances"""
    with lm_logger():
        # Create and use component inside python script
        p = m.createA1()
        assert p.f1() == 42
        assert p.f2(1, 2) == 3

        # Use component inside native plugin
        