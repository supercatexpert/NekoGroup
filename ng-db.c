/*
 * NekoGroup User Database Module Declaration
 *
 * ng-db.c
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

#include <mongo.h>
#include "ng-db.h"
#include "ng-common.h"
#include "ng-core.h"
#include "ng-bot.h"
#include "ng-config.h"

#define NG_DB_DATABASE_NAME "NekoGroupDb"

typedef struct NGDbMemberForeachData {
    NGDbMemberForeachFunc func;
    gpointer data;
}NGDbMemberForeachData;

typedef struct NGDbPrivate {
    mongo_sync_connection *db_connection;
    gchar *member_collection;
    gchar *log_collection;
}NGDbPrivate;

static NGDbPrivate ng_db_priv = {0};

static inline bson *ng_db_member_bson_build(const NGDbMemberData *member_data)
{
    bson *doc;
    doc = bson_build(BSON_TYPE_STRING, "_id", member_data->jid, -1,
        BSON_TYPE_STRING, "jid", member_data->jid, -1,
        BSON_TYPE_STRING, "nick", member_data->nick, -1,
        BSON_TYPE_INT64, "rename_freq", member_data->rename_freq,
        BSON_TYPE_INT64, "msg_count", member_data->msg_count,
        BSON_TYPE_INT64, "msg_size", member_data->msg_size,
        BSON_TYPE_UTC_DATETIME, "join_time", member_data->join_time,
        BSON_TYPE_UTC_DATETIME, "rename_time", member_data->rename_time,
        BSON_TYPE_UTC_DATETIME, "stop_time", member_data->stop_time,
        BSON_TYPE_UTC_DATETIME, "ban_time", member_data->ban_time,
        BSON_TYPE_BOOLEAN, "banned", member_data->banned,
        BSON_TYPE_BOOLEAN, "stopped", member_data->stopped,
        BSON_TYPE_BOOLEAN, "allow_pm", member_data->allow_pm,
        BSON_TYPE_INT32, "privilege", member_data->privilege,
        BSON_TYPE_NONE);
    bson_finish(doc);
    return doc;
}

static inline void ng_db_member_data_from_bson(const bson *doc,
    NGDbMemberData *member_data)
{
    bson_cursor *cursor;
    const gchar *vstring;
    gint32 vint;
    cursor = bson_find(doc, "jid");
    if(cursor!=NULL)
    {
        if(bson_cursor_get_string(cursor, &vstring))
            member_data->jid = g_strdup(vstring);
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "nick");
    if(cursor!=NULL)
    {
        if(bson_cursor_get_string(cursor, &vstring))
            member_data->nick = g_strdup(vstring);
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "rename_freq");
    if(cursor!=NULL)
    {
        bson_cursor_get_int64(cursor, &(member_data->rename_freq));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "msg_count");
    if(cursor!=NULL)
    {
        bson_cursor_get_int64(cursor, &(member_data->msg_count));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "msg_size");
    if(cursor!=NULL)
    {
        bson_cursor_get_int64(cursor, &(member_data->msg_size));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "join_time");
    if(cursor!=NULL)
    {
        bson_cursor_get_utc_datetime(cursor, &(member_data->join_time));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "rename_time");
    if(cursor!=NULL)
    {
        bson_cursor_get_utc_datetime(cursor, &(member_data->rename_time));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "stop_time");
    if(cursor!=NULL)
    {
        bson_cursor_get_utc_datetime(cursor, &(member_data->stop_time));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "ban_time");
    if(cursor!=NULL)
    {
        bson_cursor_get_utc_datetime(cursor, &(member_data->ban_time));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "banned");
    if(cursor!=NULL)
    {
        bson_cursor_get_boolean(cursor, &(member_data->banned));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "stopped");
    if(cursor!=NULL)
    {
        bson_cursor_get_boolean(cursor, &(member_data->stopped));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "allow_pm");
    if(cursor!=NULL)
    {
        bson_cursor_get_boolean(cursor, &(member_data->allow_pm));
        bson_cursor_free(cursor);
    }
    cursor = bson_find(doc, "privilege");
    if(cursor!=NULL)
    {
        if(bson_cursor_get_int32(cursor, &vint))
            member_data->privilege = vint;
        bson_cursor_free(cursor);
    }  
}

gboolean ng_db_init(const gchar *host, gint port,
    const gchar *member_collection, const gchar *log_collection,
    const gchar *user, const gchar *pass)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_sync_connection *connection;
    bson *doc;
    if(priv->db_connection!=NULL)
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
            "Connect to database already!");
        return FALSE;
    }
    connection = mongo_sync_connect(host, port, TRUE);
    if(connection==NULL)
    {
        g_warning("Cannot connect to mongo database!");
        return FALSE;
    }
    if(user!=NULL && pass!=NULL)
    {
        if(!mongo_sync_cmd_authenticate(connection, NG_DB_DATABASE_NAME,
            user, pass))
        {
            g_warning("Cannot authenticate the user with database!");
            mongo_sync_disconnect(connection);
            return FALSE;
        }
        g_message("Authenticated the user with database.");
    }
    doc = mongo_sync_cmd_exists(connection, NG_DB_DATABASE_NAME,
        member_collection);
    if(doc!=NULL)
    {
        g_message("Member collection exists.");
        bson_free(doc);
    }
    else
    {
        if(!mongo_sync_cmd_create(connection, NG_DB_DATABASE_NAME,
            member_collection, MONGO_COLLECTION_DEFAULTS))
        {
            g_warning("Cannot create member collection %s!",
                member_collection);
            mongo_sync_disconnect(connection);
            return FALSE;
        }
    }
    doc = mongo_sync_cmd_exists(connection, NG_DB_DATABASE_NAME,
        log_collection);
    if(doc!=NULL)
    {
        g_message("Log collection exists.");
        bson_free(doc);
    }
    else
    {
        if(!mongo_sync_cmd_create(connection, NG_DB_DATABASE_NAME,
            log_collection, MONGO_COLLECTION_DEFAULTS))
        {
            g_warning("Cannot create log collection %s!",
                log_collection);
            mongo_sync_disconnect(connection);
            return FALSE;
        }
    }
    priv->db_connection = connection;
    priv->member_collection = g_strdup(member_collection);
    priv->log_collection = g_strdup(log_collection);
    return TRUE;
}

void ng_db_exit()
{
    NGDbPrivate *priv = &ng_db_priv;
    g_free(priv->member_collection);
    g_free(priv->log_collection);
    if(priv->db_connection!=NULL)
    {
        mongo_sync_disconnect(priv->db_connection);
        priv->db_connection = NULL;
    }
}

guint ng_db_member_foreach(NGDbMemberForeachFunc func, gpointer data)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    bson *result;
    gchar *ns;
    guint count = 0;
    NGDbMemberData member_data;
    if(priv->db_connection==NULL) return 0;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_new();
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 0,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return 0;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return 0;
    while(mongo_sync_cursor_next(cursor))
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL) continue;
        memset(&member_data, 0, sizeof(NGDbMemberData));
        ng_db_member_data_from_bson(result, &member_data);
        func(&member_data, data);
        ng_db_member_data_free(&member_data);
        bson_free(result);
        count++;
    }
    mongo_sync_cursor_free(cursor);
    return count;
}

GHashTable *ng_db_member_get_table()
{
    GHashTable *table = NULL;
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    bson *result;
    gchar *ns;
    guint count = 0;
    NGDbMemberData *member_data = NULL;
    if(priv->db_connection==NULL) return NULL;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_new();
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 0,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return NULL;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return NULL;
    table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)ng_db_member_data_destroy);
    while(mongo_sync_cursor_next(cursor))
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL) continue;
        member_data = g_new0(NGDbMemberData, 1);
        ng_db_member_data_from_bson(result, member_data);
        if(member_data->jid!=NULL)
        {
            g_hash_table_insert(table,
                g_strdup(member_data->jid), member_data);
        }
        else
            ng_db_member_data_destroy(member_data);
        bson_free(result);
        count++;
    }
    mongo_sync_cursor_free(cursor);
    return table;
}

GHashTable *ng_db_member_search(const gchar *key)
{
    GHashTable *table = NULL;
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    bson *result;
    gchar *ns;
    guint count = 0;
    gchar *query_regex;
    NGDbMemberData *member_data = NULL;
    if(priv->db_connection==NULL) return NULL;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query_regex = g_regex_escape_string(key, -1);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_REGEXP, "nick", query_regex, "i",
        BSON_TYPE_NONE), BSON_TYPE_NONE);
    g_free(query_regex);
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 0,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return NULL;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return NULL;
    table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)ng_db_member_data_destroy);
    while(mongo_sync_cursor_next(cursor))
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL) continue;
        member_data = g_new0(NGDbMemberData, 1);
        ng_db_member_data_from_bson(result, member_data);
        if(member_data->jid!=NULL)
        {
            g_hash_table_insert(table,
                g_strdup(member_data->jid), member_data);
        }
        else
            ng_db_member_data_destroy(member_data);
        bson_free(result);
        count++;
    }
    mongo_sync_cursor_free(cursor);
    return table;
}

void ng_db_member_data_free(NGDbMemberData *data)
{
    if(data==NULL) return;
    g_free(data->jid);
    g_free(data->nick);
    data->jid = NULL;
    data->nick = NULL;
}

void ng_db_member_data_destroy(NGDbMemberData *data)
{
    if(data==NULL) return;
    ng_db_member_data_free(data);
    g_free(data);
}

gboolean ng_db_member_jid_exist(const gchar *jid)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_jid_add(const gchar *jid, const gchar *nick)
{
    NGDbPrivate *priv = &ng_db_priv;
    NGDbMemberData member_data = {0};
    bson *member_doc;
    gchar *ns;
    gboolean flag;
    if(jid==NULL) return FALSE;
    if(priv->db_connection==NULL) return FALSE;
    member_data.jid = g_strdup(jid);
    if(nick==NULL)
    {
        member_data.nick = ng_bot_generate_new_nick(jid);
    }
    else
        member_data.nick = g_strdup(nick);
    member_data.join_time = g_get_real_time()/1000;
    member_data.allow_pm = TRUE;
    member_doc = ng_db_member_bson_build(&member_data);
    ng_db_member_data_free(&member_data);
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    flag = mongo_sync_cmd_insert(priv->db_connection, ns, member_doc, NULL);
    g_free(ns);
    bson_free(member_doc);
    if(!flag)
    {
        g_warning("Cannot add JID %s to member database!", jid);
        return FALSE;
    }
    return TRUE;
}

gboolean ng_db_member_jid_remove(const gchar *jid)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    flag = mongo_sync_cmd_delete(priv->db_connection, ns, 0, select);
    bson_free(select);
    return flag;
}

gboolean ng_db_member_set_nick(const gchar *jid, const gchar *nick)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select, *update;
    gchar *ns;
    gint64 freq = 0;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL || nick==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    if(ng_db_member_get_rename_freq(jid, &freq)) freq++;
    update = bson_build_full(BSON_TYPE_DOCUMENT, "$set", TRUE,
        bson_build(BSON_TYPE_STRING, "nick", nick, -1, 
        BSON_TYPE_INT64, "rename_freq", freq,
        BSON_TYPE_UTC_DATETIME, "rename_time", g_get_real_time()/1000,
        BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(update);
    flag = mongo_sync_cmd_update(priv->db_connection, ns, 0, select, update);
    bson_free(select);
    bson_free(update);
    return flag;
}

gboolean ng_db_member_set_banned(const gchar *jid, gboolean state,
    gint64 length)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select, *update;
    gchar *ns;
    gint64 ban_time = 0;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    if(state)
        ban_time = g_get_real_time()/1000 + length;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    update = bson_build_full(BSON_TYPE_DOCUMENT, "$set", TRUE,
        bson_build(BSON_TYPE_UTC_DATETIME, "ban_time", ban_time,
        BSON_TYPE_BOOLEAN, "banned", state, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(update);
    flag = mongo_sync_cmd_update(priv->db_connection, ns, 0, select, update);
    bson_free(select);
    bson_free(update);
    return flag;
}

gboolean ng_db_member_set_stopped(const gchar *jid, gboolean state,
    gint64 length)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select, *update;
    gchar *ns;
    gint64 stop_time = 0;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    if(state)
        stop_time = g_get_real_time()/1000 + length;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    update = bson_build_full(BSON_TYPE_DOCUMENT, "$set", TRUE,
        bson_build(BSON_TYPE_UTC_DATETIME, "stop_time", stop_time,
        BSON_TYPE_BOOLEAN, "stopped", state, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(update);
    flag = mongo_sync_cmd_update(priv->db_connection, ns, 0, select, update);
    bson_free(select);
    bson_free(update);
    return flag;
}

gboolean ng_db_member_set_privilege_level(const gchar *jid,
    NGDbUserPrivilegeLevel level)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select, *update;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    if(level>=NG_DB_USER_PRIVILEGE_LAST) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    update = bson_build_full(BSON_TYPE_DOCUMENT, "$set", TRUE,
        bson_build(BSON_TYPE_INT64, "privilege", level, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(update);
    flag = mongo_sync_cmd_update(priv->db_connection, ns, 0, select, update);
    bson_free(select);
    bson_free(update);
    return flag;
}

gboolean ng_db_member_set_message_count(const gchar *jid, gint64 count,
    gint64 length)
{
    NGDbPrivate *priv = &ng_db_priv;
    bson *select, *update;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    select = bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE);
    bson_finish(select);
    update = bson_build_full(BSON_TYPE_DOCUMENT, "$set", TRUE,
        bson_build(BSON_TYPE_INT64, "msg_count", count,
        BSON_TYPE_INT64, "msg_size", length, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(update);
    flag = mongo_sync_cmd_update(priv->db_connection, ns, 0, select, update);
    bson_free(select);
    bson_free(update);
    return flag;
}

gboolean ng_db_member_jid_get_data(const gchar *jid,
    NGDbMemberData *member_data)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    bson *result;
    gchar *ns;
    gboolean flag;
    const NGConfigServerData *conf_server_data;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            conf_server_data = ng_config_get_server_data();
            ng_db_member_data_from_bson(result, member_data);
            if(conf_server_data!=NULL)
            {
                if(g_strcmp0(jid, conf_server_data->root_id)==0)
                {
                    member_data->privilege = NG_DB_USER_PRIVILEGE_ROOT;
                }
            }
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_get_nick_name(const gchar *jid, gchar **nick)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    const gchar *vstring;
    if(priv->db_connection==NULL || jid==NULL || nick==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "nick", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "nick");
            if(bcursor!=NULL)
            {
                bson_cursor_get_string(bcursor, &vstring);
                if(vstring!=NULL)
                    *nick = g_strdup(vstring);
                bson_cursor_free(bcursor);
            }
            else flag = FALSE;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_get_privilege_level(const gchar *jid,
    NGDbUserPrivilegeLevel *level)
{
    NGDbPrivate *priv = &ng_db_priv;
    const NGConfigServerData *conf_server_data;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    gint32 vint;
    if(priv->db_connection==NULL || jid==NULL || level==NULL) return FALSE;
    conf_server_data = ng_config_get_server_data();
    if(conf_server_data!=NULL)
    {
        if(g_strcmp0(jid, conf_server_data->root_id)==0)
        {
            *level = NG_DB_USER_PRIVILEGE_ROOT;
            return TRUE;
        }
    }
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "privilege", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "privilege");
            if(bcursor!=NULL)
            {
                bson_cursor_get_int32(bcursor, &vint);
                *level = vint;
                bson_cursor_free(bcursor);
            }
            else flag = FALSE;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_get_rename_freq(const gchar *jid, gint64 *freq)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL || freq==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "rename_freq", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "rename_freq");
            if(bcursor!=NULL)
            {
                bson_cursor_get_int64(bcursor, freq);
                bson_cursor_free(bcursor);
            }
            else flag = FALSE;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_get_stopped(const gchar *jid, gboolean *stopped,
    gint64 *stop_time)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    gint64 rstime = 0;
    gboolean rsstate = 0;
    if(priv->db_connection==NULL || jid==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "stopped", 1, BSON_TYPE_INT32,
        "stop_time", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "stopped");
            if(bcursor!=NULL)
            {
                bson_cursor_get_boolean(bcursor, &rsstate);
                bson_cursor_free(bcursor);
            }
            bcursor = bson_find(result, "stop_time");
            if(bcursor!=NULL)
            {
                bson_cursor_get_utc_datetime(bcursor, &rstime);
                bson_cursor_free(bcursor);
            }
            if(stopped!=NULL) *stopped = rsstate;
            if(stop_time!=NULL) *stop_time = rstime;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_get_allow_pm(const gchar *jid, gboolean *state)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || jid==NULL || state==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "_id", jid, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "allow_pm", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "allow_pm");
            if(bcursor!=NULL)
            {
                bson_cursor_get_boolean(bcursor, state);
                bson_cursor_free(bcursor);
            }
            else flag = FALSE;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_nick_exist(const gchar *nick)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson *query;
    gchar *ns;
    gboolean flag;
    if(priv->db_connection==NULL || nick==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "nick", nick, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, NULL);
    bson_free(query);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    mongo_sync_cursor_free(cursor);
    return flag;
}

gboolean ng_db_member_nick_get_jid(const gchar *nick, gchar **jid)
{
    NGDbPrivate *priv = &ng_db_priv;
    mongo_packet *packet;
    mongo_sync_cursor *cursor;
    bson_cursor *bcursor;
    bson *query, *select;
    bson *result;
    gchar *ns;
    gboolean flag;
    const gchar *vstring;
    if(priv->db_connection==NULL || jid==NULL || nick==NULL) return FALSE;
    ns = g_strconcat(NG_DB_DATABASE_NAME, ".", priv->member_collection,
        NULL);
    query = bson_build_full(BSON_TYPE_DOCUMENT, "$query", TRUE,
        bson_build(BSON_TYPE_STRING, "nick", nick, -1, BSON_TYPE_NONE),
        BSON_TYPE_NONE);
    bson_finish(query);
    select = bson_build(BSON_TYPE_INT32, "_id", 1, BSON_TYPE_NONE);
    bson_finish(select);
    packet = mongo_sync_cmd_query(priv->db_connection, ns, 0, 0, 1,
        query, select);
    bson_free(query);
    bson_free(select);
    if(packet==NULL)
    {
        g_free(ns);
        return FALSE;
    }
    cursor = mongo_sync_cursor_new(priv->db_connection, ns, packet);
    g_free(ns);
    if(cursor==NULL) return FALSE;
    flag = mongo_sync_cursor_next(cursor);
    if(flag)
    {
        result = mongo_sync_cursor_get_data(cursor);
        if(result==NULL)
        {
            flag = FALSE;
        }
        else
        {
            bcursor = bson_find(result, "_id");
            if(bcursor!=NULL)
            {
                bson_cursor_get_string(bcursor, &vstring);
                if(vstring!=NULL)
                    *jid = g_strdup(vstring);
                bson_cursor_free(bcursor);
            }
            else flag = FALSE;
            bson_free(result);
        }
    }
    mongo_sync_cursor_free(cursor);
    return flag;
}

