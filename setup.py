#!/usr/bin/env python

from setuptools import setup
import re


def find_version():
    return re.search(r"^__version__ = '(.*)'$",
                     open('pictools.py', 'r').read(),
                     re.MULTILINE).group(1)


setup(name='pictools',
      version=find_version(),
      description='PIC tools.',
      long_description=open('README.rst', 'r').read(),
      author='Erik Moqvist',
      author_email='erik.moqvist@gmail.com',
      license='MIT',
      classifiers=[
          'License :: OSI Approved :: MIT License',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 3',
      ],
      keywords=['pic', 'pic32', 'programmer'],
      url='https://github.com/eerimoq/pictools',
      install_requires=[
          'pyserial',
          'bincopy',
          'tqdm',
          'bitstruct'
      ],
      py_modules=['pictools'],
      entry_points = {
          'console_scripts': ['pictools=pictools:_main']
      })
