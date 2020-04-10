## Process this file with automake to produce Makefile.in

EXTRA_DIST = logmoko-user-guide.md

all : 
	hash pandoc && pandoc logmoko-user-guide.md  > logmoko-user-guide-single.html  || echo "No docs generated as pandoc command does not exist"

