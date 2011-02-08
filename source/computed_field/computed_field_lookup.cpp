/*******************************************************************************
FILE : computed_field_lookup.c

LAST MODIFIED : 25 July 2007

Defines fields for looking up values at given locations.
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
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_time.h"
#include "region/cmiss_region.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "user_interface/message.h"
#include "computed_field/computed_field_lookup.h"
#include "graphics/quaternion.hpp"
}

class Computed_field_lookup_package : public Computed_field_type_package
{
public:
	struct Cmiss_region *root_region;
};

namespace {

void Computed_field_nodal_lookup_FE_region_change(struct FE_region *fe_region,
	struct FE_region_changes *changes, void *computed_field_void);
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the node we are looking at changes generate a computed field change message.
==============================================================================*/

char computed_field_nodal_lookup_type_string[] = "nodal_lookup";

class Computed_field_nodal_lookup : public Computed_field_core
{
public:
	FE_node *lookup_node;

	Computed_field_nodal_lookup(FE_node *lookup_node) : 
		Computed_field_core(),
		lookup_node(ACCESS(FE_node)(lookup_node))
	{
	};
		
	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			if (Cmiss_region_get_FE_region(Computed_field_get_region(parent->source_fields[0])) ==
				FE_node_get_FE_region(lookup_node))
			{
				if (FE_region_add_callback(FE_node_get_FE_region(lookup_node), 
					Computed_field_nodal_lookup_FE_region_change, 
					(void *)parent))
				{
					return true;
				}
			}
		}
		return false;
	}
	
	~Computed_field_nodal_lookup();

private:
	Computed_field_core *copy();

	const char *get_type_string()
	{
		return(computed_field_nodal_lookup_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate_cache_at_location(Field_location* location);

	int list();

	char* get_command_string();

	int is_defined_at_location(Field_location* location);

	int has_multiple_times();
};

Computed_field_nodal_lookup::~Computed_field_nodal_lookup()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{

	ENTER(Computed_field_nodal_lookup::~Computed_field_nodal_lookup);
	if (field)
	{
		FE_region_remove_callback(FE_node_get_FE_region(lookup_node),
			Computed_field_nodal_lookup_FE_region_change, (void *)field);
		DEACCESS(FE_node)(&lookup_node);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_nodal_lookup::~Computed_field_nodal_lookup.  Invalid argument(s)");
	}
	LEAVE;

} /* Computed_field_nodal_lookup::~Computed_field_nodal_lookup */

Computed_field_core* Computed_field_nodal_lookup::copy()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	Computed_field_nodal_lookup* core =
		new Computed_field_nodal_lookup(lookup_node);

	return (core);
} /* Computed_field_compose::copy */

int Computed_field_nodal_lookup::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_nodal_lookup* other;
	int return_code;

	ENTER(Computed_field_nodal_lookup::compare);
	if (field && (other = dynamic_cast<Computed_field_nodal_lookup*>(other_core)))
	{
		if (lookup_node == other->lookup_node)
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
} /* Computed_field_nodal_lookup::compare */

