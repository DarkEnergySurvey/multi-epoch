#!/usr/bin/python

import sys
sys.path.insert(0,'/home/martelli/qc/pypeline')

import pipeline

from pipeline import etc

etc.ConfigPath = '/home/martelli/qc/pypeline/etc/'
etc.BinDir = '/home/martelli/qc/bin/'

a = pipeline.PipeLine()

fn=''

try:
	fn = sys.argv[1]
except:
	fn = 'pipe.xml'

a.loadConfig(fn)
a.execute()
