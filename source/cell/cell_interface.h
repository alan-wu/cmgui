/*******************************************************************************
FILE : cell_interface.h

LAST MODIFIED : 02 February 2001

DESCRIPTION :
The Cell Interface.
==============================================================================*/
#if !defined (CELL_INTERFACE_H)
#define CELL_INTERFACE_H

#include "general/object.h"
#include "general/debug.h"
#include "time/time.h"
#include "user_interface/message.h"
#include "user_interface/user_interface.h"

#include "graphics/light_model.h"
#include "interaction/interactive_tool.h"
#include "selection/any_object_selection.h"

#define ROOT_ELEMENT_ID "ROOT_ELEMENT_UNIQUE_IDENTIFIER_13579"

#define WRITE_INDENT( indent_level ) \
{ \
  int write_indent_counter = 0; \
  while(write_indent_counter < indent_level) \
  { \
    display_message(INFORMATION_MESSAGE," "); \
    write_indent_counter++; \
  } \
}

/*
Module objects
--------------
*/
struct Cell_interface;
/*******************************************************************************
LAST MODIFIED : 26 October 2000

DESCRIPTION :
The Cell Interface main structure.
==============================================================================*/

/*
Global functions
----------------
*/
struct Cell_interface *CREATE(Cell_interface)(
	struct Any_object_selection *any_object_selection,
  struct Colour *background_colour,
  struct Graphical_material *default_graphical_material,
  struct Light *default_light,
  struct Light_model *default_light_model,
  struct Scene *default_scene,
  struct Spectrum *default_spectrum,
  struct Time_keeper *time_keeper,
  struct LIST(GT_object) *graphics_object_list,
  struct LIST(GT_object) *glyph_list,
	struct MANAGER(Interactive_tool) *interactive_tool_manager,
  struct MANAGER(Light) *light_manager,
  struct MANAGER(Light_model) *light_model_manager,
  struct MANAGER(Graphical_material) *graphical_material_manager,
  struct MANAGER(Scene) *scene_manager,
  struct MANAGER(Spectrum) *spectrum_manager,
  struct MANAGER(Texture) *texture_manager,
  struct User_interface *user_interface,
  XtCallbackProc exit_callback
#if defined (CELL_DISTRIBUTED)
  ,struct Element_point_ranges_selection *element_point_ranges_selection,
  struct Computed_field_package *computed_field_package,
  struct MANAGER(FE_element) *element_manager,
  struct MANAGER(GROUP(FE_element)) *element_group_manager,
  struct MANAGER(FE_field) *fe_field_manager
#endif /* defined (CELL_DISTRIBUTED) */
  );
