/*******************************************************************************
FILE : delaunay.c

LAST MODIFIED : 20 April 2004

DESCRIPTION :
Specialized implementations of Delaunay triangulation for a cylinder and a
sphere.

???DB.  Do as 3-D instead.  Then would be general.  How to get convex hull?

???DB.  22 April 2002
1.  Does scaling change triangulation for cylinder?
==============================================================================*/

/*#define DEBUG*/

#include <stddef.h>
#if defined (DEBUG)
/*???debug */
#include <stdio.h>
#endif /* defined (DEBUG) */
#include <math.h>
#include "general/debug.h"
#include "general/geometry.h"
#include "unemap/delaunay.h"
#include "user_interface/message.h"

/*
Module constants
----------------
*/
#define SPHERE_DELAUNAY_TOLERANCE 0.0001

/*
Module types
------------
*/
struct Delaunay_triangle
/*******************************************************************************
LAST MODIFIED : 15 April 2004

DESCRIPTION :
cylinder.  Uses normalized theta and z for the vertex and centre coordinates.
sphere.  Uses longitude (theta) and latitude (mu) for the vertex and centre
	coordinates.
plane.  Uses x and y for vertex and centre coordinates.
???DB.  Vertex order is important for sphere (see sphere_calculate_circumcentre)
	Can order be used for cylinder?
==============================================================================*/
{
	float centre[2],radius2,vertices[6];
	int vertex_numbers[3];
}; /* struct Delaunay_triangle */

/*
Module functions
----------------
*/
static int cylinder_normalize_vertex(float *xyz,float minimum_z,float maximum_z,
	float minimum_u0,float minimum_u1,float *uv)
/*******************************************************************************
LAST MODIFIED : 7 October 2000

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(cylinder_normalize_vertex);
	return_code=0;
	if (xyz&&(minimum_z<maximum_z)&&uv)
	{
		uv[0]=(float)atan2(xyz[1],xyz[0])/(2*PI);
		uv[1]=(xyz[2]-minimum_z)/(maximum_z-minimum_z);
		uv[0] -= (1-uv[1])*minimum_u0+uv[1]*minimum_u1;
		if (uv[0]<0)
		{
			uv[0] += 1;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cylinder_normalize_vertex.  "
			"Invalid argument(s).  %p %g %g %p",xyz,minimum_z,maximum_z,uv);
	}
	LEAVE;

	return (return_code);
} /* cylinder_normalize_vertex */

static int sphere_normalize(float *xyz,float *normalized_xyz)
/*******************************************************************************
LAST MODIFIED : 24 April 2002

DESCRIPTION :
Projects a point, <xyz>, onto the surface of a unit sphere, <normalized_xyz>.
A nonzero return code indicates an error.
==============================================================================*/
{
	float radius;
	int return_code;

	ENTER(sphere_normalize);
	return_code=0;
	if (xyz&&normalized_xyz)
	{
		radius=(float)sqrt(xyz[0]*xyz[0]+xyz[1]*xyz[1]+xyz[2]*xyz[2]);
		if (radius>0)
		{
			normalized_xyz[0]=xyz[0]/radius;
			normalized_xyz[1]=xyz[1]/radius;
			normalized_xyz[2]=xyz[2]/radius;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"sphere_normalize.  "
				"Point is at origin.  %g %g %g\n",xyz[0],xyz[1],xyz[3]);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"sphere_normalize.  "
			"Invalid argument(s).  %p %p",xyz,normalized_xyz);
	}
	LEAVE;

	return (return_code);
} /* sphere_normalize */

static int cylinder_calculate_circumcentre(struct Delaunay_triangle *triangle)
/*******************************************************************************
LAST MODIFIED : 19 April 2004

DESCRIPTION :
The circumcentre is the intersection of the bisectors.
==============================================================================*/
{
	float s,temp_u,temp_v,*vertices;
	int return_code;

	ENTER(cylinder_calculate_circumcentre);
	return_code=0;
	if (triangle)
	{
		vertices=triangle->vertices;
		s=(vertices[1]-vertices[5])*(vertices[2]-vertices[0])-
			(vertices[1]-vertices[3])*(vertices[4]-vertices[0]);
		if (s!=0)
		{
			s=((vertices[2]-vertices[4])*(vertices[2]-vertices[0])-
				(vertices[1]-vertices[3])*(vertices[3]-vertices[5]))/s;
			(triangle->centre)[0]=
				((vertices[0]+vertices[4])+s*(vertices[1]-vertices[5]))/2;
			(triangle->centre)[1]=
				((vertices[1]+vertices[5])+s*(vertices[4]-vertices[0]))/2;
			temp_u=(triangle->centre)[0]-vertices[0];
			temp_v=(triangle->centre)[1]-vertices[1];
			triangle->radius2=temp_u*temp_u+temp_v*temp_v;
			return_code=1;
		}
#if defined (OLD_CODE)
		else
		{
			display_message(ERROR_MESSAGE,"cylinder_calculate_circumcentre.  "
				"Invalid triangle.  %g %g %g %g %g %g",vertices[0],vertices[1],
				vertices[2],vertices[3],vertices[4],vertices[5]);
			return_code=0;
		}
#endif /* defined (OLD_CODE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"cylinder_calculate_circumcentre.  "
			"Invalid argument");
	}
	LEAVE;

	return (return_code);
} /* cylinder_calculate_circumcentre */

static int sphere_calculate_distance(float *normalized_xyz_1,
	float *normalized_xyz_2,float *distance_address)
/*******************************************************************************
LAST MODIFIED : 26 April 2002

DESCRIPTION :
Calculates the distance between two points that lie on the unit sphere.  The
distance between them is the shorter distance along the great circle through
them.  A nonzero return code indicates an error.
==============================================================================*/
{
	float distance;
	int return_code;

	ENTER(sphere_calculate_distance);
	distance=0;
	if (normalized_xyz_1&&normalized_xyz_2)
	{
		distance=normalized_xyz_1[0]*normalized_xyz_2[0]+
			normalized_xyz_1[1]*normalized_xyz_2[1]+
			normalized_xyz_1[2]*normalized_xyz_2[2];
		if (distance< -1)
		{
			distance=PI;
		}
		else
		{
			if (distance>1)
			{
				distance=0;
			}
			else
			{
				distance=acos(distance);
			}
		}
		*distance_address=distance;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"sphere_calculate_distance.  "
			"Invalid argument(s).  %p %p",normalized_xyz_1,normalized_xyz_2);
	}
	LEAVE;

	return (return_code);
} /* sphere_calculate_distance */

static int sphere_calculate_circumcentre(float *normalized_xyz_1,
	float *normalized_xyz_2,float *normalized_xyz_3,float *normalized_xyz_centre)
/*******************************************************************************
LAST MODIFIED : 22 April 2002

DESCRIPTION :
For any 3 points there are two choices for the centre.  The straight line in
3-space between the two points goes through the centre of the sphere.  One is
chosen based on the order of the vertices.  A nonzero return code indicates an
error.
==============================================================================*/
{
	float x12,x13,y12,y13,z12,z13;
	int return_code;

	ENTER(sphere_calculate_circumcentre);
	return_code=0;
	if (normalized_xyz_1&&normalized_xyz_2&&normalized_xyz_3&&
		normalized_xyz_centre)
	{
		x12=normalized_xyz_1[0]-normalized_xyz_2[0];
		y12=normalized_xyz_1[1]-normalized_xyz_2[1];
		z12=normalized_xyz_1[2]-normalized_xyz_2[2];
		x13=normalized_xyz_1[0]-normalized_xyz_3[0];
		y13=normalized_xyz_1[1]-normalized_xyz_3[1];
		z13=normalized_xyz_1[2]-normalized_xyz_3[2];
		normalized_xyz_centre[0]=y12*z13-z12*y13;
		normalized_xyz_centre[1]=z12*x13-x12*z13;
		normalized_xyz_centre[2]=x12*y13-y12*x13;
		return_code=sphere_normalize(normalized_xyz_centre,normalized_xyz_centre);
	}
	else
	{
		display_message(ERROR_MESSAGE,"sphere_calculate_circumcentre.  "
			"Invalid argument(s).  %p %p %p %p",normalized_xyz_1,normalized_xyz_2,
			normalized_xyz_3,normalized_xyz_centre);
	}
	LEAVE;

	return (return_code);
} /* sphere_calculate_circumcentre */

/*
Global functions
----------------
*/
int cylinder_delaunay(int number_of_vertices,float *vertices,
	int *number_of_triangles_address,int **triangles_address)
