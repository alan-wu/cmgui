/*******************************************************************************
FILE : cmiss_field.c

LAST MODIFIED : 18 April 2008

DESCRIPTION :
The public interface to the Cmiss fields.
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
#include <stdlib.h>
#include <stdio.h>
#include "api/cmiss_field.h"
#include "api/cmiss_region.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "user_interface/message.h"

/*
Global functions
----------------
*/

Cmiss_field_id Cmiss_field_access(Cmiss_field_id field)
{
	return (ACCESS(Computed_field)(field));
}

int Cmiss_field_destroy(Cmiss_field_id *field_address)
{
	return (DEACCESS(Computed_field)(field_address));
}

Cmiss_field_module_id Cmiss_field_get_field_module(Cmiss_field_id field)
{
	struct Cmiss_region *region = Computed_field_get_region(field);
	return Cmiss_region_get_field_module(region);
}

int Cmiss_field_evaluate_at_node(struct Cmiss_field *field,
	struct Cmiss_node *node, double time, int number_of_values, double *values)
/*******************************************************************************
LAST MODIFIED : 29 March 2004

DESCRIPTION :
Returns the <values> of <field> at <node> and <time> if it is defined there.

The <values> array must be large enough to store as many floats as there are
number_of_components, the function checks that <number_of_values> is 
greater than or equal to the number of components.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_field_set_values_at_node);
	if (field && node && values &&
		(number_of_values >= Computed_field_get_number_of_components(field)))
	{
		return_code = Computed_field_evaluate_at_node(field, node, time, values);
		Computed_field_clear_cache(field);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_set_values_at_node.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_set_values_at_node */

int Cmiss_field_set_values_at_node(struct Cmiss_field *field,
	struct Cmiss_node *node, double time, int number_of_values, double *values)
/*******************************************************************************
LAST MODIFIED : 21 April 2005

DESCRIPTION :
Sets the <values> of the computed <field> at <node>. Only certain computed field
types allow their values to be set. Fields that deal directly with FE_fields eg.
FINITE_ELEMENT and NODE_VALUE fall into this category, as do the various
transformations, RC_COORDINATE, RC_VECTOR, OFFSET, SCALE, etc. which convert
the values into what they expect from their source field, and then call the same
function for it. If a field has more than one source field, eg. RC_VECTOR, it
can in many cases still choose which one is actually being changed, for example,
the 'vector' field in this case - coordinates should not change. This process
continues until the actual FE_field values at the node are changed or a field
is reached for which its calculation is not reversible, or is not supported yet.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_field_set_values_at_node);
	if (field && node && values &&
		(number_of_values >= Computed_field_get_number_of_components(field)))
	{
		return_code = Computed_field_set_values_at_node(field, node, time, values);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_set_values_at_node.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_set_values_at_node */

int Cmiss_field_evaluate_in_element(struct Cmiss_field *field,
	struct Cmiss_element *element, double *xi, double time, 
	struct Cmiss_element *top_level_element, int number_of_values,
	double *values, int number_of_derivatives, double *derivatives)
/*******************************************************************************
LAST MODIFIED : 29 March 2004

DESCRIPTION :
Returns the values and derivatives (if <derivatives> != NULL) of <field> at
<element>:<xi>, if it is defined over the element.

The optional <top_level_element> may be supplied for the benefit of this or
any source fields that may require calculation on it instead of a face or line.
FIBRE_AXES and GRADIENT are examples of such fields, since they require
top-level coordinate derivatives. The term "top_level" refers to an ultimate
parent element for the face or line, eg. the 3-D element parent to 2-D faces.
If no such top level element is supplied and one is required, then the first
available parent element will be chosen - if the user requires a top-level
element in the same group as the face or with the face on the correct side,
then they should supply the top_level_element here.

The <values> and <derivatives> arrays must be large enough to store all the
values and derivatives for this field in the given element, ie. values is
number_of_components in size, derivatives has the element dimension times the
number_of_components
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_field_evaluate_in_element);
	if (field && element && xi && values &&
		(number_of_values >= Computed_field_get_number_of_components(field))
		&& (!derivatives || (number_of_derivatives >= 
		Computed_field_get_number_of_components(field) * get_FE_element_dimension(element))))
	{
		return_code = Computed_field_evaluate_in_element(field, element, xi,
			time, top_level_element, values, derivatives);
		Computed_field_clear_cache(field);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_evaluate_in_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_evaluate_in_element */

char *Cmiss_field_evaluate_as_string_at_node(
	struct Cmiss_field *field, struct Cmiss_node *node, double time)
/*******************************************************************************
LAST MODIFIED : 17 January 2007

DESCRIPTION :
Returns a string describing the value/s of the <field> at the <node>. If the
field is based on an FE_field but not returning FE_values, it is asked to supply
the string. Otherwise, a string built up of comma separated values evaluated
for the field in field_evaluate_cache_at_node. The FE_value exception
is used since it is likely the values are already in the cache in most cases,
or can be used by other fields again if calculated now.
Creates a string which represents all the components.
Some basic field types such as CMISS_NUMBER have special uses in this function.
It is up to the calling function to DEALLOCATE the returned string.
==============================================================================*/
{
	char *return_code;

	ENTER(Cmiss_field_evaluate_as_string_at_node);
	if (field && node)
	{
		return_code = Computed_field_evaluate_as_string_at_node(field,
			/*component_number*/-1, node, time);
		Computed_field_clear_cache(field);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_evaluate_as_string_at_node.  Invalid argument(s)");
		return_code=NULL;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_evaluate_as_string_at_node */

int Cmiss_field_is_defined_at_node(Cmiss_field_id field,
	struct Cmiss_node *node)
/*******************************************************************************
LAST MODIFIED : 17 January 2007

DESCRIPTION :
Returns true if <field> can be calculated at <node>. If the field depends on
any other fields, this function is recursively called for them.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_field_is_defined_at_node);
	if (field && node)
	{
		return_code = Computed_field_is_defined_at_node(field, node);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_is_defined_at_node.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_is_defined_at_node */

int Cmiss_field_evaluate_at_field_coordinates(
	Cmiss_field_id field,
	Cmiss_field_id reference_field, int number_of_input_values,
	double *input_values, double time, double *values)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
Returns the <values> of <field> at the location of <input_values>
with respect to the <reference_field> if it is defined there.

The <values> array must be large enough to store as many FE_values as there are
number_of_components.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_field_evaluate_at_field_coordinates);
	if (field&&reference_field&&number_of_input_values&&input_values&&values)
	{
		return_code = Computed_field_evaluate_at_field_coordinates(field,
			reference_field, number_of_input_values, input_values, time, values);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_evaluate_at_field_coordinates.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_evaluate_at_field_coordinates */

char *Cmiss_field_get_name(Cmiss_field_id field)
{
	char *name = NULL;
	GET_NAME(Computed_field)(field, &name);
	return (name);
}

int Cmiss_field_set_name(Cmiss_field_id field, const char *name)
{
	int return_code;

	ENTER(Cmiss_field_set_name);
	if (field && name)
	{
		return_code = Computed_field_set_name(field, name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_field_set_name.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_field_set_name */

