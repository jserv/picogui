use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME' => 'PicoGUI',
    'VERSION' => '2.0',
    'PREREQ_PM'	=> {
	'Carp' => undef,
	'IO::Socket' => undef,
    },
    'PM' => {
	'PicoGUI.pm' => '$(INST_LIBDIR)/PicoGUI.pm',
	'PicoGUI/Server.pm' => '$(INST_LIBDIR)/PicoGUI/Server.pm',
	'PicoGUI/Widget.pm' => '$(INST_LIBDIR)/PicoGUI/Widget.pm',
    },

	    'LIBS'		=> [], # e.g., '-lm'
    'DEFINE'		=> '', # e.g., '-DHAVE_SOMETHING'
    'INC'		=> '', # e.g., '-I/usr/include/other'
);
