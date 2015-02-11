/*
 * gstgldisplay_wayland.c - Display abstraction for OpenGL/Wayland
 *
 * Copyright (C) 2015 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "gstgldisplay_wayland.h"

G_DEFINE_TYPE (GstGLDisplayWayland, gst_gl_display_wayland,
    GST_TYPE_GL_DISPLAY);

static guintptr
gst_gl_display_wayland_get_handle (GstGLDisplay * base_display)
{
  return GPOINTER_TO_SIZE (GST_GL_DISPLAY_WAYLAND_CAST (base_display)->display);
}

static void
gst_gl_display_wayland_init (GstGLDisplayWayland * display)
{
  GstGLDisplay *const base_display = GST_GL_DISPLAY_CAST (display);

  base_display->type = GST_GL_DISPLAY_TYPE_WAYLAND;
  display->foreign_display = FALSE;
}

static void
gst_gl_display_wayland_finalize (GObject * object)
{
  GstGLDisplayWayland *const display = GST_GL_DISPLAY_WAYLAND_CAST (object);

  if (display->display) {
    if (!display->foreign_display)
      wl_display_disconnect (display->display);
    display->display = NULL;
  }

  G_OBJECT_CLASS (gst_gl_display_wayland_parent_class)->finalize (object);
}

static void
gst_gl_display_wayland_class_init (GstGLDisplayWaylandClass * klass)
{
  GstGLDisplayClass *const base_class = GST_GL_DISPLAY_CLASS (klass);
  GObjectClass *const object_class = G_OBJECT_CLASS (klass);

  base_class->get_handle = gst_gl_display_wayland_get_handle;
  object_class->finalize = gst_gl_display_wayland_finalize;
}

/**
 * gst_gl_display_wayland_new_with_display:
 * @dpy: an existing wayland display
 *
 * Creates a new display connection from a Wayland display.
 *
 * Returns: (transfer full): a new #GstGLDisplayWayland
 */
GstGLDisplayWayland *
gst_gl_display_wayland_new_with_display (struct wl_display * dpy)
{
  GstGLDisplayWayland *display;

  g_return_val_if_fail (dpy != NULL, NULL);

  display = g_object_new (GST_TYPE_GL_DISPLAY_WAYLAND, NULL);
  display->display = dpy;
  display->foreign_display = TRUE;

  return display;
}
