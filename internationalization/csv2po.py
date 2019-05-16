#!/usr/bin/python

from __future__ import print_function
import re
import sys

def csv2po(csv_file, po_file):
    with open(csv_file) as f, open(po_file, 'w') as o:
        for i, line in enumerate(f, 1):
            if (i == 0): 
                continue
            line = line.strip()
            if (len(line) == 0):
                continue
            msgid, msgstr, contrib = line.split('`')
            line = 'msgid "' + msgid + '"\n' 
            o.write(line)
            line = 'msgstr "' + msgstr + '"\n' 
            o.write(line)

if __name__ == "__main__":
    if (len(sys.argv) != 3) :
        print("Usage: python csv2po.py csv_file po_file")
    else:
        csv_file, po_file = sys.argv[1:]
        if (po_file and csv_file):
            csv2po(csv_file, po_file)