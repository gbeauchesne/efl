/*
 * gstgldisplay_wayland.h - Display abstraction for OpenGL/Wayland
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

#ifndef GST_GL_DISPLAY_WAYLAND_H
#define GST_GL_DISPLAY_WAYLAND_H

#include <gst/gst.h>
#include <gst/gl/gstgldisplay.h>
#include <wayland-client-protocol.h>

G_BEGIN_DECLS

#define GST_TYPE_GL_DISPLAY_WAYLAND \
  (gst_gl_display_wayland_get_type ())
#define GST_GL_DISPLAY_WAYLAND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GL_DISPLAY_WAYLAND, \
   GstGLDisplayWayland))
#define GST_GL_DISPLAY_WAYLAND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GL_DISPLAY_WAYLAND, \
   GstGLDisplayWaylandClass))
#define GST_IS_GL_DISPLAY_WAYLAND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GL_DISPLAY_WAYLAND))
#define GST_IS_GL_DISPLAY_WAYLAND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GL_DISPLAY_WAYLAND))
#define GST_GL_DISPLAY_WAYLAND_CAST(obj) \
  ((GstGLDisplayWayland *) (obj))

typedef struct _GstGLDisplayWayland GstGLDisplayWayland;
typedef struct _GstGLDisplayWaylandClass GstGLDisplayWaylandClass;

/**
 * GstGLDisplayWayland:
 *
 * The contents of a #GstGLDisplayWayland are private and should only
 * be accessed through the provided API
 */
struct _GstGLDisplayWayland
{
  GstGLDisplay parent_instance;

  /*< private >*/
  struct wl_display *display;
  gboolean foreign_display;
};

struct _GstGLDisplayWaylandClass
{
  GstGLDisplayClass parent_class;
};

GType
gst_gl_display_wayland_get_type (void);

GstGLDisplayWayland *
gst_gl_display_wayland_new_with_display (struct wl_display * dpy);

G_END_DECLS

#endif /* GST_GL_DISPLAY_WAYLAND_H */
