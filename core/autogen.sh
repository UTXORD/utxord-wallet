#!/bin/sh

srcroot=`pwd`

(cd l15 && autoreconf -if --warnings=all)
(cd l15/node && autoreconf -if --warnings=all)
autoreconf -if --warnings=all
