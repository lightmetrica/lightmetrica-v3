""" JSON tests """
import pylm_test_module as pylm_test

def test_round_trip_simple():
    """ Tests simple types """
    # Null
    assert pylm_test.round_trip(None) == None
    # Boolean type
    assert pylm_test.round_trip(True) == True
    assert pylm_test.round_trip(False) == False
    # Integer type
    assert pylm_test.round_trip(42) == 42
    assert pylm_test.round_trip(-42) == -42
    # Floating point type
    assert pylm_test.round_trip(3.14) == 3.14
    assert pylm_test.round_trip(-3.14) == -3.14
    # String
    assert pylm_test.round_trip('') == ''
    assert pylm_test.round_trip('hai domo') == 'hai domo'

def test_round_trip_sequence():
    """ Tests sequence type """
    # Empty
    assert pylm_test.round_trip([]) == []
    # Simple
    assert pylm_test.round_trip([1,2,3]) == [1,2,3]
    assert pylm_test.round_trip([1.1,2.2,3.3]) == [1.1,2.2,3.3]
    assert pylm_test.round_trip(['a','ab','abc']) == ['a','ab','abc']
    # Heterogeneous type
    assert pylm_test.round_trip([42,3.14,'hai domo']) == [42,3.14,'hai domo']
    # Nested type
    assert pylm_test.round_trip([[],[[],[[],[]]]]) == [[],[[],[[],[]]]]

def test_round_trip_dict():
    """ Tests dict type """
    # Empty
    assert pylm_test.round_trip({}) == {}
    # Simple
    assert pylm_test.round_trip({'1':1,'3':2,'3':3}) == {'1':1,'3':2,'3':3}
    assert pylm_test.round_trip({'1':1.1,'3':2.2,'3':3.3}) == {'1':1.1,'3':2.2,'3':3.3}
    assert pylm_test.round_trip({'1':'a','3':'ab','3':'abc'}) == {'1':'a','3':'ab','3':'abc'}
    # Heterogeneous type
    assert pylm_test.round_trip({'1':42,'3':3.14,'3':'hai domo'}) == {'1':42,'3':3.14,'3':'hai domo'}
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
            '2.2': [1.1,2.2,3.3]
        },
        # Multiple nesting
        '3': {'3.1': {'3.2':{'3.3':{}}}}
    }
    assert pylm_test.round_trip(value) == value
    