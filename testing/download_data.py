#!/usr/bin/env python
#
# Download raw test data files, check integrity and untar

from __future__ import print_function, unicode_literals
import hashlib
import io
import os
import os.path
import urllib
import shutil
import sys
import tarfile
try:
    from urllib.request import urlretrieve
except ImportError:
    from urllib import urlretrieve

BASE_SOURCE = 'https://project-corryvreckan.web.cern.ch/project-corryvreckan/data/'
BASE_TARGET = 'data/'
# dataset names and corresponding checksum
DATASETS = {
    'timepix3tel_ebeam120': 'a196166ea38a14bbf00c2165a9aee37c291f1201ed39fd313cc6b3f25dfa225d',
    'timepix3tel_dut_atlaspix_ebeam120': 'b890d738574afcb741b87cc41a868e4ba5e3b77da037a6f006ea730eefc0c54a',
    'timepix3tel_dut150um_ebeam120_sim': 'e6de5f9fff6a2a284dc462a27073a76fdfd6a41436bdcec392e3f78276e9dbc6',
    'mimosa26tel_desy_5400MeV': '74313bddcc4ce2d8c727414390a9daedd87384a0e1eb4ee98a03b35db3289c76',
}

def sha256(path):
    """calculate sha256 checksum for the file and return in hex"""
    # from http://stackoverflow.com/a/17782753 with fixed block size
    algo = hashlib.sha256()
    with io.open(path, 'br') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            algo.update(chunk)
    return algo.hexdigest()

def check(path, checksum):
    """returns true if the file exists and matches the checksum"""
    if not os.path.isfile(path):
        print('...no file \'%s\' present' % path)
        return False
    if not sha256(path) == checksum:
        print('...file \'%s\' exists with wrong checksum, deleting' % path)
        os.remove(path)
        return False
    print('...checksum of file \'%s\' ok' % path)
    return True

def cleanup(name):
    """removes untar'ed folder of target if it exists"""
    target = os.path.join(BASE_TARGET, name)
    if os.path.exists(target) and os.path.isdir(target):
        print('...deleting existing folder \'%s\'' % target)
        try:
            shutil.rmtree(target)
        except shutil.Error as err:
            print('%s' % err)
    else:
        print('...no folder from previous run present')

def download(name, checksum):
    """download tarball if not found already with correct checksum"""
    # not sure if path.join works for urls
    source = BASE_SOURCE + name + '.tar.gz'
    target = os.path.join(BASE_TARGET, name + '.tar.gz')
    if not os.path.exists(BASE_TARGET):
        os.makedirs(BASE_TARGET)

    if not check(target, checksum):
        print('...downloading file \'%s\' to \'%s\'' % (source, target))
        urlretrieve(source, target)
        if not check(target, checksum):
            sys.exit('...checksum validation of file \'%s\' failed' % target)

def untar(name):
    source = os.path.join(BASE_TARGET, name + '.tar.gz')
    tar = tarfile.open(source)
    tar.extractall(BASE_TARGET)
    tar.close()
    print('...untar of file \'%s\' ok' % source)

if __name__ == '__main__':
    print('Starting dataset preparation...')
    if len(sys.argv) < 2:
        datasets = DATASETS
    else:
        datasets = {_: DATASETS[_] for _ in sys.argv[1:]}
    for name, checksum in datasets.items():
        print('Dataset: \'%s\'' % name)

        # Delete existing untarred files
        cleanup(name=name)

        # Download tarball if necessary
        download(name=name, checksum=checksum)

        # Untar anew from tarball
        untar(name)
