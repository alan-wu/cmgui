/*******************************************************************************
FILE : finite_element_region.cpp

LAST MODIFIED : 8 August 2006

DESCRIPTION :
Object comprising a single finite element mesh including nodes, elements and
finite element fields defined on or interpolated over them.
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

#include <cstdlib>
#include <cstdio>
extern "C" {
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_region_private.h"
#include "general/any_object_private.h"
#include "general/any_object_definition.h"
#include "general/callback_private.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "region/cmiss_region.h"
#include "region/cmiss_region_private.h"
#include "user_interface/message.h"
}

/*
Module types
------------
*/

#if defined (EMBEDDING_CODE)
/* Notes on implementing embedding:

I was unable to complete the code for passing messages about fields changing
in FE_regions that nodes or elements in this FE_region are embedded in.

The basic idea of my partially completed code, marked out with EMBEDDING_CODE
is as follows:

Each full FE_region is to maintain an embedded_fe_field_list. This list should
be created as soon as an embedded field is merged with FE_region_merge_FE_field
-- this is detected with function FE_field_is_embedded. To maintain efficiency,
if the incoming field is embedded or if we already have embedded fields, then
we have to determine if existing embedded fields are no longer so. By creating
and destroying the embedded_fe_field_list, its pointer can be used as a flag.

Once we have embedded fields, each FE_region that owns nodes, ie. all those
above plus the data_hack region, will have to monitor fields being merged into
its nodes and maintain the embedding_fe_region_list, which is a list indexed
by FE_region with a count of how many instances of embedded we have. See the
definition of this structure below. For all embedding regions EXCEPT for self,
request change callbacks. The response to these callbacks is to propagate the
field changes from the embedding region, and mark all nodes as related object
changed using the CHANGE_LOG_ALL_CHANGE function -- very efficient. Clients
of this region's callbacks should be able to respond to embedding field changes
as if the fields were native to that region.

It is possible for a region to be embedded in itself, but you should not get
a region to call back itself! Hence, if there is self-embedding, avoid these
callbacks and do a check for embedding changes before the message is sent out
-- augment the change logs in the appropriate manner.

Notes.
- The data_hack uses its master's embedded_fe_field_list.
- Only nodes have embedded element_xi fields at present.


Note this is all quite expensive, but I cannot see any alternative. Clients
should not know which regions they are embedded in -- this enables that. If
there is no embedding, there is negligible overhead in this approach. However,
a single embedded field could slow cmgui down quite a bit. The situation will
be improved once full FE_regions are widely permitted AND they have their own
list of FE_fields -- that way the definition of an embedded FE_field only slows
that region.

GRC 11/06/03

*/

struct FE_region_embedding_usage_count
/*******************************************************************************
LAST MODIFIED : 11 June 2003

DESCRIPTION :
Structure for storing a pointer to an FE_region and the number of times
==============================================================================*/
{
	/* accessed pointer to the FE_region objects are embedded in */
	struct FE_region *fe_region;
	/* ???RC will also need pointer to the region receiving callbacks, to pass as
		 user_data in the callbacks and to avoid callbacks if the region is embedded
		 in itself. This could come about in eg. self-contact. Note that before
		 change messages are sent for this_fe_region, will have to insert embedding
		 ramifications, ie. CHANGE_ALL nodes etc. */
	struct FE_region *this_fe_region;
	/* number of instances of embedding in this FE_region */
	/* ???RC I think the access_count can handle this for us -- overload ACCESS/DEACCESS */
	int embedding_usage_count;
	/* for our list macros */
	int access_count;
};

DECLARE_LIST_TYPES(FE_region_embedding_usage_count);

FULL_DECLARE_INDEXED_LIST_TYPE(FE_region_embedding_usage_count);

#endif /* defined (EMBEDDING_CODE) */

// GRC temporary until CM_element_type removed from FE_element identifier
#define DIMENSION_TO_CM_ELEMENT_TYPE(dimension) \
	((dimension == 3) ? CM_ELEMENT : ((dimension == 2) ? CM_FACE : CM_LINE))
#define CM_ELEMENT_TYPE_TO_DIMENSION(cm_type) \
	((cm_type == CM_ELEMENT) ? 3 : ((cm_type == CM_FACE) ? 2 : 1))

FULL_DECLARE_CMISS_CALLBACK_TYPES(FE_region_change, \
	struct FE_region *, struct FE_region_changes *);

struct FE_region
/*******************************************************************************
LAST MODIFIED : 5 June 2003

DESCRIPTION :
==============================================================================*/
{
	/* pointer to the Cmiss_region this FE_region is in: NOT ACCESSED */
	struct Cmiss_region *cmiss_region;
	/* if set, the master_fe_region owns all nodes, elements and fields, including
		 field information, and this FE_region is merely a list of nodes and
		 elements taken from the master_fe_region.
		 ACCESSed if a normal FE_region, not ACCESSed if a data FE_region */ 
	struct FE_region *master_fe_region;
	/* top_data_hack flag indicates that the master_fe_region owns fields and elements
	 * but not nodes, so FE_region contains independent set of data */
	int top_data_hack;
	/* ACCESSed pointer to attached FE_region containing data for this region */
	/* SAB When checking to see if an FE_node belongs to the FE_region as 
		defined by it's fields we also need to check the data_fe_region */
	struct FE_region *data_fe_region;
	/* if this is a data_fe_region, non-ACCESSing pointer to base_fe_region this
	 * is attached to. Set for top_data_hack and all data regions below it */
	struct FE_region *base_fe_region;

	/* field information, ignored if master_fe_region is used */
	struct FE_time_sequence_package *fe_time;
	struct LIST(FE_field) *fe_field_list;
	struct FE_field_info *fe_field_info;
	struct LIST(FE_node_field_info) *fe_node_field_info_list;
	struct LIST(FE_element_field_info) *fe_element_field_info_list;
	struct MANAGER(FE_basis) *basis_manager;
	/* This flag indicates whether this region explictly created the basis manager
		and therefore should destroy it when destroyed.  Would prefer to have an
		access count on the MANAGER but these don't */
	int region_owns_basis_manager;
	struct LIST(FE_element_shape) *element_shape_list;
	/* This flag indicates whether this region explictly created the element_shape_list
		and therefore should destroy it when destroyed.  Would prefer to have an
		access count on the LIST(FE_element_shape) but these don't */
	int region_owns_element_shape_list;
	/* lists of nodes and elements in this region */
	struct LIST(FE_node) *fe_node_list;
	struct LIST(FE_element) *fe_element_list[MAXIMUM_ELEMENT_XI_DIMENSIONS];

	/* change_level: if zero, change messages are sent with every change,
		 otherwise incremented/decremented for nested changes */
	int change_level;
	/* list of change callbacks */
	/* remember number of clients to make it efficient to know if changes need to
		 be remembered */
	int number_of_clients;
	struct LIST(CMISS_CALLBACK_ITEM(FE_region_change)) *change_callback_list;

	/* internal record of changes from which FE_field_changes is constructed */
	/* fields added, removed or otherwise changed are put in the following list.
		 Note this records changes to the FE_field structure itself, NOT changes to
		 values and representations of the field in nodes and elements -- these are
		 recorded separately as described below */
	struct CHANGE_LOG(FE_field) *fe_field_changes;
	/* nodes added, removed or otherwise changed */
	struct CHANGE_LOG(FE_node) *fe_node_changes;
	/* elements added, removed or otherwise changed */
	struct CHANGE_LOG(FE_element) *fe_element_changes[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	/* keep pointer to last node or element field information so only need to
		 note changed fields when this changes */
	struct FE_node_field_info *last_fe_node_field_info;
	struct FE_element_field_info *last_fe_element_field_info;

	/* information for defining faces */
	/* existence of element_type_node_sequence_list can tell us whether faces
		 are being defined */
	struct LIST(FE_element_type_node_sequence) *element_type_node_sequence_list;

#if defined (EMBEDDING_CODE)
	/* embedding information - only for full FE_regions and in a limited sense
		 for the data_hack as it uses its master's embedded_fe_field_list */
	/* following is NULL unless there is at least one such FE_field so it can
		 be used as a flag */
	struct LIST(FE_field) *embedded_fe_field_list;
	/* following is maintained only if there are instances of embedding, and
		 records the number of instances of embedding in each FE_region. While it
		 is positive, change messages are requested from each FE_region -- except
		 this FE_region, where embedding messages are propagated separately */
	struct LIST(FE_region_embedding_usage_count) *embedding_fe_region_list;
#endif /* defined (EMBEDDING_CODE) */

	/* Keep a record of where we got to searching for valid identifiers.
		We reset the cache if we delete any items */
	int next_fe_node_identifier_cache;
	int next_fe_element_identifier_cache[MAXIMUM_ELEMENT_XI_DIMENSIONS];

	/* initially false, these flags are set to true once wrapping computed field
	 * manager is informed that it should create cmiss_number and xi fields
	 * See: FE_region_need_add_cmiss_number_field, FE_region_need_add_xi_field */
	int informed_make_cmiss_number_field;
	int informed_make_xi_field;

	/* number of objects using this region */
	int access_count;
};

/*
Macros
------
*/

#define FE_REGION_FE_FIELD_CHANGE(fe_region, fe_field, change) \
/***************************************************************************** \
LAST MODIFIED : 13 November 2002 \
\
DESCRIPTION : \
If <fe_region> has clients for its change messages, records the <change> to \
<field> and if the cache_level is zero sends an update. Made into a macro for \
inlining. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	CHANGE_LOG_OBJECT_CHANGE(FE_field)(fe_region->fe_field_changes, fe_field, \
		change); \
	if (0 == fe_region->change_level) \
	{ \
		FE_region_update(fe_region); \
	} \
}

/* Using these macros to conditionally reset the cache based on the 
	constant change type in the FE_REGION_FE_NODE_CHANGE macro */
#define FE_NODE_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_ADDED(type)

#define FE_NODE_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(type)

#define FE_NODE_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_REMOVED(type)	\
	fe_region->next_fe_node_identifier_cache = 0;

#define FE_REGION_FE_NODE_CHANGE(fe_region, node, change, field_info_node) \
/***************************************************************************** \
LAST MODIFIED : 13 February 2003 \
\
DESCRIPTION : \
If <fe_region> has clients for its change messages, records the <change> to \
<node>, and if the <field_info_node> has different node_field_info from the \
last, logs the FE_fields in it as changed and remembers it as the new \
last_fe_node_field_info. If the cache_level is zero, sends an update. \
Made into a macro for efficency/inlining. \
Must supply both <node> and <field_info_node>. \
When a node is added or removed, the same node is used for <node> and \
<field_info_node>. For changes to the contents of <node>, <field_info_node> \
should contain the changed fields, consistent with merging it into <node>. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	struct FE_node_field_info *temp_fe_node_field_info; \
\
   FE_NODE_IDENTIFIER_CACHE_UPDATE_ ## change \
	CHANGE_LOG_OBJECT_CHANGE(FE_node)(fe_region->fe_node_changes, node, change); \
	temp_fe_node_field_info = FE_node_get_FE_node_field_info(field_info_node); \
	if (temp_fe_node_field_info != fe_region->last_fe_node_field_info) \
	{ \
		FE_node_field_info_log_FE_field_changes(temp_fe_node_field_info, \
			fe_region->fe_field_changes); \
		fe_region->last_fe_node_field_info = temp_fe_node_field_info; \
	} \
	if (0 == fe_region->change_level) \
	{ \
		FE_region_update(fe_region); \
	} \
}

#define FE_REGION_FE_NODE_IDENTIFIER_CHANGE(fe_region, node) \
/***************************************************************************** \
LAST MODIFIED : 16 November 2002 \
\
DESCRIPTION : \
Use this macro instead of FE_REGION_FE_NODE_CHANGE when only the identifier \
has changed. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	fe_region->next_fe_node_identifier_cache = 0; \
	CHANGE_LOG_OBJECT_CHANGE(FE_node)(fe_region->fe_node_changes, node, \
		CHANGE_LOG_OBJECT_IDENTIFIER_CHANGED(FE_node)); \
	if (0 == fe_region->change_level) \
	{ \
		FE_region_update(fe_region); \
	} \
}

#define FE_REGION_FE_NODE_FIELD_CHANGE(fe_region, node, fe_field) \
/***************************************************************************** \
LAST MODIFIED : 6 March 2003 \
\
DESCRIPTION : \
Use this macro instead of FE_REGION_FE_NODE_CHANGE when exactly one field,
<fe_field> of <node> has changed. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	CHANGE_LOG_OBJECT_CHANGE(FE_node)(fe_region->fe_node_changes, node, \
		CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_node)); \
	CHANGE_LOG_OBJECT_CHANGE(FE_field)(fe_region->fe_field_changes, \
		fe_field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field)); \
	if (0 == fe_region->change_level) \
	{ \
		FE_region_update(fe_region); \
	} \
}

/* Using these macros to conditionally reset the cache based on the 
	constant change type in the FE_REGION_FE_ELEMENT_CHANGE macro */
#define FE_ELEMENT_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_ADDED(type)

#define FE_ELEMENT_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(type)

#define FE_ELEMENT_IDENTIFIER_CACHE_UPDATE_CHANGE_LOG_OBJECT_REMOVED(type)	\
	/* Don't want to add the overhead of working out which type at this point */ \
	for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim) \
	{ \
		fe_region->next_fe_element_identifier_cache[dim] = 1; \
	} \

#define FE_REGION_FE_ELEMENT_CHANGE(fe_region, element, change, \
	field_info_element) \
/***************************************************************************** \
LAST MODIFIED : 31 May 2006 \
\
DESCRIPTION : \
If <fe_region> has clients for its change messages, records the <change> to \
<element>, and if the <field_info_element> has different element_field_info \
from the last, logs the FE_fields in it as changed and remembers it as the new \
last_fe_element_field_info. If the cache_level is zero, sends an update. \
Made into a macro for efficency/inlining. \
Must supply both <element> and <field_info_element>. \
When a element is added or removed, the same element is used for <element> and \
<field_info_element>. For changes to the contents of <element>,
<field_info_element> should contain the changed fields, consistent with \
merging it into <element>. \
============================================================================*/ \
FE_ELEMENT_IDENTIFIER_CACHE_UPDATE_ ## change \
if (0 < fe_region->number_of_clients) \
{ \
	int dimension = get_FE_element_dimension(element); \
	if (dimension) \
	{ \
		CHANGE_LOG_OBJECT_CHANGE(FE_element)( \
			fe_region->fe_element_changes[dimension - 1], element, change); \
		FE_element_log_FE_field_changes(field_info_element, \
			fe_region->fe_field_changes, &(fe_region->last_fe_element_field_info)); \
		if (0 == fe_region->change_level) \
		{ \
			FE_region_update(fe_region); \
		} \
	} \
}

#define FE_REGION_FE_ELEMENT_FE_FIELD_LIST_CHANGE(fe_region, element, \
	change, changed_fe_field_list) \
/***************************************************************************** \
LAST MODIFIED : 30 May 2003 \
\
DESCRIPTION : \
If <fe_region> has clients for its change messages, records the <change> to \
<element>, then logs each FE_field in <changed_fe_field_list> as having \
had a RELATED_OBJECT_CHANGED. If the cache_level is zero, sends an update. \
Made into a macro for consistency/efficency/inlining. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	int dimension = get_FE_element_dimension(element); \
	if (dimension) \
	{ \
		CHANGE_LOG_OBJECT_CHANGE(FE_element)( \
			fe_region->fe_element_changes[dimension - 1], element, change); \
		FOR_EACH_OBJECT_IN_LIST(FE_field)(FE_field_log_FE_field_change, \
			(void *)fe_region->fe_field_changes, changed_fe_field_list); \
		if (0 == fe_region->change_level) \
		{ \
			FE_region_update(fe_region); \
		} \
	} \
}

#define FE_REGION_FE_ELEMENT_IDENTIFIER_CHANGE(fe_region, element) \
/***************************************************************************** \
LAST MODIFIED : 16 November 2002 \
\
DESCRIPTION : \
Use this macro instead of FE_REGION_FE_ELEMENT_CHANGE when only the identifier \
has changed. \
============================================================================*/ \
/* Don't want to add the overhead of working out which type at this point */ \
for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim) \
{ \
	fe_region->next_fe_element_identifier_cache[dim] = 1; \
} \
if (0 < fe_region->number_of_clients) \
{ \
	int dimension = get_FE_element_dimension(element); \
	if (dimension) \
	{ \
		CHANGE_LOG_OBJECT_CHANGE(FE_element)( \
			fe_region->fe_element_changes[dimension - 1], element, \
			CHANGE_LOG_OBJECT_IDENTIFIER_CHANGED(FE_element)); \
		if (0 == fe_region->change_level) \
		{ \
			FE_region_update(fe_region); \
		} \
	} \
}

#define FE_REGION_FE_ELEMENT_FIELD_CHANGE(fe_region, element, fe_field) \
/***************************************************************************** \
LAST MODIFIED : 6 March 2003 \
\
DESCRIPTION : \
Use this macro instead of FE_REGION_FE_ELEMENT_CHANGE when exactly one field,
<fe_field> of <element> has changed. \
============================================================================*/ \
if (0 < fe_region->number_of_clients) \
{ \
	int dimension = get_FE_element_dimension(element); \
	if (dimension) \
	{ \
		CHANGE_LOG_OBJECT_CHANGE(FE_element)( \
			fe_region->fe_element_changes[dimension - 1], element, \
			CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element)); \
		CHANGE_LOG_OBJECT_CHANGE(FE_field)(fe_region->fe_field_changes, \
			fe_field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field)); \
		if (0 == fe_region->change_level) \
		{ \
			FE_region_update(fe_region); \
		} \
	} \
}

/*
Module functions
----------------
*/

#if defined (EMBEDDING_CODE)
CREATE(FE_region_embedding_usage_count)

int DESTROY(FE_region_embedding_usage_count)(
	struct FE_region_embedding_usage_count **fe_region_embedding_address)
/*******************************************************************************
LAST MODIFIED : 6 June 2003

DESCRIPTION :
Frees the memory for the FE_region_embedding_usage_count and sets
<*fe_region_embedding_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct FE_region_embedding_usage_count *fe_region_embedding;

	ENTER(DESTROY(FE_region_embedding_usage_count));
	if (fe_region_embedding_address &&
		(fe_region_embedding = *fe_region_embedding_address))
	{
		if (0 == fe_region_embedding->access_count)
		{
			if (fe_region_embedding->fe_region != fe_region_embedding->this_fe_region)
			{
				FE_region_remove_callback(fe_region_embedding->fe_region,
					FE_region_embedding_FE_region_change,
					(void *)fe_region_embedding->this_fe_region);
			}
			DEACCESS(FE_region)(&(fe_region_embedding->fe_region));
			DEALLOCATE(*fe_region_embedding_address);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(FE_region_embedding_usage_count).  Non-zero access count");
			return_code = 0;
		}
		*fe_region_embedding_address =
			(struct FE_region_embedding_usage_count *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE, "DESTROY(FE_region_embedding_usage_count).  "
			"Missing FE_region_embedding_usage_count");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_region_embedding_usage_count) */

