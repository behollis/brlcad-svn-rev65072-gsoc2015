#                      I N S T A L L T R E E . T C L
# BRL-CAD
#
# Copyright (c) 2002-2007 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this file; see the file named COPYING for more
# information.
#
###
#
# Author(s):
#    Bob Parker
#
# Description:
#    Script for building the install tree on Windows.
#


set rootDir [file normalize ../../..]
if {![file exists $rootDir]} {
    puts "$rootDir must exist and must be the root of the BRL-CAD source tree. "
    return
}

set installDir [file join $rootDir brlcadInstall]
if {[file exists $installDir]} {
    puts "$installDir already exists."
    return
}

# Look for required source files
set missingFile 0
if {![file exists [file join $rootDir src other iwidgets iwidgets.tcl.in]]} {
    puts "Missing [file join $rootDir src other iwidgets iwidgets.tcl.in]"
    set missingFile 1
}

if {![file exists [file join $rootDir src other iwidgets pkgIndex.tcl.in]]} {
    puts "Missing [file join $rootDir src other iwidgets pkgIndex.tcl.in]"
    set missingFile 1
}

if {![file exists [file join $rootDir src other tk win wish.exe.manifest].in]} {
    puts "Missing [file join $rootDir src other tk win wish.exe.manifest].in]"
    set missingFile 1
}

if {![file exists [file join $rootDir include conf MAJOR]]} {
    puts "Missing [file join $rootDir include conf MAJOR]]"
    set missingFile 1
}

if {![file exists [file join $rootDir include conf MINOR]]} {
    puts "Missing [file join $rootDir include conf MINOR]]"
    set missingFile 1
}

if {![file exists [file join $rootDir include conf PATCH]]} {
    puts "Missing [file join $rootDir include conf PATCH]]"
    set missingFile 1
}


if {$missingFile} {
    return
}


# Set BRL-CAD's version
set fd [open [file join $rootDir include conf MAJOR] "r"]
set major [read $fd]
close $fd

set fd [open [file join $rootDir include conf MINOR] "r"]
set minor [read $fd]
close $fd

set fd [open [file join $rootDir include conf PATCH] "r"]
set patch [read $fd]
close $fd

set major [string trim $major]
set minor [string trim $minor]
set patch [string trim $patch]

if {![string is int $major] ||
    ![string is int $minor] ||
    ![string is int $patch]} {
    puts "Failed to acquire BRL-CAD's version."
    puts "Bad value for one or more of the following:"
    puts "major - $major, minor - $minor, patch - $patch"
    return
}

set cadVersion "$major.$minor.$patch"
# End Set BRL-CAD's version

set shareDir [file join $installDir share brlcad $cadVersion]

# More initialization for tclsh
# Show tclsh where to find required tcl files
lappend auto_path [file join $rootDir src other tcl library]
lappend auto_path [file join $rootDir misc win32-msvc8 tclsh library]

file copy -force [file join $rootDir src other incrTcl itcl library itcl.tcl] [file join $rootDir misc win32-msvc8 tclsh library]
file copy -force [file join $rootDir src other incrTcl itcl library pkgIndex.tcl] [file join $rootDir misc win32-msvc8 tclsh library]
source [file join $rootDir src tclscripts ami.tcl]
package require Itcl
# End More initialization for tclsh


# Create install directories
puts "Creating [file join $installDir]"
file mkdir [file join $installDir]
puts "Creating [file join $installDir bin]"
file mkdir [file join $installDir bin]
puts "Creating [file join $installDir lib]"
file mkdir [file join $installDir lib]
puts "Creating [file join $installDir lib iwidgets4.0.2]"
file mkdir [file join $installDir lib iwidgets4.0.2]
puts "Creating [file join $installDir share]"
file mkdir [file join $installDir share]
puts "Creating [file join $installDir share brlcad]"
file mkdir [file join $installDir share brlcad]
puts "Creating [file join $shareDir]"
file mkdir [file join $shareDir]
puts "Creating [file join $shareDir plugins]"
file mkdir [file join $shareDir plugins]
puts "Creating [file join $shareDir plugins archer]"
file mkdir [file join $shareDir plugins archer]
puts "Creating [file join $shareDir plugins archer Utilities]"
file mkdir [file join $shareDir plugins archer Utilities]
puts "Creating [file join $shareDir plugins archer Wizards]"
file mkdir [file join $shareDir plugins archer Wizards]
puts "Creating [file join $shareDir db]"
file mkdir [file join $shareDir db]
#puts "Creating [file join $shareDir pix]"
#file mkdir [file join $shareDir pix]
puts "Creating [file join $shareDir sample_applications]"
file mkdir [file join $shareDir sample_applications]
# End Create install directories


