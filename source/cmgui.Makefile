
# **************************************************************************
# FILE : cmgui.Makefile
#
# LAST MODIFIED : 11 July 2007
#
# DESCRIPTION :
#
# Makefile for cmgui (the front end to CMISS)
# ==========================================================================

ifneq ($(WIN32_NATIVE_COMPILE),true)
   SHELL=/bin/sh
else # $(WIN32_NATIVE_COMPILE) != true
   SHELL=bash.exe
endif # $(WIN32_NATIVE_COMPILE) != true

ifndef CMGUI_DEV_ROOT
   CMGUI_DEV_ROOT = $(PWD:/source=)
endif # ! CMGUI_DEV_ROOT

ifndef CMISS_ROOT
   CMISS_ROOT = $(CMGUI_DEV_ROOT)
endif # ! defined CMISS_ROOT

SOURCE_PATH=$(CMGUI_DEV_ROOT)/source
PRODUCT_PATH=$(CMISS_ROOT)/cmgui
PRODUCT_SOURCE_PATH=$(PRODUCT_PATH)/source

ifndef USER_INTERFACE
   USER_INTERFACE=WX_USER_INTERFACE
endif

ifndef GRAPHICS_API
   GRAPHICS_API=OPENGL_GRAPHICS
endif

ifndef STATIC_LINK
   STATIC_LINK=false
endif

ifndef FE_VALUE_IS_DOUBLE
   FE_VALUE_IS_DOUBLE=false
endif

COMMONMAKEFILE := common.Makefile
COMMONMAKEFILE_FOUND = $(wildcard $(COMMONMAKEFILE))
include $(COMMONMAKEFILE)

#For built in unemap override on the command line or change to true.
#The source directories unemap_application and unemap_hardware_service
#must be softlinked into the cmgui source directory.
UNEMAP = false
LINK_CMISS = false
USE_PERL_INTERPRETER = true
USE_IMAGEMAGICK = true
USE_NETGEN = false
#Does your version of imagemagick include libgdcm
USE_LIBGDCM = false
ifeq ($(SYSNAME),Linux)
  USE_LIBGDCM = true
endif # SYSNAME == Linux
ifeq ($(SYSNAME),win32)
  USE_LIBGDCM = true
endif  # SYSNAME == win32
ifeq ($(SYSNAME),Darwin)
  USE_LIBGDCM = true
endif  # SYSNAME == Darwin
ifeq ($(SYSNAME:IRIX%=),)
  USE_LIBGDCM = false
endif
ifeq ($(SYSNAME),AIX)
  USE_LIBGDCM = false
endif
USE_XML2 = true
ifneq ($(filter linux aix win32 irix darwin,$(OPERATING_SYSTEM)),)
  ifneq ($(GRAPHICS_API),NO3D_GRAPHICS)
    USE_ITK = true
  else
    USE_ITK = false
  endif
else # $(OPERATING_SYSTEM) == linux || $(OPERATING_SYSTEM) == aix
  USE_ITK = false
endif # $(OPERATING_SYSTEM) == linux || $(OPERATING_SYSTEM) == aix
#Use GTKMAIN by default, gtkmain is no longer mangled into the version name
USE_GTKMAIN = true

ifeq ($(OPERATING_SYSTEM),win32)
   ifeq ($(filter CONSOLE_USER_INTERFACE GTK_USER_INTERFACE,$(USER_INTERFACE)),)
      WIN32_USER_INTERFACE = true 
   endif # $(USER_INTERFACE) != CONSOLE_USER_INTERFACE && $(USER_INTERFACE) != GTK_USER_INTERFACE
endif # OPERATING_SYSTEM == win32

TARGET_EXECUTABLE_BASENAME = cmgui

ifneq ($(ABI),64)
   TARGET_ABI_SUFFIX =
else # $(ABI) != 64
   TARGET_ABI_SUFFIX = 64
endif # $(ABI) != 64

TARGET_USER_INTERFACE_SUFFIX :=
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -motif
endif # $(USER_INTERFACE) == 	MOTIF_USER_INTERFACE
ifeq ($(USER_INTERFACE), WIN32_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -win32
endif # $(USER_INTERFACE) == 	WIN32_USER_INTERFACE
ifeq ($(USER_INTERFACE), CONSOLE_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -console
endif # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE
ifeq ($(USER_INTERFACE), GTK_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -gtk
endif # $(USER_INTERFACE) == GTK_USER_INTERFACE
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -wx
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), CARBON_USER_INTERFACE)
   TARGET_USER_INTERFACE_SUFFIX := -carbon
endif # $(USER_INTERFACE) == CARBON_USER_INTERFACE

TARGET_COMPILER_SUFFIX :=
ifeq ($(COMPILER), msvc)
   TARGET_COMPILER_SUFFIX := -msvc
endif

ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   TARGET_GRAPHICS_API_SUFFIX =
endif # $(GRAPHICS_API) == OPENGL_GRAPHICS
ifeq ($(GRAPHICS_API), NO3D_GRAPHICS)
   TARGET_GRAPHICS_API_SUFFIX = -no3dgraphics
endif # $(GRAPHICS_API) == NO3D_GRAPHICS

ifneq ($(STATIC_LINK),true)
   TARGET_STATIC_LINK_SUFFIX =
else # $(STATIC_LINK) != true
   TARGET_STATIC_LINK_SUFFIX = -static
endif # $(STATIC_LINK) != true 

ifneq ($(DEBUG),true)
   TARGET_DEBUG_SUFFIX =
else # $(DEBUG) != true
   TARGET_DEBUG_SUFFIX = -debug
endif # $(DEBUG) != true 

ifneq ($(PROFILE),true)
   TARGET_PROFILE_SUFFIX =
else # $(PROFILE) != true
   TARGET_PROFILE_SUFFIX = -profile
endif # $(PROFILE) == true 

ifneq ($(MEMORYCHECK),true)
   TARGET_MEMORYCHECK_SUFFIX =
else # $(MEMORYCHECK) != true
   TARGET_MEMORYCHECK_SUFFIX = -memorycheck
endif # $(MEMORYCHECK) != true 
 
ifneq ($(USE_GTKMAIN),true)
   MAINLOOP_SUFFIX =
else # $(USE_GTKMAIN) != true
   MAINLOOP_SUFFIX =
endif # $(USE_GTKMAIN) != true 

ifneq ($(UNEMAP),true)
   UNEMAP_SUFFIX =
else # $(UNEMAP) != true
   UNEMAP_SUFFIX = -unemap
endif # $(UNEMAP) != true 

ifeq ($(SYSNAME:IRIX%=),)
   TARGET_FILETYPE_SUFFIX =
endif # SYSNAME == IRIX%=
ifeq ($(SYSNAME),Linux)
   TARGET_FILETYPE_SUFFIX =
endif # SYSNAME == Linux
ifeq ($(SYSNAME),AIX)
   TARGET_FILETYPE_SUFFIX =
endif # SYSNAME == AIX
ifeq ($(SYSNAME),win32)
   TARGET_FILETYPE_SUFFIX = .exe
endif # SYSNAME == win32
ifeq ($(SYSNAME),CYGWIN%=)
   TARGET_FILETYPE_SUFFIX = .exe
endif # SYSNAME == CYGWIN%=

TARGET_SUFFIX = $(TARGET_ABI_SUFFIX)$(TARGET_GRAPHICS_API_SUFFIX)$(TARGET_USER_INTERFACE_SUFFIX)$(TARGET_COMPILER_SUFFIX)$(UNEMAP_SUFFIX)$(MAINLOOP_SUFFIX)$(TARGET_STATIC_LINK_SUFFIX)$(TARGET_DEBUG_SUFFIX)$(TARGET_PROFILE_SUFFIX)$(TARGET_MEMORYCHECK_SUFFIX)
BIN_TARGET = $(TARGET_EXECUTABLE_BASENAME)$(TARGET_SUFFIX)$(TARGET_FILETYPE_SUFFIX)
OBJECT_PATH=$(CMGUI_DEV_ROOT)/object/$(LIB_ARCH_DIR)/$(TARGET_EXECUTABLE_BASENAME)$(TARGET_GRAPHICS_API_SUFFIX)$(TARGET_USER_INTERFACE_SUFFIX)$(TARGET_COMPILER_SUFFIX)$(UNEMAP_SUFFIX)$(MAINLOOP_SUFFIX)$(TARGET_DEBUG_SUFFIX)$(TARGET_PROFILE_SUFFIX)
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
   XRCH_PATH=$(CMGUI_DEV_ROOT)/xrch/$(LIB_ARCH_DIR)/$(TARGET_EXECUTABLE_BASENAME)
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
   UIDH_PATH=$(CMGUI_DEV_ROOT)/uidh/$(LIB_ARCH_DIR)/$(TARGET_EXECUTABLE_BASENAME)
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE

ifeq ($(findstring lib,$(TARGET)),lib)
  BIN_PATH=$(CMGUI_DEV_ROOT)/lib/$(LIB_ARCH_DIR)/$(TARGET_EXECUTABLE_BASENAME)$(TARGET_SUFFIX)
else
  BIN_PATH=$(CMGUI_DEV_ROOT)/bin/$(BIN_ARCH_DIR)
endif

$(BIN_TARGET) :

ifdef MAKECMDGOALS
   define BUILDING_MESSAGE
Making $(MAKECMDGOALS) for $(BIN_TARGET) and $(LIB_ARCH_DIR)

   endef
else
   define BUILDING_MESSAGE
Building $(BIN_TARGET) for $(LIB_ARCH_DIR)

   endef
endif
#$(warning $(BUILDING_MESSAGE))

VPATH=$(BIN_PATH):$(UTILITIES_PATH):$(OBJECT_PATH):$(UIDH_PATH):$(XRCH_PATH)

SOURCE_DIRECTORY_INC = -I$(SOURCE_PATH)

ifeq ($(SYSNAME:IRIX%=),)
   PLATFORM_DEFINES = -DSGI -Dmips -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DUNIX
endif # SYSNAME == IRIX%=
ifeq ($(SYSNAME),Linux)
   PLATFORM_DEFINES = -DGENERIC_PC -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DUNIX
endif # SYSNAME == Linux
ifeq ($(SYSNAME),AIX)
   PLATFORM_DEFINES = -DAIX -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DUNIX
endif # SYSNAME == AIX
ifeq ($(SYSNAME),win32)
   PLATFORM_DEFINES = -DGENERIC_PC -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DWIN32_SYSTEM
endif # SYSNAME == win32
ifeq ($(SYSNAME:CYGWIN%=),)
   PLATFORM_DEFINES = -DGENERIC_PC -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DUNIX -DCYGWIN
endif # SYSNAME == CYGWIN%=
ifeq ($(SYSNAME),Darwin)
   PLATFORM_DEFINES =  -DCMGUI -DHAVE_VFSCANF
   OPERATING_SYSTEM_DEFINES = -DUNIX -DDARWIN
endif # SYSNAME == Darwin
ifneq ($(FE_VALUE_IS_DOUBLE),false)
   FE_VALUE_DEFINES = -DFE_VALUE_IS_DOUBLE
endif
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
   USER_INTERFACE_DEFINES = -DMOTIF_USER_INTERFACE
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
ifeq ($(USER_INTERFACE), WIN32_USER_INTERFACE)
   USER_INTERFACE_DEFINES = -DWIN32_USER_INTERFACE
endif # $(USER_INTERFACE) == WIN32_USER_INTERFACE
ifeq ($(USER_INTERFACE), GTK_USER_INTERFACE)
   ifeq ($(SYSNAME),win32)
      # Our generic main loop is not implemented in Win32 so we use the GTK main step
      USER_INTERFACE_DEFINES = -DGTK_USER_INTERFACE -DUSE_GTK_MAIN_STEP
   else # $(SYSNAME) == win32
      ifneq ($(USE_GTKMAIN),true)
         USER_INTERFACE_DEFINES = -DGTK_USER_INTERFACE 
      else 
         USER_INTERFACE_DEFINES = -DGTK_USER_INTERFACE -DUSE_GTK_MAIN_STEP
      endif # $(USE_GTKMAIN) != true 
   endif # $(SYSNAME) == win32
endif # $(USER_INTERFACE) == GTK_USER_INTERFACE
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
	USER_INTERFACE_DEFINES = -DWX_USER_INTERFACE 
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), CARBON_USER_INTERFACE)
	USER_INTERFACE_DEFINES = -DCARBON_USER_INTERFACE -DTARGET_API_MAC_CARBON
