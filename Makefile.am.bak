# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/khtml2png2.pot

KDE_ICON = khtml2png2

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = khtml2png2

# the application source, library search path, and link libraries

khtml2png2_LDFLAGS = $(KDE_RPATH) $(all_libraries) -lz -lkabc -lkdecore -lkdeprint -lkdeui -lkio -lkhtml -lkio -lkparts -lkspell
khtml2png2_LDADD = $(LIB_KDEUI)

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities


# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/khtml2png2


noinst_HEADERS = khtml2png.h
khtml2png2_SOURCES = khtml2png.cpp
