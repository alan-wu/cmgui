/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if 1
#include "configure/cmgui_configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

#include "zinc/fieldmodule.h"
#include "zinc/graphic.h"
#include "zinc/graphicsmaterial.h"
#include "zinc/graphicsmodule.h"
#include "zinc/font.h"
#include "zinc/region.h"
#include "zinc/spectrum.h"
#include "zinc/tessellation.h"
#include "general/debug.h"
#include "general/enumerator.h"
#include "general/enumerator_private.hpp"
#include "general/message.h"
#include "command/parser.h"
#include "graphics/glyph.hpp"
#include "graphics/graphic.h"
#include "graphics/scene.h"
#include "graphics/scene_coordinate_system.hpp"
#include "graphics/tessellation.hpp"
#include "computed_field/computed_field_finite_element.h"
#include "graphics/auxiliary_graphics_types_app.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_set_app.h"
#include "graphics/spectrum_app.h"
#include "graphics/font.h"
#include "graphics/scene_app.h"
#include "graphics/material_app.h"
#include "finite_element/finite_element_region_app.h"
#include "graphics/tessellation_app.hpp"

enum Legacy_graphic_type
{
	LEGACY_GRAPHIC_NONE,
	LEGACY_GRAPHIC_POINT,
	LEGACY_GRAPHIC_NODE_POINTS,
	LEGACY_GRAPHIC_DATA_POINTS,
	LEGACY_GRAPHIC_ELEMENT_POINTS,
	LEGACY_GRAPHIC_CYLINDERS,
	LEGACY_GRAPHIC_ISO_SURFACES
};

int gfx_modify_scene_graphic(struct Parse_state *state,
	enum cmzn_graphic_type graphic_type,
	enum Legacy_graphic_type legacy_graphic_type, const char *help_text,
	struct Modify_scene_data *modify_scene_data,
	struct Scene_command_data *scene_command_data)
{
	if (!(state && scene_command_data && modify_scene_data))
	{
		display_message(ERROR_MESSAGE,
			"gfx_modify_scene_graphic.  Invalid argument(s)");
		return 0;
	}
	cmzn_graphic_id graphic = 0;
	if (modify_scene_data->modify_this_graphic)
	{
		graphic = cmzn_graphic_access(modify_scene_data->graphic);
		graphic_type = cmzn_graphic_get_graphic_type(graphic);
	}
	else
	{
		graphic = cmzn_scene_create_graphic_app(scene_command_data->scene,
			graphic_type, modify_scene_data->graphic);
		switch (legacy_graphic_type)
		{
		case LEGACY_GRAPHIC_NODE_POINTS:
			cmzn_graphic_set_domain_type(graphic, CMZN_FIELD_DOMAIN_NODES);
			break;
		case LEGACY_GRAPHIC_DATA_POINTS:
			cmzn_graphic_set_domain_type(graphic, CMZN_FIELD_DOMAIN_DATA);
			break;
		case LEGACY_GRAPHIC_ELEMENT_POINTS:
			cmzn_graphic_set_domain_type(graphic, CMZN_FIELD_DOMAIN_MESH_HIGHEST_DIMENSION);
			break;
		default:
			// do nothing
			break;
		}
		if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
		{
			cmzn_graphic_line_attributes_id line_attributes = cmzn_graphic_get_line_attributes(graphic);
			cmzn_graphic_line_attributes_set_shape(line_attributes, CMZN_GRAPHIC_LINE_ATTRIBUTES_SHAPE_CIRCLE_EXTRUSION);
			const double two = 2;
			// default scale factor is 2.0 for radius to diameter conversion
			cmzn_graphic_line_attributes_set_scale_factors(line_attributes, 1, &two);
			cmzn_graphic_line_attributes_destroy(&line_attributes);
		}
		cmzn_scene_set_graphics_defaults_gfx_modify(scene_command_data->scene, graphic);
		if (modify_scene_data->group)
		{
			cmzn_field_id subgroup_field = cmzn_graphic_get_subgroup_field(graphic);
			if (subgroup_field)
			{
				cmzn_field_destroy(&subgroup_field);
			}
			else
			{
				cmzn_graphic_set_subgroup_field(graphic,
					cmzn_field_group_base_cast(modify_scene_data->group));
			}
		}
	}
	if (!graphic)
	{
		display_message(ERROR_MESSAGE,
			"gfx_modify_scene_graphic.  Could not create graphic");
		return 0;
	}
	REACCESS(cmzn_graphic)(&(modify_scene_data->graphic), graphic);

	int return_code = 1;

	int number_of_valid_strings;
	const char **valid_strings;
	char *seed_nodeset_name = 0;
	if (graphic->seed_nodeset)
	{
		seed_nodeset_name = cmzn_nodeset_get_name(graphic->seed_nodeset);
	}
	cmzn_graphic_contours_id contours = cmzn_graphic_cast_contours(graphic);
	cmzn_graphic_streamlines_id streamlines = cmzn_graphic_cast_streamlines(graphic);
	cmzn_graphic_line_attributes_id line_attributes = cmzn_graphic_get_line_attributes(graphic);
	cmzn_field_id line_orientation_scale_field = 0;
	cmzn_graphic_point_attributes_id point_attributes = cmzn_graphic_get_point_attributes(graphic);
	cmzn_graphic_sampling_attributes_id sampling =
		((legacy_graphic_type == LEGACY_GRAPHIC_POINT) ||
		(legacy_graphic_type == LEGACY_GRAPHIC_NODE_POINTS) ||
		(legacy_graphic_type == LEGACY_GRAPHIC_DATA_POINTS)) ?
		0 : cmzn_graphic_get_sampling_attributes(graphic);

	cmzn_field_id isoscalar_field = 0;
	int number_of_isovalues = 0;
	double *isovalues = 0;
	double last_isovalue = 0;
	double first_isovalue = 0;
	int range_number_of_isovalues = 0;
	double decimation_threshold = 0;
	if (contours)
	{
		isoscalar_field = cmzn_graphic_contours_get_isoscalar_field(contours);
		number_of_isovalues = cmzn_graphic_contours_get_list_isovalues(contours, 0, 0);
		if (number_of_isovalues)
		{
			ALLOCATE(isovalues, double, number_of_isovalues);
			cmzn_graphic_contours_get_list_isovalues(contours, number_of_isovalues, isovalues);
		}
		range_number_of_isovalues = cmzn_graphic_contours_get_range_number_of_isovalues(contours);
		first_isovalue = cmzn_graphic_contours_get_range_first_isovalue(contours);
		last_isovalue = cmzn_graphic_contours_get_range_last_isovalue(contours);
		decimation_threshold = cmzn_graphic_contours_get_decimation_threshold(contours);
	}

	Option_table *option_table = CREATE(Option_table)();
	if (help_text)
	{
		Option_table_add_help(option_table, help_text);
	}

	/* as */
	char *name = cmzn_graphic_get_name(graphic);
	Option_table_add_entry(option_table,"as", &name,
		(void *)1,set_name);

