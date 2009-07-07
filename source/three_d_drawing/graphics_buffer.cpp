/*******************************************************************************
FILE : graphics_buffer.cpp

LAST MODIFIED : 19 February 2007

DESCRIPTION :
This provides a Cmgui interface to the OpenGL contexts of many types.
******************************************************************************/
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

#if defined (BUILD_WITH_CMAKE)
#include "configure/configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

extern "C" {
#if defined (MOTIF_USER_INTERFACE)
#if defined (SGI)
/* Not compiling in as not being actively used and only available on O2's and
   cannot compile against Mesa without function pointer tables. */
/* #include <dmedia/dm_buffer.h> */
#endif /* defined (SGI) */
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include "three_d_drawing/ThreeDDraw.h"
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#include <gtk/gtk.h>
#if ( GTK_MAJOR_VERSION < 2 ) || defined (WIN32_SYSTEM)
#define GTK_USE_GTKGLAREA
#endif /* ( GTK_MAJOR_VERSION < 2 ) || defined (WIN32_SYSTEM)*/
#if defined (GTK_USE_GTKGLAREA)
#include <gtkgl/gtkglarea.h>
#else /* defined (GTK_USE_GTKGLAREA) */
#include <gtk/gtkgl.h>
#include "graphics/graphics_library.h"
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
#if defined (__CYGWIN__) || defined (__MINGW32__)
/* cygwin/mingw win32 header definitions, so we can tell what version
   we have and to get WINVER defines */
#include <w32api.h>
/* Minimum supported version to allow use of AlphaBlend function */
#define WINVER Windows98
#endif /* defined (__CYGWIN__) || defined (__MINGW32__) */
#include <GL/gl.h>
#include <windows.h>
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include <AGL/agl.h>
#endif /* defined (CARBON_USER_INTERFACE) */
}
#if defined (WX_USER_INTERFACE)
extern "C" {
#include "graphics/graphics_library.h"
}
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/debug.h>
#endif /* defined (WX_USER_INTERFACE) */
extern "C" {
#include "general/callback_private.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/object.h"
#include "three_d_drawing/graphics_buffer.h"
#if defined (UNIX) && !defined (DARWIN)
#include "user_interface/event_dispatcher.h"
#endif /* defined (UNIX) && !defined (DARWIN) */
#include "user_interface/message.h"
#include "user_interface/user_interface.h"

#include "three_d_drawing/window_system_extensions.h"

/* #define DEBUG */
#if defined (DEBUG)
#include <stdio.h>
#endif /* defined (DEBUG) */
}

#if !defined (AIX)
/* SAB 30 June 2004
	These calls should be available in every system with GLX 1.3 or greater
	but on the SGI the original code seems to work better with movies and
	with grabbing frames off the screen.  This is done by trying the SGI versions
	first on these machines.
	The SGI allows the creation of the correct Pbuffer using GLX commands
	and then generates a bad alloc error when I try to make it current.
	The code should still run on an older GLX even if it is compiled on a GLX 1.3 by
	falling through to section 4.
	AIX is still having problems with rendering triangles badly and the fbconfig code
	doesn't work well set displayed so it is back off again. */
/*???DB.  The old version of GLX (glx.h 1999/12/11), has GLX_VERSION_1_3
	defined, but doesn't define GLX_PBUFFER_WIDTH, GLX_PBUFFER_HEIGHT and
	GLX_RGBA_BIT */
#if defined (GLX_VERSION_1_3) && defined (GLX_PBUFFER_WIDTH) && \
	defined (GLX_PBUFFER_HEIGHT) && defined (GLX_RGBA_BIT)
#define USE_GLX_PBUFFER 1
#define USE_GLX_FBCONFIG 1
#endif /* defined (GLX_VERSION_1_3) */
#endif /* !defined (AIX) */
/* Despite being in the Mesa headers we do not want to use the SGI FB extensions at all,
   the GLX versions are preferable. */
#undef GLX_SGIX_pbuffer
#undef GLX_SGIX_dmbuffer

/*
Module types
------------
*/

enum Graphics_buffer_class
/*******************************************************************************
LAST MODIFIED : 10 March 2005

DESCRIPTION :
==============================================================================*/
{
	GRAPHICS_BUFFER_ONSCREEN_CLASS, /* A normal graphics buffer */
	GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS, /* Try to create an offscreen buffer with
															 shared display lists */
	GRAPHICS_BUFFER_OFFSCREEN_CLASS  /* Try to create an offscreen buffer,
													 don't worry whether it shares context or not */
};

#if defined (WX_USER_INTERFACE)
class wxGraphicsBuffer;
#endif /* defined (WX_USER_INTERFACE) */

struct Graphics_buffer_package
/*******************************************************************************
LAST MODIFIED : 5 May 2004

DESCRIPTION :
==============================================================================*/
{
	int override_visual_id;

#if defined (MOTIF_USER_INTERFACE)
	GLXContext shared_glx_context;
	Display *display;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#  if defined (GTK_USE_GTKGLAREA)
	  GtkWidget *share_glarea;
#  else /* defined (GTK_USE_GTKGLAREA) */
	  GdkGLContext *share_glcontext;
#  endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
	HGLRC wgl_shared_context;
	User_interface *user_interface;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
	wxGLContext* wxSharedContext;
#endif /* defined (WX_USER_INTERFACE) */
};

FULL_DECLARE_CMISS_CALLBACK_TYPES(Graphics_buffer_callback, \
	struct Graphics_buffer *, void *);

FULL_DECLARE_CMISS_CALLBACK_TYPES(Graphics_buffer_input_callback, \
	struct Graphics_buffer *, struct Graphics_buffer_input *);

struct Graphics_buffer
/*******************************************************************************
LAST MODIFIED : 5 May 2004

DESCRIPTION :
==============================================================================*/
{
	enum Graphics_buffer_type type;
	struct Graphics_buffer_package *package;
	int access_count;

	struct LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback))
		  *initialise_callback_list;
	struct LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback))
		  *resize_callback_list;
	struct LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback))
		  *expose_callback_list;
	struct LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_input_callback))
		  *input_callback_list;

/* For GRAPHICS_BUFFER_GLX_X3D_TYPE */
#if defined (MOTIF_USER_INTERFACE)
	Widget drawing_widget;
	Widget parent;
	Display *display;

#  if defined (GLX_SGIX_dmbuffer)
	   DMbuffer dmbuffer;
	   DMbufferpool dmpool;
#  endif /* defined (GLX_SGIX_dmbuffer) */
	   GLXContext context;
#  if defined (USE_GLX_PBUFFER)
	   GLXPbuffer glx_pbuffer;
#  else /* defined (USE_GLX_PBUFFER) */
#     if defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer)
	      GLXPbufferSGIX glx_pbuffer;
#     endif /* defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer) */
#  endif /* defined (USE_GLX_PBUFFER) */
#  if defined (USE_GLX_FBCONFIG)
	   GLXFBConfig config;
	   GLXFBConfig *config_list;
#  else /* defined (USE_GLX_FBCONFIG) */
#     if defined (GLX_SGIX_fbconfig)
	      GLXFBConfigSGIX config;
	      GLXFBConfigSGIX *config_list;
#     endif /* defined (GLX_SGIX_fbconfig) */
#  endif /* defined (USE_GLX_FBCONFIG) */
	XVisualInfo *visual_info;
	Pixmap pixmap;
	GLXPixmap glx_pixmap;
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)

	/* For GRAPHICS_BUFFER_GTKGLAREA_TYPE and GRAPHICS_BUFFER_GTKGLEXT_TYPE */
	GtkWidget *glarea;

#  if defined (GTK_USE_GTKGLAREA)
	   /* For GRAPHICS_BUFFER_GTKGLAREA_TYPE */
	   /* No inquiry functions so we save the state */
	   enum Graphics_buffer_buffering_mode buffering_mode;
	   enum Graphics_buffer_stereo_mode stereo_mode;
#   else /* defined (GTK_USE_GTKGLAREA) */
	   /* For GRAPHICS_BUFFER_GTKGLEXT_TYPE */
	   GdkGLConfig *glconfig;
	   GdkGLContext *glcontext;
	   GdkGLDrawable *gldrawable;

	gulong initialise_handler_id;
	gulong resize_handler_id;
	gulong expose_handler_id;
	gulong button_press_handler_id;
	gulong button_release_handler_id;
	gulong key_press_handler_id;
	gulong key_release_handler_id;
	gulong motion_handler_id;

#   endif /* ! defined (GTK_USER_GLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	int pixel_format;
	/* x, y, width and height are used with the windowless mode (no hWnd).
		x and y locate the current hDC with respect to the top left corner of the plugin.
		width and height are the total size of the plugin port.  */
	int width;
	int height;
	int x;
	int y;
	/* For offscreen rendering */
	int offscreen_width;
	int offscreen_height;
#ifdef WGL_ARB_pbuffer
	HPBUFFERARB pbuffer;
#endif /* defined (WGL_ARB_pbuffer) */
	/* We need the creation parameters to support delayed creation and recreation
		as the pbuffer changes size.  Double buffering is irrelevant as we are drawing
		offscreen and copying so we get it anyway and stereo is not possible. */
	int minimum_colour_buffer_depth;
	int minimum_depth_buffer_depth;
	int minimum_accumulation_buffer_depth;
	/* Windows bitmap, either used with non accelerated windows OpenGL or
		for copying from pbuffer for rendering to screen */
	HBITMAP device_independent_bitmap;
	HDC device_independent_bitmap_hdc;
	void *device_independent_bitmap_pixels;
	/* Hidden window used to get connection to independent driver OpenGL */
	HWND hidden_accelerated_window;
	Graphics_buffer *hidden_graphics_buffer;
	/* So we know how to composite we need to keep the buffering mode */
	enum Graphics_buffer_buffering_mode buffering_mode;
        /* Some calls to the scene viewer mean that we will need to rerender
           the offscreen window (such as resizing) so this flag tells us this. */
        int offscreen_render_required;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
	CGrafPtr port;
	/* These parameters are provided by the hosting application and should be
		respected by this graphics_buffer */
	int width;
	int height;
	int portx;
	int porty;
	int clip_width;
	int clip_height;
	AGLPixelFormat aglPixelFormat;
	AGLContext aglContext;
	EventHandlerRef expose_handler_ref;
	EventHandlerRef mouse_handler_ref;
	EventHandlerRef resize_handler_ref;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
	wxPanel *parent;
	wxGraphicsBuffer *canvas;
	int *attrib_list, framebuffer_width, framebuffer_height;
#if defined (OPENGL_API)
	GLuint fbo, depthbuffer, img;
#endif
#endif /* defined (WX_USER_INTERFACE) */
};

/*
Module functions
----------------
*/

#if defined (WIN32_USER_INTERFACE)
static LRESULT CALLBACK Graphics_buffer_callback_proc(HWND window, UINT message_identifier,
	WPARAM first_message, LPARAM second_message);
#endif /* defined (WIN32_USER_INTERFACE) */

DEFINE_CMISS_CALLBACK_MODULE_FUNCTIONS(Graphics_buffer_callback, void)

DEFINE_CMISS_CALLBACK_FUNCTIONS(Graphics_buffer_callback, \
	struct Graphics_buffer *,void *)

DEFINE_CMISS_CALLBACK_MODULE_FUNCTIONS(Graphics_buffer_input_callback, void)

DEFINE_CMISS_CALLBACK_FUNCTIONS(Graphics_buffer_input_callback, \
	struct Graphics_buffer *, struct Graphics_buffer_input *)

DECLARE_OBJECT_FUNCTIONS(Graphics_buffer)

static struct Graphics_buffer *CREATE(Graphics_buffer)(
	struct Graphics_buffer_package *package)
/*******************************************************************************
LAST MODIFIED : 4 May 2004

DESCRIPTION :
This is static as it is designed to be called by the different constructors
contained in the this module only.
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(CREATE(Graphics_buffer));

	if (ALLOCATE(buffer, struct Graphics_buffer, 1))
	{
		buffer->type = GRAPHICS_BUFFER_INVALID_TYPE;
		buffer->package = package;
		buffer->access_count = 0;

		buffer->initialise_callback_list=
			CREATE(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))();
		buffer->resize_callback_list=
			CREATE(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))();
		buffer->expose_callback_list=
			CREATE(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))();
		buffer->input_callback_list=
			CREATE(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_input_callback)))();

/* For GRAPHICS_BUFFER_GLX_X3D_TYPE */
#if defined (MOTIF_USER_INTERFACE)
		buffer->drawing_widget = (Widget)NULL;
		buffer->parent = (Widget)NULL;
		buffer->display = (Display *)NULL;

#  if defined (GLX_SGIX_dmbuffer)
	   buffer->dmbuffer = (DMbuffer)NULL;
	   buffer->dmpool = (DMbufferpool)NULL;
#  endif /* defined (GLX_SGIX_dmbuffer) */
	   buffer->context = (GLXContext)NULL;
#  if defined (USE_GLX_PBUFFER)
	   buffer->glx_pbuffer = (GLXPbuffer)NULL;
#  else /* defined (USE_GLX_PBUFFER) */
#     if defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer)
		buffer->glx_pbuffer = (GLXPbufferSGIX)NULL;
#     endif /* defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer) */
#  endif /* defined (USE_GLX_PBUFFER) */
#  if defined (USE_GLX_FBCONFIG)
	   buffer->config = (GLXFBConfig)NULL;
	   buffer->config_list = (GLXFBConfig *)NULL;
#  else /* defined (USE_GLX_FBCONFIG) */
#     if defined (GLX_SGIX_fbconfig)
		buffer->config = (GLXFBConfigSGIX)NULL;
		buffer->config_list = (GLXFBConfigSGIX *)NULL;
#     endif /* defined (GLX_SGIX_fbconfig) */
#  endif /* defined (USE_GLX_FBCONFIG) */
	   buffer->visual_info = (XVisualInfo *)NULL;
	   buffer->pixmap = (Pixmap)NULL;
	   buffer->glx_pixmap = (GLXPixmap)NULL;
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)

/* For GRAPHICS_BUFFER_GTKGLAREA_TYPE and GRAPHICS_BUFFER_GTKGLEXT_TYPE */
	buffer->glarea = (GtkWidget *)NULL;

#   if defined (GTK_USE_GTKGLAREA)
/* For GRAPHICS_BUFFER_GTKGLAREA_TYPE */
	    /* No inquiry functions so we save the state */
	    buffer->buffering_mode = GRAPHICS_BUFFER_ANY_BUFFERING_MODE;
		 buffer->stereo_mode = GRAPHICS_BUFFER_ANY_STEREO_MODE;
#   else /* defined (GTK_USE_GTKGLAREA) */
/* For GRAPHICS_BUFFER_GTKGLEXT_TYPE */
	    buffer->glconfig = (GdkGLConfig *)NULL;
	    buffer->glcontext = (GdkGLContext *)NULL;
	    buffer->gldrawable = (GdkGLDrawable *)NULL;
#   endif /* ! defined (GTK_USER_GLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
		 buffer->hWnd = (HWND)NULL;
		 buffer->hDC = (HDC)NULL;
		 buffer->hRC = (HGLRC)NULL;
		 buffer->pixel_format = 0;
		 buffer->width = 0;
		 buffer->height = 0;
		 buffer->x = 0;
		 buffer->y = 0;
		 buffer->offscreen_width = 0;
		 buffer->offscreen_height = 0;
#ifdef WGL_ARB_pbuffer
		 buffer->pbuffer = (HPBUFFERARB)NULL;
#endif /* defined (WGL_ARB_pbuffer) */
	    buffer->buffering_mode = GRAPHICS_BUFFER_ANY_BUFFERING_MODE;
		 buffer->minimum_colour_buffer_depth = 0;
		 buffer->minimum_depth_buffer_depth = 0;
		 buffer->minimum_accumulation_buffer_depth = 0;
		 buffer->device_independent_bitmap = (HBITMAP)NULL;
		 buffer->device_independent_bitmap_hdc = (HDC)NULL;
		 buffer->device_independent_bitmap_pixels = NULL;
		 buffer->hidden_accelerated_window = (HWND)NULL;
		 buffer->hidden_graphics_buffer = (Graphics_buffer *)NULL;
		 buffer->offscreen_render_required = 0;
#endif // defined (WIN32_USER_INTERFACE)

#if defined (CARBON_USER_INTERFACE)
		 buffer->port = 0;
		 buffer->width = 0;
		 buffer->height = 0;
		 buffer->portx = 0;
		 buffer->porty = 0;
		 buffer->clip_width = 0;
		 buffer->clip_height = 0;
		 buffer->aglPixelFormat = (AGLPixelFormat)NULL;
		 buffer->aglContext = (AGLContext)NULL;
		 buffer->expose_handler_ref = (EventHandlerRef)NULL;
		 buffer->mouse_handler_ref = (EventHandlerRef)NULL;
		 buffer->resize_handler_ref = (EventHandlerRef)NULL;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
		 buffer->parent = (wxPanel *)NULL;
		 buffer->canvas = (wxGraphicsBuffer *)NULL;
		 buffer->attrib_list = NULL;
#if defined (OPENGL_API)
		 buffer->fbo = 0;
		 buffer->depthbuffer = 0;
		 buffer->img = 0;
#endif
		 buffer->framebuffer_height = 0;
		 buffer->framebuffer_width = 0;
#endif /* defined (CARBON_USER_INTERFACE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Graphics_buffer). Unable to allocate buffer structure");
		buffer = (struct Graphics_buffer *)NULL;
	}

	LEAVE;
	return (buffer);
} /* CREATE(Graphics_buffer) */