DECLARE_ACCESS_OBJECT_FUNCTION(FE_region_embedding_usage_count)

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(FE_region_embedding_usage_count)
/*******************************************************************************
LAST MODIFIED : 29 January 2003

DESCRIPTION :
Special version of DEACCESS which if the FE_region_embedding_usage_count access_count reaches
1 and it has an fe_region member, calls FE_region_remove_FE_region_embedding_usage_count.
Since the FE_region accesses the info once, this indicates no other object is
using it so it should be flushed from the FE_region. When the owning FE_region
deaccesses the info, it is destroyed in this function.
==============================================================================*/
{
	int return_code;
	struct FE_region_embedding_usage_count *object;

	ENTER(DEACCESS(FE_region_embedding_usage_count));
	if (object_address && (object = *object_address))
	{
		(object->access_count)--;
		return_code = 1;
		if (object->access_count <= 1)
		{
			if (1 == object->access_count)
			{
				if (object->fe_region)
				{
					return_code =
						FE_region_remove_FE_region_embedding_usage_count(object->fe_region, object);
				}
			}
			else
			{
				return_code = DESTROY(FE_region_embedding_usage_count)(object_address);
			}
		}
		*object_address = (struct FE_region_embedding_usage_count *)NULL;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DEACCESS(FE_region_embedding_usage_count) */

PROTOTYPE_REACCESS_OBJECT_FUNCTION(FE_region_embedding_usage_count)
/*******************************************************************************
LAST MODIFIED : 20 February 2003

DESCRIPTION :
Special version of REACCESS which if the FE_region_embedding_usage_count access_count reaches
1 and it has an fe_region member, calls FE_region_remove_FE_region_embedding_usage_count.
Since the FE_region accesses the info once, this indicates no other object is
using it so it should be flushed from the FE_region. When the owning FE_region
deaccesses the info, it is destroyed in this function.
==============================================================================*/
{
	int return_code;
	struct FE_region_embedding_usage_count *current_object;

	ENTER(REACCESS(FE_region_embedding_usage_count));
	if (object_address)
	{
		return_code = 1;
		if (new_object)
		{
			/* access the new object */
			(new_object->access_count)++;
		}
		if (current_object = *object_address)
		{
			/* deaccess the current object */
			(current_object->access_count)--;
			if (current_object->access_count <= 1)
			{
				if (1 == current_object->access_count)
				{
					if (current_object->fe_region)
					{
						return_code = FE_region_remove_FE_region_embedding_usage_count(
							current_object->fe_region, current_object);
					}
				}
				else
				{
					return_code = DESTROY(FE_region_embedding_usage_count)(object_address);
				}
			}
		}
		/* point to the new object */
		*object_address = new_object;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"REACCESS(FE_region_embedding_usage_count).  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* REACCESS(FE_region_embedding_usage_count) */

DECLARE_OBJECT_FUNCTIONS(FE_region_embedding_usage_count)

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(FE_region_embedding_usage_count, \
	fe_region, struct FE_region *, compare_pointer)

DECLARE_INDEXED_LIST_FUNCTIONS(FE_region_embedding_usage_count)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION( \
	FE_region_embedding_usage_count, fe_region, struct FE_region *, \
	compare_pointer)

#endif /* defined (EMBEDDING_CODE) */

static int FE_region_void_detach_from_Cmiss_region(void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
Clean-up function for FE_regions accessed by their any_object.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;

	ENTER(FE_region_void_detach_from_Cmiss_region);
	if (fe_region = (struct FE_region *)fe_region_void)
	{
		fe_region->cmiss_region = (struct Cmiss_region *)NULL;
		return_code = DEACCESS(FE_region)(&fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_void_detach_from_Cmiss_region.  Missing void FE_region");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_void_detach_from_Cmiss_region */

DEFINE_CMISS_CALLBACK_MODULE_FUNCTIONS(FE_region_change, void)

DEFINE_CMISS_CALLBACK_FUNCTIONS(FE_region_change, \
	struct FE_region *, struct FE_region_changes *)

static struct LIST(FE_element) *FE_region_get_element_list(
	struct FE_region *fe_region, int dimension)
{
	if (fe_region && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		return fe_region->fe_element_list[dimension - 1];
	}
	return 0;
}

static int FE_region_create_change_logs(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 25 March 2003

DESCRIPTION :
Creates and initializes the change logs in <fe_region>.
Centralised so they are created and recreated consistently.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_create_change_logs);
	if (fe_region)
	{
		fe_region->fe_field_changes = CREATE(CHANGE_LOG(FE_field))(
			fe_region->fe_field_list, /*max_changes*/-1);
		fe_region->fe_node_changes = CREATE(CHANGE_LOG(FE_node))(
			fe_region->fe_node_list, /*max_changes*/50);
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			fe_region->fe_element_changes[dim] = CREATE(CHANGE_LOG(FE_element))(
				fe_region->fe_element_list[dim], /*max_changes*/50);
		}
		fe_region->last_fe_node_field_info = (struct FE_node_field_info *)NULL;
		fe_region->last_fe_element_field_info =
			(struct FE_element_field_info *)NULL;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_create_change_logs.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_create_change_logs */

static int FE_region_update(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 21 March 2003

DESCRIPTION :
Tells the clients of the <fe_region> about changes to fields, nodes and
elements. No messages sent if change counter is positive or if no changes have
occurred.
==============================================================================*/
{
	int number_of_fe_element_changes, number_of_fe_field_changes,
		number_of_fe_node_changes, return_code;
	struct FE_region_changes changes;

	ENTER(FE_region_update);
	if (fe_region)
	{
		return_code = 1;
		if (!fe_region->change_level)
		{
			bool any_changes = false;
			/* send callbacks only if changes have been made */
			if ((CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_field)(
				fe_region->fe_field_changes, &number_of_fe_field_changes) &&
				(0 < number_of_fe_field_changes)) ||
				(CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_node)(
					fe_region->fe_node_changes, &number_of_fe_node_changes) &&
					(0 < number_of_fe_node_changes)))
			{
				any_changes = true;
			}
			else
			{
				for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				{
					if (CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_element)(
						fe_region->fe_element_changes[dim], &number_of_fe_element_changes) &&
						(0 < number_of_fe_element_changes))
					{
						any_changes = true;
						break;
					}
				}
			}
			if (any_changes)
			{
#if defined (DEBUG)
				/*???debug start*/
				{
					CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_field)(
						fe_region->fe_field_changes, &number_of_fe_field_changes);
					CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_node)(
						fe_region->fe_node_changes, &number_of_fe_node_changes);
					CHANGE_LOG_GET_NUMBER_OF_CHANGES(FE_element)(
						fe_region->fe_element_changes, &number_of_fe_element_changes);
					/*???debug*/printf("------ UPDATE %p:  fields %d  nodes %d  elements %d   (M %p)",
						fe_region, number_of_fe_field_changes, number_of_fe_node_changes,
						number_of_fe_element_changes,fe_region->master_fe_region);
					if (fe_region->top_data_hack)
					{
						/*???debug*/printf(" DATA!\n");
					}
					else
					{
						/*???debug*/printf("\n");
					}
				}
				/*???debug stop*/
#endif /* defined (DEBUG) */

				/* fill the changes structure */
				changes.fe_field_changes = fe_region->fe_field_changes;
				changes.fe_node_changes = fe_region->fe_node_changes;
				for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				{
					changes.fe_element_changes[dim] = fe_region->fe_element_changes[dim];
				}
				/* must create new, empty change logs in the FE_region, so FE_region
					 is able to immediately receive new change messages, and so that
					 begin/end_change does not cause the same changes to be re-sent */
				FE_region_create_change_logs(fe_region);
				/* send the callbacks */
				return_code = CMISS_CALLBACK_LIST_CALL(FE_region_change)(
					fe_region->change_callback_list, fe_region, &changes);
				/* clean up the change logs in the changes */
				DESTROY(CHANGE_LOG(FE_field))(&changes.fe_field_changes);
				DESTROY(CHANGE_LOG(FE_node))(&changes.fe_node_changes);
				for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				{
					DESTROY(CHANGE_LOG(FE_element))(&changes.fe_element_changes[dim]);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_update.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_update */

struct CHANGE_LOG(FE_element) *FE_region_changes_get_FE_element_changes(
	struct FE_region_changes *changes, int dimension)
{
	if (changes && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		return changes->fe_element_changes[dimension-1];
	}
	return 0;
}

int FE_region_merge_FE_element_iterator(struct FE_element *element,
	void *fe_region_void);
/*******************************************************************************
LAST MODIFIED : 17 January 2003

DESCRIPTION :
Forward declaration.
==============================================================================*/

void FE_region_master_fe_region_change(struct FE_region *master_fe_region,
	struct FE_region_changes *changes, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 21 March 2003

DESCRIPTION :
Callback from <master_fe_region> with its <changes>.
==============================================================================*/
{
	struct FE_region *fe_region;

	ENTER(FE_region_master_fe_region_change);
	fe_region = (struct FE_region *)NULL;
	if (master_fe_region && changes &&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		/* manually step up the change_level to avoid calls to master_fe_region */
		(fe_region->change_level)++;
		CHANGE_LOG_MERGE(FE_field)(fe_region->fe_field_changes,
			changes->fe_field_changes);
		if (!fe_region->top_data_hack)
		{
			CHANGE_LOG_MERGE(FE_node)(fe_region->fe_node_changes,
				changes->fe_node_changes);
			int fe_node_change_summary;
			if (CHANGE_LOG_GET_CHANGE_SUMMARY(FE_node)(fe_region->fe_node_changes,
				&fe_node_change_summary))
			{
				/* remove nodes removed from this fe_region */
				if (fe_node_change_summary & CHANGE_LOG_OBJECT_REMOVED(FE_node))
				{
					REMOVE_OBJECTS_FROM_LIST_THAT(FE_node)(FE_node_is_not_in_list,
						(void *)master_fe_region->fe_node_list,
						fe_region->fe_node_list);
				}
			}
			for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; 0 <= dim; --dim)
			{
				CHANGE_LOG_MERGE(FE_element)(fe_region->fe_element_changes[dim],
					changes->fe_element_changes[dim]);
				int fe_element_change_summary;
				if (CHANGE_LOG_GET_CHANGE_SUMMARY(FE_element)(
					fe_region->fe_element_changes[dim], &fe_element_change_summary))
				{
					/* remove elements removed from this fe_region */
					if (fe_element_change_summary & CHANGE_LOG_OBJECT_REMOVED(FE_element))
					{
						REMOVE_OBJECTS_FROM_LIST_THAT(FE_element)(FE_element_is_not_in_list,
							(void *)master_fe_region->fe_element_list[dim],
							fe_region->fe_element_list[dim]);
					}
					if (dim < MAXIMUM_ELEMENT_XI_DIMENSIONS - 1)
					{
						/* add faces new to this fe_region; this is indicated by elements changing
						 * and lower dimension elements being added to master_fe_region */
						int element_change_summary = 0;
						CHANGE_LOG_GET_CHANGE_SUMMARY(FE_element)(changes->fe_element_changes[dim + 1],
							&element_change_summary);
						int face_change_summary = 0;
						CHANGE_LOG_GET_CHANGE_SUMMARY(FE_element)(changes->fe_element_changes[dim],
							&face_change_summary);
						if ((element_change_summary & CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element)) &&
							(face_change_summary & CHANGE_LOG_OBJECT_ADDED(FE_element)))
						{
							struct FE_element_add_faces_not_in_list_data data;
							data.current_element_list = fe_region->fe_element_list[dim];
							data.add_element_list = CREATE(LIST(FE_element))();
							FOR_EACH_OBJECT_IN_LIST(FE_element)(
								FE_element_add_faces_not_in_list, (void *)&data,
								fe_region->fe_element_list[dim + 1]);
							FOR_EACH_OBJECT_IN_LIST(FE_element)(
								FE_region_merge_FE_element_iterator, (void *)fe_region,
								data.add_element_list);
							DESTROY(LIST(FE_element))(&data.add_element_list);
						}
					}
				}
			}
		}
		/* restore change_level again */
		(fe_region->change_level)--;
		/* send message to clients if any changes have occurred */
		FE_region_update(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_master_fe_region_change.  Invalid argument(s)");
	}
	LEAVE;
} /* FE_region_master_fe_region_change */

/*
Private functions
-----------------
*/

struct FE_field_info *FE_region_get_FE_field_info(
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 2 April 2003

DESCRIPTION :
Returns a struct FE_field_info for <fe_region>.
This is an object private to FE_region that is common between all fields
owned by FE_region. FE_fields access this object, but this object maintains
a non-ACCESSed pointer to <fe_region> so fields can determine which FE_region
they belong to.
==============================================================================*/
{
	struct FE_field_info *fe_field_info;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_field_info);
	fe_field_info = (struct FE_field_info *)NULL;
	if (fe_region)
	{
		/* get the ultimate master fe_region */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (!master_fe_region->fe_field_info)
		{
			master_fe_region->fe_field_info =
				ACCESS(FE_field_info)(CREATE(FE_field_info)(master_fe_region));
		}
		fe_field_info = master_fe_region->fe_field_info;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_field_info.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_field_info);
} /* FE_region_get_FE_field_info */

struct FE_node_field_info *FE_region_get_FE_node_field_info(
	struct FE_region *fe_region,
	int number_of_values,	struct LIST(FE_node_field) *fe_node_field_list)
/*******************************************************************************
LAST MODIFIED : 19 February 2003

DESCRIPTION :
Returns a struct FE_node_field_info for the supplied <fe_node_field_list> with
<number_of_values>. The <fe_region> maintains an internal list of these
structures so they can be shared between nodes.
If <node_field_list> is omitted, an empty list is assumed.
==============================================================================*/
{
	int existing_number_of_values;
	struct FE_node_field_info *existing_fe_node_field_info, *fe_node_field_info;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_node_field_info);
	fe_node_field_info = (struct FE_node_field_info *)NULL;
	if (fe_region)
	{
		/* get the ultimate master fe_region */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (fe_node_field_list)
		{
			existing_fe_node_field_info =
				FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
					FE_node_field_info_has_matching_FE_node_field_list,
					(void *)fe_node_field_list,
					master_fe_region->fe_node_field_info_list);
		}
		else
		{
			existing_fe_node_field_info =
				FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
					FE_node_field_info_has_empty_FE_node_field_list, (void *)NULL,
					master_fe_region->fe_node_field_info_list);
		}
		if (existing_fe_node_field_info)
		{
			existing_number_of_values =
				FE_node_field_info_get_number_of_values(existing_fe_node_field_info);
			if (number_of_values == existing_number_of_values)
			{
				fe_node_field_info = existing_fe_node_field_info;
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_get_FE_node_field_info.  "
					"Existing node field information has %d values, not %d requested",
					existing_number_of_values, number_of_values);
			}
		}
		else
		{
			if (fe_node_field_info = CREATE(FE_node_field_info)(master_fe_region,
				fe_node_field_list, number_of_values))
			{
				if (!ADD_OBJECT_TO_LIST(FE_node_field_info)(fe_node_field_info,
					master_fe_region->fe_node_field_info_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_FE_node_field_info.  Could not add to FE_region");
					DESTROY(FE_node_field_info)(&fe_node_field_info);
					fe_node_field_info = (struct FE_node_field_info *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_get_FE_node_field_info.  "
					"Could not create node field information");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_node_field_info.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_node_field_info);
} /* FE_region_get_FE_node_field_info */

int FE_region_get_FE_node_field_info_adding_new_field(
	struct FE_region *fe_region, struct FE_node_field_info **node_field_info_address, 
	struct FE_node_field *new_node_field, int new_number_of_values)
/*******************************************************************************
LAST MODIFIED : 24 August 2005

DESCRIPTION :
Updates the pointer to <node_field_info_address> to point to a node_field info
which appends to the fields in <node_field_info_address> one <new_node_field>.
The node_field_info returned in <node_field_info_address> will be for the
<new_number_of_values>.
The <fe_region> maintains an internal list of these structures so they can be 
shared between nodes.  This function allows a fast path when adding a single 
field.  If the node_field passed in is only referenced by one external object
then it is assumed that this function can modify it rather than copying.  If 
there are more references then the object is copied and then modified.
This function handles the access and deaccess of the <node_field_info_address>
as if it is just updating then there is nothing to do.
==============================================================================*/
{
	int return_code;
	struct FE_node_field_info *existing_node_field_info, *new_node_field_info;
	struct LIST(FE_node_field) *node_field_list;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_node_field_info_adding_new_field);
	if (fe_region && node_field_info_address && 
		(existing_node_field_info = *node_field_info_address))
	{
		return_code = 1;
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (FE_node_field_info_used_only_once(existing_node_field_info))
		{
			FE_node_field_info_add_node_field(existing_node_field_info,
				new_node_field, new_number_of_values);
			if (new_node_field_info =
				FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
					FE_node_field_info_has_matching_FE_node_field_list,
					(void *)FE_node_field_info_get_node_field_list(existing_node_field_info),
					master_fe_region->fe_node_field_info_list))
			{
				REACCESS(FE_node_field_info)(node_field_info_address,
					new_node_field_info);
			}
		}
		else
		{
			/* Need to copy after all */
			node_field_list = CREATE_LIST(FE_node_field)();
			if (COPY_LIST(FE_node_field)(node_field_list,
					FE_node_field_info_get_node_field_list(existing_node_field_info)))
			{
				/* add the new node field */
				if (ADD_OBJECT_TO_LIST(FE_node_field)(new_node_field, node_field_list))
				{
					/* create the new node information */
					if (new_node_field_info = FE_region_get_FE_node_field_info(
						master_fe_region, new_number_of_values, node_field_list))
					{
						REACCESS(FE_node_field_info)(node_field_info_address,
							new_node_field_info);
					}
				}
			}
			DESTROY(LIST(FE_node_field))(&node_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_node_field_info_adding_new_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_get_FE_node_field_info_adding_new_field */

static struct FE_node_field_info *FE_region_clone_FE_node_field_info(
	struct FE_region *fe_region, struct FE_node_field_info *fe_node_field_info)
/*******************************************************************************
LAST MODIFIED : 27 February 2003

DESCRIPTION :
Returns a clone of <fe_node_field_info> that belongs to <fe_region> and
uses equivalent FE_fields and FE_time_sequences from <fe_region>.
<fe_region> is expected to be its own ultimate master FE_region.
Used to merge nodes from other FE_regions into.
It is an error if an equivalent/same name FE_field is not found in <fe_region>.
==============================================================================*/
{
	struct FE_node_field_info *clone_fe_node_field_info;
	struct LIST(FE_node_field) *fe_node_field_list;

	ENTER(FE_region_clone_FE_node_field_info);
	clone_fe_node_field_info = (struct FE_node_field_info *)NULL;
	if (fe_region && fe_node_field_info)
	{
		if (fe_node_field_list = FE_node_field_list_clone_with_FE_field_list(
			FE_node_field_info_get_node_field_list(fe_node_field_info),
			fe_region->fe_field_list, fe_region->fe_time))
		{
			clone_fe_node_field_info = FE_region_get_FE_node_field_info(
				fe_region, FE_node_field_info_get_number_of_values(fe_node_field_info),
				fe_node_field_list);
			DESTROY(LIST(FE_node_field))(&fe_node_field_list);
		}
		if (!clone_fe_node_field_info)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_clone_FE_node_field_info.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_clone_FE_node_field_info.  Invalid argument(s)");
	}
	LEAVE;

	return (clone_fe_node_field_info);
} /* FE_region_clone_FE_node_field_info */

int FE_region_remove_FE_node_field_info(struct FE_region *fe_region,
	struct FE_node_field_info *fe_node_field_info)
/*******************************************************************************
LAST MODIFIED : 19 February 2003

DESCRIPTION :
Provided EXCLUSIVELY for the use of DEACCESS and REACCESS functions.
Called when the access_count of <fe_node_field_info> drops to 1 so that
<fe_region> can destroy FE_node_field_info not in use.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_remove_FE_node_field_info);
	if (fe_region && fe_node_field_info)
	{
		return_code = REMOVE_OBJECT_FROM_LIST(FE_node_field_info)(
			fe_node_field_info, fe_region->fe_node_field_info_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_node_field_info.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_node_field_info */

struct FE_element_field_info *FE_region_get_FE_element_field_info(
	struct FE_region *fe_region,
	struct LIST(FE_element_field) *fe_element_field_list)
/*******************************************************************************
LAST MODIFIED : 20 February 2003

DESCRIPTION :
Returns a struct FE_element_field_info for the supplied <fe_element_field_list>.
The <fe_region> maintains an internal list of these structures so they can be
shared between elements.
If <element_field_list> is omitted, an empty list is assumed.
==============================================================================*/
{
	struct FE_element_field_info *existing_fe_element_field_info,
		*fe_element_field_info;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_element_field_info);
	fe_element_field_info = (struct FE_element_field_info *)NULL;
	if (fe_region)
	{
		/* get the ultimate master fe_region */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (fe_element_field_list)
		{
			existing_fe_element_field_info =
				FIRST_OBJECT_IN_LIST_THAT(FE_element_field_info)(
					FE_element_field_info_has_matching_FE_element_field_list,
					(void *)fe_element_field_list,
					master_fe_region->fe_element_field_info_list);
		}
		else
		{
			existing_fe_element_field_info =
				FIRST_OBJECT_IN_LIST_THAT(FE_element_field_info)(
					FE_element_field_info_has_empty_FE_element_field_list, (void *)NULL,
					master_fe_region->fe_element_field_info_list);
		}
		if (existing_fe_element_field_info)
		{
			fe_element_field_info = existing_fe_element_field_info;
		}
		else
		{
			if (fe_element_field_info =
				CREATE(FE_element_field_info)(master_fe_region, fe_element_field_list))
			{
				if (!ADD_OBJECT_TO_LIST(FE_element_field_info)(fe_element_field_info,
					master_fe_region->fe_element_field_info_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_FE_element_field_info.  Could not add to FE_region");
					DESTROY(FE_element_field_info)(&fe_element_field_info);
					fe_element_field_info = (struct FE_element_field_info *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_get_FE_element_field_info.  "
					"Could not create element field information");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_element_field_info.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_element_field_info);
} /* FE_region_get_FE_element_field_info */

static struct FE_element_field_info *FE_region_clone_FE_element_field_info(
	struct FE_region *fe_region,
	struct FE_element_field_info *fe_element_field_info)
/*******************************************************************************
LAST MODIFIED : 27 February 2003

DESCRIPTION :
Returns a clone of <fe_element_field_info> that belongs to <fe_region> and
uses equivalent FE_fields and FE_time_sequences from <fe_region>.
<fe_region> is expected to be its own ultimate master FE_region.
Used to merge elements from other FE_regions into.
It is an error if an equivalent/same name FE_field is not found in <fe_region>.
==============================================================================*/
{
	struct FE_element_field_info *clone_fe_element_field_info;
	struct LIST(FE_element_field) *fe_element_field_list;

	ENTER(FE_region_clone_FE_element_field_info);
	clone_fe_element_field_info = (struct FE_element_field_info *)NULL;
	if (fe_region && fe_element_field_info)
	{
		if (fe_element_field_list = FE_element_field_list_clone_with_FE_field_list(
			FE_element_field_info_get_element_field_list(fe_element_field_info),
			fe_region->fe_field_list))
		{
			clone_fe_element_field_info = FE_region_get_FE_element_field_info(
				fe_region, fe_element_field_list);
			DESTROY(LIST(FE_element_field))(&fe_element_field_list);
		}
		if (!clone_fe_element_field_info)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_clone_FE_element_field_info.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_clone_FE_element_field_info.  Invalid argument(s)");
	}
	LEAVE;

	return (clone_fe_element_field_info);
} /* FE_region_clone_FE_element_field_info */

int FE_region_remove_FE_element_field_info(struct FE_region *fe_region,
	struct FE_element_field_info *fe_element_field_info)
/*******************************************************************************
LAST MODIFIED : 19 February 2003

DESCRIPTION :
Provided EXCLUSIVELY for the use of DEACCESS and REACCESS functions.
Called when the access_count of <fe_element_field_info> drops to 1 so that
<fe_region> can destroy FE_element_field_info not in use.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_remove_FE_element_field_info);
	if (fe_region && fe_element_field_info)
	{
		return_code = REMOVE_OBJECT_FROM_LIST(FE_element_field_info)(
			fe_element_field_info, fe_region->fe_element_field_info_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_element_field_info.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_element_field_info */

#if defined (OLD_CODE)
int FE_node_field_info_list_has_field_with_multiple_times(
	struct LIST(FE_node_field_info) *node_field_info_list,
	struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 7 October 2002

DESCRIPTION :
Returns true if any node field info in <node_field_info_list> has a node field
for <field> that has multiple times.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_field_info_list_has_field_with_multiple_times);
	if (node_field_info_list && field)
	{
		if (FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
			FE_node_field_info_has_field_with_multiple_times,	(void *)field,
			node_field_info_list))
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
		display_message(ERROR_MESSAGE,
			"FE_node_field_info_list_has_field_with_multiple_times.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_info_list_has_field_with_multiple_times */
#endif /* defined (OLD_CODE) */

/*
Global functions
----------------
*/

struct FE_region *CREATE(FE_region)(struct FE_region *master_fe_region,
	struct MANAGER(FE_basis) *basis_manager,
	struct LIST(FE_element_shape) *element_shape_list)
/*******************************************************************************
LAST MODIFIED : 7 July 2003

DESCRIPTION :
Creates a struct FE_region.
If <master_fe_region> is supplied, the <basis_manager> and <element_shape_list>
are ignored and along with all fields, nodes and elements the FE_region may address,
will belong to the master region, and this FE_region will be merely a container
for nodes and elements.
If <master_fe_region> is not supplied, the FE_region will own all its own nodes,
elements and fields.  If <basis_manager> or <element_shape_list> are not 
supplied then a default empty object will be created for this region.  (Allowing
them to be specified allows sharing across regions).
==============================================================================*/
{
	struct FE_region *fe_region;

	ENTER(CREATE(FE_region));
	fe_region = (struct FE_region *)NULL;
	if (ALLOCATE(fe_region, struct FE_region, 1))
	{
		if (master_fe_region)
		{
			/* ACCESS master_fe_region here; unaccessed for data FE_region --
			 * see: FE_region_get_data_FE_region() */
			fe_region->master_fe_region = ACCESS(FE_region)(master_fe_region);
			fe_region->basis_manager = (struct MANAGER(FE_basis) *)NULL;
			fe_region->region_owns_basis_manager = 0;
			fe_region->element_shape_list = (struct LIST(FE_element_shape) *)NULL;
			fe_region->region_owns_element_shape_list = 0;
			fe_region->fe_time = (struct FE_time_sequence_package *)NULL;
			fe_region->fe_field_list = (struct LIST(FE_field) *)NULL;
			fe_region->fe_node_field_info_list =
				(struct LIST(FE_node_field_info) *)NULL;
			fe_region->fe_element_field_info_list =
				(struct LIST(FE_element_field_info) *)NULL;
			/* request callbacks from master_fe_region */
			FE_region_add_callback(master_fe_region,
				FE_region_master_fe_region_change, (void *)fe_region);
			fe_region->fe_node_list = CREATE_RELATED_LIST(FE_node)(master_fe_region->fe_node_list);
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
			{
				fe_region->fe_element_list[dim] = CREATE_RELATED_LIST(FE_element)(master_fe_region->fe_element_list[dim]);
			}
		}
		else
		{
			fe_region->master_fe_region = (struct FE_region *)NULL;
			if (basis_manager)
			{
				fe_region->basis_manager = basis_manager;
				fe_region->region_owns_basis_manager = 0;
			}
			else
			{
				fe_region->basis_manager = CREATE(MANAGER(FE_basis))();
				fe_region->region_owns_basis_manager = 1;
			}
			if (element_shape_list)
			{
				fe_region->element_shape_list = element_shape_list;
				fe_region->region_owns_element_shape_list = 0;
			}
			else
			{
				fe_region->element_shape_list = CREATE(LIST(FE_element_shape))();
				fe_region->region_owns_element_shape_list = 1;
			}
			fe_region->fe_time = CREATE(FE_time_sequence_package)();
			fe_region->fe_field_list = CREATE(LIST(FE_field))();
			fe_region->fe_node_field_info_list = CREATE(LIST(FE_node_field_info))();
			fe_region->fe_element_field_info_list =
				CREATE(LIST(FE_element_field_info))();
			fe_region->fe_node_list = CREATE_LIST(FE_node)();
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
			{
				fe_region->fe_element_list[dim] = CREATE_LIST(FE_element)();
			}
		}
		fe_region->top_data_hack = 0;
		fe_region->data_fe_region = (struct FE_region *)NULL;
		fe_region->base_fe_region = (struct FE_region *)NULL;
		fe_region->cmiss_region = (struct Cmiss_region *)NULL;
		fe_region->fe_field_info = (struct FE_field_info *)NULL;

		/* change log information */
		fe_region->change_level = 0;
		fe_region->number_of_clients = 0;
		fe_region->change_callback_list =
			CREATE(LIST(CMISS_CALLBACK_ITEM(FE_region_change)))();
		fe_region->fe_field_changes = (struct CHANGE_LOG(FE_field) *)NULL;
		fe_region->fe_node_changes = (struct CHANGE_LOG(FE_node) *)NULL;
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			fe_region->fe_element_changes[dim] = (struct CHANGE_LOG(FE_element) *)NULL;
		}
		fe_region->last_fe_node_field_info = (struct FE_node_field_info *)NULL;
		fe_region->last_fe_element_field_info =
			(struct FE_element_field_info *)NULL;
		FE_region_create_change_logs(fe_region);

		/* information for defining faces */
		fe_region->element_type_node_sequence_list =
			(struct LIST(FE_element_type_node_sequence) *)NULL;

#if defined (EMBEDDING_CODE)
		/* embedding information */
		fe_region->embedded_fe_field_list = (struct LIST(FE_field) *)NULL;
		fe_region->embedding_fe_region_list =
			(struct LIST(FE_region_embedding_usage_count) *)NULL;
#endif /* defined (EMBEDDING_CODE) */

		fe_region->next_fe_node_identifier_cache = 0;
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			fe_region->next_fe_element_identifier_cache[dim] = 1;
		}

		fe_region->informed_make_cmiss_number_field = 0;
		fe_region->informed_make_xi_field = 0;

		fe_region->access_count = 0;
		if (!((master_fe_region ||
					(fe_region->fe_time &&
						fe_region->fe_field_list &&
						fe_region->fe_node_field_info_list &&
						fe_region->fe_element_field_info_list)) &&
				fe_region->fe_node_list &&
				fe_region->fe_element_list &&
				fe_region->change_callback_list &&
				fe_region->fe_field_changes &&
				fe_region->fe_node_changes &&
				fe_region->fe_element_changes))
		{
			display_message(ERROR_MESSAGE,
				"CREATE(FE_region).  Could not create objects in region");
			DESTROY(FE_region)(&fe_region);
			fe_region = (struct FE_region *)NULL;
		}
	}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(FE_region).  Could not allocate memory");
		}
	LEAVE;

	return (fe_region);
} /* CREATE(FE_region) */

struct FE_region *FE_region_get_data_FE_region(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 24 October 2008

DESCRIPTION :
Gets the data FE_region for <fe_region>, which has an additional set
of discrete data objects, just like nodes, which we call "data". Data points
in the data_FE_region share the same fields as <fe_region>.
Returns NULL with error if <fe_region> is itself a data region.
==============================================================================*/
{
	struct FE_region *data_fe_region, *data_master_fe_region;

	ENTER(FE_region_get_data_FE_region);
	data_fe_region = (struct FE_region *)NULL;
	if (fe_region)
	{
		if (!fe_region->base_fe_region)
		{
			if (fe_region->data_fe_region)
			{
				data_fe_region = fe_region->data_fe_region;
			}
			else
			{
				if (fe_region->master_fe_region)
				{
					data_master_fe_region =
						FE_region_get_data_FE_region(fe_region->master_fe_region);
				}
				else
				{
					data_master_fe_region = fe_region;
				}
				if (data_fe_region = CREATE(FE_region)(data_master_fe_region,
					(struct MANAGER(FE_basis) *)NULL, (struct LIST(FE_element_shape) *)NULL))
				{
					fe_region->data_fe_region = ACCESS(FE_region)(data_fe_region);
					data_fe_region->base_fe_region = fe_region;
					/* decrement access count of master to avoid circular reference */
					data_master_fe_region->access_count--;
					if (data_master_fe_region == fe_region)
					{
						data_fe_region->top_data_hack = 1;
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_data_FE_region.  Invalid argument(s)");
	}
	LEAVE;

	return (data_fe_region);
} /* FE_region_get_data_FE_region */

int DESTROY(FE_region)(struct FE_region **fe_region_address)
/*******************************************************************************
LAST MODIFIED : 5 June 2003

DESCRIPTION :
Frees the memory for the FE_region and sets <*fe_region_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;
#if defined (EMBEDDING_CODE)
	struct FE_region_embedding_usage_count *embedding_fe_region;
#endif /* defined (EMBEDDING_CODE) */

	ENTER(DESTROY(FE_region));
	if (fe_region_address && (fe_region = *fe_region_address))
	{
		if (0 == fe_region->access_count)
		{
			if (0 != fe_region->change_level)
			{
				display_message(WARNING_MESSAGE,
					"DESTROY(FE_region).  Non-zero change_level %d",
					fe_region->change_level);
			}

			if (fe_region->data_fe_region)
			{
				DEACCESS(FE_region)(&fe_region->data_fe_region);
			}
#if defined (EMBEDDING_CODE)
			/* clean up embedding information */
			if (fe_region->embedded_fe_field_list)
			{
				DESTROY(LIST(FE_field))(&(fe_region->embedded_fe_field_list));
			}
			if (fe_region->embedding_fe_region_list)
			{
				return_code = 1;
				while (return_code && (embedding_fe_region =
					FIRST_OBJECT_IN_LIST_THAT(FE_region_embedding_usage_count)(
						(LIST_CONDITIONAL_FUNCTION() *)NULL, (void *)NULL,
						fe_region->embedding_fe_region_list)))
				{
					if (fe_region_embedding_usage->fe_region != fe_region)
					{
						/*???RC REMOVE CALLBACKS from other regions HERE */
					}
					return_code = REMOVE_OBJECT_FROM_LIST(FE_region_embedding_usage_count)
						(fe_region_embedding_usage, fe_region->embedding_fe_region_list);
				}
				DESTROY(LIST(FE_region_embedding_usage_count))(
					*(fe_region->embedding_fe_region_list));
			}
#endif /* defined (EMBEDDING_CODE) */

			DESTROY(LIST(CMISS_CALLBACK_ITEM(FE_region_change)))(
				&(fe_region->change_callback_list));
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
			{
				DESTROY(LIST(FE_element))(&(fe_region->fe_element_list[dim]));
			}
			DESTROY(LIST(FE_node))(&(fe_region->fe_node_list));
			if (fe_region->master_fe_region)
			{
				/* remove callbacks from master_fe_region */
				FE_region_remove_callback(fe_region->master_fe_region,
					FE_region_master_fe_region_change, (void *)fe_region);
				/* master_fe_region not accessed if this is a data FE_region */
				if (!fe_region->base_fe_region)
				{
					DEACCESS(FE_region)(&(fe_region->master_fe_region));
				}
			}
			if (fe_region->fe_element_field_info_list)
			{
				/* remove pointers to this fe_region because being destroyed */
				FOR_EACH_OBJECT_IN_LIST(FE_element_field_info)(
					FE_element_field_info_clear_FE_region, (void *)NULL,
					fe_region->fe_element_field_info_list);
				DESTROY(LIST(FE_element_field_info))(
					&(fe_region->fe_element_field_info_list));
			}
			if (fe_region->fe_node_field_info_list)
			{
				/* remove pointers to this fe_region because being destroyed */
				FOR_EACH_OBJECT_IN_LIST(FE_node_field_info)(
					FE_node_field_info_clear_FE_region, (void *)NULL,
					fe_region->fe_node_field_info_list);
				DESTROY(LIST(FE_node_field_info))(
					&(fe_region->fe_node_field_info_list));
			}
			if (fe_region->fe_field_info)
			{
				/* remove its pointer to this fe_region because being destroyed */
				FE_field_info_clear_FE_region(fe_region->fe_field_info);
				DEACCESS(FE_field_info)(&(fe_region->fe_field_info));
			}
			if (fe_region->basis_manager && fe_region->region_owns_basis_manager)
			{
				DESTROY(MANAGER(FE_basis))(&fe_region->basis_manager);
			}
			if (fe_region->element_shape_list && fe_region->region_owns_element_shape_list)
			{
				DESTROY(LIST(FE_element_shape))(&fe_region->element_shape_list);
			}
			if (fe_region->fe_field_list)
			{
				DESTROY(LIST(FE_field))(&(fe_region->fe_field_list));
			}
			if (fe_region->fe_time)
			{
				DESTROY(FE_time_sequence_package)(&(fe_region->fe_time));
			}
			DESTROY(CHANGE_LOG(FE_field))(&(fe_region->fe_field_changes));
			DESTROY(CHANGE_LOG(FE_node))(&(fe_region->fe_node_changes));
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
			{
				DESTROY(CHANGE_LOG(FE_element))(&(fe_region->fe_element_changes[dim]));
			}
			DEALLOCATE(*fe_region_address);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(FE_region).  Non-zero access count");
			return_code = 0;
		}
		*fe_region_address = (struct FE_region *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE, "DESTROY(FE_region).  Missing FE_region");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_region) */

DECLARE_OBJECT_FUNCTIONS(FE_region)

DEFINE_ANY_OBJECT(FE_region)

int FE_region_begin_change(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 10 December 2002

DESCRIPTION :
Increments the change counter in <region>. No update callbacks will occur until
change counter is restored to zero by calls to FE_region_end_change.
Automatically calls the same function for any master_FE_region.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_begin_change);
	if (fe_region)
	{
		fe_region->change_level++;
		if (fe_region->master_fe_region)
		{
			FE_region_begin_change(fe_region->master_fe_region);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_begin_change.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_begin_change */

int FE_region_end_change(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 11 December 2002

DESCRIPTION :
Decrements the change counter in <region>. No update callbacks occur until the
change counter is restored to zero by this function.
Automatically calls the same function for any master_FE_region.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_end_change);
	if (fe_region)
	{
		if (0 < fe_region->change_level)
		{
			/* end changes on master_FE_region first so any callbacks received from
				 master are combined with those in fe_region itself */
			if (fe_region->master_fe_region)
			{
				FE_region_end_change(fe_region->master_fe_region);
			}
			fe_region->change_level--;
			if (0 == fe_region->change_level)
			{
				FE_region_update(fe_region);
			}
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_end_change.  Change not enabled");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_end_change.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_end_change */

int FE_region_add_callback(struct FE_region *fe_region,
	CMISS_CALLBACK_FUNCTION(FE_region_change) *function, void *user_data)
/*******************************************************************************
LAST MODIFIED : 16 December 2002

DESCRIPTION :
Adds a callback to <region> so that when it changes <function> is called with
<user_data>. <function> has 3 arguments, a struct FE_region *, a
struct FE_region_changes * and the void *user_data.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_add_callback);
	if (fe_region && function)
	{
		if (CMISS_CALLBACK_LIST_ADD_CALLBACK(FE_region_change)(
			fe_region->change_callback_list, function, user_data))
		{
			fe_region->number_of_clients++;
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_add_callback.  Could not add callback");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_add_callback.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_add_callback */

int FE_region_remove_callback(struct FE_region *fe_region,
	CMISS_CALLBACK_FUNCTION(FE_region_change) *function, void *user_data)
/*******************************************************************************
LAST MODIFIED : 16 December 2002

DESCRIPTION :
Removes the callback calling <function> with <user_data> from <region>.
==============================================================================*/
{
	int return_code = 1;

	ENTER(FE_region_remove_callback);
	if (fe_region && function)
	{
		if (fe_region->change_callback_list)
		{
			if (CMISS_CALLBACK_LIST_REMOVE_CALLBACK(FE_region_change)(
						fe_region->change_callback_list, function, user_data))
			{
				fe_region->number_of_clients--;
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_remove_callback.  Could not remove callback");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_callback.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_callback */

static int FE_region_remove_FE_element_private(struct FE_region *fe_region,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 7 April 2003

DESCRIPTION :
Removes <element> and all its faces that are not shared with other elements
from <fe_region>. Should enclose call between FE_region_begin_change and
FE_region_end_change to minimise messages.
This function is recursive.
==============================================================================*/
{
	int return_code;
	struct FE_element *face, *parent_element;

	ENTER(FE_region_remove_FE_element_private);
	int dimension = get_FE_element_dimension(element);
	if (fe_region && element && (1 <= dimension) &&
		(dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		return_code = 1;
		struct LIST(FE_element) *element_list = FE_region_get_element_list(fe_region, dimension);
		if (IS_OBJECT_IN_LIST(FE_element)(element, element_list))
		{
			/* access element in case it is only accessed here */
			ACCESS(FE_element)(element);
			if (return_code =
				REMOVE_OBJECT_FROM_LIST(FE_element)(element, element_list))
			{
				/* for master_FE_region, remove face elements from all their parents;
					 mark parent elements as OBJECT_NOT_IDENTIFIER_CHANGED */
				if ((struct FE_region *)NULL == fe_region->master_fe_region)
				{
					int face_number;
					while (return_code && (return_code =
						FE_element_get_first_parent(element, &parent_element, &face_number))
						&& parent_element)
					{
						return_code = set_FE_element_face(parent_element, face_number,
							(struct FE_element *)NULL);
						FE_REGION_FE_ELEMENT_CHANGE(fe_region, parent_element,
							CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element),
							parent_element);
					}
				}
				/* remove faces used by no elements in this fe_region */
				if (dimension > 1)
				{
					int number_of_faces = 0;
					get_FE_element_number_of_faces(element, &number_of_faces);
					for (int i = 0; (i < number_of_faces) && return_code; i++)
					{
						if (get_FE_element_face(element, i, &face) && face)
						{
							int number_of_parents = 0;
							if ((return_code = get_FE_element_number_of_parents_in_list(face,
								element_list, &number_of_parents)) &&
								(0 == number_of_parents))
							{
								return_code =
									FE_region_remove_FE_element_private(fe_region, face);
							}
						}
					}
				}
				FE_REGION_FE_ELEMENT_CHANGE(fe_region, element,
					CHANGE_LOG_OBJECT_REMOVED(FE_element), element);
			}
			DEACCESS(FE_element)(&element);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_element_private.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_element_private */

int FE_region_clear(struct FE_region *fe_region, int destroy_in_master)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Removes all the fields, nodes and elements from <fe_region>.
If <fe_region> has a master FE_region, its fields, nodes and elements will
still be owned by the master, unless <destroy_in_master> is set, which causes
the nodes and elements in this <fe_region> to definitely be destroyed.
Note this function uses FE_region_begin/end_change so it sends a single change
message if not already in the middle of changes.
???RC This could be made faster.
==============================================================================*/
{
	int return_code;
	struct FE_field *fe_field;
	struct FE_region *remove_from_fe_region;
	struct LIST(FE_node) *fe_node_list;

	ENTER(FE_region_clear);
	if (fe_region)
	{
		return_code = 1;
		remove_from_fe_region = fe_region;
		if (destroy_in_master)
		{
			FE_region_get_ultimate_master_FE_region(fe_region, &remove_from_fe_region);
		}
		FE_region_begin_change(remove_from_fe_region);
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			struct FE_element *element;
			while ((element = FIRST_OBJECT_IN_LIST_THAT(FE_element)(
				(LIST_CONDITIONAL_FUNCTION(FE_element) *)NULL, (void *)NULL,
				fe_region->fe_element_list[dim])))
			{
				if (!FE_region_remove_FE_element_private(fe_region, element))
				{
					return_code = 0;
					break;
				}
			}
		}
		/* FE_region_remove_FE_node_list may not work on list in FE_region */
		fe_node_list = CREATE(LIST(FE_node))();
		if (COPY_LIST(FE_node)(fe_node_list, fe_region->fe_node_list))
		{
			if (!FE_region_remove_FE_node_list(remove_from_fe_region, fe_node_list))
			{
				return_code = 0;
			}
		}
		DESTROY(LIST(FE_node))(&fe_node_list);
		/* only remove FE_fields if fe_region is its own master */
		if (fe_region->fe_field_list)
		{
			while (return_code && (fe_field = FIRST_OBJECT_IN_LIST_THAT(FE_field)(
				(LIST_CONDITIONAL_FUNCTION(FE_field) *)NULL, (void *)NULL,
				fe_region->fe_field_list)))
			{
				return_code =
					FE_region_remove_FE_field(remove_from_fe_region, fe_field);
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "FE_region_clear.  Failed");
			return_code = 0;
		}
		FE_region_end_change(remove_from_fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_clear.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_clear */

struct FE_field *FE_region_get_FE_field_with_general_properties(
	struct FE_region *fe_region, const char *name, enum Value_type value_type,
	int number_of_components)
{
	struct FE_field *fe_field;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_field_with_general_properties);
	fe_field = (struct FE_field *)NULL;
	if (fe_region && name && (0 < number_of_components))
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		fe_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(name,
			master_fe_region->fe_field_list);
		if (fe_field)
		{
			if ((get_FE_field_FE_field_type(fe_field) != GENERAL_FE_FIELD) ||
				(get_FE_field_value_type(fe_field) != value_type) ||
				(get_FE_field_number_of_components(fe_field) != number_of_components))
			{
				fe_field = (struct FE_field *)NULL;
			}
		}
		else
		{
			fe_field = CREATE(FE_field)(name, master_fe_region);
			if (!(set_FE_field_value_type(fe_field, value_type) &&
				set_FE_field_number_of_components(fe_field, number_of_components) &&
				set_FE_field_type_general(fe_field) &&
				FE_region_merge_FE_field(master_fe_region, fe_field)))
			{
				DESTROY(FE_field)(&fe_field);
				fe_field = (struct FE_field *)NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_field_with_general_properties.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_field);
}

struct FE_field *FE_region_get_FE_field_with_properties(
	struct FE_region *fe_region, const char *name, enum FE_field_type fe_field_type,
	struct FE_field *indexer_field, int number_of_indexed_values,
	enum CM_field_type cm_field_type, struct Coordinate_system *coordinate_system,
	enum Value_type value_type, int number_of_components, char **component_names,
	int number_of_times, enum Value_type time_value_type,
	struct FE_field_external_information *external)
{
	char *component_name;
	int i;
	struct FE_field *fe_field;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_field_with_properties);
	fe_field = (struct FE_field *)NULL;
	if (fe_region && name && coordinate_system && (0 < number_of_components))
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		/* search the FE_region for a field of that name */
		if (fe_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(name,
			master_fe_region->fe_field_list))
		{
			/* make sure the field matches in every way */
			if (!FE_field_matches_description(fe_field, name, fe_field_type,
				indexer_field, number_of_indexed_values, cm_field_type,
				coordinate_system, value_type, number_of_components, component_names,
				number_of_times, time_value_type, external))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_get_FE_field_with_properties.  "
					"Inconsistent with field of same name in region");
				fe_field = (struct FE_field *)NULL;
			}
		}
		else
		{
			if ((fe_field = CREATE(FE_field)(name, master_fe_region)) &&
				set_FE_field_external_information(fe_field, external) &&						
				set_FE_field_value_type(fe_field, value_type) &&
				set_FE_field_number_of_components(fe_field, number_of_components) &&
				((CONSTANT_FE_FIELD != fe_field_type) ||
					set_FE_field_type_constant(fe_field)) &&
				((GENERAL_FE_FIELD != fe_field_type) ||
					set_FE_field_type_general(fe_field)) &&
				((INDEXED_FE_FIELD != fe_field_type) ||
					set_FE_field_type_indexed(fe_field, 
						indexer_field,number_of_indexed_values)) &&
				set_FE_field_CM_field_type(fe_field, cm_field_type) &&
				set_FE_field_coordinate_system(fe_field, coordinate_system) &&
				set_FE_field_time_value_type(fe_field, time_value_type) &&
				set_FE_field_number_of_times(fe_field, number_of_times))
			{
				if (component_names)
				{
					for (i = 0; (i < number_of_components) && fe_field; i++)
					{
						if (component_name = component_names[i])
						{
							if (!set_FE_field_component_name(fe_field, i, component_name))
							{
								DESTROY(FE_field)(&fe_field);	
								fe_field = (struct FE_field *)NULL;
							}
						}
					}
				}
			}
			else
			{
				DESTROY(FE_field)(&fe_field);
				fe_field = (struct FE_field *)NULL;
			}
			if (fe_field)
			{
				if (!FE_region_merge_FE_field(master_fe_region, fe_field))
				{
					DESTROY(FE_field)(&fe_field);
					fe_field = (struct FE_field *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_get_FE_field_with_properties.  "
					"Could not create new field");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_field_with_properties.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_field);
}


struct FE_field *FE_region_merge_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 27 March 2003

DESCRIPTION :
Checks <fe_field> is compatible with <fe_region> and any existing FE_field
using the same identifier, then merges it into <fe_region>.
If no FE_field of the same identifier exists in FE_region, <fe_field> is added
to <fe_region> and returned by this function, otherwise changes are merged into
the existing FE_field and it is returned.
A NULL value is returned on any error.
==============================================================================*/
{
	struct FE_field *merged_fe_field;
	struct FE_region *master_fe_region;

	ENTER(FE_region_merge_FE_field);
	merged_fe_field = (struct FE_field *)NULL;
	if (fe_region && fe_field)
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		/* check fe_field belongs to master_fe_region */
		if (FE_field_get_FE_region(fe_field) == master_fe_region)
		{
			if (merged_fe_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(
				get_FE_field_name(fe_field), master_fe_region->fe_field_list))
			{
				/* no change needs to be noted if fields are exactly the same */
				if (!FE_fields_match_exact(merged_fe_field, fe_field))
				{
					/* can only change fundamentals -- number of components, value type
						 if merged_fe_field is not accessed by any other objects */
					if ((1 == FE_field_get_access_count(merged_fe_field)) ||
						FE_fields_match_fundamental(merged_fe_field, fe_field))
					{
						if (FE_field_copy_without_identifier(merged_fe_field, fe_field))
						{
#if defined (DEBUG)
					/*???debug*/printf("FE_region_merge_FE_field: %p OBJECT_NOT_IDENTIFIER_CHANGED field %p\n",fe_region,merged_fe_field);
#endif /* defined (DEBUG) */
							FE_REGION_FE_FIELD_CHANGE(master_fe_region, merged_fe_field,
								CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_field));
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_region_merge_FE_field.  Could not modify field");
							merged_fe_field = (struct FE_field *)NULL;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
							"Existing field named %s is different",
							get_FE_field_name(merged_fe_field));
						merged_fe_field = (struct FE_field *)NULL;
					}
				}
			}
			else
			{
				if (ADD_OBJECT_TO_LIST(FE_field)(fe_field,
					master_fe_region->fe_field_list))
				{
					merged_fe_field = fe_field;
#if defined (DEBUG)
					/*???debug*/printf("FE_region_merge_FE_field: %p ADD field %p\n",master_fe_region,merged_fe_field);
#endif /* defined (DEBUG) */
					FE_REGION_FE_FIELD_CHANGE(master_fe_region, merged_fe_field,
						CHANGE_LOG_OBJECT_ADDED(FE_field));
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
						"Could not add field %s", get_FE_field_name(fe_field));
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
					"Field '%s' is not compatible with this finite element region",
				get_FE_field_name(fe_field));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_field.  Invalid argument(s)");
	}
	LEAVE;

	return (merged_fe_field);
} /* FE_region_merge_FE_field */

int FE_region_is_FE_field_in_use(struct FE_region *fe_region,
	struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Returns true if <FE_field> is defined on any nodes and element in <fe_region>.
==============================================================================*/
{
	char *field_name;
	int return_code;
	struct FE_region *master_fe_region, *referenced_fe_region;

	ENTER(FE_region_is_FE_field_in_use);
	return_code = 1;
	if (fe_region && fe_field)
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, master_fe_region->fe_field_list))
		{
			return_code = 0;
			if (FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
				FE_node_field_info_has_FE_field, (void *)fe_field,
				master_fe_region->fe_node_field_info_list))
			{
				/* since nodes may still exist in the change_log or slave FE_regions,
					 must now check that no remaining nodes use fe_field */
				/*???RC For now, if there are nodes then fe_field is in use */
				if (0 < NUMBER_IN_LIST(FE_node)(master_fe_region->fe_node_list))
				{
					return_code = 1;
				}
			}
			if ((!return_code) && ((struct FE_element_field_info *)NULL !=
				FIRST_OBJECT_IN_LIST_THAT(FE_element_field_info)(
					FE_element_field_info_has_FE_field, (void *)fe_field,
					master_fe_region->fe_element_field_info_list)))
			{
				/* since elements may still exist in the change_log or slave FE_regions,
					 must now check that no remaining elements use fe_field */
				/* for now, if there are elements then fe_field is in use */
				return_code = (0 < FE_region_get_number_of_FE_elements_all_dimensions(master_fe_region));
			}
		}
		else
		{
			referenced_fe_region = FE_field_get_FE_region(fe_field);
			if ((referenced_fe_region != NULL) && (referenced_fe_region != master_fe_region))
			{
				GET_NAME(FE_field)(fe_field, &field_name);
				display_message(ERROR_MESSAGE,
					"FE_region_is_FE_field_in_use.  Field %s is from another region",
					field_name);
				DEALLOCATE(field_name);
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_is_FE_field_in_use.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_is_FE_field_in_use */

int FE_region_remove_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Removes <fe_field> from <fe_region>.
Fields can only be removed if not defined on any nodes and element in
<fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_remove_FE_field);
	return_code = 0;
	if (fe_region && fe_field)
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, master_fe_region->fe_field_list))
		{
			if (FE_region_is_FE_field_in_use(master_fe_region, fe_field))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_remove_FE_field.  Field is in use in region");
				/*???RC Could undefine it at nodes and elements */
			}
			else
			{
				/* access field in case it is only accessed here */
				ACCESS(FE_field)(fe_field);
				if (return_code = REMOVE_OBJECT_FROM_LIST(FE_field)(fe_field,
					master_fe_region->fe_field_list))
				{
					FE_REGION_FE_FIELD_CHANGE(master_fe_region, fe_field,
						CHANGE_LOG_OBJECT_REMOVED(FE_field));
				}
				DEACCESS(FE_field)(&fe_field);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_remove_FE_field.  Field %p is not in region %p (master %p)",
				fe_field, fe_region, master_fe_region);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_field.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_field */

struct FE_field *FE_region_get_FE_field_from_name(struct FE_region *fe_region,
	const char *field_name)
/*******************************************************************************
LAST MODIFIED : 15 October 2002

DESCRIPTION :
Returns the field of <field_name> in <fe_region>, or NULL without error if none.
==============================================================================*/
{
	struct FE_field *field;

	ENTER(FE_region_get_FE_field_from_name);
	if (fe_region && field_name)
	{
		if (fe_region->master_fe_region)
		{
			field = FE_region_get_FE_field_from_name(fe_region->master_fe_region,
				field_name);
		}
		else
		{
			field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(field_name,
				fe_region->fe_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_field_from_name.  Invalid argument(s)");
		field = (struct FE_field *)NULL;
	}
	LEAVE;

	return (field);
} /* FE_region_get_FE_field_from_name */

int FE_region_set_FE_field_name(struct FE_region *fe_region,
	struct FE_field *field, const char *new_name)
{
	int return_code;

	ENTER(FE_region_set_FE_field_name);
	if (fe_region && field && is_standard_object_name(new_name))
	{
		struct FE_region *master_fe_region = fe_region;
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region);
		if (FE_region_contains_FE_field(master_fe_region, field))
		{
			return_code = 1;
			if (FE_region_get_FE_field_from_name(master_fe_region, new_name))
			{
				display_message(ERROR_MESSAGE, "FE_region_set_FE_field_name.  "
					"Field named \"%s\" already exists in this FE_region.", new_name);
				return_code = 0;
			}
			else
			{
				// this temporarily removes the object from all indexed lists
				if (LIST_BEGIN_IDENTIFIER_CHANGE(FE_field,name)(
					fe_region->fe_field_list, field))
				{
					return_code = set_FE_field_name(field, new_name);
					LIST_END_IDENTIFIER_CHANGE(FE_field,name)(fe_region->fe_field_list);
					if (return_code)
					{
						FE_REGION_FE_FIELD_CHANGE(master_fe_region, field,
							CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_field));
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_region_set_FE_field_name.  "
						"Could not safely change identifier in indexed lists");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_set_FE_field_name.  Field is not from this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_set_FE_field_name.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

int FE_region_contains_FE_field(struct FE_region *fe_region,
	struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 9 December 2002

DESCRIPTION :
Returns true if <field> is in <fe_region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_contains_FE_field);
	if (fe_region && field)
	{
		if (IS_OBJECT_IN_LIST(FE_field)(field, fe_region->fe_field_list))
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
		display_message(ERROR_MESSAGE,
			"FE_region_contains_FE_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_contains_FE_field */

struct FE_field *FE_region_get_first_FE_field_that(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_field) *conditional_function,
	void *user_data_void)
/*******************************************************************************
LAST MODIFIED : 25 March 2003

DESCRIPTION :
Returns the first field in <fe_region> that satisfies <conditional_function>
with <user_data_void>.
A NULL <conditional_function> returns the first FE_field in <fe_region>, if any.
==============================================================================*/
{
	struct FE_field *fe_field;

	ENTER(FE_region_get_first_FE_field_that);
	if (fe_region)
	{
		if (fe_region->master_fe_region)
		{
			fe_field = FE_region_get_first_FE_field_that(fe_region->master_fe_region,
				conditional_function, user_data_void);
		}
		else
		{
			fe_field = FIRST_OBJECT_IN_LIST_THAT(FE_field)(conditional_function,
				user_data_void, fe_region->fe_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_first_FE_field_that.  Invalid argument(s)");
		fe_field = (struct FE_field *)NULL;
	}
	LEAVE;

	return (fe_field);
} /* FE_region_get_first_FE_field_that */

int FE_region_for_each_FE_field(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_field) iterator_function, void *user_data)
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
Calls <iterator_function> with <user_data> for each FE_field in <region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_for_each_FE_field);
	if (fe_region && iterator_function)
	{
		if (fe_region->master_fe_region)
		{
			return_code = FE_region_for_each_FE_field(fe_region->master_fe_region,
				iterator_function, user_data);
		}
		else
		{
			return_code = FOR_EACH_OBJECT_IN_LIST(FE_field)(iterator_function,
				user_data, fe_region->fe_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_for_each_FE_field */

int FE_region_get_number_of_FE_fields(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 10 January 2003

DESCRIPTION :
Returns the number of FE_fields in <fe_region>.
==============================================================================*/
{
	int number_of_fields;

	ENTER(FE_region_get_number_of_FE_fields);
	if (fe_region)
	{
		if (fe_region->master_fe_region)
		{
			number_of_fields =
				FE_region_get_number_of_FE_fields(fe_region->master_fe_region);
		}
		else
		{
			number_of_fields = NUMBER_IN_LIST(FE_field)(fe_region->fe_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_fields.  Invalid argument(s)");
		number_of_fields = 0;
	}
	LEAVE;

	return (number_of_fields);
} /* FE_region_get_number_of_FE_fields */

int set_FE_field_component_FE_region(struct Parse_state *state,
	void *fe_field_component_address_void, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 6 March 2003

DESCRIPTION :
FE_region wrapper for set_FE_field_component.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region, *master_fe_region;

	ENTER(set_FE_field_component_FE_region);
	if (state && fe_field_component_address_void &&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		/* get the ultimate master FE_region; only it has field info */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		return_code = set_FE_field_component(state, fe_field_component_address_void,
			(void *)(master_fe_region->fe_field_list));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_component_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_component_FE_region */

int set_FE_field_conditional_FE_region(struct Parse_state *state,
	void *fe_field_address_void, void *parse_field_data_void)
/*******************************************************************************
LAST MODIFIED : 27 February 2003

DESCRIPTION :
FE_region wrapper for set_FE_field_conditional. <parse_field_data_void> points
at a struct Set_FE_field_conditional_FE_region_data.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region, *master_fe_region;
	struct Set_FE_field_conditional_data set_field_data;
	struct Set_FE_field_conditional_FE_region_data *parse_field_data;

	ENTER(set_FE_field_conditional_FE_region);
	if (state && fe_field_address_void && (parse_field_data =
		(struct Set_FE_field_conditional_FE_region_data *)parse_field_data_void)
		&& (fe_region = parse_field_data->fe_region))
	{
		/* get the ultimate master FE_region; only it has field info */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		set_field_data.conditional_function =
			parse_field_data->conditional_function;
		set_field_data.conditional_function_user_data =
			parse_field_data->user_data;
		set_field_data.fe_field_list = master_fe_region->fe_field_list;
		return_code = set_FE_field_conditional(state, fe_field_address_void,
			(void *)&set_field_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_conditional_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_conditional_FE_region */

int set_FE_fields_FE_region(struct Parse_state *state,
	void *fe_field_order_info_address_void, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 7 March 2003

DESCRIPTION :
FE_region wrapper for set_FE_fields.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region, *master_fe_region;

	ENTER(set_FE_fields_FE_region);
	if (state && fe_field_order_info_address_void &&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		/* get the ultimate master FE_region; only it has fe_field_list */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		return_code = set_FE_fields(state, fe_field_order_info_address_void,
			(void *)(master_fe_region->fe_field_list));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_fields_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_fields_FE_region */

int Option_table_add_set_FE_field_from_FE_region(
	struct Option_table *option_table, const char *entry_string,
	struct FE_field **fe_field_address, struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 11 March 2003

DESCRIPTION :
Adds an entry for selecting an FE_field.
==============================================================================*/
{
	int return_code = 0;
	struct FE_region *master_fe_region;

	ENTER(Option_table_add_set_FE_field_from_FE_region);
	if (option_table && entry_string && fe_field_address && fe_region)
	{
		/* get the ultimate master FE_region; only it has field info */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		Option_table_add_entry(option_table, entry_string,
			(void *)fe_field_address, master_fe_region->fe_field_list,
			set_FE_field);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Option_table_add_set_FE_field_from_FE_region.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* Option_table_add_set_FE_field_from_FE_region */

int FE_region_FE_field_has_multiple_times(struct FE_region *fe_region,
	struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 26 February 2003

DESCRIPTION :
Returns true if <fe_field> is defined with multiple times on any node or element
in the ultimate master FE_region of <fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_FE_field_has_multiple_times);
	return_code = 0;
	if (fe_region && fe_field)
	{
		/* get the ultimate master FE_region; only it has field info */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		/* currently only node fields can be time aware */
		if (FIRST_OBJECT_IN_LIST_THAT(FE_node_field_info)(
			FE_node_field_info_has_FE_field_with_multiple_times,
				(void *)fe_field, master_fe_region->fe_node_field_info_list))
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_FE_field_has_multiple_times.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_FE_field_has_multiple_times */

int FE_region_get_default_coordinate_FE_field(struct FE_region *fe_region,
	struct FE_field **fe_field)
/*******************************************************************************
LAST MODIFIED : 12 May 2003

DESCRIPTION :
Returns an <fe_field> which the <fe_region> considers to be its default
coordinate field or returns 0 and sets *<fe_field> to NULL if it has no 
"coordinate" fields.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_get_default_coordinate_FE_field);
	return_code = 0;
	if (fe_region && fe_field)
	{
		if (!fe_region->master_fe_region)
		{
			/* This is a proper FE_region, find the first FE_field
				that is a coordinate field */
			if (*fe_field = FE_region_get_first_FE_field_that(
				fe_region, FE_field_is_coordinate_field, (void *)NULL))
			{
				return_code = 1;
			}
		}
		else
		{
			/* This is a slave region, do legacy behaviour */
			*fe_field = (struct FE_field *)NULL;
			/* Look for an element field */
			for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; (NULL == *fe_field) && (0 <= dim); --dim)
			{
				FIRST_OBJECT_IN_LIST_THAT(FE_element)(
					FE_element_find_default_coordinate_field_iterator,
					(void *)fe_field, fe_region->fe_element_list[dim]);
			}
			/* Look for an node field */
			if (!(*fe_field))
			{
				FIRST_OBJECT_IN_LIST_THAT(FE_node)(
					FE_node_find_default_coordinate_field_iterator,
					(void *)fe_field, fe_region->fe_node_list);
			}
			if (*fe_field)
			{
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_default_coordinate_FE_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_get_default_coordinate_FE_field */

int FE_region_change_FE_node_identifier(struct FE_region *fe_region,
	struct FE_node *node, int new_identifier)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Gets <fe_region>, or its master_fe_region if it has one, to change the
identifier of <node> to <new_identifier>. Fails if the identifier is already
in use by an node in the same ultimate master FE_region.
???RC Needs recoding if FE_node changed from using indexed list.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_change_FE_node_identifier);
	if (fe_region && node)
	{
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
		{
			return_code = 1;
			struct FE_region *master_fe_region = fe_region;
			FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region); 
			if (FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
				new_identifier, master_fe_region->fe_node_list))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_change_FE_node_identifier.  "
					"Node with new identifier already exists");
				return_code = 0;
			}
			else
			{
				// this temporarily removes the object from all indexed lists
				if (LIST_BEGIN_IDENTIFIER_CHANGE(FE_node,cm_node_identifier)(
					master_fe_region->fe_node_list, node))
				{
					return_code = set_FE_node_identifier(node, new_identifier);
					LIST_END_IDENTIFIER_CHANGE(FE_node,cm_node_identifier)(
						master_fe_region->fe_node_list);
					if (return_code)
					{
						FE_REGION_FE_NODE_IDENTIFIER_CHANGE(master_fe_region, node);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_region_set_FE_field_name.  "
						"Could not safely change identifier in indexed lists");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_change_FE_node_identifier.  Node is not in this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_change_FE_node_identifier.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_change_FE_node_identifier */

struct FE_node *FE_region_get_FE_node_from_identifier(
	struct FE_region *fe_region, int identifier)
/*******************************************************************************
LAST MODIFIED : 25 February 2003

DESCRIPTION :
Returns the node of number <identifier> in <fe_region>, or NULL without error
if no such node found.
==============================================================================*/
{
	struct FE_node *node;

	ENTER(FE_region_get_FE_node_from_identifier);
	if (fe_region)
	{
		node = FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
			identifier, fe_region->fe_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_node_from_identifier.  Invalid argument(s)");
		node = (struct FE_node *)NULL;
	}
	LEAVE;

	return (node);
} /* FE_region_get_FE_node_from_identifier */

struct FE_node *FE_region_get_or_create_FE_node_with_identifier(
	struct FE_region *fe_region, int identifier)
/*******************************************************************************
LAST MODIFIED : 27 May 2003

DESCRIPTION :
Convenience function returning an existing node with <identifier> from
<fe_region> or any of its ancestors. If none is found, a new node with the
given <identifier> is created.
If the returned node is not already in <fe_region> it is merged before return.
It is expected that the calling function has wrapped calls to this function
with FE_region_begin/end_change.
==============================================================================*/
{
	struct FE_node *node;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_or_create_FE_node_with_identifier);
	node = (struct FE_node *)NULL;
	if (fe_region)
	{
		if (!(node = FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
			identifier, fe_region->fe_node_list)))
		{
			FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region); 
			if (master_fe_region != fe_region)
			{
				node = FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
					identifier, master_fe_region->fe_node_list);
			}
			if (!node)
			{
				node = CREATE(FE_node)(identifier, master_fe_region,
					/*template_node*/(struct FE_node *)NULL);
				if (!node)
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_or_create_FE_node_with_identifier.  "
						"Could not create node");
				}
			}
			if (node)
			{
				if (!FE_region_merge_FE_node(fe_region, node))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_or_create_FE_node_with_identifier.  "
						"Could not merge node");
					/* following cleans up node if newly created, and clears pointer */
					REACCESS(FE_node)(&node, (struct FE_node *)NULL);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_or_create_FE_node_with_identifier.  Invalid argument(s)");
	}
	LEAVE;

	return (node);
} /* FE_region_get_or_create_FE_node_with_identifier */

int FE_region_get_next_FE_node_identifier(struct FE_region *fe_region,
	int start_identifier)
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
Returns the next unused node identifier for <fe_region> starting from
<start_identifier>. Search is performed on the ultimate master FE_region for
<fe_region> since they share the same FE_node namespace.
==============================================================================*/
{
	int identifier;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_next_FE_node_identifier);
	if (fe_region)
	{
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region); 
		if (start_identifier <= 0)
		{
			identifier = 1;
		}
		else
		{
			identifier = start_identifier;
		}
		if (master_fe_region->next_fe_node_identifier_cache)
		{
			if (master_fe_region->next_fe_node_identifier_cache > identifier)
			{
				identifier = master_fe_region->next_fe_node_identifier_cache;
			}
		}
		while (FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(identifier,
			master_fe_region->fe_node_list))
		{
			identifier++;
		}
		if (start_identifier < 2)
		{
			/* Don't cache the value if we didn't start at the beginning */
			master_fe_region->next_fe_node_identifier_cache = identifier;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_next_FE_node_identifier.  Missing fe_region");
		identifier = 0;
	}
	LEAVE;

	return (identifier);
} /* FE_region_get_next_FE_node_identifier */

int FE_region_contains_FE_node(struct FE_region *fe_region,
	struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 9 December 2002

DESCRIPTION :
Returns true if <node> is in <fe_region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_contains_FE_node);
	if (fe_region && node)
	{
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
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
		display_message(ERROR_MESSAGE,
			"FE_region_contains_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_contains_FE_node */

int FE_region_contains_FE_node_conditional(struct FE_node *node,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
FE_node conditional function version of FE_region_contains_FE_node.
Returns true if <node> is in <fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;

	ENTER(FE_region_contains_FE_node_conditional);
	if (node && (fe_region = (struct FE_region *)fe_region_void))
	{
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
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
		display_message(ERROR_MESSAGE,
			"FE_region_contains_FE_node_conditional.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_contains_FE_node_conditional */

int FE_region_or_data_FE_region_contains_FE_node(struct FE_region *fe_region,
	struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 26 April 2005

DESCRIPTION :
Returns true if <node> is in <fe_region> or if the <fe_region> has a
data_FE_region attached to it in that attached region.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_or_data_FE_region_contains_FE_node);
	if (fe_region && node)
	{
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
		{
			return_code = 1;
		}
		else if (fe_region->data_fe_region && 
			IS_OBJECT_IN_LIST(FE_node)(node, fe_region->data_fe_region->fe_node_list))
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
		display_message(ERROR_MESSAGE,
			"FE_region_or_data_FE_region_contains_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_or_data_FE_region_contains_FE_node */

int FE_node_is_not_in_FE_region(struct FE_node *node, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 3 April 2003

DESCRIPTION :
Returns true if <node> is not in <fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;

	ENTER(FE_node_is_not_in_FE_region);
	if (node && (fe_region = (struct FE_region *)fe_region_void))
	{
		return_code = (!IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_is_not_in_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_is_not_in_FE_region */

int FE_region_define_FE_field_at_FE_node(struct FE_region *fe_region,
	struct FE_node *node, struct FE_field *fe_field,
	struct FE_time_sequence *fe_time_sequence,
	struct FE_node_field_creator *node_field_creator)
/*******************************************************************************
LAST MODIFIED : 28 April 2003

DESCRIPTION :
Checks <fe_region> contains <node>. If <fe_field> is already defined on it,
returns successfully, otherwise defines the field at the node using optional
<fe_time_sequence> and <node_field_creator>. Change messages are broadcast for
the ultimate master FE_region of <fe_region>.
Should place multiple calls to this function between begin_change/end_change.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_define_FE_field_at_FE_node);
	if (fe_region && node && fe_field && node_field_creator)
	{
		return_code = 1;
		if (!FE_field_is_defined_at_node(fe_field, node))
		{
			/* get the ultimate master FE_region; only it has FE_time */
			master_fe_region = fe_region;
			while (master_fe_region->master_fe_region)
			{
				master_fe_region = master_fe_region->master_fe_region;
			}
			if (FE_field_get_FE_region(fe_field) != master_fe_region)
			{
				display_message(ERROR_MESSAGE, "FE_region_define_FE_field_at_FE_node.  "
					"Field is not of this finite element region");
				return_code = 0;
			}
			if (fe_time_sequence && (!FE_time_sequence_package_has_FE_time_sequence(
				master_fe_region->fe_time, fe_time_sequence)))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_define_FE_field_at_FE_node.  "
					"Time version is not of this finite element region");
				return_code = 0;
			}
			if (return_code)
			{
				if (return_code = define_FE_field_at_node(node, fe_field,
					fe_time_sequence, node_field_creator))
				{
					if (FE_region_contains_FE_node(master_fe_region, node))
					{
						FE_REGION_FE_NODE_FIELD_CHANGE(master_fe_region, node, fe_field);
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_define_FE_field_at_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_define_FE_field_at_FE_node */

int FE_region_undefine_FE_field_at_FE_node(struct FE_region *fe_region,
	struct FE_node *node, struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 29 April 2003

DESCRIPTION :
Checks <fe_region> contains <node>. If <fe_field> is not defined on it,
returns successfully, otherwise undefines the field at the node. Change messages
are broadcast for the ultimate master FE_region of <fe_region>.
Should place multiple calls to this function between begin_change/end_change.
Note it is more efficient to use FE_region_undefine_FE_field_in_FE_node_list
for more than one node.
==============================================================================*/
{
	int number_in_elements, return_code;
	struct LIST(FE_node) *fe_node_list;

	ENTER(FE_region_undefine_FE_field_at_FE_node);
	if (fe_region && node && fe_field)
	{
		if (FE_region_contains_FE_node(fe_region, node))
		{
			return_code = 1;
			if (FE_field_is_defined_at_node(fe_field, node))
			{
				/* expensive; uses the node_list version */
				fe_node_list = CREATE(LIST(FE_node))();
				if (ADD_OBJECT_TO_LIST(FE_node)(node, fe_node_list))
				{
					return_code = FE_region_undefine_FE_field_in_FE_node_list(fe_region,
						fe_field, fe_node_list, &number_in_elements);
					if (0 < number_in_elements)
					{
						display_message(ERROR_MESSAGE,
							"FE_region_undefine_FE_field_at_FE_node.  "
							"Field in this node is in-use by element(s)");
						return_code = 0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_region_undefine_FE_field_at_FE_node.  Failed");
					return_code = 0;
				}
				DESTROY(LIST(FE_node))(&fe_node_list);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_undefine_FE_field_at_FE_node.  Node is not in region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_undefine_FE_field_at_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_undefine_FE_field_at_FE_node */

int FE_region_undefine_FE_field_in_FE_node_list(struct FE_region *fe_region,
	struct FE_field *fe_field, struct LIST(FE_node) *fe_node_list,
	int *number_in_elements_address)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Makes sure <fe_field> is not defined at any nodes in <fe_node_list> from
<fe_region>, unless that field at the node is in interpolated over an element
from <fe_region>.
Should wrap call to this function between begin_change/end_change.
<fe_node_list> is unchanged by this function.
On return, the integer at <number_in_elements_address> will be set to the
number of nodes for which <fe_field> could not be undefined because they are
used by element fields of <fe_field>.
==============================================================================*/
{
	int return_code;
	struct FE_node *node;
	struct FE_region *master_fe_region;
	struct LIST(FE_node) *tmp_fe_node_list;
	struct Node_list_field_data node_list_field_data;

	ENTER(FE_region_undefine_FE_field_in_FE_node_list);
	if (fe_region && fe_region && fe_node_list && number_in_elements_address)
	{
		/* first make sure all the nodes are from fe_region */
		if (FIRST_OBJECT_IN_LIST_THAT(FE_node)(FE_node_is_not_in_list,
			(void *)fe_region->fe_node_list, fe_node_list))
		{
			display_message(ERROR_MESSAGE,
				"FE_region_undefine_FE_field_in_FE_node_list.  "
				"Some nodes are not from this region");
			return_code = 0;
		}
		else
		{
			return_code = 1;
			/* get the ultimate master FE_region; it owns nodes/fields */
			master_fe_region = fe_region;
			while (master_fe_region->master_fe_region)
			{
				master_fe_region = master_fe_region->master_fe_region;
			}
			/* work with a copy of the node list */
			tmp_fe_node_list = CREATE(LIST(FE_node))();
			if (COPY_LIST(FE_node)(tmp_fe_node_list, fe_node_list))
			{
				FE_region_begin_change(master_fe_region);
				/* remove nodes for which fe_field is not defined */
				REMOVE_OBJECTS_FROM_LIST_THAT(FE_node)(
					FE_node_field_is_not_defined, (void *)fe_field, tmp_fe_node_list);
				*number_in_elements_address = NUMBER_IN_LIST(FE_node)(tmp_fe_node_list);
				/* remove nodes used in element fields for fe_field */
				node_list_field_data.fe_field = fe_field;
				node_list_field_data.fe_node_list = tmp_fe_node_list;
				if (FE_region_for_each_FE_element(master_fe_region,
					FE_element_ensure_FE_field_nodes_are_not_in_list,
					(void *)&node_list_field_data))
				{
					*number_in_elements_address -=
						NUMBER_IN_LIST(FE_node)(tmp_fe_node_list);
					while (return_code &&
						(node = FIRST_OBJECT_IN_LIST_THAT(FE_node)(
							(LIST_CONDITIONAL_FUNCTION(FE_node) *)NULL, (void *)NULL,
							tmp_fe_node_list)))
					{
						if (return_code = undefine_FE_field_at_node(node, fe_field))
						{
							return_code =
								REMOVE_OBJECT_FROM_LIST(FE_node)(node, tmp_fe_node_list);
							FE_REGION_FE_NODE_FIELD_CHANGE(master_fe_region, node, fe_field);
						}
					}
				}
				else
				{
					return_code = 0;
				}
				FE_region_end_change(master_fe_region);
			}
			else
			{
				return_code = 0;
			}
			DESTROY(LIST(FE_node))(&tmp_fe_node_list);
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"FE_region_undefine_FE_field_in_FE_node_list.  Failed");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_undefine_FE_field_in_FE_node_list.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_undefine_FE_field_in_FE_node_list */

struct FE_node *FE_region_create_FE_node_copy(struct FE_region *fe_region,
	int identifier, struct FE_node *source)
{
	int number;
	struct FE_node *new_node;

	ENTER(FE_region_create_FE_node_copy);
	new_node = (struct FE_node *)NULL;
	if (fe_region && source)
	{
		if (fe_region->master_fe_region)
		{
			FE_region_begin_change(fe_region);
			new_node = FE_region_create_FE_node_copy(
				fe_region->master_fe_region, identifier, source);
			if ((new_node) && ADD_OBJECT_TO_LIST(FE_node)(new_node,
				fe_region->fe_node_list))
			{
				FE_REGION_FE_NODE_CHANGE(fe_region, new_node,
					CHANGE_LOG_OBJECT_ADDED(FE_node), new_node);
			}
			FE_region_end_change(fe_region);
		}
		else if (FE_node_get_FE_region(source) == fe_region)
		{
			number = identifier;
			if (identifier <= 0)
			{
				number = FE_region_get_next_FE_node_identifier(fe_region, 0);
			}
			new_node = CREATE(FE_node)(number, (struct FE_region *)NULL, source);
			if (ADD_OBJECT_TO_LIST(FE_node)(new_node, fe_region->fe_node_list))
			{
				FE_REGION_FE_NODE_CHANGE(fe_region, new_node,
					CHANGE_LOG_OBJECT_ADDED(FE_node), new_node);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_create_FE_node_copy.  node identifier in use.");
				DESTROY(FE_node)(&new_node);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_create_FE_node_copy.  "
				"Source node is incompatible with region");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_create_FE_node_copy.  Invalid argument(s)");
	}
	LEAVE;

	return (new_node);
}

struct FE_node *FE_region_merge_FE_node(struct FE_region *fe_region,
	struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Checks <node> is compatible with <fe_region> and any existing FE_node
using the same identifier, then merges it into <fe_region>.
If no FE_node of the same identifier exists in FE_region, <node> is added
to <fe_region> and returned by this function, otherwise changes are merged into
the existing FE_node and it is returned.
During the merge, any new fields from <node> are added to the existing node of
the same identifier. Where those fields are already defined on the existing
node, the existing structure is maintained, but the new values are added from
<node>. Fails if fields are not consistent in numbers of versions and
derivatives etc.
A NULL value is returned on any error.
==============================================================================*/
{
	struct FE_node *merged_node;

	ENTER(FE_region_merge_FE_node);
	merged_node = (struct FE_node *)NULL;
	if (fe_region && node)
	{
		if (fe_region->master_fe_region && (!fe_region->top_data_hack))
		{
			/* begin/end change to prevent multiple messages */
			FE_region_begin_change(fe_region);
			if (merged_node =
				FE_region_merge_FE_node(fe_region->master_fe_region, node))
			{
				if (!IS_OBJECT_IN_LIST(FE_node)(merged_node, fe_region->fe_node_list))
				{
					if (ADD_OBJECT_TO_LIST(FE_node)(merged_node, fe_region->fe_node_list))
					{
						FE_REGION_FE_NODE_CHANGE(fe_region, merged_node,
							CHANGE_LOG_OBJECT_ADDED(FE_node), merged_node);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_region_merge_FE_node.  Could not add node %d",
							get_FE_node_identifier(merged_node));
						merged_node = (struct FE_node *)NULL;
					}
				}
			}
			FE_region_end_change(fe_region);
		}
		else
		{
			/* check node was created for this fe_region */
			if ((fe_region == FE_node_get_FE_region(node)) ||
				(fe_region->top_data_hack &&
					(fe_region->master_fe_region == FE_node_get_FE_region(node))))
			{
				if (merged_node =
					FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
						get_FE_node_identifier(node), fe_region->fe_node_list))
				{
					/* since we use merge to add existing nodes to list, often no
						 merge will be necessary, hence the following */
					if (merged_node != node)
					{
						/* merge fields from node into global merged_node */
						if (merge_FE_node(merged_node, node))
						{
							FE_REGION_FE_NODE_CHANGE(fe_region, merged_node,
								CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_node), node);
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_region_merge_FE_node.  Could not merge node %d",
								get_FE_node_identifier(merged_node));
							merged_node = (struct FE_node *)NULL;
						}
					}
				}
				else
				{
					if (ADD_OBJECT_TO_LIST(FE_node)(node, fe_region->fe_node_list))
					{
						merged_node = node;
						FE_REGION_FE_NODE_CHANGE(fe_region, merged_node,
							CHANGE_LOG_OBJECT_ADDED(FE_node), merged_node);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_region_merge_FE_node.  Could not add node %d",
							get_FE_node_identifier(merged_node));
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_merge_FE_node.  "
					"Node %d is not of this finite element region",
					get_FE_node_identifier(merged_node));
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_node.  Invalid argument(s)");
	}
	LEAVE;

	return (merged_node);
} /* FE_region_merge_FE_node */

int FE_region_merge_FE_node_existing(struct FE_region *fe_region,
	struct FE_node *destination, struct FE_node *source)
{
	int return_code;

	ENTER(FE_region_merge_FE_node_existing);
	return_code = 0;
	if (fe_region && destination && source)
	{
		if (fe_region->master_fe_region)
		{
			FE_region_begin_change(fe_region);
			if (FE_region_merge_FE_node_existing(fe_region->master_fe_region,
				destination, source))
			{
				if (!IS_OBJECT_IN_LIST(FE_node)(destination, fe_region->fe_node_list))
				{
					if (ADD_OBJECT_TO_LIST(FE_node)(destination,fe_region->fe_node_list))
					{
						FE_REGION_FE_NODE_CHANGE(fe_region, destination,
							CHANGE_LOG_OBJECT_ADDED(FE_node), destination);
					}
				}
			}
			FE_region_end_change(fe_region);
		}
		else if ((FE_node_get_FE_region(destination) == fe_region) &&
			(FE_node_get_FE_region(source) == fe_region))
		{
			if (merge_FE_node(destination, source))
			{
				return_code = 1;
				FE_REGION_FE_NODE_CHANGE(fe_region, destination,
					CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_node), source);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_merge_FE_node_existing.  "
				"Source and/or destination nodes are incompatible with region");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_node_existing.  Invalid argument(s)");
	}
	LEAVE;

	return return_code;
}

int FE_region_merge_FE_node_iterator(struct FE_node *node,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 27 March 2003

DESCRIPTION :
FE_node iterator version of FE_region_merge_FE_node.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_merge_FE_node_iterator);
	if (FE_region_merge_FE_node((struct FE_region *)fe_region_void, node))
	{
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_node_iterator */

struct FE_node *FE_region_get_first_FE_node_that(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_node) *conditional_function,
	void *user_data_void)
/*******************************************************************************
LAST MODIFIED : 23 August 2004

DESCRIPTION :
Returns the first node in <fe_region> that satisfies <conditional_function>
with <user_data_void>.
A NULL <conditional_function> returns the first FE_node in <fe_region>, if any.
==============================================================================*/
{
	struct FE_node *node;

	ENTER(FE_region_get_first_FE_node_that);
	if (fe_region)
	{
		node = FIRST_OBJECT_IN_LIST_THAT(FE_node)(conditional_function,
			user_data_void, fe_region->fe_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_first_FE_node_that.  Missing region");
		node = (struct FE_node *)NULL;
	}
	LEAVE;

	return (node);
} /* FE_region_get_first_FE_node_that */

int FE_region_for_each_FE_node(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_node) iterator_function, void *user_data)
/*******************************************************************************
LAST MODIFIED : 15 January 2003

DESCRIPTION :
Calls <iterator_function> with <user_data> for each FE_node in <region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_for_each_FE_node);
	if (fe_region && iterator_function)
	{
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_node)(iterator_function,
			user_data, fe_region->fe_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_for_each_FE_node */

int FE_region_for_each_FE_node_conditional(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_node) conditional_function,
	void *conditional_user_data,
	LIST_ITERATOR_FUNCTION(FE_node) iterator_function,
	void *iterator_user_data)
/*******************************************************************************
LAST MODIFIED : 15 January 2003

DESCRIPTION :
For each FE_node in <fe_region> satisfying <conditional_function> with
<conditional_user_data>, calls <iterator_function> with it and
<iterator_user_data> as arguments.
==============================================================================*/
{
	int return_code;
	struct FE_node_conditional_iterator_data data;

	ENTER(FE_region_for_each_FE_node_conditional);
	if (fe_region && conditional_function && iterator_function)
	{
		data.conditional_function = conditional_function;
		data.conditional_user_data = conditional_user_data;
		data.iterator_function = iterator_function;
		data.iterator_user_data = iterator_user_data;
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_node)(
			FE_node_conditional_iterator, (void *)&data, fe_region->fe_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_node_conditional.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_for_each_FE_node_conditional */

int FE_region_remove_FE_node(struct FE_region *fe_region,
	struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Removes <node> from <fe_region>.
Nodes can only be removed if not in use by elements in <fe_region>.
Should enclose call between FE_region_begin_change and FE_region_end_change to
minimise messages.
Note it is more efficient to use FE_region_remove_FE_node_list for more than
one node.
==============================================================================*/
{
	int return_code;
	struct FE_element_add_nodes_to_list_data add_nodes_data;

	ENTER(FE_region_remove_FE_node);
	return_code = 0;
	if (fe_region && node)
	{
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
		{
			/* expensive; same code as for FE_region_remove FE_node_list.
			 * Supply highest dimension element list in region
			 * to avoid repeated checks for nodes inherited from them */
			for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; (0 <= dim); --dim)
			{
				add_nodes_data.element_list = fe_region->fe_element_list[dim];
				if (NUMBER_IN_LIST(FE_element)(add_nodes_data.element_list) > 0)
					break;
			}
			add_nodes_data.node_list = CREATE(LIST(FE_node))();
			add_nodes_data.intersect_node_list = CREATE(LIST(FE_node))();
			if (add_nodes_data.node_list && add_nodes_data.intersect_node_list &&
				ADD_OBJECT_TO_LIST(FE_node)(node,
					add_nodes_data.intersect_node_list))
			{
				return_code = 1;
				for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; (0 <= dim); --dim)
				{
					if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(
						FE_element_add_nodes_to_list, (void *)&add_nodes_data,
						fe_region->fe_element_list[dim]))
					{
						return_code = 0;
						break;
					}
				}
				if (0 < NUMBER_IN_LIST(FE_node)(add_nodes_data.node_list))
				{
					return_code = 0;
				}
			}
			DESTROY(LIST(FE_node))(&add_nodes_data.node_list);
			DESTROY(LIST(FE_node))(&add_nodes_data.intersect_node_list);
			if (return_code)
			{
				/* access node in case it is only accessed here */
				ACCESS(FE_node)(node);
				if (return_code = REMOVE_OBJECT_FROM_LIST(FE_node)(node,
					fe_region->fe_node_list))
				{
					FE_REGION_FE_NODE_CHANGE(fe_region, node,
						CHANGE_LOG_OBJECT_REMOVED(FE_node), node);
				}
				DEACCESS(FE_node)(&node);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_remove_FE_node.  Node is in use by elements in region");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_remove_FE_node.  Node is not in region");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_node.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_node */

struct FE_region_remove_FE_node_iterator_data
/*******************************************************************************
LAST MODIFIED : 12 February 2003

DESCRIPTION :
Data for FE_region_remove_FE_node_iterator.
==============================================================================*/
{
	/* the FE_region to remove the node from */
	struct FE_region *fe_region;
	/* if fe_region is using a master_fe_region this list should contain
		 the list of nodes in-use by elements in this fe_region */
	struct LIST(FE_node) *exclusion_node_list;
};

static int FE_region_remove_FE_node_iterator(struct FE_node *node,
	void *data_void)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Removes <node> from fe_region.  Should enclose call between 
FE_region_begin_change and FE_region_end_change to minimise messages.
<data_void> points at a struct FE_region_remove_FE_node_iterator_data.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;
	struct FE_region_remove_FE_node_iterator_data *data;

	ENTER(FE_region_remove_FE_node_iterator);
	if (node && (data =
		(struct FE_region_remove_FE_node_iterator_data *)data_void) &&
		(fe_region = data->fe_region))
	{
		return_code = 1;
		if (IS_OBJECT_IN_LIST(FE_node)(node, fe_region->fe_node_list))
		{
			if (!IS_OBJECT_IN_LIST(FE_node)(node, data->exclusion_node_list))
			{
				if (return_code = REMOVE_OBJECT_FROM_LIST(FE_node)(node,
					fe_region->fe_node_list))
				{
					FE_REGION_FE_NODE_CHANGE(fe_region, node,
						CHANGE_LOG_OBJECT_REMOVED(FE_node), node);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_node_iterator.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_node_iterator */

int FE_region_remove_FE_node_list(struct FE_region *fe_region,
	struct LIST(FE_node) *node_list)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Attempts to removes all the nodes in <node_list> from <fe_region>.
Nodes can only be removed if not in use by elements in <fe_region>.
Should enclose call between FE_region_begin_change and FE_region_end_change to
minimise messages.
On return, <node_list> will contain all the nodes that are still in
<fe_region> after the call.
A true return code is only obtained if all nodes from <node_list> are removed.
==============================================================================*/
{
	int return_code;
	struct FE_element_add_nodes_to_list_data add_nodes_data;
	struct FE_region_remove_FE_node_iterator_data remove_nodes_data;

	ENTER(FE_region_remove_FE_node_list);
	if (fe_region && node_list)
	{
		return_code = 1;
		remove_nodes_data.exclusion_node_list = CREATE(LIST(FE_node))();
		/* expensive to determine nodes in use by elements, even more so for
		 * faces and lines.
		 * Supply highest dimension element list in region
		 * to avoid repeated checks for nodes inherited from them */
		for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; (0 <= dim); --dim)
		{
			add_nodes_data.element_list = fe_region->fe_element_list[dim];
			if (NUMBER_IN_LIST(FE_element)(add_nodes_data.element_list) > 0)
				break;
		}
		add_nodes_data.node_list = remove_nodes_data.exclusion_node_list;
		add_nodes_data.intersect_node_list = node_list;
		for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; (0 <= dim); --dim)
		{
			if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(
				FE_element_add_nodes_to_list, (void *)&add_nodes_data,
				fe_region->fe_element_list[dim]))
			{
				return_code = 0;
				break;
			}
		}
		if (return_code)
		{
			/* begin/end change to prevent multiple messages */
			FE_region_begin_change(fe_region);
			remove_nodes_data.fe_region = fe_region;
			return_code = FOR_EACH_OBJECT_IN_LIST(FE_node)(
				FE_region_remove_FE_node_iterator, (void *)&remove_nodes_data,
				node_list);
			/* remove nodes that were successfully removed */
			REMOVE_OBJECTS_FROM_LIST_THAT(FE_node)(FE_node_is_not_in_list,
				(void *)(fe_region->fe_node_list), node_list);
			if (0 < NUMBER_IN_LIST(FE_node)(node_list))
			{
				return_code = 0;
			}
			FE_region_end_change(fe_region);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_remove_FE_node_list.  Could not exclude nodes in elements");
			return_code = 0;
		}
		DESTROY(LIST(FE_node))(&remove_nodes_data.exclusion_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_node_list.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_node_list */

int FE_region_get_last_FE_nodes_idenifier(struct FE_region *fe_region)
{
	int number_of_nodes, i, identifier;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_last_FE_nodes_idenifier);
	if (fe_region)
	{
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region);
		number_of_nodes = NUMBER_IN_LIST(FE_node)(master_fe_region->fe_node_list);
		identifier = 1;
		i = 0;
		while (i < number_of_nodes)
		{
			if (FIND_BY_IDENTIFIER_IN_LIST(FE_node, cm_node_identifier)(
						identifier, master_fe_region->fe_node_list))
			{
				i++;
			}
			identifier++;
		}
		identifier = identifier - 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_last_FE_nodes_idenifier.  Invalid argument(s)");
		identifier = 0;
	}
	LEAVE;

	return (identifier);
} /* FE_region_get_last_FE_nodes_idenifier */

int FE_region_get_number_of_FE_nodes(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 23 January 2003

DESCRIPTION :
Returns the number of FE_nodes in <fe_region>.
==============================================================================*/
{
	int number_of_nodes;

	ENTER(FE_region_get_number_of_FE_nodes);
	if (fe_region)
	{
		number_of_nodes = NUMBER_IN_LIST(FE_node)(fe_region->fe_node_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_nodes.  Invalid argument(s)");
		number_of_nodes = 0;
	}
	LEAVE;

	return (number_of_nodes);
} /* FE_region_get_number_of_FE_nodes */

struct FE_node *FE_region_node_string_to_FE_node(
	struct FE_region *fe_region, const char *node_string)
/*******************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
Returns the node from <fe_region> whose number is in the string <name>.
==============================================================================*/
{
	int node_number;
	struct FE_node *node;

	ENTER(FE_region_node_string_to_FE_node);
	node = (struct FE_node *)NULL;
	if (fe_region && node_string)
	{
		if (1 == sscanf(node_string, "%d", &node_number))
		{
			node = FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(node_number,
				fe_region->fe_node_list);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Text_choose_FE_node_from_fe_region_string_to_FE_node.  "
				"Cannot read string");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_node_string_to_FE_node.  Invalid argument(s)");
	}
	LEAVE;

	return (node);
} /* FE_region_node_string_to_FE_node */

int set_FE_node_FE_region(struct Parse_state *state, void *node_address_void,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 25 February 2003

DESCRIPTION :
Used in command parsing to translate a node name into an node from <fe_region>.
==============================================================================*/
{
	const char *current_token;
	int identifier, return_code;
	struct FE_node *node;
	struct FE_region *fe_region;

	ENTER(set_FE_node_FE_region);
	struct FE_node **node_address = reinterpret_cast<struct FE_node **>(node_address_void);
	if ((state) && (node_address) &&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		if (current_token=state->current_token)
		{
			if (strcmp(PARSER_HELP_STRING,current_token)&&
				strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
			{
				if ((1 == sscanf(current_token, "%d", &identifier)) &&
					(node = FE_region_get_FE_node_from_identifier(fe_region, identifier)))
				{
					REACCESS(FE_node)(node_address, node);
					return_code = shift_Parse_state(state,1);
				}
				else
				{
					display_message(WARNING_MESSAGE, "Unknown node: %s", current_token);
					display_parse_state_location(state);
					return_code = 0;
				}
			}
			else
			{
				display_message(INFORMATION_MESSAGE, " NODE_NUMBER");
				if (node= *node_address)
				{
					display_message(INFORMATION_MESSAGE, "[%d]",
						get_FE_node_identifier(node));
				}
				return_code = 1;
			}
		}
		else
		{
			display_message(WARNING_MESSAGE, "Missing number for node");
			display_parse_state_location(state);
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_node_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_node_FE_region */

struct MANAGER(FE_basis) *FE_region_get_basis_manager(
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 14 January 2003

DESCRIPTION :
Returns the FE_basis_manager used for bases in this <fe_region>.
==============================================================================*/
{
	struct MANAGER(FE_basis) *basis_manager;

	ENTER(FE_region_get_basis_manager);
	if (fe_region)
	{
		if (fe_region->master_fe_region)
		{
			basis_manager = FE_region_get_basis_manager(fe_region->master_fe_region);
		}
		else
		{
			basis_manager = fe_region->basis_manager;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_basis_manager.  Missing finite element region");
		basis_manager = (struct MANAGER(FE_basis) *)NULL;
	}
	LEAVE;

	return (basis_manager);
} /* FE_region_get_basis_manager */

int FE_region_get_immediate_master_FE_region(struct FE_region *fe_region,
	struct FE_region **master_fe_region_address)
/*******************************************************************************
LAST MODIFIED : 18 February 2003

DESCRIPTION :
Returns the master_fe_region for this <fe_region>, which is NULL if the region
has its own namespace for fields, nodes and elements.
Function is not recursive; returns only the master FE_region for <fe_region>
without enquiring for that of its master.
See also FE_region_get_ultimate_master_FE_region.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_get_immediate_master_FE_region);
	if (fe_region && master_fe_region_address)
	{
		*master_fe_region_address = fe_region->master_fe_region;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_immediate_master_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_get_immediate_master_FE_region */

int FE_region_get_ultimate_master_FE_region(struct FE_region *fe_region,
	struct FE_region **master_fe_region_address)
/*******************************************************************************
LAST MODIFIED : 22 May 2003

DESCRIPTION :
Returns the FE_region that the fields, nodes and elements of <fe_region>
ultimately belong to. <fe_region> is returned if it has no immediate master.
???RC Until the data_hack is removed, have to stop at the ancestor with either
the data_hack or no master; this is the region owning the nodes/elements.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_ultimate_master_FE_region);
	if (fe_region && master_fe_region_address)
	{
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region && (!master_fe_region->top_data_hack))
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		*master_fe_region_address = master_fe_region;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_ultimate_master_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_get_ultimate_master_FE_region */

struct LIST(FE_element_shape) *FE_region_get_FE_element_shape_list(
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 7 July 2003

DESCRIPTION :
Returns the FE_basis_manager used for bases in this <fe_region>.
==============================================================================*/
{
	struct LIST(FE_element_shape) *element_shape_list;

	ENTER(FE_region_get_FE_element_shape_list);
	if (fe_region)
	{
		if (fe_region->master_fe_region)
		{
			element_shape_list = FE_region_get_FE_element_shape_list(fe_region->master_fe_region);
		}
		else
		{
			element_shape_list = fe_region->element_shape_list;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_element_shape_list.  Missing finite element region");
		element_shape_list = (struct LIST(FE_element_shape) *)NULL;
	}
	LEAVE;

	return (element_shape_list);
} /* FE_region_get_FE_element_shape_list */

int FE_region_change_FE_element_identifier(
	struct FE_region *fe_region, struct FE_element *element,
	int new_identifier)
{
	int return_code;

	ENTER(FE_region_change_FE_element_identifier);
	int dimension = get_FE_element_dimension(element);
	if (fe_region && element && new_identifier && (1 <= dimension) &&
		(dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		struct LIST(FE_element) *element_list = FE_region_get_element_list(fe_region, dimension);
		if (IS_OBJECT_IN_LIST(FE_element)(element, element_list))
		{
			return_code = 1;
			struct FE_region *master_fe_region = fe_region;
			FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region);
			if (FE_region_get_FE_element_from_identifier(master_fe_region,
				dimension, new_identifier))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_change_FE_element_identifier.  "
					"Element with new identifier already exists");
				return_code = 0;
			}
			else
			{
				struct LIST(FE_element) *master_element_list =
					FE_region_get_element_list(master_fe_region, dimension);
				if (LIST_BEGIN_IDENTIFIER_CHANGE(FE_element,identifier)(
					master_element_list, element))
				{
					CM_element_information cm;
					get_FE_element_identifier(element, &cm);
					cm.number = new_identifier;
					return_code = set_FE_element_identifier(element, &cm);
					LIST_END_IDENTIFIER_CHANGE(FE_element,identifier)(
						master_element_list);
					if (return_code)
					{
						FE_REGION_FE_ELEMENT_IDENTIFIER_CHANGE(master_fe_region, element);
					}
				}
				else 
				{
					display_message(ERROR_MESSAGE,
						"FE_region_change_FE_element_identifier.  "
						"Could not safely change identifier in indexed lists");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_change_FE_element_identifier.  "
				"Element is not in this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_change_FE_element_identifier.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_change_FE_element_identifier */

struct FE_element *FE_region_get_FE_element_from_identifier(
	struct FE_region *fe_region, int dimension, int identifier)
{
	struct FE_element *element;

	ENTER(FE_region_get_FE_element_from_identifier);
	if (fe_region && identifier &&
		(1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		CM_element_information cm = { DIMENSION_TO_CM_ELEMENT_TYPE(dimension), identifier };
		element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(&cm,
			FE_region_get_element_list(fe_region, dimension));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_element_from_identifier.  Invalid argument(s)");
		element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (element);
}

int FE_region_CM_element_type_to_dimension(struct FE_region *fe_region,
	enum CM_element_type cm_element_type)
{
	if (CM_ELEMENT == cm_element_type)
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
		{
			if (NUMBER_IN_LIST(FE_element)(FE_region_get_element_list(fe_region, dimension)))
				return dimension;
		}
		return 3;
	}
	else if (CM_FACE == cm_element_type)
		return 2;
	return 1;
}

struct FE_element *FE_region_get_FE_element_from_identifier_deprecated(
	struct FE_region *fe_region, struct CM_element_information *identifier)
{
	struct FE_element *element;

	ENTER(FE_region_get_FE_element_from_identifier_deprecated);
	if (fe_region && identifier)
	{
		int dimension = CM_ELEMENT_TYPE_TO_DIMENSION(identifier->type);
		element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(identifier,
			FE_region_get_element_list(fe_region, dimension));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_element_from_identifier_deprecated.  Invalid argument(s)");
		element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (element);
}

struct FE_element *FE_region_get_top_level_FE_element_from_identifier(
	struct FE_region *fe_region, int identifier)
{
	struct FE_element *element;

	ENTER(FE_region_get_top_level_FE_element_from_identifier);
	if (fe_region && identifier)
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
		{
			CM_element_information cm = { DIMENSION_TO_CM_ELEMENT_TYPE(dimension), identifier };
			element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(&cm,
				FE_region_get_element_list(fe_region, dimension));
			if (element && FE_element_is_top_level(element, (void *)NULL))
			{
				break;
			}
			else
			{
				element = NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_top_level_FE_element_from_identifier.  Invalid argument(s)");
		element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (element);
}

struct FE_element *FE_region_get_or_create_FE_element_with_identifier(
	struct FE_region *fe_region, struct CM_element_information *identifier,
	int dimension)
/*******************************************************************************
LAST MODIFIED : 27 May 2003

DESCRIPTION :
Convenience function returning an existing element with <identifier> from
<fe_region> or any of its ancestors. Existing elements are checked against the
<dimension> and no element is returned if the dimension is different.
If no existing element is found, a new element with the given <identifier> and
and unspecified shape of the given <dimension> is created.
If the returned element is not already in <fe_region> it is merged before
return.
It is expected that the calling function has wrapped calls to this function
with FE_region_begin/end_change.
==============================================================================*/
{
	struct FE_element *element;

	ENTER(FE_region_get_or_create_FE_element_with_identifier);
	element = (struct FE_element *)NULL;
	if (fe_region && identifier &&
		(1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		struct LIST(FE_element) *element_list = FE_region_get_element_list(fe_region, dimension);
		element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(identifier, element_list);
		if (!element)
		{
			/* get the ultimate master FE_region for complete list of elements */
			struct FE_region *master_fe_region = fe_region;
			while (master_fe_region->master_fe_region)
			{
				master_fe_region = master_fe_region->master_fe_region;
			}
			if (master_fe_region != fe_region)
			{
				element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(
					identifier, FE_region_get_element_list(master_fe_region, dimension));
			}
			if (!element)
			{
				struct FE_element_shape *element_shape;
				/* create an element with an unspecified shape of <dimension> */
				if (element_shape =
					CREATE(FE_element_shape)(dimension, /*type*/(int *)NULL,
					fe_region))
				{
					ACCESS(FE_element_shape)(element_shape);
					element = CREATE(FE_element)(identifier, element_shape,
						master_fe_region, /*template_element*/(struct FE_element *)NULL);
					DEACCESS(FE_element_shape)(&element_shape);
				}
				if (!element)
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_or_create_FE_element_with_identifier.  "
						"Could not create element");
				}
			}
			if (element)
			{
				if (!FE_region_merge_FE_element(fe_region, element))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_get_or_create_FE_element_with_identifier.  "
						"Could not merge element");
					/* following cleans up element if newly created, and clears pointer */
					REACCESS(FE_element)(&element, (struct FE_element *)NULL);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_or_create_FE_element_with_identifier.  "
			"Invalid argument(s)");
	}
	LEAVE;

	return (element);
} /* FE_region_get_or_create_FE_element_with_identifier */

int FE_region_get_next_FE_element_identifier(struct FE_region *fe_region,
	int dimension, int start_identifier)
{
	int identifier = 0;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_next_FE_element_identifier);
	if (fe_region && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		int dim = dimension - 1;
		/* get the ultimate master FE_region for complete list of elements */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		struct CM_element_information cm = { DIMENSION_TO_CM_ELEMENT_TYPE(dimension), 0 };
		if (start_identifier <= 0)
		{
			cm.number = master_fe_region->next_fe_element_identifier_cache[dim];
		}
		else
		{
			cm.number = start_identifier;
		}
		if (master_fe_region->next_fe_element_identifier_cache[dim])
		{
			if (master_fe_region->next_fe_element_identifier_cache[dim] > cm.number)
			{
				cm.number = master_fe_region->next_fe_element_identifier_cache[dim];
			}
		}
		while (FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(
			&cm, master_fe_region->fe_element_list[dim]))
		{
			cm.number++;
		}
		if (start_identifier <= 1)
		{
			/* Don't cache the value if we didn't start at the beginning */
			master_fe_region->next_fe_element_identifier_cache[dim] = cm.number;
		}
		identifier = cm.number;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_next_FE_element_identifier.  Missing fe_region");
	}
	LEAVE;

	return (identifier);
}

int FE_region_get_number_of_FE_elements_all_dimensions(struct FE_region *fe_region)
{
	int number_of_elements = 0;

	ENTER(FE_region_get_number_of_FE_elements_all_dimensions);
	if (fe_region)
	{
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			number_of_elements += NUMBER_IN_LIST(FE_element)(fe_region->fe_element_list[dim]);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_elements_all_dimensions.  Invalid argument(s)");
	}
	LEAVE;

	return (number_of_elements);
}

int FE_region_get_number_of_FE_elements_of_dimension(
	struct FE_region *fe_region, int dimension)
{
	int number_of_elements = 0;

	ENTER(FE_region_get_number_of_FE_elements_of_dimension);
	if (fe_region && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		number_of_elements =
			NUMBER_IN_LIST(FE_element)(fe_region->fe_element_list[dimension - 1]);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_elements_of_dimension.  Invalid argument(s)");
	}
	LEAVE;

	return (number_of_elements);
}

int FE_region_get_highest_dimension(struct FE_region *fe_region)
{
	int highest_dimension = 3;
	while (highest_dimension &&
		(0 == NUMBER_IN_LIST(FE_element)(FE_region_get_element_list(fe_region, highest_dimension))))
	{
		--highest_dimension;
	}
	return highest_dimension;
}

int FE_region_contains_FE_element(struct FE_region *fe_region,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 9 December 2002

DESCRIPTION :
Returns true if <element> is in <fe_region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_contains_FE_element);
	if (fe_region && element)
	{
		int dimension = get_FE_element_dimension(element);
		if (IS_OBJECT_IN_LIST(FE_element)(element, FE_region_get_element_list(fe_region, dimension)))
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
		display_message(ERROR_MESSAGE,
			"FE_region_contains_FE_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_contains_FE_element */

int FE_region_contains_FE_element_conditional(struct FE_element *element,
	void *fe_region_void)
{
	return FE_region_contains_FE_element((struct FE_region *)fe_region_void, element);
}

int FE_element_is_not_in_FE_region(struct FE_element *element,
	void *fe_region_void)
{
	return !FE_region_contains_FE_element((struct FE_region *)fe_region_void, element);
}

int FE_region_define_FE_field_at_element(struct FE_region *fe_region,
	struct FE_element *element, struct FE_field *fe_field,
	struct FE_element_field_component **components)
/*******************************************************************************
LAST MODIFIED : 6 March 2003

DESCRIPTION :
Checks <fe_region> contains <element>. If <fe_field> is already defined on it,
returns successfully, otherwise defines the field at the element using the
<components>. Change messages are broadcast for the ultimate master FE_region
of <fe_region>.
Should place multiple calls to this function between begin_change/end_change.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_define_FE_field_at_element);
	if (fe_region && element && fe_field && components)
	{
		if (FE_region_contains_FE_element(fe_region, element))
		{
			return_code = 1;
			if (!FE_field_is_defined_in_element(fe_field, element))
			{
				/* get the ultimate master FE_region; only it has FE_time */
				master_fe_region = fe_region;
				while (master_fe_region->master_fe_region)
				{
					master_fe_region = master_fe_region->master_fe_region;
				}
				if (FE_field_get_FE_region(fe_field) != master_fe_region)
				{
					display_message(ERROR_MESSAGE,
						"FE_region_define_FE_field_at_element.  "
						"Field is not of this finite element region");
					return_code = 0;
				}
				if (return_code)
				{
					if (return_code =
						define_FE_field_at_element(element, fe_field, components))
					{
						FE_REGION_FE_ELEMENT_FIELD_CHANGE(master_fe_region, element,
							fe_field);
					}
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_define_FE_field_at_element.  Element is not in region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_define_FE_field_at_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_define_FE_field_at_element */

struct FE_element *FE_region_create_FE_element_copy(struct FE_region *fe_region,
	int identifier, struct FE_element *source)
{
	struct FE_element *new_element;

	ENTER(FE_region_create_FE_element_copy);
	new_element = (struct FE_element *)NULL;
	if (fe_region && source)
	{
		int dimension = get_FE_element_dimension(source);
		struct LIST(FE_element) *element_list =
			FE_region_get_element_list(fe_region, dimension);
		if (fe_region->master_fe_region)
		{
			FE_region_begin_change(fe_region);
			new_element = FE_region_create_FE_element_copy(
				fe_region->master_fe_region, identifier, source);
			if ((new_element) && ADD_OBJECT_TO_LIST(FE_element)(new_element, element_list))
			{
				FE_REGION_FE_ELEMENT_CHANGE(fe_region, new_element,
					CHANGE_LOG_OBJECT_ADDED(FE_element), new_element);
			}
			FE_region_end_change(fe_region);
		}
		else if (FE_element_get_FE_region(source) == fe_region)
		{
			struct CM_element_information cm =
				{ DIMENSION_TO_CM_ELEMENT_TYPE(dimension), identifier };
			if (cm.number <= 0)
			{
				cm.number = FE_region_get_next_FE_element_identifier(
					fe_region, dimension, 0);
			}
			new_element = CREATE(FE_element)(&cm, (struct FE_element_shape *)NULL,
				(struct FE_region *)NULL, source);
			if (ADD_OBJECT_TO_LIST(FE_element)(new_element, element_list))
			{
				FE_REGION_FE_ELEMENT_CHANGE(fe_region, new_element,
					CHANGE_LOG_OBJECT_ADDED(FE_element), new_element);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_create_FE_element_copy.  Element identifier in use.");
				DESTROY(FE_element)(&new_element);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_create_FE_element_copy.  "
				"Source element is incompatible with region");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_create_FE_element_copy.  Invalid argument(s)");
	}
	LEAVE;

	return (new_element);
}

struct FE_element *FE_region_merge_FE_element(struct FE_region *fe_region,
	struct FE_element *element)
{
	struct FE_element *merged_element;
	struct LIST(FE_field) *changed_fe_field_list;

	ENTER(FE_region_merge_FE_element);
	merged_element = (struct FE_element *)NULL;
	if (fe_region && element)
	{
		int dimension = get_FE_element_dimension(element);
		struct LIST(FE_element) *element_list =
			FE_region_get_element_list(fe_region, dimension);
		if (fe_region->master_fe_region)
		{
			/* begin/end change to prevent multiple messages */
			FE_region_begin_change(fe_region);
			if (merged_element =
				FE_region_merge_FE_element(fe_region->master_fe_region, element))
			{
				/* data_fe_region does not contain element; only merges into
					 eventual master, the true root_region */
				if ((struct FE_region *)NULL==fe_region->base_fe_region)
				{
					if (!IS_OBJECT_IN_LIST(FE_element)(merged_element, element_list))
					{
						if (ADD_OBJECT_TO_LIST(FE_element)(merged_element, element_list))
						{
#if defined (DEBUG)
					/*???debug*/printf("FE_region_merge_FE_element: %p (M %p) ADD element %p\n",fe_region,fe_region->master_fe_region,merged_element);
#endif /* defined (DEBUG) */
							FE_REGION_FE_ELEMENT_CHANGE(fe_region, merged_element,
								CHANGE_LOG_OBJECT_ADDED(FE_element), merged_element);
						}
						else
						{
							struct CM_element_information identifier;
							get_FE_element_identifier(merged_element, &identifier);
							display_message(ERROR_MESSAGE,
								"FE_region_merge_FE_element.  Could not add %d-D element %d",
								dimension, identifier.number);
							merged_element = (struct FE_element *)NULL;
						}
					}
				}
			}
			FE_region_end_change(fe_region);
		}
		else
		{
			struct CM_element_information identifier;
			get_FE_element_identifier(element, &identifier);
			/* check element was created for this fe_region */
			if (fe_region == FE_element_get_FE_region(element))
			{
				if (merged_element = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(
					&identifier, element_list))
				{
					/* since we use merge to add existing elements to list, often no
						 merge will be necessary, hence the following */
					if (merged_element != element)
					{
						if (changed_fe_field_list = CREATE(LIST(FE_field))())
						{
							/* merge fields from element into global merged_element */
							if (merge_FE_element(merged_element, element,
								changed_fe_field_list))
							{
#if defined (DEBUG)
								/*???debug*/printf("FE_region_merge_FE_element: %p OBJECT_NOT_IDENTIFIER_CHANGED element %p\n",fe_region,merged_element);
#endif /* defined (DEBUG) */
								FE_REGION_FE_ELEMENT_FE_FIELD_LIST_CHANGE(fe_region,
									merged_element,
									CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element),
									changed_fe_field_list);
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_region_merge_FE_element.  Could not merge %s %d",
									CM_element_type_string(identifier.type), identifier.number);
								merged_element = (struct FE_element *)NULL;
							}
							DESTROY(LIST(FE_field))(&changed_fe_field_list);
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_region_merge_FE_element.  Could not create field list");
							merged_element = (struct FE_element *)NULL;
						}
					}
				}
				else
				{
					if (ADD_OBJECT_TO_LIST(FE_element)(element, element_list))
					{
						merged_element = element;
#if defined (DEBUG)
					/*???debug*/printf("FE_region_merge_FE_element: %p ADD element %p\n",fe_region,merged_element);
#endif /* defined (DEBUG) */
						FE_REGION_FE_ELEMENT_CHANGE(fe_region, merged_element,
							CHANGE_LOG_OBJECT_ADDED(FE_element), merged_element);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_region_merge_FE_element.  Could not add %s %d",
							CM_element_type_string(identifier.type), identifier.number);
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_merge_FE_element.  "
					"%s %d is not of this finite element region",
					CM_element_type_string(identifier.type), identifier.number);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_element.  Invalid argument(s)");
	}
	LEAVE;

	return (merged_element);
}

int FE_region_merge_FE_element_existing(struct FE_region *fe_region,
	struct FE_element *destination, struct FE_element *source)
{
	int return_code;
	struct LIST(FE_field) *changed_fe_field_list;

	ENTER(FE_region_merge_FE_element_existing);
	return_code = 0;
	if (fe_region && destination && source)
	{
		int dimension = get_FE_element_dimension(destination);
		int source_dimension = get_FE_element_dimension(source);
		if (source_dimension == dimension)
		{
			struct LIST(FE_element) *element_list =
				FE_region_get_element_list(fe_region, dimension);
			if (fe_region->master_fe_region)
			{
				FE_region_begin_change(fe_region);
				if (FE_region_merge_FE_element_existing(fe_region->master_fe_region,
					destination, source))
				{
					if (IS_OBJECT_IN_LIST(FE_element)(destination, element_list))
					{
						return_code = 1;
					}
					else if (ADD_OBJECT_TO_LIST(FE_element)(destination, element_list))
					{
						return_code = 1;
						FE_REGION_FE_ELEMENT_CHANGE(fe_region, destination,
							CHANGE_LOG_OBJECT_ADDED(FE_element), destination);
					}
				}
				FE_region_end_change(fe_region);
			}
			else if ((FE_element_get_FE_region(destination) == fe_region) &&
				(FE_element_get_FE_region(source) == fe_region))
			{
				changed_fe_field_list = CREATE(LIST(FE_field))();
				if (merge_FE_element(destination, source, changed_fe_field_list))
				{
					return_code = 1;
					FE_REGION_FE_ELEMENT_FE_FIELD_LIST_CHANGE(fe_region, destination,
						CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element),
						changed_fe_field_list);
				}
				DESTROY(LIST(FE_field))(&changed_fe_field_list);
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_merge_FE_element_existing.  "
					"Source and/or destination elements are incompatible with region");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_merge_FE_element_existing.  "
				"Cannot merge a source element of different dimension");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_element_existing.  Invalid argument(s)");
	}
	LEAVE;

	return return_code;
}

int FE_region_merge_FE_element_iterator(struct FE_element *element,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 27 January 2003

DESCRIPTION :
Iterator version of <FE_region_merge_FE_element>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_merge_FE_element_iterator);
	if (FE_region_merge_FE_element((struct FE_region *)fe_region_void, element))
	{
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_element_iterator */

int FE_region_begin_define_faces(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 15 April 2003

DESCRIPTION :
Sets up <fe_region> to automatically define faces on any elements merged into
it, and their faces recursively.
Call FE_region_end_define_faces as soon as face definition is finished.
Should put face definition calls between calls to begin_change/end_change.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_begin_define_faces);
	return_code = 0;
	/* define faces only with master FE_region */
	if (fe_region && (!fe_region->base_fe_region) &&
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region))
	{
		if (master_fe_region->element_type_node_sequence_list)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_begin_define_faces.  Already defining faces");
		}
		else
		{
			if (master_fe_region->element_type_node_sequence_list =
				CREATE(LIST(FE_element_type_node_sequence))())
			{
				bool have_elements = false;
				for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
				{
					struct LIST(FE_element) *element_list =
						FE_region_get_element_list(master_fe_region, dimension);
					if (NUMBER_IN_LIST(FE_element)(element_list))
					{
						have_elements = true;
					}
					if (have_elements && (dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS))
					{
						if (FOR_EACH_OBJECT_IN_LIST(FE_element)(
							FE_element_face_line_to_element_type_node_sequence_list,
							(void *)(master_fe_region->element_type_node_sequence_list),
							element_list))
						{
							return_code = 1;
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_region_begin_define_faces.  "
								"May not be able to share faces properly - perhaps "
								"2 existing faces have same shape and node list?");
						}
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_region_begin_define_faces.  Could not create node sequence list");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_begin_define_faces.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_begin_define_faces */

int FE_region_end_define_faces(struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 15 April 2003

DESCRIPTION :
Ends face definition in <fe_region>. Cleans up internal cache.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_end_define_faces);
	return_code = 0;
	/* define faces only with master FE_region */
	if (fe_region && (!fe_region->base_fe_region) &&
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region))
	{
		if (master_fe_region->element_type_node_sequence_list)
		{
			DESTROY(LIST(FE_element_type_node_sequence))(
				&master_fe_region->element_type_node_sequence_list);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_end_define_faces.  Already defining faces");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_end_define_faces.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_end_define_faces */

static int FE_region_merge_FE_element_nodes(struct FE_region *fe_region,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 19 May 2003

DESCRIPTION :
Merges the nodes used by fields of <element> into <fe_region>.
Currently only used by FE_region_merge_FE_element_and_faces_and_nodes.
Expects call to be wrapped in calls to FE_region_begin/end_changes.
If <element> has any parents, the nodes referenced by the default
coordinate field inherited from any single ultimate parent are merged.
???RC This "single ancestor field inheritance" is not sufficient in general;
one would have to inherit all fields from all top_level ancestor elements which
is quite expensive; deal with this once faces are actually passed to this
function -- they currently are not.
If the element has no parent, merges nodes directly referenced by <element>.
==============================================================================*/
{
	int i, number_of_parents, number_of_element_field_nodes, number_of_nodes,
		return_code;
	struct FE_node **element_field_nodes_array, *node;

	ENTER(FE_region_merge_FE_element_nodes);
	if (fe_region && element &&
		get_FE_element_number_of_nodes(element, &number_of_nodes) &&
		get_FE_element_number_of_parents(element, &number_of_parents))
	{
		return_code = 1;
		if (0 < number_of_parents)
		{
			if (calculate_FE_element_field_nodes(element, (struct FE_field *)NULL,
				&number_of_element_field_nodes, &element_field_nodes_array,
				/*top_level_element*/(struct FE_element *)NULL))
			{
				for (i = 0; i < number_of_element_field_nodes; i++)
				{
					if (node = element_field_nodes_array[i])
					{
						if (!FE_region_merge_FE_node(fe_region, node))
						{
							return_code = 0;
						}
						DEACCESS(FE_node)(&(element_field_nodes_array[i]));
					}
				}
				DEALLOCATE(element_field_nodes_array);
			}
			else
			{
				return_code = 0;
			}
		}
		else
		{
			for (i = 0; (i < number_of_nodes) && return_code; i++)
			{
				if (get_FE_element_node(element, i, &node))
				{
					if (node)
					{
						if (!FE_region_merge_FE_node(fe_region, node))
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
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_merge_FE_element_nodes.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_element_nodes.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_element_nodes */

static int FE_region_merge_FE_element_and_faces_private(
	struct FE_region *fe_region, struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Private version of FE_region_merge_FE_element that merges not only <element>
into <fe_region> but any of its faces that are defined.
<element> and any of its faces may already be in <fe_region>.
This function is called by FE_region_merge_FE_element_and_faces to merge
<element> and its faces, but NOT nodes since node merge is performed only
with the initial element.

- MUST NOT iterate over the list of elements in a region to add or define faces
as the list will be modified in the process; copy the list and iterate over
the copy.

Expects call to be wrapped in calls to FE_region_begin/end_changes.

If calls to this function are placed between FE_region_begin/end_define_faces,
then any missing faces are created and also merged into <fe_region>.
Function ensures that elements share existing faces and lines in preference to
creating new ones if they have matching shape and nodes.

???RC Can only match faces correctly for coordinate fields with standard node
to element maps and no versions. A grid-based coordinate field would fail
utterly since it has no nodes. A possible future solution for all cases is to
match the geometry exactly either by using the FE_element_field_values
(coefficients of the monomial basis functions), although there is a problem with
xi-directions not matching up, or actual centre positions of the face being a
trivial rejection, narrowing down to a single face or list of faces to compare
against.
==============================================================================*/
{
	int face_dimension, face_number, new_faces, number_of_faces, return_code;
	struct FE_element *face;
	struct FE_element_shape *element_shape, *face_shape;
	struct FE_element_type_node_sequence *element_type_node_sequence,
		*existing_element_type_node_sequence;
	struct FE_region *master_fe_region;

	ENTER(FE_region_merge_FE_element_and_faces_private);
	if (fe_region && element &&
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region) &&
		(FE_element_get_FE_region(element) == master_fe_region) &&
		get_FE_element_shape(element, &element_shape) &&
		get_FE_element_number_of_faces(element, &number_of_faces))
	{
		return_code = 1;
		new_faces = 0;
		for (face_number = 0; (face_number < number_of_faces) && return_code;
			face_number++)
		{
			face = (struct FE_element *)NULL;
			if ((return_code = get_FE_element_face(element, face_number, &face)) &&
				(!face) && master_fe_region->element_type_node_sequence_list)
			{
				if (face_shape =
					get_FE_element_shape_of_face(element_shape, face_number, fe_region))
				{
					ACCESS(FE_element_shape)(face_shape);
					get_FE_element_shape_dimension(face_shape, &face_dimension);
					int new_face_number = FE_region_get_next_FE_element_identifier(
						fe_region, face_dimension, 0);
					struct CM_element_information face_identifier =
						{ DIMENSION_TO_CM_ELEMENT_TYPE(face_dimension), new_face_number };
					if (face = CREATE(FE_element)(&face_identifier, face_shape,
						master_fe_region, (struct FE_element *)NULL))
					{
						/* must put the face in the element to inherit fields */
						set_FE_element_face(element, face_number, face);
						new_faces++;
						/* try to find an existing face in the FE_region with the same
							 shape and the same nodes as face */
						if (element_type_node_sequence =
							CREATE(FE_element_type_node_sequence)(face))
						{
							ACCESS(FE_element_type_node_sequence)(element_type_node_sequence);
							if (FE_element_type_node_sequence_is_collapsed(
								element_type_node_sequence))
							{
								/* clear the face */
								set_FE_element_face(element, face_number,
									(struct FE_element *)NULL);
								face = (struct FE_element *)NULL;
								new_faces--;
							}
							else
							{
								if (existing_element_type_node_sequence =
									FIND_BY_IDENTIFIER_IN_LIST(FE_element_type_node_sequence,
										identifier)(FE_element_type_node_sequence_get_identifier(
											element_type_node_sequence),
											master_fe_region->element_type_node_sequence_list))
								{
									face = FE_element_type_node_sequence_get_FE_element(
										existing_element_type_node_sequence);
									set_FE_element_face(element, face_number, face);
								}
								else
								{
									/* remember this sequence */
									ADD_OBJECT_TO_LIST(FE_element_type_node_sequence)(
										element_type_node_sequence,
										master_fe_region->element_type_node_sequence_list);
								}
							}
							DEACCESS(FE_element_type_node_sequence)(
								&element_type_node_sequence);
						}
						else
						{
							/* clear the face */
							set_FE_element_face(element, face_number,
								(struct FE_element *)NULL);
							face = (struct FE_element *)NULL;
							new_faces--;
							return_code = 0;
						}
					}
					else
					{
						return_code = 0;
					}
					DEACCESS(FE_element_shape)(&face_shape);
				}
				else
				{
					/* could not get face_shape */
					return_code = 0;
				}
			}
			if (face)
			{
				/* ensure the face and its lines are in the fe_region */
				return_code =
					FE_region_merge_FE_element_and_faces_private(fe_region, face);
			}
		} /* loop over faces */
		if (return_code)
		{
			if (FE_region_contains_FE_element(fe_region, element))
			{
				if (new_faces)
				{
					/* note element as having changed -- in master */
					FE_REGION_FE_ELEMENT_CHANGE(master_fe_region, element,
						CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_element), element);
				}
			}
			else
			{
				/* merge element into fe_region */
				if (!FE_region_merge_FE_element(fe_region, element))
				{
					return_code = 0;
				}
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_merge_FE_element_and_faces_private.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_element_and_faces_private.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_element_and_faces_private */

int FE_region_merge_FE_element_and_faces_and_nodes(struct FE_region *fe_region,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Version of FE_region_merge_FE_element that merges not only <element> into
<fe_region> but any of its faces that are defined.
<element> and any of its faces may already be in <fe_region>.
Also merges nodes referenced directly by <element> and its parents, if any.

- MUST NOT iterate over the list of elements in a region to add or define faces
as the list will be modified in the process; copy the list and iterate over
the copy.

FE_region_begin/end_change are called internally to reduce change messages to
one per call. User should place calls to the begin/end_change functions around
multiple calls to this function.

If calls to this function are placed between FE_region_begin/end_define_faces,
then any missing faces are created and also merged into <fe_region>.
Function ensures that elements share existing faces and lines in preference to
creating new ones if they have matching shape and nodes.

???RC Can only match faces correctly for coordinate fields with standard node
to element maps and no versions. A grid-based coordinate field would fail
utterly since it has no nodes. A possible future solution for all cases is to
match the geometry exactly either by using the FE_element_field_values
(coefficients of the monomial basis functions), although there is a problem with
xi-directions not matching up, or actual centre positions of the face being a
trivial rejection, narrowing down to a single face or list of faces to compare
against.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_merge_FE_element_and_faces_and_nodes);
	if (fe_region && element &&
		FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region) &&
		(FE_element_get_FE_region(element) == master_fe_region))
	{
		return_code = 1;
		FE_region_begin_change(fe_region);
		if (FE_region_merge_FE_element_nodes(fe_region, element))
		{
			if (!FE_region_merge_FE_element_and_faces_private(
				fe_region, element))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_merge_FE_element_and_faces_and_nodes.  "
					"Could not merge element and faces");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_merge_FE_element_and_faces_and_nodes.  "
				"Could not merge element nodes");
			return_code = 0;
		}
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_element_and_faces_and_nodes.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_element_and_faces_and_nodes */

int FE_region_merge_FE_element_and_faces_and_nodes_iterator(
	struct FE_element *element, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 27 March 2003

DESCRIPTION :
FE_element iterator version of FE_region_merge_FE_element_and_faces_and_nodes.
- MUST NOT iterate over the list of elements in a region to add or define faces
as the list will be modified in the process; copy the list and iterate over
the copy.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_merge_FE_element_and_faces_and_nodes_iterator);
	return_code = FE_region_merge_FE_element_and_faces_and_nodes(
		(struct FE_region *)fe_region_void, element);
	LEAVE;

	return (return_code);
} /* FE_region_merge_FE_element_and_faces_and_nodes_iterator */

int FE_region_define_faces(struct FE_region *fe_region)
{
	int return_code = 1;
	if (fe_region)
	{
		FE_region_begin_change(fe_region);
		FE_region_begin_define_faces(fe_region);
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (2 <= dimension) && return_code; --dimension)
		{
			return_code = FOR_EACH_OBJECT_IN_LIST(FE_element)(
				FE_region_merge_FE_element_and_faces_and_nodes_iterator,
				(void *)fe_region, FE_region_get_element_list(fe_region, dimension));
		}
		FE_region_end_define_faces(fe_region);
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_define_faces.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

int FE_region_get_FE_element_discretization(struct FE_region *fe_region,
	struct FE_element *element, int face_number,
	struct FE_field *native_discretization_field,
	int *top_level_number_in_xi,struct FE_element **top_level_element,
	int *number_in_xi)
/*******************************************************************************
LAST MODIFIED : 3 December 2002

DESCRIPTION :
FE_region wrapper for get_FE_element_discretization.
???RC Currently <fe_region> may be omitted to place no restriction on what
parent can be inherited from. This function will need to be tightened later.
==============================================================================*/
{
	int return_code;
	LIST_CONDITIONAL_FUNCTION(FE_element) *conditional = NULL;
	void *conditional_data = NULL;

	ENTER(FE_region_get_FE_element_discretization);
	if (fe_region && (fe_region->master_fe_region))
	{
		conditional = FE_region_contains_FE_element_conditional;
		conditional_data = (void *)fe_region;
	}
	else
	{
		/* if this is a master FE_region, don't need to check list */
	}
	return_code = get_FE_element_discretization(element,
		conditional, conditional_data,
		face_number, native_discretization_field, top_level_number_in_xi,
		top_level_element, number_in_xi);
	LEAVE;

	return (return_code);
}

static int FE_region_remove_FE_element_iterator(struct FE_element *element,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Removes <element> and all its faces that are not shared with other elements
from <fe_region>. Should enclose call between FE_region_begin_change and
FE_region_end_change to minimise messages.
Can only remove faces and lines if there is no master_fe_region or no parents
in this fe_region.
This function is recursive.
==============================================================================*/
{
	int return_code;
	struct FE_region *fe_region;

	ENTER(FE_region_remove_FE_element_iterator);
	if (element && (fe_region = (struct FE_region *)fe_region_void))
	{
		return_code = 1;
		int dimension = get_FE_element_dimension(element);
		if (IS_OBJECT_IN_LIST(FE_element)(element, FE_region_get_element_list(fe_region, dimension)))
		{
			/* if fe_region has a master_fe_region, can only remove element if it has
				 no parents in the list */
			if (fe_region->master_fe_region && (dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS))
			{
				int number_of_parents_in_region = 0;
				get_FE_element_number_of_parents_in_list(element,
					FE_region_get_element_list(fe_region, dimension + 1), &number_of_parents_in_region);
				if (number_of_parents_in_region > 0)
				{
					return_code = 0;
				}
			}
			if (return_code)
			{
				return_code = FE_region_remove_FE_element_private(fe_region, element);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_element_iterator.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_element_iterator */

int FE_region_remove_FE_element(struct FE_region *fe_region,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 14 May 2003

DESCRIPTION :
Removes <element> and all its faces that are not shared with other elements
from <fe_region>.
FE_region_begin/end_change are called internally to reduce change messages to
one per call. User should place calls to the begin/end_change functions around
multiple calls to this function.
Can only remove faces and lines if there is no master_fe_region or no parents
in this fe_region.
This function is recursive.
==============================================================================*/
{
	int return_code = 0;

	ENTER(FE_region_remove_FE_element);
	if (fe_region && element)
	{
		int return_code = 1;
		int dimension = get_FE_element_dimension(element);
		/* if fe_region has a master_fe_region, can only remove element if it has
			 no parents in the list */
		if (fe_region->master_fe_region && (dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS))
		{
			int number_of_parents_in_region = 0;
			get_FE_element_number_of_parents_in_list(element,
				FE_region_get_element_list(fe_region, dimension + 1), &number_of_parents_in_region);
			if (number_of_parents_in_region > 0)
			{
				return_code = 0;
			}
		}
		if (return_code)
		{
			FE_region_begin_change(fe_region);
			return_code = FE_region_remove_FE_element_private(fe_region, element);
			FE_region_end_change(fe_region);
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_remove_FE_element.  "
				"Element cannot be removed while it has parent element in region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_element */

int FE_region_remove_FE_element_list(struct FE_region *fe_region,
	struct LIST(FE_element) *element_list)
/*******************************************************************************
LAST MODIFIED : 14 January 2003

DESCRIPTION :
Attempts to removes all the elements in <element_list>, and all their faces that
are not shared with other elements from <fe_region>.
FE_region_begin/end_change are called internally to reduce change messages to
one per call. User should place calls to the begin/end_change functions around
multiple calls to this function.
Can only remove faces and lines if there is no master_fe_region or no parents
in this fe_region.
On return, <element_list> will contain all the elements that are still in
<fe_region> after the call.
A true return code is only obtained if all elements from <element_list> are
removed.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_remove_FE_element_list);
	if (fe_region && element_list)
	{
		FE_region_begin_change(fe_region);
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_element)(
			FE_region_remove_FE_element_iterator, (void *)fe_region, element_list);
		REMOVE_OBJECTS_FROM_LIST_THAT(FE_element)(FE_element_is_not_in_FE_region,
			(void *)fe_region, element_list);
		if (0 < NUMBER_IN_LIST(FE_element)(element_list))
		{
			return_code = 0;
		}
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_element_list.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_element_list */

int FE_region_element_or_parent_has_field(struct FE_region *fe_region,
	struct FE_element *element, struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
Returns true if <element> is in <fe_region> and has <field> defined on it or
any parents also in <fe_region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_element_or_parent_has_field);
	if (fe_region && element && field)
	{
		return_code = FE_element_or_parent_has_field(element, field,
			FE_region_contains_FE_element_conditional, (void *)fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_element_or_parent_has_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_element_or_parent_has_field */

struct FE_element *FE_region_get_first_FE_element_that(
	struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_element) *conditional_function,
	void *user_data_void)
{
	struct FE_element *element;

	ENTER(FE_region_get_first_FE_element_that);
	if (fe_region)
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (1 <= dimension); --dimension)
		{
			element = FIRST_OBJECT_IN_LIST_THAT(FE_element)(conditional_function,
				user_data_void, FE_region_get_element_list(fe_region, dimension));
			if (element)
				break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_first_FE_element_that.  Invalid argument(s)");
		element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (element);
}

struct FE_element *FE_region_get_first_FE_element_of_dimension_that(
	struct FE_region *fe_region, int dimension,
	LIST_CONDITIONAL_FUNCTION(FE_element) *conditional_function,
	void *user_data_void)
{
	struct FE_element *element;

	ENTER(FE_region_get_first_FE_element_that);
	if (fe_region && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		element = FIRST_OBJECT_IN_LIST_THAT(FE_element)(conditional_function,
			user_data_void, FE_region_get_element_list(fe_region, dimension));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_first_FE_element_that.  Invalid argument(s)");
		element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (element);
}

int FE_region_for_each_FE_element(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_element) iterator_function, void *user_data)
{
	int return_code;

	ENTER(FE_region_for_each_FE_element);
	if (fe_region && iterator_function)
	{
		return_code = 1;
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (return_code) && (1 <= dimension); --dimension)
		{
			return_code = FOR_EACH_OBJECT_IN_LIST(FE_element)(iterator_function,
				user_data, FE_region_get_element_list(fe_region, dimension));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_for_each_FE_element */

int FE_region_for_each_FE_element_of_dimension(struct FE_region *fe_region,
	int dimension, LIST_ITERATOR_FUNCTION(FE_element) iterator_function,
	void *user_data)
{
	int return_code;

	ENTER(FE_region_for_each_FE_element_of_dimension);
	if (fe_region && iterator_function &&
		(1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_element)(iterator_function,
			user_data, FE_region_get_element_list(fe_region, dimension));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int FE_region_for_each_FE_element_conditional(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_element) conditional_function,
	void *conditional_user_data,
	LIST_ITERATOR_FUNCTION(FE_element) iterator_function,
	void *iterator_user_data)
{
	int return_code;
	struct FE_element_conditional_iterator_data data;

	ENTER(FE_region_for_each_FE_element_conditional);
	if (fe_region && conditional_function && iterator_function)
	{
		data.conditional_function = conditional_function;
		data.conditional_user_data = conditional_user_data;
		data.iterator_function = iterator_function;
		data.iterator_user_data = iterator_user_data;
		return_code = FE_region_for_each_FE_element(fe_region,
			FE_element_conditional_iterator, (void *)&data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_element_conditional.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int FE_region_for_each_FE_element_of_dimension_conditional(
	struct FE_region *fe_region, int dimension,
	LIST_CONDITIONAL_FUNCTION(FE_element) conditional_function,
	void *conditional_user_data,
	LIST_ITERATOR_FUNCTION(FE_element) iterator_function,
	void *iterator_user_data)
{
	int return_code;
	struct FE_element_conditional_iterator_data data;

	ENTER(FE_region_for_each_FE_element_of_dimension_conditional);
	if (fe_region && conditional_function && iterator_function)
	{
		data.conditional_function = conditional_function;
		data.conditional_user_data = conditional_user_data;
		data.iterator_function = iterator_function;
		data.iterator_user_data = iterator_user_data;
		return_code = FE_region_for_each_FE_element_of_dimension(fe_region,
			dimension, FE_element_conditional_iterator, (void *)&data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_element_of_dimension_conditional.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int FE_region_FE_element_meets_topological_criteria(struct FE_region *fe_region,
	struct FE_element *element, int dimension, int exterior, int face_number)
{
	int return_code;

	ENTER(FE_region_FE_element_meets_topological_criteria);
	if (fe_region && element)
	{
		return_code = FE_element_meets_topological_criteria(element, dimension,
			exterior, face_number,
			FE_region_contains_FE_element_conditional, (void *)fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_FE_element_meets_topological_criteria.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

struct FE_element *FE_region_element_string_to_FE_element(
	struct FE_region *fe_region, const char *name)
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
Calls element_string_to_FE_element with the <element_list> private to
<fe_region>.
==============================================================================*/
{
	struct FE_element *element;

	ENTER(FE_region_element_string_to_FE_element);
	element = (struct FE_element *)NULL;
	if (fe_region && name)
	{
		int highest_dimension = FE_region_get_highest_dimension(fe_region);
		if (highest_dimension)
		{
			element = element_string_to_FE_element(name,
				FE_region_get_element_list(fe_region, highest_dimension));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_element_string_to_FE_element.  Invalid argument(s)");
	}
	LEAVE;

	return (element);
} /* FE_region_element_string_to_FE_element */

struct FE_element *FE_region_any_element_string_to_FE_element(
	struct FE_region *fe_region, const char *name)
{
	struct FE_element *element;

	ENTER(FE_region_any_element_string_to_FE_element);
	element = (struct FE_element *)NULL;
	if (fe_region && name)
	{
		const char *number_string, *type_string;
		struct CM_element_information cm;
		if (type_string = strpbrk(name,"eEfFlL"))
		{
			if (('l' == *type_string)||('L' == *type_string))
			{
				cm.type = CM_LINE;
			}
			else if (('f' == *type_string)||('F' == *type_string))
			{
				cm.type = CM_FACE;
			}
			else
			{
				cm.type = CM_ELEMENT;
			}
		}
		else
		{
			cm.type = CM_ELEMENT;
		}
		if (number_string = strpbrk(name,"+-0123456789"))
		{
			cm.number = atoi(number_string);
			int dimension = CM_ELEMENT_TYPE_TO_DIMENSION(cm.type);
			element = FE_region_get_FE_element_from_identifier(fe_region, dimension, cm.number);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_any_element_string_to_FE_element.  Invalid argument(s)");
	}
	LEAVE;

	return (element);
}

int set_FE_element_top_level_FE_region(struct Parse_state *state,
	void *element_address_void, void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 26 February 2003

DESCRIPTION :
A modifier function for specifying a top level element, used, for example, to
set the seed element for a xi_texture_coordinate computed_field.
==============================================================================*/
{
	const char *current_token;
	int return_code = 0;
	struct FE_element *element, **element_address;
	struct FE_region *fe_region;

	ENTER(set_FE_element_top_level_FE_region);
	if (state && (element_address = (struct FE_element **)element_address_void) &&
		(fe_region = (struct FE_region *)fe_region_void))
	{
		if (current_token = state->current_token)
		{
			if (strcmp(PARSER_HELP_STRING,current_token)&&
				strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))
			{
				int element_number = 0;
				if (1 == sscanf(current_token, "%d", &element_number))
				{
					shift_Parse_state(state,1);
					element = FE_region_get_top_level_FE_element_from_identifier(fe_region, element_number);
					if (element)
					{
						REACCESS(FE_element)(element_address, element);
						return_code = 1;
					}
				}
				if (!return_code)
				{
					display_message(WARNING_MESSAGE,
						"Unknown seed element: %s", current_token);
					display_parse_state_location(state);
				}
			}
			else
			{
				display_message(INFORMATION_MESSAGE, " ELEMENT_NUMBER");
				if (element = *element_address)
				{
					struct CM_element_information cm;
					get_FE_element_identifier(element, &cm);
					display_message(INFORMATION_MESSAGE, "[%d]", cm.number);
				}
				return_code = 1;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "Missing seed element number");
			display_parse_state_location(state);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_element_top_level_FE_region.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* set_FE_element_top_level_FE_region */

struct FE_region_smooth_FE_element_data
/*******************************************************************************
LAST MODIFIED : 12 March 2003

DESCRIPTION :
Data for FE_region_smooth_FE_element.
==============================================================================*/
{
	FE_value time;
	struct FE_field *fe_field;
	struct FE_field *element_count_fe_field;
	struct FE_region *fe_region;
	struct LIST(FE_node) *copy_node_list;
};

static int FE_region_smooth_FE_element(struct FE_element *element,
	void *smooth_element_data_void)
/*******************************************************************************
LAST MODIFIED : 16 April 2003

DESCRIPTION :
Creates a copy of element, calls FE_element_smooth_FE_field for it, then
merges it back into its FE_region.
Place between begin/end_change calls when used for a series of elements.
Must call FE_region_smooth_FE_node for copy_node_list afterwards.
<smooth_element_data_void> points at a struct FE_region_smooth_FE_element_data.
==============================================================================*/
{
	int return_code;
	struct CM_element_information cm;
	struct FE_element *copy_element;
	struct FE_region_smooth_FE_element_data *smooth_element_data;

	ENTER(FE_region_smooth_FE_element);
	if (element && (smooth_element_data =
		(struct FE_region_smooth_FE_element_data *)smooth_element_data_void))
	{
		return_code = 1;
		/* skip elements without field defined appropriately */
		if (FE_element_field_is_standard_node_based(element,
			smooth_element_data->fe_field))
		{
			/*???RC Should make a copy of element with JUST fe_field - would result
				in a more precise change message and faster response */
			if (get_FE_element_identifier(element, &cm) &&
				(copy_element = CREATE(FE_element)(&cm,
					(struct FE_element_shape *)NULL, (struct FE_region *)NULL, element)))
			{
				ACCESS(FE_element)(copy_element);
				if (FE_element_smooth_FE_field(copy_element,
					smooth_element_data->fe_field,
					smooth_element_data->time,
					smooth_element_data->element_count_fe_field,
					smooth_element_data->copy_node_list))
				{
					if (!FE_region_merge_FE_element(smooth_element_data->fe_region,
						copy_element))
					{
						return_code = 0;
					}
				}
				else
				{
					return_code = 0;
				}
				DEACCESS(FE_element)(&copy_element);
			}
			else
			{
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_smooth_FE_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_smooth_FE_element */

struct FE_region_smooth_FE_node_data
/*******************************************************************************
LAST MODIFIED : 12 March 2003

DESCRIPTION :
Data for FE_region_smooth_FE_node.
==============================================================================*/
{
	FE_value time;
	struct FE_field *fe_field;
	struct FE_field *element_count_fe_field;
	struct FE_region *fe_region;
};

static int FE_region_smooth_FE_node(struct FE_node *node,
	void *smooth_node_data_void)
/*******************************************************************************
LAST MODIFIED : 27 March 2003

DESCRIPTION :
Calls FE_node_smooth_FE_field for <node> then merges the result into the
<fe_region>.
<node> must not be a global node.
Place between begin/end_change calls when used for a series of nodes.
<smooth_node_data_void> points at a struct FE_region_smooth_FE_node_data.
==============================================================================*/
{
	int return_code;
	struct FE_region_smooth_FE_node_data *smooth_node_data;

	ENTER(FE_region_smooth_FE_node);
	if (node && (smooth_node_data =
		(struct FE_region_smooth_FE_node_data *)smooth_node_data_void))
	{
		if (FE_node_smooth_FE_field(node, smooth_node_data->fe_field,
			smooth_node_data->time, smooth_node_data->element_count_fe_field) &&
			((struct FE_node *)NULL !=
				FE_region_merge_FE_node(smooth_node_data->fe_region, node)))
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
		display_message(ERROR_MESSAGE,
			"FE_region_smooth_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_smooth_FE_node */

int FE_region_smooth_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field, FE_value time)
/*******************************************************************************
LAST MODIFIED : 29 April 2003

DESCRIPTION :
Smooths node-based <fe_field> over its nodes and elements in <fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;
	struct FE_region_smooth_FE_element_data smooth_element_data;
	struct FE_region_smooth_FE_node_data smooth_node_data;

	ENTER(FE_region_smooth_FE_field);       
	if (fe_region && fe_field)
	{
		/* get the ultimate master FE_region; only it has fields */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}

		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, master_fe_region->fe_field_list))
		{
			return_code = 1;
			// use highest dimension non-empty element list
			int dimension;
			struct LIST(FE_element) *element_list = NULL;
			for (dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
			{
				struct LIST(FE_element) *element_list =
					FE_region_get_element_list(fe_region, dimension);
				if (NUMBER_IN_LIST(FE_element)(element_list))
					break;
			}
			if (dimension)
			{
				FE_region_begin_change(master_fe_region);
				smooth_element_data.time = time;
				smooth_element_data.fe_field = fe_field;
				/* create a field to store an integer value per component of fe_field */
				smooth_element_data.element_count_fe_field =
					CREATE(FE_field)("count", fe_region);
				set_FE_field_number_of_components(
					smooth_element_data.element_count_fe_field,
					get_FE_field_number_of_components(fe_field));
				set_FE_field_value_type(smooth_element_data.element_count_fe_field,
					INT_VALUE);
				ACCESS(FE_field)(smooth_element_data.element_count_fe_field);
				smooth_element_data.fe_region = master_fe_region;
				smooth_element_data.copy_node_list = CREATE(LIST(FE_node))();
				if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(FE_region_smooth_FE_element,
					(void *)&smooth_element_data, element_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_smooth_FE_field.  Error smoothing elements");
					return_code = 0;
				}

				smooth_node_data.time = time;
				smooth_node_data.fe_field = fe_field;
				smooth_node_data.element_count_fe_field =
					smooth_element_data.element_count_fe_field;
				smooth_node_data.fe_region = master_fe_region;
				if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(FE_region_smooth_FE_node,
					(void *)&smooth_node_data, smooth_element_data.copy_node_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_region_smooth_FE_field.  Error smoothing nodes");
					return_code = 0;
				}

				DESTROY(LIST(FE_node))(&smooth_element_data.copy_node_list);
				DEACCESS(FE_field)(&smooth_element_data.element_count_fe_field);

				FE_region_end_change(master_fe_region);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_smooth_FE_field.  FE_field is not from this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_smooth_FE_field.  Invalid argument(s)");
		return_code = 0;;
	}
	LEAVE;

	return (return_code);
} /* FE_region_smooth_FE_field */

struct FE_time_sequence *FE_region_get_FE_time_sequence_matching_series(
	struct FE_region *fe_region, int number_of_times, FE_value *times)
/*******************************************************************************
LAST MODIFIED : 20 February 2003

DESCRIPTION :
Finds or creates a struct FE_time_sequence in <fe_region> with the given
<number_of_times> and <times>.
==============================================================================*/
{
	struct FE_time_sequence *fe_time_sequence;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_time_sequence_matching_series);
	fe_time_sequence = (struct FE_time_sequence *)NULL;
	if (fe_region && (0 < number_of_times) && times)
	{
		/* get the ultimate master FE_region; only it has FE_time */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (!(fe_time_sequence = get_FE_time_sequence_matching_time_series(
			master_fe_region->fe_time, number_of_times, times)))
		{
			display_message(ERROR_MESSAGE,
				"FE_region_get_FE_time_sequence_matching_series.  "
				"Could not get time version");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_time_sequence_matching_series.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_time_sequence);
} /* FE_region_get_FE_time_sequence_matching_series */

struct FE_time_sequence *FE_region_get_FE_time_sequence_merging_two_time_series(
	struct FE_region *fe_region, struct FE_time_sequence *time_sequence_one,
	struct FE_time_sequence *time_sequence_two)
/*******************************************************************************
LAST MODIFIED : 20 February 2003

DESCRIPTION :
Finds or creates a struct FE_time_sequence in <fe_region> which has the list of
times formed by merging the two time_sequences supplied.
==============================================================================*/
{
	struct FE_time_sequence *fe_time_sequence;
	struct FE_region *master_fe_region;

	ENTER(FE_region_get_FE_time_sequence_merging_two_time_series);
	fe_time_sequence = (struct FE_time_sequence *)NULL;
	if (fe_region && time_sequence_one && time_sequence_two)
	{
		/* get the ultimate master FE_region; only it has FE_time */
		master_fe_region = fe_region;
		while (master_fe_region->master_fe_region)
		{
			master_fe_region = master_fe_region->master_fe_region;
		}
		if (!(fe_time_sequence = get_FE_time_sequence_merging_two_time_series(
			master_fe_region->fe_time, time_sequence_one, time_sequence_two)))
		{
			display_message(ERROR_MESSAGE,
				"FE_region_get_FE_time_sequence_merging_two_time_series.  "
				"Could not get time version");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_time_sequence_merging_two_time_series.  "
			"Invalid argument(s)");
	}
	LEAVE;

	return (fe_time_sequence);
} /* FE_region_get_FE_time_sequence_merging_two_time_series */

struct FE_basis *FE_region_get_FE_basis_matching_basis_type(
	struct FE_region *fe_region, int *basis_type)
/*******************************************************************************
LAST MODIFIED : 29 October 2002

DESCRIPTION :
Finds or creates a struct FE_basis in <fe_region> with the given <basis_type>.
Recursive if fe_region has a master_fe_region.
==============================================================================*/
{
	struct FE_basis *fe_basis;

	ENTER(FE_region_get_FE_basis_matching_basis_type);
	fe_basis = (struct FE_basis *)NULL;
	if (fe_region && basis_type)
	{
		if (fe_region->master_fe_region)
		{
			fe_basis = FE_region_get_FE_basis_matching_basis_type(
				fe_region->master_fe_region, basis_type);
		}
		else
		{
			fe_basis = make_FE_basis(basis_type, fe_region->basis_manager);
		}
		if (!fe_basis)
		{
			display_message(ERROR_MESSAGE,
				"FE_region_get_FE_basis_matching_basis_type.  Could not get basis");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_basis_matching_basis_type.  Invalid argument(s)");
	}
	LEAVE;

	return (fe_basis);
} /* FE_region_get_FE_basis_matching_basis_type */

int Cmiss_region_attach_FE_region(struct Cmiss_region *cmiss_region,
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
Adds <fe_region> to the list of objects attached to <cmiss_region>.
To clean up, <fe_region> will be deaccessed when detached from the Cmiss_region.
If <cmiss_region> already has an FE_region, an error is reported.
This function returns it, or NULL if no FE_region found.
==============================================================================*/
{
	int return_code;
	struct Any_object *any_object;

	ENTER(Cmiss_region_attach_FE_region);
	return_code = 0;
	if (cmiss_region && fe_region &&
		((struct Cmiss_region *)NULL == fe_region->cmiss_region))
	{
		if (Cmiss_region_get_FE_region(cmiss_region))
		{
			display_message(ERROR_MESSAGE, "Cmiss_region_attach_FE_region.  "
				"Cmiss_region already has an FE_region");
		}
		else
		{
			if ((any_object = CREATE(ANY_OBJECT(FE_region))(fe_region)) &&
				Cmiss_region_private_attach_any_object(cmiss_region, any_object))
			{
				/* fe_region maintains a pointer to its cmiss_region */
				fe_region->cmiss_region = cmiss_region;
				/* access the FE_region and provide a clean-up function for the
					 any_object which will deaccess it when any_object is destroyed */
				ACCESS(FE_region)(fe_region);
				Any_object_set_cleanup_function(any_object,
					FE_region_void_detach_from_Cmiss_region);
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_region_attach_FE_region.  Could not attach object");
				DESTROY(Any_object)(&any_object);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_attach_FE_region.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* Cmiss_region_attach_FE_region */

struct FE_region *Cmiss_region_get_FE_region(struct Cmiss_region *cmiss_region)
/*******************************************************************************
LAST MODIFIED : 1 October 2002

DESCRIPTION :
Currently, a Cmiss_region may have at most one FE_region.
This function returns it, or NULL if no FE_region found.
==============================================================================*/
{
	struct FE_region *fe_region;

	ENTER(Cmiss_region_get_FE_region);
	fe_region = (struct FE_region *)NULL;
	if (cmiss_region)
	{
		fe_region = FIRST_OBJECT_IN_LIST_THAT(ANY_OBJECT(FE_region))(
			(ANY_OBJECT_CONDITIONAL_FUNCTION(FE_region) *)NULL, (void *)NULL,
			Cmiss_region_private_get_any_object_list(cmiss_region));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_region_get_FE_region.  Missing Cmiss_region");
	}
	LEAVE;

	return (fe_region);
} /* Cmiss_region_get_FE_region */

int FE_region_get_Cmiss_region(struct FE_region *fe_region,
	struct Cmiss_region **cmiss_region_address)
/*******************************************************************************
LAST MODIFIED : 12 November 2002

DESCRIPTION :
Returns in <cmiss_region_address> either the owning <cmiss_region> for
<fe_region> or NULL if none/error.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_get_Cmiss_region);
	if (fe_region && cmiss_region_address)
	{
		*cmiss_region_address = fe_region->cmiss_region;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_Cmiss_region.  Invalid argument(s)");
		if (cmiss_region_address)
		{
			*cmiss_region_address = (struct Cmiss_region *)NULL;
		}
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_get_Cmiss_region */

int FE_regions_can_be_merged(struct FE_region *global_fe_region,
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 24 March 2003

DESCRIPTION :
Returns true if definitions of fields, nodes and elements in <fe_region> are
compatible with those in <global_fe_region>, such that FE_region_merge_FE_region
should succeed. Neither region is modified.
Note order: <fe_region> is to be merged into <global_fe_region>.
=============================================================================*/
{
	int return_code;
	struct FE_element_can_be_merged_data check_elements_data;
	struct FE_node_can_be_merged_data check_nodes_data;
	struct FE_region *global_master_fe_region, *master_fe_region;
	struct LIST(FE_field) *global_fe_field_list;

	ENTER(FE_regions_can_be_merged);
	if (fe_region && global_fe_region)
	{
		if (FE_region_get_immediate_master_FE_region(fe_region,
			&master_fe_region) &&
			FE_region_get_immediate_master_FE_region(global_fe_region,
				&global_master_fe_region))
		{
			if (master_fe_region && global_master_fe_region)
			{
				/* both master_fe_region and global_master_fe_region exist so
					 can rely on them having passed this function */
				return_code = 1;
			}
			else if (master_fe_region)
			{
				display_message(ERROR_MESSAGE, "FE_regions_can_be_merged.  "
					"Cannot merge region with master into region without");
				return_code = 0;
			}
			else
			{
				return_code = 1;
				/* check fields */
				/*???RC data_hack FE_region uses master's fe_field_list */
				if (global_fe_region->top_data_hack)
				{
					global_fe_field_list = global_master_fe_region->fe_field_list;
				}
				else
				{
					global_fe_field_list = global_fe_region->fe_field_list;
				}
				/* check all fields of the same name have same definitions */
				if (!FOR_EACH_OBJECT_IN_LIST(FE_field)(FE_field_can_be_merged,
					(void *)global_fe_field_list, fe_region->fe_field_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_regions_can_be_merged.  Fields are not compatible");
					return_code = 0;
				}

				/* check nodes */
				if (return_code)
				{
					/* check nodes against the ultimate master FE_region or the first
						 data_hack region found */
					FE_region_get_ultimate_master_FE_region(global_fe_region, &global_master_fe_region);
					/* check that definition of fields in nodes of fe_region match that
						 of equivalent fields in equivalent nodes of global_fe_region */
					/* for efficiency, pairs of FE_node_field_info from the opposing nodes
						 are stored if compatible to avoid checks later */
					check_nodes_data.number_of_compatible_node_field_info = 0;
					/* store in pairs in the single array to reduce allocations */
					check_nodes_data.compatible_node_field_info =
						(struct FE_node_field_info **)NULL;
					check_nodes_data.node_list = global_master_fe_region->fe_node_list;
					if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(FE_node_can_be_merged,
						(void *)&check_nodes_data, fe_region->fe_node_list))
					{
						display_message(ERROR_MESSAGE,
							"FE_regions_can_be_merged.  Nodes are not compatible");
						return_code = 0;
					}
					DEALLOCATE(check_nodes_data.compatible_node_field_info);
				}

				/* check elements */
				if (return_code)
				{
					/* check elements against the ultimate master FE_region */
					global_master_fe_region = global_fe_region;
					while (global_master_fe_region->master_fe_region)
					{
						global_master_fe_region = global_master_fe_region->master_fe_region;
					}
					/* check that definition of fields in elements of fe_region match that
						 of equivalent fields in equivalent elements of global_fe_region */
					/* also check shape and that fields are appropriately defined on the
						 either the nodes referenced in the element or global_fe_reigon */
					/* for efficiency, pairs of FE_element_field_info from the opposing
						 elements are stored if compatible to avoid checks later */
					check_elements_data.number_of_compatible_element_field_info = 0;
					/* store in pairs in the single array to reduce allocations */
					check_elements_data.compatible_element_field_info =
						(struct FE_element_field_info **)NULL;
					check_elements_data.global_fe_region = global_master_fe_region;
					if (global_fe_region->top_data_hack)
					{
						check_elements_data.global_node_list = (struct LIST(FE_node) *)NULL;
					}
					else
					{
						check_elements_data.global_node_list =
							global_fe_region->fe_node_list;
					}
					if (!FE_region_for_each_FE_element(fe_region,
						FE_element_can_be_merged, (void *)&check_elements_data))
					{
						display_message(ERROR_MESSAGE,
							"FE_regions_can_be_merged.  Elements are not compatible");
						return_code = 0;
					}
					DEALLOCATE(check_elements_data.compatible_element_field_info);
				}
				
				/* check data */
				if (return_code && fe_region->data_fe_region)
				{
					return_code = FE_regions_can_be_merged(
						FE_region_get_data_FE_region(global_fe_region),
						fe_region->data_fe_region);
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_regions_can_be_merged.  Error getting master FE_regions");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_regions_can_be_merged.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_regions_can_be_merged */

static int FE_regions_check_master_FE_regions_against_parents(
	struct FE_region *fe_region, struct FE_region *parent_fe_region,
	struct FE_region *global_fe_region, struct FE_region *global_parent_fe_region)
/*******************************************************************************
LAST MODIFIED : 14 November 2002

DESCRIPTION :
Compares any master_FE_region for <fe_region> and <global_fe_region> for matches
against their respective parents. Returns 0 if:
* <fe_region> has a master and <global_fe_region> does not;
* both fe_regions have masters and only one matches its parent;
Note any of the arguments may be NULL.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region, *global_master_fe_region;

	ENTER(FE_regions_check_master_FE_regions_against_parents);
	if (fe_region && global_fe_region)
	{
		return_code = 0;
		if (FE_region_get_immediate_master_FE_region(fe_region,
			&master_fe_region) &&
			FE_region_get_immediate_master_FE_region(global_fe_region,
				&global_master_fe_region))
		{
			if (master_fe_region)
			{
				if (global_master_fe_region)
				{
					if ((master_fe_region == parent_fe_region) &&
						(global_master_fe_region == global_parent_fe_region))
					{
						/* ok if masters are same as respective parents */
						return_code = 1;
					}
					else if ((master_fe_region != parent_fe_region) &&
						(global_master_fe_region != global_parent_fe_region))
					{
						/* ok if masters are different from respective parents */
						return_code = 1;
					}
				}
			}
			else
			{
				/* OK if either no masters or just global_master_fe_region */
				return_code = 1;
			}
		}
	}
	else
	{
		/* OK if either fe_region or global_fe_regions absent */
		return_code = 1;
	}
	LEAVE;

	return (return_code);
} /* FE_regions_check_master_FE_regions_against_parents */

struct FE_regions_merge_with_master_data
/*******************************************************************************
LAST MODIFIED : 21 November 2002

DESCRIPTION :
Data for passing to FE_node_FE_regions_merge_with_master and
FE_element_FE_regions_merge_with_master.
==============================================================================*/
{
	struct FE_region *fe_region, *master_fe_region;
};

static int FE_node_merge_into_FE_region_with_master(struct FE_node *node,
	void *merge_data_void)
/*******************************************************************************
LAST MODIFIED : 28 November 2002

DESCRIPTION :
Tries to find the node with the same identifier as <node> in <fe_region>. If
not found, adds the node with that identifier from <master_fe_region>. The
matching node must exist in either case.
==============================================================================*/
{
	int cm_node_identifier, return_code;
	struct FE_node *add_node;
	struct FE_regions_merge_with_master_data *merge_data;

	ENTER(FE_node_merge_into_FE_region_with_master);
	if (node && (merge_data =
		(struct FE_regions_merge_with_master_data *)merge_data_void) &&
		merge_data->fe_region && merge_data->master_fe_region)
	{
		return_code = 1;
		cm_node_identifier = get_FE_node_identifier(node);
		if (!FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
			cm_node_identifier, merge_data->fe_region->fe_node_list))
		{
			if (add_node = FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
				cm_node_identifier, merge_data->master_fe_region->fe_node_list))
			{
				if (!ADD_OBJECT_TO_LIST(FE_node)(add_node,
					merge_data->fe_region->fe_node_list))
				{
					display_message(ERROR_MESSAGE,
						"FE_node_merge_into_FE_region_with_master.  Could not add to list");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_node_merge_into_FE_region_with_master.  Could not add to list");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_merge_into_FE_region_with_master.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_merge_into_FE_region_with_master */

static int FE_element_merge_into_FE_region_with_master(
	struct FE_element *element, void *merge_data_void)
/*******************************************************************************
LAST MODIFIED : 28 November 2002

DESCRIPTION :
Tries to find the element with the same identifier as <element> in <fe_region>.
If not found, adds the element with that identifier from <master_fe_region>.
The matching element must exist in either case.
==============================================================================*/
{
	int return_code;
	struct CM_element_information identifier;
	struct FE_element *add_element;
	struct FE_regions_merge_with_master_data *merge_data;

	ENTER(FE_element_merge_into_FE_region_with_master);
	int dimension = get_FE_element_dimension(element);
	if (element && (merge_data =
		(struct FE_regions_merge_with_master_data *)merge_data_void) &&
		merge_data->fe_region && merge_data->master_fe_region &&
		get_FE_element_identifier(element, &identifier))
	{
		return_code = 1;
		/* If we are defining faces and lines on the parent then we need to merge
			to ensure faces and lines are in the child region too */
		if (merge_data->master_fe_region->element_type_node_sequence_list
			|| (!FE_region_get_FE_element_from_identifier(merge_data->fe_region,
				dimension, identifier.number)))
		{
			add_element = FE_region_get_FE_element_from_identifier(
				merge_data->master_fe_region, dimension, identifier.number);
			if (add_element)
			{
				if (merge_data->master_fe_region->element_type_node_sequence_list)
				{
					/* We are defining faces so add dependent faces and lines too */
					return_code = FE_region_merge_FE_element_and_faces_private(
						merge_data->fe_region, add_element);
				}
				else
				{
					if (!ADD_OBJECT_TO_LIST(FE_element)(add_element,
						FE_region_get_element_list(merge_data->fe_region, dimension)))
					{
						display_message(ERROR_MESSAGE,
							"FE_element_merge_into_FE_region_with_master.  "
							"Could not add to list");
						return_code = 0;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_merge_into_FE_region_with_master.  Could not add to list");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_merge_into_FE_region_with_master.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_merge_into_FE_region_with_master */

int FE_regions_merge_with_master(struct FE_region *global_fe_region,
	struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
<global_fe_region> and <fe_region> must both have a master FE_region.
Makes sure that for every node and elements in <fe_region>, the equivalent node
or element from the master of <global_fe_region> is in <global_fe_region>.
Change messages stemming from this function will only inform about nodes and
elements added to <global_fe_region>, not those already in there.
Does not modify <fe_region>.
==============================================================================*/
{
	int return_code;
	struct FE_region *global_master_fe_region, *master_fe_region;
	struct FE_regions_merge_with_master_data merge_data;

	ENTER(FE_regions_merge_with_master);
	if (global_fe_region && fe_region &&
		FE_region_get_immediate_master_FE_region(global_fe_region,
			&global_master_fe_region)
		&& FE_region_get_immediate_master_FE_region(fe_region, &master_fe_region) &&
		global_master_fe_region && master_fe_region)
	{
		return_code = 1;
		FE_region_begin_change(global_fe_region);
		merge_data.fe_region = global_fe_region;
		merge_data.master_fe_region = global_master_fe_region;
		if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(
			FE_node_merge_into_FE_region_with_master, (void *)&merge_data,
			fe_region->fe_node_list))
		{
			display_message(ERROR_MESSAGE,
				"FE_regions_merge_with_master.  Could not merge nodes");
			return_code = 0;
		}
		if (!FE_region_for_each_FE_element(fe_region,
			FE_element_merge_into_FE_region_with_master, (void *)&merge_data))
		{
			display_message(ERROR_MESSAGE,
				"FE_regions_merge_with_master.  Could not merge elements");
			return_code = 0;
		}
		if (fe_region->data_fe_region)
		{
			if (!FE_regions_merge_with_master(
				FE_region_get_data_FE_region(global_fe_region), fe_region->data_fe_region))
			{
				display_message(ERROR_MESSAGE,
					"FE_regions_merge_with_master.  Could not merge data");
				return_code = 0;
			}
		}
		FE_region_end_change(global_fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_regions_merge_with_master.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_regions_merge_with_master */

static int FE_field_merge_into_FE_region(struct FE_field *fe_field,
	void *fe_region_void)
/*******************************************************************************
LAST MODIFIED : 1 May 2003

DESCRIPTION :
FE_field iterator version of FE_region_merge_FE_field.
???RC Not really happy with this; only workable because FE_region merge
consumes the region being merged in ... otherwise would have to make a clone
of the FE_field for the new FE_region.
==============================================================================*/
{
	char *indexer_fe_field_name;
	int number_of_indexed_values, return_code;
	struct FE_field *indexer_fe_field;
	struct FE_region *fe_region;

	ENTER(FE_field_merge_into_FE_region);
	if (fe_field && (fe_region = (struct FE_region *)fe_region_void))
	{
		return_code = 1;
		indexer_fe_field = (struct FE_field *)NULL;
		/* if the field is indexed, the indexer field needs to be merged first,
			 and the merged indexer field substituted */
		if (INDEXED_FE_FIELD == get_FE_field_FE_field_type(fe_field))
		{
			if (get_FE_field_type_indexed(fe_field,
				&indexer_fe_field, &number_of_indexed_values))
			{
				if (GET_NAME(FE_field)(indexer_fe_field, &indexer_fe_field_name))
				{
					if (FE_field_merge_into_FE_region(indexer_fe_field, fe_region_void))
					{
						/* get the merged indexer field */
						if (!(indexer_fe_field = FE_region_get_FE_field_from_name(fe_region,
							indexer_fe_field_name)))
						{
							return_code = 0;
						}
					}
					else
					{
						return_code = 0;
					}
					DEALLOCATE(indexer_fe_field_name);
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
		}
		if (return_code)
		{
			/* change fe_field to belong to <fe_region>;
				 substitute the indexer field if required */
			if (!(FE_field_set_FE_field_info(fe_field,
				FE_region_get_FE_field_info(fe_region)) &&
				((!indexer_fe_field) ||
					FE_field_set_indexer_field(fe_field, indexer_fe_field)) &&
				FE_region_merge_FE_field(fe_region, fe_field)))
			{
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_merge_into_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_field_merge_into_FE_region */

struct FE_regions_merge_embedding_data
/*******************************************************************************
LAST MODIFIED : 29 November 2002

DESCRIPTION :
When a region containing objects with embedded element_xi values is merged
into a global region, the elements -- often of unspecified shape -- need to be
converted into their global namesakes. Since an element does not know the
region it is in per-se, a slightly involved procedure is required to find
its global equivalent by matching against the element's element_field_info.
For now, only the root FE_region may contain elements, so only it is included
in this structure. To ease the eventual full implementation, this structure is
established and function FE_regions_merge_embedding_data_get_global_element
used to extract the element; all that is required is filling in the details.
==============================================================================*/
{
	struct FE_region *root_fe_region;
};

static struct FE_element *FE_regions_merge_embedding_data_get_global_element(
	struct FE_regions_merge_embedding_data *embedding_data,
	struct FE_element *element)
/*******************************************************************************
LAST MODIFIED : 29 November 2002

DESCRIPTION :
Returns the global element equivalent for <element> using the information in
the <embedding_data>.
==============================================================================*/
{
	struct CM_element_information identifier;
	struct FE_element *global_element;

	ENTER(FE_regions_merge_embedding_data_get_global_element);
	if (embedding_data && embedding_data->root_fe_region && element &&
		get_FE_element_identifier(element, &identifier))
	{
		int dimension = get_FE_element_dimension(element);
		global_element = FE_region_get_FE_element_from_identifier(
			embedding_data->root_fe_region, dimension, identifier.number);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_regions_merge_embedding_data_get_global_element.  "
			"Invalid argument(s)");
		global_element = (struct FE_element *)NULL;
	}
	LEAVE;

	return (global_element);
} /* FE_regions_merge_embedding_data_get_global_element */

struct FE_node_merge_into_FE_region_data
/*******************************************************************************
LAST MODIFIED : 29 November 2002

DESCRIPTION :
Data for passing to FE_node_merge_into_FE_region.
==============================================================================*/
{
	struct FE_region *fe_region;
	/* use following array and number to build up matching pairs of old node
		 field info what they become in the global_fe_region.
		 Note these are ACCESSed */
	struct FE_node_field_info **matching_node_field_info;
	int number_of_matching_node_field_info;
	/* data for merging embedded fields */
	struct FE_regions_merge_embedding_data *embedding_data;
	/* array of element_xi fields that may be defined on node prior to its being
		 merged; these require substitution of global element pointers */
	int number_of_embedded_fields;
	struct FE_field **embedded_fields;
};

/***************************************************************************//**
 * If <field> is embedded, ie. returns element_xi, adds it to the embedded_field
 * array in the FE_node_merge_into_FE_region_data.
 * @param merge_nodes_data_void  A struct FE_node_merge_into_FE_region_data *.
 */
static int FE_field_add_embedded_field_to_array(struct FE_field *field,
	void *merge_nodes_data_void)
{
	int return_code;
	struct FE_field **embedded_fields;
	struct FE_node_merge_into_FE_region_data *merge_nodes_data;

	ENTER(FE_field_add_embedded_field_to_array);
	if (field && (merge_nodes_data =
		(struct FE_node_merge_into_FE_region_data *)merge_nodes_data_void))
	{
		return_code = 1;
		if (ELEMENT_XI_VALUE == get_FE_field_value_type(field))
		{
			if (REALLOCATE(embedded_fields, merge_nodes_data->embedded_fields,
				struct FE_field *, (merge_nodes_data->number_of_embedded_fields + 1)))
			{
				/* must be mapped to a global field */
				embedded_fields[merge_nodes_data->number_of_embedded_fields] =
					FE_region_merge_FE_field(merge_nodes_data->fe_region, field);
				merge_nodes_data->embedded_fields = embedded_fields;
				merge_nodes_data->number_of_embedded_fields++;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_field_add_embedded_field_to_array.  Invalid argument(s)");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_add_embedded_field_to_array.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_field_add_embedded_field_to_array */

static int FE_node_merge_into_FE_region(struct FE_node *node,
	void *data_void)
/*******************************************************************************
LAST MODIFIED : 29 November 2002

DESCRIPTION :
FE_node iterator version of FE_region_merge_FE_node. Before merging, substitutes
into <node> an appropriate node field info from <fe_region>.
<data_void> points at a struct FE_node_merge_into_FE_region_data.
==============================================================================*/
{
	enum FE_nodal_value_type *nodal_value_types;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int component_number, i, number_of_components, number_of_derivatives,
		number_of_versions, return_code, value_number, version_number;
	struct FE_element *element, *global_element;
	struct FE_field *field;
	struct FE_node_field_info *current_node_field_info,
		**matching_node_field_info, *node_field_info;
	struct FE_node_merge_into_FE_region_data *data;
	struct FE_region *fe_region, *master_fe_region;

	ENTER(FE_node_merge_into_FE_region);
	if (node &&
		(current_node_field_info = FE_node_get_FE_node_field_info(node)) &&
		(data = (struct FE_node_merge_into_FE_region_data *)data_void) &&
		(fe_region = data->fe_region))
	{
		/* 1. Convert node to use a new FE_node_field_info from this FE_region */
		/* fast path: check if the node_field_info has already been assimilated */
		matching_node_field_info = data->matching_node_field_info;
		node_field_info = (struct FE_node_field_info *)NULL;
		for (i = 0; (i < data->number_of_matching_node_field_info) &&
			(!node_field_info); i++)
		{
			if (*matching_node_field_info == current_node_field_info)
			{
				node_field_info = *(matching_node_field_info + 1);
			}
			matching_node_field_info += 2;
		}
		if (!node_field_info)
		{
			if (fe_region->top_data_hack)
			{
				master_fe_region = fe_region->master_fe_region;
			}
			else
			{
				master_fe_region = fe_region;
			}
			if (node_field_info = FE_region_clone_FE_node_field_info(
				master_fe_region, current_node_field_info))
			{
				/* store combination of node field info in matching list */
				if (REALLOCATE(matching_node_field_info,
					data->matching_node_field_info, struct FE_node_field_info *,
					2*(data->number_of_matching_node_field_info + 1)))
				{
					matching_node_field_info[data->number_of_matching_node_field_info*2] =
						ACCESS(FE_node_field_info)(current_node_field_info);
					matching_node_field_info
						[data->number_of_matching_node_field_info*2 + 1] =
						ACCESS(FE_node_field_info)(node_field_info);
					data->matching_node_field_info = matching_node_field_info;
					(data->number_of_matching_node_field_info)++;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_node_merge_into_FE_region.  "
					"Could not clone node_field_info");
			}
		}
		if (node_field_info)
		{
			/* substitute the new node field info */
			FE_node_set_FE_node_field_info(node, node_field_info);
			return_code = 1;
			/* substitute global elements etc. in embedded fields */
			for (i = 0; i < data->number_of_embedded_fields; i++)
			{
				field = data->embedded_fields[i];
				if (FE_field_is_defined_at_node(field, node))
				{
					number_of_components = get_FE_field_number_of_components(field);
					for (component_number = 0; component_number < number_of_components;
						component_number++)
					{
						number_of_versions = get_FE_node_field_component_number_of_versions(
							node,field, component_number);
						number_of_derivatives =
							get_FE_node_field_component_number_of_derivatives(node, field,
								component_number);
						if (nodal_value_types =
							get_FE_node_field_component_nodal_value_types(node, field,
								component_number))
						{
							for (version_number = 0; version_number < number_of_versions;
								version_number++)
							{
								for (value_number = 0; value_number <= number_of_derivatives;
								 value_number++)
								{
									if (get_FE_nodal_element_xi_value(node, field,
										component_number, version_number,
										nodal_value_types[value_number], &element, xi))
									{
										if (element)
										{
											global_element =
												FE_regions_merge_embedding_data_get_global_element(
													data->embedding_data, element);
											if (global_element)
											{
												if (!set_FE_nodal_element_xi_value(node, field,
													component_number, version_number,
													nodal_value_types[value_number], global_element, xi))
												{
													return_code = 0;
												}
											}
											else
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
							}
							DEALLOCATE(nodal_value_types);
						}
						else
						{
							return_code = 0;
						}
					}
				}
			}
			/* now the actual merge! */
			if (!FE_region_merge_FE_node(data->fe_region, node))
			{
				return_code = 0;
			}
		}
		else
		{
			return_code = 0;
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "FE_node_merge_into_FE_region.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_merge_into_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_merge_into_FE_region */

struct FE_element_merge_into_FE_region_data
/*******************************************************************************
LAST MODIFIED : 26 March 2003

DESCRIPTION :
Data for passing to FE_element_merge_into_FE_region.
==============================================================================*/
{
	struct FE_region *fe_region;
	/* use following array and number to build up matching pairs of old element
		 field info what they become in the global_fe_region.
		 Note these are ACCESSed */
	struct FE_element_field_info **matching_element_field_info;
	int number_of_matching_element_field_info;
};

static int FE_element_merge_into_FE_region(struct FE_element *element,
	void *data_void)
/*******************************************************************************
LAST MODIFIED : 30 April 2003

DESCRIPTION :
FE_element iterator version of FE_region_merge_FE_element. Before merging,
substitutes into <element> an appropriate element field info from <fe_region>,
plus nodes from <fe_region> of the same number as those currently used.
<data_void> points at a struct FE_element_merge_into_FE_region_data.
==============================================================================*/
{
	int element_dimension, i, number_of_faces, number_of_nodes, return_code;
	struct CM_element_information element_identifier;
	struct FE_element *old_face, *other_element, *new_face;
	struct FE_element_merge_into_FE_region_data *data;
	struct FE_element_field_info *current_element_field_info, *element_field_info,
		**matching_element_field_info;
	struct FE_element_shape *element_shape;
	struct FE_node *new_node, *old_node;
	struct FE_region *fe_region, *master_fe_region;

	ENTER(FE_element_merge_into_FE_region);
	if (element && (current_element_field_info =
		FE_element_get_FE_element_field_info(element)) &&
		get_FE_element_shape(element, &element_shape) &&
		(data = (struct FE_element_merge_into_FE_region_data *)data_void) &&
		(fe_region = data->fe_region))
	{
		element_dimension = get_FE_element_dimension(element);
		if (FE_element_shape_is_unspecified(element_shape))
		{
			/* must find an existing element of the same dimension */
			get_FE_element_identifier(element, &element_identifier);
			master_fe_region = data->fe_region;
			if (fe_region->top_data_hack)
			{
				master_fe_region = fe_region->master_fe_region;
			}
			else
			{
				master_fe_region = fe_region;
			}
			other_element = FE_region_get_FE_element_from_identifier(master_fe_region,
				element_dimension, element_identifier.number);
			if (other_element)
			{
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_merge_into_FE_region.  No matching embedding element");
				return_code = 0;
			}
		}
		else
		{
			return_code = 1;
			/* 1. Convert element to use a new FE_element_field_info from this
				 FE_region */
			/* fast path: check if the element_field_info has already been matched */
			matching_element_field_info = data->matching_element_field_info;
			element_field_info = (struct FE_element_field_info *)NULL;
			for (i = 0; (i < data->number_of_matching_element_field_info) &&
				(!element_field_info); i++)
			{
				if (*matching_element_field_info == current_element_field_info)
				{
					element_field_info = *(matching_element_field_info + 1);
				}
				matching_element_field_info += 2;
			}
			if (!element_field_info)
			{
				if (element_field_info = FE_region_clone_FE_element_field_info(
					fe_region, current_element_field_info))
				{
					/* store combination of element field info in matching list */
					if (REALLOCATE(matching_element_field_info,
						data->matching_element_field_info, struct FE_element_field_info *,
						2*(data->number_of_matching_element_field_info + 1)))
					{
						matching_element_field_info
							[data->number_of_matching_element_field_info*2]
							= ACCESS(FE_element_field_info)(current_element_field_info);
						matching_element_field_info
							[data->number_of_matching_element_field_info*2 + 1] =
							ACCESS(FE_element_field_info)(element_field_info);
						data->matching_element_field_info = matching_element_field_info;
						(data->number_of_matching_element_field_info)++;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_element_merge_into_FE_region.  "
						"Could not clone element_field_info");
				}
			}
			if (element_field_info)
			{
				/* substitute the new element field info */
				FE_element_set_FE_element_field_info(element, element_field_info);
				/* substitute global nodes */
				if (get_FE_element_number_of_nodes(element, &number_of_nodes))
				{
					for (i = 0; (i < number_of_nodes) && return_code; i++)
					{
						if (get_FE_element_node(element, i, &old_node))
						{
							if (old_node)
							{
								if (new_node =
									FIND_BY_IDENTIFIER_IN_LIST(FE_node,cm_node_identifier)(
										get_FE_node_identifier(old_node), fe_region->fe_node_list))
								{
									if (!set_FE_element_node(element, i, new_node))
									{
										return_code = 0;
									}
								}
								else
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
				}
				else
				{
					return_code = 0;
				}
				/* substitute global faces */
				if (get_FE_element_number_of_faces(element, &number_of_faces))
				{
					int face_dimension = element_dimension - 1;
					for (i = 0; (i < number_of_faces) && return_code; i++)
					{
						if (get_FE_element_face(element, i, &old_face))
						{
							if (old_face)
							{
								if (get_FE_element_identifier(old_face, &element_identifier) &&
									(new_face = FIND_BY_IDENTIFIER_IN_LIST(FE_element,identifier)(
										&element_identifier, FE_region_get_element_list(fe_region, face_dimension))))
								{
									if (!set_FE_element_face(element, i, new_face))
									{
										return_code = 0;
									}
								}
								else
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
				}
				else
				{
					return_code = 0;
				}
				if (!FE_region_merge_FE_element_and_faces_private(data->fe_region, element))
				{
					return_code = 0;
				}
			}
			else
			{
				return_code = 0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"FE_element_merge_into_FE_region.  Failed");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_merge_into_FE_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_merge_into_FE_region */

static int FE_regions_merge(struct FE_region *global_fe_region,
	struct FE_region *fe_region,
	struct FE_regions_merge_embedding_data *embedding_data)
/*******************************************************************************
LAST MODIFIED : 26 March 2003

DESCRIPTION :
Merges into <global_fe_region> the fields, nodes and elements from <fe_region>.
Note that <fe_region> is left in a polluted state containing objects that partly
belong to the <global_fe_region> and partly to itself.
Currently it needs to be left around for the remainder of the merge up and down
the region graph, but it needs to be destroyed as soon as possible.
???RC It would be more expensive to return fe_region unchanged, especially for
nodes that are in it and added to global_fe_region, since the original is
transferred.
==============================================================================*/
{
	int i, return_code;
	struct FE_element_field_info **matching_element_field_info;
	struct FE_element_merge_into_FE_region_data merge_elements_data;
	struct FE_node_field_info **matching_node_field_info;
	struct FE_node_merge_into_FE_region_data merge_nodes_data;
	struct LIST(FE_field) *fe_field_list;

	ENTER(FE_regions_merge);
	if (global_fe_region && fe_region &&
		(!fe_region->master_fe_region || fe_region->top_data_hack) &&
		embedding_data)
	{
		return_code = 1;
		FE_region_begin_change(global_fe_region);

		if (!fe_region->top_data_hack)
		{
			/* merge fields */
			if (!FOR_EACH_OBJECT_IN_LIST(FE_field)(
				FE_field_merge_into_FE_region, (void *)global_fe_region,
				fe_region->fe_field_list))
			{
				display_message(ERROR_MESSAGE,
					"FE_regions_merge.  Could not merge fields");
				return_code = 0;
			}
		}

		/* merge nodes */
		if (return_code)
		{
			merge_nodes_data.fe_region = global_fe_region;
			/* use following array and number to build up matching pairs of old node
				 field info what they become in the global_fe_region */
			merge_nodes_data.matching_node_field_info =
				(struct FE_node_field_info **)NULL;
			merge_nodes_data.number_of_matching_node_field_info = 0;
			merge_nodes_data.embedding_data = embedding_data;
			/* work out which fields in fe_region are embedded */
			merge_nodes_data.embedded_fields = (struct FE_field **)NULL;
			merge_nodes_data.number_of_embedded_fields = 0;
			if (fe_region->top_data_hack)
			{
				fe_field_list = fe_region->master_fe_region->fe_field_list;
			}
			else
			{
				fe_field_list = fe_region->fe_field_list;
			}
			if (!FOR_EACH_OBJECT_IN_LIST(FE_field)(
				FE_field_add_embedded_field_to_array, (void *)&merge_nodes_data,
				fe_field_list))
			{
				display_message(ERROR_MESSAGE,
					"FE_regions_merge.  Could not get embedded fields");
				return_code = 0;
			}
			if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(
				FE_node_merge_into_FE_region, (void *)&merge_nodes_data,
				fe_region->fe_node_list))
			{
				display_message(ERROR_MESSAGE,
					"FE_regions_merge.  Could not merge nodes");
				return_code = 0;
			}
			if (merge_nodes_data.matching_node_field_info)
			{
				matching_node_field_info = merge_nodes_data.matching_node_field_info;
				for (i = merge_nodes_data.number_of_matching_node_field_info;
					0 < i; i--)
				{
					DEACCESS(FE_node_field_info)(matching_node_field_info);
					DEACCESS(FE_node_field_info)(matching_node_field_info + 1);
					matching_node_field_info += 2;
				}
				DEALLOCATE(merge_nodes_data.matching_node_field_info);
			}
			if (merge_nodes_data.embedded_fields)
			{
				DEALLOCATE(merge_nodes_data.embedded_fields);
			}
		}

		/* merge elements */
		if (!fe_region->top_data_hack && return_code)
		{
			/* swap elements for global equivalents */
			merge_elements_data.fe_region = global_fe_region;
			/* use following array and number to build up matching pairs of old
				 element field info what they become in the global_fe_region */
			merge_elements_data.matching_element_field_info =
				(struct FE_element_field_info **)NULL;
			merge_elements_data.number_of_matching_element_field_info = 0;
			/* merge elements from lowest to highest dimension so that faces are
				 merged before their parent elements */
			for (int dimension = 1; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
			{
				if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(
					FE_element_merge_into_FE_region, (void *)&merge_elements_data,
					FE_region_get_element_list(fe_region, dimension)))
				{
					display_message(ERROR_MESSAGE,
						"FE_regions_merge.  Could not merge elements");
					return_code = 0;
					break;
				}
			}
			if (merge_elements_data.matching_element_field_info)
			{
				matching_element_field_info =
					merge_elements_data.matching_element_field_info;
				for (i = merge_elements_data.number_of_matching_element_field_info;
					0 < i; i--)
				{
					DEACCESS(FE_element_field_info)(matching_element_field_info);
					DEACCESS(FE_element_field_info)(matching_element_field_info + 1);
					matching_element_field_info += 2;
				}
				DEALLOCATE(merge_elements_data.matching_element_field_info);
			}
		}
		
		/* merge data */
		if (return_code && fe_region->data_fe_region)
		{
			return_code = FE_regions_merge(
				FE_region_get_data_FE_region(global_fe_region),
				fe_region->data_fe_region, embedding_data);
		}
		FE_region_end_change(global_fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_regions_merge.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_regions_merge */

/***************************************************************************//**
 * Returns true if the FE_regions within the region tree can be merged into
 * the global_region tree.
 * 
 * @param global_region  Region to check merge into.
 * @param region  Source region tree to check merge.
 * @return  1 if merge is possible, 0 if not i.e. some objects are incompatible.
 */
int Cmiss_regions_FE_regions_can_be_merged(
	struct Cmiss_region *global_region, struct Cmiss_region *region)
{
	char *child_region_name, *global_path, *path;
	int return_code;
	struct Cmiss_region *child_region, *global_child_region;
	struct FE_region *fe_region, *global_fe_region;

	ENTER(Cmiss_regions_FE_regions_can_be_merged);
	if (global_region && region)
	{
		return_code = 1;
		/* check FE_regions */
		fe_region = Cmiss_region_get_FE_region(region);
		global_fe_region = Cmiss_region_get_FE_region(global_region);
		if (!FE_regions_can_be_merged(global_fe_region, fe_region))
		{
			global_path = Cmiss_region_get_path(global_region);
			path = Cmiss_region_get_path(region);
			display_message(ERROR_MESSAGE,
				"Cannot merge incompatible fields from temporary region %s into %s", path, global_path);
			DEALLOCATE(path);
			DEALLOCATE(global_path);
			return_code = 0;
		}
		/* check child regions can be merged */
		child_region = Cmiss_region_get_first_child(region);
		while ((NULL != child_region) && return_code)
		{
			child_region_name = Cmiss_region_get_name(child_region);
			global_child_region =
				Cmiss_region_find_child_by_name(global_region, child_region_name);
			if (global_child_region)
			{
				return_code = 
					FE_regions_check_master_FE_regions_against_parents(
						fe_region, Cmiss_region_get_FE_region(child_region),
						global_fe_region, Cmiss_region_get_FE_region(global_child_region)) &&
					Cmiss_regions_FE_regions_can_be_merged(
						global_child_region, child_region);
			}
			Cmiss_region_destroy(&global_child_region);
			DEALLOCATE(child_region_name);
			Cmiss_region_reaccess_next_sibling(&child_region);
		}
		REACCESS(Cmiss_region)(&child_region, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_regions_FE_regions_can_be_merged.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_regions_FE_regions_can_be_merged */

/****************************************************************************//**
 * Merges the fields, nodes and elements of region into global_region.
 * 
 * @param global_region  Region to merge into.
 * @param region  Source region to merge.
 * @param merge_only_existing_regions  If set, only regions with existing
 * global_region counterparts are merged in this sweep; should call this function
 * a second time without this flag set in order to merge all regions.
 * @param new_global_region  Set to 1 if global_region was created immediately
 * before this call (by this function, nested) and therefore must be merged.
 * @param embedding_data  Contains root region for embedded element:xi locations.
 * @return  1 if merge was successful, 0 if not.
 */
static int Cmiss_regions_merge_FE_regions_private(
	struct Cmiss_region *global_region, struct Cmiss_region *region,
	int merge_only_existing_regions, int new_global_region,
	struct FE_regions_merge_embedding_data *embedding_data)
{
	char *child_region_name, *global_path, *path;
	int new_global_child, return_code;
	struct Cmiss_region *child_region, *global_child_region;
	struct FE_region *fe_region, *global_fe_region;

	ENTER(Cmiss_regions_merge_FE_regions_private);
	if (global_region && region && embedding_data)
	{
		return_code = 1;
		Cmiss_region_begin_change(global_region);

		if (merge_only_existing_regions || new_global_region)
		{
			fe_region = Cmiss_region_get_FE_region(region);
			global_fe_region = Cmiss_region_get_FE_region(global_region);
			if (Cmiss_region_is_group(region))
			{
				return_code = FE_regions_merge_with_master(global_fe_region, fe_region);
			}
			else
			{
				return_code = FE_regions_merge(global_fe_region, fe_region, embedding_data);
			}
			if (!return_code)
			{
				global_path = Cmiss_region_get_path(global_region);
				path = Cmiss_region_get_path(region);
				display_message(ERROR_MESSAGE,
					"Could not merge temporary region %s into %s", path, global_path);
				DEALLOCATE(path);
				DEALLOCATE(global_path);
			}
		}

		/* merge child regions */
		child_region = Cmiss_region_get_first_child(region);
		while ((NULL != child_region) && return_code)
		{
			child_region_name = Cmiss_region_get_name(child_region);
			global_child_region =
				Cmiss_region_find_child_by_name(global_region, child_region_name);
			new_global_child = 0;
			if ((NULL == global_child_region) && (!merge_only_existing_regions))
			{
				if (Cmiss_region_is_group(child_region))
				{
					global_child_region = Cmiss_region_create_group(global_region);
				}
				else
				{
					global_child_region =
						Cmiss_region_create_region(global_region);
				}
				new_global_child = 1;
				Cmiss_region_set_name(global_child_region, child_region_name);
				if (!Cmiss_region_append_child(global_region, global_child_region))
				{
					return_code = 0;
				}
			}
			if (global_child_region && return_code)
			{
				return_code = Cmiss_regions_merge_FE_regions_private(
					global_child_region, child_region, merge_only_existing_regions,
					new_global_child, embedding_data);
			}
			Cmiss_region_destroy(&global_child_region);
			DEALLOCATE(child_region_name);
			Cmiss_region_reaccess_next_sibling(&child_region);
		}
		REACCESS(Cmiss_region)(&child_region, NULL);

		Cmiss_region_end_change(global_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_regions_merge_FE_regions_private.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_regions_merge_FE_regions_private */

int Cmiss_regions_merge_FE_regions(struct Cmiss_region *global_region,
	struct Cmiss_region *region)
/*******************************************************************************
LAST MODIFIED : 18 June 2003

DESCRIPTION :
Merges into <global_region> the fields, nodes and elements from <region>.
It is expected that Cmiss_regions_FE_regions_can_be_merged has already been
passed for the two regions; if so the merge should proceed without error.
The can_be_merged check should not be necessary if the members of <region> have
been extracted from <global_region> in the first place and only field values
changed or new objects added.

IMPORTANT NOTE:
Caller should destroy <region> after calling this function since the FE_regions
it contains will be significantly modified during the merge. It would be more
expensive to keep FE_regions unchanged during the merge, a behaviour not
required at this time for import functions. If this is required in future,
FE_regions_merge would have to be changed.
==============================================================================*/
{
	int return_code;
	struct FE_regions_merge_embedding_data embedding_data;

	ENTER(Cmiss_regions_merge_FE_regions);
	if (global_region && region)
	{
		Cmiss_region_begin_change(global_region);
		/*???RC embedding data currently only allows embedding elements to be in
			the root region */
		embedding_data.root_fe_region = Cmiss_region_get_FE_region(global_region);
		if (embedding_data.root_fe_region&&
			(embedding_data.root_fe_region->top_data_hack))
		{
			embedding_data.root_fe_region =
				embedding_data.root_fe_region->master_fe_region;
		}
		/* merge existing regions before new regions to handle embedding cleanly */
		/* ???GRC not a complete solution */
		return_code = Cmiss_regions_merge_FE_regions_private(
			global_region, region, /*merge_only_existing_regions*/1,
			/*new_global_region*/0, &embedding_data) &&
			Cmiss_regions_merge_FE_regions_private(
				global_region, region, /*merge_only_existing_regions*/0,
				/*new_global_region*/0, &embedding_data);
		Cmiss_region_end_change(global_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_regions_merge_FE_regions.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_regions_merge_FE_regions */

int FE_region_notify_FE_node_field_change(struct FE_region *fe_region,
	struct FE_node *node, struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 21 April 2005

DESCRIPTION :
Tells the <fe_region> to notify any interested clients that the <node> has
been modified only for <fe_field>.  This is intended to be called by 
<finite_element.c> only as any external code will call through the modify
functions in <finite_element.c>.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_notify_FE_node_field_change);
	if (fe_region && node && fe_field)
	{
		return_code = 1;
		if (FE_region_contains_FE_node(fe_region, node))
		{
			FE_region_get_ultimate_master_FE_region(fe_region, &master_fe_region); 
			FE_REGION_FE_NODE_FIELD_CHANGE(master_fe_region, node, fe_field);
		}
		/* If we notify the region indicated by the node_field_info of changes 
			to a data point then this will be the parent of the data_fe_region,
			so we need to check for this node in the data hack region too */
		else if (fe_region->data_fe_region && 
			FE_region_contains_FE_node(fe_region->data_fe_region, node))
		{
			FE_REGION_FE_NODE_FIELD_CHANGE(fe_region->data_fe_region, 
				node, fe_field);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_notify_FE_node_field_change.  Node is not in region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_notify_FE_node_field_change.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_notify_FE_node_field_change */

int FE_region_notify_FE_element_field_change(struct FE_region *fe_region,
	struct FE_element *element, struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 21 April 2005

DESCRIPTION :
Tells the <fe_region> to notify any interested clients that the <element> has
been modified only for <fe_field>.  This is intended to be called by 
<finite_element.c> only as any external code will call through the modify
functions in <finite_element.c>.
==============================================================================*/
{
	int return_code;
	struct FE_region *master_fe_region;

	ENTER(FE_region_notify_FE_element_field_change);
	if (fe_region && element && fe_field)
	{
		if (FE_region_contains_FE_element(fe_region, element))
		{
			return_code = 1;
			/* get the ultimate master fe_region */
			master_fe_region = fe_region;
			while (master_fe_region->master_fe_region)
			{
				master_fe_region = master_fe_region->master_fe_region;
			}
			FE_REGION_FE_ELEMENT_FIELD_CHANGE(master_fe_region, element, fe_field);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_notify_FE_element_field_change.  Element is not in region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_notify_FE_element_field_change.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_region_notify_FE_element_field_change */

int FE_region_need_add_cmiss_number_field(struct FE_region *fe_region)
{
	int return_code;

	ENTER(FE_region_need_add_cmiss_number_field);
	return_code = 0;
	if (fe_region)
	{
		if (!fe_region->informed_make_cmiss_number_field)
		{
			if (NUMBER_IN_LIST(FE_node)(fe_region->fe_node_list) ||
				FE_region_get_number_of_FE_elements_all_dimensions(fe_region) ||
				(fe_region->data_fe_region && 
					NUMBER_IN_LIST(FE_node)(fe_region->data_fe_region->fe_node_list)))
			{
				fe_region->informed_make_cmiss_number_field = 1;
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_need_add_cmiss_number_field.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_need_add_cmiss_number_field */

int FE_region_need_add_xi_field(struct FE_region *fe_region)
{
	int return_code;

	ENTER(FE_region_need_add_xi_field);
	return_code = 0;
	if (fe_region)
	{
		if (!fe_region->informed_make_xi_field)
		{
			if (FE_region_get_number_of_FE_elements_all_dimensions(fe_region))
			{
				fe_region->informed_make_xi_field = 1;
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_need_add_xi_field.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_need_add_xi_field */

struct LIST(FE_element_field_info) *FE_region_get_FE_element_field_info_list_private(
	struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->fe_element_field_info_list;
	return NULL;
}
