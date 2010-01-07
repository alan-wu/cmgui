/*******************************************************************************
FILE : computed_field_canny_edge_detection_image_filter.c

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
#include "image_processing/computed_field_cannyEdgeDetectionFilter.h"
}
#include "itkImage.h"
#include "itkVector.h"
#include "itkCannyEdgeDetectionImageFilter.h"

using namespace CMISS;

namespace {

char computed_field_canny_edge_detection_image_filter_type_string[] = "canny_edge_detection_filter";

class Computed_field_canny_edge_detection_image_filter : public Computed_field_ImageFilter
{

public:

  double variance;
  double maximumError;
  double upperThreshold;
  double lowerThreshold;
  //  double outsideValue;

  Computed_field_canny_edge_detection_image_filter(Computed_field *source_field,
  	double variance,  double maximumError, double upperThreshold, double lowerThreshold);

	~Computed_field_canny_edge_detection_image_filter()
	{
	}

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
	  return new Computed_field_canny_edge_detection_image_filter(field->source_fields[0],
	  	variance, maximumError, upperThreshold, lowerThreshold);
	}

	char *get_type_string()
	{
		return(computed_field_canny_edge_detection_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_canny_edge_detection_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter* other;
	int return_code;

	ENTER(Computed_field_canny_edge_detection_image_filter::compare);

	// check field exists and can be cast to correct type
	if (field && (other = dynamic_cast<Computed_field_canny_edge_detection_image_filter*>(other_core))) {
		if( variance == other->variance &&
		    maximumError == other->maximumError &&
		    upperThreshold == other->upperThreshold &&
		    lowerThreshold == other->lowerThreshold) {
		  return_code = 1;  // parameters match, field is the same
		} else {
			return_code = 0;
		}
	} else {
		return_code = 0;		
	}

	LEAVE;

	return (return_code);
} /* Computed_field_canny_edge_detection_image_filter::compare */

int Computed_field_canny_edge_detection_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;

	ENTER(List_Computed_field_canny_edge_detection_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    variance : %g\n", variance);
		display_message(INFORMATION_MESSAGE,
			"    maximum_error : %g\n", maximumError);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upperThreshold);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lowerThreshold);

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_canny_edge_detection_image_filter */

char *Computed_field_canny_edge_detection_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_canny_edge_detection_image_filter::get_command_string);
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
		sprintf(temp_string, " variance %g", variance);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " maximum_error %g", maximumError);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " upper_threshold %g", upperThreshold);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " lower_threshold %g", lowerThreshold);
		append_string(&command_string, temp_string, &error);		

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_canny_edge_detection_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_canny_edge_detection_image_filter::get_command_string */

template < class ImageType >
class Computed_field_canny_edge_detection_image_filter_Functor :
	public Computed_field_ImageFilter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter *canny_edge_detection_image_filter;

public:

	Computed_field_canny_edge_detection_image_filter_Functor(
		Computed_field_canny_edge_detection_image_filter *canny_edge_detection_image_filter) :
		Computed_field_ImageFilter_FunctorTmpl< ImageType >(canny_edge_detection_image_filter),
		canny_edge_detection_image_filter(canny_edge_detection_image_filter)
	{
	}

	int set_filter(Field_location* location)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;
		
		typedef itk::CannyEdgeDetectionImageFilter< ImageType , ImageType > FilterType;
		typedef typename ImageType::PixelType  OutputImagePixelType;
		OutputImagePixelType upper;
		OutputImagePixelType lower;

		typename FilterType::Pointer filter = FilterType::New();
		
		// check whether we need to scale these values from
		// 0-1 to 0-255
		filter->SetVariance( (float)(canny_edge_detection_image_filter->variance) );
		filter->SetMaximumError( (float)(canny_edge_detection_image_filter->maximumError) );

		// wrong currently!
		upper = canny_edge_detection_image_filter->upperThreshold;
		filter->SetUpperThreshold(upper);
		lower = canny_edge_detection_image_filter->lowerThreshold;
		filter->SetLowerThreshold(lower);
	
		return_code = canny_edge_detection_image_filter->update_output_image
			(location, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));
	  
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_canny_edge_detection_image_filter_Functor */
	
Computed_field_canny_edge_detection_image_filter::Computed_field_canny_edge_detection_image_filter(
	Computed_field *source_field, double variance, double maximumError,
	double upperThreshold, double lowerThreshold) :
  Computed_field_ImageFilter(source_field), variance(variance), maximumError(maximumError),
  upperThreshold(upperThreshold), lowerThreshold(lowerThreshold)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create the computed_field representation of the CannyEdgeDetectionFilter.
==============================================================================*/
{
} /* Computed_field_canny_edge_detection_image_filter */

void Computed_field_canny_edge_detection_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_twoormoredimensions(
		Computed_field_canny_edge_detection_image_filter_Functor, this);
