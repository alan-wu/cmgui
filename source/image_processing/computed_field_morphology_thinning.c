/******************************************************************
  FILE: computed_field_morphology_thinning.c

  LAST MODIFIED: 27 October 2004

  DESCRIPTION:Implement image morphological thinning
==================================================================*/
#include <math.h>
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_find_xi.h"
#include "computed_field/computed_field_private.h"
#include "computed_field/computed_field_set.h"
#include "image_processing/image_cache.h"
#include "general/image_utilities.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "user_interface/message.h"
#include "image_processing/computed_field_morphology_thinning.h"


#define my_Min(x,y) ((x) <= (y) ? (x) : (y))
#define my_Max(x,y) ((x) <= (y) ? (y) : (x))
#define n(x,y,z) ((z)*(image->sizes[1])*(image->sizes[0]) + (y) * (image->sizes[0]) + (x))


struct Computed_field_morphology_thinning_package
/*******************************************************************************
LAST MODIFIED : 17 February 2004

DESCRIPTION :
A container for objects required to define fields in this module.
==============================================================================*/
{
	struct MANAGER(Computed_field) *computed_field_manager;
	struct Cmiss_region *root_region;
	struct Graphics_buffer_package *graphics_buffer_package;
};


struct Computed_field_morphology_thinning_type_specific_data
{
	int iteration_times;

	float cached_time;
	int element_dimension;
	struct Cmiss_region *region;
	struct Graphics_buffer_package *graphics_buffer_package;
	struct Image_cache *image;
	struct MANAGER(Computed_field) *computed_field_manager;
	void *computed_field_manager_callback_id;
};

static char computed_field_morphology_thinning_type_string[] = "morphology_thinning";

int Computed_field_is_type_morphology_thinning(struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_type_morphology_thinning);
	if (field)
	{
		return_code =
		  (field->type_string == computed_field_morphology_thinning_type_string);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_type_morphology_thinning.  Missing field");
		return_code = 0;
	}

	return (return_code);
} /* Computed_field_is_type_morphology_thinning */

static void Computed_field_morphology_thinning_field_change(
	struct MANAGER_MESSAGE(Computed_field) *message, void *field_void)
/*******************************************************************************
LAST MODIFIED : 5 December 2003

DESCRIPTION :
Manager function called back when either of the source fields change so that
we know to invalidate the image cache.
==============================================================================*/
{
	struct Computed_field *field;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_source_field_change);
	if (message && (field = (struct Computed_field *)field_void) && (data =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data))
	{
		switch (message->change)
		{
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Computed_field):
			case MANAGER_CHANGE_OBJECT(Computed_field):
			{
				if (Computed_field_depends_on_Computed_field_in_list(
					field->source_fields[0], message->changed_object_list) ||
					Computed_field_depends_on_Computed_field_in_list(
					field->source_fields[1], message->changed_object_list))
				{
					if (data->image)
					{
						data->image->valid = 0;
					}
				}
			} break;
			case MANAGER_CHANGE_ADD(Computed_field):
			case MANAGER_CHANGE_REMOVE(Computed_field):
			case MANAGER_CHANGE_IDENTIFIER(Computed_field):
			{
				/* do nothing */
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_source_field_change.  "
			"Invalid arguments.");
	}
	LEAVE;
} /* Computed_field_morphology_thinning_source_field_change */

