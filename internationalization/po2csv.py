#!/usr/bin/python

from __future__ import print_function
import re
import csv
import sys

def po2csv(po_file, csv_file):
    mode = 0

    with open("geoda.po") as f, open('translations.csv', 'w') as c:
        start = 0
        end = 0
        source = ''
        dest = ''

        for i, line in enumerate(f, 1):
            if(line.startswith('#')):
                mode = 0
                continue

            if (line.startswith('msgid') or (mode == 1 and not line.startswith('msgstr')) ):
                if (line.startswith('msgid')):
                    # store previously processed item if any
                    if len(source) > 0:
                        new_l = '"' + source + '","' + dest + '"\n'
                        c.write(new_l)
                    source = ''
                    dest = '' 
                mode = 1
                start = line.find('"') + 1
                end = line.rfind('"')
                source += line[start: end]
                
            elif ( (mode == 1 and line.startswith('msgstr')) or mode == 2):
                mode = 2
                start = line.find('"') + 1
                end = line.rfind('"')
                dest += line[start : end]

if __name__ == "__main__":
    #po_file, csv_file = sys.argv[1:]
    po_file, csv_file = "geoda.po", "geoda.csv"
    if (po_file and csv_file):
        po2csv(po_file, csv_file)