#!/usr/bin/python

from __future__ import print_function
import re
import csv
import sys

def po2dict(po_file, result):
    mode = 0

    with open(po_file) as f:
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
                    result[source] = dest
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

def updatePO(po_file, po_items):
    new_items = po_items.copy()

    current_items = {}
    po2dict(po_file, current_items)
    for msgid, msgstr in current_items.iteritems():
        new_items[msgid] = msgstr

    return new_items

def dict2Csv(po_items, file_path):
    f = open(file_path, 'w')
    msgid_list = po_items.keys()
    msgid_list.sort()
    for msgid in msgid_list:
        msgstr = po_items[msgid]
        line = '"' + msgid + '","' + msgstr + '"\n'
        f.write(line)
    f.close()

def dict2PO(po_items, file_path):
    f = open(file_path, 'w')
    msgid_list = po_items.keys()
    msgid_list.sort()
    for msgid in msgid_list:
        msgstr = po_items[msgid]
        line = 'msgid "' + msgid + '"\n'
        f.write(line)
        line = 'msgstr "' + msgstr + '"\n'
        f.write(line)
        f.write('\n')
    f.close()
    
all_items = {}
po2dict('geoda.po', all_items)
po2dict('xrc.po', all_items)

all_items = updatePO('./pofiles/zh_CN.po', all_items)
dict2Csv(all_items, './pofiles/new_zh_CN.csv')
dict2PO(all_items, './pofiles/new_zh_CN.po')