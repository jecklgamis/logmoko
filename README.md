logmoko
=======

A C logging framework

About Logmoko

  Logmoko is a small logging framework for use with the C programming language. 
  It allows user to control which log statements are logged based on a 
  configured log level and to which interface (e.g. console, file, remote 
  connection).

  Logmoko was developed from a refactored logging facility used in
  RCUNIT, a C unit testing framework. It was also inspired by modern loggers 
  used in Java.

Building Logmoko

Unix platform:

   1. Configure the package
        $./configure

      To configure the package for debug mode:

        $./configure --enable-debug=yes

      The --prefix={installation directory} can be given to the
      configure script. If not given, the default prefix (/usr/local) is
      used.

   2. Build the package, samples, basic tests, and documentation
        $make

   3. Install the package
        $make install

   4. To uninstall the package
        $make uninstall

   See the INSTALL file for more details.

What Gets Installed?
   Logmoko static library (liblogmoko.a) in {prefix}/lib
   Logmoko header files in {prefix}/include

Licenses
   LGPL 
  
