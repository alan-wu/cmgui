/*******************************************************************************
FILE : finite_element.h

LAST MODIFIED : 21 December 1999

DESCRIPTION :
The data structures used for representing finite elements in the graphical
interface to CMISS.
???DB.  Get rid of the find_FE_ and replace with FIRST_OBJECT_IN_ ?
???DB.  Hide most of this ?
==============================================================================*/
#if !defined (FINITE_ELEMENT_H)
#define FINITE_ELEMENT_H

#include "command/parser.h"
#include "general/geometry.h"
#include "general/managed_group.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "general/value.h"

/*
Global types
------------
*/

enum FE_nodal_value_type
/*******************************************************************************
LAST MODIFIED : 27 January 1998

DESCRIPTION :
The type of a nodal value.
==============================================================================*/
{
	FE_NODAL_VALUE,
	FE_NODAL_D_DS1,
	FE_NODAL_D_DS2,
	FE_NODAL_D_DS3,
	FE_NODAL_D2_DS1DS2,
	FE_NODAL_D2_DS1DS3,
	FE_NODAL_D2_DS2DS3,
	FE_NODAL_D3_DS1DS2DS3,
	FE_NODAL_UNKNOWN
}; /* enum FE_nodal_value_type */

struct FE_node;

DECLARE_LIST_TYPES(FE_node);

DECLARE_MANAGER_TYPES(FE_node);

DECLARE_MANAGED_GROUP_TYPES(FE_node);

enum FE_basis_type
/*******************************************************************************
LAST MODIFIED : 20 October 1997

DESCRIPTION :
The different basis types available.
NOTE: Must keep this up to date with functions
FE_basis_type_string
==============================================================================*/
{
	NO_RELATION=0,
		/*???DB.  Used on the off-diagonals of the type matrix */
	BSPLINE,
	CUBIC_HERMITE,
	CUBIC_LAGRANGE,
	FOURIER,
	HERMITE_LAGRANGE,
	LAGRANGE_HERMITE,
	LINEAR_LAGRANGE,
	LINEAR_SIMPLEX,
	POLYGON,
	QUADRATIC_LAGRANGE,
	QUADRATIC_SIMPLEX,
	SERENDIPITY,
	SINGULAR,
	TRANSITION
}; /* enum FE_basis_type */

enum FE_field_type
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
==============================================================================*/
{
	CONSTANT_FE_FIELD, /* fixed values */
	INDEXED_FE_FIELD,  /* indexed set of fixed values */
	GENERAL_FE_FIELD,  /* values held in nodes, elements */
	UNKNOWN_FE_FIELD
}; /* enum FE_field_type */

typedef int (Standard_basis_function)(void *,FE_value *,FE_value *);

