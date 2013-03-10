## Process this file with automake to produce Makefile.in

EXTRA_DIST = logmoko-user-guide.txt 

all : 
	hash asciidoc 2>/dev/null && asciidoc logmoko-user-guide.txt || echo "No docs generated as asciidoc command does not exist"

