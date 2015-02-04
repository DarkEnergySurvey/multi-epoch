#!/usr/bin/env python

import glob
from distutils.core import setup

bin_files = glob.glob("bin/*.py") + glob.glob("bin/*.txt") 
etc_files = glob.glob("etc/*.*")

setup(name='multiepoch',
      version='0.1beta',
      description='The DESDM multi-epoch pipeline',
      license = "GPL",
      author='Felipe Menanteau',
      author_email='felipe@illinois.edu',
      packages=[
          'multiepoch',
          'multiepoch.tasks',
          'multiepoch.config',
          ],
      package_dir = {'': 'python'},
      scripts    = bin_files,           # Clean this up FM
      data_files = [('etc',etc_files)], # Clean this up FM
      )

