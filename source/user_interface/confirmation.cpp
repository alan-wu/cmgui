/*******************************************************************************
FILE : confirmation.c

LAST MODIFIED : 17 December 2002

DESCRIPTION :
Routines for waiting for user input.
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
#if defined (BUILD_WITH_CMAKE)
#include "configure/cmgui_configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

extern "C" {
#if defined (UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif /* defined (UNIX) */
#if defined (WIN32_SYSTEM)
#include <direct.h>
#endif /* defined (WIN32_SYSTEM) */
#include <stdio.h>
#include <string.h>
#if defined (MOTIF_USER_INTERFACE)
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#endif /* defined (MOTIF_USER_INTERFACE) */
#include "general/debug.h"
#include "user_interface/user_interface.h"
#include "user_interface/confirmation.h"
#include "user_interface/filedir.h"
#include "user_interface/message.h"
#include "user_interface/gui_dialog_macros.h"
}
#if defined (WX_USER_INTERFACE)
#include "wx/wx.h"
#include <wx/tglbtn.h>
#include "wx/xrc/xmlres.h"
#endif /* defined (WX_USER_INTERFACE)*/


/*
Module types
------------
*/
enum Confirm_type
{
	WARNING_OK_CANCEL,
	ERROR_OK,
	INFORMATION_OK,
	WARNING_OK,
	QUESTION_YES_NO
}; /* enum Confirm_type */

enum Confirm_filetype
{
	CONFIRM_CHANGE_DIRECTORY,
	CONFIRM_READ,
	CONFIRM_WRITE
}; /* enum Confirm_filetype */

struct Confirmation
{
	int dialog_active;
	int response;
}; /* struct Confirmation */

struct Confirmation_string
{
	int dialog_active;
	char *response;
}; /* struct Confirmation */

struct File_confirmation
{
	int dialog_active;
	int response;
	char *filename;
}; /* struct Confirmation */

/*
Module functions
----------------
*/
#if defined (MOTIF_USER_INTERFACE)
static void confirmation_ok(Widget widget,XtPointer confirmation_void,
	XtPointer reason)
/*******************************************************************************
LAST MODIFIED : 7 April 1998

DESCRIPTION :
==============================================================================*/
{
	struct Confirmation *confirmation;

	ENTER(confirmation_ok);
	USE_PARAMETER(widget);
	USE_PARAMETER(reason);
	if (confirmation=(struct Confirmation *)confirmation_void)
	{
		confirmation->response=1;
		confirmation->dialog_active=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"confirmation_ok.  Could not get confirmation structure");
	}
	LEAVE;
} /* confirmation_ok */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static void confirmation_cancel(Widget widget,XtPointer confirmation_void,
	XtPointer reason)
/*******************************************************************************
LAST MODIFIED : 7 April 1998

DESCRIPTION :
==============================================================================*/
{
	struct Confirmation *confirmation;

	ENTER(confirmation_cancel);
	USE_PARAMETER(widget);
	USE_PARAMETER(reason);
	if (confirmation=(struct Confirmation *)confirmation_void)
	{
		confirmation->response=0;
		confirmation->dialog_active=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"confirmation_ok.  Could not get confirmation structure");
	}
	LEAVE;
} /* confirmation_cancel */
#endif /* defined (MOTIF_USER_INTERFACE) */

static int confirmation(enum Confirm_type type,const char *title,const char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,Confirmation_add_widgets_function add_widgets_function,
	void *add_widgets_user_data,
#endif /* defined (MOTIF_USER_INTERFACE) */
	 struct User_interface *user_interface 
#if  defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
												)
