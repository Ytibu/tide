#!/bin/sh

if [ ! -d bin/module ]
then
    mkdir bin/module
else
    unlink bin/project_name
    unlink bin/module/libproject_name.so
fi

cp ../bin/tide bin/project_name
cp lib/libproject_name.so bin/module/