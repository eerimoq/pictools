#!/usr/bin/env python

from setuptools import setup
from setuptools import find_packages
import re


def find_version():
    return re.search(r"^__version__ = '(.*)'$",
                     open('pic32tools.py', 'r').read(),
                     re.MULTILINE).group(1)


setup(name='pic32tools',
      version=find_version(),
      description='PIC32 tools.',
      long_description=open('README.rst', 'r').read(),
      author='Erik Moqvist',
      author_email='erik.moqvist@gmail.com',
      license='MIT',
      classifiers=[
          'License :: OSI Approved :: MIT License',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 3',
      ],
      keywords=['pic32', 'programmer'],
      url='https://github.com/eerimoq/pic32tools',
      install_requires=[
          'pyserial',
          'bincopy'
      ],
      entry_points = {
          'console_scripts': ['pic32tools=pic32tools:_main']
      })
