/*******************************************************************************
FILE : time_editor_dialog.c

LAST MODIFIED : 17 May 2002

DESCRIPTION :
This module creates a time_editor_dialog.
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
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include "general/callback_motif.h"
#include "general/debug.h"
#include "time/time_keeper.h"
#include "time/time_editor.h"
#include "time/time_editor_dialog.h"
static char time_editor_dialog_uidh[] =
#include "time/time_editor_dialog.uidh"
	;
#include "user_interface/message.h"

/*
Module Constants
----------------
*/
/* UIL Identifiers */
#define time_editor_dialog_editor_form_ID 1

/*
Global Types
------------
*/
struct Time_editor_dialog
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Contains all the information carried by the time_editor_dialog widget.
Note that we just hold a pointer to the time_editor_dialog.
==============================================================================*/
{
	struct Time_editor *time_editor;
	/* store the address of pointer, so can set to NULL from callback */
	struct Time_editor_dialog **time_editor_dialog_address;
	Widget editor_form;
	Widget dialog,widget,dialog_parent;
	struct User_interface *user_interface;
}; /* struct Time_editor_dialog */

/*
Module variables
----------------
*/
#if defined (MOTIF_USER_INTERFACE)
static int time_editor_dialog_hierarchy_open=0;
static MrmHierarchy time_editor_dialog_hierarchy;
#endif /* defined (MOTIF_USER_INTERFACE) */

/*
Module functions
----------------
*/
static void time_editor_dialog_identify_widget(Widget widget,int widget_num,
	unsigned long *reason)
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
Finds the id of the widgets on the time_editor_dialog widget.
==============================================================================*/
{
	struct Time_editor_dialog *temp_time_editor_dialog;

	ENTER(time_editor_dialog_identify_widget);
	USE_PARAMETER(reason);
	/* find out which time_editor_dialog widget we are in */
	XtVaGetValues(widget,XmNuserData,&temp_time_editor_dialog,NULL);
	switch (widget_num)
	{
		case time_editor_dialog_editor_form_ID:
		{
			temp_time_editor_dialog->editor_form=widget;
		} break;
		default:
		{
			display_message(WARNING_MESSAGE,
				"time_editor_dialog_identify_widget.  Invalid widget number");
		} break;
	}
	LEAVE;
} /* time_editor_dialog_identify_widget */

static void time_editor_dialog_close_CB(Widget caller,
	void *time_editor_dialog_void,void *cbs)
/*******************************************************************************
LAST MODIFIED : 13 December 2001

DESCRIPTION :
Callback for the time_editor_dialog dialog
==============================================================================*/
{
	struct Time_editor_dialog *time_editor_dialog;

	ENTER(time_editor_dialog_close_CB);
	USE_PARAMETER(caller);
	USE_PARAMETER(cbs);
	if (time_editor_dialog=(struct Time_editor_dialog *)time_editor_dialog_void)
	{
		/* Must use this address so that the pointer is cleared from the 
			Cmiss data when bring_up_time_editor_dialog is called again. */
		DESTROY(Time_editor_dialog)(time_editor_dialog->time_editor_dialog_address);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_close_CB.  Missing time_editor_dialog");
	}
	LEAVE;
} /* time_editor_dialog_close_CB */

