""" JSON tests """
import pylm_test_module as pylm_test

def test_round_trip():
    """ Tests round trip of JSON types """

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

    # Null
    