endif # $(USER_INTERFACE) == CARBON_USER_INTERFACE
ifeq ($(USER_INTERFACE), CONSOLE_USER_INTERFACE)
  USER_INTERFACE = -DCONSOLE_USER_INTERFACE
endif # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE

# MOTIF_USER_INTERFACE or GTK_USER_INTERFACE
ifeq ($(filter-out MOTIF_USER_INTERFACE GTK_USER_INTERFACE,$(USER_INTERFACE)),)
  ifeq ($(SYSNAME),Linux)
    #Don't put the system X_LIB into the compiler if we are cross compiling
    GCC_LIBC = $(shell gcc -print-libgcc-file-name)
    GLIBC_CROSS_COMPILE = /hpc/cmiss/cross-compile/i386-glibc23-linux
    ifeq ($(GCC_LIBC:$(GLIBC_CROSS_COMPILE)%=),)
      #We are cross compiling
      X_INC += -I$(GLIBC_CROSS_COMPILE)/include
      X_LIB = $(GLIBC_CROSS_COMPILE)/lib
    else
      X_INC += -I/usr/X11R6/include
      ifeq ($(INSTRUCTION),x86_64)
        X_LIB = /usr/X11R6/lib64
      else
#        X_LIB = /usr/X11R6/lib
        X_LIB = /usr/lib
      endif
    endif
  endif
  ifeq ($(SYSNAME),AIX)
    X_LIB = /usr/X11R6/lib
  endif
endif

GRAPHICS_LIBRARY_DEFINES =
GRAPHICS_LIB =
GRAPHICS_INC =
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   GRAPHICS_LIBRARY_DEFINES = -DOPENGL_API
   ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
      GRAPHICS_LIBRARY_DEFINES += -DDM_BUFFERS
   endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
   ifneq ($(OPERATING_SYSTEM),darwin)
   	ifneq ($(wildcard $(CMISS_ROOT)/mesa/include/$(LIB_ARCH_DIR)),)
      		GRAPHICS_INC += -I$(CMISS_ROOT)/mesa/include/$(LIB_ARCH_DIR)
   	endif
   	GRAPHICS_LIB += $(patsubst %,-L%,$(firstword $(wildcard $(CMISS_ROOT)/mesa/lib/$(LIB_ARCH_DIR) $(X_LIB))))
   endif
   ifeq ($(OPERATING_SYSTEM),darwin)
      ifneq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
          GRAPHICS_LIB += -framework Carbon -framework AGL -L/System/Library/Frameworks/OpenGL.framework/Libraries/
      endif # $(USER_INTERFACE) != MOTIF_USER_INTERFACE
    endif # $(OPERATING_SYSTEM) == darwin
    ifeq ($(OPERATING_SYSTEM),win32)
      ifeq ($(COMPILER),msvc)
        GRAPHICS_LIB += opengl32.lib glu32.lib
      else
        GRAPHICS_LIB += -lopengl32 -lglu32
      endif
    else # $(OPERATING_SYSTEM) == win32
      GRAPHICS_LIB += -lGL -lGLU
    endif # $(OPERATING_SYSTEM) == win32 
endif

ifeq ($(LINK_CMISS),false)
   CONNECTIVITY_DEFINES =
   WORMHOLE_LIB =
else # LINK_CMISS
   WORMHOLE_PATH=$(CMISS_ROOT)/wormhole
   WORMHOLE_INC = -I${WORMHOLE_PATH}/source
   CONNECTIVITY_DEFINES = -DLINK_CMISS
   # WORMHOLE_LIB = -L/usr/people/bullivan/wormhole/lib -lwormhole_n32
   # WORMHOLE_INC = -I/usr/people/bullivan/wormhole/source
   # WORMHOLE_LIB = -L/usr/people/bullivan/wormhole/lib -lwormhole
   # WORMHOLE_INC = -I/usr/people/bullivan/wormhole/source
   WORMHOLE_LIB = -L${WORMHOLE_PATH}/lib/$(LIB_ARCH_DIR) -lwormhole
endif # LINK_CMISS

ifneq ($(USE_IMAGEMAGICK),true)
   IMAGEMAGICK_DEFINES =
   IMAGEMAGICK_LIB = 
else # ! USE_IMAGEMAGICK
   IMAGEMAGICK_DEFINES += -DUSE_IMAGEMAGICK
   IMAGEMAGICK_PATH = $(CMISS_ROOT)/third_party
   IMAGEMAGICK_INC =  -I$(IMAGEMAGICK_PATH)/include/$(LIB_ARCH_DIR) -I$(IMAGEMAGICK_PATH)/include/$(LIB_ARCH_DIR)/ImageMagick
   MAGICK_LIB_NAME = libMagickCore.a
   ifeq ($(SYSNAME),AIX)
      MAGICK_LIB_NAME = libMagick.a
   endif
   ifeq ($(SYSNAME:IRIX%=),)
      MAGICK_LIB_NAME = libMagick.a
   endif
   IMAGEMAGICK_LIB = $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/$(MAGICK_LIB_NAME) $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libtiff.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libpng.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libjpeg.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libbz2.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libz.a
   ifeq ($(USE_LIBGDCM), true)
      ifeq ($(COMPILER),msvc)
         IMAGEMAGICK_LIB += $(foreach lib, gdcmCWRAPPER gdcmMSFF gdcmDSED gdcmzlib gdcmDICT gdcmCommon gdcmIOD gdcmopenjpeg gdcmjpeg8 gdcmjpeg16 gdcmjpeg12 gdcmexpat, $(IMAGEMAGICK_PATH)/$(LIB_ARCH_DIR)/lib/lib$(lib).a)
      else
         IMAGEMAGICK_LIB += -L$(IMAGEMAGICK_PATH)/$(LIB_ARCH_DIR)/lib -lgdcmCWRAPPER -lgdcmMSFF -lgdcmDSED -lgdcmzlib -lgdcmDICT -lgdcmCommon -lgdcmIOD -lgdcmopenjpeg -lgdcmjpeg8 -lgdcmjpeg16 -lgdcmjpeg12 -lgdcmexpat
         ifeq ($(SYSNAME),Linux)
            IMAGEMAGICK_LIB += -lgdcmuuid
         endif
      endif
   endif
   ifneq ($(wildcard $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libltdl.a),)
      #When this first appeared it seemed to be configured for most versions, now it seems to be configured for very few.  Assume we need it only if it is found.
      IMAGEMAGICK_LIB += $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libltdl.a
   endif
ifeq ($(USE_XML2),true)
   IMAGEMAGICK_LIB += $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libxml2.a
endif # USE_XML2

   ifeq ($(SYSNAME),AIX)
      IMAGEMAGICK_LIB += /usr/lib/libiconv.a
   endif # SYSNAME == AIX
endif # ! USE_IMAGEMAGICK

ifeq ($(USE_ITK),true)
   ITK_DEFINES = -DUSE_ITK
   ITK_DIR = $(CMISS_ROOT)/itk/$(LIB_ARCH_DIR)
   ITK_SRCDIR = $(ITK_DIR)/include/InsightToolkit
   ITK_INC = -I$(ITK_SRCDIR) -I$(ITK_SRCDIR)/Algorithms -I$(ITK_SRCDIR)/BasicFilters -I$(ITK_SRCDIR)/Common -I$(ITK_SRCDIR)/Numerics/Statistics -I$(ITK_SRCDIR)/Utilities/vxl/vcl -I$(ITK_SRCDIR)/Utilities/vxl/core
   ITK_LIBPATH_PREFIX = -L
   ITK_LIB_PREFIX = -l
   ITK_LIB_SUFFIX =
   ifeq ($(COMPILER),msvc)
     ITK_LIBPATH_PREFIX = /libpath:
     ITK_LIB_PREFIX = 
     ITK_LIB_SUFFIX = .lib
   endif
   ITK_BINDIR = $(ITK_DIR)/lib/InsightToolkit
   ITK_LIB = $(ITK_LIBPATH_PREFIX)$(ITK_BINDIR) $(ITK_LIB_PREFIX)ITKAlgorithms$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)ITKStatistics$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)ITKBasicFilters$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)ITKCommon$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkvnl_algo$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkvnl$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkv3p_netlib$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkvcl$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itksys$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)ITKDICOMParser$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkzlib$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkzlib$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itktiff$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkjpeg12$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)itkjpeg16$(ITK_LIB_SUFFIX) $(ITK_LIB_PREFIX)ITKNrrdIO$(ITK_LIB_SUFFIX) 
else # $(USE_ITK) == true
   ITK_DEFINES =
   ITK_SRCDIR = 
   ITK_BINDIR =
   ITK_INC = 
   ITK_LIB = 
endif # $(USE_ITK) == true

ifeq ($(USE_NETGEN),true)
	NETGEN_LIB_PREFIX = lib
	NETGEN_LIB_SUFFIX = .a
	ifeq ($(COMPILER),msvc)
		NETGEN_LIB_PREFIX = 
		NETGEN_LIB_SUFFIX = .lib
	endif
	NETGEN_INC = -I$(CMISS_ROOT)/third_party/$(LIB_ARCH_DIR)/include
	NETGEN_LIB = $(CMISS_ROOT)/third_party/$(LIB_ARCH_DIR)/lib/$(NETGEN_LIB_PREFIX)nglib$(NETGEN_LIB_SUFFIX)
	NETGEN_DEFINES = -DUSE_NETGEN
else
	NETGEN_INC =
	NETGEN_LIB =
	NETGEN_DEFINES =
endif

ifneq ($(USE_PERL_INTERPRETER),true)
   INTERPRETER_INC =
   INTERPRETER_DEFINES = 
   INTERPRETER_SRCS =
   INTERPRETER_LIB =
   INTERPRETER_LINK_FLAGS =
else # ! USE_PERL_INTERPRETER
   INTERPRETER_PATH = $(CMISS_ROOT)/perl_interpreter
   INTERPRETER_INC = -I$(INTERPRETER_PATH)/source/
   INTERPRETER_DEFINES = -DUSE_PERL_INTERPRETER
   INTERPRETER_SRCS =
   INTERPRETER_LIB = \
	   $(INTERPRETER_PATH)/lib/$(LIB_ARCH_DIR)/libperlinterpreter.a
   INTERPRETER_LINK_FLAGS =
   ifeq ($(OPERATING_SYSTEM),win32)
      INTERPRETER_LIB = \
	      $(INTERPRETER_PATH)/lib/$(LIB_ARCH_DIR)/libperlinterpreter-includeperl.a
   endif # $(OPERATING_SYSTEM) == win32
endif # ! USE_PERL_INTERPRETER

ifneq ($(UNEMAP), true)
   UNEMAP_DEFINES =
   UNEMAP_SRCS =
else # UNEMAP != true
   # for all nodal stuff UNEMAP_DEFINES = -DUNEMAP -DSPECTRAL_TOOLS -DUNEMAP_USE_NODES 
   UNEMAP_DEFINES = -DUNEMAP -DSPECTRAL_TOOLS -DUNEMAP_USE_3D -DNOT_ACQUISITION_ONLY
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
	 UNEMAP_DEFINES += -DMOTIF
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
   UNEMAP_SRCS = \
	   unemap_application/acquisition.c \
	    unemap_application/acquisition_window.c \
	    unemap_application/acquisition_work_area.c \
	    unemap_application/analysis.c \
	    unemap_application/analysis_calculate.c \
	    unemap_application/analysis_drawing.c \
	    unemap_application/analysis_window.c \
	    unemap_application/analysis_work_area.c \
	    unemap_application/bard.c \
	    unemap_application/beekeeper.c \
	    unemap_application/cardiomapp.c \
	    unemap_application/delaunay.c \
	    unemap_application/edf.c \
	    unemap_application/eimaging_time_dialog.c \
	    unemap_application/drawing_2d.c \
	    unemap_application/interpolate.c \
	    unemap_application/map_dialog.c \
	    unemap_application/mapping.c \
	    unemap_application/mapping_window.c \
	    unemap_application/neurosoft.c \
	    unemap_application/pacing_window.c \
	    unemap_application/page_window.c \
	    unemap_application/rig.c \
	    unemap_application/rig_node.c \
	    unemap_application/setup_dialog.c \
	    unemap_application/spectral_methods.c \
	    unemap_application/system_window.c \
	    unemap_application/trace_window.c \
	    unemap_application/unemap_command.c \
	    unemap_application/unemap_hardware_client.c \
	    unemap_application/unemap_package.c 
