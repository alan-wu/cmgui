/*******************************************************************************
FILE : scene_editor_wx.cpp

LAST MODIFIED : 26 February 2007

DESCRIPTION :
codes used to build scene editor with wxWidgets.
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


extern "C" {
#include <stdio.h>
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/graphical_element.h"
#include "graphics/graphics_object.h"
#include "user_interface/message.h"
}
#if defined (WX_USER_INTERFACE)
#include "wx/wx.h"
#include <wx/tglbtn.h>
#include "wx/xrc/xmlres.h"
#include "choose/choose_manager_class.hpp"
#include "choose/choose_enumerator_class.hpp"
#include <wx/collpane.h>
#include <wx/splitter.h>
#include "computed_field/computed_field_finite_element.h"
extern "C" {
#include "graphics/scene_editor_wx.h"
}
#include "graphics/scene_editor_wx.xrch"
#endif /* defined (WX_USER_INTERFACE)*/

/*
Module types
------------
*/

#if defined (WX_USER_INTERFACE)
class wxSceneEditor;
#endif /* defined (WX_USER_INTERFACE) */

struct Scene_editor_object
/*******************************************************************************
LAST MODIFIED : 4 December 2001

DESCRIPTION :
==============================================================================*/
{
	struct Scene_editor *scene_editor;

	/* name of object: scene or scene_object being viewed */
	char *name;

	/* parent_scene, if any, plus the scene this object represents, if any */
	struct Scene *parent_scene, *scene;
	struct Scene_object *scene_object;
	/* if there is a parent_scene, whether the widget shows object is visible */
	int visible;
	/* if object is a scene, whether its childrens' widgets are displayed */
	int expanded;
	/* list of child objects to put in expand_form if expanded scene list */
	struct LIST(Scene_editor_object) *scene_editor_objects;
	int in_use;
	int access_count;
}; /* struct Scene_editor_object */

PROTOTYPE_OBJECT_FUNCTIONS(Scene_editor_object);
DECLARE_LIST_TYPES(Scene_editor_object);
PROTOTYPE_LIST_FUNCTIONS(Scene_editor_object);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Scene_editor_object, name, \
	char *);

FULL_DECLARE_INDEXED_LIST_TYPE(Scene_editor_object);


struct Scene_editor
/*******************************************************************************
LAST MODIFIED : 02 Febuary 2007

DESCRIPTION :
==============================================================================*/
{
	/* if autoapply flag is set, any changes to the currently edited graphical
		 element will automatically be applied globally */
	int auto_apply, child_edited, child_expanded, transformation_edited,
		transformation_expanded;

	/* access gt_element_group for current_object if applicable */
	struct GT_element_group *gt_element_group;
	struct GT_element_group *edit_gt_element_group;
	Scene *scene;
	Scene_object *scene_object;
	/* keep address of pointer to editor so can self-destroy */
	struct Scene_editor **scene_editor_address;
	void *scene_manager_callback_id;
	struct MANAGER(Graphical_material) *graphical_material_manager;
	struct Graphical_material *default_material;
	struct Graphical_material *selected_material;
	struct Graphics_font *default_font;
	struct MANAGER(Scene) *scene_manager;
	struct User_interface *user_interface;
	struct Computed_field_package *computed_field_package;
	enum GT_element_settings_type current_settings_type;
	struct GT_element_settings *current_settings;
	struct Computed_field *default_coordinate_field;
	struct LIST(GT_object) *glyph_list;
	struct MANAGER(VT_volume_texture) *volume_texture_manager;
	struct MANAGER(Computed_field) *computed_field_manager;
	struct FE_field *native_discretization_field;
	struct Computed_field *coordinate_field;
	struct Computed_field *radius_scalar_field;
	enum Graphics_select_mode select_mode;
	float constant_radius,radius_scale_factor;
#if defined (WX_USER_INTERFACE)
	wxSceneEditor *wx_scene_editor;
	wxPanel *lower_panel;
	wxScrolledWindow *sceneediting;
	wxFrame *frame;
	wxCheckListBox *checklistbox;
	wxCheckListBox *graphicalitemslistbox;
	wxSplitterWindow *lowersplitter;
	wxSplitterWindow *topsplitter;
	wxCheckBox *autocheckbox;
	wxButton *applybutton;
	wxButton *revertbutton;
#endif /*defined (WX_USER_INTERFACE)*/
}; /*struct Scene_editor*/


int DESTROY(Scene_editor_object)(
	struct Scene_editor_object **scene_editor_object_address);
/*******************************************************************************
LAST MODIFIED : 31 October 2001

DESCRIPTION :
Prototype.
==============================================================================*/
static int set_general_settings(void *scene_editor_void);
/*******************************************************************************
LAST MODIFIED : 16 Match 2007

DESCRIPTION :
Prototype.
==============================================================================*/

static int get_and_set_graphical_element_settings(void *scene_editor_void);
/*******************************************************************************
LAST MODIFIED : 19 Match 2007

DESCRIPTION :
Prototype.
==============================================================================*/

static int add_scene_object_to_scene_check_box(struct Scene_object *scene_object,
																							 void *scene_editor_void);
/*******************************************************************************
LAST MODIFIED : 2 Match 2007

DESCRIPTION :
Prototype.
==============================================================================*/

static int Scene_editor_add_element_settings_item(
																									struct GT_element_settings *settings, void *scene_editor_void);
/*
Module functions
----------------
*/

struct Scene_editor_object *CREATE(Scene_editor_object)(
	struct Scene_editor *scene_editor, struct Scene *scene,
	struct Scene_object *scene_object)
/*******************************************************************************
LAST MODIFIED : 4 December 2001

DESCRIPTION :
Creates a scene_editor_object for editing either the <scene> or a
<scene_object>. Note Scene_editor_object_update function is responsible for
creating and updating widgets.
==============================================================================*/
{
	struct Scene *parent_scene;
	struct Scene_editor_object *scene_editor_object;

	ENTER(CREATE(Scene_editor_object));
	parent_scene = (struct Scene *)NULL;
	if (scene_editor && ((scene && (!scene_object)) ||
		((!scene) && scene_object &&
			(parent_scene = Scene_object_get_parent_scene(scene_object)))))
	{
		if (ALLOCATE(scene_editor_object, struct Scene_editor_object, 1))
		{
			scene_editor_object->scene_editor = scene_editor;
			scene_editor_object->parent_scene = parent_scene;
			scene_editor_object->name = (char *)NULL;
			if (scene)
			{
				scene_editor_object->scene = scene;
				scene_editor_object->scene_object = (struct Scene_object *)NULL;
				GET_NAME(Scene)(scene, &(scene_editor_object->name));
				scene_editor_object->visible = 1;
				scene_editor_object->expanded = 0;
			}
			else
			{
				scene_editor_object->scene_object = ACCESS(Scene_object)(scene_object);
				if (SCENE_OBJECT_SCENE == Scene_object_get_type(scene_object))
				{
					scene_editor_object->scene =
						Scene_object_get_child_scene(scene_object);
				}
				else
				{
					scene_editor_object->scene = (struct Scene *)NULL;
				}
				GET_NAME(Scene_object)(scene_object, &(scene_editor_object->name));
				scene_editor_object->visible =
					(g_VISIBLE == Scene_object_get_visibility(scene_object));
				scene_editor_object->expanded = 0;
			}
			if (scene_editor_object->scene)
			{
				scene_editor_object->scene_editor_objects =
					CREATE(LIST(Scene_editor_object))();
			}
			else
			{
				scene_editor_object->scene_editor_objects =
					(struct LIST(Scene_editor_object) *)NULL;
			}
			scene_editor_object->in_use = 0;
			scene_editor_object->access_count = 0;

			if ((!(scene_editor_object->name)) || (scene_editor_object->scene &&
				(!scene_editor_object->scene_editor_objects)))
			{
				display_message(ERROR_MESSAGE,
					"CREATE(Scene_editor_object).  Could not fill object");
				DESTROY(Scene_editor_object)(&scene_editor_object);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Scene_editor_object).  Could not allocate space for object");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Scene_editor_object).  Invalid argument(s)");
		scene_editor_object = (struct Scene_editor_object *)NULL;
	}
	LEAVE;

	return (scene_editor_object);
} /* CREATE(Scene_editor_object) */

int DESTROY(Scene_editor_object)(
	struct Scene_editor_object **scene_editor_object_address)
/*******************************************************************************
LAST MODIFIED : 4 December 2001

DESCRIPTION :
==============================================================================*/
{
	struct Scene_editor_object *scene_editor_object;
	int return_code;

	ENTER(DESTROY(Scene_editor_object));
	if (scene_editor_object_address &&
		(scene_editor_object = *scene_editor_object_address))
	{
		if (0 == scene_editor_object->access_count)
		{
			if (scene_editor_object->scene_object)
			{
				DEACCESS(Scene_object)(&(scene_editor_object->scene_object));
			}
			DEALLOCATE(scene_editor_object->name);
			if (scene_editor_object->scene_editor_objects)
			{
				DESTROY(LIST(Scene_editor_object))(
					&(scene_editor_object->scene_editor_objects));
			}
			DEALLOCATE(*scene_editor_object_address);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Scene_editor_object).  Non-zero access_count");
			return_code = 0;
		}
		*scene_editor_object_address = (struct Scene_editor_object *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Scene_editor_object).  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene_editor_object) */

DECLARE_OBJECT_FUNCTIONS(Scene_editor_object)

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Scene_editor_object, name, char *, \
	strcmp)

DECLARE_INDEXED_LIST_FUNCTIONS(Scene_editor_object)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Scene_editor_object, \
	name, char *, strcmp)


struct Scene_editor_object *Scene_editor_get_first_object(
	struct Scene_editor *scene_editor)