# Copy files to the bin directory
puts "copy [file join $rootDir src archer archer] [file join $installDir bin]"
file copy [file join $rootDir src archer archer] [file join $installDir bin]
puts "copy [file join $rootDir src archer archer.bat] [file join $installDir bin]"
file copy [file join $rootDir src archer archer.bat] [file join $installDir bin]
puts "copy [file join $rootDir src mged mged.bat] [file join $installDir bin]"
file copy [file join $rootDir src mged mged.bat] [file join $installDir bin]
# End Copy files to the bin directory


# Copy files to the lib directory
puts "copy [file join $rootDir src other blt library] [file join $installDir lib blt2.4]"
file copy [file join $rootDir src other blt library] [file join $installDir lib blt2.4]
puts "copy [file join $rootDir src other tcl library] [file join $installDir lib tcl8.5]"
file copy [file join $rootDir src other tcl library] [file join $installDir lib tcl8.5]
puts "copy [file join $rootDir src other tk library] [file join $installDir lib tk8.5]"
file copy [file join $rootDir src other tk library] [file join $installDir lib tk8.5]
puts "copy [file join $rootDir src other incrTcl itcl library] [file join $installDir lib itcl3.4]"
file copy [file join $rootDir src other incrTcl itcl library] [file join $installDir lib itcl3.4]
puts "copy [file join $rootDir src other incrTcl itk library] [file join $installDir lib itk3.4]"
file copy [file join $rootDir src other incrTcl itk library] [file join $installDir lib itk3.4]
puts "copy [file join $rootDir src other iwidgets generic] [file join $installDir lib iwidgets4.0.2]"
file copy [file join $rootDir src other iwidgets generic] [file join $installDir lib iwidgets4.0.2 scripts]
# End Copy files to the lib directory


# Copy files to the share directories
puts "copy [file join $rootDir AUTHORS] [file join $shareDir]"
file copy [file join $rootDir AUTHORS] [file join $shareDir]
puts "copy [file join $rootDir COPYING] [file join $shareDir]"
file copy [file join $rootDir COPYING] [file join $shareDir]
puts "copy [file join $rootDir doc] [file join $shareDir]"
file copy [file join $rootDir doc] [file join $shareDir]
puts "copy [file join $rootDir doc html] [file join $shareDir]"
file copy [file join $rootDir doc html] [file join $shareDir]
puts "copy [file join $rootDir HACKING] [file join $shareDir]"
file copy [file join $rootDir HACKING] [file join $shareDir]
puts "copy [file join $rootDir INSTALL] [file join $shareDir]"
file copy [file join $rootDir INSTALL] [file join $shareDir]
puts "copy [file join $rootDir NEWS] [file join $shareDir]"
file copy [file join $rootDir NEWS] [file join $shareDir]
puts "copy [file join $rootDir README] [file join $shareDir]"
file copy [file join $rootDir README] [file join $shareDir]
puts "copy [file join $rootDir misc fortran_example.f] [file join $shareDir sample_applications]"
file copy [file join $rootDir misc fortran_example.f] [file join $shareDir sample_applications]
puts "copy [file join $rootDir misc vfont] [file join $shareDir]"
file copy [file join $rootDir misc vfont] [file join $shareDir]
puts "copy [file join $rootDir src tclscripts] [file join $shareDir]"
file copy [file join $rootDir src tclscripts] [file join $shareDir]
puts "copy [file join $rootDir src archer plugins utility.tcl] [file join $shareDir plugins archer]"
file copy [file join $rootDir src archer plugins utility.tcl] \
    [file join $shareDir plugins archer]
