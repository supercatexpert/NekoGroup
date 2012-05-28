/*
 * NekoGroup User Database Header Declaration
 *
 * ng-db.h
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

#ifndef HAVE_NG_DB_H
#define HAVE_NG_DB_H

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>

G_BEGIN_DECLS

typedef enum NGDbUserPrivilegeLevel {
    NG_DB_USER_PRIVILEGE_NORMAL,
    NG_DB_USER_PRIVILEGE_POWER,
    NG_DB_USER_PRIVILEGE_ROOT,
    NG_DB_USER_PRIVILEGE_LAST
}NGDbUserPrivilegeLevel;

typedef struct NGDbMemberData NGDbMemberData;
typedef void (*NGDbMemberForeachFunc)(const NGDbMemberData *mem_data,
    gpointer data);

struct NGDbMemberData {
    gchar *jid;
    gchar *nick;
    gint64 rename_freq;
    gint64 msg_count;
    gint64 msg_size;
    gint64 join_time;
    gint64 rename_time;
    gint64 stop_time;
    gint64 ban_time;
    gboolean banned;
    gboolean stopped;
    gboolean allow_pm;
    NGDbUserPrivilegeLevel privilege;
};

gboolean ng_db_init(const gchar *host, gint port,
    const gchar *member_collection, const gchar *log_collection,
    const gchar *user, const gchar *pass);
void ng_db_exit();
guint ng_db_member_foreach(NGDbMemberForeachFunc func, gpointer data);
GHashTable *ng_db_member_get_table();
GHashTable *ng_db_member_search(const gchar *key);
void ng_db_member_data_free(NGDbMemberData *data);
void ng_db_member_data_destroy(NGDbMemberData *data);
gboolean ng_db_member_jid_exist(const gchar *jid);
gboolean ng_db_member_jid_add(const gchar *jid, const gchar *nick);
gboolean ng_db_member_jid_remove(const gchar *jid);
gboolean ng_db_member_set_nick(const gchar *jid, const gchar *nick);
gboolean ng_db_member_set_banned(const gchar *jid, gboolean state,
    gint64 length);
gboolean ng_db_member_set_stopped(const gchar *jid, gboolean state,
    gint64 length);
gboolean ng_db_member_set_privilege_level(const gchar *jid,
    NGDbUserPrivilegeLevel level);
gboolean ng_db_member_set_message_count(const gchar *jid, gint64 count,
    gint64 length);
gboolean ng_db_member_jid_get_data(const gchar *jid,
    NGDbMemberData *member_data);
gboolean ng_db_member_get_nick_name(const gchar *jid, gchar **nick);
gboolean ng_db_member_get_privilege_level(const gchar *jid,
    NGDbUserPrivilegeLevel *level);
gboolean ng_db_member_get_rename_freq(const gchar *jid, gint64 *freq);
gboolean ng_db_member_get_stopped(const gchar *jid, gboolean *stopped,
    gint64 *stop_time);
gboolean ng_db_member_get_allow_pm(const gchar *jid, gboolean *state);
gboolean ng_db_member_nick_exist(const gchar *nick);
gboolean ng_db_member_nick_get_jid(const gchar *nick, gchar **jid);

G_END_DECLS

#endif

