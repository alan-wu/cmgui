# **************************************************************************
# FILE : common.Makefile
#
# LAST MODIFIED : 11 July 2007
#
# DESCRIPTION :
#
# Makefile for common rules for cmgui, unemap and cell
# ==========================================================================


ifndef SYSNAME
  SYSNAME := $(shell uname)
  ifeq ($(SYSNAME),)
    $(error error with shell command uname)
  endif
endif

ifndef NODENAME
  NODENAME := $(shell uname -n)
  ifeq ($(NODENAME),)
    $(error error with shell command uname -n)
  endif
endif

ifndef MACHNAME
  MACHNAME := $(shell uname -m)
  ifeq ($(MACHNAME),)
    $(error error with shell command uname -m)
  endif
endif

ifndef DEBUG
  ifndef OPT
    OPT = false
  endif
  ifeq ($(OPT),false)
    DEBUG = true
  else
    DEBUG = false
  endif
endif

ifndef MP
  MP = false
endif

# set architecture dependent directories and default options
ifeq ($(SYSNAME:IRIX%=),)# SGI
  # Specify what application binary interface (ABI) to use i.e. 32, n32 or 64
  ifndef ABI
    ifdef SGI_ABI
      ABI := $(patsubst -%,%,$(SGI_ABI))
    else
      ABI = n32
    endif
  endif
  # Specify which instruction set to use i.e. -mips#
  ifndef MIPS
    # Using mips3 for most basic version on esu* machines
    # as there are still some Indys around.
    # Although mp versions are unlikely to need mips3 they are made this way
    # because it makes finding library locations easier.
    MIPS = 4
    ifeq ($(filter-out esu%,$(NODENAME)),)
      ifeq ($(ABI),n32)
        ifneq ($(DEBUG),false)
          MIPS=3
        endif
      endif
    endif
  endif
  INSTRUCTION = mips
  OPERATING_SYSTEM = irix
endif
ifeq ($(SYSNAME),Linux)
  ifndef STATIC
    STATIC = true
  endif
  ifeq ($(filter-out i%86,$(MACHNAME)),)
    INSTRUCTION = i686
  else
    INSTRUCTION := $(MACHNAME)
  endif
  OPERATING_SYSTEM = linux
endif
ifeq ($(SYSNAME:CYGWIN%=),)# CYGWIN
  #Default to win32
  SYSNAME=win32
endif
ifeq ($(SYSNAME:MINGW32%=),)# MSYS
  #Default to win32
  SYSNAME=win32
endif
ifeq ($(SYSNAME),win32)
  INSTRUCTION = i386
  OPERATING_SYSTEM = win32
  ifndef STATIC
    STATIC = true
  endif
endif
ifeq ($(SYSNAME),SunOS)
  INSTRUCTION = 
  OPERATING_SYSTEM = solaris
endif
ifeq ($(SYSNAME),AIX)
  ifndef ABI
    ifdef OBJECT_MODE
      ifneq ($(OBJECT_MODE),32_64)
        ABI = $(OBJECT_MODE)
      endif
    endif
  endif
  ifndef ABI
    ABI = 32
  endif
  INSTRUCTION = rs6000
  OPERATING_SYSTEM = aix
endif
ifeq ($(SYSNAME),Darwin)
  ifeq ($(filter-out i%86,$(MACHNAME)),)
    INSTRUCTION = i386
  else
    ifeq ($(MACHNAME),Power Macintosh)
      INSTRUCTION = ppc
    else
      INSTRUCTION := $(MACHNAME)
    endif
  endif
  ifndef ABI
    ABI = 32
  endif
  OPERATING_SYSTEM = darwin
endif

BIN_ARCH_DIR = $(INSTRUCTION)-$(OPERATING_SYSTEM)
ifdef ABI
   LIB_ARCH_DIR = $(INSTRUCTION)-$(ABI)-$(OPERATING_SYSTEM)
else # ABI
   LIB_ARCH_DIR = $(INSTRUCTION)-$(OPERATING_SYSTEM)
endif # ABI

OBJ_SUFFIX = o
CCOFLAG = -o 
LINKOFLAG = -o 
LINKOPTIONFLAG =
AR = ar
AROFLAG = cr\

