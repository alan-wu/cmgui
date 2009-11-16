/*******************************************************************************
FILE : cmiss_finite_element.c

LAST MODIFIED : 1 April 2004

DESCRIPTION :
The public interface to the Cmiss_finite_elements.
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
#include <stdarg.h>
#include "api/cmiss_node.h"
#include "general/debug.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_region.h"
#include "user_interface/message.h"

/*
Global functions
----------------
*/

int Cmiss_node_get_identifier(struct Cmiss_node *node)
/*******************************************************************************
LAST MODIFIED : 1 April 2004

DESCRIPTION :
Returns the integer identifier of the <node>.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_node_get_identifier);
	return_code = get_FE_node_identifier(node);
	LEAVE;

	return (return_code);
} /* Cmiss_node_get_identifier */

/* Note that Cmiss_node_set_identifier is not here as the function does 
	not currently exist in finite_element.[ch] either. */

Cmiss_node_id Cmiss_node_create(int node_identifier,
	Cmiss_region_id region)
/*******************************************************************************
LAST MODIFIED : 10 November 2004

DESCRIPTION :
Creates and returns a node with the specified <cm_node_identifier>.
Note that <cm_node_identifier> must be non-negative.
A blank node with the given identifier but no fields is returned.
The new node is set to belong to the ultimate master FE_region of <region>.
==============================================================================*/
{
	Cmiss_node_id node;
	struct FE_region *fe_region;

	ENTER(create_Cmiss_node);
	node = (Cmiss_node_id)NULL;
	if (fe_region=Cmiss_region_get_FE_region(region))
	{
		node = ACCESS(FE_node)(CREATE(FE_node)(node_identifier, fe_region, (struct FE_node *)NULL));
	}
	LEAVE;

	return (node);
} /* create_Cmiss_node */

Cmiss_node_id Cmiss_node_create_from_template(int node_identifier,
	Cmiss_node_id template_node)
/*******************************************************************************
LAST MODIFIED : 10 November 2004

DESCRIPTION :
Creates and returns a node with the specified <cm_node_identifier>.
Note that <cm_node_identifier> must be non-negative.
The node copies all the fields and values of the <template_node> and will
belong to the same region.
==============================================================================*/
{
	Cmiss_node_id node;

	ENTER(Cmiss_node_create_from_template);
	node = ACCESS(FE_node)(CREATE(FE_node)(node_identifier, (struct FE_region *)NULL, 
			template_node));
	LEAVE;

	return (node);
} /* Cmiss_node_create_from_template */

int Cmiss_node_destroy(Cmiss_node_id *node_id_address)
/*******************************************************************************
LAST MODIFIED : 10 November 2004

DESCRIPTION :
Frees the memory for the node, sets <*node_address> to NULL.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_node_destroy);
	return_code = DEACCESS(FE_node)(node_id_address);
	LEAVE;

	return (return_code);
} /* destroy_Cmiss_node */

