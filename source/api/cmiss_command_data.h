/*******************************************************************************
FILE : cmiss_command_data.h

LAST MODIFIED : 5 April 2004

DESCRIPTION :
The public interface to the some of the internal functions of cmiss.
==============================================================================*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef __CMISS_COMMAND_H__
#define __CMISS_COMMAND_H__

#include "api/cmiss_field.h"
#include "api/cmiss_time_keeper.h"
#if defined (WIN32_USER_INTERFACE)
#include <windows.h>
#endif /* defined (WIN32_USER_INTERFACE) */

/*
Global types
------------
*/
struct Cmiss_command_data;
typedef struct Cmiss_command_data *Cmiss_command_data_id;

struct manager_Cmiss_texture;
typedef struct manager_Cmiss_texture *Cmiss_texture_manager_id;

struct Cmiss_scene_viewer_package;
typedef struct Cmiss_scene_viewer_package *Cmiss_scene_viewer_package_id;

/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Shifted the Cmiss_command_data to be internal to cmiss.c
==============================================================================*/

/*
Global functions
----------------
*/

#if !defined (WIN32_USER_INTERFACE)
struct Cmiss_command_data *Cmiss_command_data_create(int argc,char *argv[],
	char *version_string);
#else /* !defined (WIN32_USER_INTERFACE) */
struct Cmiss_command_data *Cmiss_command_data_create(int argc,char *argv[],
	char *version_string, HINSTANCE current_instance, 
        HINSTANCE previous_instance, LPSTR command_line,int initial_main_window_state);
#endif /* !defined (WIN32_USER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Initialise all the subcomponents of cmgui and create the Cmiss_command_data
==============================================================================*/

int Cmiss_command_data_destroy(struct Cmiss_command_data **command_data_address);
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Clean up the command_data, deallocating all the associated memory and resources.
==============================================================================*/

int Cmiss_command_data_execute_command(struct Cmiss_command_data *command_data,
	const char *command);
/*******************************************************************************
LAST MODIFIED : 9 July 2007

DESCRIPTION :
Parses the supplied <command> using the command parser interpreter.
==============================================================================*/

struct Cmiss_region *Cmiss_command_data_get_root_region(
	struct Cmiss_command_data *command_data);
/*******************************************************************************
LAST MODIFIED : 18 April 2003

DESCRIPTION :
Returns the root region from the <command_data>.
==============================================================================*/

int Cmiss_command_data_main_loop(struct Cmiss_command_data *command_data);
/*******************************************************************************
LAST MODIFIED : 19 December 2002

DESCRIPTION :
Process events until some events request the program to finish.
==============================================================================*/

struct Cmiss_scene_viewer *Cmiss_command_data_get_graphics_window_pane_by_name(
	struct Cmiss_command_data *command_data, const char *name, int pane_number);
/*******************************************************************************
LAST MODIFIED : 26 January 2007

DESCRIPTION :
Returns the a handle to the scene_viewer that inhabits the pane of a graphics_window.
==============================================================================*/

/***************************************************************************//**
 * Returns the handle to time keeper and also increments the access count of 
 * the returned time keeper by one.
 *
 * @param command_data  The Cmiss command data object.
 * @return  The time keeper if successfully called otherwise NULL.
 */
Cmiss_time_keeper_id Cmiss_command_data_get_time_keeper(
	Cmiss_command_data_id command_data);

/***************************************************************************//**
 * Returns a handle to the texture manager.
 *
 * @param command_data  The Cmiss command data object.
 * @return  The Texture manager.
 */
Cmiss_texture_manager_id Cmiss_command_data_get_texture_manager(
 Cmiss_command_data_id command_data);

/**
 * Returns a handle to a scene viewer package
 *
 * @param command_data The Cmiss command data object
 * @return The scene viewer package
 */
Cmiss_scene_viewer_package_id Cmiss_command_data_get_scene_viewer_package(
	Cmiss_command_data_id command_data);

#endif /* __CMISS_COMMAND_H__ */
