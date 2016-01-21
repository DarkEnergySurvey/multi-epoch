#!/usr/bin/env python

import json

json_file = 'DES2246-4457_ccdinfo.json'
with open(json_file, 'rb') as fp:
        json_dict = json.load(fp)

for name in json_dict.keys():
    print name, json_dict[name]