#else
	create_filters_singlecomponent_twoormoredimensions
		< Computed_field_canny_edge_detection_image_filter_Functor,
		Computed_field_canny_edge_detection_image_filter >
		(this);
#endif
}

} //namespace

struct Computed_field *Cmiss_field_module_create_canny_edge_detection_image_filter(
	struct Cmiss_field_module *field_module,
  struct Computed_field *source_field, double variance, double maximumError, 
  double upperThreshold, double lowerThreshold)
{
	Computed_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_canny_edge_detection_image_filter(source_field,
				variance, maximumError, upperThreshold, lowerThreshold));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_module_create_canny_edge_detection_image_filter.  Invalid argument(s)");
	}

	return (field);
}

int Cmiss_field_get_type_canny_edge_detection_image_filter(struct Computed_field *field,
      struct Computed_field **source_field, double *variance, double *maximumError,
      double *upperThreshold, double *lowerThreshold)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_CANNYEDGEDETECTIONFILTER, the source_field
used by it is returned - otherwise an error is reported.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter* core;
	int return_code;

	ENTER(Cmiss_field_get_type_canny_edge_detection_image_filter);
	if (field && (core = dynamic_cast<Computed_field_canny_edge_detection_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*variance = core->variance;
		*maximumError = core->maximumError;
		*upperThreshold = core->upperThreshold;
		*lowerThreshold = core->lowerThreshold;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_get_type_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_get_type_canny_edge_detection_image_filter */

int define_Computed_field_type_canny_edge_detection_image_filter(struct Parse_state *state,
	void *field_modify_void, void *computed_field_simple_package_void)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_CANNYEDGEDETECTIONFILTER (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	int return_code;
	double variance;
	double maximumError;
	double upperThreshold;
	double lowerThreshold;

	struct Computed_field *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_canny_edge_detection_image_filter);
	USE_PARAMETER(computed_field_simple_package_void);
	if (state && (field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code = 1;
		/* get valid parameters for projection field */
		source_field = (struct Computed_field *)NULL;
		variance = 0;
		maximumError = 0.01;
		upperThreshold = 0;
		lowerThreshold = 0;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_canny_edge_detection_image_filter_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code =
				Cmiss_field_get_type_canny_edge_detection_image_filter(field_modify->get_field(), &source_field, 
				  &variance, &maximumError, &upperThreshold, &lowerThreshold);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}

			/* Discover the source field */
			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
				"The canny_edge_detection field uses the itk::CannyEdgeDetectionImageFilter code to detect edges in a field. The <field> it operates on is usually a sample_texture field, based on a texture that has been created from image file(s).  Increasing the <variance> smooths the input image more, which reduces sensitivity to image noise at the expense of losing some detail. Decreasing the <maximum_error> also reduces edges detected as the result of noise.  The <upper_threshold> sets the level which a point must be above to use it as the start of the edge. The edge will then grow from that point until the level drops below the <lower_threshold>. Increasing the <upper_threshold> will decrease the number of edges detected. Increasing the <lower_threshold> will reduce the length of edges.  See a/testing/image_processing_2D for an example of using this field.");

			/* field */
			set_source_field_data.computed_field_manager =
				field_modify->get_field_manager();
			set_source_field_data.conditional_function = Computed_field_is_scalar;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			Option_table_add_entry(option_table, "field", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			/* variance */
			Option_table_add_double_entry(option_table, "variance",
				&variance);
			/* maximumError */
			Option_table_add_double_entry(option_table, "maximum_error",
				&maximumError);
			/* upperThreshold */
			Option_table_add_double_entry(option_table, "upper_threshold",
				&upperThreshold);
			/* lowerThreshold */
			Option_table_add_double_entry(option_table, "lower_threshold",
				&lowerThreshold);
			return_code = Option_table_multi_parse(option_table, state);
			DESTROY(Option_table)(&option_table);

			if (return_code)
			{
				if (!source_field)
				{
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_canny_edge_detection_image_filter.  "
						"Missing source field");
					return_code = 0;
				}
			}
			if (return_code)
			{
				return_code = field_modify->update_field_and_deaccess(
					Cmiss_field_module_create_canny_edge_detection_image_filter(
						field_modify->get_field_module(), source_field, variance,
						maximumError, upperThreshold, lowerThreshold));
			}

			if (!return_code)
			{
				if ((!state->current_token) ||
					(strcmp(PARSER_HELP_STRING, state->current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING, state->current_token)))
				{
						
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_canny_edge_detection_image_filter.  Failed");
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
			"define_Computed_field_type_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_canny_edge_detection_image_filter */

int Computed_field_register_types_canny_edge_detection_image_filter(
	struct Computed_field_package *computed_field_package)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_register_types_canny_edge_detection_image_filter);
	if (computed_field_package)
	{
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_canny_edge_detection_image_filter_type_string, 
			define_Computed_field_type_canny_edge_detection_image_filter,
			Computed_field_package_get_simple_package(computed_field_package));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_canny_edge_detection_image_filter */