	/* element points sample mode: cell_centres/cell_corners/cell_density/set_location */
	const char *sample_mode_string = 0;
	if (sampling)
	{
		cmzn_element_point_sample_mode sample_mode = cmzn_graphic_sampling_attributes_get_mode(sampling);
		sample_mode_string = ENUMERATOR_STRING(cmzn_element_point_sample_mode)(sample_mode);
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_element_point_sample_mode)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_element_point_sample_mode) *)NULL,
			(void *)NULL);
		Option_table_add_enumerator(option_table,number_of_valid_strings,
			valid_strings,&sample_mode_string);
		DEALLOCATE(valid_strings);
	}

	/* deprecated sample modes:
	   cell_density, cell_random -> cell_poisson (with warnings)
	   exact_xi -> set_location */
	const char *old_sample_mode_strings[] = { "cell_density", "cell_random", "exact_xi" };
	const enum cmzn_element_point_sample_mode old_to_new_sample_mode[] =
	{
		CMZN_ELEMENT_POINT_SAMPLE_CELL_POISSON,
		CMZN_ELEMENT_POINT_SAMPLE_CELL_POISSON,
		CMZN_ELEMENT_POINT_SAMPLE_SET_LOCATION
	};
	const int old_sample_mode_strings_count =
		sizeof(old_to_new_sample_mode) / sizeof(cmzn_element_point_sample_mode);
	const char *old_sample_mode_string = 0;
	if (sampling)
	{
		Option_table_add_enumerator(option_table, old_sample_mode_strings_count,
			old_sample_mode_strings, &old_sample_mode_string);
	}

	int three = 3;
	double glyph_offset[3];
	if (point_attributes)
	{
		cmzn_graphic_point_attributes_get_glyph_offset(point_attributes, 3, glyph_offset);
	}

	/* glyph centre: note equal to negative of point offset! */
	double glyph_centre[3];
	if (point_attributes && (legacy_graphic_type != LEGACY_GRAPHIC_NONE))
	{
		for (int i = 0; i < 3; i++)
		{
			glyph_centre[i] = (glyph_offset[i] != 0.0) ? -glyph_offset[i] : 0.0;
		}
		Option_table_add_double_vector_entry(option_table, "centre", glyph_centre, &three);
	}

	/* circle_discretization */
	int circle_discretization = 0;
	if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
	{
		Option_table_add_entry(option_table, "circle_discretization",
			(void *)&circle_discretization, (void *)NULL,
			set_circle_divisions);
	}

	/* constant_radius */
	double constant_radius = 0;
	if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
	{
		cmzn_graphic_line_attributes_get_base_size(line_attributes, 1, &constant_radius);
		constant_radius *= 0.5; // convert from diameter
		Option_table_add_double_entry(option_table, "constant_radius", &constant_radius);
	}

	/* coordinate */
	cmzn_field_id coordinate_field = cmzn_graphic_get_coordinate_field(graphic);
	Set_Computed_field_conditional_data set_coordinate_field_data;
	set_coordinate_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_coordinate_field_data.conditional_function = Computed_field_has_up_to_3_numerical_components;
	set_coordinate_field_data.conditional_function_user_data = (void *)NULL;
	Option_table_add_Computed_field_conditional_entry(option_table, "coordinate",
		&coordinate_field, &set_coordinate_field_data);

	/* coordinate system */
	cmzn_scene_coordinate_system coordinate_system = cmzn_graphic_get_coordinate_system(graphic);
	const char *coordinate_system_string =
		ENUMERATOR_STRING(cmzn_scene_coordinate_system)(coordinate_system);
	valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_scene_coordinate_system)(
		&number_of_valid_strings,
		(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_scene_coordinate_system) *)NULL,
		(void *)NULL);
	Option_table_add_enumerator(option_table, number_of_valid_strings,
		valid_strings, &coordinate_system_string);
	DEALLOCATE(valid_strings);

	/* data */
	cmzn_field_id data_field = cmzn_graphic_get_data_field(graphic);
	Set_Computed_field_conditional_data set_data_field_data;
	set_data_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_data_field_data.conditional_function = Computed_field_has_numerical_components;
	set_data_field_data.conditional_function_user_data = (void *)NULL;
	Option_table_add_Computed_field_conditional_entry(option_table, "data",
		&data_field, &set_data_field_data);

	/* decimation_threshold */
	if (contours)
	{
		Option_table_add_double_entry(option_table, "decimation_threshold",
			&decimation_threshold);
	}

	/* delete */
	Option_table_add_entry(option_table,"delete",
		&(modify_scene_data->delete_flag),NULL,set_char_flag);

	/* sample density */
	cmzn_field_id sample_density_field = 0;
	Set_Computed_field_conditional_data set_sample_density_field_data;
	set_sample_density_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_sample_density_field_data.conditional_function = Computed_field_is_scalar;
	set_sample_density_field_data.conditional_function_user_data = (void *)NULL;
	if (sampling)
	{
		sample_density_field = cmzn_graphic_sampling_attributes_get_density_field(sampling);
		Option_table_add_Computed_field_conditional_entry(option_table, "density",
			&sample_density_field, &set_sample_density_field_data);
	}

	/* discretization */
	int element_divisions_size = 0;
	int *element_divisions = 0;
	if ((legacy_graphic_type == LEGACY_GRAPHIC_ELEMENT_POINTS) ||
		(graphic_type == CMZN_GRAPHIC_STREAMLINES))
	{
		Option_table_add_divisions_entry(option_table, "discretization",
			&element_divisions, &element_divisions_size);
	}

	/* domain type */
	cmzn_field_domain_type domain_type = cmzn_graphic_get_domain_type(graphic);
	const char *domain_type_string = ENUMERATOR_STRING(cmzn_field_domain_type)(domain_type);
	if ((legacy_graphic_type != LEGACY_GRAPHIC_POINT) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_NODE_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_DATA_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_ELEMENT_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_ISO_SURFACES))
	{
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_field_domain_type)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_field_domain_type) *)NULL,
			(void *)NULL);
		Option_table_add_enumerator(option_table, number_of_valid_strings,
			valid_strings, &domain_type_string);
		DEALLOCATE(valid_strings);
	}

	/* deprecated streamline_type ellipse/line/rectangle/ribbon/cylinder
	 * (replaced with line_shape, some with same token so not migrated) */
	const char *streamline_type_strings[3] = { "ellipse", "rectangle", "cylinder" };
	const struct { enum cmzn_graphic_line_attributes_shape shape; FE_value thickness_to_width_ratio; }
		streamline_type_to_line_shape[] =
		{
			{ CMZN_GRAPHIC_LINE_ATTRIBUTES_SHAPE_CIRCLE_EXTRUSION, 0.2 },
			{ CMZN_GRAPHIC_LINE_ATTRIBUTES_SHAPE_SQUARE_EXTRUSION, 0.2 },
			{ CMZN_GRAPHIC_LINE_ATTRIBUTES_SHAPE_CIRCLE_EXTRUSION, 1.0 }
		};
	const char *streamline_type_string = 0;
	if (streamlines)
	{
		Option_table_add_enumerator(option_table, 3, streamline_type_strings, &streamline_type_string);
	}

	/* exterior */
	char exterior_flag = static_cast<char>(cmzn_graphic_is_exterior(graphic));
	if ((legacy_graphic_type != LEGACY_GRAPHIC_POINT) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_NODE_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_DATA_POINTS))
	{
		Option_table_add_entry(option_table, "exterior", &exterior_flag,
			NULL, set_char_flag);
	}

	/* face */
	enum cmzn_element_face_type face_type = CMZN_ELEMENT_FACE_ALL;
	if ((legacy_graphic_type != LEGACY_GRAPHIC_POINT) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_NODE_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_DATA_POINTS))
	{
		face_type = cmzn_graphic_get_face(graphic);
		Option_table_add_entry(option_table,"face", &face_type,
			NULL, set_graphic_face_type);
	}

	/* first_iso_value */
	if (contours)
	{
		Option_table_add_double_entry(option_table,"first_iso_value",
			&first_isovalue);
	}

	/* font */
	char *font_name = (char *)NULL;
	if (point_attributes)
	{
		Option_table_add_name_entry(option_table, "font", &font_name);
	}

	/* glyph */
	char *glyph_name = 0;
	if (point_attributes)
	{
		cmzn_glyph_id glyph = cmzn_graphic_point_attributes_get_glyph(point_attributes);
		if (glyph)
		{
			glyph_name = cmzn_glyph_get_name(glyph);
			cmzn_glyph_destroy(&glyph);
		}
		Option_table_add_string_entry(option_table, "glyph", &glyph_name, " GLYPH_NAME|none");
	}

	/* deprecated: glyph scaling mode constant/scalar/vector/axes/general (redundant) */
	const char *deprecated_glyph_scaling_mode_strings[] = { "constant", "scalar", "vector", "axes", "general" };
	const char *glyph_scaling_mode_string = 0;
	if ((legacy_graphic_type != LEGACY_GRAPHIC_NONE) && point_attributes)
	{
		Option_table_add_enumerator(option_table, /*number_of_valid_strings*/5,
			deprecated_glyph_scaling_mode_strings, &glyph_scaling_mode_string);
	}

	/* iso_scalar */
	Set_Computed_field_conditional_data set_isoscalar_field_data;
	set_isoscalar_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_isoscalar_field_data.conditional_function = Computed_field_is_scalar;
	set_isoscalar_field_data.conditional_function_user_data = (void *)NULL;
	if (contours)
	{
		Option_table_add_Computed_field_conditional_entry(option_table, "iso_scalar",
			&isoscalar_field, &set_isoscalar_field_data);
	}

	/* iso_values */
	if (contours)
	{
		Option_table_add_variable_length_double_vector_entry(option_table,
			"iso_values", &number_of_isovalues, &isovalues);
	}

	/* last_iso_value */
	if (contours)
	{
		Option_table_add_double_entry(option_table,"last_iso_value",
			&last_isovalue);
	}

	/* label */
	cmzn_field_id label_field = 0;
	Set_Computed_field_conditional_data set_label_field_data;
	set_label_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_label_field_data.conditional_function = (MANAGER_CONDITIONAL_FUNCTION(Computed_field) *)NULL;
	set_label_field_data.conditional_function_user_data = (void *)NULL;
	if (point_attributes)
	{
		label_field = cmzn_graphic_point_attributes_get_label_field(point_attributes);
		Option_table_add_Computed_field_conditional_entry(option_table, "label",
			&label_field, &set_label_field_data);
	}

	/* label_offset */
	double label_offset[3];
	if (point_attributes)
	{
		cmzn_graphic_point_attributes_get_label_offset(point_attributes, 3, label_offset);
		Option_table_add_double_vector_entry(option_table, "label_offset", label_offset, &three);
	}

	/* label_text */
	struct Multiple_strings label_strings = { /*number_of_strings*/0, (char **)0 };
	if (point_attributes)
	{
		Option_table_add_multiple_strings_entry(option_table, "label_text",
			&label_strings, " LABEL_STRING [& LABEL_STRING [& ...]]");
	}

	/* ldensity */
	Set_Computed_field_conditional_data set_label_density_field_data;
	set_label_density_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_label_density_field_data.conditional_function = Computed_field_has_up_to_3_numerical_components;
	set_label_density_field_data.conditional_function_user_data = (void *)NULL;
	if (point_attributes)
	{
		Option_table_add_Computed_field_conditional_entry(option_table, "ldensity",
			&(graphic->label_density_field), &set_label_density_field_data);
	}

	/* streamline length */
	double streamline_length = 0;
	if (streamlines)
	{
		streamline_length = cmzn_graphic_streamlines_get_track_length(streamlines);
		Option_table_add_non_negative_double_entry(option_table,"length", &streamline_length);
	}

	/* line shape: line/ribbon/circle_extrusion/square_extrusion */
	const char *line_shape_string = 0;
	if (line_attributes)
	{
		line_shape_string = ENUMERATOR_STRING(cmzn_graphic_line_attributes_shape)(
			cmzn_graphic_line_attributes_get_shape(line_attributes));
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_graphic_line_attributes_shape)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_graphic_line_attributes_shape) *)0,
			(void *)NULL);
		Option_table_add_enumerator(option_table, number_of_valid_strings,
			valid_strings, &line_shape_string);
		DEALLOCATE(valid_strings);
	}

	/* line_base_size */
	const int line_base_size_count = 2;
	double line_base_size[2] = { 0.0, 0.0 };
	if (line_attributes && (legacy_graphic_type != LEGACY_GRAPHIC_CYLINDERS))
	{
		cmzn_graphic_line_attributes_get_base_size(line_attributes, 2, line_base_size);
		Option_table_add_double_product_entry(option_table, "line_base_size",
			line_base_size_count, line_base_size);
	}

	/* line_scale_factors */
	const int line_scale_factors_count = 2;
	double line_scale_factors[2] = { 1.0, 1.0 };
	if (line_attributes && (legacy_graphic_type != LEGACY_GRAPHIC_CYLINDERS))
	{
		cmzn_graphic_line_attributes_get_scale_factors(line_attributes, 2, line_scale_factors);
		Option_table_add_double_product_entry(option_table, "line_scale_factors",
			line_scale_factors_count, line_scale_factors);
	}

	/* line_width */
	double line_width = cmzn_graphic_get_render_line_width(graphic);
	Option_table_add_positive_double_entry(option_table, "line_width", &line_width);

	/* material */
	cmzn_graphics_material_id material = cmzn_graphic_get_material(graphic);
	Option_table_add_set_Material_entry(option_table, "material", &material,
		scene_command_data->material_module);

	/* glyph repeat mode REPEAT_NONE|REPEAT_AXES_2D|REPEAT_AXES_3D|REPEAT_MIRROR */
	const char *glyph_repeat_mode_string = 0;
	if (point_attributes)
	{
		glyph_repeat_mode_string = ENUMERATOR_STRING(cmzn_glyph_repeat_mode)(
			cmzn_graphic_point_attributes_get_glyph_repeat_mode(point_attributes));
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_glyph_repeat_mode)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_glyph_repeat_mode) *)NULL,
			(void *)NULL);
		Option_table_add_enumerator(option_table, number_of_valid_strings,
			valid_strings, &glyph_repeat_mode_string);
		DEALLOCATE(valid_strings);
	}

	/* native_discretization (tessellation_field) */
	cmzn_field_id tessellation_field = cmzn_graphic_get_tessellation_field(graphic);
	Set_Computed_field_conditional_data set_tessellation_field_data;
	set_tessellation_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_tessellation_field_data.conditional_function = (MANAGER_CONDITIONAL_FUNCTION(Computed_field) *)NULL;
	set_tessellation_field_data.conditional_function_user_data = (void *)NULL;
	if ((legacy_graphic_type != LEGACY_GRAPHIC_POINT) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_NODE_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_DATA_POINTS))
	{
		tessellation_field = cmzn_graphic_get_tessellation_field(graphic);
		Option_table_add_Computed_field_conditional_entry(option_table, "native_discretization",
			&tessellation_field, &set_tessellation_field_data);
	}

	/* no_data/field_scalar/magnitude_scalar/travel_scalar */
	Streamline_data_type streamline_data_type = STREAM_NO_DATA;
	const char *streamline_data_type_string =
		ENUMERATOR_STRING(Streamline_data_type)(streamline_data_type);
	if (graphic_type == CMZN_GRAPHIC_STREAMLINES)
	{
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(Streamline_data_type)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(Streamline_data_type) *)NULL,
			(void *)NULL);
		Option_table_add_enumerator(option_table, number_of_valid_strings,
			valid_strings, &streamline_data_type_string);
		DEALLOCATE(valid_strings);
	}

	/* glyph offset */
	if (point_attributes && (legacy_graphic_type == LEGACY_GRAPHIC_NONE))
	{
		Option_table_add_double_vector_entry(option_table, "offset", glyph_offset, &three);
	}

	/* orientation */
	cmzn_field_id orientation_scale_field = 0;
	Set_Computed_field_conditional_data set_orientation_scale_field_data;
	set_orientation_scale_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_orientation_scale_field_data.conditional_function = Computed_field_is_orientation_scale_capable;
	set_orientation_scale_field_data.conditional_function_user_data = (void *)NULL;
	if (point_attributes)
	{
		orientation_scale_field = cmzn_graphic_point_attributes_get_orientation_scale_field(point_attributes);
		Option_table_add_Computed_field_conditional_entry(option_table, "orientation",
			&orientation_scale_field, &set_orientation_scale_field_data);
	}

	/* point_size */
	double point_size = cmzn_graphic_get_render_point_size(graphic);
	Option_table_add_positive_double_entry(option_table, "point_size", &point_size);

	/* position */
	Option_table_add_entry(option_table,"position",
		&(modify_scene_data->position),NULL,set_int_non_negative);

	/* radius_scalar */
	Set_Computed_field_conditional_data set_radius_scalar_field_data;
	set_radius_scalar_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_radius_scalar_field_data.conditional_function = Computed_field_is_scalar;
	set_radius_scalar_field_data.conditional_function_user_data = (void *)NULL;
	if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
	{
		line_orientation_scale_field =
			cmzn_graphic_line_attributes_get_orientation_scale_field(line_attributes);
		Option_table_add_Computed_field_conditional_entry(option_table, "radius_scalar",
			&line_orientation_scale_field, &set_radius_scalar_field_data);
	}

	/* range_number_of_iso_values */
	if (contours)
	{
		Option_table_add_int_positive_entry(option_table,
			"range_number_of_iso_values", &range_number_of_isovalues);
	}

	/* render_polygon_mode: render_shaded|render_wireframe */
	const char *render_polygon_mode_string = 0;
	cmzn_graphic_render_polygon_mode render_polygon_mode =  cmzn_graphic_get_render_polygon_mode(graphic);
	render_polygon_mode_string = ENUMERATOR_STRING(cmzn_graphic_render_polygon_mode)(render_polygon_mode);
	valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_graphic_render_polygon_mode)(
		&number_of_valid_strings,
		(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_graphic_render_polygon_mode) *)NULL, (void *)NULL);
	Option_table_add_enumerator(option_table,number_of_valid_strings,
		valid_strings,&render_polygon_mode_string);
	DEALLOCATE(valid_strings);

	/* forward_track|reverse_track */
	const char *streamlines_track_direction_string = 0;
	if (streamlines)
	{
		streamlines_track_direction_string = ENUMERATOR_STRING(cmzn_graphic_streamlines_track_direction)(
			cmzn_graphic_streamlines_get_track_direction(streamlines));
		valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_graphic_streamlines_track_direction)(
			&number_of_valid_strings,
			(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_graphic_streamlines_track_direction) *)0, (void *)0);
		Option_table_add_enumerator(option_table, number_of_valid_strings,
			valid_strings, &streamlines_track_direction_string);
		DEALLOCATE(valid_strings);
	}

	/* deprecated cylinder radius scale_factor (replaced with line_scale_factors) */
	double radius_scale_factor = 0;
	if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
	{
		cmzn_graphic_line_attributes_get_scale_factors(line_attributes, 1, &radius_scale_factor);
		radius_scale_factor *= 0.5; // convert from diameter
		Option_table_add_entry(option_table, "scale_factor",
			&radius_scale_factor, NULL, set_double);
	}

	/* glyph scale_factors */
	double glyph_scale_factors[3];
	if (point_attributes)
	{
		cmzn_graphic_point_attributes_get_scale_factors(point_attributes, 3, glyph_scale_factors);
		Option_table_add_double_product_entry(option_table, "scale_factors", 3, glyph_scale_factors);
	}

	/* secondary_material (was: multipass_pass1_material) */
	if (graphic_type == CMZN_GRAPHIC_LINES)
	{
		Option_table_add_set_Material_entry(option_table, "secondary_material", &(graphic->secondary_material),
			scene_command_data->material_module);
	}

	/* seed_element */
	if (graphic_type == CMZN_GRAPHIC_STREAMLINES)
	{
		Option_table_add_entry(option_table, "seed_element",
			&(graphic->seed_element), cmzn_region_get_FE_region(scene_command_data->region),
			set_FE_element_top_level_FE_region);
	}

	// seed_node_mesh_location_field
	Set_Computed_field_conditional_data set_seed_mesh_location_field_data;
	set_seed_mesh_location_field_data.conditional_function = Computed_field_has_value_type_mesh_location;
	set_seed_mesh_location_field_data.conditional_function_user_data = (void *)NULL;
	set_seed_mesh_location_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	if (graphic_type == CMZN_GRAPHIC_STREAMLINES)
	{
		Option_table_add_Computed_field_conditional_entry(option_table, "seed_node_mesh_location_field",
			&(graphic->seed_node_mesh_location_field), &set_seed_mesh_location_field_data);
	}

	// seed_nodeset
	if (graphic_type == CMZN_GRAPHIC_STREAMLINES)
	{
		Option_table_add_string_entry(option_table, "seed_nodeset", &seed_nodeset_name,
			" NODE_GROUP_FIELD_NAME|[GROUP_NAME.]nodes|datapoints|none");
	}

	/* select_mode */
	enum cmzn_graphic_select_mode select_mode = cmzn_graphic_get_select_mode(graphic);
	const char *select_mode_string = ENUMERATOR_STRING(cmzn_graphic_select_mode)(select_mode);
	valid_strings = ENUMERATOR_GET_VALID_STRINGS(cmzn_graphic_select_mode)(
		&number_of_valid_strings,
		(ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_graphic_select_mode) *)NULL,
		(void *)NULL);
	Option_table_add_enumerator(option_table,number_of_valid_strings,
		valid_strings,&select_mode_string);
	DEALLOCATE(valid_strings);

	/* selected_material */
	cmzn_graphics_material_id selected_material = cmzn_graphic_get_selected_material(graphic);
	Option_table_add_set_Material_entry(option_table, "selected_material", &selected_material,
		scene_command_data->material_module);

	/* glyph base size */
	double glyph_base_size[3];
	if (point_attributes)
	{
		cmzn_graphic_point_attributes_get_base_size(point_attributes, 3, glyph_base_size);
		Option_table_add_double_product_entry(option_table, "size", 3, glyph_base_size);
	}

	/* spectrum */
	cmzn_spectrum_id spectrum = cmzn_graphic_get_spectrum(graphic);
	Option_table_add_entry(option_table,"spectrum",
		&spectrum, scene_command_data->spectrum_manager,
		set_Spectrum);

	/* subgroup field */
	cmzn_field_id subgroup_field = cmzn_graphic_get_subgroup_field(graphic);
	Set_Computed_field_conditional_data set_subgroup_field_data;
	set_subgroup_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_subgroup_field_data.conditional_function = Computed_field_is_scalar;
	set_subgroup_field_data.conditional_function_user_data = (void *)NULL;
	Option_table_add_Computed_field_conditional_entry(option_table, "subgroup",
		&subgroup_field, &set_subgroup_field_data);

	/* tessellation */
	cmzn_tessellation_id tessellation = cmzn_graphic_get_tessellation(graphic);
	if ((legacy_graphic_type != LEGACY_GRAPHIC_POINT) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_NODE_POINTS) &&
		(legacy_graphic_type != LEGACY_GRAPHIC_DATA_POINTS))
	{
		Option_table_add_cmzn_tessellation_entry(option_table, "tessellation",
			scene_command_data->tessellation_module, &tessellation);
	}

	/* texture_coordinates */
	cmzn_field_id texture_coordinate_field = cmzn_graphic_get_texture_coordinate_field(graphic);
	Set_Computed_field_conditional_data set_texture_coordinate_field_data;
	set_texture_coordinate_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_texture_coordinate_field_data.conditional_function = Computed_field_has_up_to_3_numerical_components;
	set_texture_coordinate_field_data.conditional_function_user_data = (void *)NULL;
	if ((graphic_type == CMZN_GRAPHIC_SURFACES) ||
		(graphic_type == CMZN_GRAPHIC_CONTOURS) ||
		(graphic_type == CMZN_GRAPHIC_LINES))
	{
		Option_table_add_Computed_field_conditional_entry(option_table, "texture_coordinates",
			&texture_coordinate_field, &set_texture_coordinate_field_data);
	}

	/* deprecated use_elements/use_faces/use_lines (translated into domain type) */
	const char *use_element_type_strings[] = { "use_elements", "use_faces", "use_lines" };
	const enum cmzn_field_domain_type use_element_type_to_domain_type[] =
	{
		CMZN_FIELD_DOMAIN_MESH_HIGHEST_DIMENSION,
		CMZN_FIELD_DOMAIN_MESH_2D,
		CMZN_FIELD_DOMAIN_MESH_1D
	};
	const char *use_element_type_string = 0;
	if ((legacy_graphic_type == LEGACY_GRAPHIC_ISO_SURFACES) ||
		(legacy_graphic_type == LEGACY_GRAPHIC_ELEMENT_POINTS))
	{
		Option_table_add_enumerator(option_table, 3, use_element_type_strings, &use_element_type_string);
	}

	/* variable_scale */
	cmzn_field_id signed_scale_field = 0;
	Set_Computed_field_conditional_data set_signed_scale_field_data;
	set_signed_scale_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_signed_scale_field_data.conditional_function = Computed_field_has_up_to_3_numerical_components;
	set_signed_scale_field_data.conditional_function_user_data = (void *)NULL;
	if (point_attributes)
	{
		signed_scale_field = cmzn_graphic_point_attributes_get_signed_scale_field(point_attributes);
		Option_table_add_Computed_field_conditional_entry(option_table, "variable_scale",
			&signed_scale_field, &set_signed_scale_field_data);
	}

	/* vector */
	cmzn_field_id stream_vector_field = 0;
	Set_Computed_field_conditional_data set_stream_vector_field_data;
	set_stream_vector_field_data.computed_field_manager = scene_command_data->computed_field_manager;
	set_stream_vector_field_data.conditional_function = Computed_field_is_stream_vector_capable;
	set_stream_vector_field_data.conditional_function_user_data = (void *)NULL;
	if (streamlines)
	{
		stream_vector_field = cmzn_graphic_streamlines_get_stream_vector_field(streamlines);
		Option_table_add_Computed_field_conditional_entry(option_table, "vector",
			&stream_vector_field, &set_stream_vector_field_data);
	}

	/* visible/invisible */
	int visibility_flag = static_cast<int>(cmzn_graphic_get_visibility_flag(graphic));
	Option_table_add_switch(option_table, "visible", "invisible", &visibility_flag);

	/* deprecated: streamline width (replaced with line_base_size) */
	double streamline_width = 0.0;
	if (graphic_type == CMZN_GRAPHIC_STREAMLINES)
	{
		Option_table_add_double_entry(option_table, "width", &streamline_width);
	}

	/* xi (sample location) */
	double *sample_location = 0;
	int sample_location_components = 0;
	if (sampling)
	{
		sample_location_components = 3;
		ALLOCATE(sample_location, double, sample_location_components);
		cmzn_graphic_sampling_attributes_get_location(sampling, sample_location_components, sample_location);
		Option_table_add_variable_length_double_vector_entry(option_table, "xi",
			&sample_location_components, &sample_location);
	}

	if ((return_code=Option_table_multi_parse(option_table,state)))
	{
		if (name)
		{
			cmzn_graphic_set_name(graphic, name);
		}
		cmzn_graphic_set_subgroup_field(graphic, subgroup_field);
		cmzn_graphic_set_coordinate_field(graphic, coordinate_field);
		cmzn_graphic_set_data_field(graphic, data_field);
		bool use_spectrum = (0 != data_field);
		cmzn_graphic_set_exterior(graphic, (0 != exterior_flag));
		cmzn_graphic_set_face(graphic, face_type);
		cmzn_graphic_set_tessellation(graphic, tessellation);
		cmzn_graphic_set_tessellation_field(graphic, tessellation_field);
		if ((graphic_type == CMZN_GRAPHIC_SURFACES) ||
			(graphic_type == CMZN_GRAPHIC_CONTOURS) ||
			(graphic_type == CMZN_GRAPHIC_LINES))
		{
			cmzn_graphic_set_texture_coordinate_field(graphic, texture_coordinate_field);
		}
		cmzn_graphic_set_material(graphic, material);
		cmzn_graphic_set_render_line_width(graphic, line_width);
		cmzn_graphic_set_render_point_size(graphic, point_size);
		cmzn_graphic_set_selected_material(graphic, selected_material);

		if (contours)
		{
			if (!isoscalar_field)
			{
				display_message(ERROR_MESSAGE,
					"gfx_modify_scene_iso_surfaces.  Missing iso_scalar field");
				return_code=0;
			}
			cmzn_graphic_contours_set_isoscalar_field(contours, isoscalar_field);
			if (((0 < range_number_of_isovalues) && isovalues) ||
				((0 >= range_number_of_isovalues) && (0 == isovalues)))
			{
				display_message(ERROR_MESSAGE,
					"Must specify either <iso_values> OR <range_number_of_iso_values>, <first_iso_value> and <last_iso_value>.");
				return_code = 0;
			}
			else if (range_number_of_isovalues)
			{
				cmzn_graphic_contours_set_range_isovalues(contours, range_number_of_isovalues, first_isovalue, last_isovalue);
			}
			else
			{
				cmzn_graphic_contours_set_list_isovalues(contours, number_of_isovalues, isovalues);
			}
			cmzn_graphic_contours_set_decimation_threshold(contours, decimation_threshold);
		}

		cmzn_graphic_set_visibility_flag(graphic, 0 != visibility_flag);

		if (sampling)
		{
			cmzn_element_point_sample_mode sample_mode;
			STRING_TO_ENUMERATOR(cmzn_element_point_sample_mode)(
				sample_mode_string, &sample_mode);
			if (old_sample_mode_string)
			{
				for (int i = 0; i < old_sample_mode_strings_count; ++i)
				{
					if (old_sample_mode_string == old_sample_mode_strings[i])
					{
						sample_mode = old_to_new_sample_mode[i];
					}
				}
				if (CMZN_ELEMENT_POINT_SAMPLE_CELL_POISSON == sample_mode)
				{
					display_message(WARNING_MESSAGE, "Migrating obsolete sampling mode '%s' to cell_poisson", old_sample_mode_string);
				}
			}
			if ((CMZN_ELEMENT_POINT_SAMPLE_CELL_POISSON == sample_mode) &&
				(0 == sample_density_field))
			{
				display_message(ERROR_MESSAGE,
					"No density field specified for sample mode 'cell_poisson'");
				return_code = 0;
			}
			cmzn_graphic_sampling_attributes_set_mode(sampling, sample_mode);
			cmzn_graphic_sampling_attributes_set_density_field(sampling, sample_density_field);
			cmzn_graphic_sampling_attributes_set_location(sampling, sample_location_components, sample_location);
		}

		if ((graphic_type != CMZN_GRAPHIC_LINES) &&
			(graphic_type != CMZN_GRAPHIC_SURFACES))
		{
			STRING_TO_ENUMERATOR(cmzn_field_domain_type)(domain_type_string, &domain_type);
			cmzn_graphic_set_domain_type(graphic, domain_type);
		}
		// translate legacy use_element_type to domain_type
		if (use_element_type_string)
		{
			for (int i = 0; i < 3; i++)
			{
				if (fuzzy_string_compare_same_length(use_element_type_string, use_element_type_strings[i]))
				{
					domain_type = use_element_type_to_domain_type[i];
					cmzn_graphic_set_domain_type(graphic, domain_type);
					break;
				}
			}
		}

		if (point_attributes)
		{
			cmzn_glyph_repeat_mode glyph_repeat_mode = CMZN_GLYPH_REPEAT_NONE;
			STRING_TO_ENUMERATOR(cmzn_glyph_repeat_mode)(glyph_repeat_mode_string, &glyph_repeat_mode);
			if (legacy_graphic_type == LEGACY_GRAPHIC_POINT)
			{
				// scale factors and orientation were never used, and offset was in model units.
				// now the offset is in multiples of glyph axes, i.e. need divide by the base size
				// to work as before
				for (int i = 0; i < 3; i++)
				{
					if (glyph_base_size[i] != 0.0)
					{
						glyph_centre[i] /= glyph_base_size[i];
					}
				}
			}

			cmzn_glyph_id glyph = 0;
			if (glyph_name)
			{
				glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, glyph_name);
			}
			if (glyph_name && (!glyph) && (0 != strcmp(glyph_name, "none")))
			{
				if ((0 == strcmp(glyph_name, "mirror_arrow_solid")) ||
					(0 == strcmp(glyph_name, "mirror_cone")) ||
					(0 == strcmp(glyph_name, "mirror_line")))
				{
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, glyph_name + 7);
					glyph_repeat_mode = CMZN_GLYPH_REPEAT_MIRROR;
				}
				else if ((0 == strcmp(glyph_name, "arrow_line")) ||
					(0 == strcmp(glyph_name, "mirror_arrow_line")))
				{
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, "arrow");
					if (glyph_name[0] == 'm')
					{
						glyph_repeat_mode = CMZN_GLYPH_REPEAT_MIRROR;
					}
					// fix lateral scaling of old arrow_line glyph; now unit width arrow
					glyph_base_size[1] *= 0.25;
					glyph_base_size[2] *= 0.25;
					glyph_scale_factors[1] *= 0.25;
					glyph_scale_factors[2] *= 0.25;
				}
				else if (0 == strcmp(glyph_name, "cylinder6"))
				{
					// just using normal cylinder, default circle divisions
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, "cylinder");
				}
				else if (0 == strcmp(glyph_name, "cylinder_hires"))
				{
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, "cylinder");
					circle_discretization = 48;
				}
				else if (0 == strcmp(glyph_name, "cylinder_solid_hires"))
				{
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, "cylinder_solid");
					circle_discretization = 48;
				}
				else if (0 == strcmp(glyph_name, "sphere_hires"))
				{
					glyph = cmzn_glyph_module_find_glyph_by_name(scene_command_data->glyph_module, "sphere");
					circle_discretization = 48;
				}
				if (!glyph)
				{
					display_message(ERROR_MESSAGE, "Unknown glyph: ", glyph_name);
					return_code = 0;
				}
			}
			cmzn_graphic_point_attributes_set_glyph(point_attributes, glyph);
			cmzn_glyph_destroy(&glyph);
			cmzn_graphic_point_attributes_set_glyph_repeat_mode(point_attributes, glyph_repeat_mode);
			cmzn_graphic_point_attributes_set_orientation_scale_field(point_attributes, orientation_scale_field);

			if (legacy_graphic_type != LEGACY_GRAPHIC_NONE)
			{
				// reverse centre to get offset:
				for (int i = 0; i < 3; i++)
				{
					glyph_offset[i] = (glyph_centre[i] != 0.0) ? -glyph_centre[i] : 0.0;
				}
			}
			cmzn_graphic_point_attributes_set_glyph_offset(point_attributes, 3, glyph_offset);
			cmzn_graphic_point_attributes_set_base_size(point_attributes, 3, glyph_base_size);
			cmzn_graphic_point_attributes_set_scale_factors(point_attributes, 3, glyph_scale_factors);
			cmzn_graphic_point_attributes_set_signed_scale_field(point_attributes, signed_scale_field);
			cmzn_graphic_point_attributes_set_label_field(point_attributes, label_field);
			cmzn_graphic_point_attributes_set_label_offset(point_attributes, 3, label_offset);
			if (0 < label_strings.number_of_strings)
			{
				for (int i = 0; i < label_strings.number_of_strings; ++i)
				{
					cmzn_graphic_point_attributes_set_label_text(point_attributes, i + 1, label_strings.strings[i]);
				}
			}
			if (font_name)
			{
				cmzn_font_module_id font_module = cmzn_graphics_module_get_font_module(
					scene_command_data->graphics_module);
				cmzn_font *new_font = cmzn_font_module_find_font_by_name(
					font_module, font_name);
				cmzn_font_module_destroy(&font_module);
				if (new_font)
				{
					cmzn_graphic_point_attributes_set_font(point_attributes, new_font);
					cmzn_font_destroy(&new_font);
				}
				else
				{
					display_message(WARNING_MESSAGE, "Unknown font: %s", font_name);
				}
			}
		}

		STRING_TO_ENUMERATOR(cmzn_scene_coordinate_system)(
			coordinate_system_string, &coordinate_system);
		cmzn_graphic_set_coordinate_system(graphic, coordinate_system);

		cmzn_graphic_render_polygon_mode render_polygon_mode;
		STRING_TO_ENUMERATOR(cmzn_graphic_render_polygon_mode)(render_polygon_mode_string, &render_polygon_mode);
		cmzn_graphic_set_render_polygon_mode(graphic, render_polygon_mode);

		STRING_TO_ENUMERATOR(cmzn_graphic_select_mode)(select_mode_string, &select_mode);
		cmzn_graphic_set_select_mode(graphic, select_mode);

		if ((0 != element_divisions_size) || (0 != circle_discretization))
		{
			cmzn_tessellation_module_id tessellationModule =
				cmzn_graphics_module_get_tessellation_module(scene_command_data->graphics_module);
			cmzn_tessellation_id fixedTessellation =
				cmzn_tessellation_module_find_or_create_fixed_tessellation(tessellationModule,
					element_divisions_size, element_divisions, circle_discretization,
					tessellation);
			cmzn_graphic_set_tessellation(graphic, fixedTessellation);
			cmzn_tessellation_destroy(&fixedTessellation);
			cmzn_tessellation_module_destroy(&tessellationModule);
		}

		if (legacy_graphic_type == LEGACY_GRAPHIC_CYLINDERS)
		{
			// convert radius to diameter
			line_base_size[1] = line_base_size[0] = 2.0*constant_radius;
			line_scale_factors[1] = line_scale_factors[0] = 2.0*radius_scale_factor;
		}

		if (streamlines)
		{
			if (!stream_vector_field)
			{
				display_message(INFORMATION_MESSAGE,"Must specify a vector before any streamlines can be created");
			}
			if (return_code && seed_nodeset_name)
			{
				cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(scene_command_data->region);
				cmzn_nodeset_id seed_nodeset =
					cmzn_fieldmodule_find_nodeset_by_name(field_module, seed_nodeset_name);
				if (seed_nodeset || (fuzzy_string_compare(seed_nodeset_name, "none")))
				{
					if (graphic->seed_nodeset)
					{
						cmzn_nodeset_destroy(&graphic->seed_nodeset);
					}
					// take over reference:
					graphic->seed_nodeset = seed_nodeset;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Unknown seed_nodeset %s", seed_nodeset_name);
					return_code = 0;
				}
				cmzn_fieldmodule_destroy(&field_module);
			}
			if ((graphic->seed_node_mesh_location_field && (!graphic->seed_nodeset)) ||
				((!graphic->seed_node_mesh_location_field) && graphic->seed_nodeset))
			{
				display_message(ERROR_MESSAGE,
					"Must specify both seed_nodeset and seed_node_mesh_location_field, or neither");
				return_code = 0;
			}
			if (return_code)
			{
				cmzn_graphic_streamlines_set_stream_vector_field(streamlines, stream_vector_field);
				cmzn_graphic_streamlines_set_track_length(streamlines, streamline_length);
				cmzn_graphic_streamlines_track_direction streamlines_track_direction;
				STRING_TO_ENUMERATOR(cmzn_graphic_streamlines_track_direction)(
					streamlines_track_direction_string, &streamlines_track_direction);
				cmzn_graphic_streamlines_set_track_direction(streamlines, streamlines_track_direction);
				// translate deprecated streamline_type to line_shape, and width to line_base_size[2] depending on type
				if (streamline_type_string)
				{
					for (int i = 0; i < 3; i++)
					{
						if (fuzzy_string_compare_same_length(streamline_type_string, streamline_type_strings[i]))
						{
							// set line_shape_string for processing below!
							line_shape_string = ENUMERATOR_STRING(cmzn_graphic_line_attributes_shape)(streamline_type_to_line_shape[i].shape);
							line_base_size[0] = streamline_width;
							line_base_size[1] = streamline_width*streamline_type_to_line_shape[i].thickness_to_width_ratio;
							break;
						}
					}
				}
				else if (streamline_width != 0.0)
				{
					// handle width of legacy ribbon shape
					line_base_size[1] = line_base_size[0] = streamline_width;
				}
				STRING_TO_ENUMERATOR(Streamline_data_type)(streamline_data_type_string, &streamline_data_type);
				if (data_field)
				{
					if (STREAM_FIELD_SCALAR != streamline_data_type)
					{
						display_message(WARNING_MESSAGE,
							"Must use field_scalar option with data; ensuring this");
						streamline_data_type=STREAM_FIELD_SCALAR;
					}
				}
				else
				{
					if (STREAM_FIELD_SCALAR == streamline_data_type)
					{
						display_message(WARNING_MESSAGE,
							"Must specify data field with field_scalar option");
						streamline_data_type=STREAM_NO_DATA;
					}
				}
				use_spectrum = (STREAM_NO_DATA != streamline_data_type);
				cmzn_graphic_set_streamline_data_type(graphic, streamline_data_type);
			}
		}
		if (use_spectrum)
		{
			if (spectrum)
			{
				cmzn_graphic_set_spectrum(graphic, spectrum);
			}
			else
			{
				cmzn_graphic_set_spectrum(graphic, scene_command_data->default_spectrum);
			}
		}
		else
		{
			cmzn_graphic_set_spectrum(graphic, static_cast<cmzn_spectrum *>(0));
		}

		if (line_attributes)
		{
			cmzn_graphic_line_attributes_shape line_shape;
			STRING_TO_ENUMERATOR(cmzn_graphic_line_attributes_shape)(line_shape_string, &line_shape);
			cmzn_graphic_line_attributes_set_shape(line_attributes, line_shape);
			cmzn_graphic_line_attributes_set_base_size(line_attributes, 2, line_base_size);
			cmzn_graphic_line_attributes_set_scale_factors(line_attributes, 2, line_scale_factors);
			cmzn_graphic_line_attributes_set_orientation_scale_field(line_attributes, line_orientation_scale_field);
		}
	}
	DESTROY(Option_table)(&option_table);
	if (!return_code)
	{
		/* parse error, help */
		cmzn_graphic_destroy(&(modify_scene_data->graphic));
	}
	if (glyph_name)
	{
		DEALLOCATE(glyph_name);
	}
	cmzn_field_destroy(&coordinate_field);
	cmzn_field_destroy(&data_field);
	cmzn_field_destroy(&isoscalar_field);
	cmzn_field_destroy(&line_orientation_scale_field);
	cmzn_field_destroy(&label_field);
	if (label_strings.strings)
	{
		for (int i = 0; i < label_strings.number_of_strings; ++i)
		{
			DEALLOCATE(label_strings.strings[i]);
		}
		DEALLOCATE(label_strings.strings);
	}
	cmzn_field_destroy(&orientation_scale_field);
	cmzn_field_destroy(&stream_vector_field);
	cmzn_field_destroy(&subgroup_field);
	cmzn_field_destroy(&signed_scale_field);
	cmzn_field_destroy(&tessellation_field);
	cmzn_field_destroy(&texture_coordinate_field);
	cmzn_field_destroy(&sample_density_field);
	if (sample_location)
	{
		DEALLOCATE(sample_location);
	}
	if (font_name)
	{
		DEALLOCATE(font_name);
	}
	if (seed_nodeset_name)
	{
		DEALLOCATE(seed_nodeset_name);
	}
	cmzn_graphic_destroy(&graphic);
	if (isovalues)
		DEALLOCATE(isovalues);
	cmzn_graphic_contours_destroy(&contours);
	cmzn_graphic_streamlines_destroy(&streamlines);
	cmzn_graphic_line_attributes_destroy(&line_attributes);
	cmzn_graphic_point_attributes_destroy(&point_attributes);
	cmzn_graphic_sampling_attributes_destroy(&sampling);
	cmzn_spectrum_destroy(&spectrum);
	cmzn_tessellation_destroy(&tessellation);
	cmzn_graphics_material_destroy(&material);
	cmzn_graphics_material_destroy(&selected_material);
	DEALLOCATE(name);
	return return_code;
}

