#!/usr/bin/env python

from setuptools import setup

version = '0.1.0'
repo_url = 'https://github.com/agerasev/script'
download_url = repo_url + '/tarball/' + version

setup(name='script',
      version=version,
      description='Suite for building, deploying and testing heterogenous software for i.MX* SoC family',
      long_description=open('README.md').read(),
      long_description_content_type="text/markdown",
      url=repo_url,
      download_url=download_url,
      author='Alexey Gerasev',
      author_email='alexey.gerasev@gmail.com',
      keywords='imx soc heterogenous build deploy test epics ioc freertos crosscompile',
      license='MIT',
      py_modules=['script'],
      entry_points = {
        'console_scripts': ['script=script:main'],
      },
      classifiers=[
        'Topic :: Software Development :: Embedded Systems',
        'Topic :: Software Development :: Build Tools',
        'Topic :: Software Development :: Testing',
        'License :: OSI Approved :: MIT License',
        'Environment :: Console'
      ])