int time_editor_dialog_create(
	struct Time_editor_dialog **time_editor_dialog_address,Widget parent,
	struct Time_keeper *time_keeper,struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Creates a dialog widget that allows the user to edit time.
==============================================================================*/
{
	Atom WM_DELETE_WINDOW;
	int init_widgets,return_code;
	MrmType time_editor_dialog_dialog_class;
	struct Callback_data close_callback;
	struct Time_editor_dialog *time_editor_dialog=NULL;
	static MrmRegisterArg callback_list[]=
	{
		{"time_editor_d_identify_widget",
			(XtPointer)time_editor_dialog_identify_widget}
	};
	static MrmRegisterArg identifier_list[]=
	{
		{"time_editor_d_structure",(XtPointer)NULL},
		{"time_editor_d_editor_form_ID",
		(XtPointer)time_editor_dialog_editor_form_ID}
	};

	ENTER(time_editor_dialog_create);
	return_code=0;
	/* check arguments */
	if (parent&&user_interface)
	{
		if (MrmOpenHierarchy_binary_string(time_editor_dialog_uidh,sizeof(time_editor_dialog_uidh),
			&time_editor_dialog_hierarchy,&time_editor_dialog_hierarchy_open))
		{
			/* allocate memory */
			if (ALLOCATE(time_editor_dialog,struct Time_editor_dialog,1))
			{
				/* initialise the structure */
				time_editor_dialog->time_editor_dialog_address=
					time_editor_dialog_address;
				time_editor_dialog->dialog_parent=parent;
				time_editor_dialog->widget=(Widget)NULL;
				time_editor_dialog->dialog=(Widget)NULL;
				time_editor_dialog->user_interface=user_interface;
				/* make the dialog shell */
				if (time_editor_dialog->dialog=XtVaCreatePopupShell(
					"Time Editor",topLevelShellWidgetClass,parent,XmNallowShellResize,
					TRUE,NULL))
				{
					/* register the callbacks */
					if (MrmSUCCESS==MrmRegisterNamesInHierarchy(
						time_editor_dialog_hierarchy,callback_list,
						XtNumber(callback_list)))
					{
						/* assign and register the identifiers */
						identifier_list[0].value=(XtPointer)time_editor_dialog;
						if (MrmSUCCESS==MrmRegisterNamesInHierarchy(
							time_editor_dialog_hierarchy,identifier_list,
							XtNumber(identifier_list)))
						{
							/* fetch position window widget */
							if (MrmSUCCESS==MrmFetchWidget(time_editor_dialog_hierarchy,
								"time_editor_dialog_widget",time_editor_dialog->dialog,
								&(time_editor_dialog->widget),
								&time_editor_dialog_dialog_class))
							{
								XtManageChild(time_editor_dialog->widget);
								/* set the mode toggle to the correct position */
								init_widgets=1;
								if (!CREATE(Time_editor)(&(time_editor_dialog->time_editor),
									time_editor_dialog->editor_form,time_keeper,user_interface))
								{
									display_message(ERROR_MESSAGE,"time_editor_dialog_create.  "
										"Could not create editor widget.");
									init_widgets=0;
								}
								if (init_widgets)
								{
									close_callback.procedure = time_editor_dialog_close_CB;
									close_callback.data = (void *)time_editor_dialog;
									time_editor_set_close_callback(time_editor_dialog->time_editor,
										&close_callback);

									XtRealizeWidget(time_editor_dialog->dialog);
									XtPopup(time_editor_dialog->dialog,XtGrabNone);
									create_Shell_list_item(&time_editor_dialog->dialog,
										user_interface);
									/* Set up window manager callback for close window message */
									WM_DELETE_WINDOW=XmInternAtom(
										XtDisplay(time_editor_dialog->dialog),"WM_DELETE_WINDOW",
										False);
									XmAddWMProtocolCallback(time_editor_dialog->dialog,
										WM_DELETE_WINDOW,time_editor_dialog_close_CB,
										time_editor_dialog);
									if (time_editor_dialog_address)
									{
										*time_editor_dialog_address=time_editor_dialog;
									}
									return_code=1;
								}
								else
								{
									DEALLOCATE(time_editor_dialog);
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,"time_editor_dialog_create.  "
									"Could not fetch time_editor_dialog");
								DEALLOCATE(time_editor_dialog);
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"time_editor_dialog_create.  "
								"Could not register identifiers");
							DEALLOCATE(time_editor_dialog);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"time_editor_dialog_create.  Could not register callbacks");
						DEALLOCATE(time_editor_dialog);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"time_editor_dialog_create.  Could not create popup shell");
					DEALLOCATE(time_editor_dialog);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"time_editor_dialog_create.  "
					"Could not allocate time_editor_dialog");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"time_editor_dialog_create.  Could not open hierarchy");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_create.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_create */

/*
Global functions
----------------
*/
int time_editor_dialog_get_time_keeper(
	struct Time_editor_dialog *time_editor_dialog,
	struct Time_keeper **time_keeper_address)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Get the <*time_keeper_address> for the <time_editor_dialog>.
==============================================================================*/
{
	int return_code;

	ENTER(time_editor_dialog_get_time_keeper);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog&&time_keeper_address)
	{
		return_code=time_editor_get_time_keeper(time_editor_dialog->time_editor,
			time_keeper_address);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_get_time_keeper.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_get_time_keeper */

int time_editor_dialog_set_time_keeper(
	struct Time_editor_dialog *time_editor_dialog,
	struct Time_keeper *time_keeper)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Set the <time_keeper> for the <time_editor_dialog>.
==============================================================================*/
{
	int return_code;

	ENTER(time_editor_dialog_set_time);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog)
	{
		return_code=time_editor_set_time_keeper(time_editor_dialog->time_editor,
			time_keeper);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_set_time_keeper.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_set_time_keeper */

int time_editor_dialog_get_step(struct Time_editor_dialog *time_editor_dialog,
	float *step)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Get the <*step> for the <time_editor_dialog>.
==============================================================================*/
{
	int return_code;

	ENTER(time_editor_dialog_get_step);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog&&step)
	{
		return_code=time_editor_get_step(time_editor_dialog->time_editor,step);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_get_step.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_get_step */

int time_editor_dialog_set_step(struct Time_editor_dialog *time_editor_dialog,
	float step)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Set the <step> for the <time_editor_dialog>.
==============================================================================*/
{
	int return_code;

	ENTER(time_editor_dialog_set_step);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog)
	{
		return_code=time_editor_set_step(time_editor_dialog->time_editor,step);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_set_step.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_set_step */

int bring_up_time_editor_dialog(
	struct Time_editor_dialog **time_editor_dialog_address,
	Widget parent,struct Time_keeper *time_keeper,
	struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
If there is a <*time_editor_dialog_address> in existence, then bring it to the 
front, otherwise create a new one.
==============================================================================*/
{
	int return_code;
	struct Time_editor_dialog *time_editor_dialog;

	ENTER(bring_up_time_editor_dialog);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog_address&&time_keeper&&user_interface)
	{
		if (time_editor_dialog= *time_editor_dialog_address)
		{
			time_editor_dialog_set_time_keeper(time_editor_dialog,time_keeper);
			XtPopup(time_editor_dialog->dialog,XtGrabNone);
			return_code=1;
		}
		else
		{
			if (time_editor_dialog_create(time_editor_dialog_address,parent,
				time_keeper,user_interface))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"bring_up_time_editor_dialog.  Error creating dialog");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"bring_up_time_editor_dialog.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* bring_up_time_editor_dialog */

int DESTROY(Time_editor_dialog)(
	struct Time_editor_dialog **time_editor_dialog_address)
/*******************************************************************************
LAST MODIFIED : 17 May 2002

DESCRIPTION :
Destroy the <*time_editor_dialog_address> and sets <*time_editor_dialog_address>
to NULL.
==============================================================================*/
{
	int return_code;
	struct Time_editor_dialog *time_editor_dialog;

	ENTER(time_editor_dialog_destroy);
	return_code=0;
	/* check arguments */
	if (time_editor_dialog_address)
	{
		return_code=1;
		if (time_editor_dialog= *time_editor_dialog_address)
		{
			destroy_Shell_list_item_from_shell(&time_editor_dialog->dialog,
				time_editor_dialog->user_interface);
			DESTROY(Time_editor)(&time_editor_dialog->time_editor);
			/* deaccess the time_editor_dialog */
			XtDestroyWidget(time_editor_dialog->dialog);
			time_editor_dialog->dialog=(Widget)NULL;
			/* set the pointer to the time_editor_dialog to NULL */
			if (time_editor_dialog->time_editor_dialog_address)
			{
				*(time_editor_dialog->time_editor_dialog_address)=
					(struct Time_editor_dialog *)NULL;
			}
			/* deallocate the memory for the user data */
			DEALLOCATE(time_editor_dialog);
			*time_editor_dialog_address=(struct Time_editor_dialog *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"time_editor_dialog_destroy. Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* time_editor_dialog_destroy */
