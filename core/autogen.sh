#!/bin/sh

srcroot=`pwd`

(cd l15 && autoreconf -if --warnings=all)
(cd bitcoin && autoreconf -if --warnings=all)
autoreconf -if --warnings=all
