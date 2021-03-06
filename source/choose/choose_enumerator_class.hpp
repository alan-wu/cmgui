/*******************************************************************************
FILE : choose_enumerator_class.hpp

LAST MODIFIED : 10 March 2007

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
#if !defined (CHOOSE_ENUMERATOR_CLASS_H)
#define CHOOSE_ENUMERATOR_CLASS_H

template < class Enumerator > class wxEnumeratorChooser : public wxChoice
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
{
private:
	Callback_base< typename Enumerator::Enumerator_type > *callback;
	int first_value;
	bool isBitMasks;

public:
	wxEnumeratorChooser<Enumerator>(wxPanel *parent,
		int number_of_items, const char **item_names,
		int first_value,
		typename Enumerator::Enumerator_type current_value,
		User_interface *user_interface, bool enum_is_bitmasks) :
		wxChoice(parent, /*id*/-1, wxPoint(0,0), wxSize(-1,-1)),
		first_value(first_value),
		isBitMasks(enum_is_bitmasks)
	{
		USE_PARAMETER(user_interface);
		callback = NULL;
		build_main_menu(number_of_items, item_names, current_value);

		Connect(wxEVT_COMMAND_CHOICE_SELECTED,
			wxCommandEventHandler(wxEnumeratorChooser::OnChoiceSelected));

		wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );
		sizer->Add(this,
			wxSizerFlags(1).Align(wxALIGN_CENTER).Expand());
		parent->SetSizer(sizer);

		Show();
	}

	~wxEnumeratorChooser<Enumerator>()
		{
			if (callback)
				delete callback;
		}

	void OnChoiceSelected(wxCommandEvent& event)
	{
		USE_PARAMETER(event);
		if (callback)
		{
			callback->callback_function(get_item());
		}
   }

	typename Enumerator::Enumerator_type get_item()
	{
		if (!isBitMasks)
		{
			return (static_cast<typename Enumerator::Enumerator_type>
				(GetSelection() + first_value));
		}
		else
		{
			int index = GetSelection();
			int enum_value = first_value;
			while (index)
			{
				enum_value = enum_value << 1;
				--index;
			}
			return static_cast<typename Enumerator::Enumerator_type>(enum_value);
		}
	}

	int set_callback(Callback_base< typename Enumerator::Enumerator_type >
		*callback_object)
	{
		if (callback)
			delete callback;
		callback = callback_object;
		return (1);
	}

	int set_item(typename Enumerator::Enumerator_type new_value)
	{
		unsigned int return_code;

		if (!isBitMasks)
		{
			SetSelection(new_value - first_value);
		}
		else
		{
			// handle enums that are linear increasing OR powers of 2
			// requires appropriate increment operators being defined
			int index = 0;
			int enum_value = first_value;
			while (enum_value < new_value)
			{
				enum_value = enum_value << 1;
				++index;
			}
			SetSelection(index);
		}

		// Could check to see that the value was actually set
		return_code = 1;

		return (return_code);
	}

	int build_main_menu(int number_of_items,
		const char **item_names,
		typename Enumerator::Enumerator_type current_value)
	{
		int i;
		Clear();
		for (i = 0 ; i < number_of_items ; i++)
		{
			Append(wxString::FromAscii(item_names[i]));
		}
		set_item(current_value);
		return 1;
	}
};

