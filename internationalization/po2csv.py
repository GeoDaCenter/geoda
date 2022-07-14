#!/usr/bin/python

from __future__ import print_function
import codecs
import sys
#import xlsxwriter

def dict2PO(po_items, file_path):
    f = open(file_path, 'w', encoding='utf-8')
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


def dict2csv(po_items, file_path):
    f = open(file_path, 'w', encoding='utf-8')
    line = 'msgid`msgstr`contributors\n'
    f.write(line)
    msgid_list = sorted(po_items)
    for msgid in msgid_list:
        msgstr, contrib = po_items[msgid]
        line = '[' + msgid + ']`[' + msgstr + ']`' + contrib + '\n'
        f.write(line)
    f.close()

def po2dict(po_file, result):
    mode = 0

    with open(po_file) as f:
        start = 0
        end = 0

        source = ''
        dest = ''
        contrib = ''
        next_contrib = ''

        for i, line in enumerate(f, 1):
            line = line.strip()
            if(line.startswith('#') or len(line) == 0):
                if (line.startswith('#contributors')):
                    if (mode == 2):
                        next_contrib = line.split(':')[1] 
                    else:
                        contrib = line.split(':')[1]

            elif (line.startswith('msgid') or (mode == 1 and not line.startswith('msgstr'))):
                if (mode == 2 and len(source + dest + contrib) > 0):
                    # store previously processed item if any
                    result[source] = [dest, contrib]
                    source = ''
                    dest = '' 
                    contrib = next_contrib

                start = line.find('"') + 1
                end = line.rfind('"')
                source += line[start: end]
                mode = 1
                
            elif (line.startswith('msgstr') or mode == 2):
                mode = 2
                start = line.find('"') + 1
                end = line.rfind('"')
                dest += line[start : end]

        if (mode == 2 and len(source + dest + contrib) > 0):
            # store previously processed item if any
            result[source] = [dest, contrib]

if __name__ == "__main__":
    if (len(sys.argv) != 3) :
        print("Usage: python po2csv.py po_file csv_file")
    else:
        po_file, csv_file = sys.argv[1:]
        if (po_file and csv_file):
            po_items = {}
            po2dict(po_file, po_items)
            dict2csv(po_items, csv_file)
