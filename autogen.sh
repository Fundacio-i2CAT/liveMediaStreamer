#!/usr/bin/env bash

libtoolize
aclocal
autoconf
autoheader
automake -a -v
./configure $@