int Computed_field_nodal_lookup::is_defined_at_location(Field_location* location)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns true if <field> can be calculated in <element>, which is determined
by checking the source field on the lookup node.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_nodal_lookup::is_defined_at_location);
	if (field && location)
	{
		Field_node_location nodal_location(lookup_node,
			location->get_time());

		return_code = Computed_field_is_defined_at_location(field->source_fields[0],
			&nodal_location);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_nodal_lookup::is_defined_at_location.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_nodal_lookup::is_defined_at_location */

int Computed_field_nodal_lookup::evaluate_cache_at_location(
    Field_location* location)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	int return_code,i;

	ENTER(Computed_field_nodal_lookup::evaluate_cache_at_location);
	if (field)
	{
		Field_node_location nodal_location(lookup_node,
			location->get_time());

		/* 1. Precalculate any source fields that this field depends on */
		/* only calculate the time_field at this time and then use the
		   results of that field for the time of the source_field */
		if (return_code=Computed_field_evaluate_cache_at_location(
          field->source_fields[0], &nodal_location))
		{
			/* 2. Copy the results */
			for (i=0;i<field->number_of_components;i++)
			{
				field->values[i] = field->source_fields[0]->values[i];
			}
			/* 3. Set all derivatives to 0 */
			for (i=0;i<(field->number_of_components * 
					location->get_number_of_derivatives());i++)
			{
				field->derivatives[i] = (FE_value)0.0;
			}
			field->derivatives_valid = 1;
		}
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


int Computed_field_nodal_lookup::list(
	)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_time_nodal_lookup);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    node : %d\n",
			get_FE_node_identifier(lookup_node));
		Cmiss_region *region = NULL;
		FE_region_get_Cmiss_region(FE_node_get_FE_region(lookup_node), &region);
		char *region_path = Cmiss_region_get_path(region);
		if (region_path)
		{
			display_message(INFORMATION_MESSAGE, "    region : %s\n", region_path);
			DEALLOCATE(region_path);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_nodal_lookup.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_nodal_lookup */

char *Computed_field_nodal_lookup::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field.
==============================================================================*/
{
	char *command_string, *field_name, node_id[10];
	int error,node_number;

	ENTER(Computed_field_time_nodal_lookup::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_nodal_lookup_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " region ", &error);
		Cmiss_region *region = NULL;
		FE_region_get_Cmiss_region(FE_node_get_FE_region(lookup_node), &region);
		char *region_path = Cmiss_region_get_path(region);
		if (region_path)
		{
			make_valid_token(&region_path);
			append_string(&command_string, region_path, &error);
			DEALLOCATE(region_path);
		}
		append_string(&command_string, " node ", &error);
    node_number = get_FE_node_identifier(lookup_node);
    sprintf(node_id,"%d",node_number);
    append_string(&command_string, " ", &error);
    append_string(&command_string, node_id, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_nodal_lookup::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_nodal_lookup::get_command_string */

int Computed_field_nodal_lookup::has_multiple_times ()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Always has multiple times.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_nodal_lookup::has_multiple_times);
	if (field)
	{
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_nodal_lookup::has_multiple_times.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_value::has_multiple_times */

void Computed_field_nodal_lookup_FE_region_change(struct FE_region *fe_region,
	struct FE_region_changes *changes, void *computed_field_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the node we are looking at changes generate a computed field change message.
==============================================================================*/
{
	Computed_field *field;
	Computed_field_nodal_lookup *core;
	int node_change;

	ENTER(Computed_field_FE_region_change);
	USE_PARAMETER(fe_region);
	if (changes && (field = (struct Computed_field *)computed_field_void) &&
		(core = dynamic_cast<Computed_field_nodal_lookup*>(field->core)))
	{
		/* I'm not sure if we could also check if we depend on an FE_field change
			and so reduce the total number of changes? */
		if (field->manager && CHANGE_LOG_QUERY(FE_node)(changes->fe_node_changes,
				core->lookup_node, &node_change))
		{
			if (node_change | CHANGE_LOG_OBJECT_CHANGED(FE_node))
			{
				Computed_field_dependency_changed(field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_FE_region_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Computed_field_FE_region_change */

} //namespace

struct Computed_field *Computed_field_create_nodal_lookup(
	struct Cmiss_field_module *field_module,
	struct Computed_field *source_field, struct FE_node *lookup_node) 
{
	Computed_field *field = NULL;
	if (source_field && lookup_node)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodal_lookup(lookup_node));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_nodal_lookup.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_nodal_lookup(struct Computed_field *field,
  struct Computed_field **source_field, struct FE_node **lookup_node)
{
	Computed_field_nodal_lookup* core;
	int return_code = 0;

	ENTER(Computed_field_get_type_nodal_lookup);
	if (field && (NULL != (core =
		dynamic_cast<Computed_field_nodal_lookup*>(field->core))) &&
		source_field && lookup_node)
	{
		*source_field = field->source_fields[0];
		*lookup_node = core->lookup_node;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_nodal_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_nodal_lookup */

int define_Computed_field_type_nodal_lookup(struct Parse_state *state,
	void *field_modify_void, void *computed_field_lookup_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_NODAL_LOOKUP (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field *source_field;
	Computed_field_lookup_package *computed_field_lookup_package;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;
  int node_identifier;
  char node_flag, *region_path;
  struct Cmiss_region *region;

	ENTER(define_Computed_field_type_nodal_lookup);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void) &&
		(computed_field_lookup_package=
		(Computed_field_lookup_package *)
		computed_field_lookup_package_void))
	{
		return_code = 1;
		source_field = (struct Computed_field *)NULL;
		region = NULL;
		node_flag = 0;
		node_identifier = 0;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_nodal_lookup_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			struct FE_node *node = NULL;
			return_code = Computed_field_get_type_nodal_lookup(field_modify->get_field(), 
				&source_field, &node);
			if (return_code)
			{
				node_identifier = get_FE_node_identifier(node);
				FE_region_get_Cmiss_region(FE_node_get_FE_region(node), &region);
			}
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			if (region)
			{
				region_path = Cmiss_region_get_path(region);
			}
			else
			{
				region_path = Cmiss_region_get_root_region_path();
			}
			option_table = CREATE(Option_table)();
			/* source field */
			set_source_field_data.computed_field_manager=
				field_modify->get_field_manager();
			set_source_field_data.conditional_function=
				Computed_field_has_numerical_components;
			set_source_field_data.conditional_function_user_data=(void *)NULL;
			Option_table_add_entry(option_table,"field", &source_field,
				&set_source_field_data,set_Computed_field_conditional);
			/* the node to nodal_lookup */
			Option_table_add_entry(option_table, "node", &node_identifier,
				&node_flag, set_int_and_char_flag);
			/* and the node region */
			Option_table_add_entry(option_table,"region",&region_path,
				 computed_field_lookup_package->root_region,set_Cmiss_region_path);
			/* process the option table */
			return_code=Option_table_multi_parse(option_table,state);
			/* no errors,not asking for help */
			if (return_code && node_flag)
			{
				if (Cmiss_region_get_region_from_path_deprecated(
							computed_field_lookup_package->root_region,region_path,
							&region))
				{
					struct FE_node *node = FE_region_get_FE_node_from_identifier(
						Cmiss_region_get_FE_region(region), node_identifier);
					if (node)
					{
						return_code = field_modify->update_field_and_deaccess(
							Computed_field_create_nodal_lookup(field_modify->get_field_module(),
								source_field, node));
					}
					else
					{
						/* error */
						display_message(ERROR_MESSAGE,
							"define_Computed_field_type_time_nodal_lookup.  "
							"Could not find node %d in region %s", node_identifier, region_path);
						return_code = 0;
					}
				}
				else
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_time_nodal_lookup.  Invalid region");
					return_code = 0;
				}
			}
			else
			{
				if ((!state->current_token)||
					(strcmp(PARSER_HELP_STRING,state->current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_time_nodal_lookup.  Failed");
				}
			}
			DESTROY(Option_table)(&option_table);
			DEALLOCATE(region_path);
		}
		REACCESS(Computed_field)(&source_field, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_nodal_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_nodal_lookup */

namespace {

void Computed_field_quaternion_SLERP_FE_region_change(struct FE_region *fe_region,
	 struct FE_region_changes *changes, void *computed_field_void);
/*******************************************************************************
LAST MODIFIED : 10 Oct 2007

DESCRIPTION :
If the node we are looking at changes generate a computed field change message.
==============================================================================*/

char computed_field_quaternion_SLERP_type_string[] = "quaternion_SLERP";

class Computed_field_quaternion_SLERP : public Computed_field_core
{
public:
	FE_node *nodal_lookup_node;

	Computed_field_quaternion_SLERP(
		FE_node *nodal_lookup_node) :
		Computed_field_core(),
		nodal_lookup_node(ACCESS(FE_node)(nodal_lookup_node))
	{
		normalised_t = 0;
		quaternion_component = NULL;
	};

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			if (FE_region_add_callback(FE_node_get_FE_region(nodal_lookup_node), 
				Computed_field_quaternion_SLERP_FE_region_change, 
				(void *)parent))
			{
				return true;
			}
		}
		return false;
	}
	 
	~Computed_field_quaternion_SLERP();
	 
private:

	 double *quaternion_component, old_w, old_x, old_y, old_z, w, x, y, z, normalised_t;

	 Computed_field_core *copy();

	 const char *get_type_string()
	 {
			return(computed_field_quaternion_SLERP_type_string);
	 }
	 
	int compare(Computed_field_core* other_field);

	int evaluate_cache_at_location(Field_location* location);

	int list();

	char* get_command_string();

	int is_defined_at_location(Field_location* location);

	int has_multiple_times();
};

Computed_field_quaternion_SLERP::~Computed_field_quaternion_SLERP()
/*******************************************************************************
LAST MODIFIED : 10 October 2007

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{
	ENTER(Computed_field_quaternion_SLERP::~Computed_field_quaternion_SLERP);
	if (field)
	{
		FE_region_remove_callback(FE_node_get_FE_region(nodal_lookup_node),
			Computed_field_quaternion_SLERP_FE_region_change,
			(void *)field);
		if (nodal_lookup_node)
		{
			DEACCESS(FE_node)(&(nodal_lookup_node));
		}
		if (quaternion_component)
		{
			DEALLOCATE(quaternion_component);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_quaternion_SLERP::~Computed_field_quaternion_SLERP.  "
			"Invalid argument(s)");
	}
	LEAVE;
} /* Computed_field_quaternionSLERP::~Computed_field_quaternion_SLERP */

Computed_field_core* Computed_field_quaternion_SLERP::copy()
/*******************************************************************************
LAST MODIFIED : 18 October 2007

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	Computed_field_quaternion_SLERP* core = new Computed_field_quaternion_SLERP(
		nodal_lookup_node);

	return (core);
} /* Computed_field_quaternion_SLERP::copy */

int Computed_field_quaternion_SLERP::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2007

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_quaternion_SLERP* other;
	int return_code;

	ENTER(Computed_field_quaternion_SLERP::compare);
	if (field && (other = dynamic_cast<Computed_field_quaternion_SLERP*>(other_core)))
	{
		if (nodal_lookup_node == other->nodal_lookup_node)
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
} /* Computed_field_quaternion_SLERP::compare */

int Computed_field_quaternion_SLERP::is_defined_at_location(Field_location* location)
/*******************************************************************************
LAST MODIFIED : 10 Oct 2007

DESCRIPTION :
Returns true if <field> can be calculated in <element>, which is determined
by checking the source field on the lookup node.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_quaternion_SLERP::is_defined_at_location);
	if (field && location)
	{
		Field_node_location nodal_location(nodal_lookup_node,
			location->get_time());

		return_code = Computed_field_is_defined_at_location(field->source_fields[0],
			&nodal_location);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_quaternion_SLERP::is_defined_at_location.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_quaternion_SLERP::is_defined_at_location */

int Computed_field_quaternion_SLERP::evaluate_cache_at_location(
	 Field_location* location)
/*******************************************************************************
LAST MODIFIED : 5 October 2007

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	 int i, return_code = 0, *time_index_one, *time_index_two;
	 double time;
	 FE_time_sequence *time_sequence;
	 FE_value *xi, *lower_time, *upper_time; 

	 ENTER(Computed_field_quaternion_SLERP::evaluate_cache_at_location);
	 if (field && location && field->number_of_components == 4 &&
			field->source_fields[0]->number_of_components == 4)
	 {
			time = location->get_time();
			//t is the normalised time scaled from 0 to 1
			if ((ALLOCATE(time_index_one, int, 1)) &&
				 (ALLOCATE(time_index_two, int, 1)) &&
				 (ALLOCATE(xi, FE_value, 1)) &&
				 (ALLOCATE(upper_time, FE_value, 1)) &&
				 (ALLOCATE(lower_time, FE_value, 1)))
			{
				 time_sequence = Computed_field_get_FE_node_field_FE_time_sequence(field->source_fields[0],
						nodal_lookup_node);
				 if (time_sequence)
				 {
						FE_time_sequence_get_interpolation_for_time(
							 time_sequence, time, time_index_one,
							 time_index_two, xi);
						FE_time_sequence_get_time_for_index(
							 time_sequence, *time_index_one, lower_time);
						FE_time_sequence_get_time_for_index(
							 time_sequence, *time_index_two, upper_time);
						normalised_t = *xi;
						// get the starting quaternion
						Field_node_location old_location(nodal_lookup_node, *lower_time);
						Computed_field_evaluate_source_fields_cache_at_location(field, &old_location);
						old_w = (double)(field->source_fields[0]->values[0]);
						old_x = (double)(field->source_fields[0]->values[1]);
						old_y = (double)(field->source_fields[0]->values[2]);
						old_z = (double)(field->source_fields[0]->values[3]);
						// get the last quaternion
						Field_node_location new_location(nodal_lookup_node, *upper_time);
						Computed_field_evaluate_source_fields_cache_at_location(field, &new_location);
						w = (double)(field->source_fields[0]->values[0]);
						x = (double)(field->source_fields[0]->values[1]);
						y = (double)(field->source_fields[0]->values[2]);
						z = (double)(field->source_fields[0]->values[3]);
						
						Quaternion *from= new Quaternion(old_w, old_x, old_y, old_z);
						Quaternion *to = new Quaternion(w, x, y, z);
						Quaternion *current = new Quaternion();
						from->normalise();
						to->normalise();
						current->interpolated_with_SLERP(*from, *to, normalised_t);
						
						if (!quaternion_component)
						{
							 ALLOCATE(quaternion_component, double, 4); 
						}
						if(quaternion_component)
						{
							 current->get(quaternion_component);
						}
						for (i=0; i<4;i++)
						{
							 field->values[i] =quaternion_component [i];
						}
						return_code = 1;
						DEALLOCATE(time_index_one);
						DEALLOCATE(time_index_two);
						DEALLOCATE(xi);
						DEALLOCATE(upper_time);
						DEALLOCATE(lower_time);
				 }
				 else
				 {
						display_message(ERROR_MESSAGE,
							 "Computed_field_quaternion::evaluate_cache_at_location.  "
							 "time sequence is missing.");
				 }
			}
			else
			{
				 display_message(ERROR_MESSAGE,
						"Computed_field_quaternion::evaluate_cache_at_location.  "
						"not enough memory.");
			}
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Computed_field_quaternion_SLERP::evaluate_cache_at_location.  "
				 "Invalid argument(s)");
			return_code = 0;
	 }
	 LEAVE;
	 
	 return (return_code);
} /* Computed_field_quaternion_SLERP::evaluate_cache_at_location */

int Computed_field_quaternion_SLERP::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_quaternion_SLERP);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    node : %d\n",
			get_FE_node_identifier(nodal_lookup_node));
		Cmiss_region *region = NULL;
		FE_region_get_Cmiss_region(FE_node_get_FE_region(nodal_lookup_node), &region);
		char *region_path = Cmiss_region_get_path(region);
		if (region_path)
		{
			display_message(INFORMATION_MESSAGE, "    region : %s\n", region_path);
			DEALLOCATE(region_path);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_quaternion_SLERP.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_quaternion_SLERP */

char *Computed_field_quaternion_SLERP::get_command_string()
/*******************************************************************************
LAST MODIFIED : 5 October 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, node_id[10];
	int error, node_number;

	ENTER(Computed_field_quaternion_SLERP::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_quaternion_SLERP_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			 make_valid_token(&field_name);
			 append_string(&command_string, field_name, &error);
			 DEALLOCATE(field_name);
		}
		append_string(&command_string, " region ", &error);
		Cmiss_region *region = NULL;
		FE_region_get_Cmiss_region(FE_node_get_FE_region(nodal_lookup_node), &region);
		char *region_path = Cmiss_region_get_path(region);
		if (region_path)
		{
			make_valid_token(&region_path);
			append_string(&command_string, region_path, &error);
			DEALLOCATE(region_path);
		}
		append_string(&command_string, " node ", &error);
		node_number = get_FE_node_identifier(nodal_lookup_node);
		sprintf(node_id,"%d",node_number);
		append_string(&command_string, " ", &error);
		append_string(&command_string, node_id, &error);
	}
	else
	{
		 display_message(ERROR_MESSAGE,
				"Computed_field_quaternion_SLERP::get_command_string.  "
				"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_quaternion_SLERP::get_command_string */

int Computed_field_quaternion_SLERP::has_multiple_times()
/*******************************************************************************
LAST MODIFIED : 10 Oct 2007

DESCRIPTION :
Always has multiple times.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_quaternion_SLERP::has_multiple_times);
	if (field)
	{
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			 "Computed_field_quaternion_SLERP::has_multiple_times.  "
			 "Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_quaternion_SLERP::has_multiple_times */

void Computed_field_quaternion_SLERP_FE_region_change(struct FE_region *fe_region,
	 struct FE_region_changes *changes, void *computed_field_void)
/*******************************************************************************
LAST MODIFIED : 10 Oct 2007

DESCRIPTION :
If the node we are looking at changes generate a computed field change message.
==============================================================================*/
{
	Computed_field *field;
	Computed_field_quaternion_SLERP *core;
	int node_change;

	ENTER(Computed_field_FE_region_change);
	USE_PARAMETER(fe_region);
	if (changes && (field = (struct Computed_field *)computed_field_void) &&
		(core = dynamic_cast<Computed_field_quaternion_SLERP*>(field->core)))
	{
		/* I'm not sure if we could also check if we depend on an FE_field change
			and so reduce the total number of changes? */
		if (field->manager && CHANGE_LOG_QUERY(FE_node)(changes->fe_node_changes,
				core->nodal_lookup_node, &node_change))
		{
			if (node_change | CHANGE_LOG_OBJECT_CHANGED(FE_node))
			{
				Computed_field_dependency_changed(field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_FE_region_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Computed_field_FE_region_change */

} //namespace

/***************************************************************************//**
 * Create field that perfoms a quaternion SLERP interpolation where time
 * variation of quaternion parameters are held in a node.
 * <source_field> must have 4 components.
 */
struct Computed_field *Computed_field_create_quaternion_SLERP(
	struct Cmiss_field_module *field_module,
	 struct Computed_field *source_field, struct Cmiss_region *region,
	 int quaternion_SLERP_node_identifier) 
{
	Computed_field *field = NULL;
	FE_node *quaternion_SLERP_node = FE_region_get_FE_node_from_identifier(
		Cmiss_region_get_FE_region(region), quaternion_SLERP_node_identifier);
	if (source_field && (4 == source_field->number_of_components) &&
		region && quaternion_SLERP_node)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_quaternion_SLERP(quaternion_SLERP_node));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_quaternion_SLERP.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_quaternion_SLERP(struct Computed_field *field,
	 struct Computed_field **quaternion_SLERP_field,
	 struct FE_node **lookup_node)
/*******************************************************************************
LAST MODIFIED : 10 October 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_QUATERNION_SLERP, the function returns the source
<quaternion_SLERP_field>, <quaternion_SLERP_region>, and the <quaternion_SLERP_node_identifier>.
Note that nothing returned has been ACCESSed.
==============================================================================*/
{
	 Computed_field_quaternion_SLERP* core;
	 int return_code;

	ENTER(Computed_field_get_type_quatenions_SLERP);
	if (field && (core = dynamic_cast<Computed_field_quaternion_SLERP*>(field->core)))
	{
		 *quaternion_SLERP_field = field->source_fields[0];
		 *lookup_node = core->nodal_lookup_node;
		 return_code = 1;
	}
	else
	{
		 display_message(ERROR_MESSAGE,
				"Computed_field_get_type_nodal_lookup.  Invalid argument(s)");
		 return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_quaternion_SLERP */

int define_Computed_field_type_quaternion_SLERP(struct Parse_state *state,
	void *field_modify_void, void *computed_field_lookup_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type 'quaterions' (if it is not already) and allows its
contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field *source_field;
	Computed_field_lookup_package *computed_field_lookup_package;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;
	int node_identifier;
	char node_flag, *region_path;
	struct Cmiss_region *region;
	struct FE_node *lookup_node = NULL;
	
	ENTER(define_Computed_field_type_quaternion_SLERP);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void) &&
		 (computed_field_lookup_package=
				(Computed_field_lookup_package *)
				computed_field_lookup_package_void))
	{
		return_code=1;
		source_field = (struct Computed_field *)NULL;
		region = NULL;
		node_flag = 0;
		node_identifier = 0;
		if ((NULL != field_modify->get_field()) &&
		(computed_field_quaternion_SLERP_type_string ==
			Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code = Computed_field_get_type_quaternion_SLERP(field_modify->get_field(), 
				 &source_field, &lookup_node);
		}
		if (return_code)
		{
			if (lookup_node)
			{
				node_identifier = get_FE_node_identifier(lookup_node);
				FE_region_get_Cmiss_region(FE_node_get_FE_region(lookup_node), &region);
			}
			if (source_field)
			{
				 ACCESS(Computed_field)(source_field);
			}
			if (region)
			{
				region_path = Cmiss_region_get_path(region);
			}
			else
			{
				region_path = Cmiss_region_get_root_region_path();
			}
			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
				 "A 4 components quaternion field. The components of "
				 "the quaternion field are expected to be the w, x, y, z components"
				 "of a quaternion (4 components in total). The quaternion field  is"
				 "evaluated and interpolated using SLERP at a normalised time between two"
				 "quaternions (read in from the exnode generally). This quaternion field"
				 "can be convert to a matrix with quaternion_to_matrix field, the resulting"
				 "matrix can be used to create a smooth time dependent rotation for an object"
				 "using the quaternion_to_matrix field. This field must be define directly from"
				 "exnode file or from a matrix_to_quaternion field");
			set_source_field_data.computed_field_manager =
				field_modify->get_field_manager();
			set_source_field_data.conditional_function =
				 Computed_field_has_4_components;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			Option_table_add_entry(option_table, "field", &source_field,
				 &set_source_field_data, set_Computed_field_conditional);
			/* the node to nodal_lookup */
			Option_table_add_entry(option_table, "node", &node_identifier,
				 &node_flag, set_int_and_char_flag);
			/* and the node region */
			Option_table_add_entry(option_table,"region",&region_path,
				 computed_field_lookup_package->root_region,set_Cmiss_region_path);
			/* process the option table */
			return_code = Option_table_multi_parse(option_table, state);
			/* no errors, not asking for help */
			if (return_code && node_flag)
			{
				if (Cmiss_region_get_region_from_path_deprecated(
					computed_field_lookup_package->root_region, region_path, &region))
				{
					return_code = field_modify->update_field_and_deaccess(
						Computed_field_create_quaternion_SLERP(field_modify->get_field_module(),
							 source_field, region, node_identifier));
				}
				else
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_quaternion_SLERP.  Invalid region");
					return_code = 0;
				}
			}
			else
			{
				 if ((!state->current_token)||
						(strcmp(PARSER_HELP_STRING,state->current_token)&&
							 strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
				 {
						/* error */
						display_message(ERROR_MESSAGE,
							 "define_Computed_field_type_quaternion_SLERP.  Failed");
				 }
			}
			DESTROY(Option_table)(&option_table);
			DEALLOCATE(region_path);
		}
		REACCESS(Computed_field)(&source_field, NULL);
	}
	else
	{
		 display_message(ERROR_MESSAGE,
				"define_Computed_field_type_quaternion_SLERP.  Invalid argument(s)");
		 return_code = 0;
	}
	LEAVE;
	
	return (return_code);
} /* define_Computed_field_type_quaternion_SLERP */

int Computed_field_register_types_lookup(
	struct Computed_field_package *computed_field_package,
	struct Cmiss_region *root_region)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	Computed_field_lookup_package
		*computed_field_lookup_package =
		new Computed_field_lookup_package;

	ENTER(Computed_field_register_types_lookup);
	if (computed_field_package)
	{
		computed_field_lookup_package->root_region = root_region;
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_nodal_lookup_type_string,
			define_Computed_field_type_nodal_lookup,
			computed_field_lookup_package);
		return_code = Computed_field_package_add_type(computed_field_package,
			 computed_field_quaternion_SLERP_type_string,
			 define_Computed_field_type_quaternion_SLERP,
			 computed_field_lookup_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_nodal_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_lookup */

