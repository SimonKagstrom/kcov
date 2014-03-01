def second_fn(arg):
    if arg > 5: # Comment here, but valid line
        print("Yep")
    # This is a comment
    print("Hopp")
    kalle_anka()
    arne_anka(arg)

def kalle_anka():
    '''
    satt
    pa en planka
'''
    pass

def arne_anka(arg):
    b = """
    satt
    inte
    pa
    en
    planka
"""
    print(b)
    jens_anka(arg)
    jens_anka(9)

def viktor_anka():
    """Single-line multi-line string"""

    return 0

def jens_anka(kalle):
    v = """Single-line multi-line string"""

    if kalle > 5:
        print "Hej"
    elif kalle > 128:
        print "Commodore 128"
    else:
        print "Hopp"
