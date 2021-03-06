#!/usr/bin/env python
# coding: utf-8

"""
Simple example showing how to use PinkTrace version constants
"""

from __future__ import print_function

import pinktrace

print("Built using %s %d.%d.%d%s" % (pinktrace.PACKAGE,
    pinktrace.VERSION_MAJOR, pinktrace.VERSION_MINOR, pinktrace.VERSION_MICRO,
    pinktrace.VERSION_SUFFIX), end="")

if pinktrace.GIT_HEAD:
    print(" git-%s" % pinktrace.GIT_HEAD)
else:
    print()