endif # UNEMAP != true

ifndef CELL
   CELL_DEFINES =
   CELL_SRCS =
else # ! CELL 
   CELL_DEFINES = -DCELL -DCELL_DISTRIBUTED
   CELL_SRCS = \
	   cell/cell_calculate.c \
	   cell/cell_calculate_dialog.c \
	   cell/cell_cmgui_interface.c \
	   cell/cell_cmiss_interface.c \
	   cell/cell_component.c \
	   cell/cell_export_dialog.c \
	   cell/cell_graphic.c \
	   cell/cell_input.c \
	   cell/cell_interface.c \
	   cell/cell_output.c \
	   cell/cell_plot.c \
	   cell/cell_unemap_interface.c \
	   cell/cell_variable.c \
	   cell/cell_variable_editing_dialog.c \
	   cell/cell_variable_unemap_interface.c \
      cell/cell_window.c \
      cell/distributed_editing_dialog.c \
      cell/distributed_editing_interface.c \
      cell/cell_model_routines/andre.f \
      cell/integrator_routines/euler.f \
      cell/integrator_routines/improved_euler.f \
      cell/integrator_routines/runge_kutta.f
endif # ! CELL 

ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
   UIDH_INC = -I$(XRCH_PATH)
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
   UIDH_INC = -I$(UIDH_PATH)
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE

EXTERNAL_INPUT_DEFINES =
EXTERNAL_INPUT_LIB = 
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
  ifeq ($(SYSNAME:IRIX%=),)
    EXTERNAL_INPUT_DEFINES += -DDIALS -DSPACEBALL -DPOLHEMUS -DFARO -DEXT_INPUT -DSELECT_DESCRIPTORS
    EXTERNAL_INPUT_LIB += -lXext -lXi
  endif # SYSNAME == IRIX%=
  ifeq ($(SYSNAME),Linux)
    EXTERNAL_INPUT_DEFINES = -DSELECT_DESCRIPTORS
  endif # SYSNAME == Linux 
  ifeq ($(SYSNAME),AIX)
    EXTERNAL_INPUT_DEFINES = -DSELECT_DESCRIPTORS
  endif # SYSNAME == Linux 
  ifeq ($(SYSNAME:CYGWIN%=),)
    EXTERNAL_INPUT_DEFINES = -DSELECT_DESCRIPTORS
  endif # SYSNAME == CYGWIN%=
endif # GRAPHICS_API == OPENGL_GRAPHICS

HAPTIC_LIB =
HAPTIC_INC =
# HAPTIC_LIB = -L/usr/local/phantom/GHOST/lib -lghost_n32Mips3
# HAPTIC_INC = -I/usr/local/phantom/GHOST/lib
# HAPTIC_LIB = -L/usr/local/phantom/GHOST/lib -lghost_o32
# HAPTIC_INC = -I/usr/local/phantom/GHOST/lib

# XML Stuff goes here!!

ifndef CELL
   XML_PATH =
   XML_DEFINES = 
   XML_LIB =
   XML_INC = 
else # ! CELL
   XML_PATH = $(CMISS_ROOT)/xml
   XML_DEFINES =
   ifeq ($(SYSNAME:IRIX%=),)
      XML_DEFINES += -DIRIX -D_REENTRANT
   endif # SYSNAME == IRIX%=
   ifeq ($(SYSNAME),Linux)
      XML_DEFINES = -DLINUX -D_REENTRANT
   endif # SYSNAME == Linux
   XML_LIB = $(XML_PATH)/library/$(LIB_ARCH_DIR)/libXML-optimised-1.0.0.a
   XML_INC = -I$(XML_PATH)/include
endif # ! CELL

ifneq ($(USE_XML2),true)
   XML2_PATH =
   XML2_DEFINES =
   XML2_INC =
   XML2_LIB =
else
   XML2_PATH = $(CMISS_ROOT)/third_party/
   XML2_DEFINES = -DUSE_XML2
   XML2_INC = -I$(XML2_PATH)/include/$(LIB_ARCH_DIR)/libxml2/
   XML2_LIB = $(XML2_PATH)/lib/$(LIB_ARCH_DIR)/libxml2.a
ifndef USE_IMAGEMAGICK
   XML2_LIB += $(XML2_PATH)/lib/$(LIB_ARCH_DIR)/libz.a
endif # USE_IMAGEMAGICK
endif

STEREO_DISPLAY_DEFINES = -DSTEREO
# STEREO_DISPLAY_DEFINES =

# POSTSCRIPT_DEFINES = -DFEEDBACK_POSTSCRIPT
POSTSCRIPT_DEFINES =

#  SHORT_NAMES were created to support OS's where external names <= 32 characters
#  NAME_DEFINES = -DSHORT_NAMES
NAME_DEFINES =

#  Temporary flags that are used during development
TEMPORARY_DEVELOPMENT_FLAGS =
# TEMPORARY_DEVELOPMENT_FLAGS = -DINTERNAL_UIDS -DPHANTOM_FARO
# TEMPORARY_DEVELOPMENT_FLAGS = -DINTERPOLATE_TEXELS -DOTHER_FIBRE_DIR -DREPORT_GL_ERRORS
# TEMPORARY_DEVELOPMENT_FLAGS = -DDO_NOT_ADD_DETAIL
# TEMPORARY_DEVELOPMENT_FLAGS = -DSELECT_ELEMENTS -DMERGE_TIMES -DPETURB_LINES -DDEPTH_OF_FIELD -DDO_NOT_ADD_DETAIL -DSGI_MOVIE_FILE
# TEMPORARY_DEVELOPMENT_FLAGS = -DSELECT_ELEMENTS -DMERGE_TIMES -DPETURB_LINES -DDEPTH_OF_FIELD -DBLEND_NODAL_VALUES -DNEW_SPECTRUM
# TEMPORARY_DEVELOPMENT_FLAGS = -DSELECT_ELEMENTS -DMERGE_TIMES -DPETURB_LINES -DDEPTH_OF_FIELD -DDO_NOT_ADD_DETAIL -DBLEND_NODAL_VALUES -DALLOW_MORPH_UNEQUAL_POINTSETS
# TEMPORARY_DEVELOPMENT_FLAGS = -DWINDOWS_DEV_FLAG

HELP_DEFINES = -DNETSCAPE_HELP
HELP_INCLUDE =
HELP_LIB =
HELP_SRCS = \
	help/help_interface.c

MEMORYCHECK_LIB = 
ifeq ($(SYSNAME:IRIX%=),)
   ifdef MEMORYCHECK
      MEMORYCHECK_LIB += -lmalloc_ss -lfpe 
   endif # MEMORYCHECK
endif # SYSNAME == IRIX%=

USER_INTERFACE_INC = 
USER_INTERFACE_LIB =
ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
   ifeq ($(SYSNAME),Linux)
      USER_INTERFACE_INC += $(X_INC)
      USER_INTERFACE_LIB += $(GRAPHICS_LIB) -L$(X_LIB) 

      ifneq ($(STATIC_LINK),true)
         #I am statically linking Motif so that it does not have to be installed at runtime.
         #Debian distributions are marking libXp as deprecated, although Motif still requires it, so it isn't installed by default to lets link it statically too.
         USER_INTERFACE_LIB += -Wl,-Bstatic -lMrm -lXm -lXp -Wl,-Bdynamic -lXt -lX11 -lXmu -lXext -lICE
      else # STATIC_LINK != true
         #Mandrake 8.2 static libs are incompatible, this works around it by
         #comparing the size of the symbols and forcing Xmu to preload its
         #version if they differ in size.  Older greps don't have -o option.
         Xm_XeditRes = $(shell /usr/bin/objdump -t $(X_LIB)/libXm.a | /bin/grep '00000[0-f][0-f][1-f] _XEditResCheckMessages')
         Xmu_XeditRes = $(shell /usr/bin/objdump -t $(X_LIB)/libXmu.a | /bin/grep '00000[0-f][0-f][1-f] _XEditResCheckMessages')
         ifneq ($(Xm_XeditRes),)
            ifneq ($(Xmu_XeditRes),)
               USER_INTERFACE_LIB += -u _XEditResCheckMessages -lXmu 
            endif
         endif
         USER_INTERFACE_LIB += -lMrm -lXm -lXp -lXt -lX11 -lXmu -lXext -lSM -lICE
      endif # STATIC_LINK != true
   else # SYSNAME == Linux
      USER_INTERFACE_LIB += $(GRAPHICS_LIB) 
      ifeq ($(SYSNAME),Darwin)
         #OPENMOTIF_DIR set in common.Makefile
         USER_INTERFACE_INC += -I$(OPENMOTIF_DIR)/include
         USER_INTERFACE_LIB += $(OPENMOTIF_DIR)/lib/libMrm.a $(OPENMOTIF_DIR)/lib/libXm.a -L/usr/X11R6/lib -lXp -lXt -lX11 -lXmu -lXext
      else # SYSNAME == Darwin
         ifeq ($(SYSNAME:CYGWIN%=),)
            USER_INTERFACE_INC += -I/usr/X11R6/include
            X_LIB = /usr/X11R6/lib
            USER_INTERFACE_LIB += -L$(X_LIB)
            USER_INTERFACE_LIB += -lMrm -lXm -lXp -lXt -lX11 -lXmu -lXext -lSM -lICE
         else # SYSNAME == CYGWIN%=
            USER_INTERFACE_LIB += -lMrm -lXm -lXt -lX11 -lXmu -lXext
            ifeq ($(SYSNAME:IRIX%=),)
               USER_INTERFACE_LIB += -lSgm
            endif # SYSNAME == IRIX%=
         endif # SYSNAME == CYGWIN%=
      endif # SYSNAME == Darwin
   endif # SYSNAME == Linux
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
ifeq ($(USER_INTERFACE),GTK_USER_INTERFACE)
   ifeq ($(SYSNAME),Linux)
      USER_INTERFACE_INC += $(X_INC)
      USER_INTERFACE_LIB += -L$(X_LIB)
   endif
   ifeq ($(SYSNAME:CYGWIN%=),)
      X_LIB = /usr/X11R6/lib
      USER_INTERFACE_LIB += -L$(X_LIB)
   endif # SYSNAME == CYGWIN%=
   ifneq ($(SYSNAME),win32)
      USE_GTK2 = true
      ifeq ($(USE_GTK2),true)
         USER_INTERFACE_INC += $(shell pkg-config gtkglext-1.0 gtk+-2.0 --cflags)
         ifneq ($(STATIC_LINK),true)
            USER_INTERFACE_LIB += -Wl,-Bstatic -lgtkglext-x11-1.0 -lgdkglext-x11-1.0 -Wl,-Bdynamic $(shell pkg-config gtk+-2.0 pangox --libs) -lXmu $(GRAPHICS_LIB)
         else # $(STATIC_LINK) != true
            USER_INTERFACE_LIB += -lgtkglext-x11-1.0 -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 $(GRAPHICS_LIB)
         endif # $(STATIC_LINK) != true
      else # $(USE_GTK2) == true
         USER_INTERFACE_INC +=  -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include/
         USER_INTERFACE_LIB +=  -lgtkgl $(GRAPHICS_LIB) -lgtk -lgdk -lgmodule -lglib -ldl -lXi -lXext -lX11
      endif # $(USE_GTK2) == true
   else # $(SYSNAME) != win32
      # SAB It seems that ld currently requires (version 2.13.90 20021005) the 
      # perl_library to have a full filename alphabetically ahead of these gtk libs,
      # therefore be careful if you relocate these libraries perl_interpreter versus
      # win32_lib
      USER_INTERFACE_PATH = $(CMISS_ROOT)/win32lib/gtk2
      USER_INTERFACE_INC += -I$(USER_INTERFACE_PATH)/include/gtk-2.0 -I$(USER_INTERFACE_PATH)/include/pango-1.0 -I$(USER_INTERFACE_PATH)/include/glib-2.0/ -I$(USER_INTERFACE_PATH)/include/atk-1.0 -I$(USER_INTERFACE_PATH)/include/gtkgl-2.0/ -I$(USER_INTERFACE_PATH)/lib/glib-2.0/include -I$(USER_INTERFACE_PATH)/lib/gtk-2.0/include
      USER_INTERFACE_LIB += -L$(USER_INTERFACE_PATH)/lib -lgtkgl-2.0 -lgtk-win32-2.0.dll -lgdk-win32-2.0.dll -latk-1.0.dll -lgdk_pixbuf-2.0.dll -lpangowin32-1.0.dll -lpango-1.0.dll -lgobject-2.0.dll -lgmodule-2.0.dll -lglib-2.0.dll $(GRAPHICS_LIB)
      # USER_INTERFACE_INC = -L"c:\perl\5.6.1\lib\MSWin32-x86\CORE" -L"c:\dev\gcc\lib" -Ic:/dev/gtk2/include/gtk-2.0 -Ic:/dev/gtk2/include/pango-1.0 -Ic:/dev/gtk2/include/glib-2.0/ -Ic:/dev/gtk2/include/atk-1.0 -Ic:/dev/gtk2/include/gtkgl-2.0/ -Ic:/dev/gtk2/lib/glib-2.0/include -Ic:/dev/gtk2/lib/gtk-2.0/include
      # USER_INTERFACE_LIB += -L"c:\dev\gtk2\lib" -lgtkgl-2.0 -lgtk-win32-2.0 -lgdk-win32-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangowin32-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 */
   endif # $(SYSNAME) != win32
