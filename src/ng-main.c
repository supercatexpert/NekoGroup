/*
 * NekoGroup Main Database Module Declaration
 *
 * ng-main.c
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

#include <glib/gprintf.h>
#include "ng-main.h"
#include "ng-common.h"
#include "ng-core.h"
#include "ng-db.h"
#include "ng-config.h"
#include "ng-bot.h"

static GMainLoop *main_loop = NULL;

gboolean ng_main_run(gint *argc, gchar **argv[])
{
    const NGConfigServerData *server_data;
    const NGConfigDbData *db_data;
    g_type_init();
    if(!ng_config_init(*argv[0]))
    {
        g_error("Cannot load configuration data!");
        return FALSE;
    }
    server_data = ng_config_get_server_data();
    db_data = ng_config_get_db_data();
    if(!ng_db_init(db_data->server, db_data->port, db_data->member_collection,
        db_data->log_collection, db_data->user, db_data->password))
    {
        g_error("Cannot initialize database!");
        return FALSE;
    }    
    if(ng_core_init(server_data->server, server_data->port,
        server_data->require_tls, server_data->jid, server_data->user,
        server_data->password)!=0)
    {
        g_error("Cannot initialize communiation core!");
        return FALSE;
    }
    ng_core_set_title(server_data->title);
    ng_bot_init();
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);
    return TRUE;
}

void ng_main_exit()
{
    ng_bot_exit();
    ng_core_exit();
    ng_db_exit();
    ng_config_exit();
}

void ng_main_quit()
{
    g_main_loop_quit(main_loop);
}


