/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>

#include "general/debug.h"
#include "general/message.h"
#include "command/parser.h"
#include "graphics/element_point_ranges.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_discretization.h"
#include "general/multi_range_app.h"

int set_Element_point_ranges(struct Parse_state *state,
	void *element_point_ranges_address_void, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
Modifier function to set an element_point_ranges. <element_point_ranges_address>
should point to a currently-NULL pointer to a struct Element_point_ranges. Upon
successful return an Element_point_ranges will be created and the pointer to it
returned in this location, for the calling function to use or destroy.
==============================================================================*/
{
	const char *current_token;
	const char **valid_strings,*sample_mode_string;
	enum cmzn_element_point_sampling_mode sampling_mode;
	float xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int dimension, i, number_of_xi_points, number_of_valid_strings, return_code,
		start, stop;
	struct Element_point_ranges *element_point_ranges,
		**element_point_ranges_address;
	struct Element_point_ranges_identifier element_point_ranges_identifier;
	struct FE_region *fe_region;
	struct Option_table *option_table;

	ENTER(set_Element_point_ranges);
	if (state&&(element_point_ranges_address=
		(struct Element_point_ranges **)element_point_ranges_address_void)&&
		((struct Element_point_ranges *)NULL == *element_point_ranges_address)&&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		element_point_ranges_identifier.element=(struct FE_element *)NULL;
		element_point_ranges_identifier.top_level_element=(struct FE_element *)NULL;
		element_point_ranges_identifier.sampling_mode=
			CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION;
		for (i=0;i<MAXIMUM_ELEMENT_XI_DIMENSIONS;i++)
		{
			element_point_ranges_identifier.exact_xi[i]=xi[i]=0.5;
		}
		return_code=1;
		current_token=state->current_token;
		if (current_token)
		{
			if (strcmp(PARSER_HELP_STRING,current_token)&&
				strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
			{
				int element_dimension = 0;
				/* element type */
				if (fuzzy_string_compare(current_token,"element"))
				{
					element_dimension = 3;
				}
				else if (fuzzy_string_compare(current_token,"face"))
				{
					element_dimension = 2;
				}
				else if (fuzzy_string_compare(current_token,"line"))
				{
					element_dimension = 1;
				}
				else
				{
					display_message(WARNING_MESSAGE,"Missing element|face|line");
					display_parse_state_location(state);
					return_code=0;
				}
				/* element number */
				if (return_code)
				{
					shift_Parse_state(state,1);
					current_token=state->current_token;
					if (current_token)
					{
						if (strcmp(PARSER_HELP_STRING,current_token)&&
							strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
						{
							FE_mesh *fe_mesh = FE_region_find_FE_mesh_by_dimension(fe_region, element_dimension);
							const int identifier = atoi(current_token);
							element_point_ranges_identifier.element = fe_mesh->findElementByIdentifier(identifier);
							if (element_point_ranges_identifier.element)
							{
								shift_Parse_state(state,1);
							}
							else
							{
								display_message(ERROR_MESSAGE, "Unknown %d-D element: %d", element_dimension, identifier);
								display_parse_state_location(state);
								return_code=0;
							}
						}
						else
						{
							display_message(INFORMATION_MESSAGE," NUMBER");
							return_code=0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"Missing element number");
						display_parse_state_location(state);
						return_code=0;
					}
				}
				/* top_level_element number */
				if (return_code)
				{
					current_token=state->current_token;
					if (current_token&&
						fuzzy_string_compare(current_token,"top_level_element"))
					{
						shift_Parse_state(state,1);
						current_token=state->current_token;
						if (current_token)
						{
							if (strcmp(PARSER_HELP_STRING,current_token)&&
								strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
							{
								const int identifier = atoi(current_token);
								shift_Parse_state(state,1);
								element_point_ranges_identifier.top_level_element =
									FE_region_get_top_level_FE_element_from_identifier(fe_region, identifier);
								if (element_point_ranges_identifier.top_level_element)
								{
									if (!FE_element_is_top_level_parent_of_element(
										element_point_ranges_identifier.top_level_element,
										(void *)element_point_ranges_identifier.element))
									{
										display_message(ERROR_MESSAGE, "Invalid top_level_element: %d", identifier);
										return_code=0;
									}
								}
								else
								{
									display_message(WARNING_MESSAGE, "Unknown top_level_element: %d", identifier);
									display_parse_state_location(state);
									return_code=0;
								}
							}
							else
							{
								display_message(INFORMATION_MESSAGE," NUMBER");
								return_code=0;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"Missing top_level_element number");
							display_parse_state_location(state);
							return_code=0;
						}
					}
					else
					{
						if (!(element_point_ranges_identifier.top_level_element =
							FE_region_get_first_FE_element_that(fe_region,
								FE_element_is_top_level_parent_of_element,
								(void *)element_point_ranges_identifier.element)))
						{
							display_message(ERROR_MESSAGE,"No top_level_element");
							return_code=0;
						}
					}
				}
				/* sampling_mode */
				if (return_code)
				{
					option_table = CREATE(Option_table)();
					sampling_mode = element_point_ranges_identifier.sampling_mode;
					sample_mode_string =
						ENUMERATOR_STRING(cmzn_element_point_sampling_mode)(sampling_mode);
					valid_strings=
						cmzn_element_point_sampling_mode_get_valid_strings_for_Element_point_ranges(
							&number_of_valid_strings);
					Option_table_add_enumerator(option_table,number_of_valid_strings,
						valid_strings,&sample_mode_string);
					DEALLOCATE(valid_strings);
					return_code=Option_table_parse(option_table,state);
					if (return_code)
					{
						STRING_TO_ENUMERATOR(cmzn_element_point_sampling_mode)(
							sample_mode_string, &sampling_mode);
						element_point_ranges_identifier.sampling_mode = sampling_mode;
					}
					DESTROY(Option_table)(&option_table);
				}
				if (return_code)
				{
					dimension=
						get_FE_element_dimension(element_point_ranges_identifier.element);
					for (i=0;i<dimension;i++)
					{
						element_point_ranges_identifier.number_in_xi[i]=1;
					}
					switch (element_point_ranges_identifier.sampling_mode)
					{
						case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
						case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
						{
							/* number_in_xi */
							return_code=set_int_vector(state,
								(void *)element_point_ranges_identifier.number_in_xi,
									(void *)&dimension);
							if (return_code)
							{
								/* check number_in_xi are all > 0 */
								if ((!FE_element_get_xi_points(
									element_point_ranges_identifier.element,
									element_point_ranges_identifier.sampling_mode,
									element_point_ranges_identifier.number_in_xi,
									element_point_ranges_identifier.exact_xi,
									(cmzn_fieldcache_id)0,
									/*coordinate_field*/(struct Computed_field *)NULL,
									/*density_field*/(struct Computed_field *)NULL,
									&number_of_xi_points,
									/*xi_points_address*/(FE_value_triple **)NULL)) ||
									(1 > number_of_xi_points))
								{
									display_message(WARNING_MESSAGE, "Invalid number in xi");
									display_parse_state_location(state);
									return_code = 0;
								}
							}
						} break;
						case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
						{
							/* xi */
							return_code=set_float_vector(state,(void *)xi,(void *)&dimension);
							for (i=0;i<dimension;i++)
							{
								element_point_ranges_identifier.exact_xi[i]=xi[i];
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"set_Element_point_ranges.  Invalid sampling_mode");
							return_code=0;
						}
					}
				}
				if (return_code)
				{
					/* create the element_point_ranges */
					element_point_ranges=CREATE(Element_point_ranges)(
						&element_point_ranges_identifier);
					if (element_point_ranges)
					{
						switch (element_point_ranges_identifier.sampling_mode)
						{
							case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
							case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
							{
								/* ranges */
								if (set_Multi_range(state,
									(void *)(Element_point_ranges_get_ranges(element_point_ranges)),(void *)NULL))
								{
									if (!((0<Multi_range_get_number_of_ranges(
										Element_point_ranges_get_ranges(element_point_ranges)))&&
										(!Multi_range_get_last_start_value(
											Element_point_ranges_get_ranges(element_point_ranges),0,&start))&&
										(!Multi_range_get_next_stop_value(
											Element_point_ranges_get_ranges(element_point_ranges),number_of_xi_points-1,
											&stop))))
									{
										display_message(WARNING_MESSAGE,"Invalid ranges");
										display_parse_state_location(state);
										DESTROY(Element_point_ranges)(&element_point_ranges);
										return_code=0;
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"set_Element_point_ranges.  Could not build ranges");
									DESTROY(Element_point_ranges)(&element_point_ranges);
									return_code=0;
								}
							} break;
							case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
							{
								if (!Multi_range_add_range(Element_point_ranges_get_ranges(element_point_ranges),0,0))
								{
									display_message(ERROR_MESSAGE,
										"set_Element_point_ranges.  Could not add exact_xi point");
									DESTROY(Element_point_ranges)(&element_point_ranges);
									return_code=0;
								}
							} break;
							default:
							{
							} break;
						}
						if (element_point_ranges)
						{
							*element_point_ranges_address=element_point_ranges;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"set_Element_point_ranges.  Could not create object");
						return_code=0;
					}
				}
			}
			else
			{
				display_message(INFORMATION_MESSAGE," element|face|line # {"
					"cell_centres|cell_corners #xi1 #xi2.. #xiN #,#..#,# etc. | "
					"exact_xi xi1 xi2...}");
				return_code=0;
			}
		}
		else
		{
			display_message(WARNING_MESSAGE,"Missing element|face|line");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_Element_point_ranges.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_Element_point_ranges */
