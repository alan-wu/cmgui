/*******************************************************************************
FILE : choose_manager_class.hpp

LAST MODIFIED : 12 March 2007

DESCRIPTION :
Class for implementing an wxList dialog control for choosing an object
from its manager (subject to an optional conditional function). Handles manager
messages to keep the menu up-to-date.
Calls the client-specified callback routine if a different object is chosen.
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CHOOSE_MANAGER_CLASS_H)
#define CHOOSE_MANAGER_CLASS_H

#include "choose/choose_class.hpp"
#include "general/mystring.h"
#include "general/message.h"

template < class Managed_object, class Manager > class Managed_object_chooser
/*****************************************************************************
LAST MODIFIED : 12 March 2007

DESCRIPTION :
============================================================================*/
{
private:
	Manager *manager;
	wxPanel *parent;
	typename Manager::Manager_conditional_function *conditional_function;
	void *conditional_function_user_data;
	void *manager_callback_id;
	wxChooser<Managed_object*> *chooser;
	Callback_base<Managed_object *> *update_callback;
   int number_of_items;
   Managed_object **items;
   char **item_names;
   bool null_item_flag;

	static void global_object_change(typename Manager::Manager_message_type *message,
		void *class_chooser_void);

public:
	Managed_object_chooser(wxPanel *parent,
		Managed_object *current_object,
		typename Manager::Manager_type *struct_manager,
		typename Manager::List_conditional_function *conditional_function,
		void *conditional_function_user_data,
		User_interface *user_interface) :
		manager(new Manager(struct_manager)), parent(parent),
		conditional_function(conditional_function),
		conditional_function_user_data(conditional_function_user_data)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
	{
		manager_callback_id = (void *)NULL;
		chooser = (wxChooser<Managed_object*> *)NULL;
		update_callback = (Callback_base<Managed_object*> *)NULL;
		number_of_items = 0;
		items = (Managed_object **)NULL;
		item_names = (char **)NULL;
		null_item_flag = false;
		if (build_items())
		{
			chooser = new wxChooser<Managed_object*>
				(parent, number_of_items,
				items, item_names, current_object,
				user_interface);
			typedef int (Managed_object_chooser::*Member_function)(Managed_object *);
			Callback_base<Managed_object*> *callback =
			   new Callback_member_callback<Managed_object*,
				Managed_object_chooser, Member_function>(
				this, &Managed_object_chooser::chooser_callback);

			chooser->set_callback(callback);

			manager_callback_id =
					manager->register_callback(global_object_change, this);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Managed_object_chooser::Managed_object_chooser.   "
				" Could not get items");
		}

	} /* Managed_object_chooser::Managed_object_chooser */

	~Managed_object_chooser()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
	{
		int i;

		if (number_of_items>=0)
		{
			 if (items)
			 {
					DEALLOCATE(items);
			 }
			 if (item_names)
			 {
					for (i=0;i<number_of_items;i++)
					{
						 DEALLOCATE(item_names[i]);
					}
					DEALLOCATE(item_names);
			 }
		}
		if (manager_callback_id)
		{
			manager->deregister_callback(manager_callback_id);
		}
		if (update_callback)
		{
			delete update_callback;
		}
		delete manager;

	} /* Managed_object_chooser::~Managed_object_chooser() */

	int chooser_callback(Managed_object *object)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Called by the
============================================================================*/
	{
		int return_code;

		if (update_callback)
		{
			/* now call the procedure with the user data */
			update_callback->callback_function(object);
		}
		return_code=1;

		return (return_code);
	} /* Managed_object_chooser::get_callback */

	/***************************************************************************//**
	* Set the chooser to include a NULL object as one of the selection.
	*
	* @param flag  flag to set this option on or off in the chooser
	*/
	int include_null_item(bool flag)
	{
		int return_code = 1;
		if (null_item_flag != flag)
		{
			null_item_flag = flag;
			if (build_items())
			{
				return_code=chooser->build_main_menu(
					number_of_items, items, item_names, (Managed_object *)NULL);
			}
		}
		return return_code;
	}

