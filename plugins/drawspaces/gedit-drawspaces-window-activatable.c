/*
 * Copyright (C) 2008-2014 Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-drawspaces-window-activatable.h"
#include "gedit-drawspaces-app-activatable.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-utils.h>

#define GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_DRAWSPACES_WINDOW_ACTIVATABLE,		\
				GeditDrawspacesWindowActivatablePrivate))

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditDrawspacesWindowActivatable,
				gedit_drawspaces_window_activatable,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init))

struct _GeditDrawspacesWindowActivatablePrivate
{
	GSettings *settings;
	GeditWindow *window;
	GtkSourceDrawSpacesFlags flags;

	guint enable : 1;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void draw_spaces (GeditDrawspacesWindowActivatable *window_activatable);

static void
on_settings_changed (GSettings                        *settings,
		     const gchar                      *key,
		     GeditDrawspacesWindowActivatable *window_activatable)
{
	window_activatable->priv->flags = g_settings_get_flags (window_activatable->priv->settings,
	                                                        SETTINGS_KEY_DRAW_SPACES);

	draw_spaces (window_activatable);
}

static void
on_show_white_space_changed (GSettings                        *settings,
		             const gchar                      *key,
		             GeditDrawspacesWindowActivatable *window_activatable)
{
	window_activatable->priv->enable = g_settings_get_boolean (settings, key);

	draw_spaces (window_activatable);
}

static void
gedit_drawspaces_window_activatable_init (GeditDrawspacesWindowActivatable *window_activatable)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesWindowActivatable initializing");

	window_activatable->priv = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE_GET_PRIVATE (window_activatable);

	window_activatable->priv->settings = g_settings_new (DRAWSPACES_SETTINGS_BASE);

	g_signal_connect (window_activatable->priv->settings,
	                  "changed::show-white-space",
	                  G_CALLBACK (on_show_white_space_changed),
	                  window_activatable);
	g_signal_connect (window_activatable->priv->settings,
			  "changed::draw-spaces",
			  G_CALLBACK (on_settings_changed),
			  window_activatable);
}

static void
gedit_drawspaces_window_activatable_dispose (GObject *object)
{
	GeditDrawspacesWindowActivatable *window_activatable = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (object);
	GeditDrawspacesWindowActivatablePrivate *priv = window_activatable->priv;

	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesWindowActivatable disposing");

	g_clear_object (&priv->settings);
	g_clear_object (&priv->window);

	G_OBJECT_CLASS (gedit_drawspaces_window_activatable_parent_class)->dispose (object);
}

static void
gedit_drawspaces_window_activatable_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec)
{
	GeditDrawspacesWindowActivatable *window_activatable = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			window_activatable->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_drawspaces_window_activatable_get_property (GObject    *object,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
	GeditDrawspacesWindowActivatable *window_activatable = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, window_activatable->priv->window);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
draw_spaces (GeditDrawspacesWindowActivatable *window_activatable)
{
	GeditDrawspacesWindowActivatablePrivate *priv;
	GList *views, *l;

	priv = window_activatable->priv;

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = g_list_next (l))
	{
		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (l->data),
						 priv->enable ? priv->flags : 0);
	}

	g_list_free (views);
}

static void
tab_added_cb (GeditWindow *window,
	      GeditTab *tab,
	      GeditDrawspacesWindowActivatable *window_activatable)
{
	GeditView *view;

	if (window_activatable->priv->enable)
	{
		view = gedit_tab_get_view (tab);

		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (view),
						 window_activatable->priv->flags);
	}
}

static void
get_config_options (GeditDrawspacesWindowActivatable *window_activatable)
{
	GeditDrawspacesWindowActivatablePrivate *priv = window_activatable->priv;

	priv->enable = g_settings_get_boolean (priv->settings,
					       SETTINGS_KEY_SHOW_WHITE_SPACE);

	priv->flags = g_settings_get_flags (priv->settings,
					    SETTINGS_KEY_DRAW_SPACES);
}

static void
gedit_drawspaces_window_activatable_window_activate (GeditWindowActivatable *activatable)
{
	GeditDrawspacesWindowActivatablePrivate *priv;
	GMenuItem *item;
	GAction *action;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (activatable)->priv;

	get_config_options (GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (activatable));

	action = g_settings_create_action (priv->settings,
	                                   SETTINGS_KEY_SHOW_WHITE_SPACE);
	g_action_map_add_action (G_ACTION_MAP (priv->window),
	                         action);
	g_object_unref (action);

	if (priv->enable)
	{
		draw_spaces (GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (activatable));
	}

	g_signal_connect (priv->window, "tab-added",
			  G_CALLBACK (tab_added_cb), activatable);
}

static void
gedit_drawspaces_window_activatable_window_deactivate (GeditWindowActivatable *activatable)
{
	GeditDrawspacesWindowActivatablePrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (activatable)->priv;

	g_action_map_remove_action (G_ACTION_MAP (priv->window),
	                            SETTINGS_KEY_SHOW_WHITE_SPACE);

	priv->enable = FALSE;
	draw_spaces (GEDIT_DRAWSPACES_WINDOW_ACTIVATABLE (activatable));

	g_signal_handlers_disconnect_by_func (priv->window, tab_added_cb,
					      activatable);
}

static void
gedit_drawspaces_window_activatable_class_init (GeditDrawspacesWindowActivatableClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_drawspaces_window_activatable_dispose;
	object_class->set_property = gedit_drawspaces_window_activatable_set_property;
	object_class->get_property = gedit_drawspaces_window_activatable_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");

	g_type_class_add_private (object_class, sizeof (GeditDrawspacesWindowActivatablePrivate));
}

static void
gedit_drawspaces_window_activatable_class_finalize (GeditDrawspacesWindowActivatableClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_drawspaces_window_activatable_window_activate;
	iface->deactivate = gedit_drawspaces_window_activatable_window_deactivate;
}

void
gedit_drawspaces_window_activatable_register (GTypeModule *module)
{
	gedit_drawspaces_window_activatable_register_type (module);

	peas_object_module_register_extension_type (PEAS_OBJECT_MODULE (module),
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_DRAWSPACES_WINDOW_ACTIVATABLE);
}

/* ex:set ts=8 noet: */