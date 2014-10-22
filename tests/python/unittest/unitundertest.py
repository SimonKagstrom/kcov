#!/usr/bin/python

import sys

class testme:
    def __init__(self):
        print 'ctor'

    def testmethod1(self,a,b):
        print 'in testmethod1()'
        return a+b
