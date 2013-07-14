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
 * along with NekoGroup; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
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

static gchar *ng_config_get_program_dir(const gchar *argv0)
{
    gchar *bin_dir = NULL;
    gchar *exec_path = NULL;
    gchar *tmp;
    char full_path[PATH_MAX];
    exec_path = g_file_read_link("/proc/self/exe", NULL);
    if(exec_path!=NULL)
    {
        bin_dir = g_path_get_dirname(exec_path);
        g_free(exec_path);
        exec_path = NULL;
    }
    else exec_path = g_file_read_link("/proc/curproc/file", NULL);
    if(exec_path!=NULL)
    {
        bin_dir = g_path_get_dirname(exec_path);
        g_free(exec_path);
        exec_path = NULL;
    }
    else exec_path = g_file_read_link("/proc/self/path/a.out", NULL);
    if(exec_path!=NULL)
    {
        bin_dir = g_path_get_dirname(exec_path);
        g_free(exec_path);
        exec_path = NULL;
    }
    if(bin_dir==NULL)
    {
        if(g_path_is_absolute(argv0))
            exec_path = g_strdup(argv0);
        else
        {
            bin_dir = g_get_current_dir();
            exec_path = g_build_filename(bin_dir, argv0, NULL);
            g_free(bin_dir);
        }
        strncpy(full_path, exec_path, PATH_MAX-1);
        tmp = g_strdup(exec_path);
        g_free(exec_path);
        exec_path = realpath(tmp, full_path);
        g_free(tmp);
        if(exec_path!=NULL)
            bin_dir = g_path_get_dirname(exec_path);
        else
            bin_dir = NULL;
    }
    if(bin_dir==NULL) bin_dir = g_get_current_dir();
    return bin_dir;
}

static gboolean ng_config_load_config(const gchar *file)
{
    GKeyFile *keyfile;
    gchar *vstring;
    gint vint;
    gboolean vbool;
    gint hour = 0, min = 0;
    if(file==NULL) return FALSE;
    keyfile = g_key_file_new();
    if(!g_key_file_load_from_file(keyfile, file, 0, NULL))
    {
        g_key_file_free(keyfile);
        return FALSE;
    }

    vstring = g_key_file_get_string(keyfile, "Server", "Server", NULL);
    g_free(config_server_data.server);
    config_server_data.server = g_strdup(vstring);
    g_free(vstring);
    
    vstring = g_key_file_get_string(keyfile, "Server", "JID", NULL);
    g_free(config_server_data.jid);
    config_server_data.jid = g_strdup(vstring);
    g_free(vstring);
    
    vstring = g_key_file_get_string(keyfile, "Server", "User", NULL);
    g_free(config_server_data.user);
    config_server_data.user = g_strdup(vstring);
    g_free(vstring); 

    vstring = g_key_file_get_string(keyfile, "Server", "Password", NULL);
    g_free(config_server_data.password);
    config_server_data.password = g_strdup(vstring);
    g_free(vstring);    
    
    vint = g_key_file_get_integer(keyfile, "Server", "Port", NULL);
    config_server_data.port = vint;
    
    vbool = g_key_file_get_boolean(keyfile, "Server", "RequireTLS", NULL);
    config_server_data.require_tls = vbool;

    vstring = g_key_file_get_string(keyfile, "Server", "RootID", NULL);
    g_free(config_server_data.root_id);
    config_server_data.root_id = g_strdup(vstring);
    g_free(vstring);

    vstring = g_key_file_get_string(keyfile, "Server", "Title", NULL);
    g_free(config_server_data.title);
    config_server_data.title = g_strdup(vstring);
    g_free(vstring); 
    
    vstring = g_key_file_get_string(keyfile, "Server", "TimeZone", NULL);
    sscanf(vstring, "%d:%d", &hour, &min);
    config_server_data.timezone = hour * 60 + min;
    g_free(vstring);
    
    vstring = g_key_file_get_string(keyfile, "Database", "Server", NULL);
    g_free(config_db_data.server);
    config_db_data.server = g_strdup(vstring);
    g_free(vstring);

    vint = g_key_file_get_integer(keyfile, "Database", "Port", NULL);
    config_db_data.port = vint;

    vstring = g_key_file_get_string(keyfile, "Database", "User", NULL);
    g_free(config_db_data.user);
    config_db_data.user = g_strdup(vstring);
    g_free(vstring); 

    vstring = g_key_file_get_string(keyfile, "Database", "Password", NULL);
    g_free(config_db_data.password);
    config_db_data.password = g_strdup(vstring);
    g_free(vstring); 

    vstring = g_key_file_get_string(keyfile, "Database", "MemberCollection",
        NULL);
    g_free(config_db_data.member_collection);
    config_db_data.member_collection = g_strdup(vstring);
    g_free(vstring);

    vstring = g_key_file_get_string(keyfile, "Database", "LogCollection",
        NULL);
    g_free(config_db_data.log_collection);
    config_db_data.log_collection = g_strdup(vstring);
    g_free(vstring);

    g_key_file_free(keyfile);
    return TRUE;
}

gboolean ng_config_init(const gchar *argv0)
{
    gchar *prog_dir;
    gchar *conf_file = NULL;
    gboolean flag = FALSE;
    prog_dir = ng_config_get_program_dir(argv0);
    if(prog_dir!=NULL)
    {
        conf_file = g_build_filename(prog_dir, "data", "config.ini", NULL);
        g_free(prog_dir);
    }
    if(conf_file!=NULL)
    {
        flag = ng_config_load_config(conf_file);
        g_free(conf_file);
    }
    if(!flag)
    {
        conf_file = g_strdup("/etc/nekogroup.conf");
        flag = ng_config_load_config(conf_file);
        g_free(conf_file);
    }
    return flag;
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