/*******************************************************************************
LAST MODIFIED : 16 November 2001

DESCRIPTION :
Returns the first scene_editor_object in the <scene_editor>'s list, actually
that of the first one in the scene.
==============================================================================*/
{
	char *name;
	struct Scene *scene;
	struct Scene_editor_object *scene_editor_object;
	struct Scene_object *scene_object;

	ENTER(Scene_editor_get_first_object);
	scene_editor_object = (struct Scene_editor_object *)NULL;
	if (scene_editor)
	{
		if ((scene = Scene_editor_get_scene(scene_editor)) &&
			(scene_object = first_Scene_object_in_Scene_that(scene,
				(LIST_CONDITIONAL_FUNCTION(Scene_object) *)NULL, (void *)NULL)))
		{
			/* get the first scene_editor_object in scene -- can't get first in
				 scene_editor_objects list since ordered by name */
			GET_NAME(Scene_object)(scene_object, &name);
  		DEALLOCATE(name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_editor_get_first_object.  Invalid argument(s)");
	}
	LEAVE;

	return (scene_editor_object);
} /* Scene_editor_get_first_object */

#if defined (WX_USER_INTERFACE)

class wxSceneEditor : public wxFrame
{				
	Scene_editor *scene_editor;
	wxScrolledWindow *sceneediting;
	wxFrame *frame;
	wxSplitterWindow *lowersplitter;
	wxSplitterWindow *topsplitter;
	wxCheckListBox *scenechecklist;
	wxListBox *scenelistbox;
	wxCheckListBox *graphicalitemschecklist;
	wxListBox *graphicalitemslistbox;
	wxStaticText *currentsceneobjecttext;
	wxButton *sceneupbutton;
	wxButton *scenedownbutton;
	wxCheckBox *nativediscretizationcheckbox;
	wxCheckBox *autocheckbox;
	wxButton *applybutton;
	wxButton *revertbutton;
	wxTextCtrl *elementdiscretisationpanel;
	wxTextCtrl *circlediscretisationpanel;
	wxTextCtrl *nametextfield;
	wxPanel *native_discretization_field_chooser_panel;
	wxCheckBox	*coordinatefieldcheckbox;
	wxPanel	*coordinate_field_chooser_panel;
	wxTextCtrl *constantradiustextctrl;
	wxCheckBox *radiusscalarcheckbox;
	wxTextCtrl *scalefactorstextctrl;
	wxPanel *radius_scalar_chooser_panel;
	wxStaticText *constantradius;
	wxStaticText *scalefactor;
	DEFINE_MANAGER_CLASS(Scene);
	Managed_object_chooser<Scene,MANAGER_CLASS(Scene)>
		*scene_chooser;
	wxString TempText;
	DEFINE_MANAGER_CLASS(Computed_field);
	Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
	 *computed_field_chooser;
	Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
	 *FE_field_chooser;	
	DEFINE_ENUMERATOR_TYPE_CLASS(GT_element_settings_type);
	Enumerator_chooser<ENUMERATOR_TYPE_CLASS(GT_element_settings_type)>
		*settings_type_chooser;
	Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
		*coordinate_field_chooser;
	DEFINE_MANAGER_CLASS(Graphical_material);
	Managed_object_chooser<Graphical_material,MANAGER_CLASS(Graphical_material)>
		*graphical_material_chooser;
	Managed_object_chooser<Graphical_material,MANAGER_CLASS(Graphical_material)>
		*selected_material_chooser;
	DEFINE_ENUMERATOR_TYPE_CLASS(Graphics_select_mode);
	Enumerator_chooser<ENUMERATOR_TYPE_CLASS(Graphics_select_mode)>
		*select_mode_chooser;
	Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
	 *radius_scalar_chooser;	

public:

  wxSceneEditor(Scene_editor *scene_editor): 
		scene_editor(scene_editor)
  {
	{	
		wxXmlInit_scene_editor_wx();
	}
		scene_editor->wx_scene_editor = (wxSceneEditor *)NULL;
		wxXmlResource::Get()->LoadFrame(this,
			(wxWindow *)NULL, _T("CmguiSceneEditor"));

 /* Set the chooser panel  in the secne editor */
  wxPanel *scene_object_chooser_panel = 
		XRCCTRL(*this, "SceneObjectChooserPanel", wxPanel);
	scene_chooser = 
  new Managed_object_chooser<Scene,MANAGER_CLASS(Scene)>
	  (scene_object_chooser_panel, scene_editor->scene, scene_editor->scene_manager,
	  (MANAGER_CONDITIONAL_FUNCTION(Scene) *)NULL, (void *)NULL, scene_editor->user_interface);
	  Callback_base< Scene* > *scene_object_callback = 
		  new Callback_member_callback< Scene*, 
		  wxSceneEditor, int (wxSceneEditor::*)(Scene *) >
		  (this, &wxSceneEditor::scene_object_callback);
      scene_chooser->set_callback(scene_object_callback);

 /* Set the collapsible pane in the secne editor */
  wxCollapsiblePane *collpane = XRCCTRL(*this, "CollapsiblePane", wxCollapsiblePane);
  wxPanel *GeneralSettingPanel = new wxPanel;
  wxWindow *win = collpane->GetPane();
  wxXmlResource::Get()->LoadPanel(GeneralSettingPanel,
	 win, _T("CmguiSceneEditorGeneralSettings"));
  wxSizer *paneSz = new wxBoxSizer(wxVERTICAL);
  paneSz->Add(GeneralSettingPanel, 1, wxEXPAND|wxALL, 2);
  win->SetSizer(paneSz);
  paneSz->SetSizeHints(win);
  collpane->Collapse(1);
  wxPanel *lowestpanel = 
	 XRCCTRL(*this, "CmguiSceneEditor", wxPanel);
  lowestpanel->Fit();


	/* Set the default_coordinate_field_chooser_panel*/
		scene_editor->default_coordinate_field=
			GT_element_group_get_default_coordinate_field(scene_editor->edit_gt_element_group);
	wxPanel *default_coordinate_field_chooser_panel =
		XRCCTRL(*this, "DefaultCoordinateFieldChooser",wxPanel);
	computed_field_chooser = 
		new Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
	  (default_coordinate_field_chooser_panel, scene_editor->default_coordinate_field, scene_editor->computed_field_manager,
		 (MANAGER_CONDITIONAL_FUNCTION(Computed_field) *)NULL, (void *)NULL, scene_editor->user_interface);
	  Callback_base< Computed_field* > *default_coordinate_field_callback = 
		  new Callback_member_callback< Computed_field*, 
		  wxSceneEditor, int (wxSceneEditor::*)(Computed_field *) >
		  (this, &wxSceneEditor::default_coordinate_field_callback);
      computed_field_chooser->set_callback(default_coordinate_field_callback);

			/* Set the native_discretisation_chooser_panel*/
 			scene_editor->native_discretization_field = GT_element_group_get_native_discretization_field(scene_editor->edit_gt_element_group);
 			wxPanel *native_discretization_field_chooser =
 				XRCCTRL(*this, "NativeDiscretisationFieldChooser",wxPanel);			
 			FE_field_chooser =
 				new Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
 				(native_discretization_field_chooser, /*scene_editor->native_discretization_field*/(Computed_field*)NULL, scene_editor->computed_field_manager,
				 Computed_field_is_type_finite_element_iterator, (void *)NULL, scene_editor->user_interface);
 			Callback_base< Computed_field* > *native_discretization_field_callback = 
 				new Callback_member_callback< Computed_field*, 
 		  wxSceneEditor, int (wxSceneEditor::*)(Computed_field *) >
 				(this, &wxSceneEditor::native_discretization_field_callback);
       FE_field_chooser->set_callback(native_discretization_field_callback);

			/* Set the coordinate_field_chooser_panel*/
		scene_editor->coordinate_field=
			GT_element_settings_get_coordinate_field(scene_editor->current_settings);
		wxPanel *coordinate_field_chooser_panel =
			XRCCTRL(*this, "CoordinateFieldChooserPanel",wxPanel);
	coordinate_field_chooser = 
		new Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
	  (coordinate_field_chooser_panel, scene_editor->default_coordinate_field, scene_editor->computed_field_manager,
		 (MANAGER_CONDITIONAL_FUNCTION(Computed_field) *)NULL, (void *)NULL, scene_editor->user_interface);
 			Callback_base< Computed_field* > *coordinate_field_callback = 
 				new Callback_member_callback< Computed_field*, 
 		  wxSceneEditor, int (wxSceneEditor::*)(Computed_field *) >
 				(this, &wxSceneEditor::coordinate_field_callback);
      coordinate_field_chooser->set_callback(coordinate_field_callback);

			/* Set the graphical_material_chooser_panel*/
			wxPanel *graphical_material_chooser_panel =
			XRCCTRL(*this, "GraphicalMaterialChooserPanel",wxPanel);
	graphical_material_chooser = 
		new Managed_object_chooser<Graphical_material,MANAGER_CLASS(Graphical_material)>
	  (graphical_material_chooser_panel, scene_editor->default_material, scene_editor->graphical_material_manager,
		 (MANAGER_CONDITIONAL_FUNCTION(Graphical_material) *)NULL, (void *)NULL, scene_editor->user_interface);
 			Callback_base< Graphical_material* > *graphical_material_callback = 
 				new Callback_member_callback< Graphical_material*, 
 		  wxSceneEditor, int (wxSceneEditor::*)(Graphical_material *) >
 				(this, &wxSceneEditor::graphical_material_callback);
      graphical_material_chooser->set_callback(graphical_material_callback);

			/* Set the selected_material_chooser_panel*/
			wxPanel *selected_material_chooser_panel =
			XRCCTRL(*this, "SelectedMaterialChooserPanel",wxPanel);
	selected_material_chooser = 
		new Managed_object_chooser<Graphical_material,MANAGER_CLASS(Graphical_material)>
	  (selected_material_chooser_panel, scene_editor->selected_material, scene_editor->graphical_material_manager,
		 (MANAGER_CONDITIONAL_FUNCTION(Graphical_material) *)NULL, (void *)NULL, scene_editor->user_interface);
 			Callback_base< Graphical_material* > *selected_material_callback = 
 				new Callback_member_callback< Graphical_material*, 
 		  wxSceneEditor, int (wxSceneEditor::*)(Graphical_material *) >
 				(this, &wxSceneEditor::selected_material_callback);
      selected_material_chooser->set_callback(selected_material_callback);

			/* Graphical element settings type chooser */
			wxPanel *settings_type_chooser_panel = 
				XRCCTRL(*this, "TypeFormChooser", wxPanel);
			settings_type_chooser = 
				new Enumerator_chooser<ENUMERATOR_TYPE_CLASS(GT_element_settings_type)>
				(settings_type_chooser_panel, 
				scene_editor->current_settings_type,
				(ENUMERATOR_CONDITIONAL_FUNCTION(GT_element_settings_type) *)NULL,
				(void *)NULL, scene_editor->user_interface);
			settings_type_chooser_panel->Fit();
			Callback_base< enum GT_element_settings_type > *settings_type_callback = 
				new Callback_member_callback< enum GT_element_settings_type, 
				wxSceneEditor, int (wxSceneEditor::*)(enum GT_element_settings_type) >
				(this, &wxSceneEditor::settings_type_callback);
			settings_type_chooser->set_callback(settings_type_callback);
			settings_type_chooser->set_value(scene_editor->current_settings_type);

			/* Set the select_mode_chooser_panel*/
			wxPanel *select_mode_chooser_panel = 
				XRCCTRL(*this, "SelectModeChooserPanel", wxPanel);
			select_mode_chooser = 
				new Enumerator_chooser<ENUMERATOR_TYPE_CLASS(Graphics_select_mode)>
				(select_mode_chooser_panel, 
				scene_editor->select_mode,
				(ENUMERATOR_CONDITIONAL_FUNCTION(Graphics_select_mode) *)NULL,
				(void *)NULL, scene_editor->user_interface);
			select_mode_chooser_panel->Fit();
			Callback_base< enum Graphics_select_mode > *select_mode_callback = 
				new Callback_member_callback< enum Graphics_select_mode, 
				wxSceneEditor, int (wxSceneEditor::*)(enum Graphics_select_mode) >
				(this, &wxSceneEditor::select_mode_callback);
			select_mode_chooser->set_callback(select_mode_callback);
			select_mode_chooser->set_value(GT_element_settings_get_select_mode(
			   scene_editor->current_settings));

			wxFrame *frame=XRCCTRL(*this, "CmguiSceneEditor", wxFrame);
			frame->Fit();

 Show();
};

  wxSceneEditor()
  {
  };

  ~wxSceneEditor()
  {
	  delete scene_chooser;
	  delete computed_field_chooser;
  }

	int scene_object_callback(Scene *scene)
/*******************************************************************************
LAST MODIFIED : 9 February 2007

DESCRIPTION :
Callback from wxChooser<Scene> when choice is made.
==============================================================================*/
	{
		Scene_editor_set_scene(scene_editor, scene);
		scene_editor->lower_panel->Hide();
		wxCheckListBox *checklist = scene_editor->checklistbox;
 		checklist->Clear();
 		for_each_Scene_object_in_Scene(scene,
 																	 add_scene_object_to_scene_check_box, (void *)scene_editor);
		return 1;
	}

	int default_coordinate_field_callback(Computed_field *default_coordinate_field)
/*******************************************************************************
LAST MODIFIED : 9 February 2007

DESCRIPTION :
Callback from wxChooser<Scene> when choice is made.
==============================================================================*/
 {
	if (default_coordinate_field&&
		scene_editor)
	{
		GT_element_group_set_default_coordinate_field(
			scene_editor->edit_gt_element_group,
			(struct Computed_field *)default_coordinate_field);
		/* inform the client of the change */
		 set_general_settings((void *)scene_editor);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphical_element_editor_update_default_coordinate_field.  "
			"Invalid argument(s)");
	}
		return 1;
} /* graphical_element_editor_update_default_coordinate_field */
	

	int native_discretization_field_callback(Computed_field *native_discretization_field)
/*******************************************************************************
LAST MODIFIED : 9 February 2007

DESCRIPTION :
Callback from wxChooser<Scene> when choice is made.
==============================================================================*/
	{
		int return_code;

		return_code = Computed_field_get_type_finite_element(native_discretization_field,
			&scene_editor->native_discretization_field);

		
		if (GT_element_group_get_native_discretization_field(
			scene_editor->edit_gt_element_group))
		{
			GT_element_group_set_native_discretization_field(
				scene_editor->edit_gt_element_group,
				scene_editor->native_discretization_field);
			/* inform the client of the change */
			set_general_settings((void *)scene_editor);
		}
		else
			{
				display_message(ERROR_MESSAGE,
												"graphical_element_editor_update_native_discretization_field.  "
												"Invalid argument(s)");
			}	

		return (return_code);
	}

int coordinate_field_callback(Computed_field *coordinate_field)
/*******************************************************************************
LAST MODIFIED : 19 March 2007

DESCRIPTION :
Callback from wxChooser<Coordinate Field> when choice is made.
==============================================================================*/
	{
		GT_element_settings_set_coordinate_field(
			 scene_editor->current_settings,
			 coordinate_field);
		AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
		return(1);
	}



int graphical_material_callback(Graphical_material *default_material)
/*******************************************************************************
LAST MODIFIED : 19 March 2007

DESCRIPTION :
Callback from wxChooser<Graphical Material> when choice is made.
==============================================================================*/
	{
		GT_element_settings_set_material(scene_editor->current_settings,
			default_material);
		AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
		return(1);
	}

int selected_material_callback(Graphical_material *selected_material)
/*******************************************************************************
LAST MODIFIED : 20 March 2007

DESCRIPTION :
Callback from wxChooser<Selected Material> when choice is made.
==============================================================================*/
	{
		GT_element_settings_set_selected_material(scene_editor->current_settings,
			selected_material);
		AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
		return(1);
	}

	int settings_type_callback(enum GT_element_settings_type new_value)
/*******************************************************************************
LAST MODIFIED : 12 March 2007

DESCRIPTION :
Callback from wxChooser<Scene> when choice is made.
==============================================================================*/
	{
		GT_element_settings *settings;
		GT_element_settings *newsettings;
		int return_code;

		if (scene_editor)
			{
				scene_editor->current_settings_type = new_value;
				settings = first_settings_in_GT_element_group_that(
					scene_editor->edit_gt_element_group,
					GT_element_settings_type_matches,
					(void*)scene_editor->current_settings_type);
				graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
				int number= graphicalitemschecklist->GetCount();
				for (int i=0;number>i;i++) 
					{
						newsettings = static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(i));
						if (settings == newsettings)
							{
								graphicalitemschecklist->SetSelection(i);
								break;
							}
						graphicalitemschecklist->SetSelection(wxNOT_FOUND);
					}
				if (settings != scene_editor->current_settings)
					{
						if (scene_editor->current_settings)
							REACCESS(GT_element_settings)(&(scene_editor->current_settings),
																						settings);
						if (settings)
							{
								/* if settings_type changed, select it in settings_type option menu */
								GT_element_settings_type settings_type = GT_element_settings_get_settings_type(settings);
								if (settings_type != scene_editor->current_settings_type)
									{
										scene_editor->current_settings_type = settings_type;
									}
							}
						return_code = 1;
					}
			}
		else
			{
				display_message(ERROR_MESSAGE,
				 "setting type callback.  Invalid argument(s)");
				return_code =  0;
			}
		return (return_code);	
	}

	int select_mode_callback(enum Graphics_select_mode select_mode)
/*******************************************************************************
LAST MODIFIED : 19 March 2007

DESCRIPTION :
Callback from wxChooser<select mode> when choice is made.
==============================================================================*/
	{
		GT_element_settings_set_select_mode(
																				scene_editor->current_settings, select_mode);
		AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);

		return 1;
	}

	int setSceneObject(Scene *scene)