ifeq ($(SYSNAME:IRIX%=),)
   ifneq ($(ABI),64)
      UIL = uil
   else # ABI != 64
      UIL = uil64
   endif # ABI != 64
   CC = cc -c
   CPP = CC -c
   CPP_FLAGS = -LANG:std -LANG:vla=ON -no_auto_include
   FORTRAN = f77 -c
   MAKEDEPEND = makedepend -f- -Y --
   CPREPROCESS = cc -P
	STRIP =
   STRIP_SHARED =
   # LINK = cc
   # Must use C++ linker for XML */
   LINK = CC
   ifneq ($(DEBUG),true)
      OPTIMISATION_FLAGS = -O
      COMPILE_FLAGS = -pedantic -woff 1521
      COMPILE_DEFINES = -DOPTIMISED
      STRICT_FLAGS = 
      CPP_STRICT_FLAGS = 
      NON_STRICT_FLAGS = 
      CPP_NON_STRICT_FLAGS = 
      NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
   else # DEBUG != true
      OPTIMISATION_FLAGS = -g
      COMPILE_FLAGS = -fullwarn -pedantic -woff 1521
      COMPILE_DEFINES = -DREPORT_GL_ERRORS -DUSE_PARAMETER_ON 
      # STRICT_FLAGS = -diag_error 1042,1174,1185,1196,1409,1551,1552,3201
      STRICT_FLAGS = -diag_error 1000-9999
      # need to suppress for Boost mostly but also for STL
			#   1682 is if you have virtual functions with the same name and different
			#     arguments, all or none need to be over-ridden in a derived class
      CPP_STRICT_FLAGS = -diag_error 1000-9999 -diag_suppress 1110,1174,1209,1234,1375,1424,1682,3201,1506,3303,1182
      NON_STRICT_FLAGS = -diag_warning 1429
      CPP_NON_STRICT_FLAGS = -diag_warning 1429
      NON_STRICT_FLAGS_PATTERN = three_d_drawing/graphics_buffer.c | three_d_drawing/movie_extensions.c
      CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
   endif # DEBUG != true
   ifeq ($(MIPS),3)
      OPTIMISATION_FLAGS += -mips3
   else
      OPTIMISATION_FLAGS += -mips4
   endif

   ifeq ($(ABI),64)
      TARGET_TYPE_FLAGS = -G0 -64
      TARGET_TYPE_DEFINES = -DO64
   else # ABI == 64
      TARGET_TYPE_FLAGS = -n32
      TARGET_TYPE_DEFINES =
   endif # ABI == 64
endif # SYSNAME == IRIX%=
ifeq ($(SYSNAME),Linux)
   UIL = uil
#    ifeq ($(MACHNAME),ia64)
#       # Using Intel compilers
#       CC = ecc -c
#       CPP = $(CC)
#       # turn on warnings,
#       # suppress messages about non-standard fortran (including REAL*8,
#       # more than 19 continuation lines),
#       # suppress comment messages (including obsolescent alternate return),
#       # auto arrays (as well as scalars), no aliasing,
#       # no preprocessing, temporary arrays on stack if possible.
#       FORTRAN = efc -c -W1 -w95 -cm -auto -fpp0 -stack_temps
#    else
      # gcc
      CC = gcc -c -std=gnu99
      CPP = g++ -c
      ifeq ($(PROFILE),true)
        CC += -g -pg
        CPP += -g -pg
      endif	
      CPP_FLAGS =
      FORTRAN = g77 -c -fno-second-underscore