/*******************************************************************************
LAST MODIFIED : 17 December 2002

DESCRIPTION :
This routine supplies a dialog window which requires a response before anything
else will continue and returns 1 if the OK button is clicked and 0 if the cancel
button is clicked.
==============================================================================*/
{
	int return_code;
#if defined (MOTIF_USER_INTERFACE)
	static Arg settings_list[]=
		{
			{XmNmessageString,(XtArgVal)0},
			{XmNtitle,(XtArgVal)0},
			/*???DB.  Changed dialogStyle to deleteResponse because of something
				hanging in unemap acquisition, but can't remember what.  So leaving as
				is */
			{XmNdialogStyle,(XtArgVal)XmDIALOG_FULL_APPLICATION_MODAL}
/*			{XmNdeleteResponse,(XtArgVal)XmDESTROY}*/
		};
	struct Confirmation confirmation;
	Widget message_box,message_parent,message_shell;
	XmString message_string,ok_label_string,cancel_label_string;
#endif /* defined (MOTIF_USER_INTERFACE) */

	ENTER(confirmation);
#if !defined (MOTIF_USER_INTERFACE)
	USE_PARAMETER(type);
	USE_PARAMETER(title);
	USE_PARAMETER(prompt);
#endif /* !defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
	USE_PARAMETER(execute_command);
#endif /* defined (WX_USER_INTERFACE) */
	return_code=0;
	if (user_interface)
	{
#if defined (MOTIF_USER_INTERFACE)
		message_string=XmStringCreateSimple(const_cast<char *>(prompt));
		settings_list[0].value=(XtArgVal)message_string;
		settings_list[1].value=(XtArgVal)title;
		message_box=(Widget)NULL;
		if (!(message_parent=parent))
		{
			message_parent=User_interface_get_application_shell(user_interface);
		}
		switch (type)
		{
			case WARNING_OK_CANCEL:
			case WARNING_OK:
			{
				message_box=XmCreateWarningDialog(message_parent,const_cast<char *>(title),settings_list,
					XtNumber(settings_list));
			} break;
			case ERROR_OK:
			{
				message_box=XmCreateErrorDialog(message_parent,const_cast<char *>(title),settings_list,
					XtNumber(settings_list));
			} break;
			case INFORMATION_OK:
			{
				message_box=XmCreateInformationDialog(message_parent,const_cast<char *>(title),
					settings_list,XtNumber(settings_list));
			} break;
			case QUESTION_YES_NO:
			{
				message_box=XmCreateQuestionDialog(message_parent,const_cast<char *>(title),settings_list,
					XtNumber(settings_list));
			} break;
		}
		message_shell=XtParent(message_box);
		switch (type)
		{
			case WARNING_OK_CANCEL:
			{
				/* remove the help button */
				XtUnmanageChild(XmMessageBoxGetChild(message_box,XmDIALOG_HELP_BUTTON));
			} break;
			case ERROR_OK:
			case INFORMATION_OK:
			case WARNING_OK:
			{
				/* remove the help button */
				XtUnmanageChild(XmMessageBoxGetChild(message_box,XmDIALOG_HELP_BUTTON));
				/* remove the cancel button */
				XtUnmanageChild(XmMessageBoxGetChild(message_box,
					XmDIALOG_CANCEL_BUTTON));
			} break;
			case QUESTION_YES_NO:
			{
				ok_label_string=XmStringCreateSimple(const_cast<char *>("Yes"));
				cancel_label_string=XmStringCreateSimple(const_cast<char *>("No"));
				XtVaSetValues(message_box,
					XmNokLabelString,ok_label_string,
					XmNcancelLabelString,cancel_label_string,
					NULL);
				/* remove the help button */
				XtUnmanageChild(XmMessageBoxGetChild(message_box,XmDIALOG_HELP_BUTTON));
				XmStringFree(ok_label_string);
				XmStringFree(cancel_label_string);
			} break;
		}
		if (add_widgets_function)
		{
			add_widgets_function(message_box,add_widgets_user_data);
		}
		XtAddCallback(message_box,XmNokCallback,confirmation_ok,&confirmation);
		XtAddCallback(message_box,XmNcancelCallback,confirmation_cancel,
			&confirmation);
		create_Shell_list_item(&(message_shell),user_interface);
		busy_cursor_on(message_shell,user_interface);
		XtManageChild(message_box);
		confirmation.response=1;
		confirmation.dialog_active=1;
		while (confirmation.dialog_active)
		{
			application_main_step(user_interface);
		}
		busy_cursor_off(message_shell,user_interface);
		destroy_Shell_list_item_from_shell(&(message_shell),user_interface);
		XmStringFree(message_string);
		return_code=confirmation.response;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
		switch (type)
		{
			case ERROR_OK:
			{
				return_code=MessageBox((HWND)NULL,prompt,title,
					MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_TOPMOST);
				if (IDOK==return_code)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			} break;
			case INFORMATION_OK:
			{
				return_code=MessageBox((HWND)NULL,prompt,title,
					MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_TOPMOST);
				if (IDOK==return_code)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			} break;
			case QUESTION_YES_NO:
			{
				return_code=MessageBox((HWND)NULL,prompt,title,
					MB_YESNO|MB_ICONWARNING|MB_APPLMODAL|MB_TOPMOST);
				if (IDYES==return_code)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			} break;
			case WARNING_OK:
			{
				return_code=MessageBox((HWND)NULL,prompt,title,
					MB_OK|MB_ICONWARNING|MB_APPLMODAL|MB_TOPMOST);
				if (IDOK==return_code)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			} break;
			case WARNING_OK_CANCEL:
			{
				return_code=MessageBox((HWND)NULL,prompt,title,
					MB_OKCANCEL|MB_ICONWARNING|MB_APPLMODAL|MB_TOPMOST);
				if (IDOK==return_code)
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			} break;
		}
#endif /* defined (WIN32_USER_INTERFACE) */
	}
	LEAVE;

	return (return_code);
} /* confirmation */