puts "copy [file join $rootDir src archer plugins wizards.tcl] [file join $shareDir plugins archer]"
file copy [file join $rootDir src archer plugins wizards.tcl] \
    [file join $shareDir plugins archer]
puts "copy [file join $rootDir src conv g-xxx.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src conv g-xxx.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src conv g-xxx_facets.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src conv g-xxx_facets.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src gtools g_transfer.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src gtools g_transfer.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src libpkg tpkg.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src libpkg tpkg.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src librt g_xxx.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src librt g_xxx.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src librt raydebug.tcl] [file join $shareDir sample_applications]"
file copy [file join $rootDir src librt raydebug.tcl] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src librt nurb_example.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src librt nurb_example.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src rt rtexample.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src rt rtexample.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src util pl-dm.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src util pl-dm.c] [file join $shareDir sample_applications]
puts "copy [file join $rootDir src util roots_example.c] [file join $shareDir sample_applications]"
file copy [file join $rootDir src util roots_example.c] [file join $shareDir sample_applications]
# End Copy files to the share directories


# Remove undesired directories/files as a result of wholesale copies
file delete -force [file join $installDir lib blt2.4 CVS]
file delete -force [file join $installDir lib blt2.4 Makefile.am]
file delete -force [file join $installDir lib blt2.4 dd_protocols CVS]
file delete -force [file join $installDir lib blt2.4 dd_protocols Makefile.am]
file delete -force [file join $installDir lib itcl3.4 CVS]
file delete -force [file join $installDir lib itcl3.4 Makefile.am]
file delete -force [file join $installDir lib itk3.4 CVS]
file delete -force [file join $installDir lib itk3.4 Makefile.am]
file delete -force [file join $installDir lib iwidgets4.0.2 CVS]
file delete -force [file join $installDir lib iwidgets4.0.2 Makefile.am]
file delete -force [file join $installDir lib iwidgets4.0.2 scripts CVS]
file delete -force [file join $installDir lib iwidgets4.0.2 scripts Makefile.am]
file delete -force [file join $installDir lib tcl8.5 CVS]
file delete -force [file join $installDir lib tcl8.5 dde CVS]
file delete -force [file join $installDir lib tcl8.5 encoding CVS]
file delete -force [file join $installDir lib tcl8.5 http CVS]
file delete -force [file join $installDir lib tcl8.5 http1.0 CVS]
file delete -force [file join $installDir lib tcl8.5 msgcat CVS]
file delete -force [file join $installDir lib tcl8.5 msgs CVS]
file delete -force [file join $installDir lib tcl8.5 opt CVS]
file delete -force [file join $installDir lib tcl8.5 platform CVS]
file delete -force [file join $installDir lib tcl8.5 reg CVS]
file delete -force [file join $installDir lib tcl8.5 tcltest CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Africa CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata America Argentina CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata America CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata America Indiana CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata America Kentucky CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata America North_Dakota CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Antarctica CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Arctic CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Asia CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Atlantic CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Australia CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Brazil CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Canada CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Chile CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Etc CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Europe CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Indian CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Mexico CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata Pacific CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata SystemV CVS]
file delete -force [file join $installDir lib tcl8.5 tzdata US CVS]
file delete -force [file join $installDir lib tk8.5 CVS]
#file delete -force [file join $installDir lib tk8.5 Makefile.am]
file delete -force [file join $installDir lib tk8.5 demos CVS]
#file delete -force [file join $installDir lib tk8.5 demos Makefile.am]
file delete -force [file join $installDir lib tk8.5 demos images CVS]
#file delete -force [file join $installDir lib tk8.5 demos images Makefile.am]
file delete -force [file join $installDir lib tk8.5 images CVS]
#file delete -force [file join $installDir lib tk8.5 images Makefile.am]
file delete -force [file join $installDir lib tk8.5 msgs CVS]
#file delete -force [file join $installDir lib tk8.5 msgs Makefile.am]
file delete -force [file join $installDir lib tk8.5 ttk CVS]
#file delete -force [file join $installDir lib tk8.5 ttk Makefile.am]

