/*******************************************************************************
FILE : cmiss_texture.c

LAST MODIFIED : 4 December 2007

DESCRIPTION :
The public interface to the Cmiss_texture object.
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
 * Shane Blackett shane at blackett.co.nz
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

#include "api/cmiss_texture.h"
#include "general/debug.h"
#include "general/image_utilities.h"
#include "graphics/texture.h"
#include "user_interface/message.h"

/*
Global functions
----------------
*/

enum Cmiss_texture_combine_mode Cmiss_texture_get_combine_mode(Cmiss_texture_id texture)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
Returns how the texture is combined with the material: blend, decal or modulate.
==============================================================================*/
{
	enum Texture_combine_mode texture_combine_mode;
	enum Cmiss_texture_combine_mode combine_mode;

	ENTER(Cmiss_texture_get_combine_mode);
	if (texture)
	{
      texture_combine_mode = Texture_get_combine_mode(texture);
      switch(texture_combine_mode)
		{
			case TEXTURE_BLEND:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_BLEND;
			} break;
			case TEXTURE_DECAL:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_DECAL;
			} break;
			case TEXTURE_MODULATE:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_MODULATE;
			} break;
			case TEXTURE_ADD:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_ADD;
			} break;
			case TEXTURE_ADD_SIGNED:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_ADD_SIGNED;
			} break;
			case TEXTURE_MODULATE_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_MODULATE_SCALE_4;
			} break;
			case TEXTURE_BLEND_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_BLEND_SCALE_4;
			} break;
			case TEXTURE_SUBTRACT:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_SUBTRACT;
			} break;
			case TEXTURE_ADD_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_ADD_SCALE_4;
			} break;
			case TEXTURE_SUBTRACT_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_SUBTRACT_SCALE_4;
			} break;
			case TEXTURE_INVERT_ADD_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_INVERT_ADD_SCALE_4;
			} break;
			case TEXTURE_INVERT_SUBTRACT_SCALE_4:
			{
				combine_mode = CMISS_TEXTURE_COMBINE_INVERT_SUBTRACT_SCALE_4;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_texture_get_combine_mode.  "
					"Combine mode not supported in public interface.");
				combine_mode = CMISS_TEXTURE_COMBINE_DECAL;
			} break;
		}
	}
	else
	{
      display_message(ERROR_MESSAGE,
			"Cmiss_texture_get_combine_mode.  Invalid argument(s)");
		combine_mode = CMISS_TEXTURE_COMBINE_DECAL;
	}
	LEAVE;

	return (combine_mode);
} /* Cmiss_texture_get_combine_mode */

int Cmiss_texture_set_combine_mode(Cmiss_texture_id texture,
   enum Cmiss_texture_combine_mode combine_mode)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
Sets how the texture is combined with the material: blend, decal or modulate.
==============================================================================*/
{
  int return_code;

  ENTER(Cmiss_texture_set_combine_mode);
  if (texture)
  {
	  switch(combine_mode)
	  {
		  case CMISS_TEXTURE_COMBINE_BLEND:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_BLEND);
		  } break;
		  case CMISS_TEXTURE_COMBINE_DECAL:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_DECAL);
		  } break;
		  case CMISS_TEXTURE_COMBINE_MODULATE:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_MODULATE);
		  } break;
		  case CMISS_TEXTURE_COMBINE_ADD:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_ADD);
		  } break;
		  case CMISS_TEXTURE_COMBINE_ADD_SIGNED:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_ADD_SIGNED);
		  } break;
		  case CMISS_TEXTURE_COMBINE_MODULATE_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_MODULATE_SCALE_4);
		  } break;
		  case CMISS_TEXTURE_COMBINE_BLEND_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_BLEND_SCALE_4);
		  } break;
		  case CMISS_TEXTURE_COMBINE_SUBTRACT:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_SUBTRACT);
		  } break;
		  case CMISS_TEXTURE_COMBINE_ADD_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_ADD_SCALE_4);
		  } break;
		  case CMISS_TEXTURE_COMBINE_SUBTRACT_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_SUBTRACT_SCALE_4);
		  } break;
		  case CMISS_TEXTURE_COMBINE_INVERT_ADD_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_INVERT_ADD_SCALE_4);
		  } break;
		  case CMISS_TEXTURE_COMBINE_INVERT_SUBTRACT_SCALE_4:
		  {
			  return_code = Texture_set_combine_mode(texture, 
				  TEXTURE_INVERT_SUBTRACT_SCALE_4);
		  } break;
		  default:
		  {
			  display_message(ERROR_MESSAGE,
				  "Cmiss_texture_set_combine_mode.  "
				  "Unknown combine mode.");
			  return_code = 0;
		  } break;
	  }
  }
  else
    {
      display_message(ERROR_MESSAGE,
		      "Cmiss_texture_set_combine_mode.  Invalid argument(s)");
      return_code=0;
    }
  LEAVE;

  return (return_code);
} /* Cmiss_texture_set_combine_mode */

