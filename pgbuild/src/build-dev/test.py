#!/usr/bin/env python
import PGBuild.Repository
import PGBuild.CommandLine.Output

p = PGBuild.CommandLine.Output.Progress()

PGBuild.Repository.open("http://umn.dl.sourceforge.net/sourceforge/pgui/picogui-0.45.tar.gz").update("release-fu", p)
PGBuild.Repository.open("http://navi.picogui.org/svn/picogui/trunk/pgbuild").update("svn-fu", p)