int gfx_modify_scene_contours(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text =
		"Create contours i.e. isosurfaces in volume elements <domain_elements_3d> or isolines in face or surface elements "
		"<domain_elements_2d>.  "
		"The isosurface will be generated at the values in the elements where <iso_scalar> equals the iso values specified.  "
		"The iso values can be specified either as a list with the <iso_values> option "
		"or by specifying <range_number_of_iso_values>, <first_iso_value> and <last_iso_value>.  "
		"The <as> parameter allows a name to be specified for this setting.  "
		"The <coordinate> parameter optionally overrides the groups default coordinate field.  "
		"If a <data> field is specified then the <spectrum> is used to render the data values as colour on the generated isosurface.  "
		"If a <decimation_threshold> is specified then the resulting iso_surface will be decimated according to the threshold.  "
		"If <delete> is specified then if the graphic matches an existing setting (either by parameters or name) then it will be removed.  "
		"If <exterior> is specified then only faces with one parent will be selected when <use_faces> is specified.  "
		"If <face> is specified then only that face will be selected when <use_faces> is specified.  "
		"The <material> is used to render the surface.  "
		"You can specify the <position> the graphic has in the graphic list.  "
		"You can specify the <line_width>, this option only applies when <use_faces> is specified.  "
		"You can render a mesh as solid <render_shaded> or as a wireframe <render_wireframe>.  "
		"If <select_on> is active then the element tool will select the elements the iso_surface was generated from.  "
		"If <no_select> is active then the iso_surface cannot be selected.  "
		"If <draw_selected> is active then iso_surfaces will only be generated in elements that are selected.  "
		"Conversely, if <draw_unselected> is active then iso_surfaces will only be generated in elements that are not selected.  "
		"The <texture_coordinates> are used to lay out a texture if the <material> contains a texture.  "
		"A graphic can be made <visible> or <invisible>.  ";

	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_CONTOURS,
		LEGACY_GRAPHIC_NONE, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_cylinders(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_LINES,
		LEGACY_GRAPHIC_CYLINDERS, /*help_text*/(const char *)0,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_data_points(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text = "Deprecated; use points with domain_data option instead.";
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_POINTS,
		LEGACY_GRAPHIC_DATA_POINTS, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_element_points(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text = "Deprecated; use points with domain_elements* option instead.";
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_POINTS,
		LEGACY_GRAPHIC_ELEMENT_POINTS, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_iso_surfaces(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text = "Deprecated; use contours with domain_elements* option instead.";
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_CONTOURS,
		LEGACY_GRAPHIC_ISO_SURFACES, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_lines(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_LINES,
		LEGACY_GRAPHIC_NONE, /*help_text*/(const char *)0,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_node_points(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text = "Deprecated; use points with domain_nodes option instead.";
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_POINTS,
		LEGACY_GRAPHIC_NODE_POINTS, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_point(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	const char *help_text = "Deprecated; use points with domain_point option instead.";
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_POINTS,
		LEGACY_GRAPHIC_POINT, help_text,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_streamlines(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_STREAMLINES,
		LEGACY_GRAPHIC_NONE, /*help_text*/(const char *)0,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_surfaces(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_SURFACES,
		LEGACY_GRAPHIC_NONE, /*help_text*/(const char *)0,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

int gfx_modify_scene_points(struct Parse_state *state,
	void *modify_scene_data_void,void *scene_command_data_void)
{
	return gfx_modify_scene_graphic(state, CMZN_GRAPHIC_POINTS,
		LEGACY_GRAPHIC_NONE, /*help_text*/(const char *)0,
		reinterpret_cast<Modify_scene_data *>(modify_scene_data_void),
		reinterpret_cast<Scene_command_data *>(scene_command_data_void));
}

