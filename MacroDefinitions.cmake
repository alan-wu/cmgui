MACRO( GET_AVAILABLE_USER_INTERFACES )
	# Get a list of available user interfaces
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
		SET( AVAILABLE_USER_INTERFACES motif console gtk wx )
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
		SET( AVAILABLE_USER_INTERFACES console gtk wx )
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		SET( AVAILABLE_USER_INTERFACES carbon )
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
ENDMACRO( GET_AVAILABLE_USER_INTERFACES )

MACRO( GET_COMPILER_NAME )
	# Get the compiler name
	IF( MINGW )
		SET( COMPILER_NAME "mingw" )
	ENDIF( MINGW )
	IF( MSYS )
		SET( COMPILER_NAME "msys" )
	ENDIF( MSYS )
	IF( BORLAND )
		SET( COMPILER_NAME "borland" )
	ENDIF( BORLAND )
	IF( WATCOM )
		SET( COMPILER_NAME "watcom" )
	ENDIF( WATCOM )
	IF( MSVC OR MSVC_IDE OR MSVC60 OR MSVC70 OR MSVC71 OR MSVC80 OR CMAKE_COMPILER_2005 OR MSVC90 )
		SET( COMPILER_NAME "msvc" )
	ENDIF( MSVC OR MSVC_IDE OR MSVC60 OR MSVC70 OR MSVC71 OR MSVC80 OR CMAKE_COMPILER_2005 OR MSVC90 )
	IF( CMAKE_COMPILER_IS_GNUCC )
		SET( COMPILER_NAME "gcc" )
	ENDIF( CMAKE_COMPILER_IS_GNUCC )
	IF( CMAKE_COMPILER_IS_GNUCXX )
		SET( COMPILER_NAME "g++" )
	ENDIF( CMAKE_COMPILER_IS_GNUCXX )
	IF( CYGWIN )
		SET( COMPILER_NAME "cygwin" )
	ENDIF( CYGWIN )

ENDMACRO( GET_COMPILER_NAME )

MACRO( CHECK_USER_INTERFACE_AVAILABILITY UI )
	# Check UI against all UI possible for current platform
	SET( UI_AVAILABLE FALSE )
	GET_AVAILABLE_USER_INTERFACES( )

	LIST( FIND AVAILABLE_USER_INTERFACES ${UI} UI_INDEX )
	IF( UI_INDEX GREATER 0 )
		SET( UI_AVAILABLE TRUE )
	ENDIF( UI_INDEX GREATER 0 )
ENDMACRO( CHECK_USER_INTERFACE_AVAILABILITY UI )

