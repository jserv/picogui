#!/usr/bin/env python
import PGBuild.Repository.Tar
import PGBuild.CommandLine.Output

p = PGBuild.CommandLine.Output.Progress()
t = PGBuild.Repository.Tar.Repository("http://umn.dl.sourceforge.net/sourceforge/pgui/picogui-0.45.tar.gz")

t.download("foo", p)
