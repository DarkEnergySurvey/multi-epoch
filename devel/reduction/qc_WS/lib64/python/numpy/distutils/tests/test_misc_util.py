#!/usr/bin/env python

import os
import sys
from numpy.testing import *
from numpy.distutils.misc_util import appendpath, minrelpath, gpaths, rel_path
from os.path import join, sep

ajoin = lambda *paths: join(*((sep,)+paths))

class test_appendpath(NumpyTestCase):

    def check_1(self):
        assert_equal(appendpath('prefix','name'),join('prefix','name'))
        assert_equal(appendpath('/prefix','name'),ajoin('prefix','name'))
        assert_equal(appendpath('/prefix','/name'),ajoin('prefix','name'))
        assert_equal(appendpath('prefix','/name'),join('prefix','name'))

    def check_2(self):
        assert_equal(appendpath('prefix/sub','name'),
                     join('prefix','sub','name'))
        assert_equal(appendpath('prefix/sub','sup/name'),
                     join('prefix','sub','sup','name'))
        assert_equal(appendpath('/prefix/sub','/prefix/name'),
                     ajoin('prefix','sub','name'))

    def check_3(self):
        assert_equal(appendpath('/prefix/sub','/prefix/sup/name'),
                     ajoin('prefix','sub','sup','name'))
        assert_equal(appendpath('/prefix/sub/sub2','/prefix/sup/sup2/name'),
                     ajoin('prefix','sub','sub2','sup','sup2','name'))
        assert_equal(appendpath('/prefix/sub/sub2','/prefix/sub/sup/name'),
                     ajoin('prefix','sub','sub2','sup','name'))

class test_minrelpath(NumpyTestCase):

    def check_1(self):
        import os
        n = lambda path: path.replace('/',os.path.sep)
        assert_equal(minrelpath(n('aa/bb')),n('aa/bb'))
        assert_equal(minrelpath('..'),'..')
        assert_equal(minrelpath(n('aa/..')),'')
        assert_equal(minrelpath(n('aa/../bb')),'bb')
        assert_equal(minrelpath(n('aa/bb/..')),'aa')
        assert_equal(minrelpath(n('aa/bb/../..')),'')
        assert_equal(minrelpath(n('aa/bb/../cc/../dd')),n('aa/dd'))
        assert_equal(minrelpath(n('.././..')),n('../..'))
        assert_equal(minrelpath(n('aa/bb/.././../dd')),n('dd'))

class test_gpaths(NumpyTestCase):

    def check_gpaths(self):
        local_path = minrelpath(os.path.join(os.path.dirname(__file__),'..'))
        ls = gpaths('command/*.py', local_path)
        assert os.path.join(local_path,'command','build_src.py') in ls,`ls`
        f = gpaths('system_info.py', local_path)
        assert os.path.join(local_path,'system_info.py')==f[0],`f`

if __name__ == "__main__":
    NumpyTest().run()