endif # $(USER_INTERFACE) == GTK_USER_INTERFACE
ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)
   WX_DIR = 
   ifneq ($(DEBUG),true)
     WX_DEBUG_FLAG = no
   else # $(DEBUG) != true
     WX_DEBUG_FLAG = yes
   endif # $(DEBUG) != true
   #Default list does not include gl, so we list them here.
   #Using xrc means that we require most things (and static wx libs don't automatically pull
   #in the other dependent wx-libs)
   #Use linkdeps so that we don't get all the other system libraries.
   WX_STATIC_FLAG=yes
   WX_CONFIG_LIBS := $(shell $(WX_DIR)wx-config --linkdeps --unicode=no --debug=$(WX_DEBUG_FLAG) --static=$(WX_STATIC_FLAG) xrc,gl,xml,adv,html,core,base)
   ifneq ($(WX_CONFIG_LIBS),)
     #Presume it succeeded, use this config
     USER_INTERFACE_LIB += $(WX_CONFIG_LIBS)
   else
     $(warning Static wx build not detected, trying a dynamic version)
     #Use the full libs
     WX_STATIC_FLAG = no
     WX_CONFIG_LIBS := $(shell $(WX_DIR)wx-config --libs --unicode=no --debug=$(WX_DEBUG_FLAG) --static=$(WX_STATIC_FLAG) xrc,gl,xml,adv,html,core,base)
     ifneq ($(WX_CONFIG_LIBS),)
       #Presume this worked
       USER_INTERFACE_LIB += $(WX_CONFIG_LIBS)
     else
       $(error cmgui build error wx config not matched for $(WX_DIR)wx-config --libs --unicode=no --debug=$(WX_DEBUG_FLAG) xrc,gl,xml,adv,html,core,base)
     endif
   endif
   WX_USER_INTERFACE_INC := $(shell $(WX_DIR)wx-config --cxxflags --unicode=no --debug=$(WX_DEBUG_FLAG) --static=$(WX_STATIC_FLAG))
   WINDRES = $(shell $(WX_DIR)wx-config --rescomp --unicode=no --debug=$(WX_DEBUG_FLAG) --static=$(WX_STATIC_FLAG))
   USER_INTERFACE_INC += $(WX_USER_INTERFACE_INC)
   USER_INTERFACE_LIB += $(GRAPHICS_LIB)
   ifeq ($(OPERATING_SYSTEM),linux)
      ifneq ($(STATIC_LINK),true)
         USER_INTERFACE_LIB += $(shell pkg-config gtk+-2.0 gthread-2.0 --libs) -lXmu  -lXinerama -lXxf86vm
      else # $(STATIC_LINK) != true
         USER_INTERFACE_LIB += -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lwxexpat-2.6-i686-linux
      endif # $(STATIC_LINK) != true
   else # $(OPERATING_SYSTEM) == linux
      ifeq ($(OPERATING_SYSTEM),win32)
         USER_INTERFACE_LIB += -lcomctl32 -lctl3d32 -L$(shell $(WX_DIR)wx-config --prefix)/lib -lwxexpat-2.8-i386-mingw32msvc
      else # $(OPERATING_SYSTEM) == win32
         ifeq ($(OPERATING_SYSTEM),darwin)
            USER_INTERFACE_LIB += -L$(shell $(WX_DIR)wx-config --prefix)/lib -framework QuickTime -framework IOKit -framework Carbon -framework Cocoa -framework System -framework WebKit
         else # $(OPERATING_SYSTEM) == darwin
            USER_INTERFACE_LIB += $(shell $(WX_DIR)wx-config --libs xrc,gl,xml,adv,html,core,base)
         endif # $(OPERATING_SYSTEM) == darwin
      endif # $(OPERATING_SYSTEM) == win32
   endif # $(OPERATING_SYSTEM) == linux
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE),CARBON_USER_INTERFACE)
   USER_INTERFACE_INC += 
	USER_INTERFACE_LIB += $(GRAPHICS_LIB)
endif # $(USER_INTERFACE) == CARBON_USER_INTERFACE
ifeq ($(USER_INTERFACE),CONSOLE_USER_INTERFACE)
   USER_INTERFACE_INC +=
   USER_INTERFACE_LIB += $(GRAPHICS_LIB)
endif # $(USER_INTERFACE) == CONSOLE_USER_INTERFACE
ifeq ($(USER_INTERFACE),WIN32_USER_INTERFACE)
   USER_INTERFACE_INC +=
   USER_INTERFACE_LIB += $(GRAPHICS_LIB)
endif # $(USER_INTERFACE) == WIN32_USER_INTERFACE

ifeq ($(SYSNAME:IRIX%=),)
   SYSTEM_LIB = -lPW -lftn -lm -lC -lCio -lpthread 
endif # SYSNAME == IRIX%=
ifeq ($(SYSNAME),Linux)
   ifneq ($(STATIC_LINK),true)
      #For the dynamic link we really need to statically link the c++ as this
      #seems to be particularly variable between distributions.
      LIBSTDC++ = $(shell g++ --print-file-name=libstdc++.a)
      SYSTEM_LIB = $(LIBG2C) $(LIBSTDC++) -lcrypt -lm -ldl -lc -lpthread
      # For the shared object libraries the stdc++ is included several times, do thsi
      # dynamically.
      SOLIB_LIB = $(LIBG2C) -lstdc++ -lcrypt -lm -ldl -lc -lpthread
#???DB.  Need setenv LD_RUN_PATH /home/bullivan/gcc-3.3.1/lib
#      SYSTEM_LIB = -L/home/bullivan/gcc-3.3.1/lib -lg2c -lm -ldl -lc -lpthread /usr/lib/libcrypt.a -lstdc++
   else # $(STATIC_LINK) != true
      SYSTEM_LIB = -lg2c -lm -ldl -lpthread -lcrypt -lstdc++
#      SYSTEM_LIB = -L/home/bullivan/gcc-3.3.1/lib -lg2c -lm -ldl -lpthread -lcrypt -lstdc++
   endif # $(STATIC_LINK) != true
endif # SYSNAME == Linux
ifeq ($(SYSNAME),AIX)
   SYSTEM_LIB = -lm -ldl -lSM -lICE -lpthread -lcrypt -lbsd -lld -lC128
endif # SYSNAME == AIX
ifeq ($(SYSNAME),win32)
   ifeq ($(COMPILER),msvc)
     CYGWINPATH := $(shell cygpath --mixed /)
     GCCLIBGCC = $(patsubst /usr/lib%,$(CYGWINPATH)/lib%,$(shell gcc -mno-cygwin -print-file-name=libgcc.a))
     GCCLIBSTDC++ = $(patsubst /usr/lib%,$(CYGWINPATH)/lib%,$(shell gcc -mno-cygwin -print-file-name=libstdc++.a))
     LIBMINGW32 = $(CYGWINPATH)/lib/mingw/libmingw32.a
     LIBMINGWEX = $(CYGWINPATH)/lib/mingw/libmingwex.a
     LIBMINGW_MSVCRT = $(CYGWINPATH)/lib/mingw/libmsvcrt.a
     SYSTEM_LIB = $(GCCLIBSTDC++) $(LIBMINGW32) $(GCCLIBGCC) $(LIBMINGW_MSVCRT) $(LIBMINGWEX) kernel32.lib advapi32.lib ws2_32.lib gdi32.lib msimg32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib netapi32.lib uuid.lib wsock32.lib mpr.lib winmm.lib version.lib odbc32.lib user32.lib /NODEFAULTLIB:LIBCMT /DEFAULTLIB:MSVCRT
     SOLIB_LIB = $(GCCLIBSTDC++) $(LIBMINGW32) $(GCCLIBGCC) $(LIBMINGWEX)
   else
     SYSTEM_LIB = -lws2_32 -lgdi32 -lmsimg32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -lnetapi32 -luuid -lwsock32 -lmpr -lwinmm -lversion -lodbc32 -lstdc++
   endif
endif # SYSNAME == win32
ifeq ($(SYSNAME),Darwin)
      # The lib -lSystemStubs is used to resolve some linking differences between some code
      # with gcc-3.3 and gcc-4
      SYSTEM_LIB = /usr/local/g77/lib/libg2c.a -lm -ldl -lpthread -liconv -lstdc++ -lSystemStubs
endif # SYSNAME == Darwin

ALL_DEFINES = $(COMPILE_DEFINES) $(TARGET_TYPE_DEFINES) $(FE_VALUE_DEFINES) \
	$(PLATFORM_DEFINES) $(OPERATING_SYSTEM_DEFINES) $(USER_INTERFACE_DEFINES) \
	$(STEREO_DISPLAY_DEFINES) $(CONNECTIVITY_DEFINES) \
	$(EXTERNAL_INPUT_DEFINES) \
	$(GRAPHICS_LIBRARY_DEFINES) $(HELP_DEFINES) \
	$(POSTSCRIPT_DEFINES) $(NAME_DEFINES) $(TEMPORARY_DEVELOPMENT_FLAGS) \
	$(UNEMAP_DEFINES) $(ITK_DEFINES) \
	$(CELL_DEFINES) $(MOVIE_FILE_DEFINES) $(INTERPRETER_DEFINES)\
	$(IMAGEMAGICK_DEFINES) $(XML2_DEFINES) $(NETGEN_DEFINES)

ALL_INCLUDES = $(SOURCE_DIRECTORY_INC) $(HAPTIC_INC) $(WORMHOLE_INC) \
	$(XML_INC) $(UIDH_INC) $(GRAPHICS_INC) $(USER_INTERFACE_INC) \
	$(INTERPRETER_INC) $(IMAGEMAGICK_INC) $(ITK_INC) $(XML2_INC) $(NETGEN_INC)

ALL_FLAGS = $(OPTIMISATION_FLAGS) $(COMPILE_FLAGS) $(TARGET_TYPE_FLAGS) \
	$(ALL_DEFINES) $(ALL_INCLUDES)

ALL_LIB = $(LINKOPTIONFLAG) $(USER_INTERFACE_LIB) $(HAPTIC_LIB) \
	$(WORMHOLE_LIB) $(INTERPRETER_LIB) $(IMAGEMAGICK_LIB) \
	$(EXTERNAL_INPUT_LIB) $(HELP_LIB) $(ITK_LIB) \
	$(MOVIE_FILE_LIB) $(XML_LIB) $(XML2_LIB) $(MEMORYCHECK_LIB) \
	$(SYSTEM_LIB) $(NETGEN_LIB)