enum Cmiss_texture_compression_mode Cmiss_texture_get_compression_mode(
   Cmiss_texture_id texture)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
==============================================================================*/
{
	enum Texture_compression_mode texture_compression_mode;
	enum Cmiss_texture_compression_mode compression_mode;

	ENTER(Cmiss_texture_get_compression_mode);
	if (texture)
	{
      texture_compression_mode = Texture_get_compression_mode(texture);
      switch(texture_compression_mode)
		{
			case TEXTURE_UNCOMPRESSED:
			{
				compression_mode = CMISS_TEXTURE_COMPRESSION_UNCOMPRESSED;
			} break;
			case TEXTURE_COMPRESSED_UNSPECIFIED:
			{
				compression_mode = CMISS_TEXTURE_COMPRESSION_COMPRESSED_UNSPECIFIED;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_texture_get_compression_mode.  "
					"Compression mode not supported in public interface.");
				compression_mode = CMISS_TEXTURE_COMPRESSION_UNCOMPRESSED;
			} break;
		}
	}
	else
	{
      display_message(ERROR_MESSAGE,
			"Cmiss_texture_get_compression_mode.  Invalid argument(s)");
		compression_mode = CMISS_TEXTURE_COMPRESSION_UNCOMPRESSED;
	}
	LEAVE;

	return (compression_mode);
} /* Cmiss_texture_get_compression_mode */

int Cmiss_texture_set_compression_mode(Cmiss_texture_id texture,
   enum Cmiss_texture_compression_mode compression_mode)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
Indicate to the graphics hardware how you would like the texture stored in
graphics memory.
==============================================================================*/
{
  int return_code;

  ENTER(Cmiss_texture_set_compression_mode);
  if (texture)
  {
	  switch(compression_mode)
	  {
		  case CMISS_TEXTURE_COMPRESSION_UNCOMPRESSED:
		  {
			  return_code = Texture_set_compression_mode(texture, 
				  TEXTURE_UNCOMPRESSED);
		  } break;
		  case CMISS_TEXTURE_COMPRESSION_COMPRESSED_UNSPECIFIED:
		  {
			  return_code = Texture_set_compression_mode(texture, 
				  TEXTURE_COMPRESSED_UNSPECIFIED);
		  } break;
		  default:
		  {
			  display_message(ERROR_MESSAGE,
				  "Cmiss_texture_set_compression_mode.  "
				  "Unknown compression mode.");
			  return_code = 0;
		  } break;
	  }
  }
  else
    {
      display_message(ERROR_MESSAGE,
		      "Cmiss_texture_set_compression_mode.  Invalid argument(s)");
      return_code=0;
    }
  LEAVE;

  return (return_code);
} /* Cmiss_texture_set_compression_mode */

enum Cmiss_texture_filter_mode Cmiss_texture_get_filter_mode(
   Cmiss_texture_id texture)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
==============================================================================*/
{
	enum Texture_filter_mode texture_filter_mode;
	enum Cmiss_texture_filter_mode filter_mode;

	ENTER(Cmiss_texture_get_filter_mode);
	if (texture)
	{
      texture_filter_mode = Texture_get_filter_mode(texture);
      switch(texture_filter_mode)
		{
			case TEXTURE_LINEAR_FILTER:
			{
				filter_mode = CMISS_TEXTURE_FILTER_LINEAR;
			} break;
			case TEXTURE_NEAREST_FILTER:
			{
				filter_mode = CMISS_TEXTURE_FILTER_NEAREST;
			} break;
			case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
			{
				filter_mode = CMISS_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
			} break;
			case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
			{
				filter_mode = CMISS_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
			} break;
			case TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER:
			{
				filter_mode = CMISS_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_texture_get_filter_mode.  "
					"Filter mode not supported in public interface.");
				filter_mode = CMISS_TEXTURE_FILTER_NEAREST;
			} break;
		}
	}
	else
	{
      display_message(ERROR_MESSAGE,
			"Cmiss_texture_get_filter_mode.  Invalid argument(s)");
		filter_mode = CMISS_TEXTURE_FILTER_NEAREST;
	}
	LEAVE;

	return (filter_mode);
} /* Cmiss_texture_get_filter_mode */