file delete -force [file join $shareDir doc .cvsignore]
file delete -force [file join $shareDir doc book Makefile.am]
file delete -force [file join $shareDir doc book CVS]
file delete -force [file join $shareDir doc CVS]
file delete -force [file join $shareDir doc html]
file delete -force [file join $shareDir doc legal Makefile.am]
file delete -force [file join $shareDir doc legal CVS]
file delete -force [file join $shareDir doc Makefile.am]

file delete -force [file join $shareDir html CVS]
file delete -force [file join $shareDir html Makefile.am]
file delete -force [file join $shareDir html manuals Anim_Tutorial CVS]
file delete -force [file join $shareDir html manuals Anim_Tutorial Makefile.am]
file delete -force [file join $shareDir html manuals archer CVS]
file delete -force [file join $shareDir html manuals archer Makefile.am]
file delete -force [file join $shareDir html manuals cadwidgets CVS]
file delete -force [file join $shareDir html manuals cadwidgets Makefile.am]
file delete -force [file join $shareDir html manuals CVS]
file delete -force [file join $shareDir html manuals libbu CVS]
file delete -force [file join $shareDir html manuals libbu Makefile.am]
file delete -force [file join $shareDir html manuals libdm CVS]
file delete -force [file join $shareDir html manuals libdm Makefile.am]
file delete -force [file join $shareDir html manuals librt CVS]
file delete -force [file join $shareDir html manuals librt Makefile.am]
file delete -force [file join $shareDir html manuals Makefile.am]
file delete -force [file join $shareDir html manuals mged .cvsignore]
file delete -force [file join $shareDir html manuals mged CVS]
file delete -force [file join $shareDir html manuals mged Makefile.am]
file delete -force [file join $shareDir html manuals mged animmate CVS]
file delete -force [file join $shareDir html manuals mged animmate Makefile.am]
file delete -force [file join $shareDir html manuals shaders CVS]
file delete -force [file join $shareDir html manuals shaders Makefile.am]
file delete -force [file join $shareDir html ReleaseNotes CVS]
file delete -force [file join $shareDir html ReleaseNotes Makefile.am]
file delete -force [file join $shareDir html ReleaseNotes Rel5.0 CVS]
file delete -force [file join $shareDir html ReleaseNotes Rel5.0 Makefile.am]
file delete -force [file join $shareDir html ReleaseNotes Rel5.0 Summary CVS]
file delete -force [file join $shareDir html ReleaseNotes Rel5.0 Summary Makefile.am]
file delete -force [file join $shareDir html ReleaseNotes Rel6.0 CVS]
file delete -force [file join $shareDir html ReleaseNotes Rel6.0 Makefile.am]

