/*******************************************************************************
FILE : computed_field_meanImageFilter.c

LAST MODIFIED : 9 September 2006

DESCRIPTION :
Wraps itk::MeanImageFilter
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
#include "computed_field/computed_field.h"
}
#include "computed_field/computed_field_private.hpp"
#include "image_processing/computed_field_ImageFilter.hpp"
extern "C" {
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "user_interface/message.h"
#include "image_processing/computed_field_meanImageFilter.h"
}
#include "itkImage.h"
#include "itkVector.h"
#include "itkMeanImageFilter.h"

using namespace CMISS;

namespace {

char computed_field_meanImageFilter_type_string[] = "mean_filter";

class Computed_field_meanImageFilter : public Computed_field_ImageFilter
{

public:
	int *radius_sizes;

	Computed_field_meanImageFilter(Computed_field *field,
		int *radius_sizes_in) : Computed_field_ImageFilter(field)
	{
		int i;

		radius_sizes = new int[dimension];
		for (i = 0 ; i < dimension ; i++)
		{
			radius_sizes[i] = radius_sizes_in[i];
		}
	};

	~Computed_field_meanImageFilter()
	{
		if (radius_sizes)
		{
			delete radius_sizes;
		}
	};

private:
	Computed_field_core *copy(Computed_field* new_parent)
	{
		return new Computed_field_meanImageFilter(new_parent, radius_sizes);
	}

	char *get_type_string()
	{
		return(computed_field_meanImageFilter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate_cache_at_location(Field_location* location);

	int list();

	char* get_command_string();

	template < class ImageType >
	int set_filter(Field_location* location);

	template < class ComputedFieldFilter >
	friend int Computed_field_ImageFilter::select_filter(
		ComputedFieldFilter *filter_class, Field_location* location);
};

int Computed_field_meanImageFilter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_meanImageFilter* other;
	int i, return_code;

	ENTER(Computed_field_meanImageFilter::compare);
	if (field && (other = dynamic_cast<Computed_field_meanImageFilter*>(other_core)))
	{
		if (dimension == other->dimension)
		{
			return_code = 1;
			for (i = 0 ; return_code && (i < dimension) ; i++)
			{
				if (radius_sizes[i] != other->radius_sizes[i])
				{
					return_code = 0;
				}
			}
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
} /* Computed_field_meanImageFilter::compare */

template < class ImageType >
int Computed_field_meanImageFilter::set_filter(Field_location* location)
/*******************************************************************************
LAST MODIFIED : 7 September 2006

DESCRIPTION :
Set the filter to type meanImageFilter and set the radius from the computed field
values
==============================================================================*/
{
	int i, return_code;
	
	typedef itk::MeanImageFilter< ImageType , ImageType > FilterType;

	typename FilterType::Pointer filter = FilterType::New();
	typename FilterType::InputSizeType radius;
		
	for (i = 0 ; i < dimension ; i++)
	{
		radius[i] = radius_sizes[i];
	}
	filter->SetRadius( radius );
	
	return_code = update_output< ImageType, FilterType >
		(location, filter);

	return (return_code);
} /* set_filter */

int Computed_field_meanImageFilter::evaluate_cache_at_location(
    Field_location* location)
/*******************************************************************************
LAST MODIFIED : 7 September 2006

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_meanImageFilter::evaluate_cache_at_location);
	if (field && location)
	{
		return_code = select_filter(this, location);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_meanImageFilter::evaluate_cache_at_location.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_meanImageFilter::evaluate_cache_at_location */

int Computed_field_meanImageFilter::list()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int i, return_code;

	ENTER(List_Computed_field_meanImageFilter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    filter radii :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE, " %d", radius_sizes[i]);
		}
		display_message(INFORMATION_MESSAGE, "\n");		
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_meanImageFilter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_meanImageFilter */

char *Computed_field_meanImageFilter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error, i;

	ENTER(Computed_field_meanImageFilter::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, get_type_string(), &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " radius_sizes", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %d", radius_sizes[i]);
			append_string(&command_string, temp_string, &error);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_meanImageFilter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_meanImageFilter::get_command_string */

} //namespace

int Computed_field_set_type_meanImageFilter(struct Computed_field *field,
	struct Computed_field *source_field, int *radius_sizes)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_MEANIMAGEFILTER.  The <radius_sizes> is