int Cmiss_texture_set_filter_mode(Cmiss_texture_id texture,
   enum Cmiss_texture_filter_mode filter_mode)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
Specfiy how the graphics hardware rasterises the texture onto the screen.
==============================================================================*/
{
  int return_code;

  ENTER(Cmiss_texture_set_filter_mode);
  if (texture)
  {
	  switch(filter_mode)
	  {
		  case CMISS_TEXTURE_FILTER_NEAREST:
		  {
			  return_code = Texture_set_filter_mode(texture, 
				  TEXTURE_NEAREST_FILTER);
		  } break;
		  case CMISS_TEXTURE_FILTER_LINEAR:
		  {
			  return_code = Texture_set_filter_mode(texture, 
				  TEXTURE_LINEAR_FILTER);
		  } break;
		  case CMISS_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		  {
			  return_code = Texture_set_filter_mode(texture, 
				  TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER);
		  } break;
		  case CMISS_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		  {
			  return_code = Texture_set_filter_mode(texture, 
				  TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER);
		  } break;
		  case CMISS_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		  {
			  return_code = Texture_set_filter_mode(texture, 
				  TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER);
		  } break;
		  default:
		  {
			  display_message(ERROR_MESSAGE,
				  "Cmiss_texture_set_filter_mode.  "
				  "Unknown filter mode.");
			  return_code = 0;
		  } break;
	  }
  }
  else
  {
	  display_message(ERROR_MESSAGE,
		  "Cmiss_texture_set_filter_mode.  Invalid argument(s)");
	  return_code=0;
  }
  LEAVE;

  return (return_code);
} /* Cmiss_texture_set_filter_mode */

int Cmiss_texture_set_pixels(Cmiss_texture_id texture,
	int width, int height, int depth,
	int number_of_components, int number_of_bytes_per_component,
	int source_width_bytes, unsigned char *source_pixels)