static int Computed_field_morphology_thinning_clear_type_specific(
	struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_clear_type_specific);
	if (field && (data =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data))
	{
		if (data->region)
		{
			DEACCESS(Cmiss_region)(&data->region);
		}
		if (data->image)
		{
			DEACCESS(Image_cache)(&data->image);
		}
		if (data->computed_field_manager && data->computed_field_manager_callback_id)
		{
			MANAGER_DEREGISTER(Computed_field)(
				data->computed_field_manager_callback_id,
				data->computed_field_manager);
		}
		DEALLOCATE(field->type_specific_data);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_clear_type_specific.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_morphology_thinning_clear_type_specific */

static void *Computed_field_morphology_thinning_copy_type_specific(
	struct Computed_field *source_field, struct Computed_field *destination_field)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	struct Computed_field_morphology_thinning_type_specific_data *destination,
		*source;

	ENTER(Computed_field_morphology_thinning_copy_type_specific);
	if (source_field && destination_field && (source =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		source_field->type_specific_data))
	{
		if (ALLOCATE(destination,
			struct Computed_field_morphology_thinning_type_specific_data, 1))
		{
			destination->iteration_times = source->iteration_times;
			destination->cached_time = source->cached_time;
			destination->region = ACCESS(Cmiss_region)(source->region);
			destination->element_dimension = source->element_dimension;
			destination->graphics_buffer_package = source->graphics_buffer_package;
			destination->computed_field_manager = source->computed_field_manager;
			destination->computed_field_manager_callback_id =
				MANAGER_REGISTER(Computed_field)(
				Computed_field_morphology_thinning_field_change, (void *)destination_field,
				destination->computed_field_manager);
			if (source->image)
			{
				destination->image = ACCESS(Image_cache)(CREATE(Image_cache)());
				Image_cache_update_dimension(destination->image,
					source->image->dimension, source->image->depth,
					source->image->sizes, source->image->minimums,
					source->image->maximums);
			}
			else
			{
				destination->image = (struct Image_cache *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_morphology_thinning_copy_type_specific.  "
				"Unable to allocate memory.");
			destination = NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_copy_type_specific.  "
			"Invalid arguments.");
		destination = NULL;
	}
	LEAVE;

	return (destination);
} /* Computed_field_morphology_thinning_copy_type_specific */

int Computed_field_morphology_thinning_clear_cache_type_specific
   (struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_clear_type_specific);
	if (field && (data =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data))
	{
		if (data->image)
		{
			/* data->image->valid = 0; */
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_clear_type_specific.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_morphology_thinning_clear_type_specific */

static int Computed_field_morphology_thinning_type_specific_contents_match(
	struct Computed_field *field, struct Computed_field *other_computed_field)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data,
		*other_data;

	ENTER(Computed_field_morphology_thinning_type_specific_contents_match);
	if (field && other_computed_field && (data =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data) && (other_data =
		(struct Computed_field_morphology_thinning_type_specific_data *)
		other_computed_field->type_specific_data))
	{
		if ((data->iteration_times == other_data->iteration_times) &&
			data->image && other_data->image &&
			(data->image->dimension == other_data->image->dimension) &&
			(data->image->depth == other_data->image->depth))
		{
			/* Check sizes and minimums and maximums */
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
} /* Computed_field_morphology_thinning_type_specific_contents_match */

#define Computed_field_morphology_thinning_is_defined_in_element \
	Computed_field_default_is_defined_in_element
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Check the source fields using the default.
==============================================================================*/

#define Computed_field_morphology_thinning_is_defined_at_node \
	Computed_field_default_is_defined_at_node
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Check the source fields using the default.
==============================================================================*/

#define Computed_field_morphology_thinning_has_numerical_components \
	Computed_field_default_has_numerical_components
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Window projection does have numerical components.
==============================================================================*/

#define Computed_field_morphology_thinning_not_in_use \
	(Computed_field_not_in_use_function)NULL
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
No special criteria.
==============================================================================*/
static int morphology_open(FE_value *X_index,FE_value *E_index, FE_value *E1_index, int *sizes, int direction)
/*************************************************************************************************************
LAST MODIFIED: 28 October 2004
DESCRIPTION: 'direction' = 1, 2, 3,..., 6. 1-y+direction, 2-z-direction, 3-y-direction, 4-z+direction,
              5-x-direction, 6-x+direction.
=============================================================================================================*/
{
                int return_code;
	int x, y, z, current, neighbor;
	ENTER(morphology_open);
	return_code = 1;
	if (direction == 1)
	{
                for (z = 0 ; z < sizes[2] ; z++) /* erosion */
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= (z*sizes[1] + y+1)*sizes[0] + x;   
						if ((y+1) < sizes[1])
						{
							if (X_index[neighbor]==1.0)
							{
								E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
					        E_index[current] = 0.0;
					}
				}
			}
		} /* erosion */
		for (z = 0 ; z < sizes[2] ; z++)/* dilation */
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= (z*sizes[1] + y-1)*sizes[0] + x;   
					if ((y-1) >= 0)
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		} /* dilation */
	}
	else if (direction == 2)
	{
	        for (z = 0 ; z < sizes[2] ; z++) /*-z-direction*/
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= ((z-1)*sizes[1] + y)*sizes[0] + x;   
						if ((z-1) >= 0)
						{
							if(X_index[neighbor]==1.0)
							{
							        E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
						E_index[current] = 0.0;
					}
				}
			}
		}
		for (z = 0 ; z < sizes[2] ; z++)
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= ((z+1)*sizes[1] + y)*sizes[0] + x;   
					if ((z+1) < sizes[2])
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		}
	}
	else if (direction == 3)
	{
	        for (z = 0 ; z < sizes[2] ; z++) /* erosion */
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= (z*sizes[1] + y-1)*sizes[0] + x;   
						if ((y-1) >= 0)
						{
							if (X_index[neighbor]==1.0)
							{
								E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
					        E_index[current] = 0.0;
					}
				}
			}
		} /* erosion */
		for (z = 0 ; z < sizes[2] ; z++)/* dilation */
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= (z*sizes[1] + y+1)*sizes[0] + x;   
					if ((y+1) < sizes[1])
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		} /* dilation */
	}
	else if (direction == 4)
	{
	        for (z = 0 ; z < sizes[2] ; z++) /*-z-direction*/
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= ((z+1)*sizes[1] + y)*sizes[0] + x;   
						if ((z+1) < sizes[2])
						{
							if(X_index[neighbor]==1.0)
							{
							        E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
						E_index[current] = 0.0;
					}
				}
			}
		}
		for (z = 0 ; z < sizes[2] ; z++)
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= ((z-1)*sizes[1] + y)*sizes[0] + x;   
					if ((z-1) >= 0)
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		}
	}
	else if (direction == 5)
	{
	       for (z = 0 ; z < sizes[2] ; z++) /*-x-direction*/
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= (z*sizes[1] + y)*sizes[0] + x-1;   
						if ((x-1) >= 0)
						{
							if(X_index[neighbor]==1.0)
							{
							        E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
						E_index[current] = 0.0;
					}
				}
			}
		}
		for (z = 0 ; z < sizes[2] ; z++)
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= (z*sizes[1] + y)*sizes[0] + x+1;   
					if ((x+1) < sizes[0])
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		}
	}
	else if (direction == 6)
	{
	        for (z = 0 ; z < sizes[2] ; z++) /*-x-direction*/
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					if (X_index[current] == 1.0)
					{
						neighbor= (z*sizes[1] + y)*sizes[0] + x+1;   
						if ((x+1) < sizes[0])
						{
							if(X_index[neighbor]==1.0)
							{
							        E_index[current] = 1.0;
							}
							else
							{
								E_index[current] = 0.0;
							}
						}
						else
						{
							E_index[current] = 0.0;
						}
					}
					else
					{
						E_index[current] = 0.0;
					}
				}
			}
		}
		for (z = 0 ; z < sizes[2] ; z++)
		{
			for (y = 0; y < sizes[1]; y++)
			{
				for (x = 0; x < sizes[0]; x++)
				{
					current = (z*sizes[1] + y)*sizes[0] + x;
					neighbor= (z*sizes[1] + y)*sizes[0] + x-1;   
					if ((x-1) >= 0)
					{
						E1_index[current] = E_index[neighbor];
					}
					else
					{
						E1_index[current] = 0.0;
					}
				}
			}
		}    
	}
	else
	{
	        display_message(ERROR_MESSAGE,
			"Computed_field_morphology_open.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;
	return (return_code);
}/* morphology_open XoB */

static int residual(FE_value *E_index, FE_value *E1_index, FE_value *E2_index, 
           FE_value *X_index, FE_value *G_index, int *sizes)
/*************************************************************************************************************
LAST MODIFIED: 28 October 2004
DESCRIPTION: residual = X\(XoB)
=============================================================================================================*/
{
        int x, y, z, current;
	int return_code;
	FE_value residual;
	ENTER(residual);
	return_code = 1;
	for (z = 0 ; z < sizes[2] ; z++)
	{
		for (y = 0; y < sizes[1]; y++)
		{
			for (x = 0; x < sizes[0]; x++)
			{
				current = (z*sizes[1] + y)*sizes[0] + x; 
				if (X_index[current] == 1.0 && my_Max(E_index[current],E1_index[current])== 0.0)
				{
					residual = 1.0;   
				}
				else
				{
					residual = 0.0;
				}
				E2_index[current] = my_Max(my_Max(E2_index[current],residual),G_index[current]);
				X_index[current] = my_Max(E_index[current],E2_index[current]);
			}
		}
	}
	LEAVE;
	return (return_code);
	
}/*residual*/

static int Image_cache_morphology_thinning(struct Image_cache *image, int iteration_times)
/*******************************************************************************
LAST MOErFIED : 27 October 2004

DESCRIPTION :
Perform a morphology thinning operation on the image cache in the basis of S = X\((X-B)+B).
==============================================================================*/
{
	char *storage;
	FE_value *data_index, *result_index, *a_index, *x_index, *e_index,*e1_index, *e2_index,*g_index;
	
	int i, k, return_code, storage_size;
	int n, stop, x, y, z, current, dir;
	/* int n1,n2;*/

	ENTER(Image_cache_morphology_thinning);
	if (image && (image->dimension > 0) && (image->depth > 0))
	{
		return_code = 1;
		/* We only need the one stencil as it is just a reordering for the other dimensions */
		/* Allocate a new storage block for our data */
		storage_size = image->depth;
		for (i = 0 ; i < image->dimension ; i++)
		{
			storage_size *= image->sizes[i];
		}
		if (ALLOCATE(a_index, FE_value, storage_size/image->depth) &&
			ALLOCATE(storage, char, storage_size * sizeof(FE_value)) &&
			ALLOCATE(x_index, FE_value, storage_size/image->depth) &&
			ALLOCATE(e_index, FE_value, storage_size/image->depth) &&
			ALLOCATE(e2_index, FE_value, storage_size/image->depth) &&
			ALLOCATE(g_index, FE_value, storage_size/image->depth) &&
			ALLOCATE(e1_index, FE_value, storage_size/image->depth))
		{
			result_index = (FE_value *)storage;
			for (i = 0 ; i < storage_size ; i++)
			{
				*result_index = 0.0;
				result_index++;
			}
			data_index = (FE_value *)image->data;
			for (i = 0; i < storage_size/image->depth; i++)
			{
				x_index[i] = *data_index;
				e_index[i] = 0.0;
				e1_index[i] = 0.0;
				a_index[i] = 0.0;
				data_index += image->depth;
			}
			result_index = (FE_value *)storage;
			for (n = 0; n < iteration_times; n++)
			{
			        
				stop = 0.0;
				for (i = 0 ; i < storage_size / image->depth ; i++)
				{
				        stop += x_index[i];
				}
				if (stop == 0.0)
				{
				        break;
				}
				else
				{
				        for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	e2_index[i] = 0.0;
			        	}
				        dir = 1; /* y - direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
				        for (z = 0 ; z < image->sizes[2] ; z++)/*determining gaps*/
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((y-1) >= 0)
								{
								        if ((z-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y-1,z-1)],(1.0-x_index[n(x,y,z-1)])));
										if ((x+1) < image->sizes[0])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z-1)],(1.0-x_index[n(x+1,y,z-1)])));
										}
										if ((x-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z-1)],(1.0-x_index[n(x-1,y,z-1)])));
										}
									}
									if ((z+1) < image->sizes[2])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y-1,z+1)],(1.0-x_index[n(x,y,z+1)])));
										if ((x+1) < image->sizes[0])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z+1)],(1.0-x_index[n(x+1,y,z+1)])));
										}
										if ((x-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z+1)],(1.0-x_index[n(x-1,y,z+1)])));
										}
									}
									if ((x-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z)],(1.0-x_index[n(x-1,y,z)])));
										
									}
									if ((x+1) < image->sizes[0])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z)],(1.0-x_index[n(x+1,y,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x,y-1,z)]));
								}
								
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					} /*determining gaps*/
					morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					
					return_code = residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
				        dir = 2; /* -z - direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
				        for (z = 0 ; z < image->sizes[2] ; z++)
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((z+1) < image->sizes[2])
								{
								        if ((x-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y,z+1)],(1.0-x_index[n(x-1,y,z)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z+1)],(1.0-x_index[n(x-1,y+1,z)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z+1)],(1.0-x_index[n(x-1,y-1,z)])));
										}
									}
									if ((x+1) < image->sizes[0])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y,z+1)],(1.0-x_index[n(x+1,y,z)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z+1)],(1.0-x_index[n(x+1,y+1,z)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z+1)],(1.0-x_index[n(x+1,y-1,z)])));
										}
									}
									if ((y-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y-1,z+1)],(1.0-x_index[n(x,y-1,z)])));
										
									}
									if ((y+1) < image->sizes[1])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y+1,z+1)],(1.0-x_index[n(x,y+1,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x,y,z+1)]));  
								}
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					}/*g_index, determining gaps*/
				        
					morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
				        
					dir = 3; /*-y-direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
					for (z = 0 ; z < image->sizes[2] ; z++)/*determining gaps*/
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((y+1) < image->sizes[1])
								{
								        if ((z-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y+1,z-1)],(1.0-x_index[n(x,y,z-1)])));
										if ((x+1) < image->sizes[0])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z-1)],(1.0-x_index[n(x+1,y,z-1)])));
										}
										if ((x-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z-1)],(1.0-x_index[n(x-1,y,z-1)])));
										}
									}
									if ((z+1) < image->sizes[2])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y+1,z+1)],(1.0-x_index[n(x,y,z+1)])));
										if ((x+1) < image->sizes[0])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z+1)],(1.0-x_index[n(x+1,y,z+1)])));
										}
										if ((x-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z+1)],(1.0-x_index[n(x-1,y,z+1)])));
										}
									}
									if ((x-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z)],(1.0-x_index[n(x-1,y,z)])));
										
									}
									if ((x+1) < image->sizes[0])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z)],(1.0-x_index[n(x+1,y,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x,y+1,z)]));
								}
								
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					}/*g_index*/
					morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
					
					dir = 4;/* z - direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
				        for (z = 0 ; z < image->sizes[2] ; z++)
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((z-1) >= 0)
								{
								        if ((x-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y,z-1)],(1.0-x_index[n(x-1,y,z)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z-1)],(1.0-x_index[n(x-1,y+1,z)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z-1)],(1.0-x_index[n(x-1,y-1,z)])));
										}
									}
									if ((x+1) < image->sizes[0])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y,z-1)],(1.0-x_index[n(x+1,y,z)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z-1)],(1.0-x_index[n(x+1,y+1,z)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z-1)],(1.0-x_index[n(x+1,y-1,z)])));
										}
									}
									if ((y-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y-1,z-1)],(1.0-x_index[n(x,y-1,z)])));
										
									}
									if ((y+1) < image->sizes[1])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x,y+1,z-1)],(1.0-x_index[n(x,y+1,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x,y,z-1)]));  
								}
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					}/*g_index, determining gaps*/
				        
					morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
				
				        dir = 5; /* -x - direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
					for (z = 0 ; z < image->sizes[2] ; z++)/*determining gaps*/
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((x+1) < image->sizes[0])
								{
								        if ((z-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y,z-1)],(1.0-x_index[n(x,y,z-1)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z-1)],(1.0-x_index[n(x,y+1,z-1)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z-1)],(1.0-x_index[n(x,y-1,z-1)])));
										}
									}
									if ((z+1) < image->sizes[2])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y,z+1)],(1.0-x_index[n(x,y,z+1)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z+1)],(1.0-x_index[n(x,y+1,z+1)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z+1)],(1.0-x_index[n(x,y-1,z+1)])));
										}
									}
									if ((y-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y-1,z)],(1.0-x_index[n(x,y-1,z)])));
										
									}
									if ((y+1) < image->sizes[1])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x+1,y+1,z)],(1.0-x_index[n(x,y+1,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x+1,y,z)]));
								}
								
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					}/*g_index*/
				        
				        morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
				 
					dir = 6;/* x - direction*/
					for (i = 0; i < storage_size/image->depth; i++)
			        	{
				        	g_index[i] = 0.0;
			        	}
					for (z = 0 ; z < image->sizes[2] ; z++)/*determining gaps*/
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								if ((x-1) >= 0)
								{
								        if ((z-1) >= 0)
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y,z-1)],(1.0-x_index[n(x,y,z-1)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z-1)],(1.0-x_index[n(x,y+1,z-1)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z-1)],(1.0-x_index[n(x,y-1,z-1)])));
										}
									}
									if ((z+1) < image->sizes[2])
									{
									        g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y,z+1)],(1.0-x_index[n(x,y,z+1)])));
										if ((y+1) < image->sizes[1])
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z+1)],(1.0-x_index[n(x,y+1,z+1)])));
										}
										if ((y-1) >= 0)
										{
										         g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z+1)],(1.0-x_index[n(x,y-1,z+1)])));
										}
									}
									if ((y-1) >= 0)
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y-1,z)],(1.0-x_index[n(x,y-1,z)])));
										
									}
									if ((y+1) < image->sizes[1])
									{
										g_index[current] = my_Max(g_index[current], my_Min(x_index[n(x-1,y+1,z)],(1.0-x_index[n(x,y+1,z)])));
										
									}
									g_index[current] = my_Min(g_index[current],(1.0-x_index[n(x-1,y,z)]));
								}
								
								g_index[current] = my_Min(g_index[current],x_index[current]);
								
							}
						}
					}/*g_index*/
				        
				        morphology_open(x_index,e_index, e1_index, image->sizes, dir);
					residual(e_index, e1_index, e2_index, x_index, g_index, image->sizes);
					
					for (z = 0 ; z < image->sizes[2] ; z++)
					{
					        for (y = 0; y < image->sizes[1]; y++)
						{
						        for (x = 0; x < image->sizes[0]; x++)
							{
							        current = (z*image->sizes[1] + y)*image->sizes[0] + x;
								/*e4_index[current] = my_Min((1.0-e4_index[current]),e3_index[current]);*/ 
								x_index[current] = my_Min(x_index[current],(1.0-e2_index[current]));
								a_index[current] = my_Max(a_index[current], e2_index[current]);
							}
						}
					}
				}
				
                        } /*loop n*/
			for (i = 0; i <storage_size / image->depth; i++)
			{
				
				for (k = 0; k < image->depth; k++)
				{
				         result_index[k] = a_index[i];
				}
				result_index += image->depth;
			}
			if (return_code)
			{
				DEALLOCATE(image->data);
				image->data = storage;
				image->valid = 1;
			}
			else
			{
				DEALLOCATE(storage);
			}
			DEALLOCATE(a_index);
			DEALLOCATE(x_index);
			DEALLOCATE(e_index);
			DEALLOCATE(e1_index);
			DEALLOCATE(g_index);
			DEALLOCATE(e2_index);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Image_cache_thinning_detection.  Not enough memory");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Image_cache_thinning_detection.  "
			"Invalid arguments.");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Image_cache_morphology_thinning */

