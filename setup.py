#!/usr/bin/env python


import os, glob, re
from distutils.core import setup
from distutils.command.install_lib import install_lib
import distutils

# Hack to keep the +x permisions on the multiepoch/tasks folder
# Inspired by http://stackoverflow.com/questions/18409296/package-data-files-with-executable-permissions
class my_install_lib(install_lib):
    def run(self):
        # Run install_lib first
        install_lib.run(self)

        # perform custom action on some files
        for fn in self.get_outputs():
            if re.search(r'multiepoch/tasks/[a-zA-Z0-9][a-zA-Z0-9_]*.py$', fn):
                print "Custom change +x for file: %s" % fn
                mode = ((os.stat(fn).st_mode) | 0555) & 07777
                distutils.log.info("changing mode of %s to %o", fn, mode)
                os.chmod(fn, mode)


## -------------
#bin_files = glob.glob("bin/*.py") + glob.glob("bin/*.txt")
bin_files = glob.glob("bin/*") 
etc_files = glob.glob("etc/*.*")
sql_files = glob.glob("example_queries/*")

setup(name='multiepoch',
      version='0.2.2',
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
      scripts    = bin_files,           
      data_files = [('etc',etc_files),
                    ('example_queries',sql_files),
                    ], # Clean this up FM
      cmdclass={'install_lib':my_install_lib}, # to use custom install of lib files
      )