/*******************************************************************************
LAST MODIFIED : 3 February 2002

DESCRIPTION :
Calculates the Delaunay triangulation of the <vertices> on a cylinder whose axis
is z.
<number_of_vertices> is the number of vertices to be triangulated
<vertices> is an array of length 3*<number_of_vertices> containing the x,y,z
	coordinates of the vertices
<*number_of_triangles_address> is the number of triangles calculated by the
	function
<*triangles_address> is an array of length 3*<*number_of_triangles_address>,
	allocated by the function, containing the vertex numbers for each triangle
==============================================================================*/
{
	float *adjacent_angles,*adjacent_angle,*adjacent_uv,*adjacent_uvs,angle,
		maximum_z,minimum_u0,minimum_u1,minimum_z,temp_u,temp_v,uv[2],*vertex;
	int *adjacent_vertices,*adjacent_vertex,i,ii,j,k,l,m,maximum_vertex,
		minimum_vertex,n,number_of_adjacent_vertices,number_of_returned_triangles,
		number_of_triangles,return_code,*returned_triangles,vertex_number;
	struct Delaunay_triangle **temp_triangles,*triangle,**triangles;

	ENTER(cylinder_delaunay);
	return_code=0;
	/* check the arguments */
	if ((1<number_of_vertices)&&vertices&&number_of_triangles_address&&
		triangles_address)
	{
		/* find the vertex with minimum z and the vertex with maximum z */
		vertex=vertices+2;
		minimum_vertex=0;
		maximum_vertex=0;
		minimum_z= *vertex;
		maximum_z=minimum_z;
		for (i=1;i<number_of_vertices;i++)
		{
			vertex += 3;
			if (*vertex<minimum_z)
			{
				minimum_vertex=i;
				minimum_z= *vertex;
			}
			else
			{
				if (*vertex>maximum_z)
				{
					maximum_vertex=i;
					maximum_z= *vertex;
				}
			}
		}
		if (minimum_z<maximum_z)
		{
			minimum_u0=0;
			minimum_u1=0;
			cylinder_normalize_vertex(vertices+(3*minimum_vertex),minimum_z,maximum_z,
				minimum_u0,minimum_u1,uv);
			minimum_u0=uv[0];
			cylinder_normalize_vertex(vertices+(3*maximum_vertex),minimum_z,maximum_z,
				minimum_u0,minimum_u1,uv);
			minimum_u1=uv[0];
#if defined (DEBUG)
			/*???debug */
			printf("minimum %g %g (%d), maximum %g %g (%d)\n",minimum_u0,minimum_z,
				minimum_vertex,minimum_u1,maximum_z,maximum_vertex);
#endif /* defined (DEBUG) */
			number_of_triangles=6;
			if (ALLOCATE(triangles,struct Delaunay_triangle *,number_of_triangles))
			{
				i=number_of_triangles;
				do
				{
					i--;
				}
				while ((i>=0)&&ALLOCATE(triangles[i],struct Delaunay_triangle,1));
				if (i<0)
				{
					(triangles[0]->vertex_numbers)[0]=minimum_vertex-number_of_vertices;
					(triangles[0]->vertices)[0]= -1;
					(triangles[0]->vertices)[1]=0;
					(triangles[0]->vertex_numbers)[1]=maximum_vertex-number_of_vertices;
					(triangles[0]->vertices)[2]= -1;
					(triangles[0]->vertices)[3]=1;
					(triangles[0]->vertex_numbers)[2]=minimum_vertex;
					(triangles[0]->vertices)[4]=0;
					(triangles[0]->vertices)[5]=0;
					(triangles[1]->vertex_numbers)[0]=maximum_vertex-number_of_vertices;
					(triangles[1]->vertices)[0]= -1;
					(triangles[1]->vertices)[1]=1;
					(triangles[1]->vertex_numbers)[1]=maximum_vertex;
					(triangles[1]->vertices)[2]=0;
					(triangles[1]->vertices)[3]=1;
					(triangles[1]->vertex_numbers)[2]=minimum_vertex;
					(triangles[1]->vertices)[4]=0;
					(triangles[1]->vertices)[5]=0;
					(triangles[2]->vertex_numbers)[0]=minimum_vertex;
					(triangles[2]->vertices)[0]=0;
					(triangles[2]->vertices)[1]=0;
					(triangles[2]->vertex_numbers)[1]=maximum_vertex;
					(triangles[2]->vertices)[2]=0;
					(triangles[2]->vertices)[3]=1;
					(triangles[2]->vertex_numbers)[2]=minimum_vertex+number_of_vertices;
					(triangles[2]->vertices)[4]=1;
					(triangles[2]->vertices)[5]=0;
					(triangles[3]->vertex_numbers)[0]=maximum_vertex;
					(triangles[3]->vertices)[0]=0;
					(triangles[3]->vertices)[1]=1;
					(triangles[3]->vertex_numbers)[1]=maximum_vertex+number_of_vertices;
					(triangles[3]->vertices)[2]=1;
					(triangles[3]->vertices)[3]=1;
					(triangles[3]->vertex_numbers)[2]=minimum_vertex+number_of_vertices;
					(triangles[3]->vertices)[4]=1;
					(triangles[3]->vertices)[5]=0;
					(triangles[4]->vertex_numbers)[0]=minimum_vertex+number_of_vertices;
					(triangles[4]->vertices)[0]=1;
					(triangles[4]->vertices)[1]=0;
					(triangles[4]->vertex_numbers)[1]=maximum_vertex+number_of_vertices;
					(triangles[4]->vertices)[2]=1;
					(triangles[4]->vertices)[3]=1;
					(triangles[4]->vertex_numbers)[2]=minimum_vertex+2*number_of_vertices;
					(triangles[4]->vertices)[4]=2;
					(triangles[4]->vertices)[5]=0;
					(triangles[5]->vertex_numbers)[0]=maximum_vertex+number_of_vertices;
					(triangles[5]->vertices)[0]=1;
					(triangles[5]->vertices)[1]=1;
					(triangles[5]->vertex_numbers)[1]=maximum_vertex+2*number_of_vertices;
					(triangles[5]->vertices)[2]=2;
					(triangles[5]->vertices)[3]=1;
					(triangles[5]->vertex_numbers)[2]=minimum_vertex+2*number_of_vertices;
					(triangles[5]->vertices)[4]=2;
					(triangles[5]->vertices)[5]=0;
					for (i=0;i<number_of_triangles;i++)
					{
						cylinder_calculate_circumcentre(triangles[i]);
					}
					return_code=1;
					ii=0;
					vertex=vertices;
					while (return_code&&(ii<2*number_of_vertices))
					{
						i=ii/2;
						if ((i!=minimum_vertex)&&(i!=maximum_vertex))
						{
#if defined (DEBUG)
							/*???debug */
							printf("number_of_triangles=%d\n",number_of_triangles);
							for (l=0;l<number_of_triangles;l++)
							{
								printf("  triangle %d  %g %g  %g\n",l,triangles[l]->centre[0],
									triangles[l]->centre[1],triangles[l]->radius2);
								printf("    %d  %g %g\n",triangles[l]->vertex_numbers[0],
									(triangles[l]->vertices)[0],(triangles[l]->vertices)[1]);
								printf("    %d  %g %g\n",triangles[l]->vertex_numbers[1],
									(triangles[l]->vertices)[2],(triangles[l]->vertices)[3]);
								printf("    %d  %g %g\n",triangles[l]->vertex_numbers[2],
									(triangles[l]->vertices)[4],(triangles[l]->vertices)[5]);
							}
#endif /* defined (DEBUG) */
							cylinder_normalize_vertex(vertex,minimum_z,maximum_z,minimum_u0,
								minimum_u1,uv);
							if (1==ii%2)
							{
								if (uv[0]<0.5)
								{
									uv[0] += 1;
									i += number_of_vertices;
								}
								else
								{
									uv[0] -= 1;
									i -= number_of_vertices;
								}
							}
#if defined (DEBUG)
							/*???debug */
							printf("vertex %d %g %g %g  %g %g\n",i,vertex[0],vertex[1],
								vertex[2],uv[0],uv[1]);
#endif /* defined (DEBUG) */
							/* delete the triangles that no longer satisfy the in-circle
								criterion */
							j=0;
							k=0;
							number_of_adjacent_vertices=0;
							adjacent_vertices=(int *)NULL;
							adjacent_angles=(float *)NULL;
							adjacent_uvs=(float *)NULL;
							while (return_code&&(j<number_of_triangles))
							{
								temp_u=uv[0]-(triangles[j]->centre)[0];
								temp_v=uv[1]-(triangles[j]->centre)[1];
								if (temp_u*temp_u+temp_v*temp_v<=triangles[j]->radius2)
								{
									k++;
									/* add the vertices to the list of adjacent vertices,
										ordering the adjacent vertices so that they go
										anti-clockwise around the new vertex */
									REALLOCATE(adjacent_vertex,adjacent_vertices,int,
										number_of_adjacent_vertices+4);
									REALLOCATE(adjacent_angle,adjacent_angles,float,
										number_of_adjacent_vertices+3);
									REALLOCATE(adjacent_uv,adjacent_uvs,float,
										2*number_of_adjacent_vertices+8);
									if (adjacent_vertex&&adjacent_angle&&adjacent_uv)
									{
										adjacent_vertices=adjacent_vertex;
										adjacent_angles=adjacent_angle;
										adjacent_uvs=adjacent_uv;
										for (l=0;l<3;l++)
										{
											m=0;
											vertex_number=(triangles[j]->vertex_numbers)[l];
											angle=(triangles[j]->vertices)[2*l]-uv[0];
											angle=atan2((triangles[j]->vertices)[2*l+1]-uv[1],angle);
											while ((m<number_of_adjacent_vertices)&&
												(vertex_number!=adjacent_vertices[m])&&
												(angle<adjacent_angles[m]))
											{
												m++;
											}
											if ((m>=number_of_adjacent_vertices)||
												(vertex_number!=adjacent_vertices[m]))
											{
												adjacent_vertex=adjacent_vertices+
													number_of_adjacent_vertices;
												adjacent_angle=adjacent_angles+
													number_of_adjacent_vertices;
												adjacent_uv=adjacent_uvs+
													2*number_of_adjacent_vertices;
												for (n=number_of_adjacent_vertices;n>m;n--)
												{
													adjacent_vertex--;
													adjacent_vertex[1]=adjacent_vertex[0];
													adjacent_angle--;
													adjacent_angle[1]=adjacent_angle[0];
													adjacent_uv--;
													adjacent_uv[2]=adjacent_uv[0];
													adjacent_uv--;
													adjacent_uv[2]=adjacent_uv[0];
												}
												*adjacent_vertex=vertex_number;
												*adjacent_angle=angle;
												*adjacent_uv=(triangles[j]->vertices)[2*l];
												adjacent_uv++;
												*adjacent_uv=(triangles[j]->vertices)[2*l+1];
												number_of_adjacent_vertices++;
#if defined (DEBUG)
												/*???debug */
												printf("adjacent vertices\n");
												for (n=0;n<number_of_adjacent_vertices;n++)
												{
													printf("  %d.  %g %g  %g\n",adjacent_vertices[n],
														adjacent_uvs[2*n],adjacent_uvs[2*n+1],
														adjacent_angles[n]);
												}
#endif /* defined (DEBUG) */
											}
										}
									}
									else
									{
										if (adjacent_vertex)
										{
											adjacent_vertices=adjacent_vertex;
										}
										if (adjacent_angle)
										{
											adjacent_angles=adjacent_angle;
										}
										if (adjacent_uv)
										{
											adjacent_uvs=adjacent_uv;
										}
										return_code=0;
										display_message(ERROR_MESSAGE,"cylinder_delaunay.  "
											"Could not reallocate adjacent vertices");
									}
									DEALLOCATE(triangles[j]);
								}
								else
								{
									if (k>0)
									{
										triangles[j-k]=triangles[j];
									}
								}
								j++;
							}
							if (return_code&&(k>0))
							{
								number_of_triangles -= k;;
								/* determine new triangles */
								if (REALLOCATE(temp_triangles,triangles,
									struct Delaunay_triangle *,
									number_of_triangles+number_of_adjacent_vertices))
								{
									triangles=temp_triangles;
									adjacent_vertex=adjacent_vertices;
									adjacent_angle=adjacent_angles;
									adjacent_uv=adjacent_uvs;
									adjacent_vertices[number_of_adjacent_vertices]=
										adjacent_vertices[0];
									adjacent_uvs[2*number_of_adjacent_vertices]=adjacent_uvs[0];
									adjacent_uvs[2*number_of_adjacent_vertices+1]=adjacent_uvs[1];
									k=0;
									while (return_code&&(k<number_of_adjacent_vertices))
									{
										if (triangle=ALLOCATE(triangles[number_of_triangles],
											struct Delaunay_triangle,1))
										{
											triangle->vertex_numbers[0]=i;
											(triangle->vertices)[0]=uv[0];
											(triangle->vertices)[1]=uv[1];
											triangle->vertex_numbers[1]= *adjacent_vertex;
											adjacent_vertex++;
											(triangle->vertices)[2]= *adjacent_uv;
											adjacent_uv++;
											(triangle->vertices)[3]= *adjacent_uv;
											adjacent_uv++;
											triangle->vertex_numbers[2]= *adjacent_vertex;
											(triangle->vertices)[4]=adjacent_uv[0];
											(triangle->vertices)[5]=adjacent_uv[1];
											k++;
											if (cylinder_calculate_circumcentre(triangle))
											{
												number_of_triangles++;
											}
											else
											{
												DEALLOCATE(triangles[number_of_triangles]);
											}
										}
										else
										{
											display_message(ERROR_MESSAGE,
												"cylinder_delaunay.  Could not allocate triangle");
											return_code=0;
										}
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"cylinder_delaunay.  Could not reallocate triangles");
								}
							}
							DEALLOCATE(adjacent_vertices);
							DEALLOCATE(adjacent_angles);
							DEALLOCATE(adjacent_uvs);
						}
						if (1==ii%2)
						{
							vertex += 3;
						}
						ii++;
					}
#if defined (DEBUG)
					/*???debug */
					printf("number_of_triangles=%d (end)\n",number_of_triangles);
					for (l=0;l<number_of_triangles;l++)
					{
						printf("  triangle %d  %g %g  %g\n",l,triangles[l]->centre[0],
							triangles[l]->centre[1],triangles[l]->radius2);
						m=triangles[l]->vertex_numbers[0];
						if (m<0)
						{
							m += number_of_vertices;
						}
						else
						{
							if (m>=number_of_vertices)
							{
								m -= number_of_vertices;
							}
						}
						printf("    %d (%d)  %g %g  %g %g %g\n",
							triangles[l]->vertex_numbers[0],m,
							(triangles[l]->vertices)[0],(triangles[l]->vertices)[1],
							vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
						m=triangles[l]->vertex_numbers[1];
						if (m<0)
						{
							m += number_of_vertices;
						}
						else
						{
							if (m>=number_of_vertices)
							{
								m -= number_of_vertices;
							}
						}
						printf("    %d (%d)  %g %g  %g %g %g\n",
							triangles[l]->vertex_numbers[1],m,
							(triangles[l]->vertices)[2],(triangles[l]->vertices)[3],
							vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
						m=triangles[l]->vertex_numbers[2];
						if (m<0)
						{
							m += number_of_vertices;
						}
						else
						{
							if (m>=number_of_vertices)
							{
								m -= number_of_vertices;
							}
						}
						printf("    %d (%d)  %g %g  %g %g %g\n",
							triangles[l]->vertex_numbers[2],m,
							(triangles[l]->vertices)[4],(triangles[l]->vertices)[5],
							vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
					}
#endif /* defined (DEBUG) */
					if (return_code)
					{
						/* return results */
						number_of_returned_triangles=0;
						for (i=0;i<number_of_triangles;i++)
						{
							j=2;
							while ((j>=0)&&((((triangles[i])->vertex_numbers)[j]<0)||
								(number_of_vertices<=((triangles[i])->vertex_numbers)[j])))
							{
								j--;
							}
							if (j>=0)
							{
								number_of_returned_triangles++;
							}
						}
						if (ALLOCATE(returned_triangles,int,3*number_of_returned_triangles))
						{
							*triangles_address=returned_triangles;
							*number_of_triangles_address=number_of_returned_triangles;
							for (i=0;i<number_of_triangles;i++)
							{
								j=2;
								while ((j>=0)&&((((triangles[i])->vertex_numbers)[j]<0)||
									(number_of_vertices<=((triangles[i])->vertex_numbers)[j])))
								{
									j--;
								}
								if (j>=0)
								{
									for (j=0;j<3;j++)
									{
										*returned_triangles=((triangles[i])->vertex_numbers)[j];
										if (*returned_triangles>=number_of_vertices)
										{
											*returned_triangles -= number_of_vertices;
										}
										else
										{
											if (*returned_triangles<0)
											{
												*returned_triangles += number_of_vertices;
											}
										}
										returned_triangles++;
									}
								}
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"cylinder_delaunay.  Could not allocate returned_triangles");
						}
					}
					/* tidy up */
					for (i=0;i<number_of_triangles;i++)
					{
						DEALLOCATE(triangles[i]);
					}
					DEALLOCATE(triangles);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"cylinder_delaunay.  Could not allocate initial triangles 2");
					i++;
					while (i<number_of_triangles)
					{
						DEALLOCATE(triangles[i]);
						i++;
					}
					DEALLOCATE(triangles);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"cylinder_delaunay.  Could not allocate initial triangles");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cylinder_delaunay.  All vertices have same z.  No triangulation");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cylinder_delaunay.  Invalid argument(s) %d %p %p %p",number_of_vertices,
			vertices,number_of_triangles_address,triangles_address);
	}
	LEAVE;

	return (return_code);
} /* cylinder_delaunay */

int sphere_delaunay(int number_of_vertices,float *vertices,
	int *number_of_triangles_address,int **triangles_address)
/*******************************************************************************
LAST MODIFIED : 29 April 2002

DESCRIPTION :
Calculates the Delaunay triangulation of the <vertices> on a sphere whose centre
is the origin.
<number_of_vertices> is the number of vertices to be triangulated
<vertices> is an array of length 3*<number_of_vertices> containing the x,y,z
	coordinates of the vertices
<*number_of_triangles_address> is the number of triangles calculated by the
	function
<*triangles_address> is an array of length 3*<*number_of_triangles_address>,
	allocated by the function, containing the vertex numbers for each triangle
==============================================================================*/
{
	float **adjacent_line,**adjacent_lines,distance,dot_product,*edge_vertex_1,
		*edge_vertex_2,length_1,length_2,*vertex,*vertex_1,*vertex_2,*vertex_3,
		*vertex_4,xyz_centre[3],x1,x2,x3,x4,y1,y2,y3,y4,z1,z2,z3,z4;
	int i,j,k,l,maximum_number_of_adjacent_lines,maximum_number_of_triangles,
		number_of_adjacent_lines,number_of_triangles,return_code,
		*returned_triangles;
	struct Triangle
	{
		float centre[3],radius,*vertices[3];
	} **temp_triangles,*triangle,**triangles;
#if defined (OLD_CODE)
	float *adjacent_angles,*adjacent_angle,*adjacent_uv,*adjacent_uvs,angle,
		length,uv[2],*vertex,xref1,xref2,x1,x2,x3,yref1,yref2,y1,y2,y3,zref1,zref2,
		z1,z2,z3;
	int *adjacent_vertices,*adjacent_vertex,i,j,k,l,m,n,
		number_of_adjacent_vertices,number_of_returned_triangles,
		number_of_triangles,return_code,*returned_triangles,vertex_number;
	struct Delaunay_triangle **temp_triangles,*triangle,**triangles;
#endif /* defined (OLD_CODE) */

	ENTER(sphere_delaunay);
	return_code=0;
	/* check the arguments */
	if ((2<number_of_vertices)&&vertices&&number_of_triangles_address&&
		triangles_address)
	{
		/* normalize vertices */
		i=number_of_vertices;
		vertex=vertices;
		while ((i>0)&&(return_code=sphere_normalize(vertex,vertex)))
		{
			i--;
			vertex += 3;
		}
		if (return_code)
		{
			if (4<=number_of_vertices)
			{
				/* find a tetrahedron for the initial triangulation (4 triangles) */
				i=0;
				vertex=vertices;
				/* first corner */
				vertex_1=vertex;
				/* second corner must be different from first corner and not opposite
					to first corner */
				do
				{
					i=i+1;
					vertex += 3;
					return_code=sphere_calculate_distance(vertex_1,vertex,&distance);
				} while (return_code&&(i<number_of_vertices-3)&&
					((distance<=SPHERE_DELAUNAY_TOLERANCE)||
					(distance>=PI-SPHERE_DELAUNAY_TOLERANCE)));
				if (return_code&&(SPHERE_DELAUNAY_TOLERANCE<distance)&&
					(distance<PI-SPHERE_DELAUNAY_TOLERANCE))
				{
					vertex_2=vertex;
					/* third corner can't be on the same line as first two corners */
					x1=vertices[0];
					y1=vertices[1];
					z1=vertices[2];
					x2=vertex[0];
					y2=vertex[1];
					z2=vertex[2];
					x3=y1*z2-z1*y2;
					y3=z1*x2-x1*z2;
					z3=x1*y2-y1*x2;
					do
					{
						i=i+1;
						vertex += 3;
						x2=vertex[0];
						y2=vertex[1];
						z2=vertex[2];
						x4=y1*z2-z1*y2;
						y4=z1*x2-x1*z2;
						z4=x1*y2-y1*x2;
						length_1=(float)sqrt(x4*x4+y4*y4+z4*z4);
						length_2=(float)sqrt((x3-x4)*(x3-x4)+(y3-y4)*(y3-y4)+
							(z3-z4)*(z3-z4));
					} while ((i<number_of_vertices-2)&&
						((length_1<=SPHERE_DELAUNAY_TOLERANCE)||
						(length_2<=SPHERE_DELAUNAY_TOLERANCE)));
					if ((length_1>SPHERE_DELAUNAY_TOLERANCE)&&
						(length_2>SPHERE_DELAUNAY_TOLERANCE))
					{
						vertex_3=vertex;
						/* fourth corner can't be in same plane as other 3 */
						if (return_code=sphere_calculate_circumcentre(vertex_1,vertex_2,
							vertex,xyz_centre))
						{
							x1=xyz_centre[0];
							y1=xyz_centre[1];
							z1=xyz_centre[2];
							do
							{
								i=i+1;
								vertex += 3;
								if (return_code=sphere_calculate_circumcentre(vertex_1,
									vertex_2,vertex,xyz_centre))
								{
									x2=xyz_centre[0];
									y2=xyz_centre[1];
									z2=xyz_centre[2];
									x4=y1*z2-z1*y2;
									y4=z1*x2-x1*z2;
									z4=x1*y2-y1*x2;
									length_1=(float)sqrt(x4*x4+y4*y4+z4*z4);
								}
							} while (return_code&&(i<number_of_vertices-1)&&
								(length_1<=SPHERE_DELAUNAY_TOLERANCE));
							if (return_code&&(length_1>SPHERE_DELAUNAY_TOLERANCE))
							{
								vertex_4=vertex;
								/* determine which side of the first face the fourth corner
									is */
								x2=vertex_4[0]-vertex_1[0];
								y2=vertex_4[1]-vertex_1[1];
								z2=vertex_4[2]-vertex_1[2];
								dot_product=x1*x2+y1*y2+z1*z2;
								if (dot_product>0)
								{
									vertex=vertex_2;
									vertex_2=vertex_3;
									vertex_3=vertex;
								}
							}
							else
							{
								return_code=0;
							}
						}
					}
					else
					{
						return_code=0;
					}
				}
				else
				{
					return_code=0;
				}
				if (return_code)
				{
					maximum_number_of_triangles=4;
					if (ALLOCATE(triangles,struct Triangle *,maximum_number_of_triangles))
					{
						i=maximum_number_of_triangles;
						do
						{
							i--;
						}
						while ((i>=0)&&ALLOCATE(triangles[i],struct Triangle,1));
						if (i<0)
						{
							/* initial triangulation covers the sphere */
							triangle=triangles[0];
							(triangle->vertices)[0]=vertex_1;
							(triangle->vertices)[1]=vertex_2;
							(triangle->vertices)[2]=vertex_3;
							if (sphere_calculate_circumcentre((triangle->vertices)[0],
								(triangle->vertices)[1],(triangle->vertices)[2],
								triangle->centre)&&sphere_calculate_distance(
								(triangle->vertices)[0],triangle->centre,
								&(triangle->radius)))
							{
								triangle=triangles[1];
								(triangle->vertices)[0]=vertex_1;
								(triangle->vertices)[1]=vertex_4;
								(triangle->vertices)[2]=vertex_2;
								if (sphere_calculate_circumcentre((triangle->vertices)[0],
									(triangle->vertices)[1],(triangle->vertices)[2],
									triangle->centre)&&sphere_calculate_distance(
									(triangle->vertices)[0],triangle->centre,
									&(triangle->radius)))
								{
									triangle=triangles[2];
									(triangle->vertices)[0]=vertex_2;
									(triangle->vertices)[1]=vertex_4;
									(triangle->vertices)[2]=vertex_3;
									if (sphere_calculate_circumcentre((triangle->vertices)[0],
										(triangle->vertices)[1],(triangle->vertices)[2],
										triangle->centre)&&sphere_calculate_distance(
										(triangle->vertices)[0],triangle->centre,
										&(triangle->radius)))
									{
										triangle=triangles[3];
										(triangle->vertices)[0]=vertex_3;
										(triangle->vertices)[1]=vertex_4;
										(triangle->vertices)[2]=vertex_1;
										if (sphere_calculate_circumcentre((triangle->vertices)[0],
											(triangle->vertices)[1],(triangle->vertices)[2],
											triangle->centre)&&sphere_calculate_distance(
											(triangle->vertices)[0],triangle->centre,
											&(triangle->radius)))
										{
											number_of_triangles=4;
										}
										else
										{
											return_code=0;
										}
									}
									else
									{
										return_code=0;
									}
								}
								else
								{
									return_code=0;
								}
							}
							else
							{
								return_code=0;
							}
							if (return_code)
							{
								i=number_of_vertices;
								vertex=vertices;
								adjacent_lines=(float **)NULL;
								maximum_number_of_adjacent_lines=0;
								while (return_code&&(i>0))
								{
									if ((vertex!=vertex_1)&&(vertex!=vertex_2)&&
										(vertex!=vertex_3)&&(vertex!=vertex_4))
									{
										/* delete the triangles that no longer satisfy the in-circle
											criterion and create a list of the adjacent lines */
										number_of_adjacent_lines=0;
										j=0;
										while (return_code&&(j<number_of_triangles))
										{
											triangle=triangles[j];
											if (return_code=sphere_calculate_distance(
												triangle->centre,vertex,&length_1))
											{
												if (length_1<=(triangle->radius)+
													SPHERE_DELAUNAY_TOLERANCE)
												{
													/* vertex is inside the triangle's circumcircle */
													/* add the triangle's edges to the list of adjacent
														lines */
													edge_vertex_2=(triangle->vertices)[2];
													k=0;
													while (return_code&&(k<3))
													{
														edge_vertex_1=edge_vertex_2;
														edge_vertex_2=(triangle->vertices)[k];
														/* check for duplicates */
														l=number_of_adjacent_lines;
														adjacent_line=adjacent_lines;
														while ((l>0)&&
															!(((edge_vertex_1==adjacent_line[0])&&
															(edge_vertex_2==adjacent_line[1]))||
															((edge_vertex_2==adjacent_line[0])&&
															(edge_vertex_1==adjacent_line[1]))))
														{
															adjacent_line += 2;
															l--;
														}
														if (l>0)
														{
															/* duplicate line.  Remove the match from list */
															if (l>1)
															{
																l=2*number_of_adjacent_lines-2;
																adjacent_line[0]=adjacent_lines[l];
																adjacent_line[1]=adjacent_lines[l+1];
															}
															number_of_adjacent_lines--;
														}
														else
														{
															/* new line.  Add to list */
															if (number_of_adjacent_lines>=
																maximum_number_of_adjacent_lines)
															{
																if (REALLOCATE(adjacent_line,adjacent_lines,
																	float *,
																	2*(maximum_number_of_adjacent_lines+5)))
																{
																	adjacent_lines=adjacent_line;
																	maximum_number_of_adjacent_lines += 5;
																}
																else
																{
																	display_message(ERROR_MESSAGE,
																		"sphere_delaunay.  "
																		"Could not REALLOCATE adjacent_lines");
																	return_code=0;
																}
															}
															if (return_code)
															{
																l=2*number_of_adjacent_lines;
																adjacent_lines[l]=edge_vertex_1;
																adjacent_lines[l+1]=edge_vertex_2;
																number_of_adjacent_lines++;
															}
														}
														k++;
													}
													if (return_code)
													{
														/* delete triangle */
														if (j<number_of_triangles-1)
														{
															triangles[j]=triangles[number_of_triangles-1];
															triangles[number_of_triangles-1]=triangle;
														}
														number_of_triangles--;
													}
												}
												else
												{
													j++;
												}
											}
										}
										if (return_code)
										{
											/* create new triangles */
											if (number_of_triangles+number_of_adjacent_lines>
												maximum_number_of_triangles)
											{
												j=maximum_number_of_triangles;
												maximum_number_of_triangles=number_of_triangles+
													number_of_adjacent_lines+10;
												if (REALLOCATE(temp_triangles,triangles,
													struct Triangle *,maximum_number_of_triangles))
												{
													triangles=temp_triangles;
													temp_triangles += j;
													while (return_code&&(j<maximum_number_of_triangles))
													{
														if (ALLOCATE(*temp_triangles,struct Triangle,1))
														{
															j++;
															temp_triangles++;
														}
														else
														{
															maximum_number_of_triangles=j;
															display_message(ERROR_MESSAGE,"sphere_delaunay.  "
																"Could not ALLOCATE triangle");
															return_code=0;
														}
													}
												}
												else
												{
													maximum_number_of_triangles=j;
													display_message(ERROR_MESSAGE,"sphere_delaunay.  "
														"Could not REALLOCATE triangles");
													return_code=0;
												}
											}
											if (return_code)
											{
												adjacent_line=adjacent_lines+
													(2*number_of_adjacent_lines);
												l=number_of_adjacent_lines;
												while (return_code&&(l>0))
												{
													adjacent_line -= 2;
													triangle=triangles[number_of_triangles];
													(triangle->vertices)[0]=vertex;
													(triangle->vertices)[1]=adjacent_line[0];
													(triangle->vertices)[2]=adjacent_line[1];
													if (sphere_calculate_circumcentre(
														(triangle->vertices)[0],(triangle->vertices)[1],
														(triangle->vertices)[2],triangle->centre)&&
														sphere_calculate_distance((triangle->vertices)[0],
														triangle->centre,&(triangle->radius)))
													{
														number_of_triangles++;
													}
													else
													{
														return_code=0;
													}
													l--;
												}
											}
										}
									}
									i--;
									vertex += 3;
								}
								DEALLOCATE(adjacent_lines);
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"sphere_delaunay.  Could not set up initial trianglulation");
							}
							if (return_code)
							{
								/* return results */
								if (ALLOCATE(returned_triangles,int,3*number_of_triangles))
								{
									*triangles_address=returned_triangles;
									*number_of_triangles_address=number_of_triangles;
									for (i=0;i<number_of_triangles;i++)
									{
										triangle=triangles[i];
										for (j=0;j<3;j++)
										{
											*returned_triangles=((triangle->vertices)[j]-vertices)/3;
											returned_triangles++;
										}
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"sphere_delaunay.  Could not allocate returned_triangles");
									return_code=0;
								}
							}
							/* tidy up */
							for (i=0;i<maximum_number_of_triangles;i++)
							{
								DEALLOCATE(triangles[i]);
							}
							DEALLOCATE(triangles);
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"sphere_delaunay.  Could not allocate initial triangles 2");
							i++;
							while (i<maximum_number_of_triangles)
							{
								DEALLOCATE(triangles[i]);
								i++;
							}
							DEALLOCATE(triangles);
							return_code=0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"sphere_delaunay.  Could not allocate initial triangles.  %d",
							number_of_triangles);
						return_code=0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"sphere_delaunay.  Could not find initial tetrahedron");
				}
			}
			else
			{
				number_of_triangles=2;
				if (ALLOCATE(returned_triangles,int,3*number_of_triangles))
				{
					returned_triangles[0]=0;
					returned_triangles[1]=1;
					returned_triangles[2]=2;
					returned_triangles[3]=0;
					returned_triangles[4]=2;
					returned_triangles[5]=1;
					*number_of_triangles_address=number_of_triangles;
					*triangles_address=returned_triangles;
				}
				else
				{
					display_message(ERROR_MESSAGE,"sphere_delaunay.  "
						"Could not allocate returned triangles for 3 vertices.  %d",
						number_of_triangles);
					return_code=0;
				}
			}
		}
#if defined (OLD_CODE)
		number_of_triangles=2;
		if (ALLOCATE(triangles,struct Delaunay_triangle *,number_of_triangles))
		{
			i=number_of_triangles;
			do
			{
				i--;
			}
			while ((i>=0)&&ALLOCATE(triangles[i],struct Delaunay_triangle,1));
			if (i<0)
			{
				/* initial triangulation covers the sphere */
				(triangles[0]->vertex_numbers)[0]=0;
				sphere_normalize(vertices,triangles[0]->vertices);
				(triangles[0]->vertex_numbers)[1]=1;
				sphere_normalize(vertices+3,(triangles[0]->vertices)+2);
				(triangles[0]->vertex_numbers)[2]=2;
				sphere_normalize(vertices+6,(triangles[0]->vertices)+4);
				(triangles[1]->vertex_numbers)[0]=0;
				sphere_normalize(vertices,triangles[1]->vertices);
				(triangles[1]->vertex_numbers)[1]=2;
				sphere_normalize(vertices+6,(triangles[1]->vertices)+2);
				(triangles[1]->vertex_numbers)[2]=1;
				sphere_normalize(vertices+3,(triangles[1]->vertices)+4);
				for (i=0;i<number_of_triangles;i++)
				{
					sphere_calculate_circumcentre(triangles[i]);
				}
				return_code=1;
				i=3;
				vertex=vertices+9;
				while (return_code&&(i<number_of_vertices))
				{
#if defined (DEBUG)
					/*???debug */
					printf("number_of_triangles=%d\n",number_of_triangles);
					for (l=0;l<number_of_triangles;l++)
					{
						printf("  triangle %d  %g %g  %g\n",l,triangles[l]->centre[0],
							triangles[l]->centre[1],triangles[l]->radius2);
						printf("    %d  %g %g\n",triangles[l]->vertex_numbers[0],
							(triangles[l]->vertices)[0],(triangles[l]->vertices)[1]);
						printf("    %d  %g %g\n",triangles[l]->vertex_numbers[1],
							(triangles[l]->vertices)[2],(triangles[l]->vertices)[3]);
						printf("    %d  %g %g\n",triangles[l]->vertex_numbers[2],
							(triangles[l]->vertices)[4],(triangles[l]->vertices)[5]);
					}
#endif /* defined (DEBUG) */
					sphere_normalize(vertex,uv);
#if defined (DEBUG)
					/*???debug */
					printf("vertex %d %g %g %g  %g %g\n",i,vertex[0],vertex[1],
						vertex[2],uv[0],uv[1]);
#endif /* defined (DEBUG) */
					/* delete the triangles that no longer satisfy the in-circle
						criterion */
					j=0;
					k=0;
					number_of_adjacent_vertices=0;
					adjacent_vertices=(int *)NULL;
					adjacent_angles=(float *)NULL;
					adjacent_uvs=(float *)NULL;
					x1=cos(uv[1])*cos(uv[0]);
					y1=cos(uv[1])*sin(uv[0]);
					z1=sin(uv[1]);
					/* calculate a coordinate vectors for the plane perpendicular to
						(x1,y1,z1) */
					if (x1<0)
					{
						x2= -x1;
					}
					else
					{
						x2=x1;
					}
					if (y1<0)
					{
						y2= -y1;
					}
					else
					{
						y2=y1;
					}
					if (z1<0)
					{
						z2= -z1;
					}
					else
					{
						z2=z1;
					}
					if (x2>y2)
					{
						if (x2>z2)
						{
							if (y2>z2)
							{
								xref1=y1;
								yref1= -x1;
								zref1=0;
							}
							else
							{
								xref1=z1;
								yref1=0;
								zref1= -x1;
							}
						}
						else
						{
							xref1= -z1;
							yref1=0;
							zref1=x1;
						}
					}
					else
					{
						if (y2>z2)
						{
							if (x2>z2)
							{
								xref1= -y1;
								yref1=x1;
								zref1=0;
							}
							else
							{
								xref1=0;
								yref1=z1;
								zref1= -y1;
							}
						}
						else
						{
							xref1=0;
							yref1= -z1;
							zref1=y1;
						}
					}
					length=sqrt(xref1*xref1+yref1*yref1+zref1*zref1);
					xref1 /= length;
					yref1 /= length;
					zref1 /= length;
					xref2=y1*zref1-z1*yref1;
					yref2=z1*xref1-x1*zref1;
					zref2=x1*yref1-y1*xref1;
					while (return_code&&(j<number_of_triangles))
					{
#if defined (DEBUG)
						/*???debug */
						printf("triangle %d.  %g %g\n",j,
							sphere_calculate_distance(uv,triangles[j]->centre),
							triangles[j]->radius2);
#endif /* defined (DEBUG) */
						if (sphere_calculate_distance(uv,triangles[j]->centre)<=
							triangles[j]->radius2)
						{
							k++;
							/* add the vertices to the list of adjacent vertices,
								ordering the adjacent vertices so that they go
								anti-clockwise around the new vertex */
							REALLOCATE(adjacent_vertex,adjacent_vertices,int,
								number_of_adjacent_vertices+4);
							REALLOCATE(adjacent_angle,adjacent_angles,float,
								number_of_adjacent_vertices+3);
							REALLOCATE(adjacent_uv,adjacent_uvs,float,
								2*number_of_adjacent_vertices+8);
							if (adjacent_vertex&&adjacent_angle&&adjacent_uv)
							{
								adjacent_vertices=adjacent_vertex;
								adjacent_angles=adjacent_angle;
								adjacent_uvs=adjacent_uv;
								for (l=0;l<3;l++)
								{
									m=0;
									vertex_number=(triangles[j]->vertex_numbers)[l];
									x2=cos((triangles[j]->vertices)[2*l+1])*
										cos((triangles[j]->vertices)[2*l]);
									y2=cos((triangles[j]->vertices)[2*l+1])*
										sin((triangles[j]->vertices)[2*l]);
									z2=sin((triangles[j]->vertices)[2*l+1]);
									/* cross-product */
									x3=y1*z2-z1*y2;
									y3=z1*x2-x1*z2;
									z3=x1*y2-y1*x2;
									angle=atan2(x3*xref1+y3*yref1+z3*zref1,
										x3*xref2+y3*yref2+z3*zref2);
									while ((m<number_of_adjacent_vertices)&&
										(vertex_number!=adjacent_vertices[m])&&
										(angle<adjacent_angles[m]))
									{
										m++;
									}
									if ((m>=number_of_adjacent_vertices)||
										(vertex_number!=adjacent_vertices[m]))
									{
										adjacent_vertex=adjacent_vertices+
											number_of_adjacent_vertices;
										adjacent_angle=adjacent_angles+
											number_of_adjacent_vertices;
										adjacent_uv=adjacent_uvs+
											2*number_of_adjacent_vertices;
										for (n=number_of_adjacent_vertices;n>m;n--)
										{
											adjacent_vertex--;
											adjacent_vertex[1]=adjacent_vertex[0];
											adjacent_angle--;
											adjacent_angle[1]=adjacent_angle[0];
											adjacent_uv--;
											adjacent_uv[2]=adjacent_uv[0];
											adjacent_uv--;
											adjacent_uv[2]=adjacent_uv[0];
										}
										*adjacent_vertex=vertex_number;
										*adjacent_angle=angle;
										*adjacent_uv=(triangles[j]->vertices)[2*l];
										adjacent_uv++;
										*adjacent_uv=(triangles[j]->vertices)[2*l+1];
										number_of_adjacent_vertices++;
#if defined (DEBUG)
										/*???debug */
										printf("adjacent vertices\n");
										for (n=0;n<number_of_adjacent_vertices;n++)
										{
											printf("  %d.  %g %g  %g\n",adjacent_vertices[n],
												adjacent_uvs[2*n],adjacent_uvs[2*n+1],
												adjacent_angles[n]);
										}
#endif /* defined (DEBUG) */
									}
								}
							}
							else
							{
								if (adjacent_vertex)
								{
									adjacent_vertices=adjacent_vertex;
								}
								if (adjacent_angle)
								{
									adjacent_angles=adjacent_angle;
								}
								if (adjacent_uv)
								{
									adjacent_uvs=adjacent_uv;
								}
								return_code=0;
								display_message(ERROR_MESSAGE,"sphere_delaunay.  "
									"Could not reallocate adjacent vertices");
							}
							DEALLOCATE(triangles[j]);
						}
						else
						{
							if (k>0)
							{
								triangles[j-k]=triangles[j];
							}
						}
						j++;
					}
					if (return_code&&(k>0))
					{
						number_of_triangles -= k;;
						/* determine new triangles */
						if (REALLOCATE(temp_triangles,triangles,
							struct Delaunay_triangle *,
							number_of_triangles+number_of_adjacent_vertices))
						{
							triangles=temp_triangles;
							adjacent_vertex=adjacent_vertices;
							adjacent_angle=adjacent_angles;
							adjacent_uv=adjacent_uvs;
							adjacent_vertices[number_of_adjacent_vertices]=
								adjacent_vertices[0];
							adjacent_uvs[2*number_of_adjacent_vertices]=adjacent_uvs[0];
							adjacent_uvs[2*number_of_adjacent_vertices+1]=adjacent_uvs[1];
							k=0;
							while (return_code&&(k<number_of_adjacent_vertices))
							{
								if (triangle=ALLOCATE(triangles[number_of_triangles],
									struct Delaunay_triangle,1))
								{
									triangle->vertex_numbers[0]=i;
									(triangle->vertices)[0]=uv[0];
									(triangle->vertices)[1]=uv[1];
									triangle->vertex_numbers[1]= *adjacent_vertex;
									adjacent_vertex++;
									(triangle->vertices)[2]= *adjacent_uv;
									adjacent_uv++;
									(triangle->vertices)[3]= *adjacent_uv;
									adjacent_uv++;
									triangle->vertex_numbers[2]= *adjacent_vertex;
									(triangle->vertices)[4]=adjacent_uv[0];
									(triangle->vertices)[5]=adjacent_uv[1];
									k++;
									if (sphere_calculate_circumcentre(triangle))
									{
										number_of_triangles++;
									}
									else
									{
										DEALLOCATE(triangles[number_of_triangles]);
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"sphere_delaunay.  Could not allocate triangle");
									return_code=0;
								}
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"sphere_delaunay.  Could not reallocate triangles");
						}
					}
					DEALLOCATE(adjacent_vertices);
					DEALLOCATE(adjacent_angles);
					DEALLOCATE(adjacent_uvs);
					i++;
					vertex += 3;
				}
#if defined (DEBUG)
				/*???debug */
				printf("number_of_triangles=%d (end)\n",number_of_triangles);
				for (l=0;l<number_of_triangles;l++)
				{
					printf("  triangle %d  %g %g  %g\n",l,triangles[l]->centre[0],
						triangles[l]->centre[1],triangles[l]->radius2);
					m=triangles[l]->vertex_numbers[0];
					printf("    %d.  %g %g  %g %g %g\n",m,
						(triangles[l]->vertices)[0],(triangles[l]->vertices)[1],
						vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
					m=triangles[l]->vertex_numbers[1];
					printf("    %d.  %g %g  %g %g %g\n",m,
						(triangles[l]->vertices)[2],(triangles[l]->vertices)[3],
						vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
					m=triangles[l]->vertex_numbers[2];
					printf("    %d.  %g %g  %g %g %g\n",m,
						(triangles[l]->vertices)[4],(triangles[l]->vertices)[5],
						vertices[3*m],vertices[3*m+1],vertices[3*m+2]);
				}
#endif /* defined (DEBUG) */
				if (return_code)
				{
					/* return results */
					number_of_returned_triangles=number_of_triangles;
					if (ALLOCATE(returned_triangles,int,3*number_of_returned_triangles))
					{
						*triangles_address=returned_triangles;
						*number_of_triangles_address=number_of_returned_triangles;
						for (i=0;i<number_of_triangles;i++)
						{
							for (j=0;j<3;j++)
							{
								*returned_triangles=((triangles[i])->vertex_numbers)[j];
								returned_triangles++;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"sphere_delaunay.  Could not allocate returned_triangles");
					}
				}
				/* tidy up */
				for (i=0;i<number_of_triangles;i++)
				{
					DEALLOCATE(triangles[i]);
				}
				DEALLOCATE(triangles);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"sphere_delaunay.  Could not allocate initial triangles 2");
				i++;
				while (i<number_of_triangles)
				{
					DEALLOCATE(triangles[i]);
					i++;
				}
				DEALLOCATE(triangles);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"sphere_delaunay.  Could not allocate initial triangles");
		}
#endif /* defined (OLD_CODE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"sphere_delaunay.  Invalid argument(s) %d %p %p %p",number_of_vertices,
			vertices,number_of_triangles_address,triangles_address);
	}
	LEAVE;

	return (return_code);
} /* sphere_delaunay */

#if defined (OLD_CODE)
/* uses sphere_delaunay way of removing and adding triangles */
int plane_delaunay(int number_of_vertices,float *vertices,
	int *number_of_triangles_address,int **triangles_address)
/*******************************************************************************
LAST MODIFIED : 18 April 2004

DESCRIPTION :
Calculates the Delaunay triangulation of the <vertices> on a plane.
<number_of_vertices> is the number of vertices to be triangulated
<vertices> is an array of length 2*<number_of_vertices> containing the x,y
	coordinates of the vertices
<*number_of_triangles_address> is the number of triangles calculated by the
	function
<*triangles_address> is an array of length 3*<*number_of_triangles_address>,
	allocated by the function, containing the vertex numbers for each triangle
==============================================================================*/
{
	float *vertex,x_maximum,x_minimum,x_temp,y_maximum,y_minimum,y_temp;
	int *adjacent_line,*adjacent_lines,edge_vertex_1,edge_vertex_2,i,j,k,l,
		maximum_number_of_adjacent_lines,maximum_number_of_triangles,
		number_of_adjacent_lines,number_of_returned_triangles,number_of_triangles,
		return_code,*returned_triangles;
	struct Delaunay_triangle **temp_triangles,*triangle,**triangles;

	ENTER(plane_delaunay);
	return_code=0;
	/* check the arguments */
	if ((2<number_of_vertices)&&vertices&&number_of_triangles_address&&
		triangles_address)
	{
		/* find the x and y ranges */
		i=2*number_of_vertices-1;
		y_minimum=vertices[i];
		y_maximum=y_minimum;
		i--;
		x_minimum=vertices[i];
		x_maximum=x_minimum;
		while (i>0)
		{
			i--;
			if (vertices[i]<y_minimum)
			{
				y_minimum=vertices[i];
			}
			else
			{
				if (vertices[i]>y_maximum)
				{
					y_maximum=vertices[i];
				}
			}
			i--;
			if (vertices[i]<x_minimum)
			{
				x_minimum=vertices[i];
			}
			else
			{
				if (vertices[i]>x_maximum)
				{
					x_maximum=vertices[i];
				}
			}
		}
		/* expand range to be outside */
		if (x_maximum==x_minimum)
		{
			x_maximum += 1;
			x_minimum -= 1;
		}
		else
		{
			x_maximum += x_maximum-x_minimum;
			x_minimum -= x_maximum-x_minimum;
		}
		if (y_maximum==y_minimum)
		{
			y_maximum += 1;
			y_minimum -= 1;
		}
		else
		{
			y_maximum += y_maximum-y_minimum;
			y_minimum -= y_maximum-y_minimum;
		}
		/* create initial triangulation */
		number_of_triangles=2;
		maximum_number_of_triangles=2;
		if (ALLOCATE(triangles,struct Delaunay_triangle *,number_of_triangles))
		{
			i=number_of_triangles;
			do
			{
				i--;
			}
			while ((i>=0)&&ALLOCATE(triangles[i],struct Delaunay_triangle,1));
			if (i<0)
			{
				(triangles[0]->vertex_numbers)[0]= -4;
				(triangles[0]->vertices)[0]=x_minimum;
				(triangles[0]->vertices)[1]=y_minimum;
				(triangles[0]->vertex_numbers)[1]= -2;
				(triangles[0]->vertices)[2]=x_minimum;
				(triangles[0]->vertices)[3]=y_maximum;
				(triangles[0]->vertex_numbers)[2]= -3;
				(triangles[0]->vertices)[4]=x_maximum;
				(triangles[0]->vertices)[5]=y_minimum;
				(triangles[1]->vertex_numbers)[0]= -3;
				(triangles[1]->vertices)[0]=x_maximum;
				(triangles[1]->vertices)[1]=y_minimum;
				(triangles[1]->vertex_numbers)[1]= -2;
				(triangles[1]->vertices)[2]=x_minimum;
				(triangles[1]->vertices)[3]=y_maximum;
				(triangles[1]->vertex_numbers)[2]= -1;
				(triangles[1]->vertices)[4]=x_maximum;
				(triangles[1]->vertices)[5]=y_maximum;
				for (i=0;i<number_of_triangles;i++)
				{
					/* circumcentre is calculated the same */
					cylinder_calculate_circumcentre(triangles[i]);
				}
				return_code=1;
				i=0;
				vertex=vertices;
				adjacent_lines=(int *)NULL;
				maximum_number_of_adjacent_lines=0;
				while (return_code&&(i<number_of_vertices))
				{
					/* delete the triangles that no longer satisfy the in-circle
						criterion and create a list of the adjacent lines */
					number_of_adjacent_lines=0;
					j=0;
					while (return_code&&(j<number_of_triangles))
					{
						triangle=triangles[j];
						x_temp=vertex[0]-(triangle->centre)[0];
						y_temp=vertex[1]-(triangle->centre)[1];
						if (x_temp*x_temp+y_temp*y_temp<=triangle->radius2)
						{
							/* vertex is inside the triangle's circumcircle */
							/* add the triangle's edges to the list of adjacent lines */
							edge_vertex_2=(triangle->vertex_numbers)[2];
							k=0;
							while (return_code&&(k<3))
							{
								edge_vertex_1=edge_vertex_2;
								edge_vertex_2=(triangle->vertex_numbers)[k];
								/* check for duplicates */
								l=number_of_adjacent_lines;
								adjacent_line=adjacent_lines;
								while ((l>0)&&
									!(((edge_vertex_1==adjacent_line[0])&&
									(edge_vertex_2==adjacent_line[1]))||
									((edge_vertex_2==adjacent_line[0])&&
									(edge_vertex_1==adjacent_line[1]))))
								{
									adjacent_line += 2;
									l--;
								}
								if (l>0)
								{
									/* duplicate line.  Cancels out.  Remove the match froms
										list */
									if (l>1)
									{
										l=2*number_of_adjacent_lines-2;
										adjacent_line[0]=adjacent_lines[l];
										adjacent_line[1]=adjacent_lines[l+1];
									}
									number_of_adjacent_lines--;
								}
								else
								{
									/* new line.  Add to list */
									if (number_of_adjacent_lines>=
										maximum_number_of_adjacent_lines)
									{
										if (REALLOCATE(adjacent_line,adjacent_lines,int,
											2*(maximum_number_of_adjacent_lines+5)))
										{
											adjacent_lines=adjacent_line;
											maximum_number_of_adjacent_lines += 5;
										}
										else
										{
											display_message(ERROR_MESSAGE,
												"plane_delaunay.  Could not REALLOCATE adjacent_lines");
											return_code=0;
										}
									}
									if (return_code)
									{
										l=2*number_of_adjacent_lines;
										adjacent_lines[l]=edge_vertex_1;
										adjacent_lines[l+1]=edge_vertex_2;
										number_of_adjacent_lines++;
									}
								}
								k++;
							}
							if (return_code)
							{
								/* delete triangle */
								if (j<number_of_triangles-1)
								{
									triangles[j]=triangles[number_of_triangles-1];
									triangles[number_of_triangles-1]=triangle;
								}
								number_of_triangles--;
							}
						}
						else
						{
							j++;
						}
					}
					if (return_code)
					{
						/* create new triangles */
						if (number_of_triangles+number_of_adjacent_lines>
							maximum_number_of_triangles)
						{
							j=maximum_number_of_triangles;
							maximum_number_of_triangles=number_of_triangles+
								number_of_adjacent_lines+10;
							if (REALLOCATE(temp_triangles,triangles,
								struct Delaunay_triangle *,maximum_number_of_triangles))
							{
								triangles=temp_triangles;
								temp_triangles += j;
								while (return_code&&(j<maximum_number_of_triangles))
								{
									if (ALLOCATE(*temp_triangles,struct Delaunay_triangle,1))
									{
										j++;
										temp_triangles++;
									}
									else
									{
										maximum_number_of_triangles=j;
										display_message(ERROR_MESSAGE,"plane_delaunay.  "
											"Could not ALLOCATE triangle");
										return_code=0;
									}
								}
							}
							else
							{
								maximum_number_of_triangles=j;
								display_message(ERROR_MESSAGE,"plane_delaunay.  "
									"Could not REALLOCATE triangles");
								return_code=0;
							}
						}
						if (return_code)
						{
							adjacent_line=adjacent_lines+(2*number_of_adjacent_lines);
							l=number_of_adjacent_lines;
							while (return_code&&(l>0))
							{
								adjacent_line -= 2;
								triangle=triangles[number_of_triangles];
								(triangle->vertex_numbers)[0]=i;
								(triangle->vertices)[0]=vertex[0];
								(triangle->vertices)[1]=vertex[1];
								(triangle->vertex_numbers)[1]=adjacent_line[0];
								if (0<=adjacent_line[0])
								{
									(triangle->vertices)[2]=vertices[2*adjacent_line[0]];
									(triangle->vertices)[3]=vertices[2*adjacent_line[0]+1];
								}
								else
								{
									switch (adjacent_line[0])
									{
										case -4:
										{
											(triangle->vertices)[2]=x_minimum;
											(triangle->vertices)[3]=y_minimum;
										} break;
										case -3:
										{
											(triangle->vertices)[2]=x_maximum;
											(triangle->vertices)[3]=y_minimum;
										} break;
										case -2:
										{
											(triangle->vertices)[2]=x_minimum;
											(triangle->vertices)[3]=y_maximum;
										} break;
										case -1:
										{
											(triangle->vertices)[2]=x_maximum;
											(triangle->vertices)[3]=y_maximum;
										} break;
									}
								}
								(triangle->vertex_numbers)[2]=adjacent_line[1];
								if (0<=adjacent_line[1])
								{
									(triangle->vertices)[4]=vertices[2*adjacent_line[1]];
									(triangle->vertices)[5]=vertices[2*adjacent_line[1]+1];
								}
								else
								{
									switch (adjacent_line[1])
									{
										case -4:
										{
											(triangle->vertices)[4]=x_minimum;
											(triangle->vertices)[5]=y_minimum;
										} break;
										case -3:
										{
											(triangle->vertices)[4]=x_maximum;
											(triangle->vertices)[5]=y_minimum;
										} break;
										case -2:
										{
											(triangle->vertices)[4]=x_minimum;
											(triangle->vertices)[5]=y_maximum;
										} break;
										case -1:
										{
											(triangle->vertices)[4]=x_maximum;
											(triangle->vertices)[5]=y_maximum;
										} break;
									}
								}
								if (cylinder_calculate_circumcentre(triangle))
								{
									number_of_triangles++;
								}
								else
								{
									/*???DB.  Assume that triangle has zero area because two nodes
										have the same coordinates.  Don't add triangle */
#if defined (OLD_CODE)
									display_message(ERROR_MESSAGE,
										"plane_delaunay.  cylinder_calculate_circumcentre failed");
									return_code=0;
#endif /* defined (OLD_CODE) */
								}
								l--;
							}
						}
					}
					i++;
					vertex += 2;
				}
				DEALLOCATE(adjacent_lines);
				if (return_code)
				{
					/* return results */
					number_of_returned_triangles=0;
					for (i=0;i<number_of_triangles;i++)
					{
						j=2;
						while ((j>=0)&&(((triangles[i])->vertex_numbers)[j]>=0))
						{
							j--;
						}
						if (j<0)
						{
							number_of_returned_triangles++;
						}
					}
					if (ALLOCATE(returned_triangles,int,3*number_of_returned_triangles))
					{
						*triangles_address=returned_triangles;
						*number_of_triangles_address=number_of_returned_triangles;
						for (i=0;i<number_of_triangles;i++)
						{
							j=2;
							while ((j>=0)&&(((triangles[i])->vertex_numbers)[j]>=0))
							{
								j--;
							}
							if (j<0)
							{
								for (j=0;j<3;j++)
								{
									*returned_triangles=((triangles[i])->vertex_numbers)[j];
									returned_triangles++;
								}
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"plane_delaunay.  Could not allocate returned_triangles");
					}
				}
				/* tidy up */
				for (i=0;i<number_of_triangles;i++)
				{
					DEALLOCATE(triangles[i]);
				}
				DEALLOCATE(triangles);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"plane_delaunay.  Could not allocate initial triangles 2");
				i++;
				while (i<number_of_triangles)
				{
					DEALLOCATE(triangles[i]);
					i++;
				}
				DEALLOCATE(triangles);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"plane_delaunay.  Could not allocate initial triangles");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"plane_delaunay.  Invalid argument(s) %d %p %p %p",number_of_vertices,
			vertices,number_of_triangles_address,triangles_address);
	}
	LEAVE;

	return (return_code);
} /* plane_delaunay */
#else /* defined (OLD_CODE) */
int plane_delaunay(int number_of_vertices,float *vertices,
	int *number_of_triangles_address,int **triangles_address)
/*******************************************************************************
LAST MODIFIED : 20 April 2004

DESCRIPTION :
Calculates the Delaunay triangulation of the <vertices> on a plane.
<number_of_vertices> is the number of vertices to be triangulated
<vertices> is an array of length 2*<number_of_vertices> containing the x,y
	coordinates of the vertices
<*number_of_triangles_address> is the number of triangles calculated by the
	function
<*triangles_address> is an array of length 3*<*number_of_triangles_address>,
	allocated by the function, containing the vertex numbers for each triangle
==============================================================================*/
{
	float *adjacent_angle,*adjacent_angles,*adjacent_xy,*adjacent_xys,angle,
		*vertex,x_maximum,x_minimum,x_temp,y_maximum,y_minimum,y_temp;
	int *adjacent_vertex,*adjacent_vertices,i,j,k,l,m,n,
		maximum_number_of_triangles,number_of_adjacent_vertices,
		number_of_returned_triangles,number_of_triangles,return_code,
		*returned_triangles,vertex_number;
	struct Delaunay_triangle **temp_triangles,*triangle,**triangles;

	ENTER(plane_delaunay);
	return_code=0;
	/* check the arguments */
	if ((2<number_of_vertices)&&vertices&&number_of_triangles_address&&
		triangles_address)
	{
		/* find the x and y ranges */
		i=2*number_of_vertices-1;
		y_minimum=vertices[i];
		y_maximum=y_minimum;
		i--;
		x_minimum=vertices[i];
		x_maximum=x_minimum;
		while (i>0)
		{
			i--;
			if (vertices[i]<y_minimum)
			{
				y_minimum=vertices[i];
			}
			else
			{
				if (vertices[i]>y_maximum)
				{
					y_maximum=vertices[i];
				}
			}
			i--;
			if (vertices[i]<x_minimum)
			{
				x_minimum=vertices[i];
			}
			else
			{
				if (vertices[i]>x_maximum)
				{
					x_maximum=vertices[i];
				}
			}
		}
		/* expand range to be outside */
		if (x_maximum==x_minimum)
		{
			x_maximum += 1;
			x_minimum -= 1;
		}
		else
		{
			x_maximum += x_maximum-x_minimum;
			x_minimum -= x_maximum-x_minimum;
		}
		if (y_maximum==y_minimum)
		{
			y_maximum += 1;
			y_minimum -= 1;
		}
		else
		{
			y_maximum += y_maximum-y_minimum;
			y_minimum -= y_maximum-y_minimum;
		}
		/* create initial triangulation */
		number_of_triangles=2;
		maximum_number_of_triangles=2;
		if (ALLOCATE(triangles,struct Delaunay_triangle *,number_of_triangles))
		{
			i=number_of_triangles;
			do
			{
				i--;
			}
			while ((i>=0)&&ALLOCATE(triangles[i],struct Delaunay_triangle,1));
			if (i<0)
			{
				(triangles[0]->vertex_numbers)[0]= -4;
				(triangles[0]->vertices)[0]=x_minimum;
				(triangles[0]->vertices)[1]=y_minimum;
				(triangles[0]->vertex_numbers)[1]= -2;
				(triangles[0]->vertices)[2]=x_minimum;
				(triangles[0]->vertices)[3]=y_maximum;
				(triangles[0]->vertex_numbers)[2]= -3;
				(triangles[0]->vertices)[4]=x_maximum;
				(triangles[0]->vertices)[5]=y_minimum;
				(triangles[1]->vertex_numbers)[0]= -3;
				(triangles[1]->vertices)[0]=x_maximum;
				(triangles[1]->vertices)[1]=y_minimum;
				(triangles[1]->vertex_numbers)[1]= -2;
				(triangles[1]->vertices)[2]=x_minimum;
				(triangles[1]->vertices)[3]=y_maximum;
				(triangles[1]->vertex_numbers)[2]= -1;
				(triangles[1]->vertices)[4]=x_maximum;
				(triangles[1]->vertices)[5]=y_maximum;
				for (i=0;i<number_of_triangles;i++)
				{
					/* circumcentre is calculated the same */
					cylinder_calculate_circumcentre(triangles[i]);
				}
				return_code=1;
				i=0;
				vertex=vertices;
				while (return_code&&(i<number_of_vertices))
				{
					/* check for coincident vertices */
					j=0;
					while ((j<i)&&
						!((vertices[2*j]==vertex[0])&&(vertices[2*j+1]==vertex[1])))
					{
						j++;
					}
					if (j==i)
					{
						/* delete the triangles that no longer satisfy the in-circle
							criterion and create a list of the adjacent lines */
						j=0;
						k=0;
						number_of_adjacent_vertices=0;
						adjacent_vertices=(int *)NULL;
						adjacent_angles=(float *)NULL;
						adjacent_xys=(float *)NULL;
						while (return_code&&(j<number_of_triangles))
						{
							triangle=triangles[j];
							x_temp=vertex[0]-(triangle->centre)[0];
							y_temp=vertex[1]-(triangle->centre)[1];
							if (x_temp*x_temp+y_temp*y_temp<=triangle->radius2)
							{
								k++;
								/* add the vertices to the list of adjacent vertices, ordering
									the adjacent vertices so that they go anti-clockwise around
									the new vertex */
								REALLOCATE(adjacent_vertex,adjacent_vertices,int,
									number_of_adjacent_vertices+4);
								REALLOCATE(adjacent_angle,adjacent_angles,float,
									number_of_adjacent_vertices+3);
								REALLOCATE(adjacent_xy,adjacent_xys,float,
									2*number_of_adjacent_vertices+8);
								if (adjacent_vertex&&adjacent_angle&&adjacent_xy)
								{
									adjacent_vertices=adjacent_vertex;
									adjacent_angles=adjacent_angle;
									adjacent_xys=adjacent_xy;
									for (l=0;l<3;l++)
									{
										m=0;
										vertex_number=(triangles[j]->vertex_numbers)[l];
										angle=(triangles[j]->vertices)[2*l]-vertex[0];
										angle=
											atan2((triangles[j]->vertices)[2*l+1]-vertex[1],angle);
										while ((m<number_of_adjacent_vertices)&&
											(vertex_number!=adjacent_vertices[m])&&
											(angle<adjacent_angles[m]))
										{
											m++;
										}
										if ((m>=number_of_adjacent_vertices)||
											(vertex_number!=adjacent_vertices[m]))
										{
											adjacent_vertex=adjacent_vertices+
												number_of_adjacent_vertices;
											adjacent_angle=adjacent_angles+
												number_of_adjacent_vertices;
											adjacent_xy=adjacent_xys+
												2*number_of_adjacent_vertices;
											for (n=number_of_adjacent_vertices;n>m;n--)
											{
												adjacent_vertex--;
												adjacent_vertex[1]=adjacent_vertex[0];
												adjacent_angle--;
												adjacent_angle[1]=adjacent_angle[0];
												adjacent_xy--;
												adjacent_xy[2]=adjacent_xy[0];
												adjacent_xy--;
												adjacent_xy[2]=adjacent_xy[0];
											}
											*adjacent_vertex=vertex_number;
											*adjacent_angle=angle;
											*adjacent_xy=(triangles[j]->vertices)[2*l];
											adjacent_xy++;
											*adjacent_xy=(triangles[j]->vertices)[2*l+1];
											number_of_adjacent_vertices++;
#if defined (DEBUG)
											/*???debug */
											printf("adjacent vertices\n");
											for (n=0;n<number_of_adjacent_vertices;n++)
											{
												printf("  %d.  %g %g  %g\n",adjacent_vertices[n],
													adjacent_xys[2*n],adjacent_xys[2*n+1],
													adjacent_angles[n]);
											}
#endif /* defined (DEBUG) */
										}
									}
								}
								else
								{
									if (adjacent_vertex)
									{
										adjacent_vertices=adjacent_vertex;
									}
									if (adjacent_angle)
									{
										adjacent_angles=adjacent_angle;
									}
									if (adjacent_xy)
									{
										adjacent_xys=adjacent_xy;
									}
									return_code=0;
									display_message(ERROR_MESSAGE,"plane_delaunay.  "
										"Could not reallocate adjacent vertices");
								}
								DEALLOCATE(triangles[j]);
							}
							else
							{
								if (k>0)
								{
									triangles[j-k]=triangles[j];
								}
							}
							j++;
						}
						if (return_code&&(k>0))
						{
							number_of_triangles -= k;;
							/* determine new triangles */
							if (REALLOCATE(temp_triangles,triangles,
								struct Delaunay_triangle *,
								number_of_triangles+number_of_adjacent_vertices))
							{
								triangles=temp_triangles;
								adjacent_vertex=adjacent_vertices;
								adjacent_angle=adjacent_angles;
								adjacent_xy=adjacent_xys;
								adjacent_vertices[number_of_adjacent_vertices]=
									adjacent_vertices[0];
								adjacent_xys[2*number_of_adjacent_vertices]=adjacent_xys[0];
								adjacent_xys[2*number_of_adjacent_vertices+1]=adjacent_xys[1];
								k=0;
								while (return_code&&(k<number_of_adjacent_vertices))
								{
									if (triangle=ALLOCATE(triangles[number_of_triangles],
										struct Delaunay_triangle,1))
									{
										triangle->vertex_numbers[0]=i;
										(triangle->vertices)[0]=vertex[0];
										(triangle->vertices)[1]=vertex[1];
										triangle->vertex_numbers[1]= *adjacent_vertex;
										adjacent_vertex++;
										(triangle->vertices)[2]= *adjacent_xy;
										adjacent_xy++;
										(triangle->vertices)[3]= *adjacent_xy;
										adjacent_xy++;
										triangle->vertex_numbers[2]= *adjacent_vertex;
										(triangle->vertices)[4]=adjacent_xy[0];
										(triangle->vertices)[5]=adjacent_xy[1];
										k++;
										if (cylinder_calculate_circumcentre(triangle))
										{
											number_of_triangles++;
										}
										else
										{
											DEALLOCATE(triangles[number_of_triangles]);
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,
											"plane_delaunay.  Could not allocate triangle");
										return_code=0;
									}
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"plane_delaunay.  Could not reallocate triangles");
							}
						}
						DEALLOCATE(adjacent_vertices);
						DEALLOCATE(adjacent_angles);
						DEALLOCATE(adjacent_xys);
					}
#if defined (DEBUG)
					/*???debug */
					else
					{
						printf(
							"Vertices %d and %d are coincident.  Second vertex ignored\n",
							j,i);
					}
#endif /* defined (DEBUG) */
					i++;
					vertex += 2;
				}
				if (return_code)
				{
					/* return results */
					number_of_returned_triangles=0;
					for (i=0;i<number_of_triangles;i++)
					{
						j=2;
						while ((j>=0)&&(((triangles[i])->vertex_numbers)[j]>=0))
						{
							j--;
						}
						if (j<0)
						{
							number_of_returned_triangles++;
						}
					}
					if (ALLOCATE(returned_triangles,int,3*number_of_returned_triangles))
					{
						*triangles_address=returned_triangles;
						*number_of_triangles_address=number_of_returned_triangles;
						for (i=0;i<number_of_triangles;i++)
						{
							j=2;
							while ((j>=0)&&(((triangles[i])->vertex_numbers)[j]>=0))
							{
								j--;
							}
							if (j<0)
							{
								for (j=0;j<3;j++)
								{
									*returned_triangles=((triangles[i])->vertex_numbers)[j];
									returned_triangles++;
								}
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"plane_delaunay.  Could not allocate returned_triangles");
					}
				}
				/* tidy up */
				for (i=0;i<number_of_triangles;i++)
				{
					DEALLOCATE(triangles[i]);
				}
				DEALLOCATE(triangles);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"plane_delaunay.  Could not allocate initial triangles 2");
				i++;
				while (i<number_of_triangles)
				{
					DEALLOCATE(triangles[i]);
					i++;
				}
				DEALLOCATE(triangles);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"plane_delaunay.  Could not allocate initial triangles");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"plane_delaunay.  Invalid argument(s) %d %p %p %p",number_of_vertices,
			vertices,number_of_triangles_address,triangles_address);
	}
	LEAVE;

	return (return_code);
} /* plane_delaunay */
#endif /* defined (OLD_CODE) */
