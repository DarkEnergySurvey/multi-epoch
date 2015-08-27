#!/usr/bin/env python

import os,sys

def read_ascii_to_dict(filename,sep=' '):

    from despyastro import tableio

    mydict = {}
    # Get the header
    header = tableio.get_header(filename)
    header = header[1:].strip()
    keys   = header.split(sep)
    # Read filename coluns a tuple of lists of strings
    mytuple = tableio.get_str(filename,cols=range(len(keys)))

    # Repack as a dictionary
    for index, key in enumerate(keys):
        # Convert to float if possible
        try:
            if isinstance(float(mytuple[index][0]), float):
                mydict[key] = map(float,mytuple[index])
        except:
            mydict[key] = mytuple[index]
    return mydict


filename = sys.argv[1]
mydict = read_ascii_to_dict(filename)
print mydict.keys()