/*******************************************************************************
LAST MODIFIED : 9 February 2007

DESCRIPTION :
Set the selected option in the Scene Object chooser.
==============================================================================*/
	{
		scene_chooser->set_object(scene);
		return 1;
	}
int radius_scalar_callback(Computed_field *radius_scalar_field)
/*******************************************************************************
LAST MODIFIED : 20 March 2007

DESCRIPTION :
Callback from wxChooser<Radius Scalar> when choice is made.
==============================================================================*/
	{
		float constant_radius,scale_factor; 
		double temp;
		constantradiustextctrl=XRCCTRL(*this, "ConstantRadiusTextCtrl",wxTextCtrl);
		(constantradiustextctrl->GetValue()).ToDouble(&temp);
		constant_radius=(float)temp;
		scalefactorstextctrl=XRCCTRL(*this,"ScaleFactorsTextCtrl",wxTextCtrl);
		(scalefactorstextctrl->GetValue()).ToDouble(&temp);
		scale_factor=(float)temp;	
		GT_element_settings_set_radius_parameters(scene_editor->current_settings,constant_radius,
																							scale_factor,radius_scalar_field);
		AutoApplyorNot(scene_editor->gt_element_group,
			scene_editor->edit_gt_element_group);
		return 1;
	}

      void ResetScrolledWindow(wxCollapsiblePaneEvent& event)
  	{
			lowersplitter=XRCCTRL(*this,"LowerSplitter",wxSplitterWindow);
			lowersplitter->Layout();	
			topsplitter=XRCCTRL(*this,"TopSplitter",wxSplitterWindow);
			topsplitter->Layout();	
			frame = 
			  XRCCTRL(*this, "CmguiSceneEditor", wxFrame);
			frame->Layout();
			frame->SetMinSize(wxSize(50,100));
			frame->SetMaxSize(wxSize(2000,2000));
	}

	void AutoApplyorNot(struct GT_element_group  *gt_element_group, GT_element_group *edit_gt_element_group)
	{
		autocheckbox = XRCCTRL(*this, "AutoCheckBox", wxCheckBox);
		if(autocheckbox->IsChecked())
			{
				if (!GT_element_group_modify(gt_element_group,
																	edit_gt_element_group))
					{
						display_message(ERROR_MESSAGE, "wxSceneEditor::UpdateGraphicalElementList.  "
														"Could not modify graphical element");
					}
		  }
	}

      void ResetWindow(wxSplitterEvent& event)
	  {
			sceneediting = 
				XRCCTRL(*this, "SceneEditing", wxScrolledWindow);
			sceneediting->Layout();
			sceneediting->SetScrollbars(10,10,40,40);
			lowersplitter=XRCCTRL(*this,"LowerSplitter",wxSplitterWindow);
			lowersplitter->Layout();	
			topsplitter=XRCCTRL(*this,"TopSplitter",wxSplitterWindow);
			topsplitter->Layout();	
			frame = 
					XRCCTRL(*this, "CmguiSceneEditor", wxFrame);
			frame->Layout();
			frame->SetMinSize(wxSize(50,100));
		}

	void ElementDiscretisationUpdate(wxCommandEvent &event)
	{
		struct Parse_state *temp_state;
		struct Element_discretization element_discretization,
			old_element_discretization;
		elementdiscretisationpanel=XRCCTRL(*this, "ElementDiscretisationPanel",wxTextCtrl);
		TempText = elementdiscretisationpanel->GetValue();

		if (TempText)
		{
			if (temp_state=create_Parse_state(const_cast<char *>(TempText.c_str())))
			{
				if (GT_element_group_get_element_discretization(
					scene_editor->edit_gt_element_group, &old_element_discretization) &&
					set_Element_discretization(temp_state,
						(void *)&element_discretization,
						(void *)scene_editor->user_interface) &&
					((element_discretization.number_in_xi1 !=
						old_element_discretization.number_in_xi1) ||
						(element_discretization.number_in_xi2 !=
							old_element_discretization.number_in_xi2) ||
						(element_discretization.number_in_xi3 !=
							old_element_discretization.number_in_xi3)) &&
					GT_element_group_set_element_discretization(
						scene_editor->edit_gt_element_group,
						&element_discretization))
				{
					/* inform the client of the changes */
				}
				destroy_Parse_state(&temp_state);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"graphical_element_editor_element_disc_text_CB.  "
					"Could not create parse state");
			}
			set_general_settings((void *)scene_editor);
		}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphical_element_editor_element_disc_text_CB.  Invalid argument(s)");
	}
	AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
	}

	void NativeDiscretisationFieldChecked(wxCommandEvent &event)
	{
		nativediscretizationcheckbox = XRCCTRL(*this, "NativeDiscretisationFieldCheckBox",wxCheckBox);
		native_discretization_field_chooser_panel = XRCCTRL(*this, "NativeDiscretisationFieldChooser",wxPanel);
		if (nativediscretizationcheckbox->IsChecked())
			{
			native_discretization_field_chooser_panel->Enable();
			}
		else
			{
			native_discretization_field_chooser_panel->Disable();
			}			
	}

	void AutoChecked(wxCommandEvent &event)
	{
		autocheckbox = XRCCTRL(*this, "AutoCheckBox", wxCheckBox);
		applybutton = XRCCTRL(*this, "ApplyButton", wxButton);
		revertbutton = XRCCTRL(*this,"RevertButton", wxButton);
		if(autocheckbox->IsChecked())
			{
				applybutton->Disable();
		       revertbutton->Disable();
	AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
			}
		else
			{
				applybutton->Enable();
		       revertbutton->Enable();
			}
	}

	void ApplyClicked(wxCommandEvent &event)
	{
 				if (!GT_element_group_modify(scene_editor->gt_element_group,
																	scene_editor->edit_gt_element_group))
					{
						display_message(ERROR_MESSAGE, "wxSceneEditor::UpdateGraphicalElementList.  "
														"Could not modify graphical element");
					}
	}

	void CircleDiscretisationUpdate(wxCommandEvent &event)
	{
		int circle_discretization;
		circlediscretisationpanel =	 XRCCTRL(*this, "CircleDiscretisationPanel",wxTextCtrl);
		TempText = circlediscretisationpanel->GetValue();
		if (TempText)
		{
			circle_discretization = atoi(const_cast<char *>(TempText.c_str()));
			if ((circle_discretization != GT_element_group_get_circle_discretization(
				scene_editor->edit_gt_element_group)) &&
				GT_element_group_set_circle_discretization(
					scene_editor->edit_gt_element_group, circle_discretization))
				{
				}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"graphical_element_editor_circle_disc_text_CB.  Missing text");
		}
		/* always redisplay discretization to show assumed values */
		set_general_settings((void *)scene_editor);

	AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
	 }

	void SetGraphicalElementGroup(GT_element_group *gt_element_group)
	{
		GT_element_group *edit_gt_element_group;
		REACCESS(GT_element_group)(&scene_editor->gt_element_group, gt_element_group);
		if (gt_element_group)
			{
				edit_gt_element_group =
					create_editor_copy_GT_element_group(gt_element_group);
				if (!edit_gt_element_group)
					{
						display_message(ERROR_MESSAGE,
														"graphical_element_editor_set_gt_element_group.  "
														"Could not copy graphical element");
			 		}
			}
		else
			{
				edit_gt_element_group = (struct GT_element_group *)NULL;
			}
		REACCESS(GT_element_group)(&(scene_editor->edit_gt_element_group),
						edit_gt_element_group);
		scene_editor->graphicalitemslistbox = XRCCTRL(*this, "GraphicalItemsListBox",wxCheckListBox);
		scene_editor->graphicalitemslistbox->Clear();
		if (edit_gt_element_group)
			{
				for_each_settings_in_GT_element_group(edit_gt_element_group,
				   Scene_editor_add_element_settings_item, (void *)scene_editor);
				scene_editor->lower_panel->Show();
				set_general_settings((void *)scene_editor);
				get_and_set_graphical_element_settings((void *)scene_editor);
			}
	}

	void UpdateSceneObjectList(Scene_object *scene_object)
	{
		GT_element_group *gt_element_group;
		REACCESS(Scene_object)(&scene_editor->scene_object, scene_object);
 		scenechecklist=XRCCTRL(*this,"SceneCheckList",wxCheckListBox);
		int selection =	scenechecklist->GetSelection();
		if(scenechecklist->IsChecked(selection))
			{
				Scene_object_set_visibility(scene_object, g_VISIBLE);
			}
		else
			{
				Scene_object_set_visibility(scene_object, g_INVISIBLE);
			}

		switch (Scene_object_get_type(scene_object))
			{
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
				{
					gt_element_group = Scene_object_get_graphical_element_group(
						scene_object);
					computed_field_chooser->set_object(GT_element_group_get_default_coordinate_field(scene_editor->gt_element_group));
					SetGraphicalElementGroup(gt_element_group);
			} break;
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			case SCENE_OBJECT_SCENE:
				{
					scene_editor->graphicalitemslistbox = XRCCTRL(*this, "GraphicalItemsListBox",wxCheckListBox);
					scene_editor->graphicalitemslistbox->Clear();
					scene_editor->lower_panel->Hide();
					/* nothing to do */
				} break;
			}
	}

 	void 	UpdateGraphicalElementList(GT_element_settings *settings)
 	{
 		graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
		int selection =	graphicalitemschecklist->GetSelection();
		REACCESS(GT_element_settings)(&scene_editor->current_settings, settings);
		scene_editor->current_settings_type = GT_element_settings_get_settings_type(settings);
		settings_type_chooser->set_value(scene_editor->current_settings_type);
		if (graphicalitemschecklist->IsChecked(selection))
			{
				GT_element_settings_set_visibility(settings, 1);
			}
		else
			{
				GT_element_settings_set_visibility(settings, 0);
			}
		get_and_set_graphical_element_settings((void *)scene_editor);
		AutoApplyorNot(scene_editor->gt_element_group,
			scene_editor->edit_gt_element_group);
 	}

	void SceneCheckListClicked(wxCommandEvent &event)
	{
		currentsceneobjecttext=XRCCTRL(*this,"CurrentSceneObjectText",wxStaticText);
 		scenechecklist=XRCCTRL(*this,"SceneCheckList",wxCheckListBox);
		currentsceneobjecttext->SetLabel(scenechecklist->GetStringSelection());
		int selection=scenechecklist->GetSelection();
	 	UpdateSceneObjectList(static_cast<Scene_object*>(scenechecklist->GetClientData(selection)));
		frame=XRCCTRL(*this, "CmguiSceneEditor", wxFrame);
		frame->Layout();
 	}

	void SetSceneObjectPosition(Scene_object *scene_object, Scene *scene, int selection)
	{
 		REACCESS(Scene_object)(&scene_editor->scene_object, scene_object);
		Scene_set_scene_object_position(scene,scene_object,selection);
	}


	void  SceneObjectUpClicked(wxCommandEvent &event)
	{
 		scenechecklist=XRCCTRL(*this,"SceneCheckList",wxCheckListBox);
 		int selection = scenechecklist->GetSelection();
		if (selection>=1)
			{
				SetSceneObjectPosition(static_cast<Scene_object*>(scenechecklist->GetClientData(selection)), scene_editor->scene, selection);
				scenechecklist->Clear();
				for_each_Scene_object_in_Scene(scene_editor->scene,
 				  add_scene_object_to_scene_check_box, (void *)scene_editor);
				scenechecklist->SetSelection(selection-1);
			}
 	}

	void  SceneObjectDownClicked(wxCommandEvent &event)
	{
 		scenechecklist=XRCCTRL(*this,"SceneCheckList",wxCheckListBox);
		int selection = scenechecklist->GetSelection();
		int number = scenechecklist->GetCount();
		if (number>=(selection+2))
			{
				SetSceneObjectPosition(static_cast<Scene_object*>(scenechecklist->GetClientData(selection)), scene_editor->scene,(selection+2));
				scenechecklist->Clear();
				for_each_Scene_object_in_Scene(scene_editor->scene,
																			 add_scene_object_to_scene_check_box, (void *)scene_editor);
				scenechecklist->SetSelection(selection+1);
			}
	}

	void GraphicalItemsListBoxClicked(wxCommandEvent &event)
	{
 		graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
		int selection= graphicalitemschecklist->GetSelection();
		if (-1 != selection)
			{
				UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(selection)));
			}
 	}
	
	void AddToSettingList(wxCommandEvent &event)
	{
		GT_element_settings *settings;
		int return_code;

		if (settings =
				CREATE(GT_element_settings)(scene_editor->current_settings_type))
			{
				return_code = 1;
				if (scene_editor->current_settings)
					{
						/* copy current settings into new settings */
						return_code = GT_element_settings_copy_without_graphics_object(settings,
																																					 scene_editor->current_settings);
						/* make sure new settings is visible */
						GT_element_settings_set_visibility(settings,1);
					}
				else
					{
					 	/* set materials for all settings */
						GT_element_settings_set_material(settings,
																						 scene_editor->default_material);
						GT_element_settings_set_label_field(settings,
																								(struct Computed_field *)NULL, scene_editor->default_font);
						GT_element_settings_set_selected_material(settings,
																											FIND_BY_IDENTIFIER_IN_MANAGER(Graphical_material,name)(
																																																						 "default_selected",scene_editor->graphical_material_manager));
						/* for data_points, ensure either there are points with
							 default_coordinate defined at them. If not, and any have
							 the element_xi_coordinate field defined over them, use that */
						if (GT_ELEMENT_SETTINGS_DATA_POINTS==
								scene_editor->current_settings_type)
							{
								FE_region *data_fe_region = Cmiss_region_get_FE_region(
																														GT_element_group_get_data_Cmiss_region(
																																																	 scene_editor->edit_gt_element_group));
								Computed_field *default_coordinate_field=
									GT_element_group_get_default_coordinate_field(
																																scene_editor->edit_gt_element_group);
								if (!FE_region_get_first_FE_node_that(data_fe_region,
																											FE_node_has_Computed_field_defined,
																											(void *)default_coordinate_field))
									{
										MANAGER(Computed_field) *computed_field_manager=  Computed_field_package_get_computed_field_manager(
											scene_editor->computed_field_package);
										Computed_field *element_xi_coordinate_field;
										if ((element_xi_coordinate_field=
												 FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field,name)(
																																						"element_xi_coordinate",computed_field_manager)) &&
												FE_region_get_first_FE_node_that(data_fe_region,
																												 FE_node_has_Computed_field_defined,
																												 (void *)element_xi_coordinate_field))
											{
												GT_element_settings_set_coordinate_field(settings,
																																 element_xi_coordinate_field);
											}
									}
							}
						/* set iso_scalar_field for iso_surfaces */
						if (GT_ELEMENT_SETTINGS_ISO_SURFACES==
								scene_editor->current_settings_type)
							{
								Computed_field *iso_scalar_field;
								MANAGER(Computed_field) *computed_field_manager=  Computed_field_package_get_computed_field_manager(
																																																										scene_editor->computed_field_package);
								if (iso_scalar_field=FIRST_OBJECT_IN_MANAGER_THAT(Computed_field)(
																																									Computed_field_is_scalar,(void *)NULL,computed_field_manager))
									{
										double iso_value_default = 0;
										if (!GT_element_settings_set_iso_surface_parameters(settings,
																																				iso_scalar_field,/*number_of_iso_values*/1,&iso_value_default,
																																				/*decimation_threshold*/0.0))
											{
												return_code=0;
											}
									}
								else
									{
										display_message(WARNING_MESSAGE,"No scalar fields defined");
										return_code=0;
									}
							}
						/* set initial glyph for settings types that use them */
						if ((GT_ELEMENT_SETTINGS_NODE_POINTS==
								 scene_editor->current_settings_type)||
								(GT_ELEMENT_SETTINGS_DATA_POINTS==
								 scene_editor->current_settings_type)||
								(GT_ELEMENT_SETTINGS_ELEMENT_POINTS==
								 scene_editor->current_settings_type))
							{
								/* default to point glyph for fastest possible display */
								GT_object *glyph, *old_glyph;
								Glyph_scaling_mode glyph_scaling_mode;
								Triple glyph_centre,glyph_scale_factors,glyph_size;
								Computed_field *orientation_scale_field, *variable_scale_field; ;
								glyph=FIND_BY_IDENTIFIER_IN_LIST(GT_object,name)("point",
																																 scene_editor->glyph_list);
								if (!(GT_element_settings_get_glyph_parameters(settings,
																															 &old_glyph, &glyph_scaling_mode ,glyph_centre, glyph_size,
																															 &orientation_scale_field, glyph_scale_factors,
																															 &variable_scale_field) &&
											GT_element_settings_set_glyph_parameters(settings,glyph,
																															 glyph_scaling_mode, glyph_centre, glyph_size,
																															 orientation_scale_field, glyph_scale_factors,
																															 variable_scale_field)))
									{
										display_message(WARNING_MESSAGE,"No glyphs defined");
										return_code=0;
									}
							}
						if (GT_ELEMENT_SETTINGS_VOLUMES==scene_editor->current_settings_type)
							{
								/* must have a volume texture */
								VT_volume_texture *volume_texture;
								if (volume_texture=FIRST_OBJECT_IN_MANAGER_THAT(VT_volume_texture)(
																																									 (MANAGER_CONDITIONAL_FUNCTION(VT_volume_texture) *)NULL,
																																									 (void *)NULL,scene_editor->volume_texture_manager))
									{
										if (!GT_element_settings_set_volume_texture(settings,
																																volume_texture))
											{
												return_code=0;
											}
									}
								else
									{
										display_message(WARNING_MESSAGE,"No volume textures defined");
										return_code=0;
									}
							}
						/* set stream_vector_field for STREAMLINES */
						if (GT_ELEMENT_SETTINGS_STREAMLINES==
								scene_editor->current_settings_type)
							{
								Streamline_type streamline_type;
								Computed_field *stream_vector_field;
								float streamline_length,streamline_width;
								int reverse_track;
								MANAGER(Computed_field) *computed_field_manager=  Computed_field_package_get_computed_field_manager(
																																																										scene_editor->computed_field_package);
								GT_element_settings_get_streamline_parameters(
																															settings,&streamline_type,&stream_vector_field,&reverse_track,
																															&streamline_length,&streamline_width);
								if (stream_vector_field=
										FIRST_OBJECT_IN_MANAGER_THAT(Computed_field)(
																																 Computed_field_is_stream_vector_capable,(void *)NULL,
																																 computed_field_manager))
									{
										if (!GT_element_settings_set_streamline_parameters(
																																			 settings,streamline_type,stream_vector_field,reverse_track,
																																			 streamline_length,streamline_width))
											{
												return_code=0;
											}
									}
								else
									{
										display_message(WARNING_MESSAGE,"No vector fields defined");
										return_code=0;
									}
							}
						/* set use_element_type for element_points */
						if (return_code && (GT_ELEMENT_SETTINGS_ELEMENT_POINTS ==
																scene_editor->current_settings_type))
							{
								GT_element_settings_set_use_element_type(settings,USE_ELEMENTS);
							}
					}
				if (return_code && GT_element_group_add_settings(
																												 scene_editor->edit_gt_element_group, settings, 0))
					{
						//Update the list of settings
						wxCheckListBox *graphicalitemchecklist =  XRCCTRL(*this, "GraphicalItemsListBox",wxCheckListBox);
						graphicalitemschecklist->Clear();
						for_each_settings_in_GT_element_group(scene_editor->edit_gt_element_group,
						  Scene_editor_add_element_settings_item, (void *)scene_editor);
						graphicalitemchecklist->SetSelection((graphicalitemchecklist->GetCount()-1));
						UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(graphicalitemchecklist->GetCount()-1)));
					}
				if (!return_code)
					{
						DESTROY(GT_element_settings)(&settings);
					}
			}
	}

	void RemoveFromSettingList(wxCommandEvent &event)
	{
	unsigned int position;
	if 	(scene_editor->edit_gt_element_group)
		{
 		graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
		position = GT_element_group_get_settings_position(
			scene_editor->edit_gt_element_group, scene_editor->current_settings);
		GT_element_group_remove_settings(
			scene_editor->edit_gt_element_group, scene_editor->current_settings);
		/* inform the client of the changes */
		graphicalitemschecklist->Clear();
		for_each_settings_in_GT_element_group(scene_editor->edit_gt_element_group,
		 Scene_editor_add_element_settings_item, (void *)scene_editor);
		if (position>=1)
			{
				if (graphicalitemschecklist->GetCount()>=position)
					{
						graphicalitemschecklist->SetSelection(position-1);
						UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(position-1)));
					}
				else
					{
						graphicalitemschecklist->SetSelection(position-2);
						UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(position-2)));
					}
			}
		}
	else
		{
			display_message(ERROR_MESSAGE,
			   "graphical_element_editor_delete_button_CB.  Invalid argument(s)");
		}
	/*check if autoapply*/
	AutoApplyorNot(scene_editor->gt_element_group,
								 scene_editor->edit_gt_element_group);
	}

	void MoveUpInSettingList(wxCommandEvent &event)
	{
	int position;
	GT_element_settings *settings;
	if (scene_editor->edit_gt_element_group)
	{
		if (1 < (position = GT_element_group_get_settings_position(
			scene_editor->edit_gt_element_group, scene_editor->current_settings)))
		{
			settings = scene_editor->current_settings;
			ACCESS(GT_element_settings)(settings);
			GT_element_group_remove_settings(scene_editor->edit_gt_element_group,
				scene_editor->current_settings);
			GT_element_group_add_settings(scene_editor->edit_gt_element_group,
				scene_editor->current_settings, position - 1);
			DEACCESS(GT_element_settings)(&settings);
						graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
						graphicalitemschecklist->Clear();
						for_each_settings_in_GT_element_group(scene_editor->edit_gt_element_group,
						  Scene_editor_add_element_settings_item, (void *)scene_editor);
						graphicalitemschecklist->SetSelection(position-2);
						UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(position-2)));
			/* By default the settings name is the position, so it needs to be updated
			 even though the settings hasn't actually changed */
			/* inform the client of the change */
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphical_element_editor_up_button_CB.  Invalid argument(s)");
	}

	}

	void MoveDownInSettingList(wxCommandEvent &event)
	{
		int position;
		GT_element_settings *settings;

		if 	(scene_editor->edit_gt_element_group)
			{
				if (GT_element_group_get_number_of_settings(
					   scene_editor->edit_gt_element_group) >
						(position = GT_element_group_get_settings_position(
							scene_editor->edit_gt_element_group, scene_editor->current_settings)))
					{
						settings = scene_editor->current_settings;
						ACCESS(GT_element_settings)(settings);
						GT_element_group_remove_settings(scene_editor->edit_gt_element_group,
						   scene_editor->current_settings);
						GT_element_group_add_settings(scene_editor->edit_gt_element_group,
							 scene_editor->current_settings, position + 1);
						DEACCESS(GT_element_settings)(&settings);
						//		Graphical_element_editor_update_settings_list(scene_editor);
						graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
						graphicalitemschecklist->Clear();
						for_each_settings_in_GT_element_group(scene_editor->edit_gt_element_group,
						  Scene_editor_add_element_settings_item, (void *)scene_editor);
						graphicalitemschecklist->SetSelection(position);
						UpdateGraphicalElementList(static_cast<GT_element_settings*>(graphicalitemschecklist->GetClientData(position)));
			/* By default the settings name is the position, so it needs to be updated
			 even though the settings hasn't actually changed */
			/* inform the client of the change */
					}
			}
		else
			{
				display_message(ERROR_MESSAGE,
												"graphical_element_editor_down_button_CB.  Invalid argument(s)");
			}
	}

	void RenewGeneralSettingChooser(GT_element_group *edit_gt_element_group)
	{
		computed_field_chooser->set_object(GT_element_group_get_default_coordinate_field(edit_gt_element_group));
		nativediscretizationcheckbox = XRCCTRL(*this, "NativeDiscretisationFieldCheckBox",wxCheckBox);
		native_discretization_field_chooser_panel = XRCCTRL(*this, "NativeDiscretisationFieldChooser",wxPanel);
		if (nativediscretizationcheckbox->IsChecked())
			{
			native_discretization_field_chooser_panel->Enable();
			}
		else
			{
			native_discretization_field_chooser_panel->Disable();
			}			
	}

	void SettingEditorNameText(wxCommandEvent &event)
	{
		char *name,new_name[200],*text_entry;
		wxString temp;
		int position;
		GT_element_settings *settings = scene_editor->current_settings;
		if (scene_editor->current_settings)
		{
			graphicalitemschecklist=XRCCTRL(*this,"GraphicalItemsListBox",wxCheckListBox);
			GET_NAME(GT_element_settings)(scene_editor->current_settings,
																		&name);
			nametextfield = XRCCTRL(*this, "NameTextField", wxTextCtrl);
			temp = nametextfield->GetValue();
			text_entry = const_cast<char *>(temp.c_str());
			if (text_entry)
			{
				sscanf(text_entry, "%s", new_name);
				if (strcmp(name, new_name))
				{
					GT_element_settings_set_name(
						scene_editor->current_settings, new_name);
					/* inform the client of the change */
				}
				nametextfield->SetValue(new_name);
				position = GT_element_group_get_settings_position(
				    scene_editor->edit_gt_element_group, scene_editor->current_settings);

				graphicalitemschecklist->Clear();
				for_each_settings_in_GT_element_group(scene_editor->edit_gt_element_group,
				  Scene_editor_add_element_settings_item, (void *)scene_editor);
				if (position>=1)
					{
						graphicalitemschecklist->SetSelection(position-1);
						UpdateGraphicalElementList(settings);
					}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"settings_editor_constant_radius_text_CB.  Missing text");
			}
			DEALLOCATE(name);
		}
		else
		{
		 	display_message(ERROR_MESSAGE,
		      "settings_editor_constant_radius_text_CB.  Invalid argument(s)");
		}
	}

	void CoordinateFieldChecked(wxCommandEvent &event)
	{
		Computed_field *coordinate_field;
		coordinatefieldcheckbox = XRCCTRL(*this, "CoordinateFieldCheckBox",wxCheckBox);
		coordinate_field_chooser_panel = XRCCTRL(*this, "CoordinateFieldChooserPanel",wxPanel);
		if (coordinatefieldcheckbox->IsChecked())
			{
				coordinate_field=coordinate_field_chooser->get_object();
				coordinate_field_chooser_panel->Enable();
			}
		else
			{
				coordinate_field=(Computed_field *)NULL;
				coordinate_field_chooser_panel->Disable();
			}
				GT_element_settings_set_coordinate_field(
					scene_editor->current_settings, coordinate_field);
				AutoApplyorNot(scene_editor->gt_element_group,
					scene_editor->edit_gt_element_group);
	}

	void EnterRadius(wxCommandEvent &event)
 	{
		float constant_radius, scale_factor;
		double temp;
		struct Computed_field *radius_scalar_field;
		constantradiustextctrl=XRCCTRL(*this, "ConstantRadiusTextCtrl",wxTextCtrl);
		(constantradiustextctrl->GetValue()).ToDouble(&temp);
		constant_radius=(float)temp;
		scalefactorstextctrl=XRCCTRL(*this,"ScaleFactorsTextCtrl",wxTextCtrl);
		(scalefactorstextctrl->GetValue()).ToDouble(&temp);
		scale_factor=(float)temp;	
		radiusscalarcheckbox=XRCCTRL(*this, "RadiusScalarCheckBox",wxCheckBox);
		radius_scalar_chooser_panel=XRCCTRL(*this, "RadiusScalarChooserPanel",wxPanel);
		scalefactor = XRCCTRL(*this,"ScaleFactorLabel", wxStaticText);
		if (radiusscalarcheckbox->IsChecked())
			{
				scalefactorstextctrl->Enable();
				radius_scalar_chooser_panel->Enable();
				scalefactor ->Enable();
		  	   radius_scalar_field=radius_scalar_chooser->get_object();
			}
		else	
			{
				radius_scalar_field=(Computed_field *)NULL;
				radius_scalar_chooser_panel->Disable();
				scalefactorstextctrl->Disable();	
				scalefactor ->Disable();
			}
		GT_element_settings_set_radius_parameters(scene_editor->current_settings,constant_radius,
																							scale_factor,radius_scalar_field);
				AutoApplyorNot(scene_editor->gt_element_group,
					scene_editor->edit_gt_element_group);
 	}

	void SetBothMaterialChooser(GT_element_settings *settings)
	{
		graphical_material_chooser->set_object(GT_element_settings_get_material
		 (settings));
		selected_material_chooser->set_object(GT_element_settings_get_selected_material
		 (settings));
	}

	void SetCoordinateFieldChooser(GT_element_settings *settings)
	{
		Computed_field *coordinate_field;
		coordinatefieldcheckbox = XRCCTRL(*this, "CoordinateFieldCheckBox", wxCheckBox);
		coordinate_field_chooser_panel = XRCCTRL(*this, "CoordinateFieldChooserPanel", wxPanel);
		if (coordinate_field=
				GT_element_settings_get_coordinate_field(settings))
			{
				coordinate_field_chooser->set_object(coordinate_field);
				coordinatefieldcheckbox->SetValue(1);	
				coordinate_field_chooser_panel->Enable();
			}
		else
			{
				coordinatefieldcheckbox->SetValue(0);	
				coordinate_field_chooser_panel->Disable();
			}
	}

	void SetSelectMode(GT_element_settings *settings)
	{
			select_mode_chooser->set_value(GT_element_settings_get_select_mode(
			   settings));
	}

	void SetSettings(GT_element_settings *settings)
	{
// 		struct Computed_field *coordinate_field, *data_field, *iso_scalar_field,
// 		*label_field, *radius_scalar_field, *stream_vector_field,
// 		*texture_coord_field, *xi_point_density_field;
		char temp_string[50];
		struct Computed_field *radius_scalar_field;
		float constant_radius,scale_factor;
		constantradiustextctrl=XRCCTRL(*this, "ConstantRadiusTextCtrl",wxTextCtrl);
		radiusscalarcheckbox=XRCCTRL(*this, "RadiusScalarCheckBox",wxCheckBox);
		scalefactorstextctrl=XRCCTRL(*this,"ScaleFactorsTextCtrl",wxTextCtrl);
		 radius_scalar_chooser_panel=XRCCTRL(*this, "RadiusScalarChooserPanel",wxPanel);
		 constantradius = XRCCTRL(*this,"ConstantRadiusText",wxStaticText);
		 scalefactor = XRCCTRL(*this,"ScaleFactorLabel", wxStaticText);
		if (GT_ELEMENT_SETTINGS_CYLINDERS==scene_editor->current_settings_type)
			{
				GT_element_settings_get_radius_parameters(settings,
				  &constant_radius,&scale_factor,&radius_scalar_field);
				constantradiustextctrl->Show();
				radiusscalarcheckbox->Show();
				scalefactorstextctrl->Show();
				constantradius->Show();
				radius_scalar_chooser_panel->Show();
				scalefactor->Show();
				sprintf(temp_string,"%g",constant_radius);
				constantradiustextctrl->SetValue(temp_string);
				sprintf(temp_string,"%g",scale_factor);
				scalefactorstextctrl->SetValue(temp_string);

				radius_scalar_chooser = 
					new Managed_object_chooser<Computed_field,MANAGER_CLASS(Computed_field)>
					(radius_scalar_chooser_panel, scene_editor->radius_scalar_field, scene_editor->computed_field_manager,
					 Computed_field_is_scalar, (void *)NULL, scene_editor->user_interface);
				Callback_base< Computed_field* > *radius_scalar_callback = 
					new Callback_member_callback< Computed_field*, 
					wxSceneEditor, int (wxSceneEditor::*)(Computed_field *) >
					(this, &wxSceneEditor::radius_scalar_callback);
				radius_scalar_chooser->set_callback(radius_scalar_callback);
				if ((struct Computed_field *)NULL!=radius_scalar_field)
			    {
				  radius_scalar_chooser->set_object(radius_scalar_field);
				  scalefactorstextctrl->Enable();
				  radiusscalarcheckbox->SetValue(1);
				  radius_scalar_chooser_panel->Enable();
				  scalefactor->Enable();
			     }
				else
					{
						scalefactorstextctrl->Disable();
						radiusscalarcheckbox->SetValue(0);
						radius_scalar_chooser_panel->Disable();
						scalefactor->Disable();
					}
			}
		else
			{
				constantradiustextctrl->Hide();
				radiusscalarcheckbox->Hide();
				scalefactorstextctrl->Hide();
				radius_scalar_chooser_panel->Hide();
				scalefactor->Hide();
				constantradius->Hide();
			}
  	}

  DECLARE_DYNAMIC_CLASS(wxSceneEditor);
  DECLARE_EVENT_TABLE();
};

