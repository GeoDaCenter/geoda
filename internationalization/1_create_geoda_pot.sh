#!/bin/bash  
# create_geoda_pot.sh
# the first part will use find() command to find _() strings from 
find .. \( -name '*.cpp' -o -name '*.h' \) -not -path "../BuildTools/*" | xargs xgettext -d geoda -s --keyword=_ -p ./ -o geoda.pot