/*******************************************************************************
LAST MODIFIED : 4 December 2007

DESCRIPTION :
Sets the texture data to the specified <width>, <height> and <depth>, with
<number_of_components> where 1=luminance, 2=LuminanceA, 3=RGB, 4=RGBA, and
<number_of_bytes_per_component> which may be 1 or 2.
Data for the image is taken from <source_pixels> which has <source_width_bytes>
of at least <width>*<number_of_components>*<number_of_bytes_per_component>.
The source_pixels are stored in rows from the bottom to top and from left to
right in each row. Pixel colours are interleaved, eg. RGBARGBARGBA...
==============================================================================*/
{
	enum Texture_storage_type storage;
	int depth_plane, frame_bytes, return_code;

	ENTER(Cmiss_texture_set_pixels);
	if ((0 < width) && (0 < height) &&
		(1 <= number_of_components) && (number_of_components <= 4) &&
		((1 == number_of_bytes_per_component) ||
			(2 == number_of_bytes_per_component)) &&
		(width*number_of_components*number_of_bytes_per_component <=
			source_width_bytes) && source_pixels)
	{
		return_code = 1;

		switch (number_of_components)
		{
			case 1:
			{
				storage = TEXTURE_LUMINANCE;
			} break;
			case 2:
			{
				storage = TEXTURE_LUMINANCE_ALPHA;
			} break;
			case 3:
			{
				storage = TEXTURE_RGB;
			} break;
			case 4:
			{
				storage = TEXTURE_RGBA;
			} break;
			case 5:
			{
				storage = TEXTURE_ABGR;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_texture_set_pixels.  Invalid number of components");
				return_code = 0;
			}
		}

		if (return_code)
		{
			return_code = Texture_allocate_image(texture, width, height, depth,
				storage, number_of_bytes_per_component,
				"api_data");
		}


		if (return_code)
		{
			frame_bytes = source_width_bytes * height;
			for (depth_plane = 0 ; depth_plane < depth ; depth_plane++)
			{
				Texture_set_image_block(texture,
					/*left*/0, /*bottom*/0, width, height, depth_plane,
					source_width_bytes, source_pixels);
				source_pixels += frame_bytes;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_texture_set_pixels.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_texture_set_pixels */

int Cmiss_texture_get_pixels(Cmiss_texture_id texture,
	unsigned int left, unsigned int bottom, unsigned int depth_start,
	int width, int height, int depth,
	unsigned int padded_width_bytes, unsigned int number_of_fill_bytes, 
	unsigned char *fill_bytes,
	int components, unsigned char *destination_pixels)
/*******************************************************************************
LAST MODIFIED : 4 December 2007

DESCRIPTION :
Fills <destination_pixels> with all or part of <texture>.
The <left>, <bottom>, <depth_start> and  <width>, <height>, <depth> specify the part of <cmgui_image> output and must be wholly within its bounds.
Image data is ordered from the bottom row to the top, and within each row from
the left to the right, and from the front to back.
If <components> is > 0, the specified components are output at each pixel, 
otherwise all the number_of_components components of the image are output at each pixel.
Pixel values relate to components by:
  1 -> I    = Intensity;
  2 -> IA   = Intensity Alpha;
  3 -> RGB  = Red Green Blue;
  4 -> RGBA = Red Green Blue Alpha;
  5 -> BGR  = Blue Green Red

If <padded_width_bytes> is zero, image data for subsequent rows follows exactly
after the right-most pixel of the row below. If a positive number is specified,
which must be greater than <width>*number_of_components*
number_of_bytes_per_component in <cmgui_image>, each
row of the output image will take up the specified number of bytes, with
pixels beyond the extracted image <width> undefined.
If <number_of_fill_bytes> is positive, the <fill_bytes> are repeatedly output
to fill the padded row; the cycle of outputting <fill_bytes> starts at the
left of the image to make a more consitent output if more than one colour is
specified in them -- it makes no difference if <number_of_fill_bytes> is 1 or
equal to the number_of_components.
<destination_pixels> must be large enough to take the greater of
<depth> *(<padded_width_bytes> or <width>)*
<height>*number_of_components*number_of_bytes_per_component in the image.
==============================================================================*/
{
	struct Cmgui_image *cmgui_image;
	int number_of_components = 0, return_code;
	unsigned char *frame_pixels;
	unsigned int bytes_per_pixel, i, frame_bytes, width_bytes;

	ENTER(Cmiss_texture_get_pixels);
	if (texture)
	{
		if (cmgui_image = Texture_get_image(texture))
		{
			if ((0 <= ((int)depth_start + depth)) &&
				(((int)depth_start + depth) <=
				Cmgui_image_get_number_of_images(cmgui_image)))
			{
				return_code = 1;
				frame_pixels = destination_pixels;
				if (depth != 1)
				{
					/* Lifted from image_utilities.c */
					if(components == 0){
						number_of_components = 
							Cmgui_image_get_number_of_components(cmgui_image);
						components = 
							Cmgui_image_get_number_of_components(cmgui_image);
					}else if(components <= 4){
						number_of_components = components; 
					}else if(components == 5){
						number_of_components = 3;
					}
					bytes_per_pixel = number_of_components *
						Cmgui_image_get_number_of_bytes_per_component(cmgui_image);
					width_bytes = width * bytes_per_pixel;
					if (padded_width_bytes > width_bytes)
					{
						width_bytes = padded_width_bytes;
					}
					frame_bytes = width_bytes * height;
					if (depth > 0)
					{
						for (i = depth_start ; return_code &&
								  (i < depth_start + depth) ; i++)
						{
							return_code = Cmgui_image_dispatch(cmgui_image,
								/*image_number*/i, 
								left, bottom, width, height,
								padded_width_bytes, number_of_fill_bytes, fill_bytes,
								components, frame_pixels);
							frame_pixels += frame_bytes;
						}
					}
					else
					{
						for (i = depth_start ; return_code &&
								  (i > depth_start + depth) ; i--)
						{
							return_code = Cmgui_image_dispatch(cmgui_image,
								/*image_number*/i, 
								left, bottom, width, height,
								padded_width_bytes, number_of_fill_bytes, fill_bytes,
								components, frame_pixels);
							frame_pixels += frame_bytes;
						}
					}
				}
				else
				{
					return_code = Cmgui_image_dispatch(cmgui_image,
						/*image_number*/depth_start, 
						left, bottom, width, height,
						padded_width_bytes, number_of_fill_bytes, fill_bytes,
						components, frame_pixels);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Cmiss_texture_get_pixels:  "
					"Invalid depth parameters.");
				return_code = 0;
			}
			DESTROY(Cmgui_image)(&cmgui_image);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Cmiss_texture_get_pixels:  "
				"Could not get image from texture");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Cmiss_texture_get_pixels:  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;
  
	return (return_code);
} /* Cmiss_texture_get_pixels */

