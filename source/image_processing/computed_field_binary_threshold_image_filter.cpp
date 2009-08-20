/*******************************************************************************
FILE : computed_field_binary_threshold_image_filter.cpp

LAST MODIFIED : 16 May 2008

DESCRIPTION :
Wraps itk::BinaryThresholdImageFilter
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
#include "image_processing/computed_field_binary_threshold_image_filter.h"
}
#include "itkImage.h"
#include "itkVector.h"
#include "itkBinaryThresholdImageFilter.h"

using namespace CMISS;

namespace {

char computed_field_binary_threshold_image_filter_type_string[] = "binary_threshold_filter";

class Computed_field_binary_threshold_image_filter : public Computed_field_ImageFilter
{

public:
	double lower_threshold;
	double upper_threshold;

	Computed_field_binary_threshold_image_filter(Computed_field *field,
		double lower_threshold, double upper_threshold);

	~Computed_field_binary_threshold_image_filter()
	{
	}

private:
	Computed_field_core *copy(Computed_field* new_parent)
	{
		return new Computed_field_binary_threshold_image_filter(new_parent,
			lower_threshold, upper_threshold);
	}

	char *get_type_string()
	{
		return(computed_field_binary_threshold_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

/*****************************************************************************//**
 * Compare the type specific data
 * 
 * @param other_core Core of other field to compare against
 * @return Return code indicating field is the same (1) or different (0)
*/
int Computed_field_binary_threshold_image_filter::compare(Computed_field_core *other_core)
{
	Computed_field_binary_threshold_image_filter* other;
	int return_code;

	ENTER(Computed_field_binary_threshold_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_binary_threshold_image_filter*>(other_core)))
	{
		if ((dimension == other->dimension)
			&& (lower_threshold == other->lower_threshold)
			&& (upper_threshold == other->upper_threshold))
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
} /* Computed_field_binary_threshold_image_filter::compare */

template < class ImageType >
class Computed_field_binary_threshold_image_filter_Functor :
	public Computed_field_ImageFilter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_binary_threshold_image_filter *binary_threshold_image_filter;

public:

	Computed_field_binary_threshold_image_filter_Functor(
		Computed_field_binary_threshold_image_filter *binary_threshold_image_filter) :
		Computed_field_ImageFilter_FunctorTmpl< ImageType >(binary_threshold_image_filter),
		binary_threshold_image_filter(binary_threshold_image_filter)
	{
	}

/*****************************************************************************//**
 * Create a filter of the correct type, set the filter specific parameters
 * and generate the output
 * 
 * @param location Field location
 * @return Return code indicating succes (1) or failure (0)
*/
	int set_filter(Field_location* location)
	{
		int return_code;
		
		typedef itk::BinaryThresholdImageFilter< ImageType , ImageType > FilterType;
		
		typename FilterType::Pointer filter = FilterType::New();
		
		filter->SetLowerThreshold( binary_threshold_image_filter->lower_threshold );
		filter->SetUpperThreshold( binary_threshold_image_filter->upper_threshold );
		
		return_code = binary_threshold_image_filter->update_output_image
			(location, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_binary_threshold_image_filter_Functor */


/*****************************************************************************//**
 * Constructor for a binary threshold image filter field
 * Creates the computed field representation of the binary threshold image filter
 * 
 * @param field Generic computed field
 * @param lower_threshold Lower threshold value (below which values will be set to 0)
 * @param upper_threshold Upper threshold value (above which values will be set to 0)
 * @return Return code indicating succes (1) or failure (0)
*/
Computed_field_binary_threshold_image_filter::Computed_field_binary_threshold_image_filter(
	Computed_field *field,
	double lower_threshold, double upper_threshold) :
	Computed_field_ImageFilter(field),
	lower_threshold(lower_threshold), upper_threshold(upper_threshold)
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_binary_threshold_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_binary_threshold_image_filter_Functor,
		Computed_field_binary_threshold_image_filter >
		(this);
#endif
}

/*****************************************************************************//**
 * List field parameters
 * 
 * @return Return code indicating succes (1) or failure (0)
*/
int Computed_field_binary_threshold_image_filter::list()
{
	int return_code = 0;

	ENTER(List_Computed_field_binary_threshold_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lower_threshold);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upper_threshold);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_binary_threshold_image_filter */

/*****************************************************************************//**
 * Returns allocated command string for reproducing field. Includes type.
 * 
 * @return Command string to create field
*/
char *Computed_field_binary_threshold_image_filter::get_command_string()
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_binary_threshold_image_filter::get_command_string);
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
		sprintf(temp_string, " lower_threshold %g", lower_threshold);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " upper_threshold %g", upper_threshold);	
		append_string(&command_string, temp_string, &error);		
}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_binary_threshold_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_binary_threshold_image_filter::get_command_string */

} //namespace

/*****************************************************************************//**
 * If field can be cast to a COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER do so
 * and return the field.  Otherwise return NULL.
 * 
 * @param field Id of the field to cast
 * @return Id of the cast field, or NULL
*/
Cmiss_field_binary_threshold_image_filter_id Cmiss_field_binary_threshold_image_filter_cast(Cmiss_field_id field)
{
	if (dynamic_cast<Computed_field_binary_threshold_image_filter*>(field->core))
	{
		return (reinterpret_cast<Cmiss_field_binary_threshold_image_filter_id>(field));
	}
	else
	{
		return (NULL);
	}
}