#if defined (MOTIF_USER_INTERFACE)
static void Graphics_buffer_X3d_initialize_callback(Widget graphics_buffer_widget,
	XtPointer graphics_buffer_structure, XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
This is the configuration callback for the GL widget.
Sets the initial viewing transform for the graphics_buffer.
???RC Needed at all?
???RC Move functionality elsewhere?
???RC Need MakeCurrent?
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	ENTER(graphics_buffer_X3d_initialize_callback);
	USE_PARAMETER(call_data);
	USE_PARAMETER(graphics_buffer_widget);
	if (graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)
	{
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->initialise_callback_list, graphics_buffer, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_X3d_initialize_callback.  Missing graphics_buffer");
	}
	LEAVE;
} /* Graphics_buffer_X3d_initialize_callback */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static void Graphics_buffer_X3d_resize_callback(Widget graphics_buffer_widget,
	XtPointer graphics_buffer_structure,XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Called when part of the graphics_buffer window is resized. All it does is notify
callbacks interested in the graphics_buffers transformations.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;
	X3dThreeDDrawCallbackStruct *resize_callback_data;

	ENTER(graphics_buffer_X3d_resize_callback);
	if (graphics_buffer_widget&&
		(graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)&&
		(resize_callback_data=(X3dThreeDDrawCallbackStruct *)call_data)&&
		(X3dCR_RESIZE==resize_callback_data->reason))
	{
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->resize_callback_list, graphics_buffer, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_X3d_resize_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* graphics_buffer_X3d_resize_callback */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static void Graphics_buffer_X3d_expose_callback(Widget graphics_buffer_widget,
	XtPointer graphics_buffer_structure,XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Called when part of the graphics_buffer window is exposed. Does not attempt to
redraw just the exposed area. Instead, it redraws the whole picture, but only
if there are no more expose events pending.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;
	XExposeEvent *expose_event;
	X3dThreeDDrawCallbackStruct *expose_callback_data;

	ENTER(graphics_buffer_X3d_expose_callback);
	if (graphics_buffer_widget&&
		(graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)&&
		(expose_callback_data=(X3dThreeDDrawCallbackStruct *)call_data)&&
		(X3dCR_EXPOSE==expose_callback_data->reason)&&
		(expose_event=(XExposeEvent *)(expose_callback_data->event)))
	{
		/* if no more expose events in series */
		if (0==expose_event->count)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
				graphics_buffer->expose_callback_list, graphics_buffer, NULL);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_X3d_expose_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* graphics_buffer_X3d_expose_callback */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static void Graphics_buffer_X3d_input_callback(Widget graphics_buffer_widget,
	XtPointer graphics_buffer_structure, XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
The callback for mouse or keyboard input in the graphics_buffer window. The
resulting behaviour depends on the <graphics_buffer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/
{
	int input_modifier, return_code;
	struct Graphics_buffer *graphics_buffer;
	struct Graphics_buffer_input input;
	XButtonEvent *button_event;
	XEvent *event;
	XKeyEvent *key_event;
	XMotionEvent *motion_event;
	X3dThreeDDrawCallbackStruct *input_callback_data;

	ENTER(Graphics_buffer_input_callback);
	USE_PARAMETER(graphics_buffer_widget);
	if ((graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)&&
		(input_callback_data=(X3dThreeDDrawCallbackStruct *)call_data)&&
		(X3dCR_INPUT==input_callback_data->reason)&&
		(event=(XEvent *)(input_callback_data->event)))
	{
		return_code = 1;
		input.type = GRAPHICS_BUFFER_INVALID_INPUT;
		input.button_number = 0;
		input.key_code = 0;
		input.position_x = 0;
		input.position_y = 0;
		input_modifier = 0;
		switch(event->type)
		{
			case MotionNotify:
			{
				input.type = GRAPHICS_BUFFER_MOTION_NOTIFY;
				motion_event = &(event->xmotion);
				input.position_x = motion_event->x;
				input.position_y = motion_event->y;
				if (ShiftMask&(motion_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
				}
				if (ControlMask&(motion_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
				}
				if (Mod1Mask&(motion_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
				}
				if (Button1Mask&(motion_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
				}
			} break;
			case ButtonPress:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
				button_event = &(event->xbutton);
				input.button_number = button_event->button;
				input.position_x = button_event->x;
				input.position_y = button_event->y;
				input_modifier = (enum Graphics_buffer_input_modifier)0;
				if (ShiftMask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
				}
				if (ControlMask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
				}
				if (Mod1Mask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
				}
				if (Button1Mask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
				}
			} break;
			case ButtonRelease:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
				button_event = &(event->xbutton);
				input.button_number = button_event->button;
				input.position_x = button_event->x;
				input.position_y = button_event->y;
				if (ShiftMask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
				}
				if (ControlMask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
				}
				if (Mod1Mask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
				}
				if (Button1Mask&(button_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
				}
			} break;
			case KeyPress:
			{
				input.type = GRAPHICS_BUFFER_KEY_PRESS;
				key_event= &(event->xkey);
				input.key_code = key_event->keycode;
				if (ShiftMask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
				}
				if (ControlMask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
				}
				if (Mod1Mask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
				}
				if (Button1Mask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
				}
			} break;
			case KeyRelease:
			{
				input.type = GRAPHICS_BUFFER_KEY_RELEASE;
				key_event= &(event->xkey);
				input.key_code = key_event->keycode;
				if (ShiftMask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
				}
				if (ControlMask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
				}
				if (Mod1Mask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
				}
				if (Button1Mask&(key_event->state))
				{
					input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Graphics_buffer_input_callback.  Unknown X event");
				return_code=0;
				/* This event type is not being passed on */
			} break;
		}
		input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
			(input_modifier);

		if (return_code)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				graphics_buffer->input_callback_list, graphics_buffer, &input);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_input_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* Graphics_buffer_input_callback */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
#  if defined USE_GLX_FBCONFIG
static int Graphics_buffer_create_from_fb_config(struct Graphics_buffer *buffer,
	struct Graphics_buffer_package *graphics_buffer_package,
	enum Graphics_buffer_class buffer_class,  Widget x3d_parent_widget,
	int width, int height, GLXFBConfig fb_config)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	Display *display;
	Pixmap pixmap;
	GLXPixmap glx_pixmap;
	XVisualInfo *visual_info;
	Widget drawing_widget;
#if defined (USE_GLX_PBUFFER)
	GLXPbuffer pbuffer;
	int pbuffer_attribs [] =
	{
		GLX_PBUFFER_WIDTH, 0, /* Note that these 0 values are explictly overwritten below */
		GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True,
		(int) None
	};
#endif /* defined (USE_GLX_PBUFFER) */
	int pixmap_attribs [] =
	{
		/* There are currently no valid attributes */
		(int) None
	};

	ENTER(Graphics_buffer_X3d_expose_callback);

	return_code = 0;
	display = graphics_buffer_package->display;
#if defined (DEBUG)
	{
		int value;
		printf("Graphics_buffer_create_from_fb_config\n");
 		glXGetFBConfigAttrib(display, fb_config, GLX_FBCONFIG_ID, &value);
		printf("   fbconfig id : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_DOUBLEBUFFER, &value);
		printf("   doublebuffer : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_RED_SIZE, &value);
		printf("   red : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_GREEN_SIZE, &value);
		printf("   green : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_BLUE_SIZE, &value);
		printf("   blue : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_ALPHA_SIZE, &value);
		printf("   alpha : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_DEPTH_SIZE, &value);
		printf("   depth : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_ACCUM_RED_SIZE, &value);
		printf("   accumulation red : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_ACCUM_GREEN_SIZE, &value);
		printf("   accumulation green : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_ACCUM_BLUE_SIZE, &value);
		printf("   accumulation blue : %d\n", value);
 		glXGetFBConfigAttrib(display, fb_config, GLX_ACCUM_ALPHA_SIZE, &value);
		printf("   accumulation alpha : %d\n", value);
	}
#endif /* defined (DEBUG) */

	switch (buffer_class)
	{
		case GRAPHICS_BUFFER_ONSCREEN_CLASS:
		{
			visual_info = glXGetVisualFromFBConfig(display, fb_config);
			if (drawing_widget=XtVaCreateWidget("cmiss_graphics_buffer_area",
					threeDDrawingWidgetClass, x3d_parent_widget,
					XtNdepth, visual_info->depth,
					XtNvisual, visual_info->visual,
					XmNwidth, width,
					XmNheight, height,
					XmNleftAttachment,XmATTACH_FORM,
					XmNrightAttachment,XmATTACH_FORM,
					XmNbottomAttachment,XmATTACH_FORM,
					XmNtopAttachment,XmATTACH_FORM,
					XmNborderWidth,0,
					XmNleftOffset,0,
					XmNrightOffset,0,
					XmNbottomOffset,0,
					XmNtopOffset,0,
					NULL))
			{
				if (X3dThreeDisInitialised(drawing_widget))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_X3D_TYPE;
					buffer->parent = x3d_parent_widget;
					buffer->drawing_widget = drawing_widget;
					buffer->display = display;
					buffer->config = fb_config;
					buffer->visual_info = glXGetVisualFromFBConfig(display, fb_config);
					buffer->context = glXCreateNewContext(
						display, buffer->config, GLX_RGBA_TYPE,
						graphics_buffer_package->shared_glx_context, GL_TRUE);
					XtAddCallback(buffer->drawing_widget ,X3dNinitializeCallback,
						Graphics_buffer_X3d_initialize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNresizeCallback,
						Graphics_buffer_X3d_resize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNexposeCallback,
						Graphics_buffer_X3d_expose_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNinputCallback,
						Graphics_buffer_X3d_input_callback, buffer);
					return_code = 1;
				}
				else
				{
					XtDestroyWidget(buffer->drawing_widget);
				}
			}
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS:
		{
			/* On an SGI it is invalid to use the function parameters in
				the structure initialisation */
			pbuffer_attribs[1] = width;
			pbuffer_attribs[3] = height;
			if (pbuffer = glXCreatePbuffer(display,
					fb_config, pbuffer_attribs))
			{
				buffer->type = GRAPHICS_BUFFER_GLX_PBUFFER_TYPE;
				buffer->display = display;
				buffer->config = fb_config;
				buffer->glx_pbuffer = pbuffer;
				buffer->visual_info =
					glXGetVisualFromFBConfig(display, buffer->config);
				buffer->context = glXCreateNewContext(
					display, buffer->config, GLX_RGBA_TYPE,
					graphics_buffer_package->shared_glx_context, GL_TRUE);
				return_code = 1;
			}
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_CLASS:
		{
			visual_info = glXGetVisualFromFBConfig(display, buffer->config);
			/* ???? Should be freed */
			if (pixmap = XCreatePixmap(display,
					DefaultRootWindow(display), width, height,
					visual_info->depth))
			{
				if (glx_pixmap = glXCreatePixmap(display,
						fb_config, pixmap, pixmap_attribs))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_PIXMAP_TYPE;
					buffer->display = display;
					buffer->config = fb_config;
					buffer->pixmap = pixmap;
					buffer->glx_pixmap = glx_pixmap;
					buffer->visual_info = visual_info;
					/* pixmaps do not share contexts */
					buffer->context = glXCreateNewContext(
						display, buffer->config, GLX_RGBA_TYPE,
						NULL, GL_TRUE);
					return_code = 1;
				}
			}
		} break;
	}
	switch (buffer->type)
	{
		case GRAPHICS_BUFFER_GLX_X3D_TYPE:
		case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
		{
			if (!graphics_buffer_package->shared_glx_context &&
				buffer->context)
			{
				graphics_buffer_package->shared_glx_context = buffer->context;
			}
		} break;
	}
#if defined (DEBUG)
	printf("Graphics_buffer_create_from_fb_config\n");
	printf("   buffer_class : %d\n", buffer_class);
	printf("   buffer->type : %d\n", buffer->type);
	printf("   buffer->config : %p\n", buffer->config);
	printf("   buffer->visual_info : %p\n", buffer->visual_info);
	printf("   buffer->context : %p\n\n", buffer->context);
	{
		int value;
 		glXQueryContext(display, buffer->context, GLX_FBCONFIG_ID, &value);
		printf("   fbconfig id : %d\n", value);
	}
#endif /* defined (DEBUG) */

	LEAVE;

	return (return_code);
} /* Graphics_buffer_create_from_fb_config */
#  endif /* defined USE_GLX_FBCONFIG */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
#  if defined (GLX_SGIX_pbuffer)
static int Graphics_buffer_create_from_fb_config_sgi(struct Graphics_buffer *buffer,
	struct Graphics_buffer_package *graphics_buffer_package,
	enum Graphics_buffer_class buffer_class,  Widget x3d_parent_widget,
	int width, int height, GLXFBConfig fb_config)
/*******************************************************************************
LAST MODIFIED : 28 May 2004

DESCRIPTION :
Unfortunately the SGIX functions still seem to work better on the SGI than
the equivalent GLX1.3 versions.
==============================================================================*/
{
	int return_code;
	Display *display;
	Pixmap pixmap;
	GLXPixmap glx_pixmap;
	XVisualInfo *visual_info;
	Widget drawing_widget;
	GLXPbuffer pbuffer;
	int pbuffer_attribs [] =
	{
		GLX_PRESERVED_CONTENTS, True,
		(int) None
	};
	int pixmap_attribs [] =
	{
		/* There are currently no valid attributes */
		(int) None
	};

	ENTER(Graphics_buffer_X3d_expose_callback);

	return_code = 0;
	display = graphics_buffer_package->display;
	switch (buffer_class)
	{
		case GRAPHICS_BUFFER_ONSCREEN_CLASS:
		{
			visual_info =
					glXGetVisualFromFBConfigSGIX(display, fb_config);
			if (drawing_widget=XtVaCreateWidget("cmiss_graphics_buffer_area",
					threeDDrawingWidgetClass, x3d_parent_widget,
					XtNdepth, visual_info->depth,
					XtNvisual, visual_info->visual,
					XmNwidth, width,
					XmNheight, height,
					XmNleftAttachment,XmATTACH_FORM,
					XmNrightAttachment,XmATTACH_FORM,
					XmNbottomAttachment,XmATTACH_FORM,
					XmNtopAttachment,XmATTACH_FORM,
					XmNborderWidth,0,
					XmNleftOffset,0,
					XmNrightOffset,0,
					XmNbottomOffset,0,
					XmNtopOffset,0,
					NULL))
			{
				if (X3dThreeDisInitialised(drawing_widget))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_X3D_TYPE;
					buffer->parent = x3d_parent_widget;
					buffer->drawing_widget = drawing_widget;
					buffer->display = display;
					buffer->config = fb_config;
					buffer->visual_info =
						glXGetVisualFromFBConfigSGIX(display, buffer->config);
					buffer->context = glXCreateContextWithConfigSGIX(
						display, buffer->config, GLX_RGBA_TYPE_SGIX,
						graphics_buffer_package->shared_glx_context, GL_TRUE);
					XtAddCallback(buffer->drawing_widget ,X3dNinitializeCallback,
						Graphics_buffer_X3d_initialize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNresizeCallback,
						Graphics_buffer_X3d_resize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNexposeCallback,
						Graphics_buffer_X3d_expose_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNinputCallback,
						Graphics_buffer_X3d_input_callback, buffer);
					return_code = 1;
				}
				else
				{
					XtDestroyWidget(buffer->drawing_widget);
				}
			}
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS:
		{
			if (pbuffer = glXCreateGLXPbufferSGIX(display,
					fb_config, width, height, pbuffer_attribs))
			{
				buffer->type = GRAPHICS_BUFFER_GLX_PBUFFER_TYPE;
				buffer->display = display;
				buffer->config = fb_config;
				buffer->glx_pbuffer = pbuffer;
				buffer->visual_info =
					glXGetVisualFromFBConfigSGIX(display, buffer->config);
				buffer->context = glXCreateContextWithConfigSGIX(
					display, buffer->config, GLX_RGBA_TYPE_SGIX,
					graphics_buffer_package->shared_glx_context, GL_TRUE);
				return_code = 1;
			}
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_CLASS:
		{
			visual_info =
					glXGetVisualFromFBConfigSGIX(display, fb_config);
			/* ???? Should be freed */
			if (pixmap = XCreatePixmap(display,
					DefaultRootWindow(display), width, height,
					visual_info->depth))
			{
				if (glx_pixmap = glXCreatePixmap(display,
						fb_config, pixmap, pixmap_attribs))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_PIXMAP_TYPE;
					buffer->display = display;
					buffer->config = fb_config;
					buffer->pixmap = pixmap;
					buffer->glx_pixmap = glx_pixmap;
					buffer->visual_info = visual_info;
					/* pixmaps do not share contexts */
					buffer->context = glXCreateContextWithConfigSGIX(
						display, buffer->config, GLX_RGBA_TYPE,
						NULL, GL_TRUE);
					return_code = 1;
				}
			}
		} break;
	}
	switch (buffer->type)
	{
		case GRAPHICS_BUFFER_GLX_X3D_TYPE:
		case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
		{
			if (!graphics_buffer_package->shared_glx_context &&
				buffer->context)
			{
				graphics_buffer_package->shared_glx_context = buffer->context;
			}
		} break;
	}
#if defined (DEBUG)
	printf("Graphics_buffer_create_from_fb_config_sgi\n");
	printf("   buffer_class : %d\n", buffer_class);
	printf("   buffer->type : %d\n", buffer->type);
	printf("   buffer->config : %p\n", buffer->config);
	printf("   buffer->visual_info : %p\n", buffer->visual_info);
	printf("   buffer->context : %p\n\n", buffer->context);
#endif /* defined (DEBUG) */

	LEAVE;

	return (return_code);
} /* Graphics_buffer_create_from_fb_config_sgi */
#  endif /* defined (GLX_SGIX_pbuffer) */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
static int Graphics_buffer_create_from_visual_info(struct Graphics_buffer *buffer,
	struct Graphics_buffer_package *graphics_buffer_package,
	enum Graphics_buffer_class buffer_class,  Widget x3d_parent_widget,
	int width, int height, XVisualInfo *visual_info)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	Display *display;
	int return_code;
	Pixmap pixmap;
	GLXPixmap glx_pixmap;
	Widget drawing_widget;

	ENTER(Graphics_buffer_X3d_expose_callback);

	return_code = 0;
	display = graphics_buffer_package->display;
	switch (buffer_class)
	{
		case GRAPHICS_BUFFER_ONSCREEN_CLASS:
		{
			if (drawing_widget=XtVaCreateWidget("cmiss_graphics_buffer_area",
					threeDDrawingWidgetClass, x3d_parent_widget,
					XtNdepth, visual_info->depth,
					XtNvisual, visual_info->visual,
					XmNwidth, width,
					XmNheight, height,
					XmNleftAttachment,XmATTACH_FORM,
					XmNrightAttachment,XmATTACH_FORM,
					XmNbottomAttachment,XmATTACH_FORM,
					XmNtopAttachment,XmATTACH_FORM,
					XmNborderWidth,0,
					XmNleftOffset,0,
					XmNrightOffset,0,
					XmNbottomOffset,0,
					XmNtopOffset,0,
					NULL))
			{
				if (X3dThreeDisInitialised(drawing_widget))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_X3D_TYPE;
					buffer->display = display;
					buffer->drawing_widget = drawing_widget;
					buffer->parent = x3d_parent_widget;
					buffer->visual_info = visual_info;
					buffer->context = glXCreateContext(
						display, buffer->visual_info,
						graphics_buffer_package->shared_glx_context,
						GL_TRUE);
					XtAddCallback(buffer->drawing_widget ,X3dNinitializeCallback,
						Graphics_buffer_X3d_initialize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNresizeCallback,
						Graphics_buffer_X3d_resize_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNexposeCallback,
						Graphics_buffer_X3d_expose_callback, buffer);
					XtAddCallback(buffer->drawing_widget, X3dNinputCallback,
						Graphics_buffer_X3d_input_callback, buffer);
					return_code = 1;
				}
				else
				{
					XtDestroyWidget(drawing_widget);
				}
			}
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS:
		{
		} break;
		case GRAPHICS_BUFFER_OFFSCREEN_CLASS:
		{
			if (pixmap = XCreatePixmap(display,
					DefaultRootWindow(display), width, height,
					visual_info->depth))
			{
				if (glx_pixmap = glXCreateGLXPixmap(display,
						visual_info, pixmap))
				{
					buffer->type = GRAPHICS_BUFFER_GLX_PIXMAP_TYPE;
					/* This is non shared */
					buffer->display = display;
					buffer->pixmap = pixmap;
					buffer->glx_pixmap = glx_pixmap;
					buffer->visual_info = visual_info;
					buffer->context = glXCreateContext(
						display, buffer->visual_info,
						/*shared_context*/(GLXContext)NULL,
#if defined (AIX)
						/* While potentially any implementation may choke on a direct
							rendering context only the AIX fails so far */
						GL_FALSE
#else /* defined (AIX) */
						GL_TRUE
#endif /* defined (AIX) */
						);
					return_code = 1;
				}
			}
		} break;
	}
	switch (buffer->type)
	{
		case GRAPHICS_BUFFER_GLX_X3D_TYPE:
		case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
		{
			if (!graphics_buffer_package->shared_glx_context &&
				buffer->context)
			{
				graphics_buffer_package->shared_glx_context = buffer->context;
			}
		} break;
	}
#if defined (DEBUG)
	printf("Graphics_buffer_create_from_visual_info\n");
	printf("   buffer_class : %d\n", buffer_class);
	printf("   buffer->type : %d\n", buffer->type);
	printf("   buffer->visual_info : %p\n", buffer->visual_info);
	printf("   buffer->context : %p\n\n", buffer->context);
#endif /* defined (DEBUG) */

	LEAVE;

	return (return_code);
} /* Graphics_buffer_create_from_visual_info */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (WX_USER_INTERFACE)
class wxGraphicsBuffer : public wxGLCanvas
{
	Graphics_buffer *graphics_buffer;
	wxPanel *parent;

public:

	wxGraphicsBuffer(wxPanel *parent, wxGLContext* sharedContext,
		Graphics_buffer *graphics_buffer
		 , int *attrib_list):
	wxGLCanvas(parent, sharedContext, wxID_ANY, wxDefaultPosition, wxSize(10, 10),
		 wxFULL_REPAINT_ON_RESIZE, "GLCanvas"
		 , attrib_list),
		graphics_buffer(graphics_buffer), parent(parent)
	{
	};

	~wxGraphicsBuffer()
	{
		if (graphics_buffer)
		{
			graphics_buffer->canvas = (wxGraphicsBuffer *)NULL;
			if ((GetContext() == graphics_buffer->package->wxSharedContext))
			{
				graphics_buffer->package->wxSharedContext = (wxGLContext *)NULL;
			}
		}
	};

	void ClearGraphicsBufferReference()
	{
		graphics_buffer = (Graphics_buffer *)NULL;
	}

	void OnPaint( wxPaintEvent& WXUNUSED(event) )
	{
		/* Unfortunately can't find a better place to copy the shareable context */
		if (!graphics_buffer->package->wxSharedContext)
		{
			graphics_buffer->package->wxSharedContext = GetContext();
		}

		/* must always be here */
		wxPaintDC dc(this);

		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->expose_callback_list, graphics_buffer, NULL);
	}

	void OnSize(wxSizeEvent& event)
	{
		// this is also necessary to update the context on some platforms
		wxGLCanvas::OnSize(event);

		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->resize_callback_list, graphics_buffer, NULL);
	}

	void OnEraseBackground(wxEraseEvent& WXUNUSED(event))
	{
		/* Do nothing, to avoid flashing on MSW */
	}

	void OnMouse( wxMouseEvent& event )
	{
		int input_modifier, return_code;
		struct Graphics_buffer_input input;

		return_code = 1;

		input.type = GRAPHICS_BUFFER_INVALID_INPUT;
		input.button_number = 0;
		input.key_code = 0;
		input.position_x = event.GetX();
		input.position_y = event.GetY();
		input_modifier = 0;

		if (event.ShiftDown())
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
		}
		if (event.ControlDown())
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
		}
		if (event.AltDown())
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
		}

		if (event.Dragging())
		{
			input.type = GRAPHICS_BUFFER_MOTION_NOTIFY;
			if (event.LeftIsDown())
			{
				input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
			}
		}
		else if (event.ButtonDown())
		{
			input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
			switch (event.GetButton())
			{
				case wxMOUSE_BTN_LEFT:
				{
					input.button_number = 1;
				} break;
				case wxMOUSE_BTN_MIDDLE:
				{
					input.button_number = 2;
				} break;
				case wxMOUSE_BTN_RIGHT:
				{
					input.button_number = 3;
				} break;
				case wxMOUSE_BTN_NONE:
				default:
				{
					display_message(ERROR_MESSAGE,
						"wxGraphicsBuffer_input_callback::OnMouse.  Invalid button");
					return_code=0;
				} break;
			}
		}
		else if (event.ButtonUp())
		{
			input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
			switch (event.GetButton())
			{
				case wxMOUSE_BTN_LEFT:
				{
					input.button_number = 1;
				} break;
				case wxMOUSE_BTN_MIDDLE:
				{
					input.button_number = 2;
				} break;
				case wxMOUSE_BTN_RIGHT:
				{
					input.button_number = 3;
				} break;
				case wxMOUSE_BTN_NONE:
				default:
				{
					display_message(ERROR_MESSAGE,
						"wxGraphicsBuffer_input_callback::OnMouse.  Invalid button");
					return_code=0;
				} break;
			}
		}
		else
		{
		  /* Ignore other events */
		  return_code=0;
		}

		input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
			(input_modifier);

		if (return_code)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				graphics_buffer->input_callback_list, graphics_buffer, &input);
		}
	}

   DECLARE_EVENT_TABLE();
};



BEGIN_EVENT_TABLE(wxGraphicsBuffer, wxGLCanvas)
    EVT_SIZE(wxGraphicsBuffer::OnSize)
    EVT_PAINT(wxGraphicsBuffer::OnPaint)
    EVT_ERASE_BACKGROUND(wxGraphicsBuffer::OnEraseBackground)
    EVT_MOUSE_EVENTS(wxGraphicsBuffer::OnMouse)
END_EVENT_TABLE()

class wxTestingBuffer : public wxGLCanvas
{
	 wxPanel *parent;
	 Graphics_buffer *graphics_buffer;
	 wxGLContext *sharedContext;

public:
	 wxTestingBuffer(wxPanel *parent, Graphics_buffer *graphics_buffer, wxGLContext* sharedContext, int *attrib_array):
			wxGLCanvas(parent, sharedContext, wxID_ANY, wxDefaultPosition, wxSize(10, 10),
				 wxFULL_REPAINT_ON_RESIZE, "GLCanvas", attrib_array),
			parent(parent), graphics_buffer(graphics_buffer), sharedContext(sharedContext)
	 {
	 };

	~wxTestingBuffer()
	{
	};

	void Set_wx_SharedContext()
	{
		if (!sharedContext)
		{
			graphics_buffer->package->wxSharedContext = GetContext();
		}
	}
};

static void Graphics_buffer_create_buffer_wx(
	struct Graphics_buffer *buffer,
	struct Graphics_buffer_package *graphics_buffer_package,
	wxPanel *parent,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth,
	int width, int height, struct Graphics_buffer  *buffer_to_match)
/*******************************************************************************
LAST MODIFIED : 16 October  2007

DESCRIPTION :
==============================================================================*/
{
	 int *visual_attributes;
	 int return_code;
	ENTER(Graphics_buffer_create_buffer_wx);
	wxLogNull logNo;
	if (buffer)
	{
		 buffer->parent = parent;
		 return_code = 0;
		 if (buffer->type == GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE)
		 {
#if defined (OPENGL_API) && defined (GL_EXT_framebuffer_object)
			 if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
			 {
				 GLint buffer_size;
				 glGenFramebuffersEXT (1,&buffer->fbo);
				 glGenRenderbuffersEXT(1, &buffer->depthbuffer);
				 glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &buffer_size);
				 if (height > buffer_size)
				 {
					 display_message(WARNING_MESSAGE,"Graphics_buffer_create_buffer_wx.  "
						 "Request height is larger than allowed, set height to maximum possible"
						 "height.");
					 buffer->framebuffer_height = buffer_size;
				 }
				 else
				 {
					 buffer->framebuffer_height = height;
				 }
				 if (width > buffer_size)
				 {
					 display_message(WARNING_MESSAGE,"Graphics_buffer_create_buffer_wx.  "
						 "Request width is larger than allowed, set width to maximum possible"
						 "width.");
					 buffer->framebuffer_width = buffer_size;
				 }
				 else
				 {
					 buffer->framebuffer_width = width;
				 }
				 glGenTextures(1, &buffer->img);
				 glBindTexture(GL_TEXTURE_2D, buffer->img);
				 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				 glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, width, height, 0, 
					 GL_RGBA,GL_UNSIGNED_BYTE,NULL);
			 }
#endif
		 }
		 else if (buffer->type == GRAPHICS_BUFFER_WX_OFFSCREEN_TYPE)
		 {
		 }
		 else
		 {
#if defined (UNIX)
#if !defined (DARWIN)
				wxGLCanvas *test_canvas;
				int *attribute_ptr, number_of_visual_attributes, selection_level;
				visual_attributes = NULL;
				number_of_visual_attributes = 0;
				Event_dispatcher_use_wxCmguiApp_OnAssertFailure(1);
				number_of_visual_attributes = 20;
				return_code = 0;
				/* test either there are visual attributes stored in the current
					 buffer or not*/
				if (buffer_to_match)
				{
					 if (buffer_to_match->attrib_list)
					 {
							return_code = 1;
					 }
				}
				if (!return_code)
				{
					 /* if not, test, create a new visual attribute list and create a
							new canvas, else use the current visual attribute list*/
					 test_canvas = new wxTestingBuffer(parent, (Graphics_buffer *)NULL,
							graphics_buffer_package->wxSharedContext,
							visual_attributes);
					 if (ALLOCATE(visual_attributes, int, number_of_visual_attributes))
					 {
							selection_level = 5;
							while ((selection_level > 0) && (test_canvas->m_vi == NULL) || (selection_level == 5))
							{
								 attribute_ptr = visual_attributes;
								 *attribute_ptr = WX_GL_RGBA;
								 attribute_ptr++;
								 *attribute_ptr = WX_GL_MIN_RED;
								 attribute_ptr++;
								 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
								 attribute_ptr++;
								 *attribute_ptr = WX_GL_MIN_GREEN;
								 attribute_ptr++;
								 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
								 attribute_ptr++;
								 *attribute_ptr = WX_GL_MIN_BLUE;
								 attribute_ptr++;
								 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
								 attribute_ptr++;
								 if (selection_level > 3)
								 {
										*attribute_ptr = WX_GL_MIN_ALPHA;
										attribute_ptr++;
										*attribute_ptr = 1;
										attribute_ptr++;
								 }
								 if (minimum_depth_buffer_depth > 0)
								 {
										*attribute_ptr = WX_GL_DEPTH_SIZE;
										attribute_ptr++;
										*attribute_ptr = minimum_depth_buffer_depth;
										attribute_ptr++;
								 }
								 else
								 {
										if (selection_level > 2)
										{
											 /* Try to get a depth buffer anyway */
											 *attribute_ptr = WX_GL_DEPTH_SIZE;
											 attribute_ptr++;
											 *attribute_ptr = 16;
											 attribute_ptr++;
										}
								 }
								 if (minimum_accumulation_buffer_depth > 0)
								 {
										*attribute_ptr = WX_GL_MIN_ACCUM_RED;
										attribute_ptr++;
										*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
										attribute_ptr++;
										*attribute_ptr = WX_GL_MIN_ACCUM_GREEN;
										attribute_ptr++;
										*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
										attribute_ptr++;
										*attribute_ptr = WX_GL_MIN_ACCUM_BLUE;
										attribute_ptr++;
										*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
										attribute_ptr++;
								 }
								 else
								 {
										if (selection_level > 4)
										{
											 /* Try to get an accumulation buffer anyway */
											 *attribute_ptr = WX_GL_MIN_ACCUM_RED;
											 attribute_ptr++;
											 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
											 attribute_ptr++;
											 *attribute_ptr = WX_GL_MIN_ACCUM_GREEN;
											 attribute_ptr++;
											 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
											 attribute_ptr++;
											 *attribute_ptr = WX_GL_MIN_ACCUM_BLUE;
											 attribute_ptr++;
											 *attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
											 attribute_ptr++;
										}
								 }
								 switch (buffering_mode)
								 {
										case GRAPHICS_BUFFER_SINGLE_BUFFERING:
										case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
										{
											 *attribute_ptr = WX_GL_DOUBLEBUFFER;
											 attribute_ptr++;
										}break;
								 }
								 switch (stereo_mode)
								 {
										case GRAPHICS_BUFFER_MONO:
										case GRAPHICS_BUFFER_STEREO:
										{
											 *attribute_ptr = GL_STEREO;
											 attribute_ptr++;
										} break;
										/* default GRAPHICS_BUFFER_ANY_STEREO_MODE*/
								 }
								 *attribute_ptr = 0;
								 attribute_ptr++;
								 if (test_canvas)
								 {
										delete test_canvas;
								 }
								 test_canvas = new wxTestingBuffer(parent, (Graphics_buffer *)NULL,
										graphics_buffer_package->wxSharedContext,
										visual_attributes);
								 selection_level--;
								 if ((selection_level == 0) && (test_canvas->m_vi == NULL))
								 {
										DEALLOCATE(visual_attributes);
										visual_attributes = NULL;
										buffer->attrib_list = visual_attributes;
								 }
								 else if(test_canvas->m_vi != NULL)
								 {
										buffer->attrib_list = visual_attributes;
								 }
							}
							if (test_canvas)
							{
								 delete test_canvas;
							}
					 }
				}
				else
				{
					 if (buffer_to_match->attrib_list)
					 {
							/* if attrib_list is found on the buffer to match, copy it
								 into the new buffer, if not found, that means the
								 current buffer does not have any special attributes
								 setting, thus the new attributes will be default as NULL */
							int count;
							int *buffer_to_match_attribute_ptr;
							if (ALLOCATE(buffer->attrib_list,int, number_of_visual_attributes))
							{
								 buffer_to_match_attribute_ptr = buffer_to_match->attrib_list;
								 attribute_ptr = buffer->attrib_list;
								 for (count = 0; count < number_of_visual_attributes; count++)
								 {
										*attribute_ptr = *buffer_to_match_attribute_ptr;
										attribute_ptr++;
										buffer_to_match_attribute_ptr++;
								 }
							}
					 }
				}
#else /*defined (DARWIN) */
				/* Mac will receive an argument from wxGLCanvas to get
				   the best settings but requires the program to state
					 all the desired settings with a minimum settings. */
				visual_attributes = NULL;
				if (ALLOCATE(buffer->attrib_list, int, 25))
				{
					buffer->attrib_list[0] = WX_GL_RGBA;
					buffer->attrib_list[1] = WX_GL_DOUBLEBUFFER;
					buffer->attrib_list[2] = WX_GL_DEPTH_SIZE;
					buffer->attrib_list[3] = 1;
					buffer->attrib_list[4] = WX_GL_MIN_RED;
					buffer->attrib_list[5] = 1;
					buffer->attrib_list[6] = WX_GL_MIN_GREEN;
					buffer->attrib_list[7] = 1;
					buffer->attrib_list[8] = WX_GL_MIN_BLUE;
					buffer->attrib_list[9] = 1;
					buffer->attrib_list[10] = WX_GL_MIN_ALPHA;
					buffer->attrib_list[11] = 1;
					buffer->attrib_list[12] = WX_GL_MIN_ACCUM_RED;
					buffer->attrib_list[13] = 1;
					buffer->attrib_list[14] = WX_GL_MIN_ACCUM_GREEN;
					buffer->attrib_list[15] = 1;
					buffer->attrib_list[16] = WX_GL_MIN_ACCUM_BLUE;
					buffer->attrib_list[17] = 1;
					buffer->attrib_list[18] = WX_GL_MIN_ACCUM_ALPHA;
					buffer->attrib_list[19] = 1;
					buffer->attrib_list[20] = WX_GL_DEPTH_SIZE;
					buffer->attrib_list[21] = 1;
					buffer->attrib_list[22] = WX_GL_STENCIL_SIZE;
					buffer->attrib_list[23] = 1;
					buffer->attrib_list[24] = 0;
				};
#endif /*defined (DARWIN) */
#else /* defined (UNIX) */
				/* The above routine does not work for win32 as it does not have the
					 member m_vi in wxGLCanvas.
					 should find a way to get the best buffer, but this default setting should work fine. */
				visual_attributes = NULL;
				if (ALLOCATE(buffer->attrib_list, int, 5))
				{
					 buffer->attrib_list[0] = WX_GL_DOUBLEBUFFER;
					 buffer->attrib_list[1] = WX_GL_RGBA;
					 buffer->attrib_list[2] = WX_GL_MIN_ALPHA;
					 buffer->attrib_list[3] = 8;
					 buffer->attrib_list[4] = 0;
				}
#endif /* defined (UNIX) */
				if (!buffer->package->wxSharedContext)
				{
					 wxFrame *frame = new wxFrame(NULL, -1, "temporary", wxPoint(-1,-1), wxSize(500,500));
					 wxPanel *temp = new wxPanel(frame, -1, wxPoint(-1,-1), wxSize(450,450));
					 wxTestingBuffer *testingbuffer;
					 struct Graphics_buffer *temp_buffer;
					 temp_buffer = CREATE(Graphics_buffer)(graphics_buffer_package);
					 temp_buffer->type= GRAPHICS_BUFFER_WX_TYPE;
					 temp_buffer->parent = temp;
					 temp_buffer->attrib_list = NULL;
					 testingbuffer = new wxTestingBuffer(temp, temp_buffer,
							graphics_buffer_package->wxSharedContext,
							buffer->attrib_list);
					 frame->Show(true);
					 testingbuffer->Set_wx_SharedContext();
					 frame->Show(false);
					 DESTROY(Graphics_buffer)(&temp_buffer);
				}
				buffer->canvas = new wxGraphicsBuffer(parent,
					 graphics_buffer_package->wxSharedContext,
					 buffer, buffer->attrib_list);
				wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
				topsizer->Add(buffer->canvas,
					 wxSizerFlags(1).Align(wxALIGN_CENTER).Expand());
				parent->SetSizer(topsizer);
		 }
	}
	else
	{
		 display_message(ERROR_MESSAGE,"Graphics_buffer_create_buffer_wx.  "
				"Unable to create generic Graphics_buffer.");
		 buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

} /* Graphics_buffer_create_buffer_wx */
#endif /* defined (WX_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
#if defined (OPENGL_API)
static void Graphics_buffer_create_buffer_glx(struct Graphics_buffer *buffer,
	struct Graphics_buffer_package *graphics_buffer_package,
	enum Graphics_buffer_class buffer_class,
	Widget x3d_parent_widget, int width, int height,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_alpha_buffer_depth,
	int minimum_accumulation_buffer_depth,
	struct Graphics_buffer *buffer_to_match)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :


HISTORY :
I have merged here the code for choosing a visual for X3d and dm_interface so
that it is all in one place.
==============================================================================*/
{
#if defined (GLX_SGIX_dmbuffer) && defined (GLX_SGIX_fbconfig)
	DMparams *imageFormat, *poolSpec;
	DMboolean cacheable = DM_FALSE;
	DMboolean mapped = DM_FALSE;
	int dmbuffer_attribs [] =
	{
		GLX_DIGITAL_MEDIA_PBUFFER_SGIX, True,
		GLX_PRESERVED_CONTENTS_SGIX, True,
		(int) None
	};
#endif /* defined (GLX_SGIX_dmbuffer) */
	int config_index, nelements;
#if defined USE_GLX_FBCONFIG
	int glx_major_version, glx_minor_version;
#endif /* defined USE_GLX_FBCONFIG */
	Display *display;
	int *attribute_ptr, number_of_visual_attributes, selection_level,
		*visual_attributes;
	XVisualInfo *visual_info;

	ENTER(Graphics_buffer_select_visual);

	visual_attributes = NULL;
	number_of_visual_attributes = 0;
	display = graphics_buffer_package->display;

	/* 1: SGIX digital media pbuffer is preferred for offscreen when available (O2's) */
#if defined (GLX_SGIX_dmbuffer) && defined (GLX_SGIX_fbconfig)
	if ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type) &&
		((GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS == buffer_class) ||
		(GRAPHICS_BUFFER_OFFSCREEN_CLASS == buffer_class)) &&
		query_glx_extension("GLX_SGIX_dmbuffer", display, DefaultScreen(display)))
	{
		/* This is similar to the selection in 4: as it uses glXChooseVisual */
		number_of_visual_attributes = 9;
		if (minimum_depth_buffer_depth > 0)
		{
			number_of_visual_attributes += 2;
		}
		if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
		{
			visual_attributes[0] = GLX_RGBA;
			visual_attributes[1] = GLX_RED_SIZE;
			visual_attributes[2] = (minimum_colour_buffer_depth + 2) / 3;
			visual_attributes[3] = GLX_GREEN_SIZE;
			visual_attributes[4] = (minimum_colour_buffer_depth + 2) / 3;
			visual_attributes[5] = GLX_BLUE_SIZE;
			visual_attributes[6] = (minimum_colour_buffer_depth + 2) / 3;
			visual_attributes[7] = GLX_ALPHA_SIZE;
			visual_attributes[8] = minimum_alpha_buffer_depth;
			if (minimum_depth_buffer_depth > 0)
			{
				visual_attributes[9] = GLX_DEPTH_SIZE;
				visual_attributes[10] = minimum_depth_buffer_depth;
			}

			if((DM_SUCCESS==dmParamsCreate( &imageFormat ))
				&& (DM_SUCCESS==dmSetImageDefaults( imageFormat,
						width, height, DM_IMAGE_PACKING_RGBA ))
				/*	&& (DM_SUCCESS==dmParamsSetEnum( imageFormat,
				  DM_IMAGE_ORIENTATION, DM_TOP_TO_BOTTOM ))*/
				&& (DM_SUCCESS==dmParamsSetEnum( imageFormat,
						DM_IMAGE_LAYOUT, DM_IMAGE_LAYOUT_GRAPHICS )))
			{
				if((DM_SUCCESS==dmParamsCreate(&poolSpec))
					&& (DM_SUCCESS==dmBufferSetPoolDefaults( poolSpec,
					/*count*/1, 0, cacheable, mapped ))
					&& (DM_SUCCESS==dmBufferGetGLPoolParams( imageFormat,
							poolSpec )))
				{
					if ((DM_SUCCESS==dmBufferCreatePool(poolSpec, &(buffer->dmpool)))
						&&(DM_SUCCESS==dmBufferAllocatebuffer->dmpool,
								&(buffer->dmbuffer))))
					{
						if((buffer->visual_info = glXChooseVisual(display,
									DefaultScreen(display), visual_attributes))
							&& (buffer->config  = glXGetFBConfigFromVisualSGIX(
									 display, buffer->visual_info)))
						{
							if (buffer->glx_pbuffer = glXCreateGLXPbufferSGIX(display,
								buffer->config, width, height, dmbuffer_attribs))
							{
								if (buffer->context = glXCreateContextWithConfigSGIX(
									display, buffer->config, GLX_RGBA_TYPE_SGIX,
									graphics_buffer_package->shared_glx_context, GL_TRUE))
								{
									if (glXAssociateDMPbufferSGIX(display,
											buffer->glx_pbuffer, imageFormat, buffer->dmbuffer))
									{
										/* Finished I think, hooray! */
										buffer->type = GRAPHICS_BUFFER_GLX_DM_PBUFFER_TYPE;
										if (!graphics_buffer_package->shared_glx_context &&
											buffer->context)
										{
											graphics_buffer_package->shared_glx_context = buffer->context;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Unable to associate pbuffer with dmbuffer");
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot get GLX context");
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot create pbuffer");
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot get Frame Buffer Configuration");
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot allocate Dmbuffer");
					}
					dmParamsDestroy(poolSpec);
				}
				else
				{
					display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot create DM Pool Parameters");
				}
				dmParamsDestroy(imageFormat);
			}
			else
			{
				display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot create image parameters");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(Dm_buffer). Cannot allocate image attributes");
		}
		printf("Graphics_buffer_create_buffer_glx dmbuffer\n");
		printf("   buffer_class : %d\n", buffer_class);
		printf("   buffer->type : %d\n", buffer->type);
		printf("   buffer->visual_info : %p\n", buffer->visual_info);
		printf("   buffer->context : %p\n\n", buffer->context);
	}
#endif /* defined (GLX_SGIX_dmbuffer) */

	/* 2: The old equivalent SGIX pbuffer extensions seem to work better on the SGIs. */
	/* Superseded by the GLX 1.3 code in 3 so don't do it unless we are on an IRIX box and
		IRIX display. */
#if defined (SGI) && defined (GLX_SGIX_pbuffer)
	if ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type) &&
		query_glx_extension("GLX_SGIX_fbconfig", display, DefaultScreen(display)) &&
		/* Only if an SGI server */
		!strncmp("SGI", glXQueryServerString(display, DefaultScreen(display), GLX_VENDOR), 3))
	{
		XVisualInfo *visual_info;

		if (graphics_buffer_package->override_visual_id)
		{
			number_of_visual_attributes = 5;
			if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
			{
				attribute_ptr = visual_attributes;
				*attribute_ptr = GLX_RENDER_TYPE;
				attribute_ptr++;
				*attribute_ptr = GLX_RGBA_BIT;
				attribute_ptr++;
				*attribute_ptr = None;
				attribute_ptr++;

				if (buffer->config_list = glXChooseFBConfigSGIX(display,
					DefaultScreen(display), visual_attributes, &nelements))
				{
					config_index = 0;
					while ((config_index < nelements) &&
					  (GRAPHICS_BUFFER_INVALID_TYPE == buffer->type))
					{
					  if (visual_info = glXGetVisualFromFBConfigSGIX(display,
						 buffer->config_list[config_index]))
					  {
						 if (visual_info->visualid ==
							graphics_buffer_package->override_visual_id)
						 {
							if (buffer_class == GRAPHICS_BUFFER_OFFSCREEN_CLASS)
							{
							  /* Try a shared buffer first */
							  Graphics_buffer_create_from_fb_config_sgi(buffer,
								 graphics_buffer_package,
								 GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS,
								 x3d_parent_widget,
								 width, height, buffer->config_list[config_index]);
							}
							if (GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
							{
							  Graphics_buffer_create_from_fb_config_sgi(buffer,
								 graphics_buffer_package, buffer_class, x3d_parent_widget,
								 width, height, buffer->config_list[config_index]);
							}
						 }
						 XFree(visual_info);
					  }
					  config_index++;
					}
				}
			}
		}
		else if (buffer_to_match)
		{
			if (buffer_to_match->config)
			{
				/* The SGI does not have enough graphics resources to provide
					matching offscreen buffers, so we try for one that is OK instead. */
				Graphics_buffer_create_buffer_glx(buffer, graphics_buffer_package,
					buffer_class, x3d_parent_widget, width, height,
					GRAPHICS_BUFFER_ANY_BUFFERING_MODE,
					GRAPHICS_BUFFER_ANY_STEREO_MODE,
					/*minimum_colour_buffer_depth*/8, /*minimum_depth_buffer_depth*/8,
					/*minimum_alpha_buffer_depth*/1, /*minimum_accumulation_buffer_depth*/0,
					/*buffer_to_match*/(struct Graphics_buffer *)NULL);
			}
		}
		else
		{
			/* When using Mesa3D the buffers are created in software, and for a given visual
				extra buffers such as accumulation buffers are only created if requested.
				So to start with we must request the maximum we may need and then work down */
			/* Allocate the maximum possible number of attributes */
			number_of_visual_attributes = 27;
			if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
			{
				/* Unfortunately this selection algorithm may do unnecessary passes
					when minimum depth and minimum alpha passes */
				selection_level = 5;
				while ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
					&& (selection_level > 0))
				{
					attribute_ptr = visual_attributes;
					*attribute_ptr = GLX_RENDER_TYPE;
					attribute_ptr++;
					*attribute_ptr = GLX_RGBA_BIT;
					attribute_ptr++;
					switch (buffer_class)
					{
						case GRAPHICS_BUFFER_ONSCREEN_CLASS:
						{
							*attribute_ptr = GLX_DRAWABLE_TYPE;
							attribute_ptr++;
							*attribute_ptr = GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS:
						case GRAPHICS_BUFFER_OFFSCREEN_CLASS:
						{
							if ((selection_level > 1) ||
								(GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS == buffer_class))
							{
								*attribute_ptr = GLX_DRAWABLE_TYPE;
								attribute_ptr++;
								*attribute_ptr = GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
								attribute_ptr++;
							}
							else
							{
								*attribute_ptr = GLX_DRAWABLE_TYPE;
								attribute_ptr++;
								*attribute_ptr = GLX_PIXMAP_BIT;
								attribute_ptr++;
							}
						} break;
					}
					*attribute_ptr = GLX_RED_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_GREEN_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_BLUE_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					if (minimum_depth_buffer_depth > 0)
					{
						*attribute_ptr = GLX_DEPTH_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_depth_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 2)
						{
							/* Try to get a depth buffer anyway */
							*attribute_ptr = GLX_DEPTH_SIZE;
							attribute_ptr++;
							*attribute_ptr = 16;
							attribute_ptr++;
						}
					}
					if (minimum_alpha_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ALPHA_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_alpha_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 3)
						{
							/* Try to get an alpha buffer anyway */
							*attribute_ptr = GLX_ALPHA_SIZE;
							attribute_ptr++;
							*attribute_ptr = 1;
							attribute_ptr++;
						}
					}
					if (minimum_accumulation_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ACCUM_RED_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 4)
						{
							/* Try to get an accumulation buffer anyway */
							*attribute_ptr = GLX_ACCUM_RED_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
						}
					}
					switch (buffering_mode)
					{
						case GRAPHICS_BUFFER_SINGLE_BUFFERING:
						{
							*attribute_ptr = GLX_DOUBLEBUFFER;
							attribute_ptr++;
							*attribute_ptr = GL_FALSE;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
						{
							*attribute_ptr = GLX_DOUBLEBUFFER;
							attribute_ptr++;
							*attribute_ptr = GL_TRUE;
							attribute_ptr++;
						} break;
						/* default GRAPHICS_BUFFER_ANY_BUFFERING_MODE:
							do nothing as GLX_DONT_CARE is the default */
					}
					switch (stereo_mode)
					{
						case GRAPHICS_BUFFER_MONO:
						{
							*attribute_ptr = GLX_STEREO;
							attribute_ptr++;
							*attribute_ptr = GL_FALSE;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_STEREO:
						{
							*attribute_ptr = GLX_STEREO;
							attribute_ptr++;
							*attribute_ptr = GL_TRUE;
							attribute_ptr++;
						} break;
						/* default GRAPHICS_BUFFER_ANY_STEREO_MODE:
							do nothing as GLX_DONT_CARE is the default */
					}
					*attribute_ptr = None;
					attribute_ptr++;

					if (buffer->config_list = glXChooseFBConfigSGIX(display,
						DefaultScreen(display), visual_attributes, &nelements))
					{
						/* Need to copy config we select and free the list, currently leaky */

						config_index = 0;
						/* Check we can actually create what we want, pbuffer or  */
						while ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type) &&
							(config_index < nelements))
						{
							if ((selection_level > 1) && (GRAPHICS_BUFFER_OFFSCREEN_CLASS == buffer_class))
							{
								/* Try to get an offscreen shared buffer first if we can */
								Graphics_buffer_create_from_fb_config_sgi(buffer,
									graphics_buffer_package, GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS,
									x3d_parent_widget,
									width, height, buffer->config_list[config_index]);
							}
							else
							{
								Graphics_buffer_create_from_fb_config_sgi(buffer,
									graphics_buffer_package, buffer_class, x3d_parent_widget,
									width, height, buffer->config_list[config_index]);
							}
							config_index++;
						}
					}
					selection_level--;
				}
			}
		}
	}
#endif /* defined (SGI) && defined (GLX_SGIX_pbuffer) */

	/* 3: Use fbconfig functions to select visual if available */
#if defined USE_GLX_FBCONFIG
	if ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type) &&
		glXQueryVersion(display,&glx_major_version, &glx_minor_version) &&
		((glx_major_version == 1) && (glx_minor_version > 2)))
	{
		if (graphics_buffer_package->override_visual_id)
		{
			number_of_visual_attributes = 5;
			if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
			{
				attribute_ptr = visual_attributes;
				*attribute_ptr = GLX_RENDER_TYPE;
				attribute_ptr++;
				*attribute_ptr = GLX_RGBA_BIT;
				attribute_ptr++;
				*attribute_ptr = None;
				attribute_ptr++;

				if (buffer->config_list = glXChooseFBConfig(display,
					DefaultScreen(display), visual_attributes, &nelements))
				{
					config_index = 0;
					while ((config_index < nelements) &&
					  (GRAPHICS_BUFFER_INVALID_TYPE == buffer->type))
					{
					  if (visual_info = glXGetVisualFromFBConfig(display,
						 buffer->config_list[config_index]))
					  {
						  if ((int)visual_info->visualid ==
							 graphics_buffer_package->override_visual_id)
						 {
							if (buffer_class == GRAPHICS_BUFFER_OFFSCREEN_CLASS)
							{
							  /* Try a shared buffer first */
							  Graphics_buffer_create_from_fb_config(buffer,
								 graphics_buffer_package,
								 GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS,
								 x3d_parent_widget,
								 width, height, buffer->config_list[config_index]);
							}
							if (GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
							{
							  Graphics_buffer_create_from_fb_config(buffer,
								 graphics_buffer_package, buffer_class, x3d_parent_widget,
								 width, height, buffer->config_list[config_index]);
							}
						 }
						 XFree(visual_info);
					  }
					  config_index++;
					}
				}
			}
		}
		else if (buffer_to_match)
		{
			if (buffer_to_match->config)
			{
				Graphics_buffer_create_from_fb_config(buffer,
					graphics_buffer_package, buffer_class, x3d_parent_widget,
					width, height, buffer_to_match->config);
			}
		}
		else
		{
			/* When using Mesa3D the buffers are created in software, and for a given visual
				extra buffers such as accumulation buffers are only created if requested.
				So to start with we must request the maximum we may need and then work down */
			/* Allocate the maximum possible number of attributes */
			number_of_visual_attributes = 27;
			if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
			{
				/* Unfortunately this selection algorithm may do unnecessary passes
					when minimum depth and minimum alpha passes */
				selection_level = 5;
				while ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
					&& (selection_level > 0))
				{
					attribute_ptr = visual_attributes;
					*attribute_ptr = GLX_RENDER_TYPE;
					attribute_ptr++;
					*attribute_ptr = GLX_RGBA_BIT;
					attribute_ptr++;
					switch (buffer_class)
					{
						case GRAPHICS_BUFFER_ONSCREEN_CLASS:
						{
							*attribute_ptr = GLX_DRAWABLE_TYPE;
							attribute_ptr++;
							*attribute_ptr = GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS:
						case GRAPHICS_BUFFER_OFFSCREEN_CLASS:
						{
							if ((selection_level > 1) ||
								(GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS == buffer_class))
							{
								*attribute_ptr = GLX_DRAWABLE_TYPE;
								attribute_ptr++;
								*attribute_ptr = GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
								attribute_ptr++;
							}
							else
							{
								*attribute_ptr = GLX_DRAWABLE_TYPE;
								attribute_ptr++;
								*attribute_ptr = GLX_PIXMAP_BIT;
								attribute_ptr++;
							}
						} break;
					}
					*attribute_ptr = GLX_RED_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_GREEN_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_BLUE_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					if (minimum_depth_buffer_depth > 0)
					{
						*attribute_ptr = GLX_DEPTH_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_depth_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 2)
						{
							/* Try to get a depth buffer anyway */
							*attribute_ptr = GLX_DEPTH_SIZE;
							attribute_ptr++;
							*attribute_ptr = 16;
							attribute_ptr++;
						}
					}
					if (minimum_alpha_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ALPHA_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_alpha_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 3)
						{
							/* Try to get an alpha buffer anyway */
							*attribute_ptr = GLX_ALPHA_SIZE;
							attribute_ptr++;
							*attribute_ptr = 1;
							attribute_ptr++;
						}
					}
					if (minimum_accumulation_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ACCUM_RED_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 4)
						{
							/* Try to get an accumulation buffer anyway */
							*attribute_ptr = GLX_ACCUM_RED_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
						}
					}
					switch (buffering_mode)
					{
						case GRAPHICS_BUFFER_SINGLE_BUFFERING:
						{
							*attribute_ptr = GLX_DOUBLEBUFFER;
							attribute_ptr++;
							*attribute_ptr = GL_FALSE;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
						{
							*attribute_ptr = GLX_DOUBLEBUFFER;
							attribute_ptr++;
							*attribute_ptr = GL_TRUE;
							attribute_ptr++;
						} break;
						/* default GRAPHICS_BUFFER_ANY_BUFFERING_MODE:
							do nothing as GLX_DONT_CARE is the default */
					}
					switch (stereo_mode)
					{
						case GRAPHICS_BUFFER_MONO:
						{
							*attribute_ptr = GLX_STEREO;
							attribute_ptr++;
							*attribute_ptr = GL_FALSE;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_STEREO:
						{
							*attribute_ptr = GLX_STEREO;
							attribute_ptr++;
							*attribute_ptr = GL_TRUE;
							attribute_ptr++;
						} break;
						/* default GRAPHICS_BUFFER_ANY_STEREO_MODE:
							do nothing as GLX_DONT_CARE is the default */
					}
					*attribute_ptr = None;
					attribute_ptr++;

					if (buffer->config_list = glXChooseFBConfig(display,
						DefaultScreen(display), visual_attributes, &nelements))
					{
						/* Need to copy config we select and free the list, currently leaky */

						config_index = 0;
						/* Check we can actually create what we want, pbuffer or  */
						while ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type) &&
							(config_index < nelements))
						{
							if ((selection_level > 1) && (GRAPHICS_BUFFER_OFFSCREEN_CLASS == buffer_class))
							{
								/* Try to get an offscreen shared buffer first if we can */
								Graphics_buffer_create_from_fb_config(buffer,
									graphics_buffer_package, GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS,
									x3d_parent_widget,
									width, height, buffer->config_list[config_index]);
							}
							else
							{
								Graphics_buffer_create_from_fb_config(buffer,
									graphics_buffer_package, buffer_class, x3d_parent_widget,
									width, height, buffer->config_list[config_index]);
							}
							config_index++;
						}
					}
					selection_level--;
				}
			}
		}
	}
#endif /* defined GLX_fbconfig */

	/* 4: Use old GLX code */
	if (GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
	{
		if (graphics_buffer_package->override_visual_id)
		{
			XVisualInfo template_visual, *visual_info_list;

			template_visual.visualid = graphics_buffer_package->override_visual_id;

			if (visual_info_list = XGetVisualInfo(display, VisualIDMask, &template_visual,
				&nelements))
			{
				config_index = 0;
				Graphics_buffer_create_from_visual_info(buffer,
					graphics_buffer_package, buffer_class, x3d_parent_widget,
					width, height, visual_info_list + config_index);
				XFree(visual_info_list);
			}
		}
		else if (buffer_to_match)
		{
			if (buffer_to_match->visual_info)
			{
				Graphics_buffer_create_from_visual_info(buffer,
					graphics_buffer_package, buffer_class, x3d_parent_widget,
					width, height, buffer_to_match->visual_info);
			}
		}
		else
		{
			/* When using Mesa3D the buffers are created in software, and for a given visual
				extra buffers such as accumulation buffers are only created if requested.
				So to start with we must request the maximum we may need and then work down */
			/* Allocate the maximum possible number of attributes */
			number_of_visual_attributes = 27;
			if (REALLOCATE(visual_attributes, visual_attributes, int, number_of_visual_attributes))
			{
				/* Unfortunately this selection algorithm may do unnecessary passes
					when minimum depth and minimum alpha passes */
				selection_level = 6;
				while ((GRAPHICS_BUFFER_INVALID_TYPE == buffer->type)
					&& (selection_level > 0))
				{
					attribute_ptr = visual_attributes;
					*attribute_ptr = GLX_USE_GL;
					attribute_ptr++;
					*attribute_ptr = GLX_RGBA;
					attribute_ptr++;
					*attribute_ptr = GLX_RED_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_GREEN_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					*attribute_ptr = GLX_BLUE_SIZE;
					attribute_ptr++;
					*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
					attribute_ptr++;
					if (minimum_depth_buffer_depth > 0)
					{
						*attribute_ptr = GLX_DEPTH_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_depth_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 2)
						{
							/* Try to get a depth buffer anyway */
							*attribute_ptr = GLX_DEPTH_SIZE;
							attribute_ptr++;
							*attribute_ptr = 16;
							attribute_ptr++;
						}
					}
					if (minimum_alpha_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ALPHA_SIZE;
						attribute_ptr++;
						*attribute_ptr = minimum_alpha_buffer_depth;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 3)
						{
							/* Try to get an alpha buffer anyway */
							*attribute_ptr = GLX_ALPHA_SIZE;
							attribute_ptr++;
							*attribute_ptr = 1;
							attribute_ptr++;
						}
					}
					if (minimum_accumulation_buffer_depth > 0)
					{
						*attribute_ptr = GLX_ACCUM_RED_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
						*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
						attribute_ptr++;
						*attribute_ptr = (minimum_accumulation_buffer_depth + 2) / 3;
						attribute_ptr++;
					}
					else
					{
						if (selection_level > 4)
						{
							/* Try to get an accumulation buffer anyway */
							*attribute_ptr = GLX_ACCUM_RED_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_GREEN_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
							*attribute_ptr = GLX_ACCUM_BLUE_SIZE;
							attribute_ptr++;
							*attribute_ptr = (minimum_colour_buffer_depth + 2) / 3;
							attribute_ptr++;
						}
					}
					switch (buffering_mode)
					{
						case GRAPHICS_BUFFER_SINGLE_BUFFERING:
						{
							/* Defaults to single if not specified */
						} break;
						case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
						{
							*attribute_ptr = GLX_DOUBLEBUFFER;
							attribute_ptr++;
						} break;
						case GRAPHICS_BUFFER_ANY_BUFFERING_MODE:
						{
							/* Try double buffer, then try single buffer */
							if (selection_level > 5)
							{
								*attribute_ptr = GLX_DOUBLEBUFFER;
								attribute_ptr++;
							}
						} break;
					}
					switch (stereo_mode)
					{
						case GRAPHICS_BUFFER_MONO:
						{
							/* Defaults to mono if not specified */
						} break;
						case GRAPHICS_BUFFER_STEREO:
						{
							*attribute_ptr = GLX_STEREO;
							attribute_ptr++;
						} break;
						/* default GRAPHICS_BUFFER_ANY_STEREO_MODE:
							mono when not specified */
					}
					*attribute_ptr = None;
					attribute_ptr++;

					if (visual_info = glXChooseVisual(display,
						DefaultScreen(display), visual_attributes))
					{
						Graphics_buffer_create_from_visual_info(buffer,
							graphics_buffer_package, buffer_class, x3d_parent_widget,
							width, height, visual_info);
					}
					selection_level--;
				}
			}
		}
	}
	if (visual_attributes)
	{
		DEALLOCATE(visual_attributes);
	}

	LEAVE;
} /* Graphics_buffer_create_buffer_glx */
#endif /* defined (OPENGL_API) */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static void Graphics_buffer_gtkglarea_initialise_callback(GtkWidget *widget,
	gpointer graphics_buffer_void)
