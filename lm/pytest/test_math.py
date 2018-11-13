""" Math tests """
import pytest
import numpy as np
import pylm as lm
from pylm_test import math as m

def test_as_arguments():
    """ Use vector types as arguments """
    #assert m.compSum3(lm.Vec3(np.array([1,2,3], dtype=np.float32))) == pytest.approx(6)
    assert m.compSum3(np.array([1,2,3], dtype=np.float32)) == pytest.approx(6)
