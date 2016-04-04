#!/usr/bin/env python

import os
import sys

def read_astromatic_conf(configfile):

    config = {}
    for line in open(configfile).readlines():
        vals = line.split()

        if line[0] == "#":
            continue
        try:
            if vals[0] != "#":
                config[vals[0]] = vals[1]
        except:
            pass

    return config




if __name__ == "__main__":

    config_ref = sys.argv[1]
    config_new = sys.argv[2]
    conf_ref = read_astromatic_conf(config_ref)
    conf_new = read_astromatic_conf(config_new)

    print "# %-18s %-22s %-22s" % ('param',config_new, config_ref)
    for key in conf_new.keys():

        try:
            if conf_ref[key] != conf_new[key]:
                print "%-20s %-22s %-22s"  % (key,conf_new[key],conf_ref[key])
        except:
            print "%-20s %-22s ----------"  % (key,conf_new[key])


        
    