/***************************************************************************//**
* Set the chooser manager to the one in the argument if appropriate.
*
* @param new_manager object manager to be used in this chooser
*/
	int set_manager(typename Manager::Manager_type *new_manager)
	{
		int return_code = 1;

		Manager *temp_manager(new Manager(new_manager));
		if (temp_manager != manager)
		{
			if (manager && manager_callback_id)
				manager->deregister_callback(manager_callback_id);
			manager = temp_manager;
			if (build_items())
			{
				return_code=chooser->build_main_menu(
					number_of_items, items, item_names, (Managed_object *)NULL);
				return_code = 1;
				manager_callback_id = manager->manager ?
					manager->register_callback(global_object_change, this) : 0;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Managed_object_chooser::set_manager.   "
					" Could not update menu");
				return_code = 0;
			}
		}

		return (return_code);
	}

	Callback_base<Managed_object*> *get_callback()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Returns a pointer to the callback item.
============================================================================*/
	{
		return update_callback;
	} /* Managed_object_chooser::get_callback */

	int set_callback(Callback_base<Managed_object*> *new_callback)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Changes the callback item.
============================================================================*/
	{
		if (update_callback)
		{
			delete update_callback;
			update_callback = NULL;
		}
		update_callback = new_callback;
		return(1);
	} /* Managed_object_chooser::set_callback */

	Managed_object *get_object()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Returns the currently chosen object.
============================================================================*/
	{
		return(chooser->get_item());
	} /* Managed_object_chooser::get_object */

	 int get_number_of_object()
/*****************************************************************************
LAST MODIFIED : 20 April 2007

DESCRIPTION :
Returns the number in list
============================================================================*/
	{
		return(chooser->get_number_of_item());
	} /* Managed_object_chooser::get_object */


	int set_object(Managed_object *new_object)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Changes the chosen object in the choose_object_widget.
============================================================================*/
	{
		return(chooser->set_item(new_object));
	} /* Managed_object_chooser::set_object */

	int set_conditional_function(
		typename Manager::List_conditional_function *in_conditional_function,
		void *in_conditional_function_user_data, Managed_object *new_object)
/*****************************************************************************
LAST MODIFIED : 9 June 2000

DESCRIPTION :
Changes the conditional_function and user_data limiting the available
selection of objects. Also allows new_object to be set simultaneously.
============================================================================*/
	{
		int return_code;

		conditional_function = in_conditional_function;
		conditional_function_user_data = in_conditional_function_user_data;
		if (build_items())
		{
			return_code=chooser->build_main_menu(
				number_of_items, items, item_names, new_object);
		}
		else
		{
			return_code=0;
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"Managed_object_chooser::set_conditional_function"
				"Could not update menu");
		}
		return (return_code);
	} /* Managed_object_chooser::set_conditional_function */

