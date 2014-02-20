#!/usr/bin/env python
# encoding: utf-8

# Based on http://pymotw.com/2/sys/tracing.html, "Tracing a program as it runs"

import sys

def trace_lines(frame, event, arg):
    if event != 'line':
        return
    co = frame.f_code
    func_name = co.co_name
    line_no = frame.f_lineno
    filename = co.co_filename
    print 'L %s line %s' % (filename, line_no)

def trace_calls(frame, event, arg):
    if event != 'call':
        return
    co = frame.f_code
    func_name = co.co_name
    line_no = frame.f_lineno
    filename = co.co_filename
    print 'C %s of %s' % (filename, line_no)
    return trace_lines

def c(input):
    print 'input =', input
    print 'Leaving c()'

def b(arg):
    val = arg * 5
    c(val)
    print 'Leaving b()'

def a():
    b(2)
    print 'Leaving a()'
    
TRACE_INTO = ['b']

sys.settrace(trace_calls)
a()