file delete -force [file join $shareDir tclscripts CVS]
file delete -force [file join $shareDir tclscripts archer CVS]
file delete -force [file join $shareDir tclscripts archer images CVS]
file delete -force [file join $shareDir tclscripts archer images Makefile.am]
file delete -force [file join $shareDir tclscripts archer images Themes CVS]
file delete -force [file join $shareDir tclscripts archer images Themes Makefile.am]
file delete -force [file join $shareDir tclscripts archer images Themes Crystal CVS]
file delete -force [file join $shareDir tclscripts archer images Themes Crystal Makefile.am]
file delete -force [file join $shareDir tclscripts archer images Themes Crystal_Large CVS]
file delete -force [file join $shareDir tclscripts archer images Themes Crystal_Large Makefile.am]
file delete -force [file join $shareDir tclscripts archer images Themes Windows CVS]
file delete -force [file join $shareDir tclscripts archer images Themes Windows Makefile.am]
file delete -force [file join $shareDir tclscripts archer Makefile.am]
file delete -force [file join $shareDir tclscripts geometree CVS]
file delete -force [file join $shareDir tclscripts geometree Makefile.am]
file delete -force [file join $shareDir tclscripts lib CVS]
file delete -force [file join $shareDir tclscripts lib Makefile.am]
file delete -force [file join $shareDir tclscripts mged CVS]
file delete -force [file join $shareDir tclscripts mged Makefile.am]
file delete -force [file join $shareDir tclscripts nirt CVS]
file delete -force [file join $shareDir tclscripts nirt Makefile.am]
file delete -force [file join $shareDir tclscripts pl-dm CVS]
file delete -force [file join $shareDir tclscripts pl-dm Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard CVS]
file delete -force [file join $shareDir tclscripts rtwizard Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeA CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeA Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeB CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeB Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeC CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeC Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeD CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeD Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeE CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeE Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeF CVS]
file delete -force [file join $shareDir tclscripts rtwizard examples PictureTypeF Makefile.am]
file delete -force [file join $shareDir tclscripts rtwizard lib CVS]
file delete -force [file join $shareDir tclscripts rtwizard lib Makefile.am]
file delete -force [file join $shareDir tclscripts sdialogs CVS]
file delete -force [file join $shareDir tclscripts sdialogs Makefile.am]
file delete -force [file join $shareDir tclscripts sdialogs scripts CVS]
file delete -force [file join $shareDir tclscripts sdialogs scripts Makefile.am]
file delete -force [file join $shareDir tclscripts swidgets CVS]
file delete -force [file join $shareDir tclscripts swidgets Makefile.am]
file delete -force [file join $shareDir tclscripts swidgets images CVS]
file delete -force [file join $shareDir tclscripts swidgets images Makefile.am]
file delete -force [file join $shareDir tclscripts swidgets scripts CVS]
file delete -force [file join $shareDir tclscripts swidgets scripts Makefile.am]
file delete -force [file join $shareDir tclscripts util CVS]
file delete -force [file join $shareDir tclscripts util Makefile.am]
file delete -force [file join $shareDir tclscripts Makefile.am]
file delete -force [file join $shareDir vfont CVS]
file delete -force [file join $shareDir vfont Makefile.am]
# End Remove undesired directories/files as a result of wholesale copies


# Create iwidgets.tcl
set fd1 [open [file join $rootDir src other iwidgets iwidgets.tcl.in] r]
set lines [read $fd1]
close $fd1
puts "Creating [file join $installDir lib iwidgets4.0.2 iwidgets.tcl]"
set fd2 [open [file join $installDir lib iwidgets4.0.2 iwidgets.tcl] w]
set lines [regsub -all {@ITCL_VERSION@} $lines "3.4"]
set lines [regsub -all {@IWIDGETS_VERSION@} $lines "4.0.2"]
puts $fd2 $lines
close $fd2
# End Create iwidgets.tcl


# Create pkgIndex.tcl for iwidgets
set fd1 [open [file join $rootDir src other iwidgets pkgIndex.tcl.in] r]
set lines [read $fd1]
close $fd1
puts "Creating [file join $installDir lib iwidgets4.0.2 pkgIndex.tcl]"
set fd2 [open [file join $installDir lib iwidgets4.0.2 pkgIndex.tcl] w]
set lines [regsub -all {@IWIDGETS_VERSION@} $lines "4.0.2"]
puts $fd2 $lines
close $fd2
# End Create pkgIndex.tcl for iwidgets


# Create wish.exe.manifest
set fd1 [open [file join $rootDir src other tk win wish.exe.manifest].in r]
set lines [read $fd1]
close $fd1
puts "Creating [file join $rootDir src other tk win wish.exe.manifest]"
set fd2 [open [file join $rootDir src other tk win wish.exe.manifest] w]
set lines [regsub -all {@TK_WIN_VERSION@} $lines "8.5"]
set lines [regsub -all {@MACHINE@} $lines "x86"]
puts $fd2 $lines
close $fd2
# End Create wish.exe.manifest


# Create the tclIndex files
#lappend auto_path 
make_tclIndex [list [file join $shareDir tclscripts]]
make_tclIndex [list [file join $shareDir tclscripts lib]]
make_tclIndex [list [file join $shareDir tclscripts archer]]
make_tclIndex [list [file join $shareDir tclscripts mged]]
make_tclIndex [list [file join $shareDir tclscripts rtwizard]]
make_tclIndex [list [file join $shareDir tclscripts util]]

#XXX More needed

# End Create the tclIndex files
