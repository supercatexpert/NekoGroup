/*
 * NekoGroup Utility API Module Declaration
 *
 * ng-utils.c
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
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#include "ng-utils.h"

#define NG_UTILS_MEMBER_NAME_SALT "NekoSalt"

/**
 * SECTION: ng-utils
 * @Short_description: Some utility functions
 * @Title: NGUtils
 * @Include: ng-utils.h
 *
 * This module provides some utility functions for the program.
 */

gchar *ng_utils_get_jid_from_address(const gchar *address)
{
    gchar *jid, *tmp;
    tmp = g_strdup(address);
    tmp = g_strdelimit(tmp, "/", '\0');
    jid = g_strdup(tmp);
    g_free(tmp);
    return jid;
}

gchar *ng_utils_get_resource_from_address(const gchar *address)
{
    gchar *resource = NULL, *tmp;
    tmp = g_strstr_len(address, -1, "/");
    if(tmp!=NULL)
        resource = g_strdup(tmp+1);
    return resource;
}

gchar *ng_utils_get_shortname(const gchar *jid)
{
    gchar *short_name, *tmp;
    tmp = g_strdup(jid);
    tmp = g_strdelimit(tmp, "@", '\0');
    short_name = g_strdup(tmp);
    g_free(tmp);
    return short_name;
}

gchar *ng_utils_generate_new_nick(const gchar *jid)
{
    GChecksum *checksum;
    gchar *short_name;
    gchar *rnick;
    checksum = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(checksum, (const guchar *)jid, strlen(jid));
    g_checksum_update(checksum, (const guchar *)NG_UTILS_MEMBER_NAME_SALT,
        strlen(NG_UTILS_MEMBER_NAME_SALT));
    short_name = ng_utils_get_shortname(jid);
    rnick = g_strdup_printf("%s@%8.8s", short_name,
        g_checksum_get_string(checksum));
    g_free(short_name);
    g_checksum_free(checksum);
    return rnick;
}

gint64 ng_uitls_get_real_time()
{
    GTimeVal tv;
    gint64 real_time;
    g_get_current_time(&tv);
    real_time = (((gint64)tv.tv_sec) * 1000000) + tv.tv_usec;
    return real_time/1000;
}


