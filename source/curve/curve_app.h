/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */





int gfx_define_Curve(struct Parse_state *state,
	void *dummy_to_be_modified,void *curve_command_data_void);
/*******************************************************************************
LAST MODIFIED : 3 September 2004

DESCRIPTION :
==============================================================================*/

int gfx_destroy_Curve(struct Parse_state *state,
	void *dummy_to_be_modified,void *Curve_manager_void);
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
Executes a GFX DESTROY CURVE command.
==============================================================================*/

int gfx_list_Curve(struct Parse_state *state,
	void *dummy_to_be_modified,void *Curve_manager_void);
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
Executes a GFX SHOW CURVE.
==============================================================================*/


/*==============================================================================*/

int set_Curve(struct Parse_state *state,void *curve_address_void,
	void *curve_manager_void);
/*******************************************************************************
LAST MODIFIED : 5 November 1999

DESCRIPTION :
Modifier function to set the curve from a command.
*/
