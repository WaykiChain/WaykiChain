import sys, re

type_re = re.compile('.*\(type \(;[0-9]*;\) \(func \(param .*')

infile = open('test.wast', 'r')

for line in infile:
    if type_re.match(line):
        print line
