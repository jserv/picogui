#!/usr/bin/tclsh
source picogui.tcl

#create a namespace to simulate object orientation
interp create tk

proc winfo {args} {
	global ids
	if {[lindex $args 0] == "id"} {
		if {[llength $args] != 2} {
			puts {wrong # args: should be "winfo id window"}
			return
		}
		set names [array names ids]
		if {[lsearch -exact $names [lindex $args 1]] ==-1} {
			puts "bad window path name \"[lindex $args 1]\""
			return
		}
		return $ids([lindex $args 1])
	}
}
proc toplevelfunctions {window id args} {
	if {[llength $args] == 0 } {
		puts "wrong # args: should be \"$window option ?arg arg ...?\""
		return
	} elseif {[lindex $args 0]=="cget"} {
		puts "I want information form $window with id of $id"
	} elseif {[lindex $args 0]=="configure"} {
		puts "Setting information for $window with id of $id"
	} else {
		puts "bad option \"[lindex $args 0]\": must be cget or configure"
	}
}
proc mapfunction {master slave} {
	set remap [format "proc $master {args} {
		return %stk eval $slave %s%s
	}" {[} {$args} "]"]
	eval $remap
}
proc toplevel {name args} {
	global ids
	set id [pgCreateWidget popup]
	set ids($name) $id
	set defaultparent_$id $id
	set defaultrship_$id inside
	tk alias $name toplevelfunctions $name $id
	mapfunction $name $name
}
proc getparent {name} {
	set tree [split $name .]
	set tree [join [lrange $tree 0 [expr [llength $tree]-2]] .]
	if {$tree == ""} {
		return "."
	}
	return $tree
}
#Initialization code, connect and create our first toplevel widget
pgConnect localhost 0
set id [pgRegisterApp "picowish" $pg_app(normal)]
set ids(.) $id
set defaultparent_$id $id
set defaultrship_$id inside

#create the command named . in the tk namespace
tk alias . toplevelfunctions . $id
#map the . function to the current namespace
mapfunction . .
puts [info vars]
#just some sample test code
set button [pgCreateWidget button]
pgSetText $button "Hello"
pgAttach $button inside $id

pgEventLoop
