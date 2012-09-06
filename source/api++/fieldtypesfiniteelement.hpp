/***************************************************************************//**
 * FILE : fieldtypesfiniteelement.hpp
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
 * The Original Code is libZinc.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2011
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
#ifndef __FIELD_TYPES_FINITE_ELEMENT_HPP__
#define __FIELD_TYPES_FINITE_ELEMENT_HPP__

extern "C" {
#include "api/cmiss_field_finite_element.h"
}
#include "api++/field.hpp"
#include "api++/fieldmodule.hpp"
#include "api++/element.hpp"

namespace Zn
{

class FieldFiniteElement : public Field
{
public:

	FieldFiniteElement() : Field(NULL)
	{ }

	FieldFiniteElement(Cmiss_field_id field_id) : Field(field_id)
	{	}

	FieldFiniteElement(Field& field) :
		Field(reinterpret_cast<Cmiss_field_id>(Cmiss_field_cast_finite_element(field.getId())))
	{	}

};

class FieldEmbedded : public Field
{
public:

	FieldEmbedded() : Field(NULL)
	{ }

	FieldEmbedded(Cmiss_field_id field_id) : Field(field_id)
	{	}

};

class FieldFindMeshLocation : public Field
{
public:

	FieldFindMeshLocation() : Field(NULL)
	{ }

	FieldFindMeshLocation(Cmiss_field_id field_id) : Field(field_id)
	{	}

	FieldFindMeshLocation(Field& field) :
		Field(reinterpret_cast<Cmiss_field_id>(Cmiss_field_cast_find_mesh_location(field.getId())))
	{	}

	enum SearchMode
	{
		SEARCH_MODE_FIND_EXACT = CMISS_FIELD_FIND_MESH_LOCATION_SEARCH_MODE_FIND_EXACT,
		SEARCH_MODE_FIND_NEAREST = CMISS_FIELD_FIND_MESH_LOCATION_SEARCH_MODE_FIND_NEAREST,
	};

	Mesh getMesh()
	{
		return Mesh(Cmiss_field_find_mesh_location_get_mesh(
			reinterpret_cast<Cmiss_field_find_mesh_location_id>(id)));
	}

	SearchMode getSearchMode()
	{
		return static_cast<SearchMode>(Cmiss_field_find_mesh_location_get_search_mode(
			reinterpret_cast<Cmiss_field_find_mesh_location_id>(id)));
	}

	int setSearchMode(SearchMode searchMode)
	{
		return Cmiss_field_find_mesh_location_set_search_mode(
			reinterpret_cast<Cmiss_field_find_mesh_location_id>(id),
			static_cast<Cmiss_field_find_mesh_location_search_mode>(searchMode));
	}
};

class FieldNodeValue : public Field
{
public:

	FieldNodeValue() : Field(NULL)
	{ }

	FieldNodeValue(Cmiss_field_id field_id) : Field(field_id)
	{	}

};

class FieldStoredMeshLocation : public Field
{
public:

	FieldStoredMeshLocation() : Field(NULL)
	{ }

	FieldStoredMeshLocation(Cmiss_field_id field_id) : Field(field_id)
	{	}

	FieldStoredMeshLocation(Field& field) :
		Field(reinterpret_cast<Cmiss_field_id>(
			Cmiss_field_cast_stored_mesh_location(field.getId())))
	{	}
};

class FieldStoredString : public Field
{
public:

	FieldStoredString() : Field(NULL)
	{ }

	FieldStoredString(Cmiss_field_id field_id) : Field(field_id)
	{	}

	FieldStoredString(Field& field) :
		Field(reinterpret_cast<Cmiss_field_id>(
			Cmiss_field_cast_stored_string(field.getId())))
	{	}
};

inline FieldFiniteElement FieldModule::createFiniteElement(int numberOfComponents)
{
	return FieldFiniteElement(Cmiss_field_module_create_finite_element(id,
		numberOfComponents));
}

inline FieldEmbedded FieldModule::createEmbedded(Field& sourceField, Field& embeddedLocationField)
{
	return FieldEmbedded(Cmiss_field_module_create_embedded(id,
		sourceField.getId(), embeddedLocationField.getId()));
}

inline FieldFindMeshLocation FieldModule::createFindMeshLocation(
	Field sourceField, Field meshField, Mesh mesh)
{
	return FieldFindMeshLocation(Cmiss_field_module_create_find_mesh_location(id,
		sourceField.getId(), meshField.getId(), mesh.getId()));
}

inline FieldNodeValue FieldModule::createNodeValue(Field sourceField,
	NodalValueType nodalValueType, int versionNumber)
{
	return FieldNodeValue(Cmiss_field_module_create_node_value(id,
		sourceField.getId(), static_cast<Cmiss_nodal_value_type>(nodalValueType),
		versionNumber));
}

inline FieldStoredMeshLocation FieldModule::createStoredMeshLocation(Mesh mesh)
{
	return FieldStoredMeshLocation(Cmiss_field_module_create_stored_mesh_location(id,
		mesh.getId()));
}

}  // namespace Zn
#endif /* __FIELD_TYPES_FINITE_ELEMENT_HPP__ */