#    endif
   MAKEDEPEND = gcc -MM -MG
   CPREPROCESS = gcc -E -P
   ifneq ($(STATIC_LINK),true)
      LINK = gcc
   else # STATIC_LINK) != true
      LINK = gcc -static
   endif # STATIC_LINK) != true
   ifeq ($(PROFILE),true)
      LINK += -pg
   endif
	# Is the compiler a version that will not compile without the no strict aliasing (NSA) flag?
	NSA_COMPILER_VERSION := $(shell $(CC) -dumpversion | grep "4\.[12]\.[0-9]")
	ifneq (,$(NSA_COMPILER_VERSION))
	 	NO_STRICT_ALIASING_FLAG := -fno-strict-aliasing
	else
	 	NO_STRICT_ALIASING_FLAG :=
	endif
   ifneq ($(DEBUG),true)
      OPTIMISATION_FLAGS = -O3
      COMPILE_DEFINES = -DOPTIMISED
      COMPILE_FLAGS = -fPIC $(NO_STRICT_ALIASING_FLAG) 
      STRICT_FLAGS = -W -Wall -Werror -Wno-unused-parameter
      CPP_STRICT_FLAGS = -W -Wall -Werror -Wno-unused-parameter
			UNINITIALISED_FLAG = -Wno-uninitialized
      NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
      CPP_NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
      #NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
      #CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
			UNINITIALISED_FLAG_FILENAMES = wx_not_initialised.filenames
			NON_STRICT_FILENAMES = wx_non_strict_c.filenames
			CPP_NON_STRICT_FILENAMES = wx_non_strict_cpp.filenames
			ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
				UNINITIALISED_FLAG_FILENAMES = motif_not_initialised.filenames
				NON_STRICT_FILENAMES = motif_non_strict_c.filenames
				CPP_NON_STRICT_FILENAMES = motif_non_strict_cpp.filenames
			endif
			empty :=
			space := $(empty) $(empty)
			NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(NON_STRICT_FILENAMES))))
			CPP_NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(CPP_NON_STRICT_FILENAMES))))
			UNINITIALISED_FLAG_PATTERN := $(subst $(space),|,$(strip $(shell cat $(UNINITIALISED_FLAG_FILENAMES))))
      ifeq ($(PROFILE),true)
        #Don't strip if profiling
        STRIP =
        STRIP_SHARED =
      else
        STRIP = strip --strip-unneeded
        STRIP_SHARED = strip --strip-unneeded
      endif
   else  # DEBUG != true
      OPTIMISATION_FLAGS = -g
      COMPILE_DEFINES = -DREPORT_GL_ERRORS -DUSE_PARAMETER_ON
      COMPILE_FLAGS = -fPIC $(NO_STRICT_ALIASING_FLAG) 
      STRICT_FLAGS = -W -Wall -Werror
      CPP_STRICT_FLAGS = -W -Wall -Werror
			UNINITILISED_FLAG = 
      NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
      CPP_NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
      #NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
      #CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
			NON_STRICT_FILENAMES = wx_non_strict_c.filenames
			CPP_NON_STRICT_FILENAMES = wx_non_strict_cpp.filenames
			ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
				NON_STRICT_FILENAMES = motif_non_strict_c.filenames
				CPP_NON_STRICT_FILENAMES = motif_non_strict_cpp.filenames
			endif
			empty :=
			space := $(empty) $(empty)
			NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(NON_STRICT_FILENAMES))))
			CPP_NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(CPP_NON_STRICT_FILENAMES))))
			UNINITIALISED_FLAG_PATTERN = NONE
      STRIP =
      STRIP_SHARED =
   endif # DEBUG != true
   TARGET_TYPE_FLAGS =
   TARGET_TYPE_DEFINES =
   ifeq ($(MACHNAME),ia64)
      TARGET_TYPE_DEFINES = -DO64
      # To work around errors of the form:
      # /home/hunter/gui/cmgui/source/api/cmiss_core.c:60: relocation truncated to fit: PCREL21B display_message
      LINK += -Wl,-relax
   endif
endif # SYSNAME == Linux
ifeq ($(SYSNAME),AIX)
   ifneq ($(ABI),64)
      UIL = uil
   else # ABI != 64
      UIL = uil64
   endif # ABI != 64
   CC = xlc -c
   CPP = xlc -qnolm -c
   CPP_FLAGS = -qrtti
   FORTRAN = f77 -c
   MAKEDEPEND = makedepend -f-
   CPREPROCESS = 
   #Specify a runtime libpath so that the executable doesn't just add in the location
   #of every library (dynamic and static).  In particular I do not want Mesa to be loaded
   #by default even if that is what is linked against.
   LINK = xlc -blibpath:/usr/lib:/lib
   ifeq ($(ABI),32)
      #Increase the maximum memory for a 32 bit executable
      LINK += -bmaxdata:0x80000000
   endif # $(ABI) == 32
   ifneq ($(DEBUG),true)
      OPTIMISATION_FLAGS = -O2 -qmaxmem=-1
   else # DEBUG != true
      OPTIMISATION_FLAGS = -g
   endif # DEBUG != true
   COMPILE_FLAGS = 
   COMPILE_DEFINES = 
   STRICT_FLAGS = 
   CPP_STRICT_FLAGS = 
   NON_STRICT_FLAGS = 
   CPP_NON_STRICT_FLAGS = 
   NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
   CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
   UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
   STRIP =
   STRIP_SHARED =
   ifeq ($(ABI),64)
      TARGET_TYPE_FLAGS =  -q64
      TARGET_TYPE_DEFINES = -DO64
      AR_FLAGS = -X64
   else # ABI == 64
      TARGET_TYPE_FLAGS = -q32
      TARGET_TYPE_DEFINES =
   endif # ABI == 64