MACRO( DEFINE_TARGET_NAME )
	# Define a basic or verbose target name
	IF( ${TARGET_NAME_FORMAT} MATCHES "[Bb]asic" )
		SET( TARGET_NAME "cmgui" )
	ELSE( ${TARGET_NAME_FORMAT} MATCHES "[Bb]asic" )
		GET_COMPILER_NAME( )
		SET( TARGET_NAME "cmgui" )
		IF( BUILD_ABI )
			SET( TARGET_NAME ${TARGET_NAME}-64 )
		ENDIF( BUILD_ABI )
		IF( USER_INTERFACE )
			SET( TARGET_NAME ${TARGET_NAME}-${USER_INTERFACE} )
		ENDIF( USER_INTERFACE )
		IF( COMPILER_NAME )
			SET( TARGET_NAME ${TARGET_NAME}-${COMPILER_NAME} )
		ENDIF( COMPILER_NAME )
		IF( USE_UNEMAP )
			SET( TARGET_NAME ${TARGET_NAME}-unemap )
		ENDIF( USE_UNEMAP )
		IF( NOT BUILD_EXECUTABLE )
			IF( BUILD_STATIC )
				SET( TARGET_NAME ${TARGET_NAME}-static )
			ELSE( BUILD_STATIC )
				SET( TARGET_NAME ${TARGET_NAME}-dynamic )
			ENDIF( BUILD_STATIC )
		ENDIF( NOT BUILD_EXECUTABLE )
		IF( ${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug )
			SET( TARGET_NAME ${TARGET_NAME}-debug )
		ENDIF( ${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug )
		IF( ${CMAKE_BUILD_TYPE} MATCHES [Rr]elease )
			SET( TARGET_NAME ${TARGET_NAME}-optimised )
		ENDIF( ${CMAKE_BUILD_TYPE} MATCHES [Rr]elease )
		IF( BUILD_MEMORYCHECH )
			SET( TARGET_NAME ${TARGET_NAME}-memorycheck )
		ENDIF( BUILD_MEMORYCHECH )
	ENDIF( ${TARGET_NAME_FORMAT} MATCHES "[Bb]asic" )
ENDMACRO( DEFINE_TARGET_NAME )

MACRO( DEFINE_ARCHITECTURE_DIR )
	STRING( TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM ) 
	SET( ARCHITECTURE_DIR ${CMAKE_SYSTEM_PROCESSOR}-${OPERATING_SYSTEM} )
	MESSAGE( STATUS "Architecture dir: ${ARCHITECTURE_DIR}" )
ENDMACRO( DEFINE_ARCHITECTURE_DIR )

MACRO( SET_PLATFORM_DEFINES )
	# Define platform defines:
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
		SET( GENERIC_PC TRUE )
		SET( CMGUI TRUE )
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
		SET( GENERIC_PC TRUE )
		SET( CMGUI TRUE )
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		SET( CMGUI TRUE)
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
	IF( ${CMAKE_SYSTEM_NAME} MATCHES "Irix" )
		SET( CMGUI TRUE)
		SET( SGI TRUE)
	ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Irix" )
ENDMACRO( SET_PLATFORM_DEFINES )

MACRO( SET_OPERATING_SYSTEM_DEFINES )
	# Define operating system defines:
ENDMACRO( SET_OPERATING_SYSTEM_DEFINES )

MACRO( SET_USER_INTERFACE_DEFINES )
	# Define user interface defines
	IF( ${USER_INTERFACE} MATCHES "motif" )
		SET( MOTIF_USER_INTERFACE TRUE )
	ENDIF( ${USER_INTERFACE} MATCHES "motif" )
	IF( ${USER_INTERFACE} MATCHES "win32" )
		SET( WIN32_USER_INTERFACE TRUE )
	ENDIF( ${USER_INTERFACE} MATCHES "win32" )
	IF( ${USER_INTERFACE} MATCHES "gtk" )
		IF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
			SET( GTK_USER_INTERFACE TRUE )
			SET( USE_GTK_MAIN_STEP )
		ELSE( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
			IF( USE_GTKMAIN )
				SET( GTK_USER_INTERFACE TRUE )
				SET( USE_GTK_MAIN_STEP TRUE )
			ELSE( USE_GTKMAIN )
				SET( GTK_USER_INTERFACE TRUE )
			ENDIF( USE_GTKMAIN )
		ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	ENDIF( ${USER_INTERFACE} MATCHES "gtk" )
	IF( ${USER_INTERFACE} MATCHES "wx" )
		SET( WX_USER_INTERFACE TRUE )
	ENDIF( ${USER_INTERFACE} MATCHES "wx" )
	IF( ${USER_INTERFACE} MATCHES "console" )
		SET( CONSOLE_USER_INTERFACE TRUE )
	ENDIF( ${USER_INTERFACE} MATCHES "console" )
	IF( ${USER_INTERFACE} MATCHES "carbon" )
		SET( CARBON_USER_INTERFACE TRUE )
		SET( TARGET_API_MAC_CARBON TRUE )
	ENDIF( ${USER_INTERFACE} MATCHES "carbon" )
ENDMACRO( SET_USER_INTERFACE_DEFINES )

MACRO( SET_GRAPHICS_DEFINES )
	# Define graphics defines
	IF( ${GRAPHICS_API} MATCHES "OPENGL_GRAPHICS" )
		SET( OPENGL_API TRUE )
		SET( SELECT_DESCRIPTORS TRUE )
		IF( ${USER_INTERFACE} MATCHES "motif" )
			SET( DM_BUFFERS TRUE )
		ENDIF( ${USER_INTERFACE} MATCHES "motif" )
	ENDIF( ${GRAPHICS_API} MATCHES "OPENGL_GRAPHICS" )
ENDMACRO( SET_GRAPHICS_DEFINES )