/*******************************************************************************
LAST MODIFIED : 10 July 2002

DESCRIPTION :
Called when part of the graphics_buffer window is initialised. Does not attempt to
redraw just the initialised area. Instead, it redraws the whole picture, but only
if there are no more initialise events pending.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	ENTER(graphics_buffer_gtkglarea_initialise_callback);
	if (widget && (graphics_buffer = (struct Graphics_buffer *)graphics_buffer_void))
	{
#if ! defined (GTK_USE_GTKGLAREA)
		graphics_buffer->glcontext = gtk_widget_get_gl_context(graphics_buffer->glarea);
		if (!graphics_buffer->package->share_glcontext)
		{
			/* This context is owned by the widget, so we can't keep a
			 * reference to it past the life of the widget, and we
			 * certainly can't destroy it. So instead, make a copy of
			 * it. The choice of glarea as the GLDrawable is arbitrary.
			 */
			graphics_buffer->package->share_glcontext =
				gtk_widget_create_gl_context(graphics_buffer->glarea,
				graphics_buffer->glcontext, TRUE, GDK_GL_RGBA_TYPE);

#if defined (UNIX)
			Graphics_library_initialise_gtkglext_glx_extensions(
				gdk_gl_context_get_gl_config(graphics_buffer->package->share_glcontext));