API_SRCS = \
	api/cmiss_command_data.c \
	api/cmiss_field.c \
	api/cmiss_field_composite.c \
	api/cmiss_core.c \
	api/cmiss_element.c \
	api/cmiss_idle.c \
	api/cmiss_node.c \
	api/cmiss_region.c \
	api/cmiss_time.c \
	api/cmiss_time_keeper.c \
	api/cmiss_time_sequence.c \
	api/cmiss_timer.c
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   API_SRCS += \
	   api/cmiss_scene_viewer.c \
		api/cmiss_texture.c
endif
API_INTERFACE_SRCS = \
	api/cmiss_graphics_window.c
CHOOSE_INTERFACE_SRCS = \
	choose/choose_computed_field.c \
	choose/choose_curve.c \
	choose/choose_enumerator.c \
	choose/choose_fe_field.c \
	choose/choose_field_component.c \
	choose/choose_graphical_material.c \
	choose/choose_spectrum.c \
	choose/choose_texture.c \
	choose/choose_volume_texture.c \
	choose/chooser.c \
	choose/text_choose_fe_element.c \
	choose/text_choose_fe_node.c
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   CHOOSE_INTERFACE_SRCS += \
	choose/choose_gt_object.c \
	choose/choose_scene.c
endif
ifneq ($(USER_INTERFACE),WX_USER_INTERFACE)
COLOUR_INTERFACE_SRCS = \
	colour/colour_editor.c \
	colour/edit_var.c 
else
COLOUR_INTERFACE_SRCS = \
	colour/colour_editor_wx.cpp
endif
COMFILE_SRCS = \
	comfile/comfile.cpp
ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)
COMFILE_SRCS += \
    comfile/comfile_window_wx.cpp
endif #$(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
COMFILE_INTERFACE_SRCS = \
	comfile/comfile_window.c 
endif #$(USER_INTERFACE) == MOTIF_USER_INTERFACE
COMMAND_SRCS = \
	command/command.c \
	command/console.c \
	command/example_path.c \
	command/parser.c
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
COMMAND_SRCS += \
	command/cmiss.c
endif
COMMAND_INTERFACE_SRCS = \
	command/command_window.cpp
COMPUTED_FIELD_CORE_SRCS =\
	computed_field/computed_field.cpp \
	computed_field/computed_field_arithmetic_operators.cpp \
	computed_field/computed_field_composite.cpp \
	computed_field/computed_field_conditional.cpp \
	computed_field/computed_field_find_xi.cpp \
	computed_field/computed_field_finite_element.cpp \
	computed_field/computed_field_set.cpp \
	computed_field/field_location.cpp
COMPUTED_FIELD_GRAPHICS_SRCS = \
	computed_field/computed_field_find_xi_graphics.cpp
COMPUTED_FIELD_SRCS = \
	$(COMPUTED_FIELD_CORE_SRCS) \
	$(COMPUTED_FIELD_GRAPHICS_SRCS) \
	minimise/minimise.cpp \
	computed_field/computed_field_alias.cpp \
	computed_field/computed_field_compose.cpp \
	computed_field/computed_field_curve.cpp \
	computed_field/computed_field_coordinate.cpp \
	computed_field/computed_field_deformation.cpp \
	computed_field/computed_field_fibres.cpp \
	computed_field/computed_field_function.cpp \
	computed_field/computed_field_image.cpp \
	computed_field/computed_field_integration.cpp \
	computed_field/computed_field_logical_operators.cpp \
	computed_field/computed_field_lookup.cpp \
	computed_field/computed_field_matrix_operations.cpp \
	computed_field/computed_field_region_operations.cpp \
	computed_field/computed_field_string_constant.cpp \
	computed_field/computed_field_time.cpp \
	computed_field/computed_field_trigonometry.cpp \
	computed_field/computed_field_update.cpp \
	computed_field/computed_field_value_index_ranges.cpp \
	computed_field/computed_field_vector_operations.cpp \
	computed_field/computed_field_wrappers.cpp
COMPUTED_FIELD_INTERFACE_SRCS = \
	computed_field/computed_field_window_projection.cpp
ifeq ($(USE_ITK),true)
COMPUTED_FIELD_SRCS += \
	computed_field/computed_field_derivatives.cpp
endif # $(USE_ITK) == true
CURVE_SRCS = \
	curve/curve.c
CURVE_INTERFACE_SRCS = \
	curve/curve_editor.c \
	curve/curve_editor_dialog.c
DOF3_INTERFACE_SRCS = \
	dof3/dof3.c \
	dof3/dof3_control.c \
	dof3/dof3_input.c
ELEMENT_SRCS = \
	element/element_operations.c \
	element/element_point_tool.cpp \
	element/element_tool.cpp
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
ELEMENT_SRCS += \
	 element/element_point_viewer_wx.cpp
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
ELEMENT_INTERFACE_SRCS = \
	element/element_creator.c \
	element/element_point_field_viewer_widget.c \
	element/element_point_viewer.c \
	element/element_point_viewer_widget.c
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
EMOTER_SRCS = \
	emoter/em_cmgui.c \
	emoter/emoter_dialog.c
FINITE_ELEMENT_CORE_SRCS = \
	finite_element/export_finite_element.c \
	finite_element/finite_element.c \
	finite_element/finite_element_discretization.c \
	finite_element/finite_element_helper.cpp \
	finite_element/finite_element_region.c \
	finite_element/finite_element_time.c \
	finite_element/import_finite_element.c
FINITE_ELEMENT_GRAPHICS_SRCS = \
	finite_element/finite_element_to_graphics_object.cpp \
	finite_element/finite_element_to_iso_lines.cpp \
	finite_element/finite_element_to_iso_surfaces.cpp \
	finite_element/finite_element_to_streamlines.c
FINITE_ELEMENT_SRCS = \
	$(FINITE_ELEMENT_CORE_SRCS) \
	$(FINITE_ELEMENT_GRAPHICS_SRCS) \
	finite_element/export_cm_files.c \
	finite_element/finite_element_adjacent_elements.c \
	finite_element/finite_element_conversion.c \
	finite_element/finite_element_to_iges.c \
	finite_element/read_fieldml.c \
	finite_element/snake.c \
	finite_element/write_fieldml.c
ifeq ($(USE_NETGEN),true)
	FINITE_ELEMENT_SRCS += \
		finite_element/generate_mesh_netgen.cpp
endif

FINITE_ELEMENT_INTERFACE_SRCS = \
	finite_element/grid_field_calculator.c
GENERAL_SRCS = \
	general/any_object.c \
	general/callback.c \
	general/child_process.c \
	general/compare.c \
	general/debug.c \
	general/error_handler.c \
	general/geometry.cpp \
	general/heapsort.c \
	general/image_utilities.c \
	general/indexed_multi_range.c \
	general/integration.c \
	general/io_stream.c \
	general/machine.c \
	general/matrix_vector.c \
	general/multi_range.c \
	general/myio.c \
	general/mystring.c \
	general/octree.c \
	general/statistics.c \
	general/time.cpp \
	general/value.c 
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   GENERAL_SRCS += \
      general/photogrammetry.c
endif
GENERAL_INTERFACE_SRCS = \
	general/postscript.c
GRAPHICS_SRCS = \
	graphics/auxiliary_graphics_types.c \
	graphics/colour.c \
	graphics/complex.c \
	graphics/decimate_voltex.c \
	graphics/defined_graphics_objects.c \
	graphics/element_group_settings.c \
	graphics/element_point_ranges.c \
	graphics/environment_map.c \
	graphics/glyph.c \
	graphics/import_graphics_object.c \
	graphics/iso_field_calculation.c \
	graphics/laguer.c \
	graphics/material.c \
	graphics/mcubes.c \
	graphics/order_independent_transparency.c \
	graphics/render_to_finite_elements.c \
	graphics/renderstl.cpp \
	graphics/rendervrml.c \
	graphics/renderwavefront.c \
	graphics/render_triangularisation.cpp \
	graphics/quaternion.cpp\
	graphics/selected_graphic.c \
	graphics/spectrum.cpp \
	graphics/spectrum_settings.c \
	graphics/texture.c \
	graphics/texture_line.c \
	graphics/transform_tool.cpp \
	graphics/triangle_mesh.cpp \
	graphics/userdef_objects.c \
	graphics/volume_texture.c
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   GRAPHICS_SRCS += \
	   graphics/font.cpp \
	   graphics/graphical_element.cpp \
		graphics/graphics_library.c \
		graphics/graphics_object.c \
		graphics/light.c \
		graphics/light_model.c \
		graphics/render.cpp \
		graphics/rendergl.c \
		graphics/scene.c \
		graphics/scene_viewer.c \
		graphics/tile_graphics_objects.c
endif
GRAPHICS_INTERFACE_SRCS = \
	graphics/graphical_element_editor.c \
	graphics/movie_graphics.c
ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)
GRAPHICS_SRCS += \
    graphics/scene_editor_wx.cpp\
    graphics/spectrum_editor_wx.cpp \
	graphics/spectrum_editor_dialog_wx.cpp
endif #$(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
   GRAPHICS_INTERFACE_SRCS += \
		graphics/graphics_window.cpp \
		graphics/scene_editor.cpp \
		graphics/settings_editor.c \
		graphics/spectrum_editor.c \
		graphics/spectrum_editor_dialog.c \
		graphics/spectrum_editor_settings.c
endif
IMAGE_PROCESSING_SRCS = \
   image_processing/computed_field_image_resample.cpp
ifeq ($(USE_ITK),true)
   IMAGE_PROCESSING_SRCS += \
	   image_processing/computed_field_threshold_image_filter.cpp \
	   image_processing/computed_field_binary_threshold_image_filter.cpp \
	   image_processing/computed_field_cannyEdgeDetectionFilter.cpp \
	   image_processing/computed_field_meanImageFilter.cpp \
	   image_processing/computed_field_sigmoidImageFilter.cpp \
	   image_processing/computed_field_discrete_gaussian_image_filter.cpp \
	   image_processing/computed_field_curvatureAnisotropicDiffusionImageFilter.cpp \
	   image_processing/computed_field_derivativeImageFilter.cpp \
	   image_processing/computed_field_rescaleIntensityImageFilter.cpp \
	   image_processing/computed_field_connected_threshold_image_filter.cpp \
	   image_processing/computed_field_gradient_magnitude_recursive_gaussian_image_filter.cpp \
	   image_processing/computed_field_fast_marching_image_filter.cpp \
	   image_processing/computed_field_binary_dilate_image_filter.cpp \
	   image_processing/computed_field_binary_erode_image_filter.cpp \
	   image_processing/computed_field_histogram_image_filter.cpp \
	   image_processing/computed_field_ImageFilter.cpp
endif # $(USE_ITK) == true
INTERACTION_SRCS = \
	interaction/interaction_graphics.c \
	interaction/interaction_volume.c \
	interaction/interactive_event.c \
	interaction/interactive_tool.c
INTERACTION_INTERFACE_SRCS = \
	interaction/interactive_toolbar_widget.c \
	interaction/select_tool.c
IO_DEVICES_SRCS = \
	io_devices/conversion.c \
	io_devices/io_device.c \
	io_devices/matrix.c
IO_DEVICES_INTERFACE_SRCS = \
	io_devices/haptic_input_module.cpp \
	io_devices/input_module.c \
	io_devices/input_module_dialog.c \
	io_devices/input_module_widget.c
ifeq ($(LINK_CMISS),false)
   LINK_INTERFACE_SRCS =
else # LINK_CMISS
   LINK_INTERFACE_SRCS =  \
	   link/cmiss.c
endif # LINK_CMISS
MATERIAL_INTERFACE_SRCS =
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
ifneq ($(USER_INTERFACE),WX_USER_INTERFACE)
   MATERIAL_INTERFACE_SRCS +=  \
		material/material_editor.cpp \
		material/material_editor_dialog.c
else
	MATERIAL_INTERFACE_SRCS += \
		material/material_editor_dialog_wx.cpp \
		material/material_editor_wx.cpp
endif #$(USER_INTERFACE) == WX_USER_INTERFACE
endif
MOTIF_INTERFACE_SRCS =  \
	motif/image_utilities.c
