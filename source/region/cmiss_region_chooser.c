/*******************************************************************************
FILE : cmiss_region_chooser.c

LAST MODIFIED : 28 March 2003

DESCRIPTION :
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
#include <stdio.h>
#if defined (MOTIF_USER_INTERFACE)
#include <Xm/Xm.h>
#include <Xm/TextF.h>
#endif /* defined (MOTIF_USER_INTERFACE) */
#include "general/debug.h"
#include "general/mystring.h"
#include "region/cmiss_region.h"
#include "region/cmiss_region_chooser.h"
#include "user_interface/message.h"

/*
Module variables
----------------
*/

struct Cmiss_region_chooser
/*******************************************************************************
LAST MODIFIED : 9 January 2003

DESCRIPTION :
Contains information required by the chooser control dialog.
==============================================================================*/
{
	int number_of_regions, max_number_of_regions;
	/* regions contains list of regions in path starting from root_region */
	struct Cmiss_region *last_updated_region, **regions;
	Widget parent, widget;
	struct Callback_data update_callback;
}; /* struct Cmiss_region_chooser */

/*
Module functions
----------------
*/

static int Cmiss_region_chooser_update(
	struct Cmiss_region_chooser *chooser)
/*******************************************************************************
LAST MODIFIED : 9 January 2003

DESCRIPTION :
Tells CMGUI about the current values. Sends a pointer to the current object.
Avoids sending repeated updates if the object address has not changed.
==============================================================================*/
{
	int return_code;
	struct Cmiss_region *current_region;

	ENTER(Cmiss_region_chooser_update);
	if (chooser)
	{
		current_region = chooser->regions[chooser->number_of_regions - 1];
		if (current_region != chooser->last_updated_region)
		{
			if (chooser->update_callback.procedure)
			{
				/* now call the procedure with the user data and the position data */
				(chooser->update_callback.procedure)(
					chooser->widget,chooser->update_callback.data,
					current_region);
			}
			chooser->last_updated_region = current_region;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_update.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_update */

static void Cmiss_region_chooser_destroy_callback(Widget widget,
	XtPointer client_data, XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 8 January 2003

DESCRIPTION :
Callback for the chooser dialog - tidies up all memory allocation.
==============================================================================*/
{
	struct Cmiss_region_chooser *chooser;

	ENTER(Cmiss_region_chooser_destroy_callback);
	USE_PARAMETER(call_data);
	if (widget && (chooser = (struct Cmiss_region_chooser *)client_data))
	{
		DESTROY(Cmiss_region_chooser)(&chooser);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_destroy_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* Cmiss_region_chooser_destroy_callback */

static int Cmiss_region_chooser_update_path(
	struct Cmiss_region_chooser *chooser)
/*******************************************************************************
LAST MODIFIED : 13 January 2003

DESCRIPTION :
Writes the <path> into the text widget in the <chooser>.
==============================================================================*/
{
	char *path;
	int return_code;

	ENTER(Cmiss_region_chooser_update_path);
	if (chooser)
	{
		if (Cmiss_region_chooser_get_path(chooser, &path))
		{
			XmTextFieldSetString(chooser->widget, path);
			DEALLOCATE(path);
			return_code = 1;
		}
		else
		{
			XmTextFieldSetString(chooser->widget, "ERROR");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_update_path.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_update_path */

static void Cmiss_region_chooser_callback(Widget widget,
	XtPointer client_data, XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 8 January 2003

DESCRIPTION :
Callback for the text field - region path entered.
==============================================================================*/
{
	char *path;
	struct Cmiss_region_chooser *chooser;

	ENTER(Cmiss_region_chooser_callback);
	USE_PARAMETER(call_data);
	if (widget && (chooser = (struct Cmiss_region_chooser *)client_data))
	{
		/* Get the object name string */
		if (path = XmTextFieldGetString(widget))
		{
			Cmiss_region_chooser_set_path(chooser, path);
			XtFree(path);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* Cmiss_region_chooser_callback */

/***************************************************************************//**
 * Callback from current or ancestor children indicating children or name
 * changed.
 */
static void Cmiss_region_chooser_Cmiss_region_change(
	struct Cmiss_region *region, struct Cmiss_region_changes *changes,
	void *chooser_void)
{
	char *new_path;
	int i, region_number;
	struct Cmiss_region_chooser *chooser;

	ENTER(Cmiss_region_chooser_Cmiss_region_change);
	if (region && changes &&
		(chooser = (struct Cmiss_region_chooser *)chooser_void))
	{
		region_number = -1;
		for (i = 0; (i < chooser->number_of_regions) && (region_number < 0); i++)
		{
			if (region == chooser->regions[i])
			{
				region_number = i;
			}
		}
		if (0 <= region_number)
		{
			if (changes->children_changed &&
				(!Cmiss_region_contains_subregion(region,
					chooser->regions[chooser->number_of_regions - 1])))
			{
				new_path = Cmiss_region_get_path(region);
				Cmiss_region_chooser_set_path(chooser, new_path);
				DEALLOCATE(new_path);
			}
			else if (changes->name_changed)
			{
				Cmiss_region_chooser_update_path(chooser);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_Cmiss_region_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Cmiss_region_chooser_Cmiss_region_change */

/*
Global functions
----------------
*/

struct Cmiss_region_chooser *CREATE(Cmiss_region_chooser)(Widget parent,
	struct Cmiss_region *root_region, char *initial_path)
/*******************************************************************************
LAST MODIFIED : 28 March 2003

DESCRIPTION :
Creates a dialog from which a region may be chosen.
<parent> must be an XmForm.
==============================================================================*/
{
	Arg args[6];
	struct Cmiss_region_chooser *chooser;

	ENTER(CREATE(Cmiss_region_chooser));
	chooser = (struct Cmiss_region_chooser *)NULL;
	if (parent && root_region && initial_path)
	{
		if (ALLOCATE(chooser, struct Cmiss_region_chooser, 1) &&
			ALLOCATE(chooser->regions, struct Cmiss_region *, 1))
		{
			/* initialise the structure */
			chooser->number_of_regions = 1;
			chooser->max_number_of_regions = 1;
			chooser->regions[0] = ACCESS(Cmiss_region)(root_region);
			chooser->last_updated_region = (struct Cmiss_region *)NULL;
			chooser->parent = parent;
			chooser->widget = (Widget)NULL;
			chooser->update_callback.procedure = (Callback_procedure *)NULL;
			chooser->update_callback.data = (void *)NULL;

			XtSetArg(args[0], XmNleftAttachment, XmATTACH_FORM);
			XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
			XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
			XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
			XtSetArg(args[4], XmNuserData, (XtPointer)chooser);
			XtSetArg(args[5], XmNcolumns, (XtPointer)20);
			if (chooser->widget = XmCreateTextField(parent, "chooser", args, 6))
			{
				/* add callbacks for chooser widget */
				XtAddCallback(chooser->widget, XmNdestroyCallback,
					Cmiss_region_chooser_destroy_callback, (XtPointer)chooser);
				XtAddCallback(chooser->widget, XmNlosingFocusCallback,
					Cmiss_region_chooser_callback, (XtPointer)chooser);
				XtAddCallback(chooser->widget, XmNactivateCallback,
					Cmiss_region_chooser_callback, (XtPointer)chooser);
				Cmiss_region_add_callback(chooser->regions[0],
					Cmiss_region_chooser_Cmiss_region_change, (void *)chooser);
				Cmiss_region_chooser_set_path(chooser, initial_path);
				XtManageChild(chooser->widget);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CREATE(Cmiss_region_chooser).  Could not create text field widget");
				DEALLOCATE(chooser);
				chooser = (struct Cmiss_region_chooser *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Cmiss_region_chooser).  Could not allocate structure");
			DEALLOCATE(chooser);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Cmiss_region_chooser).  Invalid argument(s)");
	}
	LEAVE;

	return (chooser);
} /* CREATE(Cmiss_region_chooser) */

int DESTROY(Cmiss_region_chooser)(
	struct Cmiss_region_chooser **chooser_address)
/*******************************************************************************
LAST MODIFIED : 9 January 2003

DESCRIPTION :
==============================================================================*/
{
	int i, return_code;
	struct Cmiss_region_chooser *chooser;

	ENTER(DESTROY(Cmiss_region_chooser));
	if (chooser_address && (chooser = *chooser_address))
	{
		for (i = 0; i < chooser->number_of_regions; i++)
		{
			Cmiss_region_remove_callback(chooser->regions[i],
				Cmiss_region_chooser_Cmiss_region_change, (void *)chooser);
			DEACCESS(Cmiss_region)(&(chooser->regions[i]));
		}
		DEALLOCATE(chooser->regions);
		DEALLOCATE(*chooser_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Cmiss_region_chooser).  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Cmiss_region_chooser) */

int Cmiss_region_chooser_set_callback(struct Cmiss_region_chooser *chooser,
	Callback_procedure *procedure, void *data)
/*******************************************************************************
LAST MODIFIED : 8 January 2003

DESCRIPTION :
Sets the callback <procedure> and user <data> in the <chooser>.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_region_chooser_set_callback);
	if (chooser)
	{
		chooser->update_callback.procedure = procedure;
		chooser->update_callback.data = data;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_set_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_set_callback */

int Cmiss_region_chooser_get_path(struct Cmiss_region_chooser *chooser,
	char **path_address)
/*******************************************************************************
LAST MODIFIED : 13 January 2003

DESCRIPTION :
Gets <path> of chosen region in the <chooser>.
Up to the calling function to DEALLOCATE the returned path.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_region_chooser_get_path);
	if (chooser && path_address)
	{
		*path_address =
			Cmiss_region_get_path(chooser->regions[chooser->number_of_regions - 1]);
		return_code = (*path_address != NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_get_path.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_get_path */

int Cmiss_region_chooser_get_region(struct Cmiss_region_chooser *chooser,
	struct Cmiss_region **region_address)
/*******************************************************************************
LAST MODIFIED : 13 January 2003

DESCRIPTION :
Returns to <region_address> the region chosen in the <chooser>.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_region_chooser_get_region);
	if (chooser && region_address)
	{
		*region_address = chooser->regions[chooser->number_of_regions - 1];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_get_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_get_region */

int Cmiss_region_chooser_set_path(struct Cmiss_region_chooser *chooser,
	char *path)
/*******************************************************************************
LAST MODIFIED : 28 March 2003

DESCRIPTION :
Sets <path> of chosen region in the <chooser>.
==============================================================================*/
{
	char *temp1, *temp2;
	int i, return_code;
	struct Cmiss_region *parent, *region, **regions;

	ENTER(Cmiss_region_chooser_set_path);
	if (chooser && path)
	{
		return_code = 1;
		/* remove callbacks from and deaccess non-root regions in path */
		for (i = 1; i < chooser->number_of_regions; i++)
		{
			Cmiss_region_remove_callback(chooser->regions[i],
				Cmiss_region_chooser_Cmiss_region_change, (void *)chooser);
			DEACCESS(Cmiss_region)(&(chooser->regions[i]));
		}
		chooser->number_of_regions = 1;
		region = chooser->regions[0];
		Cmiss_region_get_partial_region_path(chooser->regions[0], path,
			&region, &temp1, &temp2);
		DEALLOCATE(temp1);
		DEALLOCATE(temp2);
		while (parent = Cmiss_region_get_parent(region))
		{
			if (chooser->number_of_regions == chooser->max_number_of_regions)
			{
				if (REALLOCATE(regions, chooser->regions, struct Cmiss_region *,
					chooser->max_number_of_regions + 1))
				{
					chooser->regions = regions;
					chooser->max_number_of_regions++;
				}
				else
				{
					return_code = 0;
					break;
				}
			}
			for (i = chooser->number_of_regions; i > 1; i--)
			{
				chooser->regions[i] = chooser->regions[i - 1];
			}
			chooser->regions[1] = ACCESS(Cmiss_region)(region);
			Cmiss_region_add_callback(region,
				Cmiss_region_chooser_Cmiss_region_change, (void *)chooser);
			chooser->number_of_regions++;
			region = parent;
			Cmiss_region_destroy(&parent);
		}
		Cmiss_region_chooser_update_path(chooser);
		Cmiss_region_chooser_update(chooser);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_chooser_set_path.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_chooser_set_path */
