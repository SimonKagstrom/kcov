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
        print("Hej")
    elif kalle > 128:
        print("Commodore 128")
    else:
        print("Hopp")
    sven_anka()

def sven_anka():
    'single-quoted line'
    jerker_anka()

def mats_anka():
    "single-quoted line"
    pass

dict = {
    2 : [ 1, 2, 3],
    3 : [ 4,
         5, 6
        ],
    5 :
        {
         9 : 1,
         },
    6 :
        { },
    7 :
        { 1 : 2
         },
    8 :
        [
         1, 2
         ]
}

def jerker_anka():
    try:
        f = open("/tmp/a")
    except:
        print("Exception")
    finally:
        print("Finally finally!")