NODE_SRCS = \
	node/node_operations.c \
	node/node_tool.cpp
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
NODE_SRCS += \
	node/node_viewer_wx.cpp
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE), MOTIF_USER_INTERFACE)
NODE_INTERFACE_SRCS = \
	node/node_field_viewer_widget.c \
	node/node_viewer.c \
	node/node_viewer_widget.c
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
REGION_SRCS = \
   region/cmiss_region.c \
   region/cmiss_region_write_info.c
ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)
REGION_INTERFACE_SRCS = \
   region/cmiss_region_chooser_wx.cpp
else
REGION_INTERFACE_SRCS = \
   region/cmiss_region_chooser.c
endif #$(USER_INTERFACE) == WX_USER_INTERFACE
SELECT_INTERFACE_SRCS = \
	select/select_curve.c \
	select/select_environment_map.c \
	select/select_graphical_material.c \
	select/select_private.c \
	select/select_spectrum.c
SELECTION_SRCS = \
	selection/any_object_selection.c \
	selection/element_point_ranges_selection.c \
	selection/element_selection.c \
	selection/node_selection.c

THREE_D_DRAWING_SRCS =
ifeq ($(GRAPHICS_API), OPENGL_GRAPHICS)
	THREE_D_DRAWING_SRCS += \
		three_d_drawing/graphics_buffer.c \
	   three_d_drawing/window_system_extensions.cpp
endif
THREE_D_DRAWING_INTERFACE_SRCS = \
	three_d_drawing/movie_extensions.c \
	three_d_drawing/ThreeDDraw.c
TIME_SRCS = \
	time/time.c \
	time/time_keeper.c
TIME_INTERFACE_SRCS = \
	time/time_editor.c \
	time/time_editor_dialog.c
ifneq ($(USER_INTERFACE),WX_USER_INTERFACE)
	TRANSFORMATION_INTERFACE_SRCS = \
		transformation/transformation_editor.c
else
	TRANSFORMATION_INTERFACE_SRCS = \
		transformation/transformation_editor_wx.cpp
endif
USER_INTERFACE_SRCS = \
	user_interface/confirmation.cpp \
	user_interface/event_dispatcher.c \
	user_interface/filedir.cpp \
	user_interface/message.c \
	user_interface/user_interface.c \
	user_interface/idle.c \
	user_interface/timer.c
USER_INTERFACE_INTERFACE_SRCS = \
	user_interface/call_work_procedures.c \
	user_interface/printer.c
VIEW_INTERFACE_SRCS = \
	view/camera.c \
	view/coord.c \
	view/coord_trans.c \
	view/poi.c \
	view/vector.c \
	view/view.c \
	view/view_control.c 

# DB.  makedepend has problems if too many files
SRCS_1 = \
	$(API_SRCS) \
	$(COMFILE_SRCS) \
	$(COMMAND_SRCS) \
	$(COMPUTED_FIELD_SRCS) \
	$(CURVE_SRCS) \
	$(ELEMENT_SRCS) \
	$(EMOTER_SRCS) \
	$(FINITE_ELEMENT_SRCS) \
	$(GENERAL_SRCS) \
	$(GRAPHICS_SRCS) \
	$(HELP_SRCS) \
	$(IMAGE_PROCESSING_SRCS) \
	$(INTERACTION_SRCS) \
	$(IO_DEVICES_SRCS) \
	$(NODE_SRCS) \
	$(REGION_SRCS) \
	$(SELECTION_SRCS) \
	$(INTERPRETER_SRCS) \
	$(THREE_D_DRAWING_SRCS) \
	$(TIME_SRCS) \
	$(USER_INTERFACE_SRCS)

ifeq ($(USER_INTERFACE),MOTIF_USER_INTERFACE)
   SRCS_2 = \
	   $(API_INTERFACE_SRCS) \
	   $(CELL_SRCS) \
	   $(CHOOSE_INTERFACE_SRCS) \
	   $(COLOUR_INTERFACE_SRCS) \
	   $(COMFILE_INTERFACE_SRCS) \
	   $(COMMAND_INTERFACE_SRCS) \
	   $(COMPUTED_FIELD_INTERFACE_SRCS) \
	   $(CURVE_INTERFACE_SRCS) \
	   $(DOF3_INTERFACE_SRCS) \
	   $(ELEMENT_INTERFACE_SRCS) \
	   $(FINITE_ELEMENT_INTERFACE_SRCS) \
	   $(GENERAL_INTERFACE_SRCS) \
	   $(GRAPHICS_INTERFACE_SRCS) \
	   $(INTERACTION_INTERFACE_SRCS) \
	   $(IO_DEVICES_INTERFACE_SRCS) \
	   $(LINK_INTERFACE_SRCS) \
	   $(MATERIAL_INTERFACE_SRCS) \
	   $(MOTIF_INTERFACE_SRCS) \
	   $(NODE_INTERFACE_SRCS) \
	   $(REGION_INTERFACE_SRCS) \
	   $(SELECT_INTERFACE_SRCS) \
	   $(SLIDER_INTERFACE_SRCS) \
	   $(THREE_D_DRAWING_INTERFACE_SRCS) \
	   $(TIME_INTERFACE_SRCS) \
	   $(TRANSFORMATION_INTERFACE_SRCS) \
	   $(USER_INTERFACE_INTERFACE_SRCS) \
	   $(VIEW_INTERFACE_SRCS)
endif # $(USER_INTERFACE) == MOTIF_USER_INTERFACE
ifeq ($(USER_INTERFACE),GTK_USER_INTERFACE)
      SRCS_2 = \
	      $(API_INTERFACE_SRCS) \
	      $(COMMAND_INTERFACE_SRCS) \
	      $(COMPUTED_FIELD_INTERFACE_SRCS) \
	      graphics/graphics_window.cpp \
	      gtk/gtk_cmiss_scene_viewer.c
endif # $(USER_INTERFACE) == GTK_USER_INTERFACE
ifeq ($(USER_INTERFACE),WX_USER_INTERFACE)
      SRCS_2 = \
	      $(API_INTERFACE_SRCS) \
	      $(COLOUR_INTERFACE_SRCS)\
	      $(COMMAND_INTERFACE_SRCS) \
	      $(COMPUTED_FIELD_INTERFACE_SRCS) \
	      $(MATERIAL_INTERFACE_SRCS) \
	      $(REGION_INTERFACE_SRCS) \
   		  $(TRANSFORMATION_INTERFACE_SRCS) \
	      graphics/graphics_window.cpp
endif # $(USER_INTERFACE) == WX_USER_INTERFACE
ifeq ($(USER_INTERFACE),CARBON_USER_INTERFACE)
      SRCS_2 = \
	      $(API_INTERFACE_SRCS) \
	      $(COMMAND_INTERFACE_SRCS) \
	      $(COMPUTED_FIELD_INTERFACE_SRCS) \
	      graphics/graphics_window.cpp
endif # $(USER_INTERFACE) == CARBON_USER_INTERFACE
ifeq ($(USER_INTERFACE),WIN32_USER_INTERFACE)
      SRCS_2 = \
	      $(API_INTERFACE_SRCS) \
	      $(COMMAND_INTERFACE_SRCS) \
	      $(COMPUTED_FIELD_INTERFACE_SRCS) \
	      graphics/graphics_window.cpp
endif # $(USER_INTERFACE) == WIN32_USER_INTERFACE

SRCS = $(SRCS_1) $(SRCS_2)

ifneq ($(MEMORYCHECK),true)
   OBJSA = $(SRCS:.c=.$(OBJ_SUFFIX))
   OBJSB = $(OBJSA:.cpp=.$(OBJ_SUFFIX))
   OBJS = $(OBJSB:.f=.$(OBJ_SUFFIX))
else # $(MEMORYCHECK) != true
   OBJSA = $(SRCS:.c=.$(OBJ_SUFFIX))
   OBJSB = $(OBJSA:.cpp=.$(OBJ_SUFFIX))
   # Override the one changed file in the source list 
   OBJSC = $(OBJSB:general/debug.$(OBJ_SUFFIX)=general/debug_memory_check.$(OBJ_SUFFIX))
   OBJS = $(OBJSC:.f=.$(OBJ_SUFFIX))
endif # $(MEMORYCHECK) != true

UNEMAP_OBJS = $(UNEMAP_SRCS:.c=.$(OBJ_SUFFIX))

MAIN_SRC = cmgui.c
MAIN_OBJ = $(MAIN_SRC:.c=.$(OBJ_SUFFIX))

clean :
	-rm -r $(OBJECT_PATH)
ifdef UIDH_PATH
	-rm -r $(UIDH_PATH)
endif # UIDH_PATH
ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
	-rm -r $(XRCH_PATH)
endif # $(USER_INTERFACE) == WX_USER_INTERFACE

clobber : clean
	-rm $(BIN_PATH)/$(BIN_TARGET)

$(OBJECT_PATH)/version.$(OBJ_SUFFIX).h : $(OBJS) $(UNEMAP_OBJS) cmgui.Makefile
	if [ ! -d $(OBJECT_PATH) ]; then \
		mkdir -p $(OBJECT_PATH); \
	fi	
	echo '/* This is a generated file.  Do not edit.  Edit cmgui.c or cmgui.imake instead */' > $(OBJECT_PATH)/version.$(OBJ_SUFFIX).h;	  
	date > date.h
	sed 's/"//;s/./#define VERSION "CMISS(cmgui) version 2.6.1  &/;s/.$$/&\\nCopyright 1996-2009, Auckland UniServices Ltd."/' < date.h >> $(OBJECT_PATH)/version.$(OBJ_SUFFIX).h

$(MAIN_OBJ) : $(MAIN_SRC) $(OBJECT_PATH)/version.$(OBJ_SUFFIX).h $(INTERPRETER_LIB)
	@set -x; \
	cat $(OBJECT_PATH)/version.$(OBJ_SUFFIX).h $*.c > $(OBJECT_PATH)/$*.$(OBJ_SUFFIX).c ;
	$(CC) $(CCOFLAG)$(OBJECT_PATH)/$*.$(OBJ_SUFFIX) $(ALL_FLAGS) $(OBJECT_PATH)/$*.$(OBJ_SUFFIX).c;

# We want to limit the dynamic symbols in the final executable so that
# dlopen routines can't access symbols we don't want them to */
ifeq ($(OPERATING_SYSTEM),irix)
   ifneq ($(EXPORT_EVERYTHING),true)
      # The .link extension prevents the makefile predecessor cycle 
      EXPORTS_FILE = cmgui.exports.link
      SRC_EXPORTS_FILE = cmgui.exports
      EXPORTS_DEPEND = $(OBJECT_PATH)/$(EXPORTS_FILE)
      EXPORTS_LINK_FLAGS = -Wl,-exports_file,$(EXPORTS_FILE)
      SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
			@if [ ! -d $(OBJECT_PATH) ]; then \
				mkdir -p $(OBJECT_PATH); \
			fi
			@if [ -f $(OBJECT_PATH)/$(EXPORTS_FILE).c ]; then \
				rm $(OBJECT_PATH)/$(EXPORTS_FILE).c; \
			fi
			cp $(SRC_EXPORTS_FILE) $(OBJECT_PATH)/$(EXPORTS_FILE).c
			cd $(OBJECT_PATH) ; $(CPREPROCESS) $(ALL_DEFINES) $(EXPORTS_FILE).c
			cp $(OBJECT_PATH)/$(EXPORTS_FILE).i $(OBJECT_PATH)/$(EXPORTS_FILE)
   else # $(EXPORT_EVERYTHING) != true
      EXPORTS_FILE = no.exports
      EXPORTS_LINK_FLAGS = 
      SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	
			touch $(OBJECT_PATH)/$(EXPORTS_FILE)
   endif # $(EXPORT_EVERYTHING) != true
