/*******************************************************************************
FILE : managed_group_private.h

LAST MODIFIED : 22 June 2000

DESCRIPTION :
Special version of group_private.h for groups that are managed. These groups
maintain a pointer to their manager while they are managed, so that adding and
removing objects from the managed group results in MANAGER_MODIFY messages
being sent to clients of the group_manager. As a result, indexed_list and
manager functions are declared automatically with the managed_group.

Since many of the functions from group_private.h remain unchanged for
managed_groups, group_private.h is #included into this file, and the routines
are reused. Two additional functions are provided for caching and uncaching
manager messages, so that if, for example, several objects are being added or
removed from a group, only one modify message is sent. This is especially
useful for graphical finite elements which respond to each modify message with
an automatic redraw.

Description from group_private.h:
Macros for declaring standard group types and declaring standard group
functions.  This file contains the details of the internal workings of the group
and should be included in the module that declares the group for a particular
object type, but should not be visible to modules that use groups for a
particular object type.  This allows changes to the internal structure to be
made without affecting other parts of the program.
???DB.  Access counts and destroying ?
==============================================================================*/
#if !defined (MANAGED_GROUP_PRIVATE_H)
#define MANAGED_GROUP_PRIVATE_H
#include <string.h>
#include "general/group.h"
#include "general/group_private.h"
#include "general/indexed_list_private.h"
#include "general/manager_private.h"
#include "general/object.h"

/*
Macros
======
*/

/*
Local types
-----------
*/
#define FULL_DECLARE_MANAGED_GROUP_TYPES( object_type ) \
DECLARE_GROUP_TYPE(object_type) \
/***************************************************************************** \
LAST MODIFIED : 10 February 1998 \
\
DESCRIPTION : \
A named list of object_type. \
============================================================================*/ \
{ \
	/* the name of the group */ \
	char *name; \
	/* a list of the object_type in the group */ \
	struct LIST(object_type) *object_list; \
	/* Keep pointer to group manager while managed so that adding and */ \
	/* removing objects from a group results in manager modify messages */\
	struct MANAGER(GROUP(object_type)) *group_manager; \
	/* manager modify messages are suspended while cache is non-zero. Allows */ \
	/* a single modify message to be sent for multiple adds and removes. */ \
	int cache; \
	/* the number of structures that point to this group.  The group cannot be \
		destroyed while this is greater than 0 */ \
	int access_count; \
}; /* struct GROUP(object_type) */ \
FULL_DECLARE_INDEXED_LIST_TYPE(GROUP(object_type)); \
FULL_DECLARE_MANAGER_TYPE(GROUP(object_type))

/*
Module functions
----------------
*/
#if defined (FULL_NAMES)
#define MANAGED_GROUP_MODIFY_MESSAGE( object_type ) \
	managed_group_modify_message_ ## object_type
#else
#define MANAGED_GROUP_MODIFY_MESSAGE( object_type )  mgm ## object_type
#endif

#define DECLARE_MANAGED_GROUP_MODIFY_MESSAGE_FUNCTION( object_type, \
	group_type ) \
static int MANAGED_GROUP_MODIFY_MESSAGE(object_type)( \
	struct group_type *group) \
/***************************************************************************** \
LAST MODIFIED : 22 June 2000 \
\
DESCRIPTION : \
If the group is managed, sends a MANAGER_MODIFY message to inform clients of \
its manager that the group has changed. \
???RC Originally wanted to pass object_type as macro argument, but can't work \
because GROUP(object_type) is not substituted as I would like. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(MANAGED_GROUP_MODIFY_MESSAGE(object_type)); \
	if (group) \
	{ \
		if (group->group_manager) \
		{ \
			MANAGER_NOTE_CHANGE(group_type)( \
				MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(group_type),group, \
				group->group_manager); \
			return_code=1; \
		} \
		else  \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGED_GROUP_MODIFY_MESSAGE(" #object_type ").  Group not managed"); \
			return_code=0; \
		} \
	} \
	else  \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGED_GROUP_MODIFY_MESSAGE(" #object_type ").  Missing group"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGED_GROUP_MODIFY_MESSAGE(object_type) */

