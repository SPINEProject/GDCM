#!/bin/sh

cd $HOME/Dashboards/GDCMScripts && git pull

PATH=$HOME/Software/cmake-2.8.2-Linux-i386/bin:$PATH

ctest -S brocoli-vtk-gcc-64-makefiles.cmake -V -O brocoli-vtk-gcc-64-makefiles.log
ctest -S brocoli-gcc-64-makefiles.cmake -V -O brocoli-gcc-64-makefiles.log

ctest -S brocoli-gdcm-gcc-64-cov.cmake -V -O brocoli-gdcm-gcc-64-cov.log