a vector of integers of dimension specified by the <source_field> dimension.
==============================================================================*/
{
	int number_of_source_fields, return_code;
	struct Computed_field **source_fields;

	ENTER(Computed_field_set_type_meanImageFilter);
	if (field && source_field &&
		Computed_field_is_scalar(source_field, (void *)NULL))
	{
		return_code = 1;
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_source_fields = 1;
		if (ALLOCATE(source_fields, struct Computed_field *,
			number_of_source_fields))
		{
			/* 2. free current type-specific data */
			Computed_field_clear_type(field);
			/* 3. establish the new type */
			field->number_of_components = source_field->number_of_components;
			source_fields[0] = ACCESS(Computed_field)(source_field);
			field->source_fields = source_fields;
			field->number_of_source_fields = number_of_source_fields;			
			field->core = new Computed_field_meanImageFilter(field, radius_sizes);
		}
		else
		{
			DEALLOCATE(source_fields);
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_set_type_meanImageFilter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_set_type_meanImageFilter */

int Computed_field_get_type_meanImageFilter(struct Computed_field *field,
	struct Computed_field **source_field, int **radius_sizes)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_MEANIMAGEFILTER, the source_field and meanImageFilter
used by it are returned - otherwise an error is reported.
==============================================================================*/
{
	Computed_field_meanImageFilter* core;
	int i, return_code;

	ENTER(Computed_field_get_type_meanImageFilter);
	if (field && (core = dynamic_cast<Computed_field_meanImageFilter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		ALLOCATE(*radius_sizes, int, core->dimension);
		for (i = 0 ; i < core->dimension ; i++)
		{
			(*radius_sizes)[i] = core->radius_sizes[i];
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_meanImageFilter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_meanImageFilter */

int define_Computed_field_type_meanImageFilter(struct Parse_state *state,
	void *field_void, void *computed_field_package_void)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_MEANIMAGEFILTER (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	int dimension, i, old_dimension, previous_state_index, *radius_sizes,
		return_code, *sizes;
	struct Computed_field *field, *source_field, *texture_coordinate_field;
	struct Computed_field_package *computed_field_package;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_meanImageFilter);
	if (state && (field = (struct Computed_field *)field_void) &&
		(computed_field_package = (Computed_field_package*)computed_field_package_void))
	{
		return_code = 1;
		/* get valid parameters for projection field */
		source_field = (struct Computed_field *)NULL;
		radius_sizes = (int *)NULL;
		old_dimension = 0;
		if (computed_field_meanImageFilter_type_string ==
			Computed_field_get_type_string(field))
		{
			return_code =
				Computed_field_get_type_meanImageFilter(field, &source_field,
					&radius_sizes);
			return_code = Computed_field_get_native_resolution(source_field,
				&old_dimension, &sizes, &texture_coordinate_field);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}

			if ((!state->current_token) ||
				(strcmp(PARSER_HELP_STRING, state->current_token)&&
					strcmp(PARSER_RECURSIVE_HELP_STRING, state->current_token)))
			{

				previous_state_index = state->current_index;

				/* Discover the source field */
				option_table = CREATE(Option_table)();
				/* field */
				set_source_field_data.computed_field_manager =
					Computed_field_package_get_computed_field_manager(computed_field_package);
				set_source_field_data.conditional_function = Computed_field_has_numerical_components;
				set_source_field_data.conditional_function_user_data = (void *)NULL;
				Option_table_add_entry(option_table, "field", &source_field,
					&set_source_field_data, set_Computed_field_conditional);
				Option_table_add_entry(option_table, (char*)NULL, NULL, NULL, set_nothing);
				return_code = Option_table_multi_parse(option_table, state);
				DESTROY(Option_table)(&option_table);
				/* Return back to where we were */
				shift_Parse_state(state, previous_state_index - state->current_index);
				/* no errors,not asking for help */
				if (return_code)
				{
					if (!source_field)
					{
						display_message(ERROR_MESSAGE,
							"define_Computed_field_type_meanImageFilter.  "
							"Missing source field");
						return_code = 0;
					}
				}
				if (return_code)
				{
					return_code = Computed_field_get_native_resolution(source_field,
						&dimension, &sizes, &texture_coordinate_field);
				
					if (!radius_sizes || (old_dimension != dimension))
					{
						REALLOCATE(radius_sizes, radius_sizes, int, dimension);
						for (i = old_dimension ; i < dimension ; i++)
						{
							radius_sizes[i] = 1;
						}
					}
					/* Read the radius sizes */
					option_table = CREATE(Option_table)();
					/* radius_sizes */
					Option_table_add_int_vector_entry(option_table,
						"radius_sizes", radius_sizes, &dimension);
					Option_table_add_entry(option_table, (char*)NULL, NULL, NULL, set_nothing);
					return_code = Option_table_multi_parse(option_table, state);
					DESTROY(Option_table)(&option_table);
				}
				if (return_code)
				{
					return_code = Computed_field_set_type_meanImageFilter(
						field, source_field, radius_sizes);

				}

				if (!return_code)
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_meanImageFilter.  Failed");
				}
			}
			else
			{
				/* Write help */
					option_table = CREATE(Option_table)();
					/* field */
					set_source_field_data.computed_field_manager =
						Computed_field_package_get_computed_field_manager(computed_field_package);
					set_source_field_data.conditional_function = Computed_field_is_scalar;
					set_source_field_data.conditional_function_user_data = (void *)NULL;
					Option_table_add_entry(option_table, "field", &source_field,
						&set_source_field_data, set_Computed_field_conditional);
					/* radius_sizes */
					Option_table_add_int_vector_entry(option_table,
						"radius_sizes", radius_sizes, &dimension);
					return_code = Option_table_multi_parse(option_table, state);
					DESTROY(Option_table)(&option_table);
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
			if (radius_sizes)
			{
				DEALLOCATE(radius_sizes);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_meanImageFilter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_meanImageFilter */

int Computed_field_register_types_meanImageFilter(
	struct Computed_field_package *computed_field_package)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_register_types_meanImageFilter);
	if (computed_field_package)
	{
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_meanImageFilter_type_string, 
			define_Computed_field_type_meanImageFilter,
			computed_field_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_meanImageFilter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_meanImageFilter */
