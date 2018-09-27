*****************************************************************
*** Build Instructions for GeoDa.  Current as of GeoDa 1.5.23 ***
*****************************************************************

Overview: We assume the build machine hosts a recently-installed
clean OS.  This build guide contains notes on setting up the compile
environment, obtaining the GeoDa source files and dependent libraries,
compiling libraries and GeoDa, and finally packaging the program
for distribution and installation.

***************************************************
*** Building GeoDa for 64-bit CentOS 7
***************************************************

NOTE: This is just basic placeholder for now!  Not currently complete.

Build machine assumptions:
- clean CentOS 64-bit installation with all OS updates

1. Install C++ developer tools along with command-line subversion

2. Use SVN to check out GeoDa trunk:
 - From user's home directory: ~/
 - svn co https://geodacenter.repositoryhosting.com/svn/geodacenter_geoda/trunk trunk
 
3. cd to ~/trunk/BuildTools/centos

4. run ./build64.sh to download and build GeoDa and everything it depends upon

5. Package GeoDa for distribution / installation.

***************************************************
*** Building GeoDa for 64-bit CentOS 6 
***************************************************

1. wxwidgets 3.1 works with GTK2, which is only avaiable on CentOS6

2. webkitgtk is version 1.0 on CentOS6. Install: yum install webkitgtk-devel

3. yum install readline-devel, autoreconf, gtk2-devel

4. 
cp libraries/lib/libwx_gtk2u_xrc-3.1.so.0.0.0 build/plugins/libwx_gtk2u_xrc-3.1.so.0
cp libraries/lib/libwx_gtk2u_stc-3.1.so.0.0.0 build/plugins/libwx_gtk2u_stc-3.1.so.0
cp libraries/lib/libwx_gtk2u_richtext-3.1.so.0.0.0 build/plugins/libwx_gtk2u_richtext-3.1.so.0
cp libraries/lib/libwx_gtk2u_ribbon-3.1.so.0.0.0 build/plugins/libwx_gtk2u_ribbon-3.1.so.0
cp libraries/lib/libwx_gtk2u_propgrid-3.1.so.0.0.0 build/plugins/libwx_gtk2u_propgrid-3.1.so.0
cp libraries/lib/libwx_gtk2u_aui-3.1.so.0.0.0 build/plugins/libwx_gtk2u_aui-3.1.so.0
cp libraries/lib/libwx_gtk2u_gl-3.1.so.0.0.0 build/plugins/libwx_gtk2u_gl-3.1.so.0
cp libraries/lib/libwx_gtk2u_html-3.1.so.0.0.0 build/plugins/libwx_gtk2u_html-3.1.so.0
cp libraries/lib/libwx_gtk2u_webview-3.1.so.0.0.0 build/plugins/libwx_gtk2u_webview-3.1.so.0
cp libraries/lib/libwx_gtk2u_qa-3.1.so.0.0.0 build/plugins/libwx_gtk2u_qa-3.1.so.0
cp libraries/lib/libwx_gtk2u_adv-3.1.so.0.0.0 build/plugins/libwx_gtk2u_adv-3.1.so.0
cp libraries/lib/libwx_gtk2u_core-3.1.so.0.0.0 build/plugins/libwx_gtk2u_core-3.1.so.0
cp libraries/lib/libwx_baseu_xml-3.1.so.0.0.0 build/plugins/libwx_baseu_xml-3.1.so.0
cp libraries/lib/libwx_baseu_net-3.1.so.0.0.0 build/plugins/libwx_baseu_net-3.1.so.0
cp libraries/lib/libwx_baseu-3.1.so.0.0.0 build/plugins/libwx_baseu-3.1.so.0