IMPLEMENT_DYNAMIC_CLASS(wxSceneEditor, wxFrame)

BEGIN_EVENT_TABLE(wxSceneEditor, wxFrame)
	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("LowerSplitter"),wxSceneEditor::ResetWindow)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("CollapsiblePane"), wxSceneEditor::ResetScrolledWindow)
	EVT_TEXT_ENTER(XRCID("CircleDiscretisationPanel"), wxSceneEditor::CircleDiscretisationUpdate)
	EVT_TEXT_ENTER(XRCID("ElementDiscretisationPanel"), wxSceneEditor::ElementDiscretisationUpdate)
	EVT_CHECKBOX(XRCID("NativeDiscretisationFieldCheckBox"),wxSceneEditor::NativeDiscretisationFieldChecked)
	EVT_CHECKBOX(XRCID("AutoCheckBox"),wxSceneEditor::AutoChecked)
	EVT_BUTTON(XRCID("ApplyButton"),wxSceneEditor::ApplyClicked)
	EVT_CHECKLISTBOX(XRCID("SceneCheckList"), wxSceneEditor::SceneCheckListClicked)
	EVT_LISTBOX(XRCID("SceneCheckList"), wxSceneEditor::SceneCheckListClicked)
	EVT_BUTTON(XRCID("SceneObjectUpButton"),wxSceneEditor::SceneObjectUpClicked)
	EVT_BUTTON(XRCID("SceneObjectDownButton"),wxSceneEditor::SceneObjectDownClicked)
  	EVT_CHECKLISTBOX(XRCID("GraphicalItemsListBox"), wxSceneEditor::GraphicalItemsListBoxClicked)
  	EVT_LISTBOX(XRCID("GraphicalItemsListBox"), wxSceneEditor::GraphicalItemsListBoxClicked)
	EVT_BUTTON(XRCID("AddButton"),wxSceneEditor::AddToSettingList)
	EVT_BUTTON(XRCID("DelButton"),wxSceneEditor::RemoveFromSettingList)
	EVT_BUTTON(XRCID("UpButton"),wxSceneEditor::MoveUpInSettingList)
	EVT_BUTTON(XRCID("DownButton"),wxSceneEditor::MoveDownInSettingList)
	EVT_TEXT_ENTER(XRCID("NameTextField"),wxSceneEditor::SettingEditorNameText)
	EVT_CHECKBOX(XRCID("CoordinateFieldCheckBox"),wxSceneEditor::CoordinateFieldChecked)
	EVT_TEXT_ENTER(XRCID("ConstantRadiusTextCtrl"), wxSceneEditor::EnterRadius)
	EVT_TEXT_ENTER(XRCID("ScaleFactorsTextCtrl"), wxSceneEditor::EnterRadius)
	EVT_CHECKBOX(XRCID("RadiusScalarCheckBox"), wxSceneEditor::EnterRadius)