endif # SYSNAME == AIX
ifeq ($(SYSNAME),win32)
   ifeq ($(COMPILER),msvc)
      CYGWIN_WRAPPER = ./cygwin-wrapper
      CC = $(CYGWIN_WRAPPER) cl.exe -c
      CPP = $(CYGWIN_WRAPPER) cl.exe -c
      OBJ_SUFFIX = obj
      CCOFLAG = -Fo
      LINKOFLAG = -Fe
      LINKOPTIONFLAG = /link
      CPP_FLAGS = /EHsc
      MAKEDEPEND = gcc -MM -MG -mno-cygwin
      CPREPROCESS = $(CYGWIN_WRAPPER) cl.exe /P
      LINK = $(CYGWIN_WRAPPER) cl.exe
      AR = $(CYGWIN_WRAPPER) lib
      WINDRES = windres
      AROFLAG = /out:
      ifeq ($(filter-out CONSOLE_USER_INTERFACE GTK_USER_INTERFACE,$(USER_INTERFACE)),)
         LINK +=  
      else # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE || $(USER_INTERFACE) == GTK_USER_INTERFACE
         LINK += 
      endif # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE || $(USER_INTERFACE) == GTK_USER_INTERFACE
      ifneq ($(DEBUG),true)
         OPTIMISATION_FLAGS = 
         COMPILE_DEFINES = -DOPTIMISED
         COMPILE_FLAGS = /MD
         STRICT_FLAGS =
         CPP_STRICT_FLAGS =
         NON_STRICT_FLAGS = 
      	 CPP_NON_STRICT_FLAGS = 
         NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
         CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
         UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
         STRIP = 
         STRIP_SHARED = 
      else # DEBUG != true
         LINKOPTIONFLAG +=
         OPTIMISATION_FLAGS = /DEBUG
         COMPILE_DEFINES =
         COMPILE_FLAGS = /MDd /Zi
         STRICT_FLAGS =
         CPP_STRICT_FLAGS =
         NON_STRICT_FLAGS = 
      	 CPP_NON_STRICT_FLAGS = 
         NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
         CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
         UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
         STRIP =
         STRIP_SHARED =
      endif # DEBUG != true
      TARGET_TYPE_FLAGS =
      TARGET_TYPE_DEFINES =
   else # COMPILER = msvc
      UIL = uil
      # CC = gcc -c -mno-cygwin -fnative-struct */
      CC = gcc -c -mno-cygwin -mms-bitfields
      # CC = gcc -c -mno-cygwin -mms-bitfields -D_LIB -D_MT -D_FILE_OFFSET_BITS=64
      CPP = g++ -c -mno-cygwin -mms-bitfields
      CPP_FLAGS =
      FORTRAN = f77 -c -mno-cygwin -mms-bitfields
      MAKEDEPEND = gcc -MM -MG -mno-cygwin
      CPREPROCESS = gcc -E -P -mno-cygwin
      WINDRES = windres
      LINK = g++ -mno-cygwin -fnative-struct -mms-bitfields
      ifeq ($(filter-out CONSOLE_USER_INTERFACE GTK_USER_INTERFACE,$(USER_INTERFACE)),)
         LINK += -mconsole 
      else # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE || $(USER_INTERFACE) == GTK_USER_INTERFACE
         LINK += -mwindows
      endif # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE || $(USER_INTERFACE) == GTK_USER_INTERFACE
      ifneq ($(DEBUG),true)
         OPTIMISATION_FLAGS = -O2
         COMPILE_DEFINES = -DOPTIMISED
         COMPILE_FLAGS =
         STRICT_FLAGS = -Werror
         CPP_STRICT_FLAGS = -Werror
         NON_STRICT_FLAGS = 
      	 CPP_NON_STRICT_FLAGS = 
         NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
         CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
         UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
         STRIP = strip --strip-unneeded
         STRIP_SHARED = strip --strip-unneeded
      else # DEBUG != true
         OPTIMISATION_FLAGS = -g
         COMPILE_DEFINES = -DREPORT_GL_ERRORS -DUSE_PARAMETER_ON
         COMPILE_FLAGS =
         STRICT_FLAGS = -W -Wall -Werror
         CPP_STRICT_FLAGS = -W -Wall -Werror
         NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
         CPP_NON_STRICT_FLAGS = -Wno-parentheses -Wno-switch
         NON_STRICT_FILENAMES = wx_non_strict_c.filenames
         CPP_NON_STRICT_FILENAMES = wx_non_strict_cpp.filenames
         ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
            NON_STRICT_FILENAMES = motif_non_strict_c.filenames
            CPP_NON_STRICT_FILENAMES = motif_non_strict_cpp.filenames
         endif
         empty :=
         space := $(empty) $(empty)
         NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(NON_STRICT_FILENAMES))))
         CPP_NON_STRICT_FLAGS_PATTERN := $(subst $(space),|,$(strip $(shell cat $(CPP_NON_STRICT_FILENAMES))))
         UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
         STRIP =
         STRIP_SHARED =
      endif # DEBUG != true
      TARGET_TYPE_FLAGS =
      TARGET_TYPE_DEFINES =
   endif # COMPILER == msvc