endif # SYSNAME == IRIX%=
ifeq ($(OPERATING_SYSTEM),linux)
   ifneq ($(STATIC_LINK),true)
      ifneq ($(EXPORT_EVERYTHING),true)
         LD_VERSION := $(shell ld -v | grep " \([3-9]\.\|2\.[2-9][0-9]\|2\.1[2-9]\)")
         NEW_BINUTILS := $(if $(LD_VERSION),true)
         ifdef NEW_BINUTILS
            # In Linux this requires an ld from binutils 2.12 or later 
            #   but is much better code so hopefully what I do here will
            #   replace the alternative when this version is ubiquitous */
            # Use a version script to reduce all variables not listed to
            #   local scope, this version script would allow regexp but
            #   the irix file does not yet */
            EXPORTS_FILE = cmgui_ld_version.script
            SRC_EXPORTS_FILE = cmgui.exports
            EXPORTS_DEPEND = $(OBJECT_PATH)/$(EXPORTS_FILE)
            EXPORTS_LINK_FLAGS = -Wl,--export-dynamic,--version-script,$(EXPORTS_FILE)
            SO_LIB_EXPORTS_LINK_FLAGS = -Wl,--version-script,$(EXPORTS_FILE)
            $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
					@if [ ! -d $(OBJECT_PATH) ]; then \
						mkdir -p $(OBJECT_PATH); \
					fi
					@if [ -f $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c ]; then \
						rm $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c; \
					fi
					cp -f $(SRC_EXPORTS_FILE) $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c
					$(CPREPROCESS) $(ALL_DEFINES) $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c -o $(OBJECT_PATH)/$(EXPORTS_FILE).E
					echo > $(OBJECT_PATH)/$(EXPORTS_FILE)	"CMISS_EXP_1.0 {"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE)	"global:"
					sed -n -e 's/\([^ ]\+\)/  \1;/p' < $(OBJECT_PATH)/$(EXPORTS_FILE).E >> $(OBJECT_PATH)/$(EXPORTS_FILE)
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE)	"local:"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE)	"  *;"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE)	"};"
         else # NEW_BINUTILS
            # So this is the nastier fix which works until ld is fixed,
            # basically I create a shared object which requires the 
            # symbols we want to export and link against that, then
            # to remove the dependency on the shared object we just
            # linked against, we declare that the executable has the
            # soname of the library, pretty sneaky */
            EXPORTS_FILE = cmgui_symbols.so
            EXPORTS_SONAME = libcmgui.so # If we are going to put this in the executable lets give it a generic name 
            SRC_EXPORTS_FILE = cmgui.exports
            EXPORTS_DEPEND = $(OBJECT_PATH)/$(EXPORTS_FILE)
            EXPORTS_LINK_FLAGS = $(EXPORTS_FILE) -Wl,-soname,$(EXPORTS_SONAME)
            SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
            $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
					@if [ ! -d $(OBJECT_PATH) ]; then \
						mkdir -p $(OBJECT_PATH); \
					fi
					@if [ -f $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c ]; then \
						rm $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c; \
					fi
					cp -f $(SRC_EXPORTS_FILE) $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c
					$(CPREPROCESS) $(ALL_DEFINES) $(OBJECT_PATH)/$(SRC_EXPORTS_FILE).list.c -o $(OBJECT_PATH)/$(EXPORTS_FILE).E
					echo > $(OBJECT_PATH)/$(EXPORTS_FILE).c	"#define lt_preloaded_symbols some_other_symbol"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE).c	"#define dynamic_ptr void *"
					sed -n -e 's/\([^ ]\+\)/extern char \1;/p' < $(OBJECT_PATH)/$(EXPORTS_FILE).E >> $(OBJECT_PATH)/$(EXPORTS_FILE).c
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE).c	"#undef lt_preloaded_symbols"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE).c	"const struct { const char *name; dynamic_ptr address; }"
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE).c	"lt_preloaded_symbols[] = {"
					sed -n -e 's/\([^ ]\+\)/  {\"\1\", (dynamic_ptr) \&\1},/p' < $(OBJECT_PATH)/$(EXPORTS_FILE).E >> $(OBJECT_PATH)/$(EXPORTS_FILE).c
					echo >> $(OBJECT_PATH)/$(EXPORTS_FILE).c	"{0, (dynamic_ptr) 0} };"
					$(LINK) -shared -o $(OBJECT_PATH)/$(EXPORTS_FILE) -Wl,-soname,$(EXPORTS_SONAME) $(OBJECT_PATH)/$(EXPORTS_FILE).c
         endif # NEW_BINUTILS
      else # $(EXPORT_EVERYTHING) != true
         EXPORTS_FILE = cmgui.dummy
         SRC_EXPORTS_FILE = cmgui.exports
         EXPORTS_LINK_FLAGS = -Wl,-E
         SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
         $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
		   	touch $(OBJECT_PATH)/$(EXPORTS_FILE)
      endif # $(EXPORT_EVERYTHING) != true
   else # STATIC_LINK) != true 
      EXPORTS_FILE = no.exports
      EXPORTS_LINK_FLAGS = 
      SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	
			touch $(OBJECT_PATH)/$(EXPORTS_FILE)
   endif # STATIC_LINK) != true
endif # SYSNAME == Linux
ifeq ($(OPERATING_SYSTEM), win32)
   ifeq ($(COMPILER),msvc)
   ifneq ($(EXPORT_EVERYTHING),true)
      define BuildExportsFile
			@if [ ! -d $(OBJECT_PATH) ]; then \
				mkdir -p $(OBJECT_PATH); \
			fi
			@if [ -f $(OBJECT_PATH)/$(2).c ]; then \
				rm $(OBJECT_PATH)/$(2).c; \
			fi
			cp $(1) $(OBJECT_PATH)/$(2).c
			cd $(OBJECT_PATH) ; $(CPREPROCESS) $(ALL_DEFINES) $(3) $(2).c -o $(OBJECT_PATH)/$(2).i
			echo > $(OBJECT_PATH)/$(2)	'EXPORTS'
			cat $(OBJECT_PATH)/$(2).i >> $(OBJECT_PATH)/$(2)
      endef

      # The .link extension prevents the makefile predecessor cycle 
      EXPORTS_FILE = cmgui.exports.def
      SRC_EXPORTS_FILE = cmgui.exports
      EXPORTS_DEPEND = $(OBJECT_PATH)/$(EXPORTS_FILE)
      EXPORTS_LINK_FLAGS = 
      SO_LIB_EXPORTS_LINK_FLAGS = 
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
			$(call BuildExportsFile,$(SRC_EXPORTS_FILE),$(EXPORTS_FILE),)

   else # $(EXPORT_EVERYTHING) != true
      EXPORTS_FILE = no.exports
      EXPORTS_LINK_FLAGS = -Wl,--export-all-symbols
      SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	
			touch $(OBJECT_PATH)/$(EXPORTS_FILE)
   endif # $(EXPORT_EVERYTHING) != true
   else
   ifneq ($(EXPORT_EVERYTHING),true)
      define BuildExportsFile
			@if [ ! -d $(OBJECT_PATH) ]; then \
				mkdir -p $(OBJECT_PATH); \
			fi
			@if [ -f $(OBJECT_PATH)/$(2).c ]; then \
				rm $(OBJECT_PATH)/$(2).c; \
			fi
			cp $(1) $(OBJECT_PATH)/$(2).c
			cd $(OBJECT_PATH) ; $(CPREPROCESS) $(ALL_DEFINES) $(3) $(2).c -o $(OBJECT_PATH)/$(2).i
			echo > $(OBJECT_PATH)/$(2)	'EXPORTS'
			cat $(OBJECT_PATH)/$(2).i >> $(OBJECT_PATH)/$(2)
      endef

      # The .link extension prevents the makefile predecessor cycle 
      EXPORTS_FILE = cmgui.exports.def
      SRC_EXPORTS_FILE = cmgui.exports
      EXPORTS_DEPEND = $(OBJECT_PATH)/$(EXPORTS_FILE)
      EXPORTS_LINK_FLAGS = 
      SO_LIB_EXPORTS_LINK_FLAGS = -Wl,-exports_file,$(EXPORTS_FILE) -Wl,--entry,_DllMainCRTStartup@12
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	$(SRC_EXPORTS_FILE)
			$(call BuildExportsFile,$(SRC_EXPORTS_FILE),$(EXPORTS_FILE),)

   else # $(EXPORT_EVERYTHING) != true
      EXPORTS_FILE = no.exports
      EXPORTS_LINK_FLAGS = -Wl,--export-all-symbols
      SO_LIB_EXPORTS_LINK_FLAGS = $(EXPORTS_LINK_FLAGS)
      $(OBJECT_PATH)/$(EXPORTS_FILE) :	
			touch $(OBJECT_PATH)/$(EXPORTS_FILE)
   endif # $(EXPORT_EVERYTHING) != true
   endif # $(COMPILER) == msvc
endif # OPERATING_SYSTEM == win32

RESOURCE_FILES = 
COMPILED_RESOURCE_FILES = 
ifeq ($(USER_INTERFACE),WIN32_USER_INTERFACE)
   RESOURCE_FILES += command/command_window.rc
   COMPILED_RESOURCE_FILES += $(RESOURCE_FILES:.rc=.res)
endif # $(USER_INTERFACE) == WIN32_USER_INTERFACE
ifeq ($(SYSNAME),win32)
	ifeq ($(USER_INTERFACE), WX_USER_INTERFACE)
      RESOURCE_FILES += icon/wxmsw_icon.rc
      COMPILED_RESOURCE_FILES += $(RESOURCE_FILES:.rc=.res)
	endif # $(USER_INTERFACE) == WX_USER_INTERFACE
endif # $(SYSNAME) == win32

#Make a shorthand for the standard build target, so we can use it with so_lib and static_lib on the same build
bin : $(BIN_TARGET)

$(BIN_TARGET) : $(OBJS) $(UNEMAP_OBJS) $(COMPILED_RESOURCE_FILES) $(MAIN_OBJ) $(EXPORTS_DEPEND)
	$(call BuildNormalTarget,$(BIN_TARGET),$(BIN_PATH),$(OBJS) $(UNEMAP_OBJS) $(MAIN_OBJ),$(ALL_LIB) $(INTERPRETER_LINK_FLAGS) $(EXPORTS_LINK_FLAGS) $(COMPILED_RESOURCE_FILES))

SO_LIB_SUFFIX = .so
ALL_SO_LINK_FLAGS =
ifeq ($(OPERATING_SYSTEM),irix)
   ALL_SO_LINK_FLAGS = -no_unresolved
endif # SYSNAME == IRIX%=
ifeq ($(OPERATING_SYSTEM),linux)
   ALL_SO_LINK_FLAGS = -Wl,--no-undefined 
endif # SYSNAME == Linux

STATIC_LIB_SUFFIX = .a
ifeq ($(COMPILER),msvc)
   STATIC_LIB_SUFFIX = .lib
endif

ifeq ($(SYSNAME),win32)
   SO_LIB_SUFFIX = .dll
   ifeq ($(COMPILER),msvc)
     SO_LIB_IMPORT_LIB_SUFFIX = .lib
   else
     SO_LIB_IMPORT_LIB_SUFFIX = .dll.a
   endif
endif # $(SYSNAME) == win32 else

SO_LIB_GENERAL = cmgui_general
SO_LIB_GENERAL_BASE = lib$(SO_LIB_GENERAL)
SO_LIB_GENERAL_SONAME = lib$(SO_LIB_GENERAL)$(SO_LIB_SUFFIX)
SO_LIB_GENERAL_TARGET = lib$(SO_LIB_GENERAL)$(SO_LIB_SUFFIX)
SO_LIB_GENERAL_EXTRA_ARGS = $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libz.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libbz2.a

LIB_GENERAL_SRCS = \
	api/cmiss_core.c \
	command/parser.c \
	general/any_object.c \
	general/compare.c \
	general/debug.c \
	general/geometry.cpp \
	general/heapsort.c \
	general/matrix_vector.c \
	general/multi_range.c \
	general/myio.c \
	general/mystring.c \
	general/value.c \
	user_interface/message.c

LIB_GENERAL_OBJS = $(addsuffix .$(OBJ_SUFFIX),$(basename $(LIB_GENERAL_SRCS)))
$(SO_LIB_GENERAL_TARGET) : $(LIB_GENERAL_OBJS) cmgui.Makefile
	$(call BuildSharedLibraryTarget,$(SO_LIB_GENERAL_BASE),$(BIN_PATH),$(LIB_GENERAL_OBJS),$(ALL_SO_LINK_FLAGS) $(SO_LIB_GENERAL_EXTRA_ARGS) $(SOLIB_LIB),$(SO_LIB_GENERAL_SONAME))

