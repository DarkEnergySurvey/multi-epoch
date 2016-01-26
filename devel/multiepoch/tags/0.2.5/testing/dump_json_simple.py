#!/usr/bin/env python

import json

json_file = 'test.json'

xx = [1,2,3,4]

o = open(json_file,"w")
o.write(json.dumps(xx,sort_keys=True,indent=4))
o.close()