endif # SYSNAME == win32
ifeq ($(SYSNAME),Darwin)
   OPENMOTIF_DIR = /usr/OpenMotif-2.1.31-22i
   UIL = $(OPENMOTIF_DIR)/bin/uil
#    ifeq ($(MACHNAME),ia64)
#       # Using Intel compilers
#       CC = ecc -c
#       CPP = $(CC)
#       # turn on warnings,
#       # suppress messages about non-standard fortran (including REAL*8,
#       # more than 19 continuation lines),
#       # suppress comment messages (including obsolescent alternate return),
#       # auto arrays (as well as scalars), no aliasing,
#       # no preprocessing, temporary arrays on stack if possible.
#       FORTRAN = efc -c -W1 -w95 -cm -auto -fpp0 -stack_temps
#    else
      # gcc
      # for profiling
      #CC = gcc -c -std=gnu99 -pg
		# -fno-common so that it doesn't use common blocks and they symbols will get exported into a library	
      CC = gcc -c -std=gnu99 -fno-common
      CPP = g++ -c
      CPP_FLAGS =
      FORTRAN = g77 -c -fno-second-underscore
#    endif
   MAKEDEPEND = gcc -MM -MG
   CPREPROCESS = gcc -E -P
   ifneq ($(STATIC_LINK),true)
      # for profiling
      #LINK = gcc -pg
      LINK = g++ -Wl,-Y,20 
      # LINK = egcs -shared -L/usr/X11R6/lib -v */
      # LINK = gcc -L/usr/X11R6/lib -v */
   else # STATIC_LINK) != true
      LINK = g++ -static
      # LINK = g++ --no-demangle -rdynamic -L/usr/X11R6/lib*/
   endif # STATIC_LINK) != true
   ifneq ($(DEBUG),true)
      OPTIMISATION_FLAGS = -O
      COMPILE_DEFINES = -DOPTIMISED
      COMPILE_FLAGS = -fPIC
      STRICT_FLAGS = -Wno-long-double -Werror
      CPP_STRICT_FLAGS = -Wno-long-double -Werror
      NON_STRICT_FLAGS = 
      CPP_NON_STRICT_FLAGS = 
      NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match
      CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
      STRIP =
      STRIP_SHARED =
   else  # DEBUG != true
      OPTIMISATION_FLAGS = -g
      COMPILE_DEFINES = -DREPORT_GL_ERRORS -DUSE_PARAMETER_ON
      COMPILE_FLAGS = -fPIC
      STRICT_FLAGS = -W -Wall -Wno-parentheses -Wno-switch -Wno-long-double -Werror
      CPP_STRICT_FLAGS = -W -Wall -Wno-parentheses -Wno-switch -Wno-unused-parameter -Wno-long-double -Werror
      NON_STRICT_FLAGS = 
      CPP_NON_STRICT_FLAGS = 
      NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      CPP_NON_STRICT_FLAGS_PATTERN = NONE # Must specify a pattern that doesn't match */
      UNINITIALISED_FLAG_PATTERN = NONE # Must specify a pattern that doesn't match */
      STRIP =
      STRIP_SHARED =
   endif # DEBUG != true
   TARGET_TYPE_FLAGS =
   TARGET_TYPE_DEFINES =
   ifeq ($(MACHNAME),ia64)
      TARGET_TYPE_DEFINES = -DO64
      # To work around errors of the form:
      # /home/hunter/gui/cmgui/source/api/cmiss_core.c:60: relocation truncated to fit: PCREL21B display_message
      LINK += -Wl,-relax
   endif