#endif /* defined (UNIX) */
		}
		graphics_buffer->gldrawable = gtk_widget_get_gl_drawable(graphics_buffer->glarea);
#endif /* defined (GTK_USER_INTERFACE) */
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->initialise_callback_list, graphics_buffer, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_gtkglarea_initialise_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* graphics_buffer_gtkglarea_initialise_callback */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static void Graphics_buffer_gtkglarea_resize_callback(GtkWidget *widget,
	GtkAllocation *allocation, gpointer graphics_buffer_void)
/*******************************************************************************
LAST MODIFIED : 10 July 2002

DESCRIPTION :
Called when part of the graphics_buffer window is resized. Does not attempt to
redraw just the resized area. Instead, it redraws the whole picture, but only
if there are no more resize events pending.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	ENTER(graphics_buffer_gtkglarea_resize_callback);
	USE_PARAMETER(allocation);
	if (widget && (graphics_buffer = (struct Graphics_buffer *)graphics_buffer_void))
	{
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->resize_callback_list, graphics_buffer, NULL);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_gtkglarea_resize_callback.  Invalid argument(s)");
	}
	LEAVE;
} /* graphics_buffer_gtkglarea_resize_callback */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static gboolean Graphics_buffer_gtkglarea_expose_callback(GtkWidget *widget,
	GdkEventExpose *expose_event, gpointer graphics_buffer_void)
/*******************************************************************************
LAST MODIFIED : 10 July 2002

DESCRIPTION :
Called when part of the graphics_buffer window is exposed. Does not attempt to
redraw just the exposed area. Instead, it redraws the whole picture, but only
if there are no more expose events pending.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	ENTER(Graphics_buffer_gtkglarea_expose_callback);
	USE_PARAMETER(expose_event);
	if (widget && (graphics_buffer = (struct Graphics_buffer *)graphics_buffer_void))
	{
		if (0 == expose_event->count)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
				graphics_buffer->expose_callback_list, graphics_buffer, NULL);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"graphics_buffer_gtkglarea_expose_callback.  Invalid argument(s)");
	}
	LEAVE;

	return(TRUE);
} /* graphics_buffer_gtkglarea_expose_callback */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static gboolean Graphics_buffer_gtkglarea_button_callback(GtkWidget *widget,
	GdkEventButton *button_event, gpointer graphics_buffer_structure)
/*******************************************************************************
LAST MODIFIED : 11 July 2002

DESCRIPTION :
The callback for mouse button input in the graphics_buffer window. The
resulting behaviour depends on the <graphics_buffer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/
{
	int input_modifier, return_code;
	struct Graphics_buffer *graphics_buffer;
	struct Graphics_buffer_input input;

	ENTER(Graphics_buffer_gtkglarea_button_callback);
	USE_PARAMETER(widget);
	if ((graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)
		&& button_event)
	{
		return_code = 1;
		input.type = GRAPHICS_BUFFER_INVALID_INPUT;
		switch(button_event->type)
		{
			case GDK_BUTTON_PRESS:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
			} break;
			case GDK_BUTTON_RELEASE:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Graphics_buffer_gtkglarea_button_callback.  Unknown button event");
				return_code=0;
				/* This event type is not being passed on */
			} break;
		}
		input.key_code = 0;
		input.button_number = button_event->button;
		/* Maybe I should change Graphics_buffer_input to have a higher
			resolution for position */
		input.position_x = static_cast<int>(button_event->x);
		input.position_y = static_cast<int>(button_event->y);
		input_modifier = 0;
		if (GDK_SHIFT_MASK&(button_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
		}
		if (GDK_CONTROL_MASK&(button_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
		}
		if (GDK_MOD1_MASK&(button_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
		}
		if (GDK_BUTTON1_MASK&(button_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
		}
		input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
			(input_modifier);
		if (return_code)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				graphics_buffer->input_callback_list, graphics_buffer, &input);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_gtkglarea_button_callback.  Invalid argument(s)");
	}
	LEAVE;

	return(TRUE);
} /* Graphics_buffer_gtkglarea_button_callback */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
static int Graphics_buffer_win32_button_callback(
	unsigned int *button_event, struct Graphics_buffer *graphics_buffer,
	WPARAM wParam, LPARAM lParam)
/*******************************************************************************
LAST MODIFIED : 11 July 2002

DESCRIPTION :
The callback for mouse button input in the graphics_buffer window. The
resulting behaviour depends on the <graphics_buffer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/
{
	int input_modifier, return_code;
	struct Graphics_buffer_input input;

	ENTER(Graphics_buffer_win32_button_callback);

	return_code = 1;
	input.type = GRAPHICS_BUFFER_INVALID_INPUT;
	switch(*button_event)
	{
		case WM_LBUTTONDOWN:
		{
			input.button_number = 1;
			input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
		} break;
		case WM_LBUTTONUP:
		{
			input.button_number = 1;
			input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
		} break;
		case WM_RBUTTONDOWN:
		{
			input.button_number = 3;
			input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
		} break;
		case WM_RBUTTONUP:
		{
			input.button_number = 3;
			input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
		} break;
		case WM_MBUTTONDOWN:
		{
			input.button_number = 2;
			input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
		} break;
		case WM_MBUTTONUP:
		{
			input.button_number = 2;
			input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
		} break;
		case WM_MOUSEMOVE:
		{
			input.button_number = 0;
			input.type = GRAPHICS_BUFFER_MOTION_NOTIFY;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Graphics_buffer_button_button_callback.  Unknown button event");
			return_code=0;
			/* This event type is not being passed on */
		} break;
	}
	input.key_code = 0;
	input.position_x = GET_X_LPARAM(lParam);
	input.position_y = GET_Y_LPARAM(lParam);
	input_modifier = 0;
	if (MK_SHIFT & wParam)
	{
		input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
	}
	if (MK_CONTROL & wParam)
	{
		input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
	}
	if (GetKeyState(VK_MENU) < 0)
	{
		input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
	}
/*
	if (MK_XBUTTON1 == wParam)
	{
		input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
	}
*/
	input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
		(input_modifier);
	if (return_code)
	{
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
			graphics_buffer->input_callback_list, graphics_buffer, &input);
	}
	LEAVE;

	return(TRUE);
} /* Graphics_buffer_win32_button_callback */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static gboolean Graphics_buffer_gtkglarea_key_callback(GtkWidget *widget,
	GdkEventKey *key_event, gpointer graphics_buffer_structure)