/*
Global functions
----------------
*/
#define DECLARE_CREATE_MANAGED_GROUP_FUNCTION( object_type ) \
/***************************************************************************** \
LAST MODIFIED : 10 February 1998 \
\
DESCRIPTION : \
Added lines to clear cache flag and pointer to group_manager. \
============================================================================*/ \
PROTOTYPE_CREATE_GROUP_FUNCTION(object_type) \
{ \
	struct GROUP(object_type) *group; \
\
	ENTER(CREATE_GROUP(object_type)); \
	/* allocate memory for the group */ \
	if (ALLOCATE(group,struct GROUP(object_type),1)) \
	{ \
		/* allocate memory for and assign the group name */ \
		if (name) \
		{ \
			if (ALLOCATE(group->name,char,strlen(name)+1)) \
			{ \
				strcpy(group->name,name); \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
"CREATE_GROUP(" #object_type ").  Could not allocate memory for group name"); \
				DEALLOCATE(group); \
			} \
		} \
		else \
		{ \
			if (ALLOCATE(group->name,char,1)) \
			{ \
				*(group->name)='\0'; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
"CREATE_GROUP(" #object_type ").  Could not allocate memory for null group name"); \
				DEALLOCATE(group); \
			} \
		} \
		if (group) \
		{ \
			if (group->object_list=CREATE_LIST(object_type)()) \
			{ \
				group->group_manager=(struct MANAGER(GROUP(object_type)) *)NULL; \
				group->cache=0; \
				group->access_count=0; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE,"CREATE_GROUP(" #object_type \
					").  Could not create list"); \
				DEALLOCATE(group->name); \
				DEALLOCATE(group); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"CREATE_GROUP(" #object_type \
			").  Could not allocate memory for " #object_type " group"); \
	} \
	LEAVE; \
\
	return (group); \
} /* CREATE_GROUP(object_type) */

#define DECLARE_REMOVE_OBJECT_FROM_MANAGED_GROUP( object_type ) \
PROTOTYPE_REMOVE_OBJECT_FROM_GROUP(object_type) \
{ \
	int return_code; \
\
	ENTER(REMOVE_OBJECT_FROM_GROUP(object_type)); \
	/* check the arguments */ \
	if (object&&group) \
	{ \
		if (return_code=REMOVE_OBJECT_FROM_LIST(object_type)(object, \
			group->object_list)) \
		{ \
			if (group->group_manager&&!group->cache) \
			{ \
				MANAGED_GROUP_MODIFY_MESSAGE(object_type)(group); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECT_FROM_GROUP(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_OBJECT_FROM_GROUP(object_type) */

#define DECLARE_REMOVE_OBJECTS_FROM_MANAGED_GROUP_THAT( object_type ) \
PROTOTYPE_REMOVE_OBJECTS_FROM_GROUP_THAT(object_type) \
{ \
	int return_code; \
\
	ENTER(REMOVE_OBJECTS_FROM_GROUP_THAT(object_type)); \
	/* check the arguments */ \
	if (conditional&&group) \
	{ \
		if (return_code=REMOVE_OBJECTS_FROM_LIST_THAT(object_type)(conditional, \
			user_data,group->object_list)) \
		{ \
			if (group->group_manager&&!group->cache) \
			{ \
				MANAGED_GROUP_MODIFY_MESSAGE(object_type)(group); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
		"REMOVE_OBJECT_FROM_GROUP_THAT(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_OBJECTS_FROM_GROUP_THAT(object_type) */

#define DECLARE_REMOVE_ALL_OBJECTS_FROM_MANAGED_GROUP( object_type ) \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_GROUP(object_type) \
{ \
	int return_code; \
\
	ENTER(REMOVE_ALL_OBJECTS_FROM_GROUP(object_type)); \
	/* check the arguments */ \
	if (group) \
	{ \
		if (return_code=REMOVE_ALL_OBJECTS_FROM_LIST(object_type)( \
			group->object_list)) \
		{ \
			if (group->group_manager&&!group->cache) \
			{ \
				MANAGED_GROUP_MODIFY_MESSAGE(object_type)(group); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
		"REMOVE_ALL_OBJECTS_FROM_GROUP(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_ALL_OBJECTS_FROM_GROUP(object_type) */

#define DECLARE_ADD_OBJECT_TO_MANAGED_GROUP( object_type ) \
PROTOTYPE_ADD_OBJECT_TO_GROUP(object_type) \
{ \
	int return_code; \
\
	ENTER(ADD_OBJECT_TO_GROUP(object_type)); \
	/* check the arguments */ \
	if (object&&group) \
	{ \
		if (return_code=ADD_OBJECT_TO_LIST(object_type)(object, \
			group->object_list)) \
		{ \
			if (group->group_manager&&!group->cache) \
			{ \
				MANAGED_GROUP_MODIFY_MESSAGE(object_type)(group); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"ADD_OBJECT_TO_GROUP(" #object_type  \
			").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* ADD_OBJECT_TO_GROUP(object_type) */

#define DECLARE_MANAGED_GROUP_BEGIN_CACHE_FUNCTION( object_type ) \
PROTOTYPE_MANAGED_GROUP_BEGIN_CACHE_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(GROUP_BEGIN_CACHE(object_type)); \
	/* check the arguments */ \
	if (group) \
	{ \
		if (group->cache) \
		{ \
			display_message(ERROR_MESSAGE, \
				"GROUP_BEGIN_CACHE(" #object_type ").  Caching already enabled"); \
			return_code=0; \
		} \
		else \
		{ \
			group->cache=1; \
			return_code=1; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GROUP_BEGIN_CACHE(" #object_type ").  Invalid argument"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GROUP_BEGIN_CACHE(object_type) */

#define DECLARE_MANAGED_GROUP_END_CACHE_FUNCTION( object_type ) \
PROTOTYPE_MANAGED_GROUP_END_CACHE_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(GROUP_END_CACHE(object_type)); \
	/* check the arguments */ \
	if (group) \
	{ \
		if (group->cache) \
		{ \
			group->cache=0; \
			if (group->group_manager) \
			{ \
				MANAGED_GROUP_MODIFY_MESSAGE(object_type)(group); \
			} \
			return_code=1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GROUP_END_CACHE(" #object_type ").  Caching not enabled"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GROUP_END_CACHE(" #object_type ").  Invalid argument"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GROUP_END_CACHE(object_type) */

#define DECLARE_MANAGED_GROUP_CAN_BE_DESTROYED_FUNCTION( object_type ) \
PROTOTYPE_MANAGED_GROUP_CAN_BE_DESTROYED_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(GROUP_CAN_BE_DESTROYED(object_type)); \
	if (group) \
	{ \
		return_code=(group->access_count == 1); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GROUP_CAN_BE_DESTROYED(" #object_type ").  Invalid argument"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GROUP_CAN_BE_DESTROYED(object_type) */

#define DECLARE_MANAGED_GROUP_MODULE_FUNCTIONS( object_type ) \
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(GROUP(object_type),name,char *,strcmp) \
DECLARE_LOCAL_MANAGER_FUNCTIONS(GROUP(object_type)) \
DECLARE_MANAGED_GROUP_MODIFY_MESSAGE_FUNCTION(object_type,GROUP(object_type))

#define DECLARE_MANAGED_GROUP_FUNCTIONS( object_type ) \
DECLARE_OBJECT_FUNCTIONS(GROUP(object_type)) \
DECLARE_CREATE_MANAGED_GROUP_FUNCTION(object_type) \
DECLARE_DESTROY_GROUP_FUNCTION(object_type) \
DECLARE_COPY_GROUP_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECT_FROM_MANAGED_GROUP(object_type) \
DECLARE_REMOVE_OBJECTS_FROM_MANAGED_GROUP_THAT(object_type) \
DECLARE_REMOVE_ALL_OBJECTS_FROM_MANAGED_GROUP(object_type) \
DECLARE_ADD_OBJECT_TO_MANAGED_GROUP(object_type) \
DECLARE_NUMBER_IN_GROUP(object_type) \
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(GROUP(object_type)) \
DECLARE_FIRST_OBJECT_IN_GROUP_THAT_FUNCTION(object_type) \
DECLARE_FOR_EACH_OBJECT_IN_GROUP_FUNCTION(object_type) \
DECLARE_IS_OBJECT_IN_GROUP_FUNCTION(object_type) \
DECLARE_MANAGED_GROUP_BEGIN_CACHE_FUNCTION(object_type) \
DECLARE_MANAGED_GROUP_END_CACHE_FUNCTION(object_type) \
DECLARE_MANAGED_GROUP_CAN_BE_DESTROYED_FUNCTION(object_type) \
DECLARE_INDEXED_LIST_FUNCTIONS(GROUP(object_type)) \
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(GROUP(object_type),name, \
	char *,strcmp) \
DECLARE_GROUP_MANAGER_COPY_FUNCTIONS(object_type) \
DECLARE_MANAGER_FUNCTIONS(GROUP(object_type)) \
DECLARE_INDEXED_LIST_IDENTIFIER_CHANGE_FUNCTIONS(GROUP(object_type),name) \
DECLARE_OBJECT_WITH_MANAGER_MANAGER_IDENTIFIER_FUNCTIONS( \
	GROUP(object_type),name,char *,group_manager)

#endif /* defined (MANAGED_GROUP_PRIVATE_H) */