endif # SYSNAME == Darwin

UTILITIES_PATH=$(CMGUI_DEV_ROOT)/utilities/$(BIN_ARCH_DIR)

# DSO Link command
DSO_LINK = $(LINK) $(ALL_FLAGS) -shared

.MAKEOPTS : -r

.SUFFIXES :
.SUFFIXES : .$(OBJ_SUFFIX) .c .d .cpp .f
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
.SUFFIXES : .xrc .xrch
endif # USER_INTERFACE == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
.SUFFIXES : .uil .uidh
endif # USER_INTERFACE == MOTIF_USER_INTERFACE
ifeq ($(SYSNAME),win32)
.SUFFIXES : .res .rc
endif # SYSNAME == win32

%.$(OBJ_SUFFIX): %.c %.d
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	@NON_STRICT_FLAGS= ; \
	case $*.c in $(NON_STRICT_FLAGS_PATTERN) ) \
		NON_STRICT_FLAGS="$$NON_STRICT_FLAGS $(NON_STRICT_FLAGS)" ;; \
	esac ; \
	case $*.c in $(UNINITIALISED_FLAG_PATTERN) ) \
		NON_STRICT_FLAGS="$$NON_STRICT_FLAGS $(UNINITIALISED_FLAG)" ;; \
	esac ; \
  set -x ; $(CC) $(CCOFLAG)$(OBJECT_PATH)/$*.$(OBJ_SUFFIX) $(ALL_FLAGS) $(STRICT_FLAGS) $$NON_STRICT_FLAGS $*.c

%.$(OBJ_SUFFIX): %.cpp %.d
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	@CPP_NON_STRICT_FLAGS= ; \
	case $*.cpp in $(CPP_NON_STRICT_FLAGS_PATTERN) ) \
		CPP_NON_STRICT_FLAGS="$$CPP_NON_STRICT_FLAGS $(CPP_NON_STRICT_FLAGS)" ;; \
	esac ; \
	case $*.cpp in $(UNINITIALISED_FLAG_PATTERN) ) \
		CPP_NON_STRICT_FLAGS="$$CPP_NON_STRICT_FLAGS $(UNINITIALISED_FLAG)" ;; \
	esac ; \
	set -x ; $(CPP) $(CCOFLAG)$(OBJECT_PATH)/$*.$(OBJ_SUFFIX) $(ALL_FLAGS) $(CPP_FLAGS) $(CPP_STRICT_FLAGS) $$CPP_NON_STRICT_FLAGS $*.cpp

%.$(OBJ_SUFFIX): %.f %.d
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	@case $*.f in  \
		* ) \
	  set -x ; $(FORTRAN) $(OPTIMISATION_FLAGS) $(TARGET_TYPE_FLAGS) $(CCOFLAG)$(OBJECT_PATH)/$*.$(OBJ_SUFFIX) $(SOURCE_DIRECTORY_INC) $*.f ;; \
	esac ;

$(OBJECT_PATH)/%.c : $(SOURCE_PATH)/%.c

ifdef NO_MAKE_DEPEND
%.d :
	@:
else # NO_MAKE_DEPEND
%.d :
	if [ ! -d $(OBJECT_PATH)/$(subst $(OBJECT_PATH)/,,$(*D)) ]; then \
		mkdir -p $(OBJECT_PATH)/$(subst $(OBJECT_PATH)/,,$(*D)) ; \
	fi
	@stem_name=$(subst $(OBJECT_PATH)/,,$*); \
   source_name=$(firstword $(wildcard $(SOURCE_PATH)/$(subst $(OBJECT_PATH)/,,$*).c $(SOURCE_PATH)/$(subst $(OBJECT_PATH)/,,$*).cpp)) ; \
	set -x ; $(MAKEDEPEND) $(ALL_FLAGS) $(STRICT_FLAGS) $${source_name} 2> /dev/null | \
	sed -e "s%^.*\.o%$${stem_name}.$(OBJ_SUFFIX) $(OBJECT_PATH)/$${stem_name}.d%;s%$(SOURCE_PATH)/%%g;s%\([^ 	]*\.[ch]p*\)%\$$(wildcard \1)%g;" > $(OBJECT_PATH)/$${stem_name}.d ;
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
   # Fix up the uidh references
	@stem_name=$(subst $(OBJECT_PATH)/,,$*); \
	sed -e 's%$(XRCH_PATH)/%%g' $(OBJECT_PATH)/$${stem_name}.d > $(OBJECT_PATH)/$${stem_name}.d2 ; \
	mv $(OBJECT_PATH)/$${stem_name}.d2 $(OBJECT_PATH)/$${stem_name}.d
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
   # Fix up the uidh references
	@stem_name=$(subst $(OBJECT_PATH)/,,$*); \
	sed -e 's%$(UIDH_PATH)/%%g' $(OBJECT_PATH)/$${stem_name}.d > $(OBJECT_PATH)/$${stem_name}.d2 ; \
	mv $(OBJECT_PATH)/$${stem_name}.d2 $(OBJECT_PATH)/$${stem_name}.d
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
endif # NO_MAKE_DEPEND

%.d: %.f
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	touch $(OBJECT_PATH)/$@

ifeq ($(USER_INTERFACE),WIN32_USER_INTERFACE)
.rc.res:
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	set -x ; \
    $(WINDRES) -o $(OBJECT_PATH)/$*.res -O coff $*.rc
endif # $(USER_INTERFACE) == WIN32_USER_INTERFACE

ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)

WX_COMPILER = $(shell $(WX_DIR)wx-config --utility=wxrc)

%.xrch : %.xrc
	@if [ ! -d $(XRCH_PATH)/$(*D) ]; then \
		mkdir -p $(XRCH_PATH)/$(*D); \
	fi
	set -x ; \
	$(WX_COMPILER) --cpp-code --verbose $*.xrc --function="wxXmlInit_$(*F)" --output=$(XRCH_PATH)/$*.xrch
ifeq ($(SYSNAME),win32)
.rc.res:
	@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
		mkdir -p $(OBJECT_PATH)/$(*D); \
	fi
	set -x ; \
    $(WINDRES) -o $(OBJECT_PATH)/$*.res -O coff $*.rc
endif # $(SYSNAME) == win32
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE

ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)

UID2UIDH = utilities/uid2uidh.pl

%.uidh : %.uil
	@if [ ! -d $(UIDH_PATH)/$(*D) ]; then \
		mkdir -p $(UIDH_PATH)/$(*D); \
	fi
	set -x ; \
	$(UIL) -o $(UIDH_PATH)/$*.uid $*.uil && \
	perl $(UID2UIDH) $(UIDH_PATH)/$*.uid $(UIDH_PATH)/$*.uidh
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE

ifneq ($(STRIP),)
   STRIP_TARGET = $(STRIP) $(LINK_DIR)/$(1)$(LINK_SUFFIX) &&
   STRIP_SHARED = $(STRIP) $(2)/$(1)$(SO_LIB_SUFFIX) &&
else
   STRIP_TARGET =
   STRIP_SHARED =
endif

#We only use a temp dir now if CMISS_TMP_DIR is set
ifeq ($(CMISS_TMP_DIR),)
   LINK_DIR = $(2)
   LINK_SUFFIX =
   LINK_MOVE =
else
   LINK_DIR = $(CMISS_TMP_DIR)
   LINK_SUFFIX = .tmp$$$$
   LINK_MOVE = mv $(CMISS_TMP_DIR)/$(1).tmp$$$$ $(2)/$(1) &&