static int confirmation_get_filename_callback(char *file_name,void *data_void)
/*******************************************************************************
LAST MODIFIED : 7 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct File_confirmation *confirmation;

	ENTER(confirmation_get_filename_callback);
	confirmation=(struct File_confirmation *)data_void;
	if (confirmation != NULL)
	{
		confirmation->response=1;
		confirmation->dialog_active=0;
		if (ALLOCATE(confirmation->filename,char,strlen(file_name)+1))
		{
			strcpy(confirmation->filename,file_name);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"confirmation_get_filename_callback.  Unable to allocate filename");
			confirmation->filename=(char *)NULL;
		}
		return_code=1;
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
	"confirmation_get_filename_callback.  Could not get confirmation structure");
	}
	LEAVE;

	return (return_code);
} /* confirmation_get_filename_callback */

static int confirmation_cancel_filename_callback(void *data_void)
/*******************************************************************************
LAST MODIFIED : 11 June 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct File_confirmation *confirmation;

	ENTER(confirmation_cancel_filename_callback);
	confirmation=(struct File_confirmation *)data_void;
	if (confirmation != NULL)
	{
		confirmation->response=0;
		confirmation->dialog_active=0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
"confirmation_cancel_filename_callback.  Could not get confirmation structure");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* confirmation_cancel_filename_callback */