END_EVENT_TABLE()

static int set_general_settings(void *scene_editor_void)
/*******************************************************************************
LAST MODIFIED : 16 Match 2007

DESCRIPTION :
Renew the display of  the general settings
==============================================================================*/
{
	Scene_editor *scene_editor = static_cast<Scene_editor*>(scene_editor_void);
	struct Element_discretization element_discretization;
	wxTextCtrl *circlediscretisationpanel =	 XRCCTRL(*scene_editor->wx_scene_editor, "CircleDiscretisationPanel",wxTextCtrl);
	wxTextCtrl *elementdiscretisationpanel =	 XRCCTRL(*scene_editor->wx_scene_editor, "ElementDiscretisationPanel",wxTextCtrl);
	char temp_string[80];
	scene_editor->wx_scene_editor->RenewGeneralSettingChooser(scene_editor->edit_gt_element_group);
	int temp = GT_element_group_get_circle_discretization(scene_editor->edit_gt_element_group);
	wxString circle_discretisation = wxString::Format(wxT("%d"), (int)temp);
	circlediscretisationpanel->SetValue(circle_discretisation);
	GT_element_group_get_element_discretization(
	  scene_editor->edit_gt_element_group,&element_discretization);
	sprintf(temp_string,"%d*%d*%d",
		element_discretization.number_in_xi1,
		element_discretization.number_in_xi2,
		element_discretization.number_in_xi3);
	 elementdiscretisationpanel->SetValue(temp_string);
	return 1;
}

