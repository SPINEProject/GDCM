## SPINE BUILD
0. Install cmake and ccmake. For Ubuntu 22.04 run: `apt install cmake-curses-gui`

1. git clone https://github.com/SPINEProject/GDCM.git
2. mkdir gdcmbin
3. cd gdcmbin
4. ccmake ../GDCM
5. Press 'c' (configure)
6. Press 'c' (configure) again, configure with the following values :

# For SPINE 2.0
# Options for ccmake
- GDCM_BUILD_APPLICATIONS          ON
- GDCM_BUILD_EXAMPLES              OFF
- GDCM_BUILD_SHARED_LIBS           ON
- GDCM_BUILD_TESTING               OFF
- GDCM_DOCUMENTATION               OFF
- GDCM_USE_VTK                     OFF
- GDCM_WRAP_CSHARP                 OFF
- GDCM_WRAP_JAVA                   OFF
- GDCM_WRAP_PHP                    OFF
- GDCM_WRAP_PYTHON                 OFF
- GDCM_BUILD_DOCBOOK_MANPAGES      OFF

7. If success, option 'g' becomes available. Press 'g' (generate)
8. make
9. (suggested/optional) make install


# For SPINE 1.0
- Swith option ```GDCM_BUILD_APPLICATIONS``` to ```ON```



This is the source code of GDCM. It is available from sf.net website.
Official GIT repository is at:

  https://sourceforge.net/p/gdcm/gdcm/

For a general introduction/features/limitations/requirement please
refer to

  http://gdcm.sourceforge.net/

Just a quick note on the build process of GDCM. GDCM build process
make use of the cmake software(*). This allow us:
1. To get rid of the autoconf/autotools insanity
2. Transparently generate Unix Makefiles, NMake Makefiles,
VS8/9/10 Solution, Xcode projects, etc.
3. Automatic nightly testing, one of the most important things
for a robust library/software development process. GDCM development is develop
based on the XP definition, and to preserve backward compatibility
make sure that code is working from one release to another: each night
we configure, we build and we test GDCM. The result are then sent to
the dashboard located at:

  https://open.cdash.org/index.php?project=GDCM

A continuous dashboard also makes sure that any commit did not introduce
any error on another platform, a warning or a broken test...

Therefore you should be able to use GDCM from the bleeding edge without
knowing too much about what is going on. All you need to do is have a look
at the GDCM dashboard, and if your platform is 'green' then you can
update your git copy and compile safely knowing that there is very little chance
that something won't work. Cheers !


(*) http://www.cmake.org for more information

For more help you can go online in the GDCM Wiki:
* http://gdcm.sourceforge.net/

In Particular:
* http://gdcm.sourceforge.net/wiki/index.php/GDCM_Release_2.0
* http://gdcm.sourceforge.net/wiki/index.php/FAQ

And a page describing each tool can be found at:
* http://gdcm.sourceforge.net/wiki/index.php/End_User_Applications

Eg:
* http://gdcm.sourceforge.net/wiki/index.php/Gdcminfo

Need VTK:
* http://gdcm.sourceforge.net/wiki/index.php/Gdcmviewer
