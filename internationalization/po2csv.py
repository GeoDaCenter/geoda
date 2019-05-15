#!/usr/bin/python

from __future__ import print_function
import re
import sys
#import xlsxwriter

def dict2PO(po_items, file_path):
    f = open(file_path, 'w')
    msgid_list = sorted(po_items)
    for msgid in msgid_list:
        msgstr, contrib = po_items[msgid]
        line = '#contributors:' + contrib + '\n'
        f.write(line)
        line = 'msgid "' + msgid + '"\n'
        f.write(line)
        line = 'msgstr "' + msgstr + '"\n'
        f.write(line)
        f.write('\n')
    f.close()

def dict2excel(po_items, file_path):
    # Create a workbook and add a worksheet.
    workbook = xlsxwriter.Workbook(file_path)
    worksheet = workbook.add_worksheet()
    row = 0
    col = 0

    msgid_list = sorted(po_items)
    for msgid in msgid_list:
        msgstr, contrib = po_items[msgid]
        worksheet.write(row, col,     msgid)
        worksheet.write(row, col + 1, msgstr)
        worksheet.write(row, col + 2, contrib)
        row += 1
    workbook.close()

def po2dict(po_file, result):
    mode = 0

    with open(po_file) as f:
        start = 0
        end = 0

        source = ''
        dest = ''
        contrib = ''

        for i, line in enumerate(f, 1):
            line = line.strip()
            if(line.startswith('#') or len(line) == 0):
                if (line.startswith('#contributors')):
                    contrib = line.split(':')[1]
                mode = 0

            elif (line.startswith('msgid') or (mode == 1 and not line.startswith('msgstr')) ):
                if (line.startswith('msgid')):
                    # store previously processed item if any
                    result[source] = [dest, contrib]
                    source = ''
                    dest = '' 
                    contrib = ''
                mode = 1
                start = line.find('"') + 1
                end = line.rfind('"')
                source += line[start: end]
                
            elif ( (mode == 1 and line.startswith('msgstr')) or mode == 2):
                mode = 2
                start = line.find('"') + 1
                end = line.rfind('"')
                dest += line[start : end]

def po2csv(po_file, csv_file):
    mode = 0

    with open(po_file) as f, open(csv_file, 'w') as c:
        start = 0
        end = 0

        source = ''
        dest = ''
        contrib = ''

        new_l = 'msgid`msgstr`contributors\n'
        c.write(new_l)

        for i, line in enumerate(f, 1):
            line = line.strip()
            if(line.startswith('#') or len(line) == 0):
                if (line.startswith('#contributors')):
                    contrib = line.split(':')[1]
                mode = 0
                continue

            elif (line.startswith('msgid') or (mode == 1 and not line.startswith('msgstr')) ):
                if (line.startswith('msgid') and len(source+dest) > 0):
                    # store previously processed item if any
                    new_l = '[' + source + ']`[' + dest + ']`' + contrib + '\n'
                    c.write(new_l)
                    source = ''
                    dest = '' 
                    contrib = ''
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
    if (len(sys.argv) != 3) :
        print("Usage: python po2csv.py po_file csv_file")
    else:
        po_file, csv_file = sys.argv[1:]
        if (po_file and csv_file):
            po2csv(po_file, csv_file)