static int get_and_set_graphical_element_settings(void *scene_editor_void)
/*******************************************************************************
LAST MODIFIED : 19 Match 2007

DESCRIPTION :
Get and set the display of  the graphical element settings
==============================================================================*/
{
	Scene_editor *scene_editor = static_cast<Scene_editor*>(scene_editor_void);

	/*for the text field*/
	char *name;
	GET_NAME(GT_element_settings)(scene_editor->current_settings,
																&name);
	wxTextCtrl *nametextfield = XRCCTRL(*scene_editor->wx_scene_editor, "NameTextField", wxTextCtrl);
	nametextfield->SetValue(name);
	DEALLOCATE(name);

	/*for the coordinate field*/
	scene_editor->wx_scene_editor->SetCoordinateFieldChooser(scene_editor->current_settings);
	/*for the selected and material chooser*/
	scene_editor->wx_scene_editor->SetBothMaterialChooser(scene_editor->current_settings);
	/*for the select mode chooser*/
	scene_editor->wx_scene_editor->SetSelectMode(scene_editor->current_settings);
	scene_editor->wx_scene_editor->SetSettings(scene_editor->current_settings);
	wxFrame *frame=XRCCTRL(*scene_editor->wx_scene_editor, "CmguiSceneEditor", wxFrame);
	frame->Layout();
	return 1;
}