static char *confirmation_get_filename(enum Confirm_filetype type,
	const char *extension,struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 8 September 2000

DESCRIPTION :
This routine supplies a file selection dialog window
==============================================================================*/
{
	char *filename;
	struct File_confirmation confirmation;
	struct File_open_data *file_open_data;

	ENTER(confirmation_get_filename);
	filename=(char *)NULL;
	if (user_interface)
	{
		/* open a file selection box */
		file_open_data=(struct File_open_data *)NULL;
		switch (type)
		{
			case CONFIRM_READ:
			case CONFIRM_WRITE:
			{
				file_open_data=create_File_open_data(extension,REGULAR,
					confirmation_get_filename_callback,(void *)&confirmation,0,
					 user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
			} break;
			case CONFIRM_CHANGE_DIRECTORY:
			{
				file_open_data=create_File_open_data((char *)NULL,DIRECTORY,
					confirmation_get_filename_callback,(void *)&confirmation,0,
					 user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
																						 );
			} break;
		}
		if (file_open_data)
		{
			register_file_cancel_callback(file_open_data,
				confirmation_cancel_filename_callback,(void *)&confirmation);
			confirmation.response=1;
			confirmation.dialog_active=1;
			confirmation.filename=(char *)NULL;
			switch (type)
			{
				case CONFIRM_READ:
				{
					open_file_and_read(
#if defined (MOTIF_USER_INTERFACE)
						(Widget)NULL,(XtPointer)file_open_data,(XtPointer)NULL
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
						file_open_data
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
						file_open_data
#endif /* defined (WX_USER_INTERFACE) */
						);
				} break;
				case CONFIRM_CHANGE_DIRECTORY:
				case CONFIRM_WRITE:
				{
					open_file_and_write(
#if defined (MOTIF_USER_INTERFACE)
						(Widget)NULL,(XtPointer)file_open_data,(XtPointer)NULL
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
						file_open_data
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
						file_open_data
#endif /* defined (WX_USER_INTERFACE) */
						);
				} break;
			}
#if defined (WIN32_USER_INTERFACE)
			/*???DB.  For WIN32_USER_INTERFACE the event loop is inside open_file_and_*.  Should
				probably be moved here ? */
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (MOTIF_USER_INTERFACE)
			while (confirmation.dialog_active&&(file_open_data->selection))
			{
				application_main_step(user_interface);
			}
#endif /* defined (MOTIF_USER_INTERFACE) */
			if ((confirmation.response)&&(confirmation.filename))
			{
				switch (type)
				{
					case CONFIRM_READ:
					case CONFIRM_WRITE:
					{
						filename=confirmation.filename;
					} break;
					case CONFIRM_CHANGE_DIRECTORY:
					{
						if (confirmation.filename)
						{
							mkdir(confirmation.filename
#if defined (UNIX)
								,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif /* defined (UNIX) */
								);
							if (0==chdir(confirmation.filename))
							{
								filename=confirmation.filename;
							}
						}
					} break;
				}
			}
			/*???DB.  The X events to do with processing the closing of the dialog
				haven't necessarily been completed */
#if defined (WX_USER_INTERFACE)
			{
				 destroy_File_open_data(&file_open_data);
			}
#endif /*defined (WX_USER_INTERFACE)*/
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"confirmation_get_filename.  Could not allocate file open data");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"confirmation_get_filename.  Invalid argument(s).  %d (%d) %p %p",type,
			CONFIRM_CHANGE_DIRECTORY,extension,user_interface);
	}
	LEAVE;

	return (filename);
} /* confirmation_get_filename */

#if defined (MOTIF_USER_INTERFACE)
static void confirmation_get_string_ok(Widget widget,
	XtPointer confirmation_void,XtPointer reason)
/*******************************************************************************
LAST MODIFIED : 17 April 1998

DESCRIPTION :
==============================================================================*/
{
	char *temp_string;
	struct Confirmation_string *confirmation;
	XmString xm_string;

	ENTER(confirmation_get_string_ok);
	USE_PARAMETER(reason);
	if (confirmation=(struct Confirmation_string *)confirmation_void)
	{
		XtVaGetValues(widget,
			XmNtextString,&xm_string,
			NULL);
		if (XmStringGetLtoR(xm_string,XmFONTLIST_DEFAULT_TAG,&temp_string))
		{
			if (ALLOCATE(confirmation->response,char,strlen(temp_string)+1))
			{
				strcpy(confirmation->response,temp_string);
			}
			XtFree(temp_string);
		}
		confirmation->dialog_active=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"confirmation_get_string_ok.  Could not get confirmation structure");
	}
	LEAVE;
} /* confirmation_get_string_ok */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static void confirmation_get_string_cancel(Widget widget,
	XtPointer confirmation_void,XtPointer reason)
/*******************************************************************************
LAST MODIFIED : 17 April 1998

DESCRIPTION :
==============================================================================*/
{
	struct Confirmation_string *confirmation;

	ENTER(confirmation_get_string_cancel);
	USE_PARAMETER(widget);
	USE_PARAMETER(reason);
	if (confirmation=(struct Confirmation_string *)confirmation_void)
	{
		confirmation->response=(char *)NULL;
		confirmation->dialog_active=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"confirmation_get_string_cancel.  Could not get confirmation structure");
	}
	LEAVE;
} /* confirmation_get_string_cancel */
#endif /* defined (MOTIF_USER_INTERFACE) */