Computed_field *Cmiss_field_create_binary_threshold_image_filter(
	struct Computed_field *source_field, double lower_threshold,
	double upper_threshold)
{
	int number_of_source_fields;
	Computed_field *field, **source_fields;

	ENTER(Cmiss_field_create_binary_threshold_image_filter);
	if ( source_field &&
		Computed_field_is_scalar(source_field, (void *)NULL))
	{
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_source_fields = 1;
		if (ALLOCATE(source_fields, struct Computed_field *,
			number_of_source_fields))
		{
			/* 2. create new field */
			field = ACCESS(Computed_field)(CREATE(Computed_field)(""));
			/* 3. establish the new type */
			field->number_of_components = source_field->number_of_components;
			source_fields[0] = ACCESS(Computed_field)(source_field);
			field->source_fields = source_fields;
			field->number_of_source_fields = number_of_source_fields;			
			Computed_field_ImageFilter* filter_core = new Computed_field_binary_threshold_image_filter(field,
				lower_threshold, upper_threshold);
			if (filter_core->functor)
			{
				field->core = filter_core;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_create_binary_threshold_image_filter.  "
					"Unable to create image filter.");
				field = (Computed_field *)NULL;
			}
		}
		else
		{
			DEALLOCATE(source_fields);
			field = (Computed_field *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_binary_threshold_image_filter.  Invalid argument(s)");
		field = (Computed_field *)NULL;
	}
	LEAVE;

	return (field);
} /* Cmiss_field_create_binary_threshold_image_filter */

/*****************************************************************************//**
 * If the field is of type COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER, 
 * the source_field and thresholds used by it are returned - 
 * otherwise an error is reported.
 *
 * @return Return code indicating succes (1) or failure (0)
*/
int Computed_field_get_type_binary_threshold_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, double *lower_threshold,
	double *upper_threshold)
{
	Computed_field_binary_threshold_image_filter* core;
	int return_code;

	ENTER(Computed_field_get_type_binary_threshold_image_filter);
	if (field && (core = dynamic_cast<Computed_field_binary_threshold_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*lower_threshold = core->lower_threshold;
		*upper_threshold = core->upper_threshold;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_binary_threshold_image_filter */

/*****************************************************************************//**
 * Converts <field> into type COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER 
 * (if it is not already) and allows its contents to be modified.
 * 
 * @param state Parse state of field to define
 * @param field_modify_void
 * @param computed_field_simple_package_void
 * @return Return code indicating succes (1) or failure (0)
*/
int define_Computed_field_type_binary_threshold_image_filter(struct Parse_state *state,
	void *field_modify_void, void *computed_field_simple_package_void)
{
	double lower_threshold, upper_threshold;
	int return_code;
	struct Computed_field *field, *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_binary_threshold_image_filter);
	USE_PARAMETER(computed_field_simple_package_void);
	if (state && (field_modify=(Computed_field_modify_data *)field_modify_void) &&
			(field=field_modify->field))
	{
		return_code = 1;
		/* get valid parameters for projection field */
		source_field = (struct Computed_field *)NULL;
		lower_threshold = 0.0;
		upper_threshold = 1.0;
		if (computed_field_binary_threshold_image_filter_type_string ==
			Computed_field_get_type_string(field))
		{
			return_code =
				Computed_field_get_type_binary_threshold_image_filter(field, &source_field,
					&lower_threshold, &upper_threshold);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}

			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
				"The binary_threshold_filter field uses the itk::BinaryThresholdImageFilter code to produce an output field where each pixel has one of two values (either 0 or 1). It is useful for separating out regions of interest. The <field> it operates on is usually a sample_texture field, based on a texture that has been created from image file(s).  Pixels with an intensity range between <lower_threshold> and the <upper_threshold> are set to 1, the rest are set to 0. See a/testing/image_processing_2D for an example of using this field.  For more information see the itk software guide.");

			/* field */
			set_source_field_data.computed_field_manager =
				Cmiss_region_get_Computed_field_manager(field_modify->region);
			set_source_field_data.conditional_function = Computed_field_is_scalar;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			Option_table_add_entry(option_table, "field", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			/* lower_threshold */
			Option_table_add_double_entry(option_table, "lower_threshold",
				&lower_threshold);
			/* upper_threshold */
			Option_table_add_double_entry(option_table, "upper_threshold",
				&upper_threshold);

			return_code = Option_table_multi_parse(option_table, state);
			DESTROY(Option_table)(&option_table);

			/* no errors,not asking for help */
			if (return_code)
			{
				if (!source_field)
				{
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_binary_threshold_image_filter.  "
						"Missing source field");
					return_code = 0;
				}
			}
			if (return_code)
			{
				return_code = Computed_field_copy_type_specific_and_deaccess(field,
					Cmiss_field_create_binary_threshold_image_filter(
						source_field, lower_threshold, upper_threshold));			
			}
			
			if (!return_code)
			{
				if ((!state->current_token) ||
					(strcmp(PARSER_HELP_STRING, state->current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING, state->current_token)))
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_binary_threshold_image_filter.  Failed");
				}
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_binary_threshold_image_filter */

int Computed_field_register_types_binary_threshold_image_filter(
	struct Computed_field_package *computed_field_package)
{
	int return_code;

	ENTER(Computed_field_register_types_binary_threshold_image_filter);
	if (computed_field_package)
	{
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_binary_threshold_image_filter_type_string, 
			define_Computed_field_type_binary_threshold_image_filter,
			Computed_field_package_get_simple_package(computed_field_package));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_binary_threshold_image_filter */
