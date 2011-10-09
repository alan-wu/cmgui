/***************************************************************************//**
 * FILE : CmissMaterial.hpp
 */
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
 * Portions created by the Initial Developer are Copyright (C) 2010
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
#ifndef __CMISS_GRAPHICS_MATERIAL_HPP__
#define __CMISS_GRAPHICS_MATERIAL_HPP__

extern "C" {
#include "api/cmiss_graphics_material.h"
}

namespace Cmiss
{

class GraphicsMaterial
{
protected:
	Cmiss_graphics_material_id id;

public:
	GraphicsMaterial() : id(NULL)
	{	}

	// takes ownership of C-style field reference
	GraphicsMaterial(Cmiss_graphics_material_id in_material_id): id(in_material_id)
	{ }

	GraphicsMaterial(const GraphicsMaterial& material) : id(Cmiss_graphics_material_access(material.id))
	{ }

	GraphicsMaterial& operator=(const GraphicsMaterial& material)
	{
		Cmiss_graphics_material_id temp_id = Cmiss_graphics_material_access(material.id);
		if (NULL != id)
		{
			Cmiss_graphics_material_destroy(&id);
		}
		id = temp_id;
		return *this;
	}
	
	~GraphicsMaterial()
	{
		if (NULL != id)
		{
			Cmiss_graphics_material_destroy(&id);
		}
	}

	int getAttributeInteger(Cmiss_graphics_material_attribute attribute)
	{
		return Cmiss_graphics_material_get_attribute_integer(id, attribute);
	}

	int setAttributeInteger(Cmiss_graphics_material_attribute attribute, int value)
	{
		return Cmiss_graphics_material_set_attribute_integer(id, attribute, value);
	}

	char *getName()
	{
		return Cmiss_graphics_material_get_name(id);
	}

	int setName(const char *name)
	{
		return Cmiss_graphics_material_set_name(id, name);
	}

	int setAlpha(float alpha)
	{
		return Cmiss_graphics_material_set_alpha(id, alpha);
	}

	int setShininess(float shininess)
	{
		return Cmiss_graphics_material_set_shininess(id, shininess);
	}

	int setAmbient(float red, float green, float blue)
	{
		return Cmiss_graphics_material_set_ambient(id, red, green, blue);
	}

	int setDiffuse(float red, float green, float blue)
	{
		return Cmiss_graphics_material_set_diffuse(id, red, green, blue);
	}

	int setEmission(float red, float green, float blue)
	{
		return Cmiss_graphics_material_set_emission(id, red, green, blue);
	}

	int setSpecular(float red, float green, float blue)
	{
		return Cmiss_graphics_material_set_specular(id, red, green, blue);
	}

};

} // namespace Cmiss

#endif /* __CMISS_GRAPHICS_MATERIAL_HPP__ */