/*******************************************************************************
LAST MODIFIED : 11 July 2002

DESCRIPTION :
The callback for key input in the graphics_buffer window. The
resulting behaviour depends on the <graphics_buffer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/
{
	int input_modifier, return_code;
	struct Graphics_buffer *graphics_buffer;
	struct Graphics_buffer_input input;

	ENTER(Graphics_buffer_gtkglarea_key_callback);
	USE_PARAMETER(widget);
	if ((graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)
		&& key_event)
	{
		return_code = 1;
		input.type = GRAPHICS_BUFFER_INVALID_INPUT;
		switch(key_event->type)
		{
			case GDK_KEY_PRESS:
			{
				input.type = GRAPHICS_BUFFER_KEY_PRESS;
			} break;
			case GDK_KEY_RELEASE:
			{
				input.type = GRAPHICS_BUFFER_KEY_RELEASE;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Graphics_buffer_gtkglarea_key_callback.  Unknown key event");
				return_code=0;
				/* This event type is not being passed on */
			} break;
		}
		input.button_number = 0;
		input.key_code = 0;
		input.position_x = 0;
		input.position_y = 0;
		input_modifier = (enum Graphics_buffer_input_modifier)0;
		if (GDK_SHIFT_MASK&(key_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
		}
		if (GDK_CONTROL_MASK&(key_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
		}
		if (GDK_MOD1_MASK&(key_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
		}
		if (GDK_BUTTON1_MASK&(key_event->state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
		}
		if (return_code)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				graphics_buffer->input_callback_list, graphics_buffer, &input);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_gtkglarea_key_callback.  Invalid argument(s)");
	}
	LEAVE;

	return(TRUE);
} /* Graphics_buffer_gtkglarea_key_callback */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
static gboolean Graphics_buffer_gtkglarea_motion_notify_callback(
	GtkWidget *widget,
	GdkEventMotion *motion_event, gpointer graphics_buffer_structure)
/*******************************************************************************
LAST MODIFIED : 11 July 2002

DESCRIPTION :
The callback for mouse button input in the graphics_buffer window. The
resulting behaviour depends on the <graphics_buffer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/
{
	GdkModifierType state;
	int input_modifier, return_code;
	struct Graphics_buffer *graphics_buffer;
	struct Graphics_buffer_input input;

	ENTER(Graphics_buffer_gtkglarea_motion_notify_callback);
	USE_PARAMETER(widget);
	if ((graphics_buffer=(struct Graphics_buffer *)graphics_buffer_structure)
		&& motion_event)
	{
		return_code = 1;
		input.button_number = 0;
		input.type = GRAPHICS_BUFFER_MOTION_NOTIFY;
		input.key_code = 0;
		if (motion_event->is_hint)
		{
			gdk_window_get_pointer(motion_event->window, &input.position_x,
				&input.position_y, &state);
		}
		else
		{
			input.position_x = static_cast<int>(motion_event->x);
			input.position_y = static_cast<int>(motion_event->y);
			state = static_cast<GdkModifierType>(motion_event->state);
		}
		input_modifier = 0;
		if (GDK_SHIFT_MASK&(state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
		}
		if (GDK_CONTROL_MASK&(state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
		}
		if (GDK_MOD1_MASK&(state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
		}
		if (GDK_BUTTON1_MASK&(state))
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
		}
		input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
			(input_modifier);
		if (return_code)
		{
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				graphics_buffer->input_callback_list, graphics_buffer, &input);
		}

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_gtkglarea_motion_notify_callback.  Invalid argument(s)");
	}
	LEAVE;

	return(TRUE);
} /* Graphics_buffer_gtkglarea_motion_notify_callback */
#endif /* defined (GTK_USER_INTERFACE) */

/*
Global functions
----------------
*/

struct Graphics_buffer_package *CREATE(Graphics_buffer_package)(
	struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
Creates a Graphics_buffer_package which enables Graphics_buffers created from
it to share graphics contexts.
==============================================================================*/
{
	struct Graphics_buffer_package *package;

	ENTER(CREATE(Graphics_buffer_package));

#if ! defined (MOTIF_USER_INTERFACE)
	USE_PARAMETER(user_interface);
#endif /* ! defined (MOTIF_USER_INTERFACE) */
	if (ALLOCATE(package, struct Graphics_buffer_package, 1))
	{
		package->override_visual_id = 0;

#if defined (MOTIF_USER_INTERFACE)
		package->shared_glx_context = (GLXContext)NULL;
		package->display = User_interface_get_display(user_interface);
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#  if defined (GTK_USE_GTKGLAREA)
		package->share_glarea = (GtkWidget *)NULL;
#  else /* defined (GTK_USE_GTKGLAREA) */
		package->share_glcontext = (GdkGLContext *)NULL;
#  endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
		package->wxSharedContext = (wxGLContext*)NULL;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
		package->user_interface = user_interface;
		package->wgl_shared_context = (HGLRC)NULL;
#endif /* defined (WIN32_USER_INTERFACE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Graphics_buffer_package). "
			"Unable to allocate package structure");
		package = (struct Graphics_buffer_package *)NULL;
	}

	LEAVE;
	return (package);
} /* CREATE(Graphics_buffer_package) */

int DESTROY(Graphics_buffer_package)(struct Graphics_buffer_package **package_ptr)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
Closes the Graphics buffer package
==============================================================================*/
{
	int return_code;
	struct Graphics_buffer_package *package;

	ENTER(DESTROY(Graphics_buffer_package));
	if (package_ptr && (package = *package_ptr))
	{
		return_code=1;
#if defined (MOTIF_USER_INTERFACE)
		/* Destroy the shared_glx_context as we did not destroy it when closing
			it's buffer */
		if (package->shared_glx_context)
		{
			glXDestroyContext(package->display, package->shared_glx_context);
		}
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#  if ! defined (GTK_USE_GTKGLAREA)
		if (package->share_glcontext)
		{
			gdk_gl_context_destroy(package->share_glcontext);
		}
#  endif /* ! defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
		/* Destroy the shared_glx_context as we did not destroy it when closing
			it's buffer */
		if (package->wgl_shared_context)
		{
			wglDeleteContext(package->wgl_shared_context);
		}
#endif /* defined (WIN32_USER_INTERFACE) */

		DEALLOCATE(*package_ptr);
		*package_ptr = (struct Graphics_buffer_package *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Graphics_buffer_package).  Missing package");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Graphics_buffer_package) */

int Graphics_buffer_package_set_override_visual_id(
	struct Graphics_buffer_package *graphics_buffer_package,
	int override_visual_id)
/*******************************************************************************
LAST MODIFIED : 21 May 2004

DESCRIPTION :
Sets a particular visual to be used by all graphics buffers.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_package_set_override_visual_id);
	if (graphics_buffer_package)
	{
		graphics_buffer_package->override_visual_id = override_visual_id;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_package_set_override_visual_id.  "
			"Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_package_set_override_visual_id */

struct Graphics_buffer *create_Graphics_buffer_offscreen(
	struct Graphics_buffer_package *graphics_buffer_package,
	int width, int height,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_offscreen);

	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
#if defined (MOTIF_USER_INTERFACE)
		Graphics_buffer_create_buffer_glx(buffer, graphics_buffer_package,
			GRAPHICS_BUFFER_OFFSCREEN_CLASS, (Widget)NULL, width, height,
			buffering_mode, stereo_mode, minimum_colour_buffer_depth,
			minimum_depth_buffer_depth, /*minimum_alpha_buffer_depth*/0,
			minimum_accumulation_buffer_depth,
			/*buffer_to_match*/(struct Graphics_buffer *)NULL);
#else /* defined (MOTIF_USER_INTERFACE) */
		USE_PARAMETER(width);
		USE_PARAMETER(height);
		USE_PARAMETER(buffering_mode);
		USE_PARAMETER(stereo_mode);
		USE_PARAMETER(minimum_colour_buffer_depth);
		USE_PARAMETER(minimum_depth_buffer_depth);
		USE_PARAMETER(minimum_accumulation_buffer_depth);
#endif /* defined (MOTIF_USER_INTERFACE) */
		if (buffer->type == GRAPHICS_BUFFER_INVALID_TYPE)
		{
#if defined (DEBUG)
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen.  "
				"Unable to create offscreen graphics buffer.");
#endif /* defined (DEBUG) */
			DESTROY(Graphics_buffer)(&buffer);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_offscreen */

struct Graphics_buffer *create_Graphics_buffer_shared_offscreen(
	struct Graphics_buffer_package *graphics_buffer_package,
	int width, int height,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_offscreen);

	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
#if defined (MOTIF_USER_INTERFACE)
		Graphics_buffer_create_buffer_glx(buffer, graphics_buffer_package,
			GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS, (Widget)NULL, width, height,
			buffering_mode, stereo_mode, minimum_colour_buffer_depth,
			minimum_depth_buffer_depth, /*minimum_alpha_buffer_depth*/0,
			minimum_accumulation_buffer_depth,
			/*buffer_to_match*/(struct Graphics_buffer *)NULL);
#else /* defined (MOTIF_USER_INTERFACE) */
		USE_PARAMETER(width);
		USE_PARAMETER(height);
		USE_PARAMETER(buffering_mode);
		USE_PARAMETER(stereo_mode);
		USE_PARAMETER(minimum_colour_buffer_depth);
		USE_PARAMETER(minimum_depth_buffer_depth);
		USE_PARAMETER(minimum_accumulation_buffer_depth);
#endif /* defined (MOTIF_USER_INTERFACE) */
		if (buffer->type == GRAPHICS_BUFFER_INVALID_TYPE)
		{
#if defined (DEBUG)
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen.  "
				"Unable to create offscreen graphics buffer.");
#endif /* defined (DEBUG) */
			DESTROY(Graphics_buffer)(&buffer);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_offscreen */

struct Graphics_buffer *create_Graphics_buffer_offscreen_from_buffer(
	int width, int height, struct Graphics_buffer *buffer_to_match)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_offscreen_from_buffer);

	if (buffer = CREATE(Graphics_buffer)(buffer_to_match->package))
	{
#if defined (MOTIF_USER_INTERFACE)
		Graphics_buffer_create_buffer_glx(buffer, buffer_to_match->package,
			GRAPHICS_BUFFER_OFFSCREEN_SHARED_CLASS, (Widget)NULL, width, height,
			GRAPHICS_BUFFER_ANY_BUFFERING_MODE, GRAPHICS_BUFFER_ANY_STEREO_MODE,
			/*minimum_colour_buffer_depth*/0, /*minimum_depth_buffer_depth */0,
			/*minimum_alpha_buffer_depth*/0, /*minimum_accumulation_buffer_depth*/0,
			buffer_to_match);
#elif defined (WX_USER_INTERFACE)
 		buffer->type = GRAPHICS_BUFFER_WX_OFFSCREEN_TYPE;
#if defined (OPENGL_API) && (GL_EXT_framebuffer_object)
		if (Graphics_library_load_extension("GL_EXT_framebuffer_object"))
		{
			buffer->type = GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE;
		}
#endif
		Graphics_buffer_create_buffer_wx(buffer, buffer_to_match->package,
			NULL, GRAPHICS_BUFFER_ANY_BUFFERING_MODE,
			GRAPHICS_BUFFER_ANY_STEREO_MODE,
			0, 0, 0, width, height,
			buffer_to_match);
#else /* defined (MOTIF_USER_INTERFACE) */
		USE_PARAMETER(width);
		USE_PARAMETER(height);
		USE_PARAMETER(buffer_to_match);
#endif /* defined (MOTIF_USER_INTERFACE) */
		if (buffer->type == GRAPHICS_BUFFER_INVALID_TYPE)
		{
#if defined (DEBUG)
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen_from_buffer.  "
				"Unable to create offscreen_from_buffer graphics buffer.");
			buffer = (struct Graphics_buffer *)NULL;
#endif /* defined (DEBUG) */
			DESTROY(Graphics_buffer)(&buffer);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_offscreen_from_buffer.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_offscreen_from_buffer */

#if defined (MOTIF_USER_INTERFACE)
struct Graphics_buffer *create_Graphics_buffer_X3d(
	struct Graphics_buffer_package *graphics_buffer_package,
	Widget parent, int width, int height,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 5 May 2004

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_X3d);

	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
		Graphics_buffer_create_buffer_glx(buffer, graphics_buffer_package,
			GRAPHICS_BUFFER_ONSCREEN_CLASS, parent, width, height,
			buffering_mode, stereo_mode, minimum_colour_buffer_depth,
			minimum_depth_buffer_depth, /*minimum_alpha_buffer_depth*/0,
			minimum_accumulation_buffer_depth,
			/*buffer_to_match*/(struct Graphics_buffer *)NULL);
		if (buffer->type != GRAPHICS_BUFFER_GLX_X3D_TYPE)
		{
#if defined (DEBUG)
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_X3d.  "
				"Unable to create X3d graphics buffer.");
#endif /* defined (DEBUG) */
			DESTROY(Graphics_buffer)(&buffer);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_X3d.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_X3d */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
struct Graphics_buffer *create_Graphics_buffer_X3d_from_buffer(
	Widget parent, int width, int height,
	struct Graphics_buffer *buffer_to_match)
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_X3d_from_buffer);

	if (buffer = CREATE(Graphics_buffer)(buffer_to_match->package))
	{
		Graphics_buffer_create_buffer_glx(buffer, buffer_to_match->package,
			GRAPHICS_BUFFER_ONSCREEN_CLASS, parent, width, height,
			GRAPHICS_BUFFER_ANY_BUFFERING_MODE, GRAPHICS_BUFFER_ANY_STEREO_MODE,
			/*minimum_colour_buffer_depth*/0, /*minimum_depth_buffer_depth */0,
			/*minimum_alpha_buffer_depth*/0, /*minimum_accumulation_buffer_depth*/0,
			buffer_to_match);
		if (buffer->type == GRAPHICS_BUFFER_INVALID_TYPE)
		{
#if defined (DEBUG)
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_X3d_from_buffer.  "
				"Unable to create X3d graphics buffer.");
#endif /* defined (DEBUG) */
			DESTROY(Graphics_buffer)(&buffer);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_X3d_from_buffer.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_X3d_from_buffer */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (MOTIF_USER_INTERFACE)
Widget Graphics_buffer_X3d_get_widget(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 17 November 2005

DESCRIPTION :
Private routine to facilitate the compilation of Graphics fonts with only
a Graphics_buffer.
==============================================================================*/
{
	Widget widget;

	ENTER(Graphics_buffer_X3d_get_widget);

	if (buffer)
	{
		widget = buffer->drawing_widget;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_X3d_get_widget.  "
			"Unable to create generic Graphics_buffer.");
		widget = (Widget)NULL;
	}
	LEAVE;

	return (widget);
} /* Graphics_buffer_X3d_get_widget */
#endif /* defined (MOTIF_USER_INTERFACE) */

#if defined (ENABLE_GTK_PBUFFER_RENDERING)
#if defined (GTK_USER_INTERFACE)
static int Graphics_buffer_gtk_reallocate_offscreen_size(
	struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 10 March 2008

DESCRIPTION :
Resizes the offscreen pbuffer used for rendering with windowless mode.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_gtk_reallocate_pbuffer_size);
	if (buffer)
	{
		return_code = 0;
		/* We never bother to reduce the size */
		if (!buffer->offscreen_width || (buffer->offscreen_width < buffer->width)
			|| !buffer->offscreen_height || (buffer->offscreen_height < buffer->height))
		{
			/* 256x256 is the current minimum, also allocate powers of two just to be
				conservative with graphics card drivers */
			int required_width = 256;
			int required_height = 256;
			while (required_width < buffer->width)
			{
				required_width *= 2;
			}
			while (required_height < buffer->height)
			{
				required_height *= 2;
			}

#if defined (GL_EXT_framebuffer_object)
			if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
			{

			GenRenderbuffersEXT(1, &buffer->framebuffer_object);

	        // Enable render-to-texture
	        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buffer->framebuffer_object);

	        // Set up color_tex and depth_rb for render-to-texture
	        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
	                                  GL_COLOR_ATTACHMENT0_EXT,
	                                  GL_TEXTURE_2D, color_tex, 0);
	        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
	                                     GL_DEPTH_ATTACHMENT_EXT,
	                                     GL_RENDERBUFFER_EXT, depth_rb);

	        // Check framebuffer completeness at the end of initialization.
	        CHECK_FRAMEBUFFER_STATUS();

	        <draw to the texture and renderbuffer>

	        // Re-enable rendering to the window
	        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);



					/* Should be selecting with our standard rules and minimums here */
					int colour_bits = 32;
					int alpha_bits = 8;
					int depth_bits = 24;
					GLint pixel_format;
					unsigned int number_of_formats;

					const int pbuffer_attributes[]=
						{WGL_DRAW_TO_PBUFFER_ARB, 1,
						 WGL_COLOR_BITS_ARB, colour_bits,
						 WGL_ALPHA_BITS_ARB, alpha_bits,
						 WGL_DEPTH_BITS_ARB, depth_bits,
						 0};
					const float float_pbuffer_attributes[]={
						0};

#if defined (DEBUG)
					printf("Trying pbuffer\n");
#endif /* defined (DEBUG) */

					/* Only get the first valid format */
					if(wglChoosePixelFormatARB(buffer->hidden_graphics_buffer->hDC,
							pbuffer_attributes, float_pbuffer_attributes, 1,
							&pixel_format, &number_of_formats))
					{
						const int pbuffer_attrib[] = {0};

						if (buffer->pbuffer=wglCreatePbufferARB(
								 buffer->hidden_graphics_buffer->hDC, pixel_format,
								 required_width, required_height, pbuffer_attrib))
						{
							if (buffer->hDC = wglGetPbufferDCARB(buffer->pbuffer))
							{
								buffer->type = GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE;
								buffer->pixel_format = pixel_format;
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
					}
					else
					{
						return_code = 0;
					}
				}
				else
				{
					/* Try non accelerated bitmap OpenGL instead */
					return_code = 1;
				}
			}
#endif /* defined (WGL_ARB_pixel_format) && (WGL_ARB_pbuffer) */
			/* In either case we need a device independent bitmap matching the
				onscreen hdc.  Either for copying the pbuffer pixels or for rendering
				directly if we cannot get a pbuffer. */
			if (buffer->device_independent_bitmap)
			{
				DeleteObject(buffer->device_independent_bitmap);
			}
			if (buffer->device_independent_bitmap_hdc)
			{
				DeleteDC(buffer->device_independent_bitmap_hdc);
			}
			{
				BITMAPINFO bmi;

				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = required_width;
				bmi.bmiHeader.biHeight= required_height;
				bmi.bmiHeader.biPlanes = 1;
				if (GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffer->buffering_mode)
				{
					bmi.bmiHeader.biBitCount = 32;
				}
				else
				{
					bmi.bmiHeader.biBitCount = 24;
				}
				if (GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE != buffer->type)
				{
					/* If the onscreen size is 32 then the setpixelformat seems to
						want this even if rendering without alpha */
					int onscreen_bits = GetDeviceCaps(onscreen_hdc, BITSPIXEL);
					if (bmi.bmiHeader.biBitCount < onscreen_bits)
					{
						bmi.bmiHeader.biBitCount = onscreen_bits;
					}
				}

#if defined (DEBUG)
				printf("Bitmap bit count %d\n", bmi.bmiHeader.biBitCount);
#endif /* defined (DEBUG) */

				bmi.bmiHeader.biCompression = BI_RGB;
				bmi.bmiHeader.biSizeImage = 0;
				bmi.bmiHeader.biXPelsPerMeter = 0;
				bmi.bmiHeader.biYPelsPerMeter = 0;
				bmi.bmiHeader.biClrUsed = 0;
				bmi.bmiHeader.biClrImportant = 0;

				buffer->device_independent_bitmap_hdc = CreateCompatibleDC(onscreen_hdc);

				buffer->device_independent_bitmap =
					CreateDIBSection(onscreen_hdc,
						&bmi,
						DIB_RGB_COLORS,
						(void **)&buffer->device_independent_bitmap_pixels,
						0,
						0);
				SelectObject(buffer->device_independent_bitmap_hdc,
					buffer->device_independent_bitmap);

#if defined (DEBUG)
				printf ("Made dib\n");
#endif /* defined (DEBUG) */

				return_code = 1;

				if (buffer->type != GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE)
				{
					buffer->type = GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE;
					/* We use the bitmap directly as the OpenGL rendering surface */
					buffer->hDC = buffer->device_independent_bitmap_hdc;

					PIXELFORMATDESCRIPTOR pfd;
					SetPixelFormat( buffer->hDC, buffer->pixel_format, &pfd );
					buffer->hRC = wglCreateContext( buffer->hDC );
					if(!wglMakeCurrent(buffer->hDC,buffer->hRC))
					{
						display_message(ERROR_MESSAGE,"Graphics_buffer_gtk_reallocate_pbuffer_size.  "
							"Bitmap make current failed");
						return_code = 0;
					}
				}

			}
			if (return_code)
			{
				buffer->offscreen_width = required_width;
				buffer->offscreen_height = required_height;
                                buffer->offscreen_render_required = 1;
			}
		}
		else
		{
			/* Already large enough so nothing to do */
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_gtk_reallocate_pbuffer_size.  "
			"Missing graphics_buffer parameter.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_gtk_reallocate_pbuffer_size */
#endif /* defined (GTK_USER_INTERFACE) */
#endif /* defined (ENABLE_GTK_PBUFFER_RENDERING) */

#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
struct Graphics_buffer *create_Graphics_buffer_gtkgl(
	struct Graphics_buffer_package *graphics_buffer_package,
	GtkContainer *parent,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 2 June 2004

DESCRIPTION :
==============================================================================*/
{
#define MAX_GL_ATTRIBUTES (50)
	GtkGLArea *share;
	int accumulation_colour_size, attribute_list[MAX_GL_ATTRIBUTES], *attribute_ptr;
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_gtkglarea);
	if (gdk_gl_query() == TRUE)
	{
		if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
		{
			if (parent)
			{
				attribute_ptr = attribute_list;

				*attribute_ptr = GDK_GL_RGBA;
				attribute_ptr++;
				if (buffering_mode == GRAPHICS_BUFFER_DOUBLE_BUFFERING)
				{
					*attribute_ptr = GDK_GL_DOUBLEBUFFER;
					attribute_ptr++;
					buffer->buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
				}
				else
				{
					/* GRAPHICS_BUFFER_ANY_BUFFERING_MODE so don't specify it */
					buffer->buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
				}
				if (stereo_mode == GRAPHICS_BUFFER_STEREO)
				{
					*attribute_ptr = GDK_GL_STEREO;
					attribute_ptr++;
					buffer->stereo_mode = GRAPHICS_BUFFER_STEREO;
				}
				else
				{
					/* GRAPHICS_BUFFER_ANY_STEREO_MODE so don't specify it */
					buffer->stereo_mode = GRAPHICS_BUFFER_MONO;
				}
				if (minimum_colour_buffer_depth)
				{
					*attribute_ptr = GDK_GL_BUFFER_SIZE;
					attribute_ptr++;
					*attribute_ptr = minimum_colour_buffer_depth;
					attribute_ptr++;
				}
				if (minimum_depth_buffer_depth)
				{
					*attribute_ptr = GDK_GL_DEPTH_SIZE;
					attribute_ptr++;
					*attribute_ptr = minimum_depth_buffer_depth;
					attribute_ptr++;
				}
				if (minimum_accumulation_buffer_depth)
				{
					accumulation_colour_size = minimum_accumulation_buffer_depth / 4;
					*attribute_ptr = GDK_GL_ACCUM_RED_SIZE;
					attribute_ptr++;
					*attribute_ptr = accumulation_colour_size;
					attribute_ptr++;
					*attribute_ptr = GDK_GL_ACCUM_GREEN_SIZE;
					attribute_ptr++;
					*attribute_ptr = accumulation_colour_size;
					attribute_ptr++;
					*attribute_ptr = GDK_GL_ACCUM_BLUE_SIZE;
					attribute_ptr++;
					*attribute_ptr = accumulation_colour_size;
					attribute_ptr++;
					*attribute_ptr = GDK_GL_ACCUM_ALPHA_SIZE;
					attribute_ptr++;
					*attribute_ptr = accumulation_colour_size;
					attribute_ptr++;
				}
				*attribute_ptr = GDK_GL_NONE;
				attribute_ptr++;
				if (graphics_buffer_package->share_glarea)
				{
					share = GTK_GL_AREA(graphics_buffer_package->share_glarea);
				}
				else
				{
					share = (GtkGLArea *)NULL;
				}
				if (buffer->glarea = gtk_gl_area_share_new(attribute_list, share))
				{
					if (!graphics_buffer_package->share_glarea)
					{
						graphics_buffer_package->share_glarea = buffer->glarea;
					}
					buffer->type = GRAPHICS_BUFFER_GTKGLAREA_TYPE;
					gtk_widget_set_events(GTK_WIDGET(buffer->glarea),
						GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_MASK|GDK_POINTER_MOTION_HINT_MASK|
						GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
						GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);
#if GTK_MAJOR_VERSION >= 2
					g_signal_connect(G_OBJECT(buffer->glarea), "realize",
						G_CALLBACK(Graphics_buffer_gtkglarea_initialise_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "size-allocate",
						G_CALLBACK(Graphics_buffer_gtkglarea_resize_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "expose-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_expose_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "button-press-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_button_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "button-release-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_button_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "key-press-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_key_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "key-release-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_key_callback),
						(gpointer)buffer);
					g_signal_connect(G_OBJECT(buffer->glarea), "motion-notify-event",
						G_CALLBACK(Graphics_buffer_gtkglarea_motion_notify_callback),
						(gpointer)buffer);
#else /* GTK_MAJOR_VERSION >= 2 */
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "realize",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_initialise_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "size-allocate",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_resize_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "expose-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_expose_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "button-press-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_button_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "button-release-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_button_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "key-press-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_key_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "key-release-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_key_callback),
						(gpointer)buffer);
					gtk_signal_connect(GTK_OBJECT(buffer->glarea), "motion-notify-event",
						GTK_SIGNAL_FUNC(Graphics_buffer_gtkglarea_motion_notify_callback),
						(gpointer)buffer);
#endif /* GTK_MAJOR_VERSION >= 2 */
					gtk_container_add(parent, GTK_WIDGET(buffer->glarea));
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkglarea.  "
						"Unable to create gtk gl area.");
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
#if defined (ENABLE_GTK_PBUFFER_RENDERING)
			else if ((GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_COPY == buffering_mode) ||
				(GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffering_mode))
			{
				buffer->minimum_colour_buffer_depth = minimum_colour_buffer_depth;
				buffer->minimum_depth_buffer_depth = minimum_depth_buffer_depth;
				buffer->minimum_accumulation_buffer_depth = minimum_accumulation_buffer_depth;

				buffer->buffering_mode = buffering_mode;

				/* If we don't use the pbuffer below we need a SINGLE BUFFERING OpenGL
					when we search pixel formats */
				buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
				if (Graphics_buffer_gtk_reallocate_offscreen_size(buffer))
				{
					if (GRAPHICS_BUFFER_GTK_COPY_PBUFFER_TYPE == buffer->type)
					{
	#if defined (DEBUG)
						printf("Using pbuffer\n");
	#endif /* defined (DEBUG) */
						use_pbuffer = 1;
					}
					else
					{
						/* Try to get some alpha planes although I think they don't work anyway */
						minimum_alpha_buffer_depth = 1;
					}
				}
				else
				{
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}

			}
#endif /* defined (ENABLE_GTK_PBUFFER_RENDERING) */
			else
			{
				display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkglarea.  "
					"Missing gtk container and not an render and copy display mode.");
				buffer = (struct Graphics_buffer *)NULL;

			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkglarea.  "
				"Unable to create generic Graphics_buffer.");
			buffer = (struct Graphics_buffer *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkglarea.  "
			"Gdk Open GL not supported.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_gtkglarea */
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (GTK_USER_INTERFACE)
#if ! defined (GTK_USE_GTKGLAREA)
struct Graphics_buffer *create_Graphics_buffer_gtkgl(
	struct Graphics_buffer_package *graphics_buffer_package,
	GtkContainer *parent,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 2 June 2004

DESCRIPTION :
==============================================================================*/
{
#define MAX_GL_ATTRIBUTES (50)
	GdkGLConfig *glconfig;
	GtkWidget *glarea;
	int accumulation_colour_size, attribute_list[MAX_GL_ATTRIBUTES], *attribute_ptr,
		selection_level;
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_gtkglarea);

	if (gdk_gl_query_extension() == TRUE)
	{
		if (parent)
		{
			if (glarea = gtk_drawing_area_new())
			{
				if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
				{
					glconfig = (GdkGLConfig *)NULL;
					selection_level = 2;
					while (!glconfig && (selection_level > 0))
					{
						attribute_ptr = attribute_list;

						*attribute_ptr = GDK_GL_RGBA;
						attribute_ptr++;
						if (buffering_mode == GRAPHICS_BUFFER_DOUBLE_BUFFERING)
						{
							*attribute_ptr = GDK_GL_DOUBLEBUFFER;
							attribute_ptr++;
						}
						/* else GRAPHICS_BUFFER_ANY_BUFFERING_MODE so don't specify it */
						if (stereo_mode == GRAPHICS_BUFFER_STEREO)
						{
							*attribute_ptr = GDK_GL_STEREO;
							attribute_ptr++;
						}
						/* else GRAPHICS_BUFFER_ANY_STEREO_MODE so don't specify it */
						if (minimum_colour_buffer_depth)
						{
							*attribute_ptr = GDK_GL_BUFFER_SIZE;
							attribute_ptr++;
							*attribute_ptr = minimum_colour_buffer_depth;
							attribute_ptr++;
						}
						if (selection_level > 1)
						{
							*attribute_ptr = GDK_GL_ALPHA_SIZE;
							attribute_ptr++;
							*attribute_ptr = 1;
							attribute_ptr++;
						}
						if (minimum_depth_buffer_depth)
						{
							*attribute_ptr = GDK_GL_DEPTH_SIZE;
							attribute_ptr++;
							*attribute_ptr = minimum_depth_buffer_depth;
							attribute_ptr++;
						}
						if (minimum_accumulation_buffer_depth)
						{
							accumulation_colour_size = minimum_accumulation_buffer_depth / 4;
							*attribute_ptr = GDK_GL_ACCUM_RED_SIZE;
							attribute_ptr++;
							*attribute_ptr = accumulation_colour_size;
							attribute_ptr++;
							*attribute_ptr = GDK_GL_ACCUM_GREEN_SIZE;
							attribute_ptr++;
							*attribute_ptr = accumulation_colour_size;
							attribute_ptr++;
							*attribute_ptr = GDK_GL_ACCUM_BLUE_SIZE;
							attribute_ptr++;
							*attribute_ptr = accumulation_colour_size;
							attribute_ptr++;
							*attribute_ptr = GDK_GL_ACCUM_ALPHA_SIZE;
							attribute_ptr++;
							*attribute_ptr = accumulation_colour_size;
							attribute_ptr++;
						}
						*attribute_ptr = GDK_GL_ATTRIB_LIST_NONE;
						attribute_ptr++;
						glconfig = gdk_gl_config_new(attribute_list);

						selection_level--;
					}
					if (glconfig &&
						gtk_widget_set_gl_capability(glarea, glconfig,
							graphics_buffer_package->share_glcontext,
							TRUE, GDK_GL_RGBA_TYPE))
					{
						buffer->glarea = glarea;
						buffer->glconfig = gtk_widget_get_gl_config(glarea);
						buffer->type = GRAPHICS_BUFFER_GTKGLEXT_TYPE;
						gtk_widget_set_events(buffer->glarea,
							GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_MASK|GDK_POINTER_MOTION_HINT_MASK|
							GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
							GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);
						buffer->initialise_handler_id =
							g_signal_connect(G_OBJECT(buffer->glarea), "realize",
							G_CALLBACK(Graphics_buffer_gtkglarea_initialise_callback),
							(gpointer)buffer);
					buffer->resize_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "size-allocate",
							G_CALLBACK(Graphics_buffer_gtkglarea_resize_callback),
							(gpointer)buffer);
					buffer->expose_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "expose-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_expose_callback),
							(gpointer)buffer);
					buffer->button_press_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "button-press-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_button_callback),
							(gpointer)buffer);
					buffer->button_release_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "button-release-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_button_callback),
							(gpointer)buffer);
					buffer->key_press_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "key-press-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_key_callback),
							(gpointer)buffer);
					buffer->key_release_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "key-release-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_key_callback),
							(gpointer)buffer);
					buffer->motion_handler_id =
						g_signal_connect(G_OBJECT(buffer->glarea), "motion-notify-event",
							G_CALLBACK(Graphics_buffer_gtkglarea_motion_notify_callback),
							(gpointer)buffer);
						gtk_container_add(parent, buffer->glarea);
					}
					else
					{
						display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkgl.  "
							"Unable to add opengl capability.");
						DESTROY(Graphics_buffer)(&buffer);
						buffer = (struct Graphics_buffer *)NULL;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkgl.  "
						"Unable to create generic Graphics_buffer.");
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkgl.  "
					"Could not create drawing area widget.");
				buffer = (struct Graphics_buffer *)NULL;
			}
		}
#if defined (ENABLE_GTK_PBUFFER_RENDERING)
		else if ((GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_COPY == buffering_mode) ||
			(GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffering_mode))
		{
			buffer->minimum_colour_buffer_depth = minimum_colour_buffer_depth;
			buffer->minimum_depth_buffer_depth = minimum_depth_buffer_depth;
			buffer->minimum_accumulation_buffer_depth = minimum_accumulation_buffer_depth;

			buffer->buffering_mode = buffering_mode;

			/* If we don't use the pbuffer below we need a SINGLE BUFFERING OpenGL
				when we search pixel formats */
			buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
			if (Graphics_buffer_gtk_reallocate_offscreen_size(buffer))
			{
				if (GRAPHICS_BUFFER_GTK_COPY_PBUFFER_TYPE == buffer->type)
				{
#if defined (DEBUG)
					printf("Using pbuffer\n");
#endif /* defined (DEBUG) */
					use_pbuffer = 1;
				}
				else
				{
					/* Try to get some alpha planes although I think they don't work anyway */
					minimum_alpha_buffer_depth = 1;
				}
			}
			else
			{
				DESTROY(Graphics_buffer)(&buffer);
				buffer = (struct Graphics_buffer *)NULL;
			}

		}
#endif /* defined (ENABLE_GTK_PBUFFER_RENDERING) */
		else
		{
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkglarea.  "
				"Missing gtk container and not an render and copy display mode.");
			buffer = (struct Graphics_buffer *)NULL;

		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_gtkgl.  "
			"Gdk Open GL EXT not supported.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_gtkgl */
#endif /* ! defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
static int Graphics_buffer_win32_reallocate_offscreen_size(
	struct Graphics_buffer *buffer, HDC onscreen_hdc)
/*******************************************************************************
LAST MODIFIED : 10 March 2008

DESCRIPTION :
Resizes the offscreen pbuffer used for rendering with windowless mode.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_win32_reallocate_pbuffer_size);
	if (buffer)
	{
		return_code = 0;
		/* We never bother to reduce the size */
		if (!buffer->offscreen_width || (buffer->offscreen_width < buffer->width)
			|| !buffer->offscreen_height || (buffer->offscreen_height < buffer->height))
		{
			/* 256x256 is the current minimum, also allocate powers of two just to be
				conservative with graphics card drivers */
			int required_width = 256;
			int required_height = 256;
			while (required_width < buffer->width)
			{
				required_width *= 2;
			}
			while (required_height < buffer->height)
			{
				required_height *= 2;
			}

#if defined (WGL_ARB_pixel_format) && (WGL_ARB_pbuffer)
			{
				/* Must create and select the offscreen buffer before testing extensions
				 otherwise default windows implementation will just respond unavailable. */
				if (!buffer->hidden_graphics_buffer)
				{
					BOOL win32_return_code;
					static char *class_name="Hidden window";
					WNDCLASS class_information;

					/* check if the class is registered */
					win32_return_code=GetClassInfo(User_interface_get_instance(
																 buffer->package->user_interface),
						class_name,&class_information);

					if (win32_return_code==FALSE)
					{
						/* register class */
						class_information.cbClsExtra=0;
						class_information.cbWndExtra=sizeof(struct Graphics_window *);
						class_information.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
						class_information.hCursor=LoadCursor(NULL,IDC_ARROW);
						class_information.hIcon=LoadIcon(
							User_interface_get_instance(buffer->package->user_interface),
							"Command_window_icon");
						class_information.hInstance=User_interface_get_instance(
							buffer->package->user_interface);
						class_information.lpfnWndProc=DefWindowProc;
						class_information.lpszClassName=class_name;
						class_information.style=CS_OWNDC;
						class_information.lpszMenuName=NULL;
						if (RegisterClass(&class_information))
						{
							win32_return_code=TRUE;
						}
					}

					/* Need to get an accelerated rendering context before
						we can try these functions!!! */
					buffer->hidden_accelerated_window =CreateWindow(class_name, "Hidden",
						WS_CAPTION | WS_POPUPWINDOW | WS_SIZEBOX,
						0, 0, 100, 100,
						NULL, NULL, User_interface_get_instance(buffer->package->user_interface), NULL);

					buffer->hidden_graphics_buffer = create_Graphics_buffer_win32(
						buffer->package,
						buffer->hidden_accelerated_window, (HDC)NULL,
						GRAPHICS_BUFFER_ANY_BUFFERING_MODE, GRAPHICS_BUFFER_ANY_STEREO_MODE,
						buffer->minimum_colour_buffer_depth, buffer->minimum_depth_buffer_depth,
						buffer->minimum_accumulation_buffer_depth);
				}

				Graphics_buffer_make_current(buffer->hidden_graphics_buffer);

#if defined (DEBUG)
				printf("Made hidden window current\n");
#endif /* defined (DEBUG) */

				if (Window_system_extensions_check_wgl_extension(WGL_ARB_pixel_format) &&
					Window_system_extensions_check_wgl_extension(WGL_ARB_pbuffer))
				{
					/* Release the previous pbuffer */
					if (buffer->pbuffer && buffer->hDC)
					{
						wglReleasePbufferDCARB(buffer->pbuffer, buffer->hDC);
					}
					if (buffer->pbuffer)
					{
						wglDestroyPbufferARB(buffer->pbuffer);
					}

					/* Should be selecting with our standard rules and minimums here */
					int colour_bits = 32;
					int alpha_bits = 8;
					int depth_bits = 24;
					GLint pixel_format;
					unsigned int number_of_formats;

					const int pbuffer_attributes[]=
						{WGL_DRAW_TO_PBUFFER_ARB, 1,
						 WGL_COLOR_BITS_ARB, colour_bits,
						 WGL_ALPHA_BITS_ARB, alpha_bits,
						 WGL_DEPTH_BITS_ARB, depth_bits,
						 0};
					const float float_pbuffer_attributes[]={
						0};

#if defined (DEBUG)
					printf("Trying pbuffer\n");
#endif /* defined (DEBUG) */

					/* Only get the first valid format */
					if(wglChoosePixelFormatARB(buffer->hidden_graphics_buffer->hDC,
							pbuffer_attributes, float_pbuffer_attributes, 1,
							&pixel_format, &number_of_formats))
					{
						const int pbuffer_attrib[] = {0};

						if (buffer->pbuffer=wglCreatePbufferARB(
								 buffer->hidden_graphics_buffer->hDC, pixel_format,
								 required_width, required_height, pbuffer_attrib))
						{
							if (buffer->hDC = wglGetPbufferDCARB(buffer->pbuffer))
							{
								buffer->type = GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE;
								buffer->pixel_format = pixel_format;
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
					}
					else
					{
						return_code = 0;
					}
				}
				else
				{
					/* Try non accelerated bitmap OpenGL instead */
					return_code = 1;
				}
			}
#endif /* defined (WGL_ARB_pixel_format) && (WGL_ARB_pbuffer) */
			/* In either case we need a device independent bitmap matching the
				onscreen hdc.  Either for copying the pbuffer pixels or for rendering
				directly if we cannot get a pbuffer. */
			if (buffer->device_independent_bitmap)
			{
				DeleteObject(buffer->device_independent_bitmap);
			}
			if (buffer->device_independent_bitmap_hdc)
			{
				DeleteDC(buffer->device_independent_bitmap_hdc);
			}
			{
				BITMAPINFO bmi;

				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = required_width;
				bmi.bmiHeader.biHeight= required_height;
				bmi.bmiHeader.biPlanes = 1;
				if (GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffer->buffering_mode)
				{
					bmi.bmiHeader.biBitCount = 32;
				}
				else
				{
					bmi.bmiHeader.biBitCount = 24;
				}
				if (GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE != buffer->type)
				{
					/* If the onscreen size is 32 then the setpixelformat seems to
						want this even if rendering without alpha */
					int onscreen_bits = GetDeviceCaps(onscreen_hdc, BITSPIXEL);
					if (bmi.bmiHeader.biBitCount < onscreen_bits)
					{
						bmi.bmiHeader.biBitCount = onscreen_bits;
					}
				}

#if defined (DEBUG)
				printf("Bitmap bit count %d %d\n", bmi.bmiHeader.biBitCount,
				       GetDeviceCaps(onscreen_hdc, BITSPIXEL));
#endif /* defined (DEBUG) */

				bmi.bmiHeader.biCompression = BI_RGB;
				bmi.bmiHeader.biSizeImage = 0;
				bmi.bmiHeader.biXPelsPerMeter = 0;
				bmi.bmiHeader.biYPelsPerMeter = 0;
				bmi.bmiHeader.biClrUsed = 0;
				bmi.bmiHeader.biClrImportant = 0;

				buffer->device_independent_bitmap_hdc = CreateCompatibleDC(onscreen_hdc);
				if (!buffer->device_independent_bitmap_hdc)
				{
					buffer->device_independent_bitmap_hdc = CreateCompatibleDC(NULL);
				}
				buffer->device_independent_bitmap =
					CreateDIBSection(buffer->device_independent_bitmap_hdc,
						&bmi,
						DIB_RGB_COLORS,
						(void **)&buffer->device_independent_bitmap_pixels,
						0,
						0);
				SelectObject(buffer->device_independent_bitmap_hdc,
					buffer->device_independent_bitmap);

#if defined (DEBUG)
				printf("Made dib %p %p %d\n",
				       buffer->device_independent_bitmap_hdc,
				       buffer->device_independent_bitmap, buffer->type);
#endif /* defined (DEBUG) */

				return_code = 1;

				if (buffer->type != GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE)
				{
					buffer->type = GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE;
					/* We use the bitmap directly as the OpenGL rendering surface */
					buffer->hDC = buffer->device_independent_bitmap_hdc;
#if defined (DEBUG)
					printf("buffer->hDC %p\n", buffer->hDC);
#endif /* defined (DEBUG) */

					PIXELFORMATDESCRIPTOR pfd;
					SetPixelFormat( buffer->hDC, buffer->pixel_format, &pfd );
					buffer->hRC = wglCreateContext( buffer->hDC );
					if(!wglMakeCurrent(buffer->hDC,buffer->hRC))
					{
						display_message(ERROR_MESSAGE,"Graphics_buffer_win32_reallocate_pbuffer_size.  "
							"Bitmap make current failed");
						return_code = 0;
					}
				}

			}
			if (return_code)
			{
				buffer->offscreen_width = required_width;
				buffer->offscreen_height = required_height;
                                buffer->offscreen_render_required = 1;
			}
		}
		else
		{
			/* Already large enough so nothing to do */
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_win32_reallocate_pbuffer_size.  "
			"Missing graphics_buffer parameter.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_win32_reallocate_pbuffer_size */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
struct Graphics_buffer *create_Graphics_buffer_win32(
	struct Graphics_buffer_package *graphics_buffer_package,
	HWND hWnd, HDC hDC,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 9 August 2004

DESCRIPTION :
Creates a Graphics buffer on the specified <hWnd> window handle.
If the <hDC> is specified it is used to render.
Alternatively if <hWnd> is NULL and <hDC> is specified then no window functions
are performed but the graphics window will render into the supplied device context.
==============================================================================*/
{
	PIXELFORMATDESCRIPTOR pfd;
	int i, selection_level, use_pbuffer;
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_win32);
	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
		/* Hardcode this for the meantime so as not to change function interface */
		int minimum_alpha_buffer_depth = 8;

		use_pbuffer = 0;

		buffer->type = GRAPHICS_BUFFER_WIN32_TYPE;

		if (hWnd)
		{
			buffer->hWnd = hWnd;

			SetWindowLongPtr(hWnd, GWL_WNDPROC,
				(LONG)Graphics_buffer_callback_proc);
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)buffer);

			if (hDC)
			{
				buffer->hDC = hDC;
			}
			else
			{
				buffer->hDC=GetDC(hWnd);
			}
		}
		else if ((GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_COPY == buffering_mode) ||
			(GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffering_mode))
		{
			buffer->minimum_colour_buffer_depth = minimum_colour_buffer_depth;
			buffer->minimum_depth_buffer_depth = minimum_depth_buffer_depth;
			buffer->minimum_accumulation_buffer_depth = minimum_accumulation_buffer_depth;

			buffer->buffering_mode = buffering_mode;

			/* If we don't use the pbuffer below we need a SINGLE BUFFERING OpenGL
				when we search pixel formats */
			buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
			if (Graphics_buffer_win32_reallocate_offscreen_size(buffer, hDC))
			{
				if (GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE == buffer->type)
				{
#if defined (DEBUG)
					printf("Using pbuffer\n");
#endif /* defined (DEBUG) */
					use_pbuffer = 1;
				}
				else
				{
					/* Try to get some alpha planes although I think they don't work anyway */
					minimum_alpha_buffer_depth = 1;
				}
			}
			else
			{
				DESTROY(Graphics_buffer)(&buffer);
				buffer = (struct Graphics_buffer *)NULL;
			}
		}
#if defined (DEBUG)
		printf ("create_Graphics_buffer_win32 %p %p %p\n",
			hWnd, hDC, buffer->hDC);
#endif /* defined (DEBUG) */

		if (buffer && !use_pbuffer)
		{
			/* Find a valid OpenGL context for the hdc */
			if (buffer->hDC)
			{
				int number_of_formats = DescribePixelFormat(buffer->hDC, 1, 0, NULL);
				int best_selection_level = 0;
				int format = 0;

				for (i = 0 ; i < number_of_formats ; i++)
				{
					/* set the pixel format for the DC */
					ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR ) );
					pfd.nSize = sizeof( pfd );
					pfd.nVersion = 1;

					DescribePixelFormat(buffer->hDC, i, pfd.nSize, &pfd);

					if ((pfd.dwFlags & PFD_SUPPORT_OPENGL)
						&& (pfd.iPixelType == PFD_TYPE_RGBA))
					{
						selection_level = 10;
					}
					else
					{
						/* No good */
						selection_level = 0;
					}

					if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
					{
						/* Windows driver accelerated OpenGL */
						selection_level -= 2;
					}
					else
					{
						if (pfd.dwFlags & PFD_GENERIC_FORMAT)
						{
							/* Software OpenGL */
							selection_level -= 4;
						}
						else
						{
							/* Independent driver accelerated OpenGL */
							/* Best */
						}
					}

					if (!hWnd)
					{
						if (!(pfd.dwFlags & PFD_DRAW_TO_BITMAP))
						{
							selection_level = 0;
						}
					}
					else
					{
						if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW))
						{
							selection_level = 0;
						}
					}

					if (pfd.cColorBits >= 24)
					{
						/* Best */
					}
					else if (pfd.cColorBits >= minimum_colour_buffer_depth)
					{
						/* Satisfactory */
						selection_level--;
					}
					else
					{
						/* Poor */
						selection_level-=2;
					}

					if (pfd.cDepthBits >= 24)
					{
						/* Best */
					}
					else if (pfd.cDepthBits >= minimum_depth_buffer_depth)
					{
						/* Satisfactory */
						selection_level--;
					}
					else
					{
						/* Poor */
						selection_level-=2;
					}

					if (pfd.cAlphaBits >= minimum_alpha_buffer_depth)
					{
						/* Best */
					}
					else if (pfd.cAlphaBits > 0)
					{
						/* Satisfactory */
						selection_level--;
					}
					else
					{
						/* Poor */
						selection_level-=2;
					}

					if (pfd.cAccumBits >= minimum_accumulation_buffer_depth)
					{
						/* Best */
					}
					else if (pfd.cAccumBits > 0)
					{
						/* Satisfactory */
						selection_level--;
					}
					else
					{
						/* Poor */
						selection_level-=2;
					}

					switch (buffering_mode)
					{
						case GRAPHICS_BUFFER_SINGLE_BUFFERING:
						{
							if (pfd.dwFlags & PFD_DOUBLEBUFFER)
							{
								selection_level = 0;
							}
						} break;
						case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
						{
							if (!(pfd.dwFlags & PFD_DOUBLEBUFFER))
							{
								selection_level = 0;
							}
						} break;
						default: /* GRAPHICS_BUFFER_ANY_BUFFERING_MODE: */
						{
						} break;
					}
					switch (stereo_mode)
					{
						case GRAPHICS_BUFFER_MONO:
						{
							if (pfd.dwFlags & PFD_STEREO)
							{
								selection_level = 0;
							}
						} break;
						case GRAPHICS_BUFFER_STEREO:
						{
							if (!(pfd.dwFlags & PFD_STEREO))
							{
								selection_level = 0;
							}
						} break;
						default: /* GRAPHICS_BUFFER_ANY_STEREO_MODE: */
						{
						} break;
					}

					if (selection_level > best_selection_level)
					{
						format = i;
						best_selection_level = selection_level;
					}
				}
				if (!format)
				{
				  /* Try the automatic chooser */
				  memset(&pfd,0, sizeof(PIXELFORMATDESCRIPTOR)) ;
				  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); 
				  pfd.nVersion = 1 ;
				  pfd.dwFlags =  PFD_SUPPORT_OPENGL ;
				  pfd.iPixelType = PFD_TYPE_RGBA ;
				  pfd.cColorBits = 24 ;
				  pfd.cDepthBits = 32 ;
				  pfd.iLayerType = PFD_MAIN_PLANE ;				  
				  // Choose the pixel format.
				  format = ChoosePixelFormat(buffer->hDC, &pfd);
				  best_selection_level = -1;  /* Set a value to show that it was selected by ChoosePixelFormat */
				}

				if (format)
				{
#if defined (DEBUG)
					printf ("Trying format %d, selection level %d\n",
						format, best_selection_level);
#endif /* defined (DEBUG) */
					if(SetPixelFormat( buffer->hDC, format, &pfd ))
					{
#if defined (DEBUG)
						printf ("SetPixelFormat %d success\n", format);
#endif /* defined (DEBUG) */
						buffer->pixel_format = format;
					}
					else
					{
						DWORD error = GetLastError();
						display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
							"Unable to set pixel format. Microsoft error code: %d", error);
						DESTROY(Graphics_buffer)(&buffer);
						buffer = (struct Graphics_buffer *)NULL;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
						"No valid pixel formats found.");
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
					"Unable to get drawing context.");
				DESTROY(Graphics_buffer)(&buffer);
				buffer = (struct Graphics_buffer *)NULL;
			}
		}

		if (buffer)
		{
			/* A work around for Intel GMA 900 cards on windows where compilation
				of textures only seems to work on the first context of a
				share group.  On these cards only use a single
				graphics context for all graphics buffers.
				The vendor string will not be defined the first time around,
				although when using multiple instances in the same memory
				space (such as with zinc) it could be set for the first
				in a share group. */
			const char *vendor_string = (const char *)glGetString(GL_VENDOR);
			if (!graphics_buffer_package->wgl_shared_context ||
				!vendor_string || strcmp("Intel", vendor_string))
			{
				/* Default normal implementation,
					different context for each buffer with shared lists */
				/* create and enable the render context (RC) */
				if(buffer->hRC = wglCreateContext( buffer->hDC ))
				{
					if (graphics_buffer_package->wgl_shared_context)
					{
						wglShareLists(graphics_buffer_package->wgl_shared_context,
							buffer->hRC);
					}
					else
					{
						graphics_buffer_package->wgl_shared_context = buffer->hRC;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
						"Unable to create the render context.");
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
			else
			{
				/* Use a single context for all graphics buffers */
				buffer->hRC = graphics_buffer_package->wgl_shared_context;
			}
			if (buffer)
			{
				if(wglMakeCurrent(buffer->hDC,buffer->hRC))
				{
#if defined (DEBUG)
					ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR ) );
					pfd.nSize = sizeof( pfd );
					pfd.nVersion = 1;

					DescribePixelFormat(buffer->hDC, buffer->pixel_format, pfd.nSize, &pfd);

					printf("Pixel format %d\n", buffer->pixel_format);
					const char *vendor_string = (const char *)glGetString(GL_VENDOR);
					printf("OpenGL vendor string %s\n", vendor_string);
					if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
					{
						/* Windows driver accelerated OpenGL */
						printf("Windows driver accelerated OpenGL\n");
					}
					else
					{
						if (pfd.dwFlags & PFD_GENERIC_FORMAT)
						{
							/* Windows software OpenGL */
							printf("Software OpenGL\n");
						}
						else
						{
							/* Independent driver accelerated OpenGL */
							printf("Independent driver accelerated OpenGL\n");
						}
					}
					printf("Colour depth %d\n",  pfd.cColorBits);
					printf("Z depth %d\n",  pfd.cDepthBits);
					printf("Alpha depth %d\n",  pfd.cAlphaBits);
					printf("Accumulation depth %d\n",  pfd.cAccumBits);
#endif /* defined (DEBUG) */
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
						"Unable enable the render context.");
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
		}
#if defined (DEBUG)
		fflush(stdout);
#endif /* defined (DEBUG) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_win32 */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE) && (defined (__CYGWIN__) || defined (__MINGW__)) \
	&& ((__W32API_MAJOR_VERSION < 3) || (__W32API_MAJOR_VERSION == 3) && (__W32API_MINOR_VERSION < 7))
/* This is a relatively recent addition (version 3.7 2006-04-07) to the free W32API headers
   so declaring here if the headers are too old,
   however the actual function and dll was available in windows 98 */
extern "C" {
	bob
  WINGDIAPI BOOL  WINAPI AlphaBlend(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
}
#endif /* defined (WIN32_USER_INTERFACE) && (defined (__CYGWIN__) || defined (__MINGW__)) */

#if defined (WIN32_USER_INTERFACE)
int Graphics_buffer_handle_windows_event(struct Graphics_buffer *buffer,
	UINT message_identifier,WPARAM first_message,LPARAM second_message)
/*******************************************************************************
LAST MODIFIED : 5 June 2007

DESCRIPTION:
Handle an external windows event.  Used to try and support windowless plugin
mode with zinc.  Requiring development.
==============================================================================*/
{
  int return_code;

  ENTER(Graphics_buffer_handle_windows_event);

  switch (message_identifier)
  {
	  case WM_CREATE:
	  {
		  return_code=1;
	  }
	  case WM_PAINT:
	  {
		  // We need to just paint what we are told otherwise we will mess up the
		  // compositing
		  HDC hdc = (HDC)first_message;
		  RECT * drc = (RECT *)second_message;

#if defined (DEBUG)
		  printf ("Graphics_buffer_handle_windows_event WMPAINT\nhdc %p left %ld right %ld top %ld bottom %ld\n",
			  hdc, drc->left, drc->right, drc->top, drc->bottom);
#endif /* defined (DEBUG) */

		  Graphics_buffer_win32_reallocate_offscreen_size(buffer, hdc);

                  if (buffer->offscreen_render_required)
		  {
		    //Graphics_buffer_expose_data expose_data;
		          CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
		  	          buffer->expose_callback_list, buffer, NULL);
			  buffer->offscreen_render_required = 0;
		  }

		  wglMakeCurrent( buffer->hDC, buffer->hRC );

		  {
 			  int x = drc->left;
			  int y = drc->top;

			  x -= buffer->x;
			  y -= buffer->y;

 			  if (x < buffer->x)
 			  {
 				  x = buffer->x;
 			  }
 			  if (y < buffer->y)
 			  {
 				  y = buffer->y;
 			  }

			  int right = drc->right;
			  int bottom = drc->bottom;

			  right -= buffer->x;
			  bottom -= buffer->y;

			  if (right > buffer->width + buffer->x)
			  {
				  right = buffer->width + buffer->x;
			  }
			  if (bottom > buffer->height + buffer->y)
			  {
				  bottom = buffer->height + buffer->y;
			  }

			  x = buffer->x;
			  y = buffer->y;
			  right = buffer->width + buffer->x;
			  bottom = buffer->height + buffer->y;

			  int width = right - x;
			  int height = bottom - y;

			  if ((width > 0) && (height > 0))
			  {
#if defined (DEBUG)
				  const char *vendor_string = (const char *)glGetString(GL_VENDOR);
				  printf ("Rendering with %s\n", vendor_string);
#endif /* defined (DEBUG) */

				  switch (buffer->type)
				  {
					  case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
					  {
						  if (GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffer->buffering_mode)
						  {
#if defined (DEBUG)
							  printf ("Going to blend %d %d %d %d (rgba %d %d %d %d)\n",
								  x - buffer->x, buffer->offscreen_height - height - y + buffer->y,
								  width, height,
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height - y + buffer->y)],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height - y + buffer->y) + 1],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height - y + buffer->y) + 2],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height - y + buffer->y) + 3]);
#endif /* defined (DEBUG) */

							  BLENDFUNCTION blendfunction =
								  {
									  AC_SRC_OVER,
									  0,
									  255,
									  AC_SRC_ALPHA
								  };

							  AlphaBlend(hdc, x, y, width, height,
								  buffer->hDC, x - buffer->x,
								  buffer->offscreen_height - height - y + buffer->y, width, height,
								  blendfunction);
						  }
						  else
						  {
							  BitBlt(hdc, x, y, width, height,
								  buffer->hDC, x - buffer->x,
								  buffer->offscreen_height - height - y + buffer->y,
								  SRCCOPY);
						  }
					  } break;
					  case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
					  {
						  glPixelStorei(GL_PACK_ROW_LENGTH, buffer->offscreen_width);
						  if (GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND == buffer->buffering_mode)
						  {
							  glReadPixels(x - buffer->x, buffer->height - height - y + buffer->y,
								  width, height, GL_BGRA,
								  GL_UNSIGNED_BYTE,
								  (unsigned char *)buffer->device_independent_bitmap_pixels +
								  4 * buffer->offscreen_width * (buffer->offscreen_height - height));

							  BLENDFUNCTION blendfunction =
								  {
									  AC_SRC_OVER,
									  0,
									  255,
									  AC_SRC_ALPHA
								  };

#if defined (DEBUG)
							  printf ("Copied pixels %d %d %d %d (rgba %d %d %d %d)\n",
								  x - buffer->x, y - buffer->y,
								  width, height,
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height)],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height) + 1],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height) + 2],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [4 * buffer->offscreen_width * (buffer->offscreen_height - height) + 3]);
							  printf ("Going to alpha blend %ld %d %d %ld %d %d\n",
								  drc->left, buffer->x, buffer->width,
								  drc->top, buffer->y, buffer->height );
