/*
 * NekoGroup Password Helper
 *
 * ng-password-helper.c
 * This file is part of NekoGroup
 *
 * Copyright (C) 2013 - Mike Manilone, license: GPL v3
 *
 * NekoGroup is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * NekoGroup is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <NekoGroup>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <config.h>
#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <libsecret/secret.h>
#include "ng-password-helper.h"

extern int getchar(void);
static gboolean setter_mode = FALSE;
static gchar *db_password = NULL;
static gchar *server_password = NULL;
static gboolean remove = FALSE;

static GOptionEntry entries[] = {
    { "setter-mode", 0, 0, G_OPTION_ARG_NONE, &setter_mode, "Whether you want to set new passwords", NULL },
    { "db-password", 0, 0, G_OPTION_ARG_STRING, &db_password, "The database password you want to set", NULL },
    { "server-password", 0, 0, G_OPTION_ARG_STRING, &server_password, "The server password you want to set", NULL },
    { "remove", 0, 0, G_OPTION_ARG_NONE, &remove, "Remove all saved passwords", NULL }
};

/* keep this the same as the one in ng-config.h */
static const SecretSchema *_get_schema(void)
{
    static const SecretSchema schema = {
        "prj.NekoGroup.Password", SECRET_SCHEMA_NONE,
        {
            { "type", SECRET_SCHEMA_ATTRIBUTE_STRING },
            { "NULL", 0 }
        }
    };
    return &schema;
}

static gchar *_lookup_password(const gchar *type, GError **error)
{
    gchar *result;
    result = NULL;
    result = secret_password_lookup_sync (_get_schema(), NULL,
        error, "type", type, NULL);
    return result;
}

/**
 * _store_password:
 * @type: of "database", "server"
 * @password: (transfer-full): password to be stored. We'll free it 
 * at once.
 * @error: #GError for error reporting
 */
static void _store_password(const gchar *type, gchar *password, 
    GError **error)
{
    secret_password_store_sync(_get_schema(), SECRET_COLLECTION_DEFAULT, 
        "NekoGroup Password", password, NULL, error, "type", type, NULL);
}

gint main(gint argc, gchar*argv[])
{
    GError *error;
    GOptionContext *context;
    error = NULL;
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    textdomain(GETTEXT_PACKAGE);
    context = g_option_context_new(_(" - NekoGroup Password Helper"));
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print(_("Option parsing failed: %s\n"), error->message);
        g_error_free(error);
        return 1;
    }
    if (remove)
    {
        GError *error;
        error = NULL;
        secret_password_clear_sync(_get_schema(), NULL, &error,
            "type", "server", NULL);
        if (error != NULL)
        {
            g_critical(_("Failed to remove the server password: %s"), 
                error->message);
            g_error_free(error);
            return 1;
        }
        secret_password_clear_sync(_get_schema(), NULL, &error,
            "type", "server", NULL);
        if (error != NULL)
        {
            g_critical(_("Failed to remove the server password: %s"), 
                error->message);
            g_error_free(error);
            return 1;
        }
        return 0;
    }
    if (setter_mode)
    {
        GError *error;
        error = NULL;
        if (!server_password || !db_password)
        {
            g_critical(_("Please provide me with two passwords."));
            return 1;
        }
        _store_password("database", db_password, &error);
        if (error)
        {
            g_critical(_("Failed to store a password: %s\n"), 
                error->message);
            g_error_free(error);
            return 1;
        }
        _store_password("server", server_password, &error);
        if (error)
        {
            g_critical(_("Failed to store a password: %s\n"), 
                error->message);
            g_error_free(error);
            return 1;
        }
        return 0;
    }
    else
    {
        gchar *db_password, *server_password;
        GError *error;
        error = NULL;
        g_print(_("This may be unsafe! Press 'Enter' to continue. "
            "Press Ctrl+C to break."));
        getchar();
        db_password = _lookup_password("database", &error);
        if(error)
        {
            g_critical(_("Failed to read password: %s\n"), error->message);
            g_error_free(error);
            return 1;
        }
        server_password = _lookup_password("server", &error);
        if(error)
        {
            g_critical(_("Failed to read password: %s\n"), error->message);
            g_error_free(error);
            return 1;
        }
        g_print(_("Server Password   : %s\n"
                  "Database Password : %s\n"), 
                  server_password?server_password:_("(null)"), 
                  db_password?db_password:_("(null)"));
        g_free(server_password);
        g_free(db_password);
        return 0;
    }
    return 0;
}