template < class Enumerator > class Enumerator_chooser
{
private:
	Enumerator *enumerator;
	wxPanel *parent;
	typename Enumerator::Conditional_function *conditional_function;
	void *conditional_function_user_data;
	wxEnumeratorChooser<Enumerator> *chooser;
	Callback_base< typename Enumerator::Enumerator_type > *update_callback;
  int number_of_items;
  const char **item_names;
	int first_value;
	bool isBitMasks;

public:
	Enumerator_chooser(wxPanel *parent,
		typename Enumerator::Enumerator_type current_value,
		typename Enumerator::Conditional_function *conditional_function,
		void *conditional_function_user_data,
		User_interface *user_interface,
		bool enum_is_bitmasks = false/*bitmask_enum*/) :
		enumerator(new Enumerator()), parent(parent),
		conditional_function(conditional_function),
		conditional_function_user_data(conditional_function_user_data),
		isBitMasks(enum_is_bitmasks)
	{
		chooser = (wxEnumeratorChooser<Enumerator> *)NULL;
		update_callback = (Callback_base< typename Enumerator::Enumerator_type > *)NULL;
		number_of_items = 0;
		item_names = (const char **)NULL;
		first_value = 0;
		if (build_items())
		{
			chooser = new wxEnumeratorChooser<Enumerator>(parent,
				number_of_items, item_names, first_value, current_value,
				user_interface, /* bitmasks enum */ isBitMasks);
			typedef int (Enumerator_chooser::*Member_function)(typename Enumerator::Enumerator_type);
			Callback_base<typename Enumerator::Enumerator_type> *callback =
				new Callback_member_callback< typename Enumerator::Enumerator_type,
				Enumerator_chooser, Member_function>(
				this, &Enumerator_chooser::chooser_callback);

			chooser->set_callback(callback);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Enumerator_chooser::Enumerator_chooser.   "
				" Could not get items");
		}

	} /* Enumerator_chooser::Enumerator_chooser */

	~Enumerator_chooser()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
	{
		if (chooser)
		{
			  chooser->Destroy();
		}
		if (update_callback)
			delete update_callback;
		delete enumerator;
		if (item_names)
		{
			 // We only free the array, not the strings themselves for
			 // strings from an enumerator
			DEALLOCATE(item_names);
		}
	} /* Enumerator_chooser::~Enumerator_chooser() */

	int chooser_callback(typename Enumerator::Enumerator_type value)
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
			update_callback->callback_function(value);
		}
		return_code=1;

		return (return_code);
	} /* Enumerator_chooser::get_callback */

	Callback_base< typename Enumerator::Enumerator_type > *get_callback()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Returns a pointer to the callback item.
============================================================================*/
	{
		return update_callback;
	} /* Enumerator_chooser::get_callback */

	int set_callback(Callback_base< typename Enumerator::Enumerator_type > *new_callback)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Changes the callback item.
============================================================================*/
	{
		if (update_callback)
		{
			delete update_callback;
		}
		update_callback = new_callback;
		return(1);
	} /* Enumerator_chooser::set_callback */

	typename Enumerator::Enumerator_type get_value()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Returns the currently chosen object.
============================================================================*/
	{
		return(chooser->get_item());
	} /* Enumerator_chooser::get_object */

	int set_value(typename Enumerator::Enumerator_type new_value)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Changes the chosen object in the choose_object_widget.
============================================================================*/
	{
		return(chooser->set_item(new_value));
	} /* Enumerator_chooser::set_object */

	int set_conditional_function(
		typename Enumerator::Conditional_function *in_conditional_function,
		void *in_conditional_function_user_data, typename Enumerator::Enumerator_type new_value)
/*****************************************************************************
LAST MODIFIED : 10 March 2007

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
				number_of_items, item_names, new_value);
		}
		else
		{
			return_code=0;
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"Enumerator_chooser::set_conditional_function"
				"Could not update menu");
		}
		return (return_code);
	} /* Enumerator_chooser::set_conditional_function */

private:

	int is_item_in_chooser(typename Enumerator::Enumerator_type value)
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
============================================================================*/
	{
		int i, return_code;

		if ((value > 0) && (value <= number_of_items))
		{
			return_code = 1;
		}
		else
		{
			return_code=0;
		}

		return (return_code);
	} /* Enumerator_chooser::is_item_in_chooser */

	int build_items()
/*****************************************************************************
LAST MODIFIED : 8 February 2007

DESCRIPTION :
Updates the arrays of all the choosable objects and their names.
============================================================================*/
	{
		int return_code;

		if (item_names)
		{
			 // We only free the array, not the strings themselves for
			 // strings from an enumerator
			DEALLOCATE(item_names);
		}

		item_names = enumerator->get_valid_strings(&number_of_items,
			conditional_function, conditional_function_user_data);
		if (enumerator->value_to_string((static_cast<typename Enumerator::Enumerator_type>(0))))
		{
			first_value = 0;
		}
		else
		{
			first_value = 1;
		}
		return_code = 1;

		return (return_code);
	}  /* Enumerator_chooser::build_items */

}; /* template < class Managed_object > class Enumerator_chooser */

#endif /* !defined (CHOOSE_ENUMERATOR_CLASS_H) */
