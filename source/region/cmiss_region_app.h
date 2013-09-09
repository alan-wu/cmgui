/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "command/parser.h"
/***************************************************************************//**
 * Option_table modifier function for selecting a region by relative path.
 * @see Option_table_add_set_cmzn_region
 */
int set_cmzn_region(struct Parse_state *state, void *region_address_void,
	void *root_region_void);

/***************************************************************************//**
 * Adds an entry to the <option_table> under the given <token> that selects a 
 * region by relative path from the <root_region>.
 * @param token  Optional token. If omitted, must be last option in table.
 * @param region_address  Address of region to set. Must be initialised to NULL
 * or an ACCESS region.
 */
int Option_table_add_set_cmzn_region(struct Option_table *option_table,
	const char *token, struct cmzn_region *root_region,
	struct cmzn_region **region_address);

int set_cmzn_region_path(struct Parse_state *state, void *path_address_void,
	void *root_region_void);
/*******************************************************************************
LAST MODIFIED : 13 January 2003

DESCRIPTION :
Modifier function for entering a path to a cmzn_region, starting at
<root_region>.
==============================================================================*/

int Option_table_add_set_cmzn_region_path(struct Option_table *option_table,
	const char *entry_name, struct cmzn_region *root_region, char **path_address);
/*******************************************************************************
LAST MODIFIED : 13 March 2003

DESCRIPTION :
Adds an entry to the <option_table> with name <entry_name> that returns a 
region path in <path_address> relative to the <root_region>.
==============================================================================*/


/***************************************************************************//**
 * Structure to pass to function
 * Option_table_add_region_path_and_field_name_entry
 * Before calling: ensure all pointers are NULL
 * After calling: DEACCESS region, if any and DEALLOCATE any strings
 */
struct cmzn_region_path_and_name
{
	struct cmzn_region *region;
	char *region_path;
	char *name;
};

/***************************************************************************//**
 * Adds the token to the option table for setting a region path and/or field
 * name. Note fields must not be the same name as a child region: region names
 * are matched first.
 *
 * @param option_table table to add token to
 * @param token token to be matched. Can be NULL for final, default entry
 * @param region_path_and_name if set contains ACCESSed region and allocated
 *   path and name which caller is required to clean up
 * @param root_region the root region from which path is relative
 */
int Option_table_add_region_path_and_or_field_name_entry(
	struct Option_table *option_table, char *token,
	struct cmzn_region_path_and_name *region_path_and_name,
	struct cmzn_region *root_region);

/***************************************************************************//**
 * Modifier function to set the region and optional group field.
 * Fields must not be the same name as a child region.
 *
 * Examples:
 *   /heart/ventricles = region and group
 *   /heart            = region only
 */
int set_cmzn_region_or_group(struct Parse_state *state,
	void *region_address_void, void *group_address_void);

/***************************************************************************//**
 * Adds token to the option table for setting a region and an optional group
 * field. Note fields must not be the same name as a child region: region names
 * are matched first.
 *
 * @param option_table  Table to add token to
 * @param token  Token to be matched. Can be NULL for final, default entry
 * @param region_address  Pointer to region which must be initialised to
 * accessed root_region, and may be updated to subregion at relative path to
 * this. Caller is responsible for deaccessing.
 * @param group_address  Pointer to group field. Must be initialised to NULL.
 * Caller is responsible for deaccessing if set.
 */
int Option_table_add_region_or_group_entry(struct Option_table *option_table,
	const char *token, cmzn_region_id *region_address,
	cmzn_field_group_id *group_address);