#endif /* defined (DEBUG) */

							  AlphaBlend(hdc, x, y, width, height,
								  buffer->device_independent_bitmap_hdc, 0, 0, width, height,
								  blendfunction);
						  }
						  else
						  {
							  glReadPixels(x - buffer->x, buffer->height - height - y + buffer->y,
								  width, height, GL_BGR,
								  GL_UNSIGNED_BYTE,
								  (unsigned char *)buffer->device_independent_bitmap_pixels +
								  3 * buffer->offscreen_width * (buffer->offscreen_height - height));

#if defined (DEBUG)
							  //memset(buffer->device_independent_bitmap_pixels, 128,
							  //	 3 * buffer->offscreen_width * buffer->offscreen_height);

							  printf ("Copied pixels %d %d %d %d (rgb %d %d %d)\n",
								  x - buffer->x, y - buffer->y,
								  width, height,
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [3 * buffer->offscreen_width * (buffer->offscreen_height - height)],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [3 * buffer->offscreen_width * (buffer->offscreen_height - height) + 1],
								  ((unsigned char *)buffer->device_independent_bitmap_pixels)
								  [3 * buffer->offscreen_width * (buffer->offscreen_height - height) + 2]);
							  printf ("Going to blt %ld %d %d %ld %d %d\n",
								  drc->left, buffer->x, buffer->width,
								  drc->top, buffer->y, buffer->height );
#endif /* defined (DEBUG) */

							  BitBlt(hdc, x, y, width, height,
								  buffer->device_independent_bitmap_hdc, 0, 0,
								  SRCCOPY);
						  }
						  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
					  } break;
					  default:
					  {
						  display_message(ERROR_MESSAGE,"Graphics_buffer_handle_windows_event.  "
							  "Unsupported buffer type.");
						  return_code = 0;
					  }
				  }

#if defined (DEBUG)
				  printf ("BitBlt %d %d %d %d\n", x, y, width, height);
				  fflush(stdout);
#endif /* defined (DEBUG) */
			  }
#if defined (DEBUG)
			  else
			  {
				  printf ("Nothing to render %d %d, %d %d %d %d\n", width, height, x, y, right, bottom);
				  fflush(stdout);
			  }
#endif /* defined (DEBUG) */
		  }
		  return_code=1;
	  } break;
	  case WM_SIZING:
	  {
#if defined (DEBUG)
		  printf ("Graphics_buffer_handle_windows_event WM_SIZING\n");
#endif /* defined (DEBUG) */

		  CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			  buffer->resize_callback_list, buffer, NULL);
		  return_code=1;
	  } break;
	  case WM_MOUSEMOVE:
	  {
		  SetCursor(LoadCursor(NULL, IDC_ARROW));
	  } // No break, fall through
	  case WM_LBUTTONDOWN:
	  case WM_LBUTTONUP:
	  case WM_RBUTTONDOWN:
	  case WM_RBUTTONUP:
	  case WM_MBUTTONDOWN:
	  case WM_MBUTTONUP:
	  {
#if defined (DEBUG)
		  printf ("Graphics_buffer_handle_windows_event WM_MOUSEMOVE\n");
#endif /* defined (DEBUG) */

		  LPARAM offset_coordinates;

		  offset_coordinates = MAKELPARAM(
			  GET_X_LPARAM(second_message) - buffer->x,
			  GET_Y_LPARAM(second_message) - buffer->y);

		  return_code = Graphics_buffer_win32_button_callback(&message_identifier,
			  buffer, first_message, offset_coordinates);
	  } break;
	  case WM_SETCURSOR:
	  {
	    /* This message does not seem to propagate through to zinc so
	     setting cursor on WM_MOUSEMOVE above instead*/
#if defined (DEBUG)
		  printf ("Graphics_buffer_handle_windows_event WM_SETCURSOR\n");
#endif /* defined (DEBUG) */
	          SetCursor(LoadCursor(NULL, IDC_ARROW));
		  return_code=1;
	  } break;
	  default:
	  {
	  } break;
  }

  return (return_code);
} /* Graphics_buffer_handle_windows_event */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
int Graphics_buffer_win32_set_window_size(struct Graphics_buffer *buffer,
	int width, int height, int x, int y)
/*******************************************************************************
LAST MODIFIED : 14 September 2007

DESCRIPTION :
Sets the maximum extent of the graphics window within which individual paints
will be requested with handle_windows_event.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_win32_set_window_size);
	if (buffer)
	{
	        if ((buffer->width != width) || (buffer->height != height))
		{
		   buffer->offscreen_render_required = 1;
                }
		buffer->width = width;
		buffer->height = height;
		buffer->x = x;
		buffer->y = y;
#if defined (DEBUG)
		printf("Graphics_buffer_win32_set_window_size width %d height %d x %d y %d\n",
			width, height, x, y);
#endif /* defined (DEBUG) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"Cmiss_graphics_buffer_win32_set_window_size.  "
			"Missing graphics_buffer parameter.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_graphics_buffer_win32_set_window_size */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
static LRESULT CALLBACK Graphics_buffer_callback_proc(HWND window,
	UINT message_identifier,WPARAM first_message,LPARAM second_message)
