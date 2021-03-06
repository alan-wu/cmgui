/*******************************************************************************
FILE : command.h

LAST MODIFIED : 15 July 2002

DESCRIPTION :
Functions and types associated with commands.
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMMAND_H)
#define COMMAND_H

#include "general/object.h"
#include "general/io_stream.h"

/*
Global constants
----------------
*/
/*???DB.  Shouldn't be here ? */
/*#define CMGUI_EXAMPLE_DIRECTORY_SYMBOL "doc"*/
#define CMGUI_EXAMPLE_DIRECTORY_SYMBOL "example"

/*
Global types
------------
*/
typedef int (Execute_command_function)(const char *command,void *user_data);

struct Execute_command;

/*
Global functions
----------------
*/
int read_iod_file_via_selection_box(char *file_name,void *dummy);
/*******************************************************************************
LAST MODIFIED : 15 June 1993

DESCRIPTION :
==============================================================================*/

struct Execute_command *CREATE(Execute_command)(void);
/*******************************************************************************
LAST MODIFIED : 15 July 2002

DESCRIPTION :
Creates a blank execute command. Must call Execute_command_set_command_function
to set the function it calls, and the user data to be passed with it.
==============================================================================*/

int DESTROY(Execute_command)(struct Execute_command **execute_command_address);
/*******************************************************************************
LAST MODIFIED : 8 December 1999

DESCRIPTION :
==============================================================================*/

int Execute_command_set_command_function(
	struct Execute_command *execute_command,
	Execute_command_function *execute_command_function,
	void *command_function_data);
/*******************************************************************************
LAST MODIFIED : 15 July 2002

DESCRIPTION :
Sets the function called by <execute_command>, and the user data to be passed
with it.
==============================================================================*/

int Execute_command_execute_string(struct Execute_command *execute_command,
	const char *string);
/*******************************************************************************
LAST MODIFIED : 8 December 1999

DESCRIPTION :
Executes the given string using the Execute_command stucture
==============================================================================*/

int execute_comfile(char *file_name,struct IO_stream_package *io_stream_package,
	struct Execute_command *execute_command);
/******************************************************************************
LAST MODIFIED : 3 September 2004

DESCRIPTION :
Opens, executes and then closes a com file.  No window is created.
=============================================================================*/
#endif /* !defined (COMMAND_H) */
