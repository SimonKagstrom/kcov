#!/usr/bin/python

import sys
import unittest

class testcase (unittest.TestCase):

    def setUp(self):
        print 'in setUp()'
        self.unitundertest = unitundertest.testme()

    def testMethod1(self):
        print 'in testcase.testMethod1()'
        expected=4
        result= self.unitundertest.testmethod1(2,2)

        self.assertEqual(expected,result)


if __name__ == '__main__':
    sys.path.append('.')

    try:
        import unitundertest
    except IOError, e:
        print 'exception'

    unittest.main()