static int Computed_field_morphology_thinning_evaluate_cache_at_node(
	struct Computed_field *field, struct FE_node *node, FE_value time)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Evaluate the fields cache at the node.
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_evaluate_cache_at_node);
	if (field && node &&
		(data = (struct Computed_field_morphology_thinning_type_specific_data *)field->type_specific_data))
	{
		return_code = 1;
		/* 1. Precalculate the Image_cache */
		if (!data->image->valid)
		{
			return_code = Image_cache_update_from_fields(data->image, field->source_fields[0],
				field->source_fields[1], data->element_dimension, data->region,
				data->graphics_buffer_package);
			/* 2. Perform image processing operation */
			return_code = Image_cache_morphology_thinning(data->image, data->iteration_times);
		}
		/* 3. Evaluate texture coordinates and copy image to field */
		Computed_field_evaluate_cache_at_node(field->source_fields[1],
			node, time);
		Image_cache_evaluate_field(data->image,field);

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_evaluate_cache_at_node.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_morphology_thinning_evaluate_cache_at_node */

static int Computed_field_morphology_thinning_evaluate_cache_in_element(
	struct Computed_field *field, struct FE_element *element, FE_value *xi,
	FE_value time, struct FE_element *top_level_element,int calculate_derivatives)
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Evaluate the fields cache at the node.
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_evaluate_cache_in_element);
	USE_PARAMETER(calculate_derivatives);
	if (field && element && xi && (field->number_of_source_fields > 0) &&
		(field->number_of_components == field->source_fields[0]->number_of_components) &&
		(data = (struct Computed_field_morphology_thinning_type_specific_data *) field->type_specific_data) &&
		data->image && (field->number_of_components == data->image->depth))
	{
		return_code = 1;
		/* 1. Precalculate the Image_cache */
		if (!data->image->valid)
		{
			return_code = Image_cache_update_from_fields(data->image, field->source_fields[0],
				field->source_fields[1], data->element_dimension, data->region,
				data->graphics_buffer_package);
			/* 2. Perform image processing operation */
			return_code = Image_cache_morphology_thinning(data->image, data->iteration_times);
		}
		/* 3. Evaluate texture coordinates and copy image to field */
		Computed_field_evaluate_cache_in_element(field->source_fields[1],
			element, xi, time, top_level_element, /*calculate_derivatives*/0);
		Image_cache_evaluate_field(data->image,field);
		
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_evaluate_cache_in_element.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_morphology_thinning_evaluate_cache_in_element */

#define Computed_field_morphology_thinning_evaluate_as_string_at_node \
	Computed_field_default_evaluate_as_string_at_node
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Print the values calculated in the cache.
==============================================================================*/

#define Computed_field_morphology_thinning_evaluate_as_string_in_element \
	Computed_field_default_evaluate_as_string_in_element
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Print the values calculated in the cache.
==============================================================================*/

#define Computed_field_morphology_thinning_set_values_at_node \
   (Computed_field_set_values_at_node_function)NULL
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Not implemented yet.
==============================================================================*/

#define Computed_field_morphology_thinning_set_values_in_element \
   (Computed_field_set_values_in_element_function)NULL
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Not implemented yet.
==============================================================================*/

#define Computed_field_morphology_thinning_get_native_discretization_in_element \
	Computed_field_default_get_native_discretization_in_element
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Inherit result from first source field.
==============================================================================*/

#define Computed_field_morphology_thinning_find_element_xi \
   (Computed_field_find_element_xi_function)NULL
/*******************************************************************************
LAST MODIFIED : 10 December 2003

DESCRIPTION :
Not implemented yet.
==============================================================================*/

static int list_Computed_field_morphology_thinning(
	struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 4 December 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(List_Computed_field_morphology_thinning);
	if (field && (field->type_string==computed_field_morphology_thinning_type_string)
		&& (data = (struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data))
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    texture coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    filter iteration_times : %d\n", data->iteration_times);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_morphology_thinning.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_morphology_thinning */

static char *Computed_field_morphology_thinning_get_command_string(
	struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 4 December 2003

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_morphology_thinning_get_command_string);
	command_string = (char *)NULL;
	if (field&& (field->type_string==computed_field_morphology_thinning_type_string)
		&& (data = (struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data) )
	{
		error = 0;
		append_string(&command_string,
			computed_field_morphology_thinning_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " texture_coordinate_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		sprintf(temp_string, " dimension %d", data->image->dimension);
		append_string(&command_string, temp_string, &error);

		sprintf(temp_string, " iteration_times %d", data->iteration_times);
		append_string(&command_string, temp_string, &error);

		sprintf(temp_string, " sizes %d %d",
		                    data->image->sizes[0],data->image->sizes[1]);
		append_string(&command_string, temp_string, &error);

		sprintf(temp_string, " minimums %f %f",
		                    data->image->minimums[0], data->image->minimums[1]);
		append_string(&command_string, temp_string, &error);

		sprintf(temp_string, " maximums %f %f",
		                    data->image->maximums[0], data->image->maximums[1]);
		append_string(&command_string, temp_string, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_morphology_thinning_get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_morphology_thinning_get_command_string */

#define Computed_field_morphology_thinning_has_multiple_times \
	Computed_field_default_has_multiple_times
/*******************************************************************************
LAST MODIFIED : 4 December 2003

DESCRIPTION :
Works out whether time influences the field.
==============================================================================*/

int Computed_field_set_type_morphology_thinning(struct Computed_field *field,
	struct Computed_field *source_field,
	struct Computed_field *texture_coordinate_field,
	int iteration_times,
	int dimension, int *sizes, FE_value *minimums, FE_value *maximums,
	int element_dimension, struct MANAGER(Computed_field) *computed_field_manager,
	struct Cmiss_region *region, struct Graphics_buffer_package *graphics_buffer_package)
/*******************************************************************************
LAST MODIFIED : 17 December 2003

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_morphology_thinning with the supplied
fields, <source_field> and <texture_coordinate_field>.  The <iteration_times> specifies
half the width and height of the filter window.  The <dimension> is the
size of the <sizes>, <minimums> and <maximums> vectors and should be less than
or equal to the number of components in the <texture_coordinate_field>.
If function fails, field is guaranteed to be unchanged from its original state,
although its cache may be lost.
==============================================================================*/
{
	int depth, number_of_source_fields, return_code;
	struct Computed_field **source_fields;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_set_type_morphology_thinning);
	if (field && source_field && texture_coordinate_field &&
		(iteration_times > 0) && (depth = source_field->number_of_components) &&
		(dimension <= texture_coordinate_field->number_of_components) &&
		region && graphics_buffer_package)
	{
		return_code=1;
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_source_fields=2;
		data = (struct Computed_field_morphology_thinning_type_specific_data *)NULL;
		if (ALLOCATE(source_fields, struct Computed_field *, number_of_source_fields) &&
			ALLOCATE(data, struct Computed_field_morphology_thinning_type_specific_data, 1) &&
			(data->image = ACCESS(Image_cache)(CREATE(Image_cache)())) &&
			Image_cache_update_dimension(
			data->image, dimension, depth, sizes, minimums, maximums) &&
			Image_cache_update_data_storage(data->image))
		{
			/* 2. free current type-specific data */
			Computed_field_clear_type(field);
			/* 3. establish the new type */
			field->type_string = computed_field_morphology_thinning_type_string;
			field->number_of_components = source_field->number_of_components;
			source_fields[0]=ACCESS(Computed_field)(source_field);
			source_fields[1]=ACCESS(Computed_field)(texture_coordinate_field);
			field->source_fields=source_fields;
			field->number_of_source_fields=number_of_source_fields;
			data->iteration_times = iteration_times;
			data->element_dimension = element_dimension;
			data->region = ACCESS(Cmiss_region)(region);
			data->graphics_buffer_package = graphics_buffer_package;
			data->computed_field_manager = computed_field_manager;
			data->computed_field_manager_callback_id =
				MANAGER_REGISTER(Computed_field)(
				Computed_field_morphology_thinning_field_change, (void *)field,
				computed_field_manager);

			field->type_specific_data = data;

			/* Set all the methods */
			COMPUTED_FIELD_ESTABLISH_METHODS(morphology_thinning);
		}
		else
		{
			DEALLOCATE(source_fields);
			if (data)
			{
				if (data->image)
				{
					DESTROY(Image_cache)(&data->image);
				}
				DEALLOCATE(data);
			}
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_set_type_morphology_thinning.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_set_type_morphology_thinning */

int Computed_field_get_type_morphology_thinning(struct Computed_field *field,
	struct Computed_field **source_field,
	struct Computed_field **texture_coordinate_field,
	int *iteration_times, int *dimension, int **sizes, FE_value **minimums,
	FE_value **maximums, int *element_dimension)
/*******************************************************************************
LAST MODIFIED : 17 December 2003

DESCRIPTION :
If the field is of type COMPUTED_FIELD_morphology_thinning, the
parameters defining it are returned.
==============================================================================*/
{
	int i, return_code;
	struct Computed_field_morphology_thinning_type_specific_data *data;

	ENTER(Computed_field_get_type_morphology_thinning);
	if (field && (field->type_string==computed_field_morphology_thinning_type_string)
		&& (data = (struct Computed_field_morphology_thinning_type_specific_data *)
		field->type_specific_data) && data->image)
	{
		*dimension = data->image->dimension;
		if (ALLOCATE(*sizes, int, *dimension)
			&& ALLOCATE(*minimums, FE_value, *dimension)
			&& ALLOCATE(*maximums, FE_value, *dimension))
		{
			*source_field = field->source_fields[0];
			*texture_coordinate_field = field->source_fields[1];
			*iteration_times = data->iteration_times;
			for (i = 0 ; i < *dimension ; i++)
			{
				(*sizes)[i] = data->image->sizes[i];
				(*minimums)[i] = data->image->minimums[i];
				(*maximums)[i] = data->image->maximums[i];
			}
			*element_dimension = data->element_dimension;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_get_type_morphology_thinning.  Unable to allocate vectors.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_morphology_thinning.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_morphology_thinning */

static int define_Computed_field_type_morphology_thinning(struct Parse_state *state,
	void *field_void, void *computed_field_morphology_thinning_package_void)
/*******************************************************************************
LAST MODIFIED : 4 December 2003

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_morphology_thinning (if it is not
already) and allows its contents to be modified.
==============================================================================*/
{
	char *current_token;
	FE_value *minimums, *maximums;
	int dimension, element_dimension, iteration_times, return_code, *sizes;
	struct Computed_field *field, *source_field, *texture_coordinate_field;
	struct Computed_field_morphology_thinning_package
		*computed_field_morphology_thinning_package;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data,
		set_texture_coordinate_field_data;

	ENTER(define_Computed_field_type_morphology_thinning);
	if (state&&(field=(struct Computed_field *)field_void)&&
		(computed_field_morphology_thinning_package=
		(struct Computed_field_morphology_thinning_package *)
		computed_field_morphology_thinning_package_void))
	{
		return_code=1;
		source_field = (struct Computed_field *)NULL;
		texture_coordinate_field = (struct Computed_field *)NULL;
		dimension = 0;
		sizes = (int *)NULL;
		minimums = (FE_value *)NULL;
		maximums = (FE_value *)NULL;
		element_dimension = 0;
		iteration_times = 1;
		/* field */
		set_source_field_data.computed_field_manager =
			computed_field_morphology_thinning_package->computed_field_manager;
		set_source_field_data.conditional_function =
			Computed_field_has_numerical_components;
		set_source_field_data.conditional_function_user_data = (void *)NULL;
		/* texture_coordinate_field */
		set_texture_coordinate_field_data.computed_field_manager =
			computed_field_morphology_thinning_package->computed_field_manager;
		set_texture_coordinate_field_data.conditional_function =
			Computed_field_has_numerical_components;
		set_texture_coordinate_field_data.conditional_function_user_data = (void *)NULL;

		if (computed_field_morphology_thinning_type_string ==
			Computed_field_get_type_string(field))
		{
			return_code = Computed_field_get_type_morphology_thinning(field,
				&source_field, &texture_coordinate_field, &iteration_times,
				&dimension, &sizes, &minimums, &maximums, &element_dimension);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			if (texture_coordinate_field)
			{
				ACCESS(Computed_field)(texture_coordinate_field);
			}

			if ((current_token=state->current_token) &&
				(!(strcmp(PARSER_HELP_STRING,current_token)&&
					strcmp(PARSER_RECURSIVE_HELP_STRING,current_token))))
			{
				option_table = CREATE(Option_table)();
				/* dimension */
				Option_table_add_int_positive_entry(option_table, "dimension",
					&dimension);
				/* element_dimension */
				Option_table_add_int_non_negative_entry(option_table, "element_dimension",
					&element_dimension);
				/* field */
				Option_table_add_Computed_field_conditional_entry(option_table,
					"field", &source_field, &set_source_field_data);
				/* maximums */
				Option_table_add_FE_value_vector_entry(option_table,
					"maximums", maximums, &dimension);
				/* minimums */
				Option_table_add_FE_value_vector_entry(option_table,
					"minimums", minimums, &dimension);
				/* iteration_times */
				Option_table_add_int_positive_entry(option_table,
					"iteration_times", &iteration_times);
				/* sizes */
				Option_table_add_int_vector_entry(option_table,
					"sizes", sizes, &dimension);
				/* texture_coordinate_field */
				Option_table_add_Computed_field_conditional_entry(option_table,
					"texture_coordinate_field", &texture_coordinate_field,
					&set_texture_coordinate_field_data);
				return_code=Option_table_multi_parse(option_table,state);
				DESTROY(Option_table)(&option_table);
			}
			/* parse the dimension ... */
			if (return_code && (current_token = state->current_token))
			{
				/* ... only if the "dimension" token is next */
				if (fuzzy_string_compare(current_token, "dimension"))
				{
					option_table = CREATE(Option_table)();
					/* dimension */
					Option_table_add_int_positive_entry(option_table, "dimension",
						&dimension);
					if (return_code = Option_table_parse(option_table, state))
					{
						if (!(REALLOCATE(sizes, sizes, int, dimension) &&
							REALLOCATE(minimums, minimums, FE_value, dimension) &&
							REALLOCATE(maximums, maximums, FE_value, dimension)))
						{
							return_code = 0;
						}
					}
					DESTROY(Option_table)(&option_table);
				}
			}
			if (return_code && (dimension < 1))
			{
				display_message(ERROR_MESSAGE,
					"define_Computed_field_type_scale.  Must specify a dimension first.");
				return_code = 0;
			}
			/* parse the rest of the table */
			if (return_code&&state->current_token)
			{
				option_table = CREATE(Option_table)();
				/* element_dimension */
				Option_table_add_int_non_negative_entry(option_table, "element_dimension",
					&element_dimension);
				/* field */
				Option_table_add_Computed_field_conditional_entry(option_table,
					"field", &source_field, &set_source_field_data);
				/* maximums */
				Option_table_add_FE_value_vector_entry(option_table,
					"maximums", maximums, &dimension);
				/* minimums */
				Option_table_add_FE_value_vector_entry(option_table,
					"minimums", minimums, &dimension);
				/* iteration_times */
				Option_table_add_int_positive_entry(option_table,
					"iteration_times", &iteration_times);
				/* sizes */
				Option_table_add_int_vector_entry(option_table,
					"sizes", sizes, &dimension);
				/* texture_coordinate_field */
				Option_table_add_Computed_field_conditional_entry(option_table,
					"texture_coordinate_field", &texture_coordinate_field,
					&set_texture_coordinate_field_data);
				return_code=Option_table_multi_parse(option_table,state);
				DESTROY(Option_table)(&option_table);
			}
			/* no errors,not asking for help */
			if (return_code)
			{
				return_code = Computed_field_set_type_morphology_thinning(field,
					source_field, texture_coordinate_field, iteration_times, dimension,
					sizes, minimums, maximums, element_dimension,
					computed_field_morphology_thinning_package->computed_field_manager,
					computed_field_morphology_thinning_package->root_region,
					computed_field_morphology_thinning_package->graphics_buffer_package);
			}
			if (!return_code)
			{
				if ((!state->current_token)||
					(strcmp(PARSER_HELP_STRING,state->current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_morphology_thinning.  Failed");
				}
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
			if (texture_coordinate_field)
			{
				DEACCESS(Computed_field)(&texture_coordinate_field);
			}
			if (sizes)
			{
				DEALLOCATE(sizes);
			}
			if (minimums)
			{
				DEALLOCATE(minimums);
			}
			if (maximums)
			{
				DEALLOCATE(maximums);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_morphology_thinning.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_morphology_thinning */

int Computed_field_register_types_morphology_thinning(
	struct Computed_field_package *computed_field_package,
	struct Cmiss_region *root_region, struct Graphics_buffer_package *graphics_buffer_package)
/*******************************************************************************
LAST MODIFIED : 12 December 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	static struct Computed_field_morphology_thinning_package
		computed_field_morphology_thinning_package;

	ENTER(Computed_field_register_types_morphology_thinning);
	if (computed_field_package)
	{
		computed_field_morphology_thinning_package.computed_field_manager =
			Computed_field_package_get_computed_field_manager(
				computed_field_package);
		computed_field_morphology_thinning_package.root_region = root_region;
		computed_field_morphology_thinning_package.graphics_buffer_package = graphics_buffer_package;
		return_code = Computed_field_package_add_type(computed_field_package,
			            computed_field_morphology_thinning_type_string,
			            define_Computed_field_type_morphology_thinning,
			            &computed_field_morphology_thinning_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_morphology_thinning.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_morphology_thinning */

