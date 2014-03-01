def second_fn(arg):
    if arg > 5: # Comment here, but valid line
        print("Yep")
    # This is a comment
    print("Hopp")
    kalle_anka()
    arne_anka()

def kalle_anka():
    '''
    satt
    pa en planka
'''
    pass

def arne_anka():
    b = """
    satt
    inte
    pa
    en
    planka
"""
    print(b)
    jens_anka()

def viktor_anka():
    """Single-line multi-line string"""

    return 0

def jens_anka():
    v = """Single-line multi-line string"""

    return v
