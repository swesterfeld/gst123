/* GLib Extra - Tentative GLib code and GLib supplements
 * Copyright (C) 1997-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
// FIXME: #define	_GNU_SOURCE
#include <string.h>
#include "glib-extra.h"

/* --- GLib main loop reentrant signal queue --- */

static gboolean g_usignal_prepare  (GSource     *source,
				    gint        *timeout);
static gboolean g_usignal_check    (GSource     *source);
static gboolean g_usignal_dispatch (GSource     *source,
                                    GSourceFunc  callback,
				    gpointer     user_data);

static GSourceFuncs usignal_funcs = {
  g_usignal_prepare,
  g_usignal_check,
  g_usignal_dispatch,
  0, // g_free, ?
  0,
  0
};
static	guint32	usignals_notified[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

typedef struct _GUSignalSource
{
  GSource source;
  guint8       index;
  guint8       shift;
  GUSignalFunc callback;
  gpointer     data;
} GUSignalSource;

static gboolean
g_usignal_prepare (GSource  *source,
		   gint     *timeout)
{
  GUSignalSource *usignal_source = (GUSignalSource *)source;
  
  return usignals_notified[usignal_source->index] & (1 << usignal_source->shift);
}

static gboolean
g_usignal_check (GSource *source)
{
  GUSignalSource *usignal_source = (GUSignalSource *)source;
  
  return usignals_notified[usignal_source->index] & (1 << usignal_source->shift);
}

static gboolean
g_usignal_dispatch (GSource    *source,
                    GSourceFunc callback,
		    gpointer    user_data)
{
  GUSignalSource *usignal_source = (GUSignalSource *)source;
  
  usignals_notified[usignal_source->index] &= ~(1 << usignal_source->shift);
  
  //return usignal_data->callback (-128 + usignal_data->index * 32 + usignal_data->shift, user_data);
  return usignal_source->callback (-128 + usignal_source->index * 32 + usignal_source->shift, usignal_source->data);
}

guint
g_usignal_add (gint8	    usignal,
	       GUSignalFunc function,
	       gpointer     data)
{
  return g_usignal_add_full (G_PRIORITY_DEFAULT, usignal, function, data, NULL);
}

guint
g_usignal_add_full (gint           priority,
		    gint8          usignal,
		    GUSignalFunc   function,
		    gpointer       data,
		    GDestroyNotify destroy)
{
  guint s = 128 + usignal;
  
  g_return_val_if_fail (function != NULL, 0);
  
 
  GSource *source = g_source_new (&usignal_funcs, sizeof (GUSignalSource));
  GUSignalSource *usignal_source = (GUSignalSource *) source;
  usignal_source->index = s / 32;
  usignal_source->shift = s % 32;
  usignal_source->callback = function;
  usignal_source->data = data;
  /*
  g_source_set_callback (source, GSourceFunc func,
					                                                    gpointer data,
											                                                 GDestroyNotify notify);
																	 */
  return g_source_attach (source, NULL);
  //return g_source_add (priority, TRUE, &usignal_funcs, usignal_data, data, destroy);
}

void
g_usignal_notify (gint8 usignal)
{
  guint index, shift;
  guint s = 128 + usignal;
  
  index = s / 32;
  shift = s % 32;
  
  usignals_notified[index] |= 1 << shift;
}