static int add_scene_object_to_scene_check_box(struct Scene_object *scene_object,
					      void *scene_editor_void)
/*******************************************************************************
LAST MODIFIED : 2 Match 2007

DESCRIPTION :
Add scene_object as checklistbox item into the box
==============================================================================*/
{
	Scene_editor *scene_editor = static_cast<Scene_editor*>(scene_editor_void);
	wxCheckListBox *checklist = scene_editor->checklistbox;
	char *name;
	int visible;

	ENTER(add_scene_object_to_scene_check_box);
	GET_NAME(Scene_object)(scene_object, &name);
	checklist->Append(name, scene_object);
	visible =(g_VISIBLE == Scene_object_get_visibility(scene_object));
	if ( visible ==1)
		{
			checklist->Check((checklist->GetCount()-1),1);
		}
	if (checklist->GetCount() == 1)
		{
		 	checklist->SetSelection(0);
		switch (Scene_object_get_type(scene_object))
			{
			case SCENE_OBJECT_GRAPHICAL_ELEMENT_GROUP:
				{
					GT_element_group *gt_element_group = Scene_object_get_graphical_element_group(
						scene_object);

					GT_element_group *edit_gt_element_group;
					REACCESS(GT_element_group)(&scene_editor->gt_element_group, gt_element_group);
					if (gt_element_group)
						{
							edit_gt_element_group =
								create_editor_copy_GT_element_group(gt_element_group);
							if (!edit_gt_element_group)
								{
									display_message(ERROR_MESSAGE,
																	"graphical_element_editor_set_gt_element_group.  "
																	"Could not copy graphical element");
								}
						}
					else
						{
							edit_gt_element_group = (struct GT_element_group *)NULL;
						}
					REACCESS(GT_element_group)(&(scene_editor->edit_gt_element_group),
																		 edit_gt_element_group);
					scene_editor->graphicalitemslistbox = XRCCTRL(*scene_editor->wx_scene_editor, "GraphicalItemsListBox",wxCheckListBox);
					scene_editor->graphicalitemslistbox->Clear();
					if (edit_gt_element_group)
						{
							for_each_settings_in_GT_element_group(edit_gt_element_group,
																										Scene_editor_add_element_settings_item, (void *)scene_editor);
							 scene_editor->lower_panel->Show();
							 set_general_settings((void *)scene_editor);
							 get_and_set_graphical_element_settings((void *)scene_editor);
						}
			} break;
			case SCENE_OBJECT_GRAPHICS_OBJECT:
			case SCENE_OBJECT_SCENE:
				{
					/* nothing to do */
				} break;
			}
		}

	DEALLOCATE(name);
	LEAVE;
return(1);
 };