/*******************************************************************************
LAST MODIFIED : 9 August 2004

DESCRIPTION:
==============================================================================*/
{
	LRESULT return_code;
	PAINTSTRUCT ps;

	ENTER(Graphics_buffer_callback_proc);

	return_code=FALSE;
	struct Graphics_buffer *graphics_buffer =
		(struct Graphics_buffer *)GetWindowLongPtr(window, GWL_USERDATA);

#if defined (DEBUG)
	printf("window callback %d\n", message_identifier);
#endif /* defined (DEBUG) */

	switch (message_identifier)
	{
		case WM_CREATE:
		{
			return_code=TRUE;
		}
		/* CS I don't think handling this message is necessary,
		   worse still sending a WM_QUIT kills the application */
		/*
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return_code=TRUE;
		} break;
		*/
		case WM_PAINT:
		{
#if defined (DEBUG)
			printf("WM_PAINT\n");
#endif /* defined (DEBUG) */
			BeginPaint(window, &ps);
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
				graphics_buffer->expose_callback_list, graphics_buffer, NULL);
			EndPaint(window, &ps);
			return_code=TRUE;
		} break;
		case WM_SIZING:
		{
			BeginPaint(window, &ps);
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
				graphics_buffer->resize_callback_list, graphics_buffer, NULL);
			EndPaint(window, &ps);
			return_code=TRUE;
		} break;
		case WM_LBUTTONDOWN:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			SetCapture(window);
			return_code=TRUE;
		} break;
		case WM_LBUTTONUP:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			ReleaseCapture();
			return_code=TRUE;
		} break;
		case WM_RBUTTONDOWN:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			SetCapture(window);
			return_code=TRUE;
		} break;
		case WM_RBUTTONUP:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			ReleaseCapture();
			return_code=TRUE;
		} break;
		case WM_MBUTTONDOWN:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			SetCapture(window);
			return_code=TRUE;
		} break;
		case WM_MBUTTONUP:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			ReleaseCapture();
			return_code=TRUE;
		} break;
		case WM_MOUSEMOVE:
		{
			Graphics_buffer_win32_button_callback(&message_identifier,
				graphics_buffer, first_message, second_message);
			return_code=TRUE;
		} break;
		case WM_ERASEBKGND:
		{
#if defined (DEBUG)
			printf("WM_ERASEBKGND\n");
#endif /* defined (DEBUG) */
			return_code=TRUE;
		} break;
		case WM_WINDOWPOSCHANGED:
		{
#if defined (DEBUG)
			printf("WM_WINDOWPOSCHANGED\n");
#endif /* defined (DEBUG) */
			return_code=TRUE;
		} break;
		case WM_MOVE:
		{
#if defined (DEBUG)
			printf("WM_MOVE\n");
#endif /* defined (DEBUG) */
			return_code=0;
		} break;
		case WM_SIZE:
		{
#if defined (DEBUG)
			printf("WM_SIZE\n");
#endif /* defined (DEBUG) */
			return_code=0;
		} break;
		default:
		{
			return_code=DefWindowProc(window,message_identifier,first_message,
				second_message);
		} break;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_callback_proc */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (WIN32_USER_INTERFACE)
int Graphics_buffer_win32_use_font_bitmaps(struct Graphics_buffer *buffer,
	HFONT font, int first_bitmap, int number_of_bitmaps, int display_list_offset)
/*******************************************************************************
LAST MODIFIED : 17 November 2005

DESCRIPTION :
==============================================================================*/
{
  	int return_code;

	ENTER(Graphics_buffer_win32_use_font_bitmaps);

	if (buffer)
	{
		switch (buffer->type)
		{
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				SelectObject(buffer->hDC, font);
				wglUseFontBitmaps(buffer->hDC, first_bitmap, number_of_bitmaps,
					display_list_offset);
				return_code = 1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_win32_use_font_bitmaps.  "
					"This function should only be used with WIN32 type buffers.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_win32_use_font_bitmaps.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_win32_use_font_bitmaps */
#endif /* defined (WIN32_USER_INTERFACE) */

#if defined (CARBON_USER_INTERFACE)
static OSStatus Graphics_buffer_mouse_Carbon_callback(
	EventHandlerCallRef handler, EventRef event,
	void* buffer_void)
/*******************************************************************************
LAST MODIFIED : 23 November 2006

DESCRIPTION :
==============================================================================*/
{
	int input_modifier, return_code;
	EventMouseButton button_number;
	HIPoint location;
	struct Graphics_buffer *buffer;
	struct Graphics_buffer_input input;
	UInt32 modifier_keys, event_type;
	Rect global_bounds;

	ENTER(Graphics_buffer_mouse_Carbon_callback);
	USE_PARAMETER(handler);
	if ((buffer=(struct Graphics_buffer *)buffer_void)
		&& event)
	{
		// Figure out if the event is ours by looking at the location
		GetEventParameter (event, kEventParamMouseLocation,
			typeHIPoint, NULL, sizeof(HIPoint), NULL, &location);

#if defined (DEBUG)
		printf("  Graphics_buffer_mouse_Carbon_callback %lf %lf\n",
			location.x, location.y);
#endif // defined (DEBUG)

 		// Convert from global screen coordinates to window coordinates
		GetWindowBounds(GetWindowFromPort(buffer->port),
			kWindowGlobalPortRgn, &global_bounds);

#if defined (DEBUG)
		printf("    global bounds %d %d\n", global_bounds.top, global_bounds.left);
#endif // defined (DEBUG)

		location.x -= global_bounds.left;
		location.y -= global_bounds.top;

#if defined (DEBUG)
		printf("    local coordinates %lf %lf\n", location.x, location.y);
#endif // defined (DEBUG)

		location.x -= -buffer->portx;
		location.y -= -buffer->porty;

#if defined (DEBUG)
		printf("    graphics buffer coordinates %lf %lf\n", location.x, location.y);
#endif // defined (DEBUG)

		if ((location.x < 0) ||
			(location.x > buffer->clip_width) ||
			(location.y < 0) ||
			(location.y > buffer->clip_height))
		{
			// If the event isn't ours, pass it on.
			OSStatus result = CallNextEventHandler(handler, event);
			return(result);
		}
		return_code = 1;
		input.type = GRAPHICS_BUFFER_INVALID_INPUT;
		event_type = GetEventKind(event);
		switch (event_type)
		{
			case kEventMouseDown:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_PRESS;
			} break;
			case kEventMouseUp:
			{
				input.type = GRAPHICS_BUFFER_BUTTON_RELEASE;
			} break;
			case kEventMouseDragged:
			{
				input.type = GRAPHICS_BUFFER_MOTION_NOTIFY;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Graphics_buffer_gtkglarea_button_callback.  Unknown button event");
				return_code=0;
				/* This event type is not being passed on */
			} break;
		}
		input.key_code = 0;

		GetEventParameter (event, kEventParamMouseButton,
			typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button_number);
		input.button_number = button_number;

		input.position_x = (int)location.x;
		input.position_y = (int)location.y;
		input_modifier = 0;

		GetEventParameter (event, kEventParamKeyModifiers,
                   typeUInt32, NULL,
                   sizeof(modifier_keys), NULL,
                   &modifier_keys);

		if (modifier_keys & shiftKey)
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_SHIFT;
		}
		if (modifier_keys & controlKey)
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_CONTROL;
		}
		if (modifier_keys & optionKey)
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_ALT;
		}
		if (button_number == kEventMouseButtonPrimary)
		{
			input_modifier |= GRAPHICS_BUFFER_INPUT_MODIFIER_BUTTON1;
		}
		if (return_code)
		{
			input.input_modifier = static_cast<enum Graphics_buffer_input_modifier>
				(input_modifier);
			CMISS_CALLBACK_LIST_CALL(Graphics_buffer_input_callback)(
				buffer->input_callback_list, buffer, &input);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_gtkglarea_button_callback.  Invalid argument(s)");
	}
	LEAVE;

	return (TRUE);
} /* Graphics_buffer_mouse_Carbon_callback */
#endif /* defined (CARBON_USER_INTERFACE) */

#if defined (CARBON_USER_INTERFACE)
static OSStatus Graphics_buffer_expose_Carbon_callback(
	EventHandlerCallRef handler, EventRef event,
	void* buffer_void)
/*******************************************************************************
LAST MODIFIED : 23 November 2006

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	USE_PARAMETER(handler);
	USE_PARAMETER(event);

	OSStatus result = eventNotHandledErr;

	if (graphics_buffer = (struct Graphics_buffer *)buffer_void)
	{
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->expose_callback_list, graphics_buffer, NULL);

		result = noErr;

	}
	LEAVE;

	return (result);
} /* Graphics_buffer_expose_Carbon_callback */
#endif /* defined (CARBON_USER_INTERFACE) */

#if defined (CARBON_USER_INTERFACE)
static OSStatus Graphics_buffer_resize_Carbon_callback(
	EventHandlerCallRef handler, EventRef event,
	void* buffer_void)
/*******************************************************************************
LAST MODIFIED : 23 November 2006

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	USE_PARAMETER(handler);
	USE_PARAMETER(event);

	OSStatus result = eventNotHandledErr;

	if (graphics_buffer = (struct Graphics_buffer *)buffer_void)
	{
		aglSetDrawable(graphics_buffer->aglContext, graphics_buffer->port);
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->resize_callback_list, graphics_buffer, NULL);
		CMISS_CALLBACK_LIST_CALL(Graphics_buffer_callback)(
			graphics_buffer->expose_callback_list, graphics_buffer, NULL);

		result = noErr;

	}
	LEAVE;

	return (result);
} /* Graphics_buffer_expose_Carbon_callback */
#endif /* defined (CARBON_USER_INTERFACE) */

#if defined (CARBON_USER_INTERFACE)
struct Graphics_buffer *create_Graphics_buffer_Carbon(
	struct Graphics_buffer_package *graphics_buffer_package,
	CGrafPtr port,
	int    portx,
	int    porty,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 21 November 2006

DESCRIPTION :
==============================================================================*/
{
	GLint attributes[] = {AGL_RGBA,
								 AGL_DOUBLEBUFFER,
								 AGL_DEPTH_SIZE, 16,
								 AGL_NONE};
	/* int accumulation_colour_size; */
	struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_Carbon);
	USE_PARAMETER(buffering_mode);
	USE_PARAMETER(stereo_mode);
	USE_PARAMETER(minimum_colour_buffer_depth);
	USE_PARAMETER(minimum_depth_buffer_depth);
	USE_PARAMETER(minimum_accumulation_buffer_depth);
	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
		buffer->type = GRAPHICS_BUFFER_CARBON_TYPE;

		buffer->port = port;
		buffer->portx = portx;
		buffer->porty = porty;
		buffer->width = 0;
		buffer->height = 0;
		buffer->clip_width = 0;
		buffer->clip_height = 0;

		if (buffer->aglPixelFormat = aglChoosePixelFormat(NULL, 0, attributes))
		{
			if (buffer->aglContext = aglCreateContext(buffer->aglPixelFormat, NULL))
			{

				if(aglSetDrawable(buffer->aglContext, port))
				{

					if (aglSetCurrentContext(buffer->aglContext))
					{
						EventHandlerUPP mouse_handler_UPP,
							expose_handler_UPP, resize_handler_UPP;

						EventTypeSpec expose_event_list[] = {
                    {kEventClassWindow, kEventWindowDrawContent}};
						EventTypeSpec resize_event_list[] = {
                    {kEventClassWindow, kEventWindowBoundsChanged}};
						EventTypeSpec mouse_event_list[] = {
							{kEventClassMouse, kEventMouseDragged},
							{kEventClassMouse, kEventMouseUp},
							{kEventClassMouse, kEventMouseDown}};

						aglEnable(buffer->aglContext, AGL_BUFFER_RECT);
						//aglEnable(buffer->aglContext, AGL_SWAP_RECT);

						expose_handler_UPP =
							NewEventHandlerUPP (Graphics_buffer_expose_Carbon_callback);

						InstallWindowEventHandler(GetWindowFromPort(port),
							expose_handler_UPP, GetEventTypeCount(expose_event_list),
							expose_event_list, buffer, &buffer->expose_handler_ref);

						resize_handler_UPP =
							NewEventHandlerUPP (Graphics_buffer_resize_Carbon_callback);

						InstallWindowEventHandler(GetWindowFromPort(port),
							resize_handler_UPP, GetEventTypeCount(resize_event_list),
							resize_event_list, buffer, &buffer->resize_handler_ref);

						mouse_handler_UPP =
							NewEventHandlerUPP (Graphics_buffer_mouse_Carbon_callback);

						InstallWindowEventHandler(GetWindowFromPort(port),
							mouse_handler_UPP, GetEventTypeCount(mouse_event_list),
							mouse_event_list, buffer, &buffer->mouse_handler_ref);

					}
					else
					{
						display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
							"Unable enable the render context.");
						DESTROY(Graphics_buffer)(&buffer);
						buffer = (struct Graphics_buffer *)NULL;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
						"Unable associate port with context.");
					DESTROY(Graphics_buffer)(&buffer);
					buffer = (struct Graphics_buffer *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
					"Unable to create the render context.");
				DESTROY(Graphics_buffer)(&buffer);
				buffer = (struct Graphics_buffer *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
				"Unable to set pixel format.");
			DESTROY(Graphics_buffer)(&buffer);
			buffer = (struct Graphics_buffer *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_win32.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
} /* create_Graphics_buffer_Carbon */
#endif /* defined (CARBON_USER_INTERFACE) */

#if defined (CARBON_USER_INTERFACE)
int Graphics_buffer_carbon_set_window_size(struct Graphics_buffer *buffer,
	int width, int height, int portx, int porty, int clip_width, int clip_height)
/*******************************************************************************
LAST MODIFIED : 16 February 2007

DESCRIPTION :
Sets the coordinates within the graphics port which the graphics_buffer should
respect.
==============================================================================*/
{
	int return_code;

	ENTER(Cmiss_graphics_buffer_get_near_and_far_plane);
	if (buffer)
	{
		buffer->width = width;
		buffer->height = height;
		buffer->portx = portx;
		buffer->porty = porty;
		buffer->clip_width = clip_width;
		buffer->clip_height = clip_height;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Cmiss_graphics_buffer_carbon_set_window_size.  "
			"Missing graphics_buffer parameter.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Cmiss_graphics_buffer_carbon_set_window_size */
#endif /* defined (CARBON_USER_INTERFACE) */

#if defined (WX_USER_INTERFACE)
struct Graphics_buffer *create_Graphics_buffer_wx(
	struct Graphics_buffer_package *graphics_buffer_package,
	wxPanel *parent,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode,
	int minimum_colour_buffer_depth, int minimum_depth_buffer_depth,
	int minimum_accumulation_buffer_depth,
	struct Graphics_buffer  *buffer_to_match)
{
	 struct Graphics_buffer *buffer;

	ENTER(create_Graphics_buffer_wx);
	if (buffer = CREATE(Graphics_buffer)(graphics_buffer_package))
	{
		 buffer->type = GRAPHICS_BUFFER_WX_TYPE;
		 Graphics_buffer_create_buffer_wx(buffer, graphics_buffer_package,
				parent, buffering_mode, stereo_mode, minimum_colour_buffer_depth,
				minimum_depth_buffer_depth, minimum_accumulation_buffer_depth, 0, 0,
				buffer_to_match);
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Graphics_buffer_wx.  "
			"Unable to create generic Graphics_buffer.");
		buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (buffer);
}

#endif /* defined (WX_USER_INTERFACE) */

int Graphics_buffer_make_current(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
==============================================================================*/
{
  	int return_code;

	ENTER(Graphics_buffer_make_current);

	if (buffer)
	{
#if defined (DEBUG)
		printf("Graphics_buffer_make_current\n");
#endif /* defined (DEBUG) */
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				glXMakeCurrent(buffer->display, XtWindow(buffer->drawing_widget),
					buffer->context);
				return_code = 1;
			} break;
#  if defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer)
			case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
			{
				glXMakeCurrent(buffer->display, buffer->glx_pbuffer,
					buffer->context);
				return_code = 1;
			} break;
#  endif /* defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer) */
			case GRAPHICS_BUFFER_GLX_PIXMAP_TYPE:
			{
				glXMakeCurrent(buffer->display, buffer->glx_pixmap,
					buffer->context);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				gtk_gl_area_make_current(GTK_GL_AREA(buffer->glarea));
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
#if defined (DEBUG)
				printf("Graphics_buffer_make_current %p %p\n",
					buffer->gldrawable, buffer->glcontext);
#endif /* defined (DEBUG) */
				gdk_gl_drawable_make_current(buffer->gldrawable, buffer->glcontext);
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				wglMakeCurrent( buffer->hDC, buffer->hRC );
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				GLint parms[4] ;
				Rect bounds, port_bounds;
				GetWindowPortBounds(GetWindowFromPort(buffer->port), &bounds);

 				GetPortBounds(buffer->port, &port_bounds);

				aglSetCurrentContext(buffer->aglContext);

				parms[0] = -buffer->portx;
				parms[1] = bounds.bottom - bounds.top + buffer->porty - buffer->clip_height;
				parms[2] = buffer->clip_width;
				parms[3] = buffer->clip_height;
				aglSetInteger(buffer->aglContext, AGL_BUFFER_RECT, parms) ;
				//aglSetInteger(buffer->aglContext, AGL_SWAP_RECT, parms) ;

				aglUpdateContext(buffer->aglContext);

				return_code = 1;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				 if (buffer->canvas)
				 {
						buffer->canvas->SetCurrent();
						return_code = 1;
				 }
				 else
				 {
						return_code = 0;
				 }
			} break;
			 case GRAPHICS_BUFFER_WX_OFFSCREEN_TYPE:
			{
				return_code = 0;
			} break;
			 case GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE:
			{
#if defined (OPENGL_API) && defined (GL_EXT_framebuffer_object)
			 if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
			 {
				 if (buffer->fbo && buffer->depthbuffer && buffer->img)
				 glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buffer->fbo);
				 glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,buffer->depthbuffer);
				 glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
					 buffer->framebuffer_width, buffer->framebuffer_height);
				 glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
					 GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D, buffer->img, 0);
				 glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, 
					 GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->depthbuffer);
				 GLenum status;
				 status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
				 switch(status) 
				 {                                          
					 case GL_FRAMEBUFFER_COMPLETE_EXT:
					 {
						 return_code = 1;
					 }    
					 break;
					 case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
					 {
						 display_message(ERROR_MESSAGE, 
							 "Graphics_buffer_make_current."  
							 "Framebuffer object format not supported.\n");
						 return_code = 0;
					 } 
					 break;
					 default:
					 {
						 display_message(ERROR_MESSAGE, 
							 "Graphics_buffer_make_current." 
							 "Framebuffer object not supported.\n");
						 return_code = 0;
					 }
				 }
			 }
#endif
			} break;
#endif /* defined (WX_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_make_current.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_make_current.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_make_current */

int Graphics_buffer_get_visual_id(struct Graphics_buffer *buffer, int *visual_id)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the visual id used by the graphics buffer.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_get_visual_id);

	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				*visual_id =(int) buffer->visual_info->visualid;
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				*visual_id = 0;
				return_code = 0;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				*visual_id = 0;
				return_code = 0;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				*visual_id = buffer->pixel_format;
				return_code = 0;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				*visual_id = 0;
				return_code = 0;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
#if defined (__WXGTK__)
				*visual_id = (int)((XVisualInfo*)buffer->canvas->m_vi)
					->visualid;
				return_code = 1;
#else /* if defined (__WXGTK__) */
				*visual_id = 0;
				return_code = 0;
#endif /* if defined (__WXGTK__) */
			} break;
#endif /* defined (WX_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_visual_id.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_visual_id.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_visual_id */

int Graphics_buffer_get_colour_buffer_depth(struct Graphics_buffer *buffer,
	int *colour_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the depth of the colour buffer used by the graphics buffer.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_get_colour_buffer_depth);
	if (buffer)
	{
		return_code = 1;
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				glXGetConfig(buffer->display, buffer->visual_info, GLX_BUFFER_SIZE, colour_buffer_depth);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			 case GRAPHICS_BUFFER_WX_TYPE:
			{
				 GLint colour_buffer_bits;
				 glGetIntegerv(GL_RED_BITS, &colour_buffer_bits);
				 *colour_buffer_depth = colour_buffer_bits;
				 glGetIntegerv(GL_BLUE_BITS, &colour_buffer_bits);
				 *colour_buffer_depth += colour_buffer_bits;
				 glGetIntegerv(GL_GREEN_BITS, &colour_buffer_bits);
				 *colour_buffer_depth += colour_buffer_bits;
				 glGetIntegerv(GL_ALPHA_BITS, &colour_buffer_bits);
				 *colour_buffer_depth += colour_buffer_bits;
				 return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				*colour_buffer_depth = 0;
				return_code = 0;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_BUFFER_SIZE,
					colour_buffer_depth);
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				PIXELFORMATDESCRIPTOR pfd;
				ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR ) );
				pfd.nSize = sizeof( pfd );
				pfd.nVersion = 1;

				DescribePixelFormat(buffer->hDC, buffer->pixel_format, pfd.nSize, &pfd);

				*colour_buffer_depth = pfd.cColorBits;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_colour_buffer_depth.  "
					"Graphics_buffer type unknown or not supported.");
				*colour_buffer_depth = 0;
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_colour_buffer_depth.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_colour_buffer_depth */

int Graphics_buffer_get_depth_buffer_depth(struct Graphics_buffer *buffer,
	int *depth_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the depth of the depth buffer used by the graphics buffer.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_get_depth_buffer_depth);
	if (buffer)
	{
		return_code = 1;
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				glXGetConfig(buffer->display, buffer->visual_info, GLX_DEPTH_SIZE, depth_buffer_depth);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			 case GRAPHICS_BUFFER_WX_TYPE:
			{
				 GLint depth_bits;
				 glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
				 *depth_buffer_depth = depth_bits;
				 return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				*depth_buffer_depth = 0;
				return_code = 0;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_DEPTH_SIZE,
					depth_buffer_depth);
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				PIXELFORMATDESCRIPTOR pfd;
				ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR ) );
				pfd.nSize = sizeof( pfd );
				pfd.nVersion = 1;

				DescribePixelFormat(buffer->hDC, buffer->pixel_format, pfd.nSize, &pfd);

				*depth_buffer_depth = pfd.cDepthBits;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_depth_buffer_depth.  "
					"Graphics_bufffer type unknown or not supported.");
				*depth_buffer_depth = 0;
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_depth_buffer_depth.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_depth_buffer_depth */

int Graphics_buffer_get_accumulation_buffer_depth(struct Graphics_buffer *buffer,
	int *accumulation_buffer_depth)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the depth of the accumulation buffer used by the graphics buffer.
==============================================================================*/
{
#if defined (MOTIF_USER_INTERFACE)
	int colour_size;
#endif /* defined (MOTIF_USER_INTERFACE) */
	int return_code;

	ENTER(Graphics_buffer_get_accumulation_buffer_depth);
	if (buffer)
	{
		return_code = 1;
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				*accumulation_buffer_depth = 0;
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_ACCUM_RED_SIZE, &colour_size);
				*accumulation_buffer_depth += colour_size;
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_ACCUM_GREEN_SIZE, &colour_size);
				*accumulation_buffer_depth += colour_size;
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_ACCUM_BLUE_SIZE, &colour_size);
				*accumulation_buffer_depth += colour_size;
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_ACCUM_ALPHA_SIZE, &colour_size);
				*accumulation_buffer_depth += colour_size;
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			 case GRAPHICS_BUFFER_WX_TYPE:
			{
				 GLint accumulation_buffer_bits;
				 glGetIntegerv(GL_ACCUM_RED_BITS, &accumulation_buffer_bits);
				 *accumulation_buffer_depth = accumulation_buffer_bits;
				 glGetIntegerv(GL_ACCUM_BLUE_BITS, &accumulation_buffer_bits);
				 *accumulation_buffer_depth += accumulation_buffer_bits;
				 glGetIntegerv(GL_ACCUM_GREEN_BITS, &accumulation_buffer_bits);
				 *accumulation_buffer_depth += accumulation_buffer_bits;
				 glGetIntegerv(GL_ACCUM_ALPHA_BITS, &accumulation_buffer_bits);
				 *accumulation_buffer_depth += accumulation_buffer_bits;
				 return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				 *accumulation_buffer_depth = 0;
				return_code = 0;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				int accum_red_size, accum_green_size, accum_blue_size, accum_alpha_size;
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_ACCUM_RED_SIZE,
					&accum_red_size);
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_ACCUM_GREEN_SIZE,
					&accum_green_size);
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_ACCUM_BLUE_SIZE,
					&accum_blue_size);
				gdk_gl_config_get_attrib(buffer->glconfig, GDK_GL_ACCUM_ALPHA_SIZE,
					&accum_alpha_size);
				*accumulation_buffer_depth = accum_red_size + accum_green_size +
					accum_blue_size + accum_alpha_size;
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				PIXELFORMATDESCRIPTOR pfd;
				ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR ) );
				pfd.nSize = sizeof( pfd );
				pfd.nVersion = 1;

				DescribePixelFormat(buffer->hDC, buffer->pixel_format, pfd.nSize, &pfd);

				*accumulation_buffer_depth = pfd.cAccumBits;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_accumulation_buffer_depth.  "
					"Graphics_bufffer type unknown or not supported.");
				*accumulation_buffer_depth = 0;
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_accumulation_buffer_depth.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_accumulation_buffer_depth */

int Graphics_buffer_get_buffering_mode(struct Graphics_buffer *buffer,
	enum Graphics_buffer_buffering_mode *buffering_mode)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the buffering mode used by the graphics buffer.