endif

define BuildNormalTarget
	echo 'Building $(2)/$(1)'
	if [ ! -d $(OBJECT_PATH) ]; then \
		mkdir -p $(OBJECT_PATH); \
	fi
	if [ ! -d $(2) ]; then \
		mkdir -p $(2) ; \
	fi
	cd $(OBJECT_PATH) && \
	(echo $(3) 2>&1 > $(1).list$$$$) && \
	$(LINK) $(LINKOFLAG)$(LINK_DIR)/$(1)$(LINK_SUFFIX) $(ALL_FLAGS) `cat $(1).list$$$$` $(4) && \
	$(STRIP_TARGET) \
	rm $(1).list$$$$ && \
        $(LINK_MOVE) \
        echo 'Success $(2)/$(1)';
endef

define BuildStaticLibraryTarget
	echo 'Building static library $(2)/$(1)'
	if [ ! -d $(OBJECT_PATH) ]; then \
		mkdir -p $(OBJECT_PATH); \
	fi
	if [ ! -d $(dir $(2)/$(1)) ]; then \
		mkdir -p $(dir $(2)/$(1)) ; \
	fi
	cd $(OBJECT_PATH) && \
	(echo $(3) 2>&1 > $(1).list$$$$) && \
	$(AR) $(AR_FLAGS) $(AROFLAG)$(LINK_DIR)/$(1)$(LINK_SUFFIX) `cat $(1).list$$$$` && \
        rm $(1).list$$$$ && \
        $(LINK_MOVE) \
        echo 'Success $(2)/$(1)';
endef

ifeq ($(OPERATING_SYSTEM), win32)
  ifeq ($(COMPILER),msvc)
    LINK_LINE = ./win32_export_all $(1).dll `cat $(1).list$$$$` &&
    LINK_LINE += $(CYGWIN_WRAPPER) link.exe /OUT:$(2)/$(1)$(SO_LIB_SUFFIX) /DEBUG /DEF:$(1).def `cat $(1).list$$$$` /libpath:"c:\Program Files\Microsoft SDKs\Windows\v6.0A\lib" /libpath:"c:\Program Files\Microsoft Visual Studio 9.0\VC\lib" $(4) $(foreach import_lib,$(6),$(import_lib)$(SO_LIB_IMPORT_LIB_SUFFIX)) /INCREMENTAL /NOLOGO /DLL /MANIFEST /MANIFESTFILE:"libcmgui_general.intermediate.manifest" /MANIFESTUAC:"level='asInvoker' uiAccess='false'"  /DEBUG  /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT
  else
    LINK_LINE = $(LINK) -shared -o $(2)/$(1)$(SO_LIB_SUFFIX) $(ALL_FLAGS) -Wl,--out-implib,$(2)/$(1)$(SO_LIB_IMPORT_LIB_SUFFIX) -Wl,--kill-at -Wl,--output-def,$(2)/$(1).def -Wl,--whole-archive `cat $(1).list$$$$`  -Wl,--no-whole-archive $(4) $(foreach import_lib,$(6),$(import_lib)$(SO_LIB_IMPORT_LIB_SUFFIX))
    LINK_LINE += && (cd $(2) ; if ! lib /machine:i386 /def:$(1).def && [ -f $(1).lib ] ; then rm $(1).lib ; fi  )
  endif
else
  LINK_LINE = $(LINK) -shared -o $(2)/$(1)$(SO_LIB_SUFFIX) $(ALL_FLAGS) `cat $(1).list$$$$` $(4) -Wl,-soname,$(5) $(foreach import_lib,$(6),$(import_lib)$(SO_LIB_SUFFIX))
endif

define BuildSharedLibraryTarget
	echo 'Building shared library $(2)/$(1)'
	if [ ! -d $(OBJECT_PATH) ]; then \
		mkdir -p $(OBJECT_PATH); \
	fi
	if [ ! -d $(dir $(2)/$(1)) ]; then \
		mkdir -p $(dir $(2)/$(1)) ; \
	fi
	cd $(OBJECT_PATH) && \
	(echo $(3) 2>&1 > $(1).list$$$$) && \
	$(LINK_LINE) && \
	$(STRIP_SHARED) \
   rm $(1).list$$$$ ;
endef
