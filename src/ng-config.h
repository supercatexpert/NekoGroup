/*
 * NekoGroup Configuration Header Declaration
 *
 * ng-config.h
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

#ifndef HAVE_NG_CONFIG_H
#define HAVE_NG_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct NGConfigServerData NGConfigServerData;
typedef struct NGConfigDbData NGConfigDbData;

struct NGConfigServerData {
    gchar *server;
    gchar *jid;
    gchar *user;
    gchar *password;
    guint16 port;
    gboolean require_tls;
    gchar *root_id;
    gint timezone;
    gchar *title;
};

struct NGConfigDbData {
    gchar *server;
    guint16 port;
    gchar *user;
    gchar *password;
    gchar *member_collection;
    gchar *log_collection;
};

gboolean ng_config_init(const gchar *argv0);
void ng_config_exit();
const NGConfigServerData *ng_config_get_server_data();
const NGConfigDbData *ng_config_get_db_data();

G_END_DECLS

#endif