static int Scene_editor_add_element_settings_item(
																									struct GT_element_settings *settings, void *scene_editor_void)
/*******************************************************************************
LAST MODIFIED : 19 November 2001

DESCRIPTION :
Iterator function for Graphical_element_editor_update_Settings_item.
==============================================================================*/
{
	char *settings_string;
	int return_code;
	struct Scene_editor *scene_editor;
	ENTER(Scene_editor_add_element_settings_item);
	if (settings && (scene_editor = static_cast<Scene_editor*>(scene_editor_void)))
	{
		settings_string = GT_element_settings_string(settings,
			SETTINGS_STRING_COMPLETE_PLUS);
		wxCheckListBox *graphicalitemschecklist =  XRCCTRL(*scene_editor->wx_scene_editor, "GraphicalItemsListBox",wxCheckListBox);
		graphicalitemschecklist->Append(settings_string, settings);
		if (  GT_element_settings_get_visibility(settings) ==1)
		{
			graphicalitemschecklist->Check((graphicalitemschecklist->GetCount()-1),1);
		}
		if (graphicalitemschecklist->GetCount() == 1)
		 {
		 	graphicalitemschecklist->SetSelection(0);
			REACCESS(GT_element_settings)(&scene_editor->current_settings,
																		settings);
		 }
      DEALLOCATE(settings_string);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_editor_add_element_settings_item.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_editor_add_element_settings_item */

#endif /* defined (WX_USER_INTERFACE) */

/*
Global functions
----------------
*/

struct Scene_editor *CREATE(Scene_editor)(	
	struct Scene_editor **scene_editor_address,
	struct MANAGER(Scene) *scene_manager, struct Scene *scene,
	struct Computed_field_package *computed_field_package,
	struct Cmiss_region *root_region,
	struct MANAGER(Graphical_material) *graphical_material_manager,
	struct Graphical_material *default_material,
	struct Graphics_font *default_font,
	struct LIST(GT_object) *glyph_list,
	struct MANAGER(Spectrum) *spectrum_manager,
	struct Spectrum *default_spectrum,
	struct MANAGER(VT_volume_texture) *volume_texture_manager,
  struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 2 Febuary 2007

DESCRIPTION :
==============================================================================*/
{
	struct Scene_editor *scene_editor;
 
	ENTER(CREATE(Scene_editor));
	scene_editor = (struct Scene_editor *)NULL;
	if (scene_manager && scene && computed_field_package && root_region &&
		graphical_material_manager && default_material &&
		glyph_list && spectrum_manager && default_spectrum &&
		volume_texture_manager && user_interface)
	{
		if (ALLOCATE(scene_editor,struct Scene_editor,1))
		{		
			scene_editor->auto_apply = 1;
			scene_editor->child_edited =1;
			scene_editor->child_expanded=1; 
			scene_editor->transformation_edited=1;
			scene_editor->transformation_expanded=1;		
			scene_editor->gt_element_group = (struct GT_element_group *)NULL;
			scene_editor->edit_gt_element_group = (struct GT_element_group *)NULL;
			scene_editor->scene = scene;
			scene_editor->graphical_material_manager = graphical_material_manager;
			scene_editor->scene_object = (struct Scene_object *)NULL;
			scene_editor->scene_editor_address = (struct Scene_editor **)NULL;
			scene_editor->scene_manager_callback_id = (void *)NULL;
			scene_editor->default_material=default_material;
			scene_editor->selected_material=default_material;
			scene_editor->default_font=default_font;
			scene_editor->glyph_list=glyph_list;
			scene_editor->scene_manager = scene_manager;
			scene_editor->user_interface=user_interface;
			scene_editor->computed_field_package=computed_field_package ;
			scene_editor->current_settings_type=GT_ELEMENT_SETTINGS_LINES;		
			scene_editor->current_settings=(GT_element_settings*)NULL;
			scene_editor->default_coordinate_field=(Computed_field *)NULL;
			scene_editor->volume_texture_manager=volume_texture_manager;
			scene_editor->computed_field_manager=Computed_field_package_get_computed_field_manager(computed_field_package);
			scene_editor->native_discretization_field=(FE_field*)NULL ;
			scene_editor->coordinate_field=(Computed_field *)NULL;	
			scene_editor->select_mode=(Graphics_select_mode)NULL;
			scene_editor->constant_radius=1.0;
			scene_editor->radius_scale_factor=1.0;
			scene_editor->radius_scalar_field = (Computed_field *)NULL;

#if defined (WX_USER_INTERFACE)
	scene_editor->wx_scene_editor = (wxSceneEditor *)NULL;
	scene_editor->wx_scene_editor = new 
		wxSceneEditor(scene_editor);
	scene_editor->lower_panel=
		XRCCTRL(*scene_editor->wx_scene_editor, "LowerPanel", wxPanel);
	scene_editor->lower_panel->Hide();
	scene_editor->checklistbox = XRCCTRL(*scene_editor->wx_scene_editor, "SceneCheckList", wxCheckListBox);
	scene_editor->checklistbox->Clear();
	for_each_Scene_object_in_Scene(scene,
		add_scene_object_to_scene_check_box, (void *)scene_editor);
 	scene_editor->autocheckbox = 
 	  XRCCTRL(*scene_editor->wx_scene_editor, "AutoCheckBox", wxCheckBox);
	scene_editor->autocheckbox->SetValue(true);
 	scene_editor->applybutton = 
 	  XRCCTRL(*scene_editor->wx_scene_editor, "ApplyButton", wxButton);
	scene_editor->applybutton->Disable();
 	scene_editor->revertbutton = 
 	  XRCCTRL(*scene_editor->wx_scene_editor, "RevertButton", wxButton);
	scene_editor->revertbutton->Disable();
	scene_editor->frame=
		XRCCTRL(*scene_editor->wx_scene_editor, "CmguiSceneEditor", wxFrame);
	scene_editor->frame->Layout();
	scene_editor->frame->SetMinSize(wxSize(50,100));
 	scene_editor->sceneediting = 
 	  XRCCTRL(*scene_editor->wx_scene_editor, "SceneEditing", wxScrolledWindow);
	scene_editor->sceneediting->Layout();
	scene_editor->sceneediting->SetScrollbars(10,10,40,40);
	scene_editor->lowersplitter=XRCCTRL(*scene_editor->wx_scene_editor,"LowerSplitter",wxSplitterWindow);
	scene_editor->lowersplitter->Layout();	
	scene_editor->topsplitter=XRCCTRL(*scene_editor->wx_scene_editor,"TopSplitter",wxSplitterWindow);
	scene_editor->topsplitter->Layout();	
#endif /*  (WX_USER_INTERFACE) */
		}
	}

	LEAVE;

	return (scene_editor);
} /* CREATE(Scene_editor_wx) */

int DESTROY(Scene_editor)(struct Scene_editor **scene_editor_address)
/*******************************************************************************
LAST MODIFIED : 16 November 2001

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Scene_editor *scene_editor;

	ENTER(DESTROY(Scene_editor));
	if (scene_editor_address && (scene_editor = *scene_editor_address) &&
		(scene_editor->scene_editor_address == scene_editor_address))	
	{
		/* must unset the current_object, if any, to remove callbacks */
		//Scene_editor_set_current_object(scene_editor,
		//	(struct Scene_editor_object *)NULL);
		if (scene_editor->scene_manager_callback_id)
		{
			MANAGER_DEREGISTER(Scene)(
				scene_editor->scene_manager_callback_id,
				scene_editor->scene_manager);
			scene_editor->scene_manager_callback_id = (void *)NULL;
		}
		DEACCESS(GT_element_settings)(&scene_editor->current_settings);
	//DESTROY(LIST(Scene_editor_object))(&(scene_editor->scene_editor_objects));
		//if (scene_editor->window_shell)
		//{
		//	destroy_Shell_list_item_from_shell(&(scene_editor->window_shell),
		//		scene_editor->user_interface);
		//}
		DEALLOCATE(*scene_editor_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Scene_editor).  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene_editor) */

int Scene_editor_bring_to_front(struct Scene_editor *scene_editor)
/*******************************************************************************
LAST MODIFIED : 31 October 2001

DESCRIPTION :
De-iconifies and brings the scene editor to the front.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_editor_bring_to_front);
	if (scene_editor)
	{
		/* bring up the dialog */
		scene_editor->wx_scene_editor->Raise();
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_editor_bring_to_front.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_editor_bring_to_front */
struct Scene *Scene_editor_get_scene(struct Scene_editor *scene_editor)
/*******************************************************************************
LAST MODIFIED : 5 November 2001

DESCRIPTION :
Returns the root scene of the <scene_editor>.
==============================================================================*/
{
	struct Scene *scene;

	ENTER(Scene_editor_get_scene);
	if (scene_editor)
	{
	//	scene = CHOOSE_OBJECT_GET_OBJECT(Scene)(scene_editor->scene_widget);
		//scene_editor->wx_scene_editor->Raise();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_editor_get_scene.  Invalid argument(s)");
		scene = (struct Scene *)NULL;
	}
	LEAVE;

	return (scene);
} /* Scene_editor_get_scene */


int Scene_editor_set_scene(struct Scene_editor *scene_editor,
	struct Scene *scene)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Sets the root scene of the <scene_editor>. Updates widgets.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_editor_set_scene);
	if (scene_editor && scene)
	{
		if (scene == Scene_editor_get_scene(scene_editor))
		{
			return_code = 1;
		}
#if defined (WX_USER_INTERFACE)
			if (scene_editor->wx_scene_editor)
			{
				scene_editor->wx_scene_editor->setSceneObject(scene);
			}
#endif /* defined (WX_USER_INTERFACE) */

	 {
			{
				display_message(ERROR_MESSAGE,
					"Scene_editor_set_scene.  Could not set new scene");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_editor_set_scene.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_editor_set_scene */


