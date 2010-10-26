#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys

handles = []
for filename in sys.argv[1:]:
  handles.append(open(filename))

done = False
while not done:
  lines = []
  for handle in handles:
    line = handle.readline()
    if not line:
      done = True
      break
    lines.append(line[:-1])
  if not done:
    print '\t'.join(lines)

for handle in handles:
  handle.close()