==============================================================================*/
{
	int return_code;
#if defined (MOTIF_USER_INTERFACE)
	int double_buffer;
#endif /* defined (MOTIF_USER_INTERFACE) */

	ENTER(Graphics_buffer_get_buffering_mode);
#if defined (GTK_USER_INTERFACE)
	USE_PARAMETER(buffering_mode);
#endif /* defined (GTK_USER_INTERFACE) */
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_DOUBLEBUFFER, &double_buffer);
				if (double_buffer)
				{
					*buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
				}
				else
				{
					*buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
				}
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				*buffering_mode = buffer->buffering_mode;
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				if (gdk_gl_config_is_double_buffered(buffer->glconfig))
				{
					*buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
				}
				else
				{
					*buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
				}
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				PIXELFORMATDESCRIPTOR pfd;
				int iPixelFormat;
				iPixelFormat = GetPixelFormat(buffer->hDC);
				DescribePixelFormat(buffer->hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
				if(pfd.dwFlags & PFD_DOUBLEBUFFER)
				{
					*buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
				}
				else
				{
					*buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
				}
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				GLint value;

				if (aglDescribePixelFormat (buffer->aglPixelFormat,
						AGL_DOUBLEBUFFER, &value))
				{
					if (value)
					{
						*buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
					}
					else
					{
						*buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
					}
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE,"Graphics_buffer_get_buffering_mode.  "
						"Invalid Pixel Format Query.");
					return_code = 0;
				}
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				*buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_buffering_mode.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_buffering_mode.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_buffering_mode */

int Graphics_buffer_get_stereo_mode(struct Graphics_buffer *buffer,
	enum Graphics_buffer_stereo_mode *stereo_mode)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Returns the stereo mode used by the graphics buffer.
==============================================================================*/
{
	int return_code;
#if defined (MOTIF_USER_INTERFACE)
	int stereo;
#endif /* defined (MOTIF_USER_INTERFACE) */

	ENTER(Graphics_buffer_get_stereo_mode);
#if defined (GTK_USER_INTERFACE)
	USE_PARAMETER(stereo_mode);
#endif /* defined (GTK_USER_INTERFACE) */
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
 				glXGetConfig(buffer->display, buffer->visual_info, GLX_STEREO, &stereo);
				if (stereo)
				{
					*stereo_mode = GRAPHICS_BUFFER_STEREO;
				}
				else
				{
					*stereo_mode = GRAPHICS_BUFFER_MONO;
				}
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				*stereo_mode = buffer->stereo_mode;
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				if (gdk_gl_config_is_stereo(buffer->glconfig))
				{
					*stereo_mode = GRAPHICS_BUFFER_STEREO;
				}
				else
				{
					*stereo_mode = GRAPHICS_BUFFER_MONO;
				}
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				PIXELFORMATDESCRIPTOR pfd;
				int iPixelFormat;
				iPixelFormat = GetPixelFormat(buffer->hDC);
				DescribePixelFormat(buffer->hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
				if(pfd.dwFlags & PFD_STEREO)
				{
					*stereo_mode = GRAPHICS_BUFFER_STEREO;
				}
				else
				{
					*stereo_mode = GRAPHICS_BUFFER_MONO;
				}
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				GLint value;

				if (aglDescribePixelFormat (buffer->aglPixelFormat,
						AGL_STEREO, &value))
				{
					if (value)
					{
						*stereo_mode = GRAPHICS_BUFFER_STEREO;
					}
					else
					{
						*stereo_mode = GRAPHICS_BUFFER_MONO;
					}
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE,"Graphics_buffer_get_buffering_mode.  "
						"Invalid Pixel Format Query.");
					return_code = 0;
				}
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				*stereo_mode = GRAPHICS_BUFFER_MONO;
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */

			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_stereo_mode.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_get_stereo_mode.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_get_stereo_mode */

int Graphics_buffer_swap_buffers(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_swap_buffers);

	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				glXSwapBuffers(buffer->display, XtWindow(buffer->drawing_widget));
				return_code = 1;
			} break;
#  if defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer)
			case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
			{
				glXSwapBuffers(buffer->display, buffer->glx_pbuffer);
				return_code = 1;
			} break;
#  endif /* defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer) */
			case GRAPHICS_BUFFER_GLX_PIXMAP_TYPE:
			{
				glXSwapBuffers(buffer->display, buffer->glx_pixmap);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				gtk_gl_area_swapbuffers(GTK_GL_AREA(buffer->glarea));
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				gdk_gl_drawable_swap_buffers(buffer->gldrawable);
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				/* Seems to help the OpenGL context get updated to the right place in Vista */
				SetWindowPos(buffer->hWnd, HWND_TOP, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				SwapBuffers(buffer->hDC);
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				buffer->canvas->SwapBuffers();
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				aglSwapBuffers(buffer->aglContext);
				return_code = 1;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_swap_buffers.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_swap_buffers.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_swap_buffers */

int Graphics_buffer_make_read_current(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 28 May 2004

DESCRIPTION :
Sets this buffer to be the GLX source and the current ThreeDWindow (the one last
made current) to be the GLX destination.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_make_read_current);

	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
#  if defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer)
			case GRAPHICS_BUFFER_GLX_PBUFFER_TYPE:
			{
				glXMakeContextCurrent(buffer->display, glXGetCurrentDrawable(),
					buffer->glx_pbuffer, glXGetCurrentContext());
				return_code = 1;
			} break;
#  endif /* defined (USE_GLX_PBUFFER) || defined (GLX_SGIX_dmbuffer) || defined (GLX_SGIX_pbuffer) */
#endif /* defined (MOTIF_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_make_read_current.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_buffer_make_read_current.  "
			"Graphics_bufffer missing.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_make_read_current */

int Graphics_buffer_get_width(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Returns the width of buffer represented by <buffer>.
==============================================================================*/
{
	int width;

	ENTER(Graphics_buffer_get_width);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xwidth;

				XtVaGetValues(buffer->drawing_widget,
					XmNwidth,&xwidth,
					NULL);
				width = xwidth;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				width = buffer->glarea->allocation.width;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				width = buffer->glarea->allocation.width;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTKGLAREA) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				if (buffer->hWnd)
				{
					RECT rect;
					if(GetWindowRect(buffer->hWnd, &rect))
					{
						width = rect.right - rect.left;
					}
					else
					{
						display_message(ERROR_MESSAGE,"Graphics_buffer_get_width.  "
							"Failed to get window rectangle");
						width = 0;
					}
				}
				else
				{
					width = buffer->width;
				}
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				int height;
				buffer->canvas->GetClientSize(&width, &height);
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				// Respect the values we have been given
				width = buffer->width;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_width.  "
					"Graphics_bufffer type unknown or not supported.");
				width = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_width.  Invalid buffer");
		width = 0;
	}
	LEAVE;

	return (width);
} /* Graphics_buffer_get_width */

int Graphics_buffer_set_width(struct Graphics_buffer *buffer, int width)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Sets the width of buffer represented by <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_set_width);
#if !defined (MOTIF_USER_INTERFACE)
	USE_PARAMETER(width);
#endif /* !defined (MOTIF_USER_INTERFACE) */
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xwidth;

				xwidth = width;
				XtVaSetValues(buffer->drawing_widget,
					XmNwidth, xwidth,
					NULL);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				int old_width, height;
				buffer->canvas->GetClientSize(&old_width, &height);
				buffer->canvas->SetClientSize(width, height);
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_set_width.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_set_width.  Invalid buffer");
		return_code = 1;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_set_width */

int Graphics_buffer_get_height(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Returns the height of buffer represented by <buffer>.
==============================================================================*/
{
	int height;

	ENTER(Graphics_buffer_get_height);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xheight;

				XtVaGetValues(buffer->drawing_widget,
					XmNheight,&xheight,
					NULL);
				height = xheight;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				height = buffer->glarea->allocation.height;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				height = buffer->glarea->allocation.height;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTKGLAREA) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				if (buffer->hWnd)
				{
					RECT rect;
					if(GetWindowRect(buffer->hWnd, &rect))
					{
						height = rect.bottom - rect.top;
					}
					else
					{
						display_message(ERROR_MESSAGE,"Graphics_buffer_get_height.  "
							"Failed to get window rectangle");
						height = 0;
					}
				}
				else
				{
					height = buffer->height;
				}
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				int width;
				buffer->canvas->GetClientSize(&width, &height);
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				// Respect the values we have been given
				height = buffer->height;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_height.  "
					"Graphics_bufffer type unknown or not supported.");
				height = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_height.  Invalid buffer");
		height = 0;
	}
	LEAVE;

	return (height);
} /* Graphics_buffer_get_height */

int Graphics_buffer_set_height(struct Graphics_buffer *buffer, int height)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Sets the height of buffer represented by <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_set_height);
#if !defined (MOTIF_USER_INTERFACE)
	USE_PARAMETER(height);
#endif /* !defined (MOTIF_USER_INTERFACE) */
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xheight;

				xheight = height;
				XtVaSetValues(buffer->drawing_widget,
					XmNheight, xheight,
					NULL);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				int width, old_height;
				buffer->canvas->GetClientSize(&width, &old_height);
				buffer->canvas->SetClientSize(width, height);
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_set_height.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_set_height.  Invalid buffer");
		return_code = 1;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_set_height */

int Graphics_buffer_get_origin_x(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 16 February 2007

DESCRIPTION :
Returns the x origin of buffer represented by <buffer>.
==============================================================================*/
{
	int origin_x;

	ENTER(Graphics_buffer_get_origin_x);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				// Respect the values we have been given
				origin_x = buffer->clip_width - buffer->width;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				origin_x = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_origin_x.  Invalid buffer");
		origin_x = 0;
	}
	LEAVE;

	return (origin_x);
} /* Graphics_buffer_get_origin_x */

int Graphics_buffer_get_origin_y(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 16 February 2007

DESCRIPTION :
Returns the y origin of buffer represented by <buffer>.
==============================================================================*/
{
	int origin_y;

	ENTER(Graphics_buffer_get_origin_y);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				// Respect the values we have been given
				origin_y = buffer->clip_height - buffer->height;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				origin_y = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_origin_y.  Invalid buffer");
	   origin_y = 0;
	}
	LEAVE;

	return (origin_y);
} /* Graphics_buffer_get_origin_y */

int Graphics_buffer_get_border_width(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
Returns the border width of buffer represented by <buffer>.
==============================================================================*/
{
	int border_width;

	ENTER(Graphics_buffer_get_border_width);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xborder_width;

				XtVaGetValues(buffer->drawing_widget,
					XmNborderWidth,&xborder_width,
					NULL);
				border_width = xborder_width;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_get_border_width.  "
					"Graphics_bufffer type unknown or not supported.");
				border_width = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_border_width.  Invalid buffer");
		border_width = 0;
	}
	LEAVE;

	return (border_width);
} /* Graphics_buffer_get_border_width */

int Graphics_buffer_set_border_width(struct Graphics_buffer *buffer, int border_width)
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
Sets the border width of buffer represented by <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_set_border_width);
#if !defined (MOTIF_USER_INTERFACE)
	USE_PARAMETER(border_width);
#endif /* !defined (MOTIF_USER_INTERFACE) */
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				Dimension xborder_width;

				xborder_width = border_width;
				/*XtVaSetValues(scene_viewer->drawing_widget,
				   XmNborderWidth, xborder_width,NULL);*/
				XtVaSetValues(buffer->drawing_widget,
					XmNleftOffset, xborder_width,
					XmNrightOffset, xborder_width,
					XmNbottomOffset, xborder_width,
					XmNtopOffset, xborder_width,NULL);
				return_code = 1;
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_set_border_width.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_set_border_width.  Invalid buffer");
		return_code = 1;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_set_border_width */

int Graphics_buffer_is_visible(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Returns 1 if the <buffer> is visible.  If the scene viewer gets zero from this
routine it will not bother rendering into it, allowing us to avoid rendering
into unmanaged or invisible widgets.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_is_visible);
	if (buffer)
	{
		return_code = 0;
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				return_code = XtIsManaged(buffer->drawing_widget)&&
					XtIsRealized(buffer->drawing_widget)&&
					XtIsManaged(buffer->parent);
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_is_visible.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_is_visible.  Invalid buffer");
		return_code=GRAPHICS_BUFFER_INVALID_TYPE;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_is_visible */

int Graphics_buffer_awaken(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Activates the graphics <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_awaken);
	if (buffer)
	{
		switch (buffer->type)
		{
#if defined (MOTIF_USER_INTERFACE)
			case GRAPHICS_BUFFER_GLX_X3D_TYPE:
			{
				XtManageChild(buffer->drawing_widget);
			} break;
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if defined (GTK_USE_GTKGLAREA)
			case GRAPHICS_BUFFER_GTKGLAREA_TYPE:
			{
				gtk_widget_show(buffer->glarea);
				return_code = 1;
			} break;
#else /* defined (GTK_USE_GTKGLAREA) */
			case GRAPHICS_BUFFER_GTKGLEXT_TYPE:
			{
				gtk_widget_show(buffer->glarea);
				return_code = 1;
			} break;
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (WIN32_USER_INTERFACE)
			case GRAPHICS_BUFFER_WIN32_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
			case GRAPHICS_BUFFER_WX_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (WX_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
			case GRAPHICS_BUFFER_CARBON_TYPE:
			{
				return_code = 1;
			} break;
#endif /* defined (CARBON_USER_INTERFACE) */
			default:
			{
				display_message(ERROR_MESSAGE,"Graphics_buffer_awaken.  "
					"Graphics_bufffer type unknown or not supported.");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_awaken.  Invalid buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_awaken */

enum Graphics_buffer_type Graphics_buffer_get_type(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 27 May 2004

DESCRIPTION :
Returns information about the type of buffer that was created.
==============================================================================*/
{
	enum Graphics_buffer_type buffer_type;

	ENTER(Graphics_buffer_get_type);
	if (buffer)
	{
		buffer_type = buffer->type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_display.  Invalid buffer");
		buffer_type = GRAPHICS_BUFFER_INVALID_TYPE;
	}
	LEAVE;

	return (buffer_type);
} /* Graphics_buffer_get_type */

#if defined (MOTIF_USER_INTERFACE)
Display *Graphics_buffer_X11_get_display(struct Graphics_buffer *buffer)
/*******************************************************************************
LAST MODIFIED : 27 May 2004

DESCRIPTION :
Returns information about the type of buffer that was created.
==============================================================================*/
{
	Display *display;

	ENTER(Graphics_buffer_get_type);
	if (buffer)
	{
		display = buffer->display;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_get_display.  Invalid buffer");
		display = (Display *)NULL;
	}
	LEAVE;

	return (display);
} /* Graphics_buffer_get_type */
#endif /* defined (MOTIF_USER_INTERFACE) */

int Graphics_buffer_add_initialise_callback(struct Graphics_buffer *buffer,
	CMISS_CALLBACK_FUNCTION(Graphics_buffer_callback) initialise_callback, void *user_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Adds an initialise callback to the graphics <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_awaken);
	if (buffer)
	{
		return_code = CMISS_CALLBACK_LIST_ADD_CALLBACK(Graphics_buffer_callback)(
			buffer->initialise_callback_list, initialise_callback,
			user_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_add_initialise_callback.  Invalid buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_add_initialise_callback */

int Graphics_buffer_add_resize_callback(struct Graphics_buffer *buffer,
	CMISS_CALLBACK_FUNCTION(Graphics_buffer_callback) resize_callback, void *user_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Adds an resize callback to the graphics <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_awaken);
	if (buffer)
	{
		return_code = CMISS_CALLBACK_LIST_ADD_CALLBACK(Graphics_buffer_callback)(
			buffer->resize_callback_list, resize_callback,
			user_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_add_resize_callback.  Invalid buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_add_resize_callback */

int Graphics_buffer_add_expose_callback(struct Graphics_buffer *buffer,
	CMISS_CALLBACK_FUNCTION(Graphics_buffer_callback) expose_callback, void *user_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Adds an expose callback to the graphics <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_awaken);
	if (buffer)
	{
		return_code = CMISS_CALLBACK_LIST_ADD_CALLBACK(Graphics_buffer_callback)(
			buffer->expose_callback_list, expose_callback,
			user_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_add_expose_callback.  Invalid buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_add_expose_callback */

int Graphics_buffer_add_input_callback(struct Graphics_buffer *buffer,
	CMISS_CALLBACK_FUNCTION(Graphics_buffer_input_callback) input_callback,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Adds an input callback to the graphics <buffer>.
==============================================================================*/
{
	int return_code;

	ENTER(Graphics_buffer_awaken);
	if (buffer)
	{
		return_code = CMISS_CALLBACK_LIST_ADD_CALLBACK(Graphics_buffer_input_callback)(
			buffer->input_callback_list, input_callback,
			user_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphics_buffer_add_input_callback.  Invalid buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphics_buffer_add_input_callback */

int DESTROY(Graphics_buffer)(struct Graphics_buffer **buffer_ptr)
/*******************************************************************************
LAST MODIFIED : 1 July 2002

DESCRIPTION :
Closes a Graphics buffer instance
x==============================================================================*/
{
	int return_code;
	struct Graphics_buffer *buffer;

	ENTER(DESTROY(Graphics_buffer));
	if (buffer_ptr && (buffer = *buffer_ptr))
	{
		return_code=1;
#if defined (MOTIF_USER_INTERFACE)
		/* I have listed everything just to make sure I have considered them all */
		/* buffer->drawing_widget, handled by window closing */
		/* buffer->parent, responsibility of shell */
		/* buffer->display, just a handle */

#  if defined (GLX_SGIX_dmbuffer)
	   if (buffer->dmbuffer)
		{
			dmBufferFree(buffer->dmbuffer);
		}
		if (buffer->dmpool)
		{
			dmBufferDestroyPool(buffer->dmpool);
		}
#  endif /* defined (GLX_SGIX_dmbuffer) */
		if (buffer->context)
		{
			/* Don't destroy if this is the context in the package for sharing */
			if (buffer->context != buffer->package->shared_glx_context)
			{
				glXDestroyContext(buffer->display, buffer->context);
			}
		}
#  if defined (USE_GLX_PBUFFER)
		if (buffer->glx_pbuffer)
		{
			glXDestroyPbuffer(buffer->display, buffer->glx_pbuffer);
		}
#  else /* defined (USE_GLX_PBUFFER) */
#     if defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer)
			glXDestroyGLXPbufferSGIX(buffer->display, buffer->glx_pbuffer);
#     endif /* defined (GLX_SGIX_dmbuffer) || (GLX_SGIX_pbuffer) */
#  endif /* defined (USE_GLX_PBUFFER) */
#  if defined (USE_GLX_FBCONFIG)
			if (buffer->config_list)
			{
				XFree(buffer->config_list);
			}
#  else /* defined (USE_GLX_FBCONFIG) */
#     if defined (GLX_SGIX_fbconfig)
			if (buffer->config_list)
			{
				XFree(buffer->config_list);
			}
#     endif /* defined (GLX_SGIX_fbconfig) */
#  endif /* defined (USE_GLX_FBCONFIG) */
			if(buffer->visual_info)
			{
				XFree(buffer->visual_info);
			}
			buffer->visual_info = (XVisualInfo *)NULL;
			if(buffer->glx_pixmap)
			{
				glXDestroyGLXPixmap(buffer->display, buffer->glx_pixmap);
			}
			if(buffer->pixmap)
			{
				XFreePixmap(buffer->display, buffer->pixmap);
			}
#endif /* defined (MOTIF_USER_INTERFACE) */
#if defined (GTK_USER_INTERFACE)
#if ! defined (GTK_USE_GTKGLAREA)
			if (buffer->glconfig)
			{
				g_object_unref(buffer->glconfig);
				buffer->glconfig = NULL;
			}

			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->initialise_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->resize_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->expose_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->button_press_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->button_release_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->key_press_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->key_release_handler_id);
			g_signal_handler_disconnect(GTK_OBJECT(buffer->glarea),
				buffer->motion_handler_id);
#endif /* defined (GTK_USE_GTKGLAREA) */
#endif /* defined (GTK_USER_INTERFACE) */

		if (buffer->initialise_callback_list)
		{
			DESTROY(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))(
				&buffer->initialise_callback_list);
		}
		if (buffer->resize_callback_list)
		{
			DESTROY(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))(
				&buffer->resize_callback_list);
		}
		if (buffer->expose_callback_list)
		{
			DESTROY(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_callback)))(
				&buffer->expose_callback_list);
		}
		if (buffer->input_callback_list)
		{
			DESTROY(LIST(CMISS_CALLBACK_ITEM(Graphics_buffer_input_callback)))(
				&buffer->input_callback_list);
		}
#if defined (WIN32_USER_INTERFACE)
		wglMakeCurrent(NULL, NULL);
		/* Only delete it here if it isn't the share context, otherwise we
			will destroy it when the graphics buffer package is destroyed.
			In the intel workaround case we only want to destroy it at the end. */
		if (buffer->hRC && (buffer->hRC != buffer->package->wgl_shared_context))
		{
			wglDeleteContext(buffer->hRC);
		}
		switch (buffer->type)
		{
			case GRAPHICS_BUFFER_WIN32_TYPE:
			{
				if (buffer->hDC)
				{
					ReleaseDC(buffer->hWnd, buffer->hDC);
				}
			} break;
#ifdef WGL_ARB_pbuffer
			case GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE:
			{
				if (buffer->pbuffer && buffer->hDC)
				{
					wglReleasePbufferDCARB(buffer->pbuffer, buffer->hDC);
				}
				if (buffer->pbuffer)
				{
					wglDestroyPbufferARB(buffer->pbuffer);
				}
			} /* no break */
#endif /* defined (WGL_ARB_pbuffer) */
			case GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE:
			{
				if (buffer->device_independent_bitmap)
				{
					DeleteObject(buffer->device_independent_bitmap);
				}
				if (buffer->device_independent_bitmap_hdc)
				{
					DeleteDC(buffer->device_independent_bitmap_hdc);
				}
			} break;
		}
		if (buffer->hidden_accelerated_window)
		{
			DestroyWindow(buffer->hidden_accelerated_window);
		}
		if (buffer->hidden_graphics_buffer)
		{
			DESTROY(Graphics_buffer)(&buffer->hidden_graphics_buffer);
		}
#endif /* defined (WIN32_USER_INTERFACE) */
#if defined (CARBON_USER_INTERFACE)
		aglDisable(buffer->aglContext, AGL_BUFFER_RECT);
		if (buffer->expose_handler_ref)
		{
			RemoveEventHandler(buffer->expose_handler_ref);
		}
		if (buffer->mouse_handler_ref)
		{
			RemoveEventHandler(buffer->mouse_handler_ref);
		}
		if (buffer->resize_handler_ref)
		{
			RemoveEventHandler(buffer->resize_handler_ref);
		}
#endif /* defined (CARBON_USER_INTERFACE) */
#if defined (WX_USER_INTERFACE)
		/* Remove reference to this object in wxGraphicsBuffer */
		if (buffer->canvas)
		{
			buffer->canvas->ClearGraphicsBufferReference();
			delete buffer->canvas;
		}
		if (buffer->attrib_list != NULL)
		{
				DEALLOCATE(buffer->attrib_list);
		}
		if (buffer->type == GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE)
		 {
#if defined (OPENGL_API) && defined (GL_EXT_framebuffer_object)
			 GLint framebuffer_flag;
			 if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
			 {
				 glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_flag);
				 if (framebuffer_flag != 0)
				 {
					 if (buffer->fbo != 0)
					 {
						 if (framebuffer_flag == (GLint)buffer->fbo)
						 {
							 glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
						 }
						 glDeleteFramebuffersEXT(1, &buffer->fbo);
					 }
					 if (buffer->depthbuffer != 0)
					 {
						 glDeleteRenderbuffersEXT(1, &buffer->depthbuffer);
					 }
					 if (buffer->img != 0)
					 {
						 glDeleteTextures(1, &buffer->img);
					 }
				 }
			 }
		 }
#endif /* defined (OPENGL_API) && defined (GL_EXT_framebuffer_object) */
#endif /* defined (WX_USER_INTERFACE) */
		DEALLOCATE(*buffer_ptr);
		*buffer_ptr = (struct Graphics_buffer *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Graphics_buffer).  Missing buffer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Graphics_buffer) */

#if defined (OPENGL_API) && defined (MOTIF_USER_INTERFACE)
int query_glx_extension(char *extName, Display *display, int screen)
/*******************************************************************************
LAST MODIFIED : 14 September 1998

DESCRIPTION :
Search for extName in the GLX extensions string. Use of strstr() is not sufficient
because extension names can be prefixes of other extension names. Could use
strtok() but the constant string returned by glGetString might be in read-only
memory.
???SAB.  Taken directly from above
==============================================================================*/
{
	char *end,*p;
	int extNameLen, n;
	int return_code;

	/* check arguments */
	if (extName)
	{
		extNameLen=strlen(extName);
		p=(char *)glXQueryExtensionsString(display,screen);
		if (NULL==p)
		{
			return_code=0;
		}
		else
		{
			end=p+strlen(p);
			return_code = 0;
			while (p<end)
			{
				n=strcspn(p," ");
				if ((extNameLen==n)&&(strncmp(extName,p,n)==0))
				{
					return_code=1;
				}
				p += (n+1);
			}
		}
	}
	else
	{
		return_code=0;
	}

	return (return_code);
} /* query_glx_extension */
#endif /* defined (OPENGL_API) && defined (MOTIF_USER_INTERFACE) */