struct FE_basis
/*******************************************************************************
LAST MODIFIED : 1 October 1995

DESCRIPTION :
Stores the information for calculating basis function values from xi
coordinates.  For each of basis there will be only one copy stored in a global
list.
==============================================================================*/
{
	/* an integer array that specifies the basis as a "product" of the bases for
		the different coordinates.  The first entry is the number_of_xi_coordinates.
		The other entries are the upper triangle of a <number_of_xi_coordinates> by
		<number_of_xi_coordinates> matrix.  The entries on the diagonal specify the
		basis type for each coordinate and entries in the same row/column indicate
		associated coordinates eg.
		1.  CUBIC_HERMITE       0              0
											CUBIC_HERMITE        0
																		LINEAR_LAGRANGE
			has cubic variation in xi1 and xi2 and linear variation in xi3 (8 nodes).
		2.  SERENDIPITY       0            1
										CUBIC_HERMITE      0
																	SERENDIPITY
			has cubic variation in xi2 and 2-D serendipity variation for xi1 and
			xi3 (16 nodes).
		3.  CUBIC_HERMITE        0               0
											LINEAR_LAGRANGE        1
																			LINEAR_LAGRANGE
			has cubic variation in xi1 and 2-D linear simplex variation for xi2 and
			xi3 (6 nodes)
???RC Shouldn't the above be LINEAR_SIMPLEX, not LINEAR_LAGRANGE?
		4.  POLYGON        0           5
								LINEAR_LAGRANGE    0
																POLYGON
			has linear variation in xi2 and a 2-D 5-gon for xi1 and xi3 (12 nodes,
			5-gon has 5 peripheral nodes and a centre node) */
	int *type;
	/* the number of basis functions */
	int number_of_basis_functions;
	/* the names for the basis values eg. "0", "dXi1 1,1" */
	char **value_names;
	/* the blending matrix is a linear mapping from the basis used (eg. cubic
		Hermite) to the standard basis (eg. Chebyshev polynomials).  In some cases,
		for example a non-polynomial basis, this may be NULL, which indicates the
		identity matrix */
	FE_value *blending_matrix;
	/* to calculate the values of the "standard" basis functions */
	int number_of_standard_basis_functions;
	void *arguments;
	Standard_basis_function *standard_basis;
	/* the number of structures that point to this basis.  The basis cannot be
		destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_basis */

DECLARE_LIST_TYPES(FE_basis);

DECLARE_MANAGER_TYPES(FE_basis);

enum Global_to_element_map_type
/*******************************************************************************
LAST MODIFIED : 22 September 1998

DESCRIPTION :
Used for specifying the type of a global to element map.
==============================================================================*/
{
	STANDARD_NODE_TO_ELEMENT_MAP,
	GENERAL_NODE_TO_ELEMENT_MAP,
	FIELD_TO_ELEMENT_MAP,
	ELEMENT_GRID_MAP
}; /* enum Global_to_element_map_type */

struct Linear_combination_of_global_values
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Stores the information for calculating an element value as a linear combination
of global values.  The application of scale factors is one of the uses for this
linear combination.
==============================================================================*/
{
	/* the number of global values */
	int number_of_global_values;
	/* array of indices for the global values (stored with the field) */
	/*???RC I think the above comment should say the following is an array of
		indices into the nodal values, within the list of all values at the node.
		In any case the index is an offset in whole FE_values relative to the
		absolute value offset in the FE_node_field_component for the associated
		node (which is a values storage/unsigned char).
		See: global_to_element_map_values() */
	int *global_value_indices;
	/* array of indices for the coefficients multiplying the global values
		(stored with the element as scale factors) */
	int *coefficient_indices;
}; /* struct Linear_combination_of_global_values */

struct Standard_node_to_element_map
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Stores the information for calculating element values by choosing nodal values
and applying a diagonal scale factor matrix.  The <nodal_values> and
<scale_factors> are stored as offsets so that the arrays stored with the nodes
and elements can be reallocated.
==============================================================================*/
{
	/* the index, within the list of nodes for the element, of the node at which
		the values are stored */
	int node_index;
	/* the number of nodal values used (which is also the number of element values
		calculated) */
	int number_of_nodal_values;
	/* array of indices for the nodal values, within the list of all values at
		the node - index is an offset in whole FE_values relative to the absolute
		value offset in the FE_node_field_component for the associated node (which
		is a values storage/unsigned char). See: global_to_element_map_values() */
	int *nodal_value_indices;
	/* array of indices for the scale factors */
	int *scale_factor_indices;
}; /* struct Standard_node_to_element_map */

struct General_node_to_element_map
/*******************************************************************************
LAST MODIFIED : 23 December 1993

DESCRIPTION :
Stores the information for calculating element values by choosing nodal values
and applying a general scale factor matrix.  The <nodal_values> and
<scale_factors> are stored as offsets so that the arrays stored with the nodes
and elements can be reallocated.
==============================================================================*/
{
	/* the index, within the list of nodes for the element, of the node at which
		the values are stored */
	int node_index;
	/* the number of nodal values used (which is also the number of element values
		calculated) */
	int number_of_nodal_values;
	/* how to calculate each element values from the nodal values */
	struct Linear_combination_of_global_values **element_values;
}; /* struct General_node_to_element_map */

/* forward declarations for the function pointer prototype (which, since used in
	the structures below, must be declared beforehand) */
struct FE_element_field_component;
struct FE_element;
struct FE_field;

typedef int (*FE_element_field_component_modify)(
	struct FE_element_field_component *,struct FE_element *,struct FE_field *,
	int,FE_value *);

struct FE_element_field_component
/*******************************************************************************
LAST MODIFIED : 22 September 1998

DESCRIPTION :
Stores the information for calculating element values, with respect to the
<basis>, from global values (this calculation includes the application of scale
factors).  There are two types - <NODE_BASED_MAP> and <GENERAL_LINEAR_MAP>.  For
a node based map, the global values are associated with nodes.  For a general
linear map, the global values do not have to be associated with nodes.  The node
based maps could be specified as general linear maps, but the node based
specification (required by CMISS) cannot be recovered from the general linear
map specification (important when the front end is being used to create meshs).
The <modify> function is called after the element values have been calculated
with respect to the <basis> and before the element values are blended to be with
respect to the standard basis.  The <modify> function is to allow for special
cases, such as CMISS nodes that have multiple theta values in cylindrical polar,
spherical polar, prolate spheroidal or oblate spheroidal coordinate systems -
either lying on the z-axis or being the first and last node in a circle.
==============================================================================*/
{
	/* the type of the global to element map */
	enum Global_to_element_map_type type;
	union
	{
		/* for a standard node based map */
		struct
		{
			/* the number of nodes */
			int number_of_nodes;
			/* how to get the element values from the nodal values */
			struct Standard_node_to_element_map **node_to_element_maps;
		} standard_node_based;
		/* for a general node based map */
		struct
		{
			/* the number of nodes */
			int number_of_nodes;
			/* how to get the element values from the nodal values */
			struct General_node_to_element_map **node_to_element_maps;
		} general_node_based;
		/* for a field map */
		struct
		{
			/* the number of element values */
			int number_of_element_values;
			/* how to calculate each element value from the global values */
			struct Linear_combination_of_global_values **element_values;
		} field_based;
		/* for an element grid */
		struct
		{
			/* the element is covered by a regular grid with <number_in_xi>
				sub-elements in each direction */
			int *number_in_xi;
			/* the component is linear over each sub-element and the grid point values
				are stored with the element starting at <value_index> */
				/*???DB.  Other bases for the sub-elements are possible, but are not
					currently used and would add to the complexity/compute time */
			int value_index;
		} element_grid_based;
	} map;
	/* the basis that the element values are with respect to */
	struct FE_basis *basis;
	/* the function for modifying element values */
	FE_element_field_component_modify modify;
}; /* struct FE_element_field_component */

struct FE_element_field
/*******************************************************************************
LAST MODIFIED : 22 December 1993

DESCRIPTION :
Stores the information for calculating the value of a field at a point within a
element.  The position of the point should be specified by Xi coordinates of the
point within the element.
==============================================================================*/
{
	/* the field which this is part of */
	struct FE_field *field;
	/* an array with <field->number_of_components> pointers to element field
		components */
	struct FE_element_field_component **components;
	/* the number of structures that point to this element field.  The element
		field cannot be destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_element_field */

DECLARE_LIST_TYPES(FE_element_field);

struct FE_element_field_values
/*******************************************************************************
LAST MODIFIED : 2 October 1998

DESCRIPTION :
The values need to calculate a field on an element.  These structures are
calculated from the element field as required and are then destroyed.
==============================================================================*/
{
	/* the field these values are for */
	struct FE_field *field;
	/* the element these values are for */
	struct FE_element *element;
	/* the element the field was inherited from */
	struct FE_element *field_element;
	/* number of sub-elements in each xi-direction of element. If NULL then field
		 is not	grid based.  Notes
		1.  struct FE_element_field allows some components to be grid-based and
			some not with different discretisations for the grid-based components.
			This structure only supports - all grid-based components with the same
			discretisation, or no grid-based components.  This restriction could be
			removed by having a <number_in_xi> for each component
		2.  the sub-elements are linear in each direction.  This means that
			<component_number_of_values> is not used
		3.  the grid-point values are not blended (to monomial) and so
			<component_standard_basis_functions> and
			<component_standard_basis_function_arguments> are not used
		4.  for grid-based <destroy_standard_basis_arguments> is used to specify
			if the <component_values> should be destroyed (element field has been
			inherited) */
	int *number_in_xi;
	/* a flag to specify whether or not values have also been calculated for the
		derivatives of the field with respect to the xi coordinates */
	char derivatives_calculated;
	/* specify whether the standard basis arguments should be destroyed (element
		field has been inherited) or not be destroyed (element field is defined for
		the element and the basis arguments are being used) */
	char destroy_standard_basis_arguments;
	/* the number of field components */
	int number_of_components;
	/* the number of values for each component */
	int *component_number_of_values;
	/* the values_storage for each component if grid-based */
	Value_storage **component_grid_values_storage;
	/* grid_offset_in_xi is allocated with 2^number_of_xi_coordinates integers
		 giving the increment in index into the values stored with the top_level
		 element for the grid. For top_level_elements the first value is 1, the
		 second is (number_in_xi[0]+1), the third is
		 (number_in_xi[0]+1)*(number_in_xi[1]+1) etc. The base_grid_offset is 0 for
		 top_level_elements. For faces and lines these values are adjusted to get
		 the correct index for the top_level_element */
	int base_grid_offset,*grid_offset_in_xi;
	/* following allocated with 2^number_of_xi for grid-based fields for use in
		 calculate_FE_element_field */
	int *element_value_offsets;
	/* the values for each component */
	FE_value **component_values;
	/* the standard basis function for each component */
	Standard_basis_function **component_standard_basis_functions;
	/* the arguments for the standard basis function for each component */
	void *component_standard_basis_function_arguments;
	/* working space for evaluating basis */
	FE_value *basis_function_values;
}; /* struct FE_element_field_values */

struct FE_element_field_info
/*******************************************************************************
LAST MODIFIED : 24 September 1995

DESCRIPTION :
The element fields defined on an element and how to calculate them.
==============================================================================*/
{

	/* list of the  element fields */
	struct LIST(FE_element_field) *element_field_list;
	/* the number of structures that point to this information.  The information
		cannot be destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_element_field_info */

DECLARE_LIST_TYPES(FE_element_field_info);

struct FE_element_node_scale_field_info
/*******************************************************************************
LAST MODIFIED : 29 September 1999

DESCRIPTION :
The node, scale factor and field and field information.  This information can be
inherited from the element's parents.  The nodes and scale factors are specific
to the element, but the field information can be shared between elements.
==============================================================================*/
{
	/* values_storage.  Element based maps have indices into this array */
	int values_storage_size;
	Value_storage *values_storage;

	/* nodes.  Node to element maps have indices into this array */
	int number_of_nodes;
	struct FE_node **nodes;
	/* there may be a number of sets of scale factors (CMISS has one for each
		basis) */
	int number_of_scale_factor_sets;
	/*???RC following void * pointers generally point at FE_basis. However, if it
		were used for anything else it would stuff up export_finite_element, since
		then we'd have no idea what to output. Move to more involved structure
		instead? */
	void **scale_factor_set_identifiers;
	int *numbers_in_scale_factor_sets;
	/* all scale factors are stored in this array.  Global to element maps have
		indices into this array */
	int number_of_scale_factors;
	FE_value *scale_factors;
	/* the field information for the element */
	struct FE_element_field_info *fields;
}; /* struct FE_element_node_scale_field_info */

enum CM_element_type
/*******************************************************************************
LAST MODIFIED : 25 January 1999

DESCRIPTION :
CM element types.
==============================================================================*/
{
  CM_ELEMENT_TYPE_INVALID,
  CM_ELEMENT,
  CM_FACE,
  CM_LINE
}; /* enum CM_element_type */

struct CM_element_information
/*******************************************************************************
LAST MODIFIED : 25 January 1999

DESCRIPTION :
Element information needed by CM.
==============================================================================*/
{
  enum CM_element_type type;
  int number;
}; /* struct CM_element_information */

enum FE_element_shape_type
/*******************************************************************************
LAST MODIFIED : 27 October 1997

DESCRIPTION :
The different basis types available.
==============================================================================*/
{
	LINE_SHAPE,
	POLYGON_SHAPE,
	SIMPLEX_SHAPE
}; /* enum FE_element_shape_type */

struct FE_element_shape
/*******************************************************************************
LAST MODIFIED : 30 October 1997

DESCRIPTION :
A description of the shape of an element in Xi space.  It includes how to
calculate face coordinates from element coordinates and how to calculate element
coordinates from face coordinates.
==============================================================================*/
{
	/* the number of xi coordinates */
	int dimension;
	/* the structure description.  Similar to type for a FE_basis
		- doesn't have the dimension in the first position (type[0])
		- the diagonal entry is the shape type
		- non-zero off-diagonal entries indicate that the dimensions are linked.
			For a polygon, it is the number of vertices
		eg. a 5-gon in dimensions 1 and 2 and linear in the third dimension
			POLYGON_SHAPE 5             0
										POLYGON_SHAPE 0
																	LINE_SHAPE
		eg. tetrahedron
			SIMPLEX_SHAPE 1             1
										SIMPLEX_SHAPE 1
																	SIMPLEX_SHAPE */
	int *type;
	/* the number of faces */
	int number_of_faces;
	/* the equations for the faces of the element.  This is a linear system
		b = A xi, where A is <number_of_faces> by <dimension> matrix whose entries
		are either 0 or 1 and b is a vector whose entries are
		0,...number_of_faces_in_this_dimension-1.  For a cube the system would be
			0   1 0 0  xi1       2
			1   1 0 0  xi2       3
			0 = 0 1 0  xi3       4
			1   0 1 0            5
			0   0 0 1            8
			1   0 0 1            9
		For a 5-gon by linear above the system would be
			0   1 0 0  xi1       5
			1   1 0 0  xi2       6
			2   1 0 0  xi3       7
			3 = 1 0 0            8
			4   1 0 0            9
			0   0 0 1           10
			1   0 0 1           11
			The "equations" for the polygon faces, don't actually describe the faces,
				but are in 1 to 1 correspondence - first represents 0<=xi1<=1/5 and
				xi2=1.
		For a tetrahedron the system would be
			0   1 0 0  xi1       2
			0 = 0 1 0  xi2       4
			0   0 0 1  xi3       8
			1   1 1 1           15
		A unique number is calculated for each number as follows
		- a value is calculated for each column by multiplying the number for the
			previous column (start with 1, left to right) by
			- the number_of_vertices for the first polygon column
			- 1 for the second polygon column
			- 2 otherwise
		- a value for each row by doing the dot product of the row and the column
			values
		- the entry of b for that row is added to the row value to give the unique
			number
		These numbers are stored in <faces>.
		The values for the above examples are given on the right */
	int *faces;
	/* for each face an affine transformation, b + A xi for calculating the
		element xi coordinates from the face xi coordinates.  For each face there is
		a <number_of_xi_coordinates> by <number_of_xi_coordinates> array with the
		translation b in the first column.  For a cube the translations could be
		(not unique)
		face 2 :  0 0 0 ,  face 3 : 1 0 0 ,  face 4 : 0 0 1 , 1(face) goes to 3 and
							0 1 0             0 1 0             0 0 0   2(face) goes to 1 to
							0 0 1             0 0 1             0 1 0   maintain right-
																													handedness (3x1=2)
		face 5 :  0 0 1 ,  face 8 : 0 1 0 ,  face 9 : 0 1 0
							1 0 0             0 0 1             0 0 1
							0 1 0             0 0 0             1 0 0
		For the 5-gon by linear the faces would be
		face 5 :  0 1/5 0 ,  face 6 : 1/5 1/5 0 ,  face 7 : 2/5 1/5 0
							1 0   0             1   0   0             1   0   0
							0 0   1             0   0   1             0   0   1
		face 8 :  3/5 1/5 0 ,  face 9 : 4/5 1/5 0
							1   0   0             1   0   0
							0   0   1             0   0   1
		face 10 : 0 1 0 ,  face 11 : 0 1 0
							0 0 1              0 0 1
							0 0 0              1 0 0
		For the tetrahedron the faces would be
		face 2 :  0 0 0 ,  face 4 : 1 -1 -1 ,  face 8 : 0  0  1
							0 1 0             0  0  0             1 -1 -1
							0 0 1             0  1  0             0  0  0
		face 15 : 0  1  0
							0  0  1
							1 -1 -1 */
	FE_value *face_to_element;
	/* the number of structures that point to this shape.  The shape cannot be
		destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_element_shape */

#if defined (OLD_CODE)
/*???DB.  Some ideas, but best to keep it simple at present */
	/* the structure description.  Similar to type for a FE_basis, but doesn't
		have the dimension in the first position (type[0]).  There are two versions,
		the "external" which is passed to CREATE(FE_element_shape) and the
		"internal" which is calculated by CREATE(FE_element_shape) and stored here.
		external
		- the diagonal entry is the shape type
		- non-zero off-diagonal entries indicate that the dimensions are linked.
			For a polygon, it is the number of vertices
		internal
		- diagonal entries
			- zero.  Then linear in this dimension
			- negative.  Then the last in a sub-shape of linked dimensions.  This is
				the offset to the previous linked dimension
			- positive.  Then not the last in a sub-shape of linked dimensions.  This
				is the offset to the next linked dimension
		- off-diagonal entries
			- zero.  Then the dimensions are not linked
			- negative.  Then the dimensions are linked and there is another linked
				dimension before the row dimension.  This is the offset to the previous
				linked dimension
			- positive.  Then the dimensions are linked and the row dimension is the
				first dimension in the linked sub-shape.
				- 1 simplex
				- 2 reserved for future developments
				- >=3 polygon.  This is the number of vertices
				is the offset to the next linked dimension
		eg. linear in xi1 and a 5-gon in xi2 and xi3
			external
				LINE_SHAPE  0             0
										POLYGON_SHAPE 5
																	POLYGON_SHAPE
			internal
				0 0  0
					1  5
						-1
		eg. tetrahedron
			external
			SIMPLEX_SHAPE 1             1
										SIMPLEX_SHAPE 1
																	SIMPLEX_SHAPE
			internal
				1 1  1
					1 -1
						-1 */
#endif /* defined (OLD_CODE) */

DECLARE_LIST_TYPES(FE_element_shape);

struct FE_element_parent
/*******************************************************************************
LAST MODIFIED : 8 January 1994

DESCRIPTION :
Information for going from an element to a parent element.
==============================================================================*/
{
	/* access_FE_element and deaccess_FE_element are NOT used for this so that
		an element can be destroyed when it has children */
	struct FE_element *parent;
	int face_number;
	/* the number of structures that point to this element parent.  The element
		parent cannot be destroyed while this is greater than 0.  Needed so that can
		have lists */
	int access_count;
}; /* struct FE_element_parent */

DECLARE_LIST_TYPES(FE_element_parent);

struct FE_element
/*******************************************************************************
LAST MODIFIED : 28 September 1998

DESCRIPTION :
A region in space with functions defined on the region.  The region is
parametrized and the functions are known in terms of the parametrized
variables.
???DB.  Changing between storing elements, faces and lines together and
	separately under different managers
==============================================================================*/
{
	/* identifier points to <cm> and is used for the <find_by_identifier>
		function */
	struct CM_element_information cm,*identifier;

	/* a description of the shape/geometry of the element in Xi space */
	struct FE_element_shape *shape;
	/* the faces (finite elements of dimension 1 less) of the element.  The number
		of faces is known from the <shape> */
	struct FE_element **faces;
	/* the parent elements are of dimension 1 greater and are the elements for
		which this element is a face */
	struct LIST(FE_element_parent) *parent_list;
	/* the nodes, scale factors and fields for the element.  This can be NULL
		except for elements without parents.  Elements with parents can inherit
		field information from their parents */
	struct FE_element_node_scale_field_info *information;
	/* the number of structures that point to this element.  The element cannot be
		destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_element */

DECLARE_LIST_TYPES(FE_element);

DECLARE_MANAGER_TYPES(FE_element);

DECLARE_MANAGED_GROUP_TYPES(FE_element);

#if defined (OLD_CODE)
/* switching to MANAGED_GROUP which automatically defines indexed list and
	manager for groups */
DECLARE_GROUP_TYPES(FE_element);

DECLARE_LIST_TYPES(GROUP(FE_element));

DECLARE_MANAGER_TYPES(GROUP(FE_element));
#endif /* defined (OLD_CODE) */

struct Add_FE_element_and_faces_to_manager_data;
/*******************************************************************************
LAST MODIFIED : 29 April 1999

DESCRIPTION :
Private structure set up by CREATE(Add_FE_element_and_faces_to_manager_data)
and cleaned up by DESTROY(Add_FE_element_and_faces_to_manager_data). For
passing to add_FE_element_and_faces_to_manager.
==============================================================================*/

enum CM_field_type
/*******************************************************************************
LAST MODIFIED : 2 February 1999

DESCRIPTION :
CMISS field types.  Values specified to correspond to CMISS
==============================================================================*/
{
	CM_ANATOMICAL_FIELD=1,
	CM_COORDINATE_FIELD=2,
	CM_FIELD=3,
	CM_DEPENDENT_FIELD=4,
	CM_UNKNOWN_FIELD
}; /* enum CM_field_type */

struct CM_field_information
/*******************************************************************************
LAST MODIFIED : 2 February 1999

DESCRIPTION :
Field information needed by CM.
???DB.  What about grids (element based fields) ?
???DB.  Should this be hidden in the name as at present ?  This is how it would
  have to be done for any other application communicating with cmgui.  User
  data ?
==============================================================================*/
{
  enum CM_field_type type;
  union
  {
		struct
    {
      int nc,niy,nr,nxc,nxt;
    } dependent;
		struct
    {
      int nr;
    } independent;
  } indices;
}; /* struct CM_field_information */

DECLARE_LIST_TYPES(FE_field);

DECLARE_MANAGER_TYPES(FE_field);

struct FE_field_component
/*******************************************************************************
LAST MODIFIED : 27 October 1995

DESCRIPTION :
Used to specify a component of a field.  If the component <number> is < 0 or
>= the number of components, it specifies all components.
==============================================================================*/
{
	struct FE_field *field;
	int number;
}; /* struct FE_field_component */

typedef int (FE_node_field_iterator_function)(struct FE_node *node, \
	struct FE_field *field,void *user_data);

typedef int (FE_element_field_iterator_function)(struct FE_element *element, \
	struct FE_field *field,void *user_data);

struct FE_element_parent_face_of_element_in_group_data
/*******************************************************************************
LAST MODIFIED : 15 February 1999

DESCRIPTION :
Used by FE_element_parent_face_of_element_in_group.
==============================================================================*/
{
	int face_number;
	struct GROUP(FE_element) *element_group;
}; /* struct FE_element_parent_face_of_element_in_group_data */

struct FE_node_field_has_embedded_element_data
/*******************************************************************************
LAST MODIFIED : 28 April 1999

DESCRIPTION :
Keeps the current <node> and the <changed_element> or <changed_node>.
==============================================================================*/
{
	struct FE_node *node;
	struct FE_element *changed_element;
	struct FE_node *changed_node;
}; /* struct FE_node_field_has_embedded_element_data */

struct Set_FE_field_conditional_data
/*******************************************************************************
LAST MODIFIED : 4 May 1999

DESCRIPTION :
User data structure passed to set_FE_field_conditional, containing the
FE_field_manager and the optional conditional_function (and
conditional_function_user_data) for selecting a field out of a subset of the
fields in the manager.
==============================================================================*/
{
	MANAGER_CONDITIONAL_FUNCTION(FE_field) *conditional_function;
	void *conditional_function_user_data;
	struct MANAGER(FE_field) *fe_field_manager;
}; /* struct Set_FE_field_conditional_data */

struct Smooth_field_over_node_data
/*******************************************************************************
LAST MODIFIED : 16 August 1998

DESCRIPTION :
Used by smooth_field_over_node.
==============================================================================*/
{
	int number_of_elements;
	struct FE_field_component field_component;
	struct MANAGER(FE_node) *node_manager;
}; /* struct Smooth_field_over_element_data */

struct Smooth_field_over_element_data
/*******************************************************************************
LAST MODIFIED : 16 August 1998

DESCRIPTION :
Used by add_node_to_smoothed_node_lists and smooth_field_over_element.
==============================================================================*/
{
	float smoothing;
	int maximum_number_of_elements_per_node;
	struct FE_field *field;
	struct LIST(FE_node) **node_lists;
	struct MANAGER(FE_element) *element_manager;
	struct MANAGER(FE_node) *node_manager;
}; /* struct Smooth_field_over_element_data */

struct FE_field_order_info
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
Use to store the order fields are read in from the input files - for elements
and nodes.
==============================================================================*/
{
	int number_of_fields,access_count;
	struct FE_field **fields;
}; /* FE_field_order_info */ 

struct FE_node_order_info
/*******************************************************************************
LAST MODIFIED : 11 August  1999

DESCRIPTION :
Use to pass info about a node group's nodes, their number, and their order.
c.f.  FE_field_order_info
==============================================================================*/
{
	int number_of_nodes;
	struct FE_node **nodes;
	int access_count;
}; /* FE_node_order_info */ 

/*
Global variables
----------------
*/
extern struct LIST(FE_element_field_info) *all_FE_element_field_info;
#if defined (OLD_CODE)
extern struct LIST(FE_element) *all_FE_element;
#endif /* defined (OLD_CODE) */
extern struct LIST(FE_element_shape) *all_FE_element_shape;
#if defined (OLD_CODE)
extern struct LIST(FE_field) *all_FE_field;
#endif /* defined (OLD_CODE) */

/*
Global functions
----------------
*/
#if defined (DEBUG)
int show_FE_nodal_FE_values(struct FE_node *node); 
#endif

struct FE_node *CREATE(FE_node)(int cm_node_identifier,
	struct FE_node *template_node);
/*******************************************************************************
LAST MODIFIED : 7 October 1999

DESCRIPTION :
Creates and returns a node with the specified <cm_node_identifier>. The node
is given the same fields and values_storage as <template_node>, or no fields if
no template is specified.
???DB.  Why is there a >=0 restriction on the <cm_node_identifier> ?
==============================================================================*/

int DESTROY(FE_node)(struct FE_node **node_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the node, sets <*node_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_node);
PROTOTYPE_COPY_OBJECT_FUNCTION(FE_node);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(FE_node);

int define_FE_field_at_node(struct FE_node *node,struct FE_field *field,
	int *components_number_of_derivatives,int *components_number_of_versions,
	enum FE_nodal_value_type **components_nodal_value_types);
/*******************************************************************************
LAST MODIFIED : 28 October 1998

DESCRIPTION :
Defines a field at a node (does not assign values).
==============================================================================*/

int for_FE_field_at_node(struct FE_field *field,
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
If an <iterator> is supplied and the <field> is defined at the <node> then the
result of the <iterator> is returned.  Otherwise, if an <iterator> is not
supplied and the <field> is defined at the <node> then a non-zero is returned.
Otherwise, zero is returned.
???DB.  Multiple behaviour dangerous ?
==============================================================================*/

int for_each_FE_field_at_node(FE_node_field_iterator_function *iterator,
	void *user_data,struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Calls the <iterator> for each field defined at the <node> until the <iterator>
returns 0 or it runs out of fields.  Returns the result of the last <iterator>
called.
==============================================================================*/

int for_each_FE_field_at_node_indexer_first(
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Calls the <iterator> for each field defined at the <node> until the <iterator>
returns 0 or it runs out of fields.  Returns the result of the last <iterator>
called. This version insists that any field used as an indexer_field for another
field in the list is output first.
==============================================================================*/

int FE_node_has_FE_field_values(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 24 September 1999

DESCRIPTION :
Returns true if any single field defined at <node> has values stored with
the field.
==============================================================================*/

int equivalent_FE_field_at_nodes(struct FE_field *field,struct FE_node *node_1,
	struct FE_node *node_2);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Returns non-zero if the <field> is defined in the same way at the two nodes.
==============================================================================*/

int equivalent_FE_fields_at_nodes(struct FE_node *node_1,
	struct FE_node *node_2);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Returns non-zero if the same fields are defined in the same ways at the two
nodes.
==============================================================================*/

struct FE_node *node_string_to_FE_node(char *name,
	struct MANAGER(FE_node) *node_manager);
/*******************************************************************************
LAST MODIFIED : 9 September 1999

DESCRIPTION :
Returns the node in the <node_manager> with the number written in the <name>.
==============================================================================*/

int FE_nodal_value_version_exists(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type);
/*******************************************************************************
LAST MODIFIED : 23 June 1999

DESCRIPTION :
Returns 1 if the field, component number, version and type are stored at the
node.
???DB.  May need speeding up
==============================================================================*/

int get_FE_nodal_value_as_string(struct FE_node *node,
	struct FE_field *field,int component_number,int version,
	enum FE_nodal_value_type type,char **string);
/*******************************************************************************
LAST MODIFIED : 6 September 1999

DESCRIPTION :
Returns as a string the value for the (<version>, <type>) for the <field>
<component_number> at the <node>.
It is up to the calling function to DEALLOCATE the returned string.
==============================================================================*/

int get_FE_nodal_double_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,double *value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Gets a particular double value (<version>, <type>) for the field <component> at
the <node>.
???DB.  May need speeding up
==============================================================================*/

int set_FE_nodal_double_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,double value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets a particular double value (<version>, <type>) for the field <component> at
the <node>.
==============================================================================*/

int get_FE_nodal_FE_value_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value *value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Gets a particular FE_value value (<version>, <type>) for the field <component>
at the <node>.
???DB.  May need speeding up
==============================================================================*/

int set_FE_nodal_FE_value_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets a particular FE_value value (<version>, <type>) for the field <component>
at the <node>.
==============================================================================*/

int get_FE_nodal_FE_value_array_value_at_FE_value_time(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value time,FE_value *value);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
Gets the FE_value <value> from the node's FE_value array at the given
<component> <version> <type>, using the <time> to index the array.
The field must have time defined for it, and the number of times must match
the number of array elements. If <time> is within the node field's time array's 
range, but doesn't correspond exactly to an array element, interpolates to determine 
<value>.
==============================================================================*/

int get_FE_nodal_short_array_value_at_FE_value_time(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value time,short *value);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
Gets the short <value> from the node's short array at the given
<component> <version> <type>, using the <time> to index the array.
The field must have time defined for it, and the number of times must match
the number of array elements. If <time> is within the node field's time array's 
range, but doesn't correspond exactly to an array element, interpolates to determine 
<value>.
==============================================================================*/

int get_FE_nodal_float_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,float *value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Gets a particular float value (<version>, <type>) for the field <component> at
the <node>.
???DB.  May need speeding up
==============================================================================*/

int set_FE_nodal_float_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,float value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets a particular float value (<version>, <type>) for the field <component> at
the <node>.
==============================================================================*/

int get_FE_nodal_int_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int *value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Gets a particular int value (<version>, <type>) for the field <component> at
the <node>.
???DB.  May need speeding up
==============================================================================*/

int set_FE_nodal_int_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int value);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets a particular int value (<version>, <type>) for the field <component> at
the <node>.
==============================================================================*/

int get_FE_nodal_element_xi_value(struct FE_node *node,
	struct FE_field *field, int component_number, int version,
	enum FE_nodal_value_type type, struct FE_element **element, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 23 April 1999

DESCRIPTION :
Gets a particular element_xi_value (<version>, <type>) for the field <component> at the
<node>.  SAB Note: It doesn't use a FE_field_component as I don't think any of them
should.
==============================================================================*/

int set_FE_nodal_element_xi_value(struct FE_node *node,
	struct FE_field *field, int component_number, int version,
	enum FE_nodal_value_type type,struct FE_element *element, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 23 April 1999

DESCRIPTION :
Sets a particular element_xi_value (<version>, <type>) for the field <component> at the
<node>.  SAB Note: It doesn't use a FE_field_component as I don't think any of them
should.
==============================================================================*/

int ensure_FE_node_is_in_group(struct FE_node *node,void *node_group_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function for adding <node> to <node_group> if not currently in it.
Note: If function is to be called several times on a group then surround by
MANAGED_GROUP_BEGIN_CACHE/END_CACHE calls for efficient manager messages.
==============================================================================*/

int ensure_FE_node_is_not_in_group(struct FE_node *node,void *node_group_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function for removing <node> from <node_group> if currently in it.
Note: If function is to be called several times on a group then surround by
MANAGED_GROUP_BEGIN_CACHE/END_CACHE calls for efficient manager messages.
==============================================================================*/

int ensure_FE_node_is_in_list(struct FE_node *node,void *node_list_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function for adding <node> to <node_list> if not currently in it.
==============================================================================*/

int ensure_FE_node_is_not_in_list(struct FE_node *node,void *node_list_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function for removing <node> from <node_list> if currently in it.
==============================================================================*/

int FE_node_can_be_destroyed(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 16 April 1999

DESCRIPTION :
Returns true if the <node> is only accessed once (assumed to be by the manager).
==============================================================================*/

int FE_node_has_embedded_element_or_node(struct FE_node *node,void *data_void);
/*******************************************************************************
LAST MODIFIED : 28 April 1999

DESCRIPTION :
Returns true if <node> conatins a field which depends on the changed_element
of changed_node in the <data_void>.
==============================================================================*/

int FE_node_not_in_list(struct FE_node *node,void *list_of_nodes_void);
/*******************************************************************************
LAST MODIFIED : 13 April 1999

DESCRIPTION :
Returns true if <node> is not in <list_of_nodes>.
==============================================================================*/

char *get_FE_nodal_value_type_string(enum FE_nodal_value_type nodal_value_type);
/*******************************************************************************
LAST MODIFIED : 21 June 1999

DESCRIPTION :
Returns a pointer to a static string token for the given <nodal_value_type>.
The calling function must not deallocate the returned string.
==============================================================================*/

#if defined (OLD_CODE) 
/* superceded by  get_FE_nodal_value_type,get_FE_nodal_array_number_of_elements */
int get_FE_nodal_array_attributes(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,enum Value_type *value_type,
	int *number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Get the value_type and the number of array values for the array in the nodal 
values_storage for the given node, component, version,type.

Give an error if field->values_storage isn't storing array types.
==============================================================================*/
#endif

enum Value_type get_FE_nodal_value_type(struct FE_node *node,
	struct FE_field_component *component,int version);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
Get's a node's field's value type
==============================================================================*/

int get_FE_nodal_array_number_of_elements(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
Returns the number of elements  for the array in the nodal 
values_storage for the given node, component, version,type.

Returns -1 upon error.

Give an error if field->values_storage isn't storing array types.
==============================================================================*/

int get_FE_nodal_double_array_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,double *array, int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 22 March 1999

DESCRIPTION :
Gets a particular double array value (<version>, <type>) for the field <component> 
at the <node> of length number_of_array_values. 
If number_of_array_values > the stored arrays max length, gets the max length.
MUST allocate space for the array before calling this function.

Use get_FE_nodal_array_attributes() to get the size of an array.
??JW write get_FE_nodal_max_array_size() to avoid many ALLOCATES 
if need to locally and repetatively get many arrays.  
==============================================================================*/

int set_FE_nodal_double_array_value(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,double *array,int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 22 March 1999

DESCRIPTION :
Finds any existing double array at the place specified by (<version>, <type>) 
for the field <component>  at the <node>.

Frees it.
Allocates a new array, according to number_of_array_values. 
Copies the contents of the passed array to this allocated one.
Copies number of array values, and the pointer to the allocated array to the specified 
place in the node->values_storage. 

Therefore, should free the passed array, after passing it to this function.

The nodal values_storage MUST have been previously allocated within 
define_FE_field_at_node()
==============================================================================*/

int get_FE_nodal_short_array(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,short *array, int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
Gets a particular short array value (<version>, <type>) for the field <component> 
at the <node> of length number_of_array_values. 
If number_of_array_values > the stored arrays max length, gets the max length.
MUST allocate space for the array before calling this function.

Use get_FE_nodal_array_attributes() to get the size of an array.
??JW write get_FE_nodal_max_array_size() to avoid many ALLOCATES 
if need to locally and repetatively get many arrays.  

==============================================================================*/

int set_FE_nodal_short_array(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,short *array,int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
Finds any existing short array at the place specified by (<version>, <type>) 
for the field <component>  at the <node>.

Frees it.
Allocates a new array, according to number_of_array_values. 
Copies the contents of the passed array to this allocated one.
Copies number of array values, and the pointer to the allocated array to the specified 
place in the node->values_storage. 

Therefore, should free the passed array, after passing it to this function.

The nodal values_storage MUST have been previously allocated within 
define_FE_field_at_node()
==============================================================================*/

int get_FE_nodal_FE_value_array(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value *array, int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
Gets a particular FE_value array value (<version>, <type>) for the field <component> 
at the <node> of length number_of_array_values. 
If number_of_array_values > the stored arrays max length, gets the max length.
MUST allocate space for the array before calling this function.

Use get_FE_nodal_array_attributes() to get the size of an array.
??JW write get_FE_nodal_max_array_size() to avoid many ALLOCATES 
if need to locally and repetatively get many arrays.  
==============================================================================*/

int set_FE_nodal_FE_value_array(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value *array,int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
Finds any existing FE_value array at the place specified by (<version>, <type>) 
for the field <component>  at the <node>.

Frees it.
Allocates a new array, according to number_of_array_values. 
Copies the contents of the passed array to this allocated one.
Copies number of array values, and the pointer to the allocated array to the specified 
place in the node->values_storage. 

Therefore, should free the passed array, after passing it to this function.

The nodal values_storage MUST have been previously allocated within 
define_FE_field_at_node()
==============================================================================*/

FE_value get_FE_nodal_FE_value_array_element(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int element_number);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
Gets a particular FE_value_array_value element for the array 
(<version>, <type>) for the field <component> 
at the <node>. 

==============================================================================*/

int set_FE_nodal_FE_value_array_element(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int element_number,FE_value value);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
sets a particular FE_value_array_value element for the array 
(<version>, <type>) for the field <component> 
at the <node>. 

==============================================================================*/

short get_FE_nodal_short_array_element(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int element_number);
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
Gets a particular short_array_value element for the array 
(<version>, <type>) for the field <component> 
at the <node>. 

==============================================================================*/

int set_FE_nodal_short_array_element(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,int element_number,short value);
/*******************************************************************************
LAST MODIFIED : 4 October 1999

DESCRIPTION :
sets a particular short_array_value element for the array 
(<version>, <type>) for the field <component> 
at the <node>. 

==============================================================================*/

int get_FE_nodal_FE_value_array_min_max(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,FE_value *minimum,FE_value *maximum);
/*******************************************************************************
LAST MODIFIED : 29 September 1999

DESCRIPTION :
Gets a the minimum and maximum values for the FE_value_array 
(<version>, <type>) for the field <component> at the <node>

==============================================================================*/

int get_FE_nodal_short_array_min_max(struct FE_node *node,
	struct FE_field_component *component,int version,
	enum FE_nodal_value_type type,short *minimum,short *maximum);
/*******************************************************************************
LAST MODIFIED : 29 September 1999

DESCRIPTION :
Gets a the minimum and maximum values for the _value_array 
(<version>, <type>) for the field <component> at the <node>

==============================================================================*/

int get_FE_nodal_string_value(struct FE_node *node,
	struct FE_field *field,int component_number,int version,
	enum FE_nodal_value_type type,char **string);
/*******************************************************************************
LAST MODIFIED : 3 September 1999

DESCRIPTION :
Returns a copy of the string for <version>, <type> of <field><component_number> 
at the <node>. Up to the calling function to DEALLOCATE the returned string.
Returned <*string> may be a valid NULL if that is what is in the node.
==============================================================================*/

int set_FE_nodal_string_value(struct FE_node *node,
	struct FE_field *field,int component_number,int version,
	enum FE_nodal_value_type type,char *string);
/*******************************************************************************
LAST MODIFIED : 3 September 1999

DESCRIPTION :
Copies and sets the <string> for <version>, <type> of <field><component_number>
at the <node>. <string> may be NULL.
==============================================================================*/

int set_FE_nodal_field_double_values(struct FE_field *field,
	struct FE_node *node,double *values, int *number_of_values);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets the node field's values storage (at node->values_storage, NOT 
field->values_storage) with the doubles in values. 
Returns the number of doubles copied in number_of_values.
Assumes that values is set up with the correct number of doubles.
Assumes that the node->values_storage has been allocated with enough 
memory to hold all the values.
Assumes that the nodal fields have been set up, with information to 
place the values.
==============================================================================*/

int get_FE_nodal_field_number_of_values(struct FE_field *field,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 20 September 1999

DESCRIPTION :
Returns the total number of values stored for that field at the node, equals
sum of (1+num_derivatives)*num_versions for each component.
==============================================================================*/

int get_FE_nodal_field_FE_value_values(struct FE_field *field,
	struct FE_node *node,int *number_of_values,FE_value **values);
/*******************************************************************************
LAST MODIFIED : 20 September 1999

DESCRIPTION :
Allocates and returns a copy of the <number_of_values>-length <values> array
stored at the <node> for all components derivatives and versions of <field>.
It is up to the calling function to DEALLOCATE the returned array.
==============================================================================*/

int set_FE_nodal_field_FE_value_values(struct FE_field *field,
	struct FE_node *node,FE_value *values, int *number_of_values);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets the node field's values storage (at node->values_storage, NOT 
field->values_storage) with the FE_values in values. 
Returns the number of FE_values copied in number_of_values.
Assumes that values is set up with the correct number of FE_values.
Assumes that the node->values_storage has been allocated with enough 
memory to hold all the values.
Assumes that the nodal fields have been set up, with information to 
place the values.
==============================================================================*/

int set_FE_nodal_field_float_values(struct FE_field *field,
	struct FE_node *node,float *values, int *number_of_values);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets the node field's values storage (at node->values_storage, NOT 
field->values_storage) with the floats in values. 
Returns the number of floats copied in number_of_values.
Assumes that values is set up with the correct number of floats.
Assumes that the node->values_storage has been allocated with enough 
memory to hold all the values.
Assumes that the nodal fields have been set up, with information to 
place the values.
==============================================================================*/

int get_FE_nodal_field_int_values(struct FE_field *field,
	struct FE_node *node,int *number_of_values,int **values);
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Allocates and returns a copy of the <number_of_values>-length <values> array
stored at the <node> for all components derivatives and versions of <field>.
It is up to the calling function to DEALLOCATE the returned array.
==============================================================================*/

int set_FE_nodal_field_int_values(struct FE_field *field,
	struct FE_node *node,int *values, int *number_of_values);
/*******************************************************************************
LAST MODIFIED : 30 August 1999

DESCRIPTION :
Sets the node field's values storage (at node->values_storage, NOT 
field->values_storage) with the integers in values. 
Returns the number of integers copied in number_of_values.
Assumes that values is set up with the correct number of ints.
Assumes that the node->values_storage has been allocated with enough 
memory to hold all the values.
Assumes that the nodal fields have been set up, with information to 
place the values.
==============================================================================*/

int get_FE_node_number_of_values(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
Returns the number of values stored at the <node>.
==============================================================================*/

int get_FE_node_number_of_fields(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Returns the number of fields stored at the <node>.
==============================================================================*/

enum FE_nodal_value_type *get_FE_node_field_component_nodal_value_types(
	struct FE_node *node,struct FE_field *field,int component_number);
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Returns an array of the (1+number_of_derivatives) value types for the
node field component.
It is up to the calling function to DEALLOCATE the returned array.
==============================================================================*/

int get_FE_node_field_component_number_of_derivatives(struct FE_node *node,
	struct FE_field *field,int component_number);
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Returns the number of derivatives for the node field component.
==============================================================================*/

int get_FE_node_field_component_number_of_versions(struct FE_node *node,
	struct FE_field *field,int component_number);
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Returns the number of versions for the node field component.
==============================================================================*/

int get_FE_node_cm_node_identifier(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
Returns the cmiss number of the <node>.
???DB.  GET_OBJECT_IDENTIFIER ?
==============================================================================*/

struct FE_field *get_FE_node_default_coordinate_field(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 3 November 1998

DESCRIPTION :
Returns the default coordinate field of the <node>.
==============================================================================*/

int merge_FE_node(struct FE_node *destination,struct FE_node *source);
/*******************************************************************************
LAST MODIFIED : 28 April 1998

DESCRIPTION :
Merges the fields in <destination> with those from <source>, leaving the
combined fields in <destination>.
???DB.  It is important that the new fields for <destination> are added after
	those for <source> (existing node in the manager) otherwise elements that
	use <source> will be broken
???DB.  This is not ideal, but can't be fixed without having pointers from the
	nodes back to the elements
==============================================================================*/

int set_FE_node(struct Parse_state *state,void *node_address_void,
	void *node_manager_void);
/*******************************************************************************
LAST MODIFIED : 28 April 1997

DESCRIPTION :
Used in command parsing to translate a node name into an node.
???DB.  Should it be here ?
==============================================================================*/

int list_FE_node(struct FE_node *node,void *list_node_information);
/*******************************************************************************
LAST MODIFIED : 9 February 1998

DESCRIPTION :
Outputs the information contained at the node.
==============================================================================*/

PROTOTYPE_LIST_FUNCTIONS(FE_node);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_node,cm_node_identifier,int);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(FE_node,cm_node_identifier,int);

PROTOTYPE_MANAGER_FUNCTIONS(FE_node);

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(FE_node,cm_node_identifier,int);

int overwrite_FE_node_with_cm_node_identifier(struct FE_node *destination,
	struct FE_node *source);
/*******************************************************************************
LAST MODIFIED : 27 September 1995

DESCRIPTION :
Overwrites without merging.
???DB.  Used by node editor.  Merging/overwriting needs sorting out.
==============================================================================*/

int overwrite_FE_node_without_cm_node_identifier(struct FE_node *destination,
	struct FE_node *source);
/*******************************************************************************
LAST MODIFIED : 27 September 1995

DESCRIPTION :
Overwrites without merging.
???DB.  Used by node editor.  Merging/overwriting needs sorting out.
==============================================================================*/

int get_next_FE_node_number(struct MANAGER(FE_node) *manager,int node_number);
/*******************************************************************************
LAST MODIFIED : 17 February 1996

DESCRIPTION :
Finds the next vacant node number in the manager.
???GMH.  Could speed up...
???DB.  Extend manager functionality ?
==============================================================================*/

PROTOTYPE_MANAGED_GROUP_FUNCTIONS(FE_node);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_GROUP_FUNCTION(FE_node,cm_node_identifier,int);

#if defined (OLD_CODE)
/* following are all set up automatically for MANAGED_GROUPs: */
PROTOTYPE_GROUP_FUNCTIONS(FE_node);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_GROUP_FUNCTION(FE_node,cm_node_identifier,int);

PROTOTYPE_LIST_FUNCTIONS(GROUP(FE_node));

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(GROUP(FE_node),name,char *);

PROTOTYPE_MANAGER_FUNCTIONS(GROUP(FE_node));

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(GROUP(FE_node),name,char *);
#endif /* defined (OLD_CODE) */

int set_FE_node_group(struct Parse_state *state,void *node_group_address_void,
	void *node_group_manager_void);
/*******************************************************************************
LAST MODIFIED : 18 June 1996

DESCRIPTION :
Used in command parsing to translate a node group name into an node group.
???DB.  Should it be here ?
==============================================================================*/

int set_FE_node_group_list(struct Parse_state *state,void *node_list_void,
	void *node_group_manager_void);
/*******************************************************************************
LAST MODIFIED : 18 June 1996

DESCRIPTION :
Used in command parsing to create a list of node groups.
???DB.  Should it be here ?
==============================================================================*/

int list_group_FE_node(struct GROUP(FE_node) *node_group,void *list_nodes);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Outputs the information contained by the node group.
==============================================================================*/

struct FE_basis *CREATE(FE_basis)(int *type);
/*******************************************************************************
LAST MODIFIED : 1 October 1995

DESCRIPTION :
A basis is created with the specified <type> (duplicated).  The basis is
returned.
==============================================================================*/

int DESTROY(FE_basis)(struct FE_basis **basis_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for <**basis_address> and sets <*basis_address> to NULL.
==============================================================================*/

struct FE_basis *make_FE_basis(int *basis_type,struct MANAGER(FE_basis) *basis_manager );
/*******************************************************************************
LAST MODIFIED :2 August 1999

DESCRIPTION :
Finds the specfied FE_basis in the basis managed. If it isn't there, creates it,
and adds it to the manager.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_basis);

PROTOTYPE_LIST_FUNCTIONS(FE_basis);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_basis,type,int *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(FE_basis,type,int *);

PROTOTYPE_MANAGER_FUNCTIONS(FE_basis);

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(FE_basis,type,int *);

char *FE_basis_type_string(enum FE_basis_type basis_type);
/*******************************************************************************
LAST MODIFIED : 1 April 1999

DESCRIPTION :
Returns a pointer to a static string token for the given <basis_type>.
The calling function must not deallocate the returned string.
#### Must ensure implemented correctly for new FE_basis_type. ####
==============================================================================*/

struct Linear_combination_of_global_values
	*CREATE(Linear_combination_of_global_values)(int number_of_global_values);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a linear combination of global values.
Allocates storage for the global and coefficient indices and sets to -1.
==============================================================================*/

int DESTROY(Linear_combination_of_global_values)(
	struct Linear_combination_of_global_values **linear_combination_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the linear combination and sets
<*linear_combination_address> to NULL.
==============================================================================*/

struct Standard_node_to_element_map *CREATE(Standard_node_to_element_map)(
	int node_index,int number_of_nodal_values);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a standard node to element map.
Allocates storage for the nodal value and scale factor indices and sets to -1.
==============================================================================*/

int DESTROY(Standard_node_to_element_map)(
	struct Standard_node_to_element_map **map_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the map and sets <*map_address> to NULL.
==============================================================================*/

struct General_node_to_element_map *CREATE(General_node_to_element_map)(
	int node_index,int number_of_nodal_values);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a general node to element map.
Allocates storage for the pointers to the linear combinations of field values
and sets to NULL.
==============================================================================*/

int DESTROY(General_node_to_element_map)(
	struct General_node_to_element_map **map_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the map and sets <*map_address> to NULL.
==============================================================================*/

struct FE_element_field_component *CREATE(FE_element_field_component)(
	enum Global_to_element_map_type type,int number_of_maps,
	struct FE_basis *basis,FE_element_field_component_modify modify);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Allocates memory and enters values for a component of a element field.
Allocates storage for the global to element maps and sets to NULL.
==============================================================================*/

int DESTROY(FE_element_field_component)(
	struct FE_element_field_component **component_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the component and sets <*component_address> to NULL.
==============================================================================*/

struct FE_element_field *CREATE(FE_element_field)(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Allocates memory and assigns fields for an element field.  The storage is
allocated for the pointers to the components and set to NULL.
==============================================================================*/

int DESTROY(FE_element_field)(struct FE_element_field **element_field_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for element field and sets <*element_field_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_element_field);

int inherit_FE_element_field(struct FE_element *element,struct FE_field *field,
	struct FE_element_field **element_field_address,
	struct FE_element **field_element_address,
	FE_value **coordinate_transformation_address,
	struct FE_element *top_level_element);
/*******************************************************************************
LAST MODIFIED : 1 July 1999

DESCRIPTION :
If <field> is NULL, element values are calculated for the coordinate field.
The optional <top_level_element> forces inheritance from it as needed.
==============================================================================*/

int calculate_FE_element_field_values(struct FE_element *element,
	struct FE_field *field,char calculate_derivatives,
	struct FE_element_field_values *element_field_values,
	struct FE_element *top_level_element);
/*******************************************************************************
LAST MODIFIED : 1 July 1999

DESCRIPTION :
If <field> is NULL, element values are calculated for the coordinate field.  The
function fills in the fields of the <element_field_values> structure, but does
not allocate memory for the structure.
The optional <top_level_element> forces inheritance from it as needed.
==============================================================================*/

int FE_element_field_values_are_for_element(
	struct FE_element_field_values *element_field_values,
	struct FE_element *element,struct FE_element *field_element);
/*******************************************************************************
LAST MODIFIED : 1 July 1999

DESCRIPTION :
Returns true if the <element_field_values> originated from <element>, either
directly or inherited from <field_element>. If <field_element> is NULL no match
is required with the field_element in the <element_field_values>.
==============================================================================*/

int clear_FE_element_field_values(
	struct FE_element_field_values *element_field_values);
/*******************************************************************************
LAST MODIFIED : 10 August 1993

DESCRIPTION :
Frees the memory for the fields of the <element_field_values> structure.
==============================================================================*/

int calculate_FE_element_field_nodes(struct FE_element *element,
	struct FE_field *field,struct LIST(FE_node) *element_field_nodes);
/*******************************************************************************
LAST MODIFIED : 7 February 1998

DESCRIPTION :
If <field> is NULL, element nodes are calculated for the coordinate field.  The
function adds the nodes to the list <element_field_nodes>.  Components that are
not node-based are ignored.
==============================================================================*/

int calculate_FE_element_field(int component_number,
	struct FE_element_field_values *element_field_values,FE_value *xi_coordinates,
	FE_value *values,FE_value *jacobian);
/*******************************************************************************
LAST MODIFIED : 2 October 1998

DESCRIPTION :
Calculates the <values> of the field specified by the <element_field_values> at
the <xi_coordinates>.  The storage for the <values> should have been allocated
outside the function.  The <jacobian> will be calculated if it is not NULL (and
the derivatives values have been calculated).  Only the <component_number>+1
component will be calculated if 0<=component_number<number of components.  For a
single component, the value will be put in the first position of <values> and
the derivatives will start at the first position of <jacobian>.
==============================================================================*/

int calculate_FE_element_field_as_string(int component_number,
	struct FE_element_field_values *element_field_values,FE_value *xi_coordinates,
	char **string);
/*******************************************************************************
LAST MODIFIED : 17 October 1999

DESCRIPTION :
Calculates the values of element field specified by the <element_field_values>
at the <xi_coordinates> and returns them as the allocated <string>. Only the
<component_number>+1 component will be calculated if
0<=component_number<number of components. If more than 1 component is calculated
then values are comma separated. Derivatives are not included in the string,
even if calculated for the <element_field_values>.
It is up to the calling function to DEALLOCATE the returned string.
==============================================================================*/

int calculate_FE_element_field_int_values(int component_number,
	struct FE_element_field_values *element_field_values,FE_value *xi_coordinates,
	int *values);
/*******************************************************************************
LAST MODIFIED : 14 October 1999

DESCRIPTION :
Calculates the <values> of the integer field specified by the
<element_field_values> at the <xi_coordinates>. The storage for the <values>
should have been allocated outside the function. Only the <component_number>+1
component will be calculated if 0<=component_number<number of components. For a
single component, the value will be put in the first position of <values>.
==============================================================================*/

int calculate_FE_element_field_string_values(int component_number,
	struct FE_element_field_values *element_field_values,FE_value *xi_coordinates,
	char **values);
/*******************************************************************************
LAST MODIFIED : 19 October 1999

DESCRIPTION :
Returns allocated copies of the string values of the field specified by the
<element_field_values> at the <xi_coordinates>. <values> must be allocated with
enough space for the number_of_components strings, but the strings themselves
are allocated here. Only the <component_number>+1 component will be calculated
if 0<=component_number<number of components. For a single component, the value
will be put in the first position of <values>.
It is up to the calling function to deallocate the returned string values.
==============================================================================*/

int calculate_FE_element_anatomical(
	struct FE_element_field_values *coordinate_element_field_values,
	struct FE_element_field_values *anatomical_element_field_values,
	FE_value *xi_coordinates,FE_value *x,FE_value *y,FE_value *z,FE_value a[3],
	FE_value b[3],FE_value c[3],FE_value *dx_dxi);
/*******************************************************************************
LAST MODIFIED : 16 November 1998

DESCRIPTION :
Calculates the cartesian coordinates (<x>, <y> and <z>), and the fibre (<a>),
cross-sheet (<b>) and sheet-normal (<c>) vectors from a coordinate element field
and an anatomical element field.  The storage for the <x>, <y>, <z>, <a>, <b>
and <c> should have been allocated outside the function.
If later conversion of a, b and c to vectors in xi space is required, the
optional <dx_dxi> parameter should be supplied and point to a enough memory to
contain the nine derivatives of x,y,z w.r.t. three xi. These are returned in the
order dx/dxi1, dx/dxi2, dx/dxi3, dy/dxi1 etc. Note that there will always be
nine values returned, regardless of the element dimension.
==============================================================================*/

PROTOTYPE_LIST_FUNCTIONS(FE_element_field);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_element_field,field, \
	struct FE_field *);

struct FE_element_field_info *CREATE(FE_element_field_info)(
	struct LIST(FE_element_field) *element_field_list);
/*******************************************************************************
LAST MODIFIED : 24 September 1995

DESCRIPTION :
Searchs the list of all element field information
(<all_FE_element_field_info>) for one containing the specified element
fields.  If one is found it is returned, otherwise a new element field
information is created with duplicated element field lists, added to
<all_FE_element_field_info> and returned.
==============================================================================*/

int DESTROY(FE_element_field_info)(
	struct FE_element_field_info **field_info_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1994

DESCRIPTION :
Removes the information from the list <all_FE_element_field_info>, frees
the memory for the information and sets <*field_info_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_element_field_info);

PROTOTYPE_LIST_FUNCTIONS(FE_element_field_info);

struct FE_element_field_info *find_FE_element_field_info_in_list(	
	struct LIST(FE_element_field) *element_field_list,
	struct LIST(FE_element_field_info) *list);
/*******************************************************************************
LAST MODIFIED : 24 September 1995

DESCRIPTION :
Searchs the <list> for the element field_information with the specified element
fields.
==============================================================================*/

struct FE_element_node_scale_field_info
	*CREATE(FE_element_node_scale_field_info)(int values_storage_size,
	Value_storage *values_storage,int number_of_nodes,struct FE_node **nodes,
	int number_of_scale_factor_sets,void **scale_factor_set_identifiers,
	int *numbers_in_scale_factor_sets,int number_of_scale_factors,
	FE_value *scale_factors,struct FE_element_field_info *field_info);
/*******************************************************************************
LAST MODIFIED : 29 September 1999

DESCRIPTION :
Allocates memory and assigns fields for an element's node, scale factor and
field information structure.  Note that the arguments are duplicated.
???RC Made it possible to have no scale factor sets.
==============================================================================*/

int DESTROY(FE_element_node_scale_field_info)(
	struct FE_element_node_scale_field_info
	**element_node_scale_field_info_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the node, scale and field information and sets
<*element_node_scale_field_info_address> to NULL.
==============================================================================*/

struct FE_element_shape *CREATE(FE_element_shape)(int dimension,int *type);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Searchs the list of all shapes (all_FE_element_shape) for a shape with the
specified <dimension> and <type>.  If one is not found, a shape is created (with
<type> duplicated) and added to the list of all shapes.  The shape is returned.
<type> is analogous to the basis type array, except that the entries are 0 or 1.
???DB.  To start with only construct "square" elements (<type> is NULL)
==============================================================================*/

int DESTROY(FE_element_shape)(struct FE_element_shape **element_shape_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Remove the shape from the list of all shapes.  Free the memory for the shape and
sets <*element_shape_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_element_shape);

PROTOTYPE_LIST_FUNCTIONS(FE_element_shape);

struct FE_element_shape *find_FE_element_shape_in_list(int dimension,int *type,
	struct LIST(FE_element_shape) *list);
/*******************************************************************************
LAST MODIFIED : 24 September 1995

DESCRIPTION :
Searchs the <list> for the element shape with the specified <dimension> and
<type> and returns the address of the element_shape.
==============================================================================*/

struct FE_element_shape *get_FE_element_shape_of_face(
	struct FE_element_shape *shape,int face_number);
/*******************************************************************************
LAST MODIFIED : 29 March 1999

DESCRIPTION :
From the parent <shape> returns the FE_element_shape for its face <face_number>.
The <shape> must be of dimension 2 or 3. Faces of 2-D elements are always lines.
The returned shape is ACCESSed here, and should be DEACCESSed when no longer
needed.
==============================================================================*/

struct FE_element_parent *CREATE(FE_element_parent)(struct FE_element *parent,
	int face_number);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Creates element parent structure and assigns the fields.
==============================================================================*/

int DESTROY(FE_element_parent)(
	struct FE_element_parent **element_parent_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the element parent structure and sets
<*element_parent_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_element_parent);

PROTOTYPE_LIST_FUNCTIONS(FE_element_parent);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_element_parent,parent, \
	struct FE_element *);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_element_parent,face_number, \
	int);

char *CM_element_type_string(enum CM_element_type cm_element_type);
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
Returns a static string describing the <cm_element_type>, eg. CM_LINE = 'line'.
Returned string must not be deallocated!
==============================================================================*/

struct FE_element *CREATE(FE_element)(struct CM_element_information *cm,
	struct FE_element *template_element);
/*******************************************************************************
LAST MODIFIED : 8 October 1999

DESCRIPTION :
Creates and returns an element with the specified <cm> identifier. The element
is given the same shape, faces and node scale field information as
<template_element>, or is blank if no template is specified. The shape and
node scale field information can be established for a blank element with
set_FE_element_shape and set_FE_element_node_scale_field_info.
The element is returned.
==============================================================================*/

int DESTROY(FE_element)(struct FE_element **element_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for the element, sets <*element_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_element);
PROTOTYPE_COPY_OBJECT_FUNCTION(FE_element);

int get_FE_element_dimension(struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 4 November 1999

DESCRIPTION :
Returns the dimension of the <element> or an error if it does not have a shape.
==============================================================================*/

int get_FE_element_shape(struct FE_element *element,
	struct FE_element_shape **shape);
/*******************************************************************************
LAST MODIFIED : 7 October 1999

DESCRIPTION :
Returns the <shape> of the <element>, if any. Only newly created blank elements
should have no shape.
==============================================================================*/

int set_FE_element_shape(struct FE_element *element,
	struct FE_element_shape *shape);
/*******************************************************************************
LAST MODIFIED : 6 October 1999

DESCRIPTION :
Sets the <shape> of <element>. Note that the element must not currently have a
shape in order for this to be set, ie. just created. Allocates and clears the
faces array in the element, so this must be clear too.
Should only be called for unmanaged elements.
==============================================================================*/

int get_FE_element_face(struct FE_element *element,int face_number,
	struct FE_element **face_element);
/*******************************************************************************
LAST MODIFIED : 7 October 1999

DESCRIPTION :
Returns the <face_element> for face <face_number> of <element>, where NULL means
there is no face. Element must have a shape and face.
==============================================================================*/

int set_FE_element_face(struct FE_element *element,int face_number,
	struct FE_element *face_element);
/*******************************************************************************
LAST MODIFIED : 7 October 1999

DESCRIPTION :
Sets face <face_number> of <element> to <face_element>, ensuring the
<face_element> has <element> as a parent. <face_element> may be NULL = no face.
Must have set the shape with set_FE_element_shape first.
Should only be called for unmanaged elements.
==============================================================================*/

int set_FE_element_node_scale_field_info(struct FE_element *element,
	int number_of_scale_factor_sets,void **scale_factor_set_identifiers,
	int *numbers_in_scale_factor_sets,int number_of_nodes);
/*******************************************************************************
LAST MODIFIED : 7 October 1999

DESCRIPTION :
Establishes <element> with FE_element_node_scale_field_information for the
specified scale factor sets and number of nodes. The element is assigned no
fields; fields may be added with define_FE_field_at_element within the framework
of the numbers of scale factors and nodes specified here. Note that the element
must not currently have any such information in order for this to be set, ie.
just created. Must have set the shape with set_FE_element_shape first.
Should only be called for unmanaged elements.
==============================================================================*/

int get_FE_element_node(struct FE_element *element,int node_number,
	struct FE_node **node);
/*******************************************************************************
LAST MODIFIED : 10 November 1999

DESCRIPTION :
Gets node <node_number>, from 0 to number_of_nodes-1 of <element> in <node>.
<element> must already have a shape and node_scale_field_information.
==============================================================================*/

int set_FE_element_node(struct FE_element *element,int node_number,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 11 October 1999

DESCRIPTION :
Sets node <node_number>, from 0 to number_of_nodes-1 of <element> to <node>.
<element> must already have a shape and node_scale_field_information.
Should only be called for unmanaged elements.
==============================================================================*/

int get_FE_element_scale_factor(struct FE_element *element,
	int scale_factor_number,FE_value *scale_factor);
/*******************************************************************************
LAST MODIFIED : 15 November 1999

DESCRIPTION :
Gets scale_factor <scale_factor_number>, from 0 to number_of_scale_factors-1 of
<element> to <scale_factor>.
<element> must already have a shape and node_scale_field_information.
==============================================================================*/

int set_FE_element_scale_factor(struct FE_element *element,
	int scale_factor_number,FE_value scale_factor);
/*******************************************************************************
LAST MODIFIED : 15 November 1999

DESCRIPTION :
Sets scale_factor <scale_factor_number>, from 0 to number_of_scale_factors-1 of
<element> to <scale_factor>.
<element> must already have a shape and node_scale_field_information.
Should only be called for unmanaged elements.
==============================================================================*/

int define_FE_field_at_element(struct FE_element *element,
	struct FE_field *field,struct FE_element_field_component **components);
/*******************************************************************************
LAST MODIFIED : 11 October 1999

DESCRIPTION :
Defines <field> at <element> using the given <components>. <element> must
already have a shape and node_scale_field_information.
Checks the range of nodes, scale factors etc. referred to by the components are
within the range of the node_scale_field_information, and that the basis
functions are compatible with the element shape.
If the components indicate the field is grid-based, checks that all the
components are grid-based with the same number_in_xi.
The <components> are duplicated by this functions, so the calling function must
destroy them.
Should only be called for unmanaged elements.
==============================================================================*/

int FE_element_has_grid_based_fields(struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
Returns true if any of the fields defined for element
==============================================================================*/

int FE_element_has_FE_field_values(struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 19 October 1999

DESCRIPTION :
Returns true if any single field defined at <element> has values stored with
the field. Returns 0 without error if no field information at element.
==============================================================================*/

int for_FE_field_at_element(struct FE_field *field,
	FE_element_field_iterator_function *iterator,void *user_data,
	struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
If an <iterator> is supplied and the <field> is defined at the <element> then
the result of the <iterator> is returned.  Otherwise, if an <iterator> is not
supplied and the <field> is defined at the <element> then a non-zero is
returned. Otherwise, zero is returned.
???DB.  Multiple behaviour dangerous ?
==============================================================================*/

int for_each_FE_field_at_element(FE_element_field_iterator_function *iterator,
	void *user_data,struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
Calls the <iterator> for each field defined at the <element> until the
<iterator> returns 0 or it runs out of fields.  Returns the result of the last
<iterator> called.
==============================================================================*/

int for_each_FE_field_at_element_indexer_first(
	FE_element_field_iterator_function *iterator,void *user_data,
	struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 5 October 1999

DESCRIPTION :
Calls the <iterator> for each field defined at the <element> until the
<iterator> returns 0 or it runs out of fields.  Returns the result of the last
<iterator> called. This version insists that any field used as an indexer_field
for another field in the list is output first.
==============================================================================*/

struct FE_field *get_FE_element_default_coordinate_field(
	struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 13 May 1999

DESCRIPTION :
Returns the first coordinate field defined over <element>, recursively getting
it from its first parent if it has no node scale field information.
==============================================================================*/

int add_FE_element_and_faces_to_group(struct FE_element *element,
	struct GROUP(FE_element) *element_group);
/*******************************************************************************
LAST MODIFIED : 14 April 1999

DESCRIPTION :
Ensures <element>, its faces (and theirs etc.) are in <element_group>. Only
top-level elements should be passed to this function.
Note: this function is recursive.
==============================================================================*/

struct Add_FE_element_and_faces_to_manager_data
  *CREATE(Add_FE_element_and_faces_to_manager_data)(
	struct MANAGER(FE_element) *element_manager);
/*******************************************************************************
LAST MODIFIED : 29 April 1999

DESCRIPTION :
Creates data for function add_FE_element_and_faces_to_manager. For efficiency,
use the created structure for as many calls to that function as possible,
however, it must be destroyed and recreated if any existing elements are
modified.
==============================================================================*/

int DESTROY(Add_FE_element_and_faces_to_manager_data)(
	struct Add_FE_element_and_faces_to_manager_data **add_element_data_address);
/*******************************************************************************
LAST MODIFIED : 29 April 1999

DESCRIPTION :
Cleans up memory used by the Add_FE_element_and_faces_to_manager_data. Must be
called to clean up user_data passed to add_FE_element_and_faces_to_manager.
==============================================================================*/

int add_FE_element_and_faces_to_manager(struct FE_element *element,
	void *add_FE_element_and_faces_to_manager_data_void);
/*******************************************************************************
LAST MODIFIED : 29 April 1999

DESCRIPTION :
Iterator function ensuring <element> is in the <element_manager>, and does the
same for any faces of the element, creating them if they do not already exist.
Function ensures that elements share existing faces and lines in preference to
creating new ones if they have matching shape and nodes. Only top-level elements
should be passed to this function. 
Notes:
- Since it is quite expensive to find faces/lines for a number of elements, the 
  user_data passed to this function must be created with
    CREATE(Add_FE_element_and_faces_to_manager_data), and destroyed with
    DESTROY(Add_FE_element_and_faces_to_manager_data) after the function is no
  longer to be called. This structure contains an indexed list for efficiently
  finding matching faces and lines that is very costly to establish.
- Enclose calls to this function in MANAGER_BEGIN/END_CACHE calls for the
  element manager.
- This function is recursive.
==============================================================================*/

int remove_FE_element_and_faces_from_group(struct FE_element *element,
	struct GROUP(FE_element) *element_group);
/*******************************************************************************
LAST MODIFIED : 14 April 1999

DESCRIPTION :
Removes <element> and all its faces that are not shared with other elements in
the group from <element_group>. Only top-level elements should be passed to this
function.
Notes:
- function assumes the element and all its faces etc. are in the element_group,
  so NUMEROUS errors will be reported if this is not the case. If in doubt, use
  FIND_BY_IDENTIFIER_IN_GROUP on the top-level element before passing it to
  this function. Note that partner function add_FE_element_and_faces_to_group
	guarantees that all faces are added with the element to the group.
- this function is recursive.
==============================================================================*/

int FE_element_can_be_destroyed(struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 23 April 1999

DESCRIPTION :
Returns true if the <element> is only accessed by its manager (ie. starting
access count of 1) and its parents, and that the same is true for its faces.
Notes:
- Faces do not access their parents - see CREATE(FE_element_parent).
- Ensure this function returns true before passing the element to
  remove_FE_element_and_faces_from_manager.
- This function is recursive.
==============================================================================*/

int remove_FE_element_and_faces_from_manager(struct FE_element *element,
	struct MANAGER(FE_element) *element_manager);
/*******************************************************************************
LAST MODIFIED : 16 April 1999

DESCRIPTION :
Removes <element> and all its faces that are not shared with other elements in
the manager from <element_manager>. The <element> is only removed from the
<element_manager> if its access_count becomes 1 after removing parent/face
connections. Before removing an element from the manager with this function,
make sure you get a true result from function FE_element_can_be_destroyed.
Only top-level elements should be passed to this function.
Notes:
- function assumes the element and all its faces etc. are in the
	element_manager, so NUMEROUS errors will be reported if this is
	not the case. If in doubt, use FIND_BY_IDENTIFIER_IN_MANAGER on the
  top-level element before passing it to this function. Note that partner
  function add_FE_element_and_faces_to_manager guarantees that any
	faces it finds or creates are added to the element_manager.
- this function is recursive.
==============================================================================*/

int merge_FE_element(struct FE_element *destination,struct FE_element *source);
/*******************************************************************************
LAST MODIFIED : 19 February 1997

DESCRIPTION :
Merges the fields in <destination> with those from <source>, leaving the
combined fields in <destination>.
==============================================================================*/

int list_FE_element(struct FE_element *element,void *dummy);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Outputs the information contained at the element.
==============================================================================*/

int list_FE_element_name(struct FE_element *element,void *dummy);
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
Outputs the name of the element as element/face/line #.
==============================================================================*/

PROTOTYPE_LIST_FUNCTIONS(FE_element);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_element,identifier, \
	struct CM_element_information *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(FE_element,identifier, \
	struct CM_element_information *);

PROTOTYPE_MANAGER_FUNCTIONS(FE_element);

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(FE_element,identifier, \
	struct CM_element_information *);

PROTOTYPE_MANAGED_GROUP_FUNCTIONS(FE_element);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_GROUP_FUNCTION(FE_element,identifier, \
	struct CM_element_information *);

#if defined (OLD_CODE)
/* following are all set up automatically for MANAGED_GROUPs: */
PROTOTYPE_GROUP_FUNCTIONS(FE_element);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_GROUP_FUNCTION(FE_element,identifier, \
	struct CM_element_information *);

PROTOTYPE_LIST_FUNCTIONS(GROUP(FE_element));

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(GROUP(FE_element),name,char *);

PROTOTYPE_MANAGER_FUNCTIONS(GROUP(FE_element));

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(GROUP(FE_element),name,char *);
#endif /* defined (OLD_CODE) */

int set_FE_element_group(struct Parse_state *state,
	void *element_group_address_void,void *element_group_manager_void);
/*******************************************************************************
LAST MODIFIED : 18 June 1996

DESCRIPTION :
Used in command parsing to translate a element group name into an element group.
???DB.  Should it be here ?
==============================================================================*/

int set_FE_element_group_or_all(struct Parse_state *state,
	void *element_group_address_void,void *element_group_manager_void);
/*******************************************************************************
LAST MODIFIED : 13 December 1999

DESCRIPTION :
Used in command parsing to translate a element group name into an element group.Valid NULL group means "all" groups.
==============================================================================*/

int list_group_FE_element(struct GROUP(FE_element) *element_group,void *dummy);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Outputs the information contained by the element group.
==============================================================================*/

int list_group_FE_element_name(struct GROUP(FE_element) *element_group,
	void *dummy);
/*******************************************************************************
LAST MODIFIED : 26 August 199

DESCRIPTION :
Outputs the name of the <element_group>.
==============================================================================*/

int theta_increasing_in_xi1(struct FE_element_field_component *component,
	struct FE_element *element,struct FE_field *field,int number_of_values,
	FE_value *values);
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Modifies the already calculated <values>.
???DB.  Only for certain bases
???RC.  Needs to be global to allow writing function in export_finite_element.
==============================================================================*/

int theta_non_decreasing_in_xi1(
	struct FE_element_field_component *component,struct FE_element *element,
	struct FE_field *field,int number_of_values,FE_value *values);
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Modifies the already calculated <values>.
???DB.  Only for certain bases
???RC.  Needs to be global to allow writing function in export_finite_element.
==============================================================================*/

int theta_decreasing_in_xi1(struct FE_element_field_component *component,
	struct FE_element *element,struct FE_field *field,int number_of_values,
	FE_value *values);
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Modifies the already calculated <values>.
???DB.  Only for certain bases
???RC.  Needs to be global to allow writing function in export_finite_element.
==============================================================================*/

int theta_non_increasing_in_xi1(
	struct FE_element_field_component *component,struct FE_element *element,
	struct FE_field *field,int number_of_values,FE_value *values);
/*******************************************************************************
LAST MODIFIED : 8 April 1999

DESCRIPTION :
Modifies the already calculated <values>.
???DB.  Only for certain bases
???RC.  Needs to be global to allow writing function in export_finite_element.
==============================================================================*/

struct FE_field *CREATE(FE_field)(void);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Creates and returns a struct FE_field with the given <name> identifier. The
new field defaults to 1 component, field_type FIELD, RECTANGULAR_CARTESIAN
coordinate system, no field values and the single component is named "1".
==============================================================================*/

int DESTROY(FE_field)(struct FE_field **field_address);
/*******************************************************************************
LAST MODIFIED : 23 September 1995

DESCRIPTION :
Frees the memory for <**field_address> and sets <*field_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_field);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(FE_field);

PROTOTYPE_LIST_FUNCTIONS(FE_field);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_field,name,char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(FE_field,name,char *);
PROTOTYPE_MANAGER_FUNCTIONS(FE_field);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(FE_field,name,char *);

int calculate_FE_field(struct FE_field *field,int component_number,
	struct FE_node *node,struct FE_element *element,FE_value *xi_coordinates,
	FE_value *value);
/*******************************************************************************
LAST MODIFIED : 23 October 1995

DESCRIPTION :
Calculates the <value> of the <field> for the specified <node> or <element> and
<xi_coordinates>.  If 0<=component_number<=number_of_components, then only the
specified component is calculated, otherwise all components are calculated.  The
storage for the <value> should have been allocated outside the function.
==============================================================================*/

int FE_field_matches_description(struct FE_field *field,char *name,
	enum FE_field_type fe_field_type,
	struct FE_field *indexer_field,int number_of_indexed_values,
	struct CM_field_information *cm_field_information,
	struct Coordinate_system *coordinate_system,enum Value_type value_type,
	int number_of_components,char **component_names,
	int number_of_times,enum Value_type time_value_type);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Returns true if <field> has exactly the same <name>, <field_info>... etc. as
those given in the parameters.
==============================================================================*/

int FE_fields_match(struct FE_field *field1,struct FE_field *field2);
/*******************************************************************************
LAST MODIFIED : 25 August 1999

DESCRIPTION :
Returns true if <field1> and <field2> are equivalent. Does this by calling
FE_field_matches_description for <field1> with the description for <field2>.
==============================================================================*/

struct FE_field *get_FE_field_manager_matched_field(
	struct MANAGER(FE_field) *fe_field_manager,char *name,
	enum FE_field_type fe_field_type,
	struct FE_field *indexer_field,int number_of_indexed_values,
	struct CM_field_information *cm_field_information,
	struct Coordinate_system *coordinate_system,enum Value_type value_type,
	int number_of_components,char **component_names,
	int number_of_times,enum Value_type time_value_type);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Using searches the <fe_field_manager> for a field, if one is found it is
checked to have the given parameters using FE_field_matches_description, with
an error reported if they do not properly match. If no field of the same name
exists, one is created and added to the manager. Note that <indexer_field> and
<number_of_indexed_values> are for INDEXED_FE_FIELD type only.
The field is returned, or NULL in case of inconsistency.
==============================================================================*/

struct FE_field *find_first_time_field_at_FE_node(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

Find the first time based field at a node
==============================================================================*/

int FE_field_can_be_destroyed(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 19 August 1999

DESCRIPTION :
Returns true if the <field> is only accessed once (assumed to be by the manager).
==============================================================================*/

int set_FE_field(struct Parse_state *state,void *field_address_void,
	void *fe_field_manager_void);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Modifier function to set the field from the command line.
==============================================================================*/

int FE_field_has_value_type(struct FE_field *field,void *user_data_value_type);
/*******************************************************************************
LAST MODIFIED : 4 May 1999

DESCRIPTION :
Returns true if the VALUE_TYPE specified in the <user_data_value_type> matches
the VALUE_TYPE of the <field>.
==============================================================================*/

int set_FE_field_conditional(struct Parse_state *state,
	void *field_address_void,void *set_field_data_void);
/*******************************************************************************
LAST MODIFIED : 4 May 1999

DESCRIPTION :
Modifier function to set the field from a command. <set_field_data_void> should
point to a struct Set_FE_field_conditional_data containing the
fe_field_manager and an optional conditional function for narrowing the
range of fields available for selection. If the conditional_function is NULL,
this function works just like set_FE_field.
==============================================================================*/

char *get_FE_field_component_name(struct FE_field *field,int component_no);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Returns the name of component <component_no> of <field>. If no name is stored
for the component, a string comprising the value component_no+1 is returned.
==============================================================================*/

int set_FE_field_component_name(struct FE_field *field,int component_no,
	char *component_name);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Sets the name of component <component_no> of <field>.
==============================================================================*/

struct Coordinate_system *get_FE_field_coordinate_system(
	struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 22 January 1999

DESCRIPTION :
Returns the coordinate system for the <field>.
==============================================================================*/

int set_FE_field_coordinate_system(struct FE_field *field,
	struct Coordinate_system *coordinate_system);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Sets the coordinate system of the <field>.
==============================================================================*/

int get_FE_field_number_of_components(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 16 November 1998

DESCRIPTION :
Returns the number of components for the <field>.
==============================================================================*/

int set_FE_field_number_of_components(struct FE_field *field,
	int number_of_components);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Sets the number of components in the <field>. Automatically assumes names for
any new components. Clears/reallocates the values_storage for FE_field_types
that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but only if number
of components changes. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int get_FE_field_number_of_values(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 16 November 1998

DESCRIPTION :
Returns the number of global values for the <field>.
==============================================================================*/

int get_FE_field_number_of_times(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

DESCRIPTION :
Returns the number of global times for the <field>.
==============================================================================*/

int set_FE_field_number_of_times(struct FE_field *field,
	int number_of_times);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

DESCRIPTION :
Sets the number of times stored with the <field>
REALLOCATES the requires memory in field->value_storage, based upon the 
field->time_value_type. 

For non-array types, the contents of field->times_storage is:
   | data type (eg FE_value) | x number_of_times

For array types, the contents of field->times is:
   ( | int (number of array values) | pointer to array (eg double *) |
	 x number_of_times )

Sets data in this memory to 0, pointers to NULL.

MUST have called set_FE_field_time_value_type() before calling this function.
Should only call this function for unmanaged fields.
==============================================================================*/

enum CM_field_type get_FE_field_CM_field_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Returns the CM_field_type of the <field>.
==============================================================================*/

int set_FE_field_CM_field_type(struct FE_field *field,
	enum CM_field_type cm_field_type);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Sets the CM_field_type of the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

enum FE_field_type get_FE_field_FE_field_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Returns the FE_field_type for the <field>.
==============================================================================*/

int set_FE_field_type_constant(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type CONSTANT_FE_FIELD.
Allocates and clears the values_storage of the field to fit
field->number_of_components of the current value_type.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int set_FE_field_type_general(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type GENERAL_FE_FIELD.
Frees any values_storage currently in use by the field.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int get_FE_field_type_indexed(struct FE_field *field,
	struct FE_field **indexer_field,int *number_of_indexed_values);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
If the field is of type INDEXED_FE_FIELD, the indexer_field and
number_of_indexed_values it uses are returned - otherwise an error is reported.
Use function get_FE_field_FE_field_type to determine the field type.
==============================================================================*/

int set_FE_field_type_indexed(struct FE_field *field,
	struct FE_field *indexer_field,int number_of_indexed_values);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type INDEXED_FE_FIELD, indexed by the given
<indexer_field> and with the given <number_of_indexed_values>. The indexer_field
must return a single integer value to be valid.
Allocates and clears the values_storage of the field to fit
field->number_of_components x number_of_indexed_values of the current
value_type. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int get_FE_field_time_FE_value(struct FE_field *field,int number,FE_value *value);
/*******************************************************************************
LAST MODIFIED : 10 June 1999

DESCRIPTION :
Gets the specified global time value for the <field>.
==============================================================================*/

enum Value_type get_FE_field_time_value_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

DESCRIPTION :
Returns the time_value_type of the <field>.
==============================================================================*/

int set_FE_field_time_value_type(struct FE_field *field,
	enum Value_type time_value_type);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

DESCRIPTION :
Sets the time_value_type of the <field>.
Should only call this function for unmanaged fields.
=========================================================================*/

enum Value_type get_FE_field_value_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 19 February 1999

DESCRIPTION :
Returns the value_type of the <field>.
==============================================================================*/

int set_FE_field_value_type(struct FE_field *field,enum Value_type value_type);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Sets the value_type of the <field>. Clears/reallocates the values_storage for
FE_field_types that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but
only if the value_type changes. If function fails the field is left exactly as
it was. Should only call this function for unmanaged fields.
=========================================================================*/

int get_FE_field_max_array_size(struct FE_field *field,
	int *max_number_of_array_values,enum Value_type *value_type);
/*******************************************************************************
LAST MODIFIED : 4 March 1999

DESCRIPTION :
Given the field, search vaules_storage  for the largest array, and return it in
max_number_of_array_values. Return the field value_type.
==============================================================================*/

int get_FE_field_array_attributes(struct FE_field *field, int value_number,
 int *number_of_array_values, enum Value_type *value_type);
/*******************************************************************************
LAST MODIFIED : 4 March 1999

DESCRIPTION :
Get the value_type and the number of array values for the array in
field->values_storage specified by value_number. 
Give an error if field->values_storage isn't storing array types.
==============================================================================*/

int get_FE_field_double_array_value(struct FE_field *field, int value_number,
	double *array, int number_of_array_values);
/*******************************************************************************
LAST MODIFIED : 4 March 1999

DESCRIPTION :
Get the double array in field->values_storage specified by value_number, of
of length number_of_array_values. If number_of_array_values > the stored arrays 
max length, gets the max length.
MUST allocate space for the array before calling this function.

Use get_FE_field_array_attributes() or get_FE_field_max_array_size() 
to get the size of an array.
==============================================================================*/

int set_FE_field_double_array_value(struct FE_field *field, int value_number,
	double *array, int number_of_array_values);
/*******************************************************************************

DESCRIPTION :
Finds any existing double array at the place specified by  value_number in 
field->values_storage.
Frees it.
Allocates a new array, according to number_of_array_values. 
Copies the contents of the passed array to this allocated one.
Copies number of array values, and the pointer to the allocated array to the
specified place in the field->values_storage. 

Therefore, should free the passed array, after passing it to this function

The field value MUST have been previously allocated with
set_FE_field_number_of_values
==============================================================================*/

int get_FE_field_string_value(struct FE_field *field,int value_number,
	char **string);
/*******************************************************************************
LAST MODIFIED : 22 September 1999

DESCRIPTION :
Returns a copy of the string stored at <value_number> in the <field>.
Up to the calling function to DEALLOCATE the returned string.
Returned <*string> may be a valid NULL if that is what is in the field.
==============================================================================*/

int set_FE_field_string_value(struct FE_field *field,int value_number,
	char *string);
/*******************************************************************************
LAST MODIFIED : 22 September 1999

DESCRIPTION :
Copies and sets the <string> stored at <value_number> in the <field>.
<string> may be NULL.
==============================================================================*/

int get_FE_field_element_xi_value(struct FE_field *field,int number,
	struct FE_element **element, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Gets the specified global value for the <field>.
==============================================================================*/

int set_FE_field_element_xi_value(struct FE_field *field,int number,
	struct FE_element *element, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Sets the specified global value for the <field>, to the passed Element and xi
The field value MUST have been previously allocated with
set_FE_field_number_of_values
==============================================================================*/

int get_FE_field_FE_value(struct FE_field *field,int number,FE_value *value);
/*******************************************************************************
LAST MODIFIED : 3 March 1999

DESCRIPTION :
Gets the specified global value for the <field>.
==============================================================================*/

int get_FE_field_FE_value_value(struct FE_field *field,int number,
	FE_value *value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Gets the specified global FE_value <value> from <field>.
==============================================================================*/

int set_FE_field_FE_value_value(struct FE_field *field,int number,
	FE_value value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global FE_value <value> in <field>.
The <field> must be of type FE_VALUE_VALUE to have such values and
<number> must be within the range from get_FE_field_number_of_values.
==============================================================================*/

int get_FE_field_int_value(struct FE_field *field,int number,int *value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Gets the specified global int <value> from <field>.
==============================================================================*/

int set_FE_field_int_value(struct FE_field *field,int number,int value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global int <value> in <field>.
The <field> must be of type INT_VALUE to have such values and
<number> must be within the range from get_FE_field_number_of_values.
==============================================================================*/

int set_FE_field_time_FE_value(struct FE_field *field,int number,
	FE_value value);
/*******************************************************************************
LAST MODIFIED : l0 June 1999

DESCRIPTION :
Sets the specified global time value for the <field>, to the passed FE_value
The field value MUST have been previously allocated with
set_FE_field_number_of_times
==============================================================================*/

char *get_FE_field_name(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 19 February 1999

DESCRIPTION :
Returns a pointer to the name for the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

int set_FE_field_name(struct FE_field *field, char *name);
/*******************************************************************************
LAST MODIFIED : 19 February 1999

DESCRIPTION :
Sets the name of the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

int get_FE_field_CM_field_information(struct FE_field *field,
	struct CM_field_information *cm_field_information);
/*******************************************************************************
LAST MODIFIED : 25 August 1999

DESCRIPTION :
Returns the <cm_field_information> of the <field>.
==============================================================================*/

int set_FE_field_CM_field_information(struct FE_field *field,
	struct CM_field_information *cm_field_information);
/*******************************************************************************
LAST MODIFIED : 25 August 1999

DESCRIPTION :
Sets the <cm_field_information> of the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

PROTOTYPE_GET_OBJECT_NAME_FUNCTION(FE_field_component);

int set_FE_field_component(struct Parse_state *state,void *component_void,
	void *fe_field_manager_void);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Used in command parsing to translate a field component name into an field
component.
???DB.  Should it be here ?
???RC.  Does not ACCESS the field (unlike set_FE_field, above).
==============================================================================*/

int FE_element_get_top_level_xi_number(struct FE_element *element,
	int xi_number);
/*******************************************************************************
LAST MODIFIED : 16 September 1998

DESCRIPTION :
Returns the xi_number of the parent or parent's parent that changes with the
<xi_number> (0..dimension-1) of <element>.  If there is no parent, then
<xi_number> is returned, or 0 if there is an error.
???RC Could stuff up for graphical finite elements if parent is not in same
element_group and xi directions are different for neighbouring elements.
==============================================================================*/

int FE_element_parent_face_of_element_in_group(
	struct FE_element_parent *element_parent,void *face_in_group_data_void);
/*******************************************************************************
LAST MODIFIED : 15 February 1999

DESCRIPTION :
Returns true if the <element_parent> refers to the given <face_number> of the
parent element AND the parent element is also in the <element_group>.
Conditional function for determining if a 1-D or 2-D element is on a particular
face of a 3-D element in the given element_group. Recursive to handle 1-D case.
==============================================================================*/

struct FE_element *FE_element_get_top_level_element_conversion(
	struct FE_element *element,struct FE_element *check_top_level_element,
	struct GROUP(FE_element) *element_group,int face_number,
	FE_value *element_to_top_level);
/*******************************************************************************
LAST MODIFIED : 29 June 1999

DESCRIPTION :
Returns the/a top level [ultimate parent] element for <element>. If supplied,
the function attempts to verify that the <check_top_level_element> is in
fact a valid top_level_element for <element>, otherwise it tries to find one in
the <element_group> and with <element> on its <face_number> (if positive), if
either are specified.

If the returned element is different to <element> (ie. is of higher dimension),
then this function also fills the matrix <element_to_top_level> with values for
converting the xi coordinates in <element> to those in the returned element.
<element_to_top_level> should be preallocated to store at least nine FE_values.

The use of the <element_to_top_level> matrix is similar to <face_to_element> in
FE_element_shape - in fact it is either a copy of it, or calculated from it.
It gives the transformation xi(top_level) = b + A xi(element), where b is in the
first column of the matrix, and the rest of the matrix is A. Its size depends
on the dimension of element:top_level, ie.,
1:2 First 4 values, in form of 2 row X 2 column matrix, used only.
1:3 First 6 values, in form of 3 row X 2 column matrix, used only.
2:3 First 9 values, in form of 3 row X 3 column matrix, used only.
NOTE: recursive to handle 1-D to 3-D case.
==============================================================================*/

int get_FE_element_discretization_from_top_level(struct FE_element *element,
	int *number_in_xi,struct FE_element *top_level_element,
	int *top_level_number_in_xi,FE_value *element_to_top_level);
/*******************************************************************************
LAST MODIFIED : 21 December 1999

DESCRIPTION :
Returns in <number_in_xi> the equivalent discretization of <element> for its
position - element, face or line - in <top_level_element>. Uses
<element_to_top_level> array for line/face conversion as returned by
FE_element_get_top_level_element_conversion.
<number_in_xi> must have space at lease MAXIMUM_ELEMENT_XI_DIMENSIONS integers,
as remaining values up to this size are cleared to zero.
==============================================================================*/

int FE_element_or_parent_contains_node(struct FE_element *element,
	void *node_void);
/*******************************************************************************
LAST MODIFIED : 25 February 1998

DESCRIPTION :
FE_element conditional function returning 1 if <element> or all of its parents
or parent's parents contains <node>.
Routine is used with graphical finite elements to redraw only those elements
affected by a node change when the mesh is edited.
==============================================================================*/

int FE_element_parent_is_exterior(struct FE_element_parent *element_parent,
	void *dummy_user_data);
/*******************************************************************************
LAST MODIFIED : 12 October 1998

DESCRIPTION :
Returns true if <element_parent> is an exterior surface (ie. has exactly one
parent).
==============================================================================*/

int FE_element_parent_has_face_number(struct FE_element_parent *element_parent,
	void *void_face_number);
/*******************************************************************************
LAST MODIFIED : 12 August 1997

DESCRIPTION :
Returns true if <element_parent> (a 2-D surface) has no parents itself, or
it is a face, with the given face_number, of one of its parent.
==============================================================================*/

int FE_element_is_dimension(struct FE_element *element,void *dimension_void);
/*******************************************************************************
LAST MODIFIED : 21 September 1998

DESCRIPTION :
Returns true if <element> is of the given <dimension>.
<dimension_void> must be a pointer to an int containing the dimension.
==============================================================================*/

int FE_element_is_dimension_3(struct FE_element *element,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 1 December 1999

DESCRIPTION :
Returns true if <element> is a 3-D element (ie. not a 2-D face or 1-D line).
==============================================================================*/

int FE_element_is_top_level(struct FE_element *element,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 1 December 1999

DESCRIPTION :
Returns true if <element> is a top-level element - CM_ELEMENT/no parents.
==============================================================================*/

int FE_element_to_element_string(struct FE_element *element,char **name_ptr);
/*******************************************************************************
LAST MODIFIED : 18 August 1998

DESCRIPTION :
Writes the cmiss.element_number of <element> into a newly allocated string and
points <*name_ptr> at it.
==============================================================================*/

struct FE_element *element_string_to_FE_element(char *name,
	struct MANAGER(FE_element) *element_manager);
/*******************************************************************************
LAST MODIFIED : 18 August 1998

DESCRIPTION :
Converts the <name> into an element number (face and line number zero), finds
and returns the element in the <element_manager> with that cmiss identifier.
==============================================================================*/

int set_FE_element_dimension_3(struct Parse_state *state,
	void *element_address_void,void *element_manager_void);
/*******************************************************************************
LAST MODIFIED : 18 August 1998

DESCRIPTION :
A modifier function for specifying a 3-D element (as opposed to a 2-D face or
1-D line number), used (eg.) to set the seed element for a volume texture.
==============================================================================*/

int set_FE_element_top_level(struct Parse_state *state,
	void *element_address_void,void *element_manager_void);
/*******************************************************************************
LAST MODIFIED : 16 June 1999

DESCRIPTION :
A modifier function for specifying a top level element
, used (eg.) to set the seed element for a xi_texture_coordinate computed_field.
==============================================================================*/

int set_FE_element(struct Parse_state *state,
	void *element_address_void,void *element_manager_void);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
A modifier function for specifying an element.
==============================================================================*/

int ensure_FE_element_and_faces_are_in_group(struct FE_element *element,
	void *element_group_void);
/*******************************************************************************
LAST MODIFIED : 21 April 1999

DESCRIPTION :
Iterator function which, if <element> is top-level (ie. cm.type is CM_ELEMENT),
adds it and all its faces to the <element_group> if not currently in it.
Note: If function is to be called several times on a group then surround by
MANAGED_GROUP_BEGIN_CACHE/END_CACHE calls for efficient manager messages.
==============================================================================*/

int ensure_FE_element_and_faces_are_not_in_group(struct FE_element *element,
	void *element_group_void);
/*******************************************************************************
LAST MODIFIED : 21 April 1999

DESCRIPTION :
Iterator function which, if <element> is top-level (ie. cm.type is CM_ELEMENT),
removes it and all its faces from the <element_group> if currently in it.
Note: If function is to be called several times on a group then surround by
MANAGED_GROUP_BEGIN_CACHE/END_CACHE calls for efficient manager messages.
==============================================================================*/

int ensure_top_level_FE_element_is_in_list(struct FE_element *element,
	void *element_list_void);
/*******************************************************************************
LAST MODIFIED : 28 April 1999

DESCRIPTION :
Iterator function which, if <element> is top-level (ie. cm.type is CM_ELEMENT),
adds it to the <element_list> if not currently in it.
==============================================================================*/

int ensure_top_level_FE_element_nodes_are_in_list(struct FE_element *element,
	void *node_list_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function which, if <element> is top-level (ie. cm.type is CM_ELEMENT),
ensures all its nodes are added to the <node_list> if not currently in it.
==============================================================================*/

int ensure_top_level_FE_element_nodes_are_not_in_list(
	struct FE_element *element,void *node_list_void);
/*******************************************************************************
LAST MODIFIED : 20 April 1999

DESCRIPTION :
Iterator function which, if <element> is top-level (ie. cm.type is CM_ELEMENT),
ensures none of its nodes are in <node_list>.
==============================================================================*/

int FE_element_or_parent_has_field(struct FE_element *element,
	struct FE_field *field,struct GROUP(FE_element) *element_group);
/*******************************************************************************
LAST MODIFIED : 1 April 1999

DESCRIPTION :
Returns true if the <element> or any of its parents has the <field> defined
over it. By supplying an <element_group> this limits the test to elements that
are also in the group.
==============================================================================*/

#if defined (OLD_CODE)
int FE_node_get_field_components(struct FE_node *node,
	struct FE_field *field,int component_no,FE_value **components);
/*******************************************************************************
LAST MODIFIED : 25 August 1998

DESCRIPTION :
Returns the values - not derivatives - held for the given <component_no> of
<field>, or all components if <component_no> is negative. The return value of
the function is the number of components returned, while <*components> will be
set to point to an array containing the values, which the calling function must
deallocate.
???RC Does not handle multiple versions.
==============================================================================*/
#endif /* defined (OLD_CODE) */

struct FE_field *FE_node_get_position_cartesian(struct FE_node *node,
	struct FE_field *coordinate_field,FE_value *node_x,FE_value *node_y,
	FE_value *node_z,FE_value *coordinate_jacobian);
/*******************************************************************************
LAST MODIFIED : 2 November 1998

DESCRIPTION :
Evaluates the supplied coordinate_field (or the first one in the node if NULL).
Sets non-present components to zero (eg if only had x and y, z would be set to
zero).  Converts to rectangular Cartesian coordinates: x,y,z.  If
<coordinate_jacobian>, then its is filled with the Jacobian for the
transformation from native coordinates to rectangular Cartesian.  Returns the
field it actually calculated.
???RC Does not handle multiple versions.
==============================================================================*/

int FE_node_set_position_cartesian(struct FE_node *node,
	struct FE_field *coordinate_field,
	FE_value node_x,FE_value node_y,FE_value node_z);
/*******************************************************************************
LAST MODIFIED : 28 April 1998

DESCRIPTION :
Sets the position of <node> in Cartesian coordinates: x[,y[,z]] using the
supplied coordinate_field (or the first one in the node if NULL). The given
Cartesian coordinates are converted into the coordinate system of the node
for the coordinate_field used.
==============================================================================*/

int FE_field_is_coordinate_field(struct FE_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 10 September 1998

DESCRIPTION :
Returns true if the field is a coordinate field.
==============================================================================*/

int FE_field_is_anatomical_fibre_field(struct FE_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 16 July 1998

DESCRIPTION :
Returns true if the field is of type ANATOMICAL and it has a FIBRE coordinate
system. Used with FIRST_OBJECT_IN_LIST_THAT(FE_field) to get any fibre field
that may be defined over a group of elements.
==============================================================================*/

int FE_field_is_defined_at_node(struct FE_field *field, struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 4 May 1999

DESCRIPTION :
Returns true if the <field> is defined for the <node>.
==============================================================================*/

int FE_field_is_defined_in_element(struct FE_field *field,
	struct FE_element *element);
/*******************************************************************************
LAST MODIFIED : 13 May 1999

DESCRIPTION :
Returns true if the <field> is defined for the <element>.
==============================================================================*/

int FE_element_field_is_grid_based(struct FE_element *element,
	struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 5 October 1999 

DESCRIPTION :
Returns true if <field> is grid-based in <element>. Only checks the first
component since we assume all subsequent components have the same basis and
numbers of grid cells in xi.
Returns 0 with no error if <field> is not defined over element or not element-
based in it.
==============================================================================*/

int get_FE_element_field_grid_map_number_in_xi(struct FE_element *element,
	struct FE_field *field,int *number_in_xi);
/*******************************************************************************
LAST MODIFIED : 5 October 1999 

DESCRIPTION :
If <field> is grid-based in <element>, returns in <number_in_xi> the numbers of
finite difference cells in each xi-direction of <element>. Note that this number
is one less than the number of grid points in each direction. <number_in_xi>
should be allocated with at least as much space as the number of dimensions in
<element>, but is assumed to have no more than MAXIMUM_ELEMENT_XI_DIMENSIONS so
that int number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS] can be passed to this
function.
==============================================================================*/

int get_FE_element_field_number_of_grid_values(struct FE_element *element,
	struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 12 October 1999 

DESCRIPTION :
If <field> is grid-based in <element>, returns the total number of grid points
at which data is stored for <field>, equal to product of <number_in_xi>+1 in
all directions. Returns 0 without error for non grid-based fields.
==============================================================================*/

int get_FE_element_field_component_grid_FE_value_values(
	struct FE_element *element,struct FE_field *field,int component_number,
	FE_value **values);
/*******************************************************************************
LAST MODIFIED : 12 October 1999

DESCRIPTION :
If <field> is grid-based in <element>, returns an allocated array of the grid
values stored for <component_number>. To get number of values returned, call
get_FE_element_field_number_of_grid_values; Grids change in xi0 fastest.
It is up to the calling function to DEALLOCATE the returned values.
==============================================================================*/

int set_FE_element_field_component_grid_FE_value_values(
	struct FE_element *element,struct FE_field *field,int component_number,
	FE_value *values);
/*******************************************************************************
LAST MODIFIED : 12 October 1999

DESCRIPTION :
If <field> is grid-based in <element>, copies <values> into the values storage
for <component_number>. To get number of values to pass, call
get_FE_element_field_number_of_grid_values; Grids change in xi0 fastest.
==============================================================================*/

int get_FE_element_field_component_grid_int_values(struct FE_element *element,
	struct FE_field *field,int component_number,int **values);
/*******************************************************************************
LAST MODIFIED : 14 October 1999

DESCRIPTION :
If <field> is grid-based in <element>, returns an allocated array of the grid
values stored for <component_number>. To get number of values returned, call
get_FE_element_field_number_of_grid_values; Grids change in xi0 fastest.
It is up to the calling function to DEALLOCATE the returned values.
==============================================================================*/

int set_FE_element_field_component_grid_int_values(struct FE_element *element,
	struct FE_field *field,int component_number,int *values);
/*******************************************************************************
LAST MODIFIED : 14 October 1999

DESCRIPTION :
If <field> is grid-based in <element>, copies <values> into the values storage
for <component_number>. To get number of values to pass, call
get_FE_element_field_number_of_grid_values; Grids change in xi0 fastest.
==============================================================================*/

int add_node_to_smoothed_node_lists(struct FE_node *node,
	int component_number,FE_value *derivative,
	struct Smooth_field_over_element_data *smooth_field_over_element_data);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
???DB.  Came from command.c .  Knows too much about nodes/elements to stay there
==============================================================================*/

int smooth_field_over_node(struct FE_node *node,
	void *void_smooth_field_over_node_data);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
???DB.  Came from command.c .  Knows too much about nodes/elements to stay there
==============================================================================*/

int smooth_field_over_element(struct FE_element *element,
	void *void_smooth_field_over_element_data);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
???DB.  Needs to be extended to use get_nodal_value, but what about scale
	factors ?
???DB.  Came from command.c .  Knows too much about nodes/elements to stay there
==============================================================================*/

PROTOTYPE_COPY_OBJECT_FUNCTION(CM_field_information);

enum CM_field_type get_CM_field_information_type(
	struct CM_field_information *field_info);
/*******************************************************************************
LAST MODIFIED : 4 February 1999

DESCRIPTION :
gets the name type and indice of field_info.
==============================================================================*/

int set_CM_field_information(struct CM_field_information *field_info,
	enum CM_field_type type, int *indices);
/*******************************************************************************
LAST MODIFIED : 4 February 1999

DESCRIPTION :
Sets the type and indices (if any) of field_info. Any inidces passed as a 
pointer to an array.
==============================================================================*/

int compare_CM_field_information(struct CM_field_information *field_info1,
	struct CM_field_information *field_info2);
/*******************************************************************************
LAST MODIFIED: 4 February 1999

DESCRIPTION:
Compares the CM_Field_information structures, field_info1 and field_info2.
==============================================================================*/

int FE_element_field_is_type_CM_coordinate(
	struct FE_element_field *element_field, void *dummy);
/*******************************************************************************
LAST MODIFIED: 11 February 1999

DESCRIPTION:
returns true if <element_field> has a field of type CM_coordinate
Not static as used in command/cmiss.c
==============================================================================*/

int FE_element_field_is_type_CM_anatomical(
	struct FE_element_field *element_field, void *dummy);
/*******************************************************************************
LAST MODIFIED: 11 February 1999

DESCRIPTION:
returns true if <element_field> has a field of type CM_anatomical
Not static as used in projection/projection.c
==============================================================================*/

struct FE_element *adjacent_FE_element(struct FE_element *element,
	int face_number);
/*******************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION :
Returns the element which shares the face given.
==============================================================================*/

int FE_element_shape_find_face_number_for_xi(struct FE_element_shape *shape, 
	FE_value *xi, int *face_number);
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
==============================================================================*/

struct GROUP(FE_node) *make_node_and_element_and_data_groups(
	struct MANAGER(GROUP(FE_node)) *node_group_manager,
	struct MANAGER(FE_node) *node_manager,
	struct MANAGER(FE_element) *element_manager,
	struct MANAGER(GROUP(FE_element))	*element_group_manager,
	struct MANAGER(GROUP(FE_node)) *data_group_manager,char *group_name);
/*******************************************************************************
LAST MODIFIED : 18 May 1999

DESCRIPTION :
Makes a node group ,and a corresponding element group and data group using the
supplied group_name.

This function is used for creating node group such that the nodes will be
visible in the 3D window/graphical element editor.

The node group is returned. The node group is cached, with
MANAGED_GROUP_BEGIN_CACHE(FE_node).  The caching must be ended by the calling
function.
==============================================================================*/

struct FE_field_order_info *CREATE(FE_field_order_info)(int number_of_fields);
/*******************************************************************************
LAST MODIFIED : 16 March 1999

Allocate space for an array of pointers to fields of length number_of_field, 
set these to NULL, copy the number-of_fields. 
==============================================================================*/

int DESTROY(FE_field_order_info)(
	struct FE_field_order_info **field_order_info_address);
/*******************************************************************************
LAST MODIFIED : 16 March 1999

Frees memory for FE_field_order_info
==============================================================================*/

int define_node_field_and_field_order_info(struct FE_node *node,
	struct FE_field *field,int *number_of_derivatives,int *number_of_versions,
	int *field_number,int number_of_fields,
	enum FE_nodal_value_type **nodal_value_types,
	struct FE_field_order_info *field_order_info);
/*******************************************************************************
LAST MODIFIED : 21 May 1999

DESCRIPTION :
Helper function for create_config_template_node() and
create_mapping_template_node() that, given the node,field and
field_order_info, defines the field at the node, and places it in the
field_order_info
==============================================================================*/

int get_FE_field_order_info_number_of_fields(
	struct FE_field_order_info *field_order_info);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Gets the <field_order_info> number_of_fields
==============================================================================*/

struct FE_field *get_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info,int field_number);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Gets the <field_order_info> field at the specified field_number
==============================================================================*/

int set_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info,int field_number,
	struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Sets the <field_order_info> field at the specified field_number
==============================================================================*/

struct FE_node_order_info *CREATE(FE_node_order_info)(
	int number_of_nodes);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Allocate space for an array of pointers to nodes of length number_of_nodes, 
set these to NULL, copy the number_of_nodes. 
==============================================================================*/

int DESTROY(FE_node_order_info)(
	struct FE_node_order_info **node_order_info_address);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Frees them memory used by node_order_info.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_node_order_info);
PROTOTYPE_COPY_OBJECT_FUNCTION(FE_node_order_info);

int get_FE_node_order_info_number_of_nodes(
	struct FE_node_order_info *node_order_info);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Gets the <node_order_info> number_of_nodes
==============================================================================*/

struct FE_node *get_FE_node_order_info_node(
	struct FE_node_order_info *node_order_info,int node_number);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Gets the <node_order_info> node at the specified node_number
==============================================================================*/

int set_FE_node_order_info_node(
	struct FE_node_order_info *node_order_info,int node_number,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION : 
Sets the <node_order_info> node at the specified node_number
==============================================================================*/

int add_nodes_FE_node_order_info(int number_of_nodes_to_add,
	struct FE_node_order_info *node_order_info);
/*******************************************************************************
LAST MODIFIED : 6 July 1999

DESCRIPTION :
As FE_node to previously created FE_node_order_info 
==============================================================================*/

int fill_FE_node_order_info(struct FE_node *node,void *dummy);
/*******************************************************************************
LAST MODIFIED : 6 July 1999

DESCRIPTION :
As FE_node to previously created FE_node_order_info (passed in dummy)
==============================================================================*/

int get_FE_node_group_access_count(struct GROUP(FE_node) *node_group);
/*******************************************************************************
LAST MODIFIED : 17 August 1999

DESCRIPTION :Debug function. May be naughty.
==============================================================================*/

int get_FE_field_access_count(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 17 August 1999

DESCRIPTION :Debug function. May be naughty.
==============================================================================*/

int free_node_and_element_and_data_groups(struct GROUP(FE_node) *node_group,
	struct MANAGER(FE_element) *element_manager,
	struct MANAGER(GROUP(FE_element))	*element_group_manager,
	struct MANAGER(FE_node) *node_manager,
	struct MANAGER(GROUP(FE_node)) *data_group_manager,
	struct MANAGER(GROUP(FE_node)) *node_group_manager );
/*******************************************************************************
LAST MODIFIED : August 27 1999

DESCRIPTION :
Given a node group, frees it and it's asscoiated element and data groups
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_H) */