SO_LIB_CORE_FIELDS = cmgui_core_fields
SO_LIB_CORE_FIELDS_BASE = lib$(SO_LIB_CORE_FIELDS)
SO_LIB_CORE_FIELDS_SONAME = lib$(SO_LIB_CORE_FIELDS)$(SO_LIB_SUFFIX)
SO_LIB_CORE_FIELDS_TARGET = lib$(SO_LIB_CORE_FIELDS)$(SO_LIB_SUFFIX)
SO_LIB_CORE_FIELDS_EXTRA_ARGS = $(XML2_LIB) $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libz.a $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libbz2.a

LIB_CORE_FIELDS_SRCS = \
	api/cmiss_element.c \
	api/cmiss_node.c \
	api/cmiss_time_sequence.c \
	general/io_stream.c \
	general/statistics.c \
	$(FINITE_ELEMENT_CORE_SRCS) \
	$(COMPUTED_FIELD_CORE_SRCS) \
	$(REGION_SRCS)

LIB_CORE_FIELDS_OBJS = $(addsuffix .$(OBJ_SUFFIX),$(basename $(LIB_CORE_FIELDS_SRCS)))

$(SO_LIB_CORE_FIELDS_TARGET) : $(LIB_CORE_FIELDS_OBJS) $(SO_LIB_GENERAL_TARGET) cmgui.Makefile
	$(call BuildSharedLibraryTarget,$(SO_LIB_CORE_FIELDS_BASE),$(BIN_PATH),$(LIB_CORE_FIELDS_OBJS),$(ALL_SO_LINK_FLAGS) $(SO_LIB_CORE_FIELDS_EXTRA_ARGS) $(SOLIB_LIB),$(SO_LIB_CORE_FIELDS_SONAME),$(BIN_PATH)/$(SO_LIB_GENERAL_BASE))

LIB_PASS_THROUGH_SRCS = \
	command/pass_through_interpreter.c

LIB_PASS_THROUGH_OBJS = $(addsuffix .$(OBJ_SUFFIX),$(basename $(LIB_PASS_THROUGH_SRCS)))
PASS_THROUGH_INTERPRETER_LIB_TARGET = libpassthroughinterpreter$(STATIC_LIB_SUFFIX)
$(PASS_THROUGH_INTERPRETER_LIB_TARGET) : $(LIB_PASS_THROUGH_OBJS)
	$(call BuildStaticLibraryTarget,$(PASS_THROUGH_INTERPRETER_LIB_TARGET),$(BIN_PATH),$(LIB_PASS_THROUGH_OBJS) $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libpcre.a)

SO_LIB_PASS_THROUGH = cmgui_pass_through
SO_LIB_PASS_THROUGH_BASE = lib$(SO_LIB_PASS_THROUGH)
SO_LIB_PASS_THROUGH_SONAME = lib$(SO_LIB_PASS_THROUGH)$(SO_LIB_SUFFIX)
SO_LIB_PASS_THROUGH_TARGET = lib$(SO_LIB_PASS_THROUGH)$(SO_LIB_SUFFIX)
SO_LIB_PASS_THROUGH_EXTRA_ARGS =  $(IMAGEMAGICK_PATH)/lib/$(LIB_ARCH_DIR)/libpcre.a

$(SO_LIB_PASS_THROUGH_TARGET) : $(LIB_PASS_THROUGH_OBJS) cmgui.Makefile
	$(call BuildSharedLibraryTarget,$(SO_LIB_PASS_THROUGH_BASE),$(BIN_PATH),$(LIB_PASS_THROUGH_OBJS),$(ALL_SO_LINK_FLAGS) $(SO_LIB_PASS_THROUGH_EXTRA_ARGS) $(SOLIB_LIB),$(SO_LIB_PASS_THROUGH_SONAME))

SO_ALL_LIB = $(GRAPHICS_LIB) $(USER_INTERFACE_LIB) $(HAPTIC_LIB) \
	$(WORMHOLE_LIB) $(IMAGEMAGICK_LIB) \
	$(EXTERNAL_INPUT_LIB) $(HELP_LIB) $(ITK_LIB) \
	$(MOVIE_FILE_LIB) $(XML_LIB) $(MEMORYCHECK_LIB) \
	$(SOLIB_LIB)
SO_ALL_LIB += $(SYSTEM_LIB)

SO_LIB_BASE = lib$(TARGET_EXECUTABLE_BASENAME)
SO_LIB_TARGET = lib$(TARGET_EXECUTABLE_BASENAME)$(SO_LIB_SUFFIX)
SO_LIB_SONAME = lib$(TARGET_EXECUTABLE_BASENAME)$(SO_LIB_SUFFIX)

REMAINING_LIB_SRCS = $(filter-out $(LIB_GENERAL_SRCS) $(LIB_CORE_FIELDS_SRCS), $(SRCS))

REMAINING_LIB_OBJS = $(addsuffix .$(OBJ_SUFFIX),$(basename $(REMAINING_LIB_SRCS)))
# SAB We are not resolving everything here (i.e. $(ALL_SO_LINK_FLAGS)) as we want to bind
# to the appropriate interpreter at runtime.  We could settle on a standard interpreter
# so_name but that would have to be used by the actual external Cmiss::Perl_cmiss
# perl and python modules that are supplying us the connections to their interpreters.
$(SO_LIB_TARGET) : $(REMAINING_LIB_OBJS) $(COMPILED_RESOURCE_FILES) $(OBJECT_PATH)/$(EXPORTS_FILE) $(SO_LIB_CORE_FIELDS_TARGET) $(SO_LIB_GENERAL_TARGET) $(SO_LIB_PASS_THROUGH_TARGET) cmgui.Makefile
	$(call BuildSharedLibraryTarget,$(SO_LIB_BASE),$(BIN_PATH),$(REMAINING_LIB_OBJS),$(SO_LIB_EXPORTS_LINK_FLAGS) $(SO_ALL_LIB) $(COMPILED_RESOURCE_FILES) ,$(SO_LIB_SONAME), $(BIN_PATH)/$(SO_LIB_CORE_FIELDS_BASE) $(BIN_PATH)/$(SO_LIB_GENERAL_BASE) $(BIN_PATH)/$(SO_LIB_PASS_THROUGH_BASE))

#Make so_lib to be a shorthand for making all the so_libs
so_lib : $(SO_LIB_GENERAL_TARGET) $(SO_LIB_CORE_FIELDS_TARGET) $(SO_LIB_TARGET) $(SO_LIB_PASS_THROUGH_TARGET)

STATIC_LIB_TARGET = lib$(TARGET_EXECUTABLE_BASENAME)$(STATIC_LIB_SUFFIX)

$(STATIC_LIB_TARGET) : $(OBJS)
	$(call BuildStaticLibraryTarget,$(STATIC_LIB_TARGET),$(BIN_PATH),$(OBJS))

#Make static_lib to be a shorthand for the fully qualified file
static_lib : $(STATIC_LIB_TARGET) $(PASS_THROUGH_INTERPRETER_LIB_TARGET)

# Force is a dummy rule used to ensure some objects are made every time as the
# target 'force' doesnt exist */
force :
	@echo "\n" > /dev/null

ifeq ($(MEMORYCHECK),true)
   # Specify rule for making a memory checking object with the normal c file 
   # This allows us to make a memory checking version without creating a new extension
   general/debug_memory_check.$(OBJ_SUFFIX): general/debug.c general/debug.h
		if [ ! -d $(OBJECT_PATH)/general ]; then \
			mkdir -p $(OBJECT_PATH)/general; \
		fi
		$(CC) -o $(OBJECT_PATH)/general/debug_memory_check.$(OBJ_SUFFIX) $(ALL_FLAGS) -DMEMORY_CHECKING general/debug.c;

   general/debug_memory_check.d : general/debug.c general/debug.h
		@if [ ! -d $(OBJECT_PATH)/$(*D) ]; then \
			mkdir -p $(OBJECT_PATH)/$(*D); \
		fi
		@set -x ; $(MAKEDEPEND) $(ALL_FLAGS) $(STRICT_FLAGS) $<  | \
		sed -e 's%^.*\.o%$*.$(OBJ_SUFFIX) $(OBJECT_PATH)/$*.d%;s%$(SOURCE_PATH)/%%g;s%$(PRODUCT_SOURCE_PATH)/%%g' > $(OBJECT_PATH)/$*.d ;
endif # $(MEMORYCHECK) == true

utilities : SpacesToTabs TabsToSpaces

SPACES_TO_TABS_SRCS = \
	utilities/spaces_to_tabs.c

SPACES_TO_TABS_OBJSA = $(SPACES_TO_TABS_SRCS:.c=.$(OBJ_SUFFIX))
SPACES_TO_TABS_OBJSB = $(SPACES_TO_TABS_OBJSA:.cpp=.$(OBJ_SUFFIX))
SPACES_TO_TABS_OBJS = $(SPACES_TO_TABS_OBJSB:.f=.$(OBJ_SUFFIX))
BUILD_SPACES_TO_TABS = $(call BuildNormalTarget,SpacesToTabs,$(UTILITIES_PATH),$(SPACES_TO_TABS_OBJS),-lm) 

SpacesToTabs: $(SPACES_TO_TABS_OBJS)
	$(BUILD_SPACES_TO_TABS)

TABS_TO_SPACES_SRCS = \
	utilities/tabs_to_spaces.c

TABS_TO_SPACES_OBJSA = $(TABS_TO_SPACES_SRCS:.c=.$(OBJ_SUFFIX))
TABS_TO_SPACES_OBJSB = $(TABS_TO_SPACES_OBJSA:.cpp=.$(OBJ_SUFFIX))
TABS_TO_SPACES_OBJS = $(TABS_TO_SPACES_OBJSB:.f=.$(OBJ_SUFFIX))
BUILD_TABS_TO_SPACES = $(call BuildNormalTarget,TabsToSpaces,$(UTILITIES_PATH),$(TABS_TO_SPACES_OBJS),-lm) 

TabsToSpaces: $(TABS_TO_SPACES_OBJS)
	$(BUILD_TABS_TO_SPACES)

DEPENDFILE = $(OBJECT_PATH)/$(BIN_TARGET).depend

DEPEND_FILES = $(OBJS:%.$(OBJ_SUFFIX)=%.d) $(UNEMAP_OBJS:%.$(OBJ_SUFFIX)=%.d) $(MAIN_OBJ:%.$(OBJ_SUFFIX)=%.d) $(SPACES_TO_TABS_OBJS:%.$(OBJ_SUFFIX)=%.d) $(TABS_TO_SPACES_OBJS:%.$(OBJ_SUFFIX)=%.d) $(LIB_PASS_THROUGH_OBJS:%.$(OBJ_SUFFIX)=%.d)
#Look in the OBJECT_PATH
DEPEND_FILES_OBJECT_PATH = $(DEPEND_FILES:%.d=$(OBJECT_PATH)/%.d)
DEPEND_FILES_OBJECT_FOUND = $(wildcard $(DEPEND_FILES_OBJECT_PATH))
DEPEND_FILES_OBJECT_NOTFOUND = $(filter-out $(DEPEND_FILES_OBJECT_FOUND),$(DEPEND_FILES_OBJECT_PATH))
DEPEND_FILES_MISSING_PART1 = $(DEPEND_FILES_OBJECT_NOTFOUND:$(OBJECT_PATH)/%.d=%.d)
DEPEND_FILES_MISSING = $(DEPEND_FILES_MISSING_PART1)
DEPEND_FILES_INCLUDE = $(DEPEND_FILES_OBJECT_FOUND) $(DEPEND_FILES_MISSING)

#Touch a dummy include so that this makefile is reloaded and therefore the new .ds
$(DEPENDFILE) : $(DEPEND_FILES_INCLUDE)
	touch $(DEPENDFILE)
#	rm `find $(OBJECT_PATH) -size 0 -name "*.d"`;\

include $(DEPENDFILE)
include $(DEPEND_FILES_INCLUDE)