/*
Global functions
----------------
*/
int confirmation_warning_ok_cancel(const char *title,const char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,
#endif /* defined (MOTIF_USER_INTERFACE) */
	 struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This routine supplies a dialog window which requires a response
before anything else will continue and returns 1 if the OK button
is clicked and 0 if the cancel button is clicked.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_warning_ok_cancel);
	return_code=confirmation(WARNING_OK_CANCEL,title,prompt,
#if defined (MOTIF_USER_INTERFACE)
		parent,(Confirmation_add_widgets_function)NULL,NULL,
#endif /* defined (MOTIF_USER_INTERFACE) */
		 user_interface
#if defined (WX_USER_INTERFACE) 
		 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (return_code);
} /* confirmation_warning_ok_cancel */

#if defined (MOTIF_USER_INTERFACE)
int confirmation_warning_ok_cancel_plus_options(const char *title,const char *prompt,
	Widget parent,Confirmation_add_widgets_function add_widgets_function,
	void *add_widgets_user_data,struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 13 May 1998

DESCRIPTION :
This routine supplies a dialog window which requires a response
before anything else will continue and returns 1 if the OK button
is clicked and 0 if the cancel button is clicked.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_warning_ok_cancel_plus_options);
	return_code=confirmation(WARNING_OK_CANCEL,title,prompt,parent,
		add_widgets_function,add_widgets_user_data,user_interface);
	LEAVE;

	return (return_code);
} /* confirmation_warning_ok_cancel_plus_options */
#endif /* defined (MOTIF_USER_INTERFACE) */

int confirmation_error_ok(char *title,char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,
#endif /* defined (MOTIF_USER_INTERFACE) */
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This routine supplies a error dialog window which requires a response before
anything else will continue and returns when the OK button is clicked.  No other
options are supplied.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_error_ok);
	return_code=confirmation(ERROR_OK,title,prompt,
#if defined (MOTIF_USER_INTERFACE)
		parent,(Confirmation_add_widgets_function)NULL,NULL,
#endif /* defined (MOTIF_USER_INTERFACE) */
		user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (return_code);
} /* confirmation_error_ok */

int confirmation_information_ok(char *title,char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,
#endif /* defined (MOTIF_USER_INTERFACE) */
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This routine supplies a information dialog window which requires a response
before anything else will continue and returns when the OK button is clicked.
No other options are supplied.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_information_ok);
	return_code=confirmation(INFORMATION_OK,title,prompt,
#if defined (MOTIF_USER_INTERFACE)
		parent,(Confirmation_add_widgets_function)NULL,NULL,
#endif /* defined (MOTIF_USER_INTERFACE) */
		 user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (return_code);
} /* confirmation_information_ok */

int confirmation_warning_ok(const char *title,const char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,
#endif /* defined (MOTIF_USER_INTERFACE) */
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This routine supplies a warning dialog window which requires a response before
anything else will continue and returns when the OK button is clicked.  No other
options are supplied.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_warning_ok);
	return_code=confirmation(WARNING_OK,title,prompt,
#if defined (MOTIF_USER_INTERFACE)
		parent,(Confirmation_add_widgets_function)NULL,NULL,
#endif /* defined (MOTIF_USER_INTERFACE) */
		 user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (return_code);
} /* confirmation_warning_ok */

