#!/usr/bin/env python

from setuptools import setup

version = '0.1.0'
repo_url = 'https://github.com/agerasev/imxdevtool'
download_url = repo_url + '/tarball/' + version

setup(name='iocdevtool',
      version=version,
      description='Suite for building, deploying and testing EPICS IOC for embedded devices',
      long_description=open('README.md').read(),
      long_description_content_type="text/markdown",
      url=repo_url,
      download_url=download_url,
      author='Alexey Gerasev',
      author_email='alexey.gerasev@gmail.com',
      keywords='epics ioc embedded test deploy',
      license='MIT',
      py_modules=['iocdevtool'],
      entry_points = {
        'console_scripts': ['iocdevtool=iocdevtool:main'],
      },
      classifiers=[
        'Topic :: Software Development :: Embedded Systems',
        'Topic :: Software Development :: Build Tools',
        'Topic :: Software Development :: Testing',
        'License :: OSI Approved :: MIT License',
        'Environment :: Console'
      ])