/*******************************************************************************
LAST MODIFIED : 16 January 2001

DESCRIPTION :
Creates the Cell interface object.
==============================================================================*/
int DESTROY(Cell_interface)(struct Cell_interface **cell_interface_address);
/*******************************************************************************
LAST MODIFIED : 26 June 2000

DESCRIPTION :
Destroys the Cell_interface object.
==============================================================================*/
int Cell_interface_close_model(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 19 October 2000

DESCRIPTION :
Destroys all the current components and variables in the interface.
==============================================================================*/
void Cell_interface_terminate_XMLParser(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 29 June 2000

DESCRIPTION :
Terminates the XML Parser. Need to do this so that the parser is only ever
initialised once and terminated once for the enitre life of this application
instance. Required because the Xerces library can only be initialised and
terminated once!!
==============================================================================*/
int Cell_interface_read_model(struct Cell_interface *cell_interface,
  char *filename);
/*******************************************************************************
LAST MODIFIED : 28 June 2000

DESCRIPTION :
Reads in a Cell model. If no <filename> is given, then prompts for a file name.
==============================================================================*/
int Cell_interface_write_model(struct Cell_interface *cell_interface,
  char *filename);
/*******************************************************************************
LAST MODIFIED : 13 November 2000

DESCRIPTION :
Writes out a Cell model.
==============================================================================*/
int Cell_interface_write_model_to_ipcell_file(
  struct Cell_interface *cell_interface,char *filename);
/*******************************************************************************
LAST MODIFIED : 11 January 2001

DESCRIPTION :
Writes out a Cell model to a ipcell file
==============================================================================*/
int Cell_interface_list_components(
  struct Cell_interface *cell_interface,int full);
/*******************************************************************************
LAST MODIFIED : 09 July 2000

DESCRIPTION :
Lists out the current set of cell components. If <full> is not 0, then a full
listing is given, otherwise simply gives a list of names.
==============================================================================*/
int Cell_interface_list_variables(
  struct Cell_interface *cell_interface,int full);
/*******************************************************************************
LAST MODIFIED : 10 July 2000

DESCRIPTION :
Lists out the current set of cell variables. If <full> is not 0, then a full
listing is given, otherwise simply gives a list of names.
==============================================================================*/
int Cell_interface_list_XMLParser_properties(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 29 June 2000

DESCRIPTION :
Lists out the current set of XML parser properties
==============================================================================*/
int *Cell_interface_get_XMLParser_properties(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 02 July 2000

DESCRIPTION :
Gets the current set of XML parser properties
==============================================================================*/
int Cell_interface_set_XMLParser_properties(
  struct Cell_interface *cell_interface,int *properties);
/*******************************************************************************
LAST MODIFIED : 02 July 2000

DESCRIPTION :
Sets the current set of XML parser properties
==============================================================================*/
int Cell_interface_list_copy_tags(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 29 June 2000

DESCRIPTION :
Lists out the current set of copy tags
==============================================================================*/
char **Cell_interface_get_copy_tags(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 09 July 2000

DESCRIPTION :
Gets a copy of the current set of copy tags.
==============================================================================*/
int Cell_interface_set_copy_tags(
  struct Cell_interface *cell_interface,char **copy_tags);
/*******************************************************************************
LAST MODIFIED : 09 July 2000

DESCRIPTION :
Sets the current set of copy tags.
==============================================================================*/
int Cell_interface_list_ref_tags(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 29 June 2000

DESCRIPTION :
Lists out the current set of ref tags
==============================================================================*/
int Cell_interface_list_hierarchy(struct Cell_interface *cell_interface,
  int full,char *name);
/*******************************************************************************
LAST MODIFIED : 11 July 2000

DESCRIPTION :
Lists out the hierarchy described by the component with the given <name>, or
the root level component if no name is given.
==============================================================================*/
int Cell_interface_calculate_model(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 26 October 2000

DESCRIPTION :
Calculates the current cell model.
==============================================================================*/
int Cell_interface_pop_up_calculate_dialog(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 15 November 2000

DESCRIPTION :
Brings up the calculate dialog.
==============================================================================*/
int Cell_interface_pop_up_unemap(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 01 November 2000

DESCRIPTION :
Pops up the UnEmap windows.
==============================================================================*/
int Cell_interface_clear_unemap(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 05 November 2000

DESCRIPTION :
Clears the UnEmap windows.
==============================================================================*/
int Cell_interface_set_save_signals(struct Cell_interface *cell_interface,
  int save);
/*******************************************************************************
LAST MODIFIED : 05 November 2000

DESCRIPTION :
Sets the value of the save signals toggle in the UnEmap interface object.
==============================================================================*/
int Cell_interface_list_root_component(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 06 November 2000

DESCRIPTION :
Lists out the root component from the cell interface.
==============================================================================*/
int Cell_interface_edit_component_variables(
  struct Cell_interface *cell_interface,char *name,int reset);
/*******************************************************************************
LAST MODIFIED : 07 November 2000

DESCRIPTION :
Pops up the cell varaible editing dialog with the variables from the component
given by <name>. If <name> is NULL, then the root component is used. <reset>
specifies whether the variable editing dialog is cleared before adding the
component <name> to the dialog.
==============================================================================*/
int Cell_interface_pop_up(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 07 November 2000

DESCRIPTION :
Pops up the cell interface
==============================================================================*/
int Cell_interface_pop_up_distributed_editing_dialog(
  struct Cell_interface *cell_interface,Widget activation);
/*******************************************************************************
LAST MODIFIED : 12 January 2001

DESCRIPTION :
Pops up the distributed editing dialog. <activation> is the toggle button
widget which activated the dialog.
==============================================================================*/
int Cell_interface_destroy_distributed_editing_dialog(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 12 January 2001

DESCRIPTION :
Destroys the distributed editing dialog.
==============================================================================*/
struct LIST(Cell_variable) *Cell_interface_get_variable_list(
  struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 17 January 2001

DESCRIPTION :
Returns the <cell_interface>'s variable list.
==============================================================================*/
int Cell_interface_close(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 22 January 2001

DESCRIPTION :
Closes all the Cell interface's.
==============================================================================*/
int Cell_interface_pop_up_export_dialog(struct Cell_interface *cell_interface);
/*******************************************************************************
LAST MODIFIED : 31 January 2001

DESCRIPTION :
Brings up the export (to CMISS) dialog.
==============================================================================*/
int Cell_interface_export_to_ipcell(struct Cell_interface *cell_interface,
  char *filename);
/*******************************************************************************
LAST MODIFIED : 02 February 2001

DESCRIPTION :
Exports an IPCELL file
==============================================================================*/
int Cell_interface_export_to_ipmatc(struct Cell_interface *cell_interface,
  char *filename,void *element_group_void,void *grid_field_void);
/*******************************************************************************
LAST MODIFIED : 02 February 2001

DESCRIPTION :
Exports an IPMATC file
==============================================================================*/

#endif /* !defined (CELL_INTERFACE_H) */
