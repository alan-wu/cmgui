/*******************************************************************************
FILE : computed_field_time.c

LAST MODIFIED : 25 August 2006

DESCRIPTION :
Implements a number of basic component wise operations on computed fields.
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
extern "C" {
#include <math.h>
#include "computed_field/computed_field.h"
}
#include "computed_field/computed_field_private.hpp"
extern "C" {
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "time/time.h"
#include "user_interface/message.h"
#include "computed_field/computed_field_time.h"
}

class Computed_field_time_package : public Computed_field_type_package
{
public:
	struct Time_keeper *time_keeper;
};

namespace {

char computed_field_time_lookup_type_string[] = "time_lookup";

class Computed_field_time_lookup : public Computed_field_core
{
public:
	Computed_field_time_lookup() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_time_lookup();
	}

	const char *get_type_string()
	{
		return(computed_field_time_lookup_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_time_lookup*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	int evaluate_cache_at_location(Field_location* location);

	int list();

	char* get_command_string();

	int has_multiple_times();

	virtual int propagate_find_element_xi(const FE_value *values, int number_of_values,
		struct FE_element **element_address, FE_value *xi,
		FE_value time, Cmiss_mesh_id mesh);
};

int Computed_field_time_lookup::evaluate_cache_at_location(
    Field_location* location)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	FE_value *temp, *temp1;
	int i, number_of_derivatives, return_code;

	ENTER(Computed_field_time_lookup::evaluate_cache_at_location);
	if (field && location && (field->number_of_source_fields == 2) && 
		(field->number_of_components == field->source_fields[0]->number_of_components) && 
		(1 == field->source_fields[1]->number_of_components))
	{
		Field_element_xi_location *element_xi_location;
		Field_node_location *node_location;

		/* 1. Precalculate any source fields that this field depends on */
		if (0 != (element_xi_location  = 
				dynamic_cast<Field_element_xi_location*>(location)))
		{
			Field_element_xi_location location_no_derivatives(
				element_xi_location->get_element(), element_xi_location->get_xi(),
				element_xi_location->get_time(), element_xi_location->get_top_level_element());

			return_code = Computed_field_evaluate_cache_at_location(
				field->source_fields[1], &location_no_derivatives);
			if (return_code)
			{
				Field_element_xi_location location_lookup_time(
					element_xi_location->get_element(), element_xi_location->get_xi(),
					field->source_fields[1]->values[0], 
					element_xi_location->get_top_level_element(),
					location->get_number_of_derivatives());
				
				return_code = Computed_field_evaluate_cache_at_location(
					field->source_fields[0], &location_lookup_time);
			}
		}
		else if (0 != (node_location = 
			dynamic_cast<Field_node_location*>(location)))
		{
			Field_node_location location_no_derivatives(
				node_location->get_node(), node_location->get_time());

			return_code = Computed_field_evaluate_cache_at_location(
				field->source_fields[1], &location_no_derivatives);
			if (return_code)
			{
				Field_node_location location_lookup_time(
					node_location->get_node(),
					field->source_fields[1]->values[0], 
					location->get_number_of_derivatives());
				
				return_code = Computed_field_evaluate_cache_at_location(
					field->source_fields[0], &location_lookup_time);
			}
		}
		else
		{
			return_code = 0;
		}
		if (return_code)
		{
			/* 2. Copy the results */
			for (i=0;i<field->number_of_components;i++)
			{
				field->values[i] = field->source_fields[0]->values[i];
			}
			number_of_derivatives = location->get_number_of_derivatives();
			if (number_of_derivatives > 0)
			{
				temp=field->derivatives;
				temp1=field->source_fields[0]->derivatives;
				for (i=(field->number_of_components*number_of_derivatives);
					  0<i;i--)
				{
					(*temp)=(*temp1);
					temp++;
					temp1++;
				}
				field->derivatives_valid = 1;
			}
			else
			{
				field->derivatives_valid = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::evaluate_cache_at_location.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_lookup::evaluate_cache_at_location */


int Computed_field_time_lookup::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_time_lookup);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    time field : %s\n",
			field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_time_lookup.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_time_lookup */

char *Computed_field_time_lookup::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_time_lookup::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_time_lookup_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " time_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, " ", &error);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_time_lookup::get_command_string */

int Computed_field_time_lookup::has_multiple_times ()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns 1 if the time_field source_field has multiple times.  The times of the
actual source field are controlled by this time field and so changes in the
global time do not matter.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_default::has_multiple_times);
	if (field && (2 == field->number_of_source_fields))
	{
		return_code=0;
		if (Computed_field_has_multiple_times(field->source_fields[1]))
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::has_multiple_times.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_lookup::has_multiple_times */

int Computed_field_time_lookup::propagate_find_element_xi(
	const FE_value *values, int number_of_values, struct FE_element **element_address,
	FE_value *xi, FE_value time, Cmiss_mesh_id mesh)
{
	FE_value *source_values;
	int i,return_code;

	ENTER(Computed_field_time_lookup::propagate_find_element_xi);
	USE_PARAMETER(time);
	if (field && values && (number_of_values == field->number_of_components))
	{
		return_code = 1;
		/* reverse the offset */
		Field_element_xi_location location_no_derivatives(
			*element_address, xi, time);
		if (Computed_field_evaluate_cache_at_location(
					field->source_fields[1], &location_no_derivatives) &&
			ALLOCATE(source_values,FE_value,field->number_of_components))
		{
			for (i=0;i<field->number_of_components;i++)
			{
				source_values[i] = values[i];
			}
			return_code=Computed_field_find_element_xi(
				field->source_fields[0],source_values,number_of_values,
				field->source_fields[1]->values[0],element_address,
				xi,mesh,/*propagate_field*/1,
				/*find_nearest_location*/0);
			DEALLOCATE(source_values);
			Computed_field_clear_cache(field->source_fields[1]);
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::propagate_find_element_xi.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_offset::propagate_find_element_xi */

} //namespace

struct Computed_field *Computed_field_create_time_lookup(
	struct Cmiss_field_module *field_module,
	struct Computed_field *source_field, struct Computed_field *time_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field && time_field &&
		(1 == time_field->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field;
		source_fields[1] = time_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_time_lookup());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_time_lookup.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_time_lookup(struct Computed_field *field,
	struct Computed_field **source_field, struct Computed_field **time_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_TIME_LOOKUP, the 
<source_field> and <time_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_time_lookup);
	if (field&&(dynamic_cast<Computed_field_time_lookup*>(field->core)))
	{
		*source_field = field->source_fields[0];
		*time_field = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_time_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_time_lookup */

int define_Computed_field_type_time_lookup(struct Parse_state *state,
	void *field_modify_void,void *computed_field_time_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_TIME_LOOKUP (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field **source_fields;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data,
		set_time_field_data;

	ENTER(define_Computed_field_type_time_lookup);
	USE_PARAMETER(computed_field_time_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		/* get valid parameters for projection field */
		source_fields = (struct Computed_field **)NULL;
		if (ALLOCATE(source_fields, struct Computed_field *, 2))
		{
			source_fields[0] = (struct Computed_field *)NULL;
			source_fields[1] = (struct Computed_field *)NULL;
			if ((NULL != field_modify->get_field()) &&
				(computed_field_time_lookup_type_string ==
					Computed_field_get_type_string(field_modify->get_field())))
			{
				return_code=Computed_field_get_type_time_lookup(field_modify->get_field(), 
					source_fields, source_fields + 1);
			}
			if (return_code)
			{
				/* must access objects for set functions */
				if (source_fields[0])
				{
					ACCESS(Computed_field)(source_fields[0]);
				}
				if (source_fields[1])
				{
					ACCESS(Computed_field)(source_fields[1]);
				}

				option_table = CREATE(Option_table)();
				/* fields */
				set_source_field_data.computed_field_manager=
					field_modify->get_field_manager();
				set_source_field_data.conditional_function=Computed_field_has_numerical_components;
				set_source_field_data.conditional_function_user_data=(void *)NULL;
				Option_table_add_entry(option_table,"field",&source_fields[0],
					&set_source_field_data,set_Computed_field_conditional);
				set_time_field_data.computed_field_manager=
					field_modify->get_field_manager();
				set_time_field_data.conditional_function=Computed_field_is_scalar;
				set_time_field_data.conditional_function_user_data=(void *)NULL;
				Option_table_add_entry(option_table,"time_field",&source_fields[1],
					&set_time_field_data,set_Computed_field_conditional);
				return_code=Option_table_multi_parse(option_table,state);
				/* no errors,not asking for help */
				if (return_code)
				{
					return_code = field_modify->update_field_and_deaccess(
						Computed_field_create_time_lookup(field_modify->get_field_module(),
							source_fields[0], source_fields[1]));
				}
				if (!return_code)
				{
					if ((!state->current_token)||
						(strcmp(PARSER_HELP_STRING,state->current_token)&&
							strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
					{
						/* error */
						display_message(ERROR_MESSAGE,
							"define_Computed_field_type_time_lookup.  Failed");
					}
				}
				if (source_fields[0])
				{
					DEACCESS(Computed_field)(&source_fields[0]);
				}
				if (source_fields[1])
				{
					DEACCESS(Computed_field)(&source_fields[1]);
				}
				DESTROY(Option_table)(&option_table);
			}
			DEALLOCATE(source_fields);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"define_Computed_field_type_time_lookup.  Not enough memory");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_time_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_time_lookup */

namespace {

char computed_field_time_value_type_string[] = "time_value";

class Computed_field_time_value : public Computed_field_core
{
public:
	
	Time_object *time_object;

	Computed_field_time_value(Time_keeper* time_keeper) : 
		Computed_field_core(),
		time_object(NULL)
	{
		time_object = Time_object_create_regular(
			/*update_frequency*/10.0, /*time_offset*/0.0); 
		if (!Time_keeper_add_time_object(time_keeper, time_object))
		{
			DEACCESS(Time_object)(&time_object);
		}
	};

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			if (time_object && Time_object_set_name(time_object, parent->name))
			{
				return true;
			}
		}
		return false;
	}

	~Computed_field_time_value()
	{
		DEACCESS(Time_object)(&time_object);
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_time_value( 
			Time_object_get_time_keeper(time_object));
	}

	const char *get_type_string()
	{
		return(computed_field_time_value_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate_cache_at_location(Field_location* location);

	int list();

	char* get_command_string();

	int has_multiple_times();
};

int Computed_field_time_value::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	Computed_field_time_value* other;
	int return_code;

	ENTER(Computed_field_time_value::compare);
	if (field && (other = dynamic_cast<Computed_field_time_value*>(other_core)))
	{
		if (Time_object_get_time_keeper(time_object) == 
			Time_object_get_time_keeper(other->time_object))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_value::compare */

int Computed_field_time_value::evaluate_cache_at_location(
    Field_location* location)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_time_value::evaluate_cache_at_location);
	USE_PARAMETER(location);
	if (field)
	{
		field->values[0] = Time_object_get_current_time(time_object);
		field->derivatives_valid = 0;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_value::evaluate_cache_at_location.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_value::evaluate_cache_at_location */


int Computed_field_time_value::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_time_value);
	if (field)
	{
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_time_value.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_time_value */

char *Computed_field_time_value::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string;
	int error;

	ENTER(Computed_field_time_value::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_time_value_type_string, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_value::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_time_value::get_command_string */

int Computed_field_time_value::has_multiple_times()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Always has multiple times.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_time_value::has_multiple_times);
	if (field)
	{
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_value::has_multiple_times.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_value::has_multiple_times */

} //namespace

struct Computed_field *Computed_field_create_time_value(
	struct Cmiss_field_module *field_module, struct Time_keeper *time_keeper)
{
	struct Computed_field *field = NULL;
	if (field_module && time_keeper)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_time_value(time_keeper));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_time_value.  Invalid argument(s)");
	}

	return (field);
}

/* Computed_field_get_type_time_value(struct Computed_field *field) */
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
There are no fields to fetch from a time value field.
==============================================================================*/

int define_Computed_field_type_time_value(struct Parse_state *state,
	void *field_modify_void,void *computed_field_time_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_TIME_VALUE (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	int return_code = 0;
	Computed_field_time_package *computed_field_time_package;
	Computed_field_modify_data *field_modify;

	ENTER(define_Computed_field_type_time_value);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void)&&
		(computed_field_time_package=
		(Computed_field_time_package *)
		computed_field_time_package_void))
	{
    if ((!(state->current_token)) ||
		(strcmp(PARSER_HELP_STRING,state->current_token)&&
		strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
		{
			return_code = field_modify->update_field_and_deaccess(
				Computed_field_create_time_value(field_modify->get_field_module(),
					computed_field_time_package->time_keeper));
		}
		else
		{
			/* Help */
			return_code = 0;
		}
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_time_value */

int Computed_field_register_types_time(
	struct Computed_field_package *computed_field_package,
	struct Time_keeper *time_keeper)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	Computed_field_time_package
		*computed_field_time_package =
		new Computed_field_time_package;

	ENTER(Computed_field_register_types_time);
	if (computed_field_package)
	{
		computed_field_time_package->time_keeper =
			time_keeper;
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_time_lookup_type_string,
			define_Computed_field_type_time_lookup,
			computed_field_time_package);
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_time_value_type_string,
			define_Computed_field_type_time_value,
			computed_field_time_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_time.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_time */

