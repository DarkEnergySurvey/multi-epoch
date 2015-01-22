#!/usr/bin/env python

import glob
from distutils.core import setup

bin_files = glob.glob("bin/*.py")

setup(name='multiepoch',
      version='0.1beta',
      description='The DESDM multi-epoch pipeline',
      license = "GPL",
      author='Felipe Menanteau',
      author_email='felipe@illinois.edu',
      packages=[
          'multiepoch',
          'multiepoch.tasks',
          ],
      package_dir = {'': 'python'},
      scripts=bin_files,
      data_files=[

     )

