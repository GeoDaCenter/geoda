import os

'''
This script generates PO files from xrc files
This script will only work on OSX (maybe linux)
This script should be run from the directory internationalization/
'''

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False
    
def process_xrc(xrcfile):
    # run wxrc --gettext command to get string files from xrc resource files
    txtfile = xrcfile[6:-3] + 'txt'
    cmd = 'wxrc --gettext -o {} {}'.format(txtfile, xrcfile)
    os.system(cmd)
    return txtfile
    
    
def create_po(txtfiles):
    pofile = 'xrc.pot'
    en_strings = {}
    for txtfile in txtfiles:
        f = open(txtfile)
        lines = f.readlines()
        for line in lines:
            if line[0] == '#':
                continue
            en_str = line[3:-4]
            if len(en_str) > 0 and not is_number(en_str):
                en_strings[en_str] = True
        f.close()  
        os.system('rm ' + txtfile)

    o = open(pofile, 'w')
    lines = []
    for msgid in en_strings.keys():
        o.write('msgid \"{}\"\n'.format(msgid))
        o.write('msgstr \"\"\n')
        o.write('\n')
        
    o.flush()
    o.close()

xrcfiles = ['../rc/menus.xrc', '../rc/dialogs.xrc', '../rc/data_viewer_dialogs.xrc', '../rc/toolbar.xrc']

txtfiles = [process_xrc(xrcfile) for xrcfile in xrcfiles]
create_po(txtfiles)