private:

	int is_item_in_chooser(Managed_object *object)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
	{
		int i, return_code;

		if (object)
		{
			return_code = 0;
			for (i = 0 ; !return_code && (i < number_of_items) ; i++)
			{
				if (object == items[i])
				{
					return_code=1;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Managed_object_chooser::is_item_in_chooser.  Invalid argument(s)");
			return_code=0;
		}

		return (return_code);
	} /* Managed_object_chooser::is_item_in_chooser */

	/** A manager iterator which adds each object to the chooser */
	static int add_object_to_list(Managed_object *object, void *chooser_object_void)
	{
		int return_code;
		Managed_object_chooser* chooser_object;

		return_code = 1;
		chooser_object = (Managed_object_chooser*)chooser_object_void;
		if (chooser_object)
		{
			if (!(chooser_object->conditional_function) ||
				(chooser_object->conditional_function)(object,
					chooser_object->conditional_function_user_data))
			{
				if (chooser_object->manager->get_object_name(object,
						chooser_object->item_names + chooser_object->number_of_items))
				{
					chooser_object->items[chooser_object->number_of_items] = object;
					chooser_object->number_of_items++;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Managed_object_chooser::add_object_to_list.  "
						"Could not get name of object");
					return_code=0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Managed_object_chooser::add_object_to_list.  "
				"Invalid object");
			return_code = 0;
		}
		return (return_code);
	}

	/** Updates the arrays of all the choosable objects and their names */
	int build_items()
	{
		if (items)
		{
			DEALLOCATE(items);
			items = 0;
		}
		if (item_names)
		{
			for (int i = 0; i < number_of_items; i++)
			{
				DEALLOCATE(item_names[i]);
			}
			DEALLOCATE(item_names);
			item_names = 0;
		}
		number_of_items = 0;
		int return_code = 1;
		if (manager && manager->manager)
		{
			Managed_object **new_items = 0;
			char **new_item_names = 0;
			int max_number_of_objects = manager->number_in_manager();
			if (null_item_flag)
			{
				max_number_of_objects++;
			}
			if ((0 == max_number_of_objects) ||
				(ALLOCATE(new_items, Managed_object *, max_number_of_objects) &&
					ALLOCATE(new_item_names, char *, max_number_of_objects)))
			{
				items = new_items;
				item_names = new_item_names;
				if (null_item_flag)
				{
					*(this->item_names) = duplicate_string("-");
					this->items[0] = NULL;
					this->number_of_items++;
				}
				manager->for_each_object_in_manager(
					add_object_to_list, this);
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}
		}
		return (return_code);
	}

	static int object_status_changed(Managed_object *object, void *class_chooser_void)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Returns true if <object> is in the chooser but should not be, or is not in the
chooser and should be.
============================================================================*/
{
	int object_is_in_chooser, object_should_be_in_chooser, return_code;
	Managed_object_chooser* class_chooser;

	if (object &&
		(class_chooser = (Managed_object_chooser*)class_chooser_void))
	{
		if (class_chooser->conditional_function)
		{
			object_is_in_chooser = class_chooser->is_item_in_chooser(object);
			object_should_be_in_chooser =
				(class_chooser->conditional_function)(object, class_chooser->conditional_function_user_data);
			return_code =
				(object_is_in_chooser && (!object_should_be_in_chooser)) ||
				((!object_is_in_chooser) && object_should_be_in_chooser);
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Managed_object_chooser::object_status_changed.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Managed_object_chooser::object_status_changed */

}; /* template < class Managed_object > class Managed_object_chooser */

template  < class Managed_object, class Manager > void Managed_object_chooser<Managed_object,Manager> ::global_object_change(
	typename Manager::Manager_message_type *message,
	void *class_chooser_void)
/*****************************************************************************
LAST MODIFIED : 7 February 2007

DESCRIPTION :
Rebuilds the choose object menu in response to manager messages.
Tries to minimise menu rebuilds as much as possible, since these cause
annoying flickering on the screen.
============================================================================*/
{
	Managed_object_chooser* class_chooser;

	if (message &&
		(class_chooser = (Managed_object_chooser*)class_chooser_void))
	{
		bool update_menu = false;
		int change_summary = class_chooser->manager->manager_message_get_change_summary(message);
		if (change_summary & (
			Manager::Manager_change_add |
			Manager::Manager_change_remove |
			Manager::Manager_change_identifier))
		{
			if ((NULL == class_chooser->conditional_function) ||
				class_chooser->manager->manager_message_has_changed_object_that(message,
					Manager::Manager_change_add | Manager::Manager_change_remove | Manager::Manager_change_identifier,
					class_chooser->conditional_function, class_chooser->conditional_function_user_data))
			{
				update_menu = true;
			}
		}
		if ((!update_menu) &&
			(change_summary & Manager::Manager_change_object_not_identifier))
		{
			if ((class_chooser->conditional_function) &&
				class_chooser->manager->manager_message_has_changed_object_that(message,
					Manager::Manager_change_object_not_identifier, object_status_changed, class_chooser))
			{
				update_menu = true;
			}
		}
		if (update_menu)
		{
		  if (!(class_chooser->build_items() &&
				class_chooser->chooser->build_main_menu(
					class_chooser->number_of_items, class_chooser->items, class_chooser->item_names,
					class_chooser->chooser->get_item())))
			{
				display_message(ERROR_MESSAGE,
					"Managed_object_chooser::global_object_change.  "
					"Could not update menu");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Managed_object_chooser::global_object_change.  "
			"Invalid argument(s)");
	}
	LEAVE;
} /* Managed_object_chooser::global_object_change */


#endif /* !defined (CHOOSE_MANAGER_CLASS_H) */
