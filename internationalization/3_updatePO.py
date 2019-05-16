#!/usr/bin/python

from __future__ import print_function
import re
import csv
import sys
import argparse
from po2csv import po2dict, dict2PO

def updatePO(po_file, all_items):
    new_items = all_items.copy()

    # read from po_file
    current_items = {}
    po2dict(po_file, current_items)

    # use all_items as ground
    msgid_list = all_items.keys()
    for msgid in msgid_list:
        if msgid in current_items:
            new_items[msgid] = current_items[msgid] 

    return new_items


parser = argparse.ArgumentParser(description='Update existing PO file (e.g. zh_CN.po, specified using argument --input) using POT files (geoda.pot and xrc.pot extracted from source code. New msgid with empty msgstr will be added into existing PO file and saved into a new file specified by argument --output')
parser.add_argument('pot_files', type=str, nargs='+', help='paths of template POT files')
parser.add_argument('--input', required=True, dest='input_po_file', type=str, help='path of an existing to-be-updated po_file')
parser.add_argument('--output', required=True, dest='output_po_file', help='path of an output po_file')
args = parser.parse_args()

if __name__ == "__main__":
    pot_files = args.pot_files
    exist_po = args.input_po_file
    output_po = args.output_po_file

    # construct a dictionary from all POT file, 
    # so that newly added msgid will be included
    all_items = {}
    for pot in pot_files:
        po2dict(pot, all_items)

    all_items = updatePO(exist_po, all_items)
    dict2PO(all_items, output_po)