int confirmation_question_yes_no(char *title,char *prompt,
#if defined (MOTIF_USER_INTERFACE)
	Widget parent,
#endif /* defined (MOTIF_USER_INTERFACE) */
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This routine supplies a dialog window which requires a response
before anything else will continue.  It returns one if the Yes button
is clicked and No if it isn't.
==============================================================================*/
{
	int return_code;

	ENTER(confirmation_question_yes_no);
	return_code=confirmation(QUESTION_YES_NO,title,prompt,
#if defined (MOTIF_USER_INTERFACE)
		parent,(Confirmation_add_widgets_function)NULL,NULL,
#endif /* defined (MOTIF_USER_INTERFACE) */
		user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (return_code);
} /* confirmation_question_yes_no */

char *confirmation_get_read_filename(const char *extension,
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 8 April 1998

DESCRIPTION :
This routine supplies a file selection dialog window
==============================================================================*/
{
	char *filename;

	ENTER(confirmation_get_read_filename);
	filename=confirmation_get_filename(CONFIRM_READ,extension,user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (filename);
} /* confirmation_get_read_filename */

char *confirmation_get_write_filename(const char *extension,
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 8 April 1998

DESCRIPTION :
This routine supplies a file selection dialog window
==============================================================================*/
{
	char *filename;

	ENTER(confirmation_get_write_filename);
	filename=confirmation_get_filename(CONFIRM_WRITE,extension,user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (filename);
} /* confirmation_get_write_filename */

char *confirmation_change_current_working_directory(
	struct User_interface *user_interface
#if defined (WX_USER_INTERFACE) 
	 , struct Execute_command *execute_command
#endif /* defined (WX_USER_INTERFACE) */
)
/*******************************************************************************
LAST MODIFIED : 7 July 1999

DESCRIPTION :
This routine supplies a file selection dialog window for changing the current
working directory.  The new directory will be created if necessary.
==============================================================================*/
{
	char *filename;

	ENTER(confirmation_change_current_working_directory);
	filename=confirmation_get_filename(CONFIRM_CHANGE_DIRECTORY,(char *)NULL,
		user_interface
#if defined (WX_USER_INTERFACE) 
					 , execute_command
#endif /* defined (WX_USER_INTERFACE) */
);
	LEAVE;

	return (filename);
} /* confirmation_change_current_working_directory */

#if defined (MOTIF_USER_INTERFACE)
char *confirmation_get_string(char *title,char *prompt,char *default_string,
	Widget parent,struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 17 April 1998

DESCRIPTION :
This routine supplies a dialog window which requires a response
before anything else will continue and returns a char * pointer
if the OK button is clicked and NULL if the cancel button is clicked.
The string <default_string> is supplied as the initial text, the box
is initially blank if <default_string> is NULL.
==============================================================================*/
{
	Arg arguments[1];
	struct Confirmation_string confirmation;
	Widget message_box,message_shell;
	XmString default_xm_string,message_xm_string;

	ENTER(confirmation);
	XtSetArg(arguments[0],XmNtitle,title);
	message_box=XmCreatePromptDialog(parent,title,arguments,1);
	message_shell=XtParent(message_box);
	message_xm_string=XmStringCreateSimple(prompt);
	XtVaSetValues(message_box,
		XmNselectionLabelString,message_xm_string,
		XmNdialogType,XmDIALOG_PROMPT,
		XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
	XmStringFree(message_xm_string);
	if (default_string)
	{
		default_xm_string=XmStringCreateSimple(default_string);
		XtVaSetValues(message_box,
			XmNtextString,default_xm_string,
			NULL);
		XmStringFree(default_xm_string);
	}
	/* remove the help button */
	XtUnmanageChild(XmSelectionBoxGetChild(message_box,XmDIALOG_HELP_BUTTON));
	XtAddCallback(message_box,XmNokCallback,confirmation_get_string_ok,
		&confirmation);
	XtAddCallback(message_box,XmNcancelCallback,confirmation_get_string_cancel,
		&confirmation);
	create_Shell_list_item(&(message_shell),user_interface);
	busy_cursor_on(message_shell,user_interface);
	XtManageChild(message_box);
	confirmation.response=(char *)NULL;
	confirmation.dialog_active=1;
	while (confirmation.dialog_active)
	{
		application_main_step(user_interface);
	}
	busy_cursor_off(message_shell,user_interface);
	destroy_Shell_list_item_from_shell(&(message_shell),user_interface);
	LEAVE;

	return (confirmation.response);
} /* confirmation_get_string */
#endif /* defined (MOTIF_USER_INTERFACE) */
