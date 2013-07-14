/*
 * NekoGroup Configuaration Module Declaration
 *
 * ng-config.c
 * This file is part of NekoGroup
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
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

/*
 * Modified by Mike Manilone <crtmike@gmx.us>
 */

#include <gio/gio.h>
#include <libsecret/secret.h>
#include "ng-config.h"
#include "ng-common.h"

/**
 * SECTION: ng-config
 * @Short_description: The configuration module
 * @Title: NGConfig
 * @Include: ng-config.h
 *
 * This module manages the configuration data of this program.
 */

static NGConfigServerData config_server_data = {0};
static NGConfigDbData config_db_data = {0};
static NGConfigNormalData config_normal_data = {0};

static const SecretSchema *_get_schema (void)
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

static gint _build_timezone(gchar *string, gboolean freep)
{
    gint hour, min;
    sscanf(string, "%d:%d", &hour, &min);
    if (freep)
        g_free (string);
    return hour*60+min;
}

static gboolean ng_config_load_config()
{
    GSettings *settings;
    
    settings = g_settings_new("prj.NekoGroup.Server");
    
    g_free(config_server_data.server);
    config_server_data.server = g_settings_get_string(settings, 
        "server");
    
    g_free(config_server_data.jid);
    config_server_data.jid = g_settings_get_string(settings, "jid");
    
    g_free(config_server_data.user);
    config_server_data.user = g_settings_get_string(settings, "user");
    
    config_server_data.unsafe = g_settings_get_boolean(settings, "unsafe");
    
    g_free(config_server_data.password);
    if (config_server_data.unsafe)
    {
        config_server_data.password = g_settings_get_string(settings, 
            "password");
    }
    
    config_server_data.port = g_settings_get_int(settings, "port");
    
    config_server_data.require_tls = g_settings_get_boolean(settings,
        "require-tls");
    
    g_free(config_server_data.root_id);
    config_server_data.root_id = g_settings_get_string(settings,
        "root-jid");
    
    g_free(config_server_data.title);
    config_server_data.title = g_settings_get_string(settings,
        "title");
    
    config_server_data.timezone = _build_timezone (g_settings_get_string(settings,
        "timezone"), TRUE);
    
    g_object_unref(settings);
    settings = g_settings_new("prj.NekoGroup.Database");
    
    g_free(config_db_data.server);
    config_db_data.server = g_settings_get_string(settings, "server");
    
    config_db_data.port = g_settings_get_int(settings, "port");
    
    g_free(config_db_data.user);
    config_db_data.user = g_settings_get_string(settings, "user");
    
    g_free(config_db_data.password);
    if(config_server_data.unsafe)
    {
        config_db_data.password = g_settings_get_string(settings, 
            "password");
    }
    
    g_free(config_db_data.member_collection);
    config_db_data.member_collection = g_settings_get_string(settings,
        "member-collection");
    g_free(config_db_data.log_collection);
    config_db_data.log_collection = g_settings_get_string(settings,
        "log-collection");
    
    g_object_unref(settings);
    settings = g_settings_new("prj.NekoGroup.Normal");
    
    config_normal_data.renick_timelimit = g_settings_get_int(settings, 
        "renick-timelimit");
    
    g_object_unref(settings);
    return TRUE;
}

gboolean ng_config_init(const gchar *argv0)
{
    ng_config_load_config();
    return TRUE;
}

void ng_config_exit()
{
    g_free(config_server_data.server);
    g_free(config_server_data.jid);
    g_free(config_server_data.user);
    g_free(config_server_data.password);
    g_free(config_server_data.root_id);
    g_free(config_server_data.title);
    memset(&config_server_data, 0, sizeof(NGConfigServerData));
    g_free(config_db_data.server);
    g_free(config_db_data.user);
    g_free(config_db_data.password);
    g_free(config_db_data.member_collection);
    g_free(config_db_data.log_collection);
    memset(&config_db_data, 0, sizeof(NGConfigDbData));
}

const NGConfigServerData *ng_config_get_server_data()
{
    return &config_server_data;
}

const NGConfigDbData *ng_config_get_db_data()
{
    return &config_db_data;
}

const NGConfigNormalData *ng_config_get_normal_data()
{
    return &config_normal_data;
}

/**
 * ng_config_get_server_password_safely:
 * 
 * Obtain server password from libsecret.
 * 
 * Return value: (transfer-full): A newly-allocated string holding 
 * the password. For your safety, free it as soon as possible.
 */
gchar *ng_config_get_server_password_safely(void)
{
    GError *error;
    gchar *password, *retval;
    error = NULL;
    password = secret_password_lookup_sync(_get_schema(),
        NULL, &error, "type", "server", NULL);
    retval = g_strdup(password);
    secret_password_free(password);
    return retval;
}

/**
 * ng_config_get_database_password_safely:
 * 
 * Obtain database password from libsecret.
 * 
 * Return value: (transfer-full): A newly-allocated string holding 
 * the password. For your safety, free it as soon as possible.
 */
gchar *ng_config_get_database_password_safely(void)
{
    GError *error;
    gchar *password, *retval;
    error = NULL;
    password = secret_password_lookup_sync(_get_schema(),
        NULL, &error, "type", "database", NULL);
    retval = g_strdup(password);
    secret_password_free(password);
    return retval;
}
