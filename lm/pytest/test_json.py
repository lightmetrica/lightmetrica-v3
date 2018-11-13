""" JSON tests """
import pytest
from numpy.testing import assert_allclose
from pylm_test import json as m

def test_round_trip_simple():
    """ Tests simple types """
    # Null
    assert m.round_trip(None) == None
    # Boolean type
    assert m.round_trip(True) == True
    assert m.round_trip(False) == False
    # Integer type
    assert m.round_trip(42) == 42
    assert m.round_trip(-42) == -42
    # Floating point type
    assert m.round_trip(3.14) ==  pytest.approx(3.14)
    assert m.round_trip(-3.14) == pytest.approx(-3.14)
    # String
    assert m.round_trip('') == ''
    assert m.round_trip('hai domo') == 'hai domo'

def test_round_trip_sequence():
    """ Tests sequence type """
    # Empty
    assert m.round_trip([]) == []
    # Simple
    assert m.round_trip([1,2,3]) == [1,2,3]
    assert m.round_trip([1.1,2.2,3.3]) ==  pytest.approx([1.1,2.2,3.3])
    assert m.round_trip(['a','ab','abc']) == ['a','ab','abc']
    # Heterogeneous type
    assert m.round_trip([42,3.14,'hai domo']) == [42,pytest.approx(3.14),'hai domo']
    # Nested type
    assert m.round_trip([[],[[],[[],[]]]]) == [[],[[],[[],[]]]]

def test_round_trip_dict():
    """ Tests dict type """
    # Empty
    assert m.round_trip({}) == {}
    # Simple
    assert m.round_trip({'1':1,'3':2,'3':3}) == {'1':1,'3':2,'3':3}
    assert m.round_trip({'1':1.1,'3':2.2}) == {'1':pytest.approx(1.1),'3':pytest.approx(2.2)}
    assert m.round_trip({'1':'a','3':'ab','3':'abc'}) == {'1':'a','3':'ab','3':'abc'}
    # Heterogeneous type
    assert m.round_trip({'1':42,'3':3.14,'3':'hai domo'}) == {'1':42,'3':pytest.approx(3.14),'3':'hai domo'}
    # Nested type
    value = {
        # Sequence of dict
        '1': [
            {'1.1': 42},
            {'1.2': 43}
        ],
        # Dict of sequence
        '2': {
            '2.1': [1,2,3],
            '2.2': [4,5,6]
        },
        # Multiple nesting
        '3': {'3.1': {'3.2':{'3.3':{}}}}
    }
    assert m.round_trip(value) == value
    