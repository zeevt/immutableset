#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
with open(sys.argv[1]) as f:
  for s1,s2 in zip((l[:-1] for l in sys.stdin),(l[:-1] for l in f)):
    print "%s\t%s" % (s1,s2)
