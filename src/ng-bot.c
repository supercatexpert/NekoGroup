/*
 * NekoGroup Bot Kernel Module Declaration
 *
 * ng-bot.c
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
 
#include "ng-bot.h"
#include "ng-common.h"
#include "ng-core.h"
#include "ng-db.h"
#include "ng-cmd.h"
#include "ng-utils.h"

static GHashTable *bot_member_table = NULL;

static void ng_bot_core_message_cb(NGCore *core, const gchar *from,
    const gchar *type, const gchar *body, gpointer data)
{
    const gchar *text;
    if(g_strcmp0(type, "chat")!=0)
    {
        return;
    }
    text = body;
    if(text==NULL) return;
    if(g_str_has_prefix(text, "-"))
    {
        /* Commands! */
        if(!ng_cmd_exec(from, text))
        {
            ng_core_send_message(from, _("Invalid command!"));
        }
        return;
    }
    ng_bot_broadcast(from, text);
}

static void ng_bot_core_receive_roster_cb(NGCore *core, const gchar *jid,
    const gchar *subscription, gpointer data)
{
    NGBotMemberData *member_data;
    if(bot_member_table==NULL || jid==NULL) return;
    member_data = g_new0(NGBotMemberData, 1);
    member_data->jid = g_strdup(jid);
    member_data->subscription = g_strdup(subscription);
    member_data->online = TRUE;
    g_hash_table_insert(bot_member_table, g_strdup(jid), member_data);
}

static void ng_bot_core_buddy_presence_cb(NGCore *core, const gchar *jid,
    const gchar *resource, const gchar *status, gpointer data)
{
    NGBotMemberData *member_data;
    if(bot_member_table==NULL || jid==NULL) return;
    member_data = g_hash_table_lookup(bot_member_table, jid);
    if(member_data!=NULL)
    {
        if(resource!=NULL)
        {
            g_free(member_data->resource);
            member_data->resource = g_strdup(resource);
        }
        g_free(member_data->status);
        member_data->status = g_strdup(status);
        member_data->online = TRUE;
    }
    else
    {
        member_data = g_new0(NGBotMemberData, 1);
        member_data->jid = g_strdup(jid);
        member_data->resource = g_strdup(resource);
        member_data->status = g_strdup(status);
        member_data->online = TRUE;
        g_hash_table_insert(bot_member_table, g_strdup(jid), member_data);
    }
}

static void ng_bot_core_buddy_unavailable(NGCore *core, const gchar *jid,
    gpointer data)
{
    NGBotMemberData *member_data;
    if(bot_member_table==NULL || jid==NULL) return;
    member_data = g_hash_table_lookup(bot_member_table, jid);
    if(member_data==NULL) return;
    g_free(member_data->resource);
    g_free(member_data->status);
    g_free(member_data->subscription);
    member_data->resource = NULL;
    member_data->status = NULL;
    member_data->subscription = NULL;
    member_data->online = FALSE;
}

static void ng_bot_core_subscribe_request_cb(NGCore *core, const gchar *jid,
    gpointer data)
{
    gchar *nick, *text;
    if(jid==NULL) return;
    nick = ng_utils_generate_new_nick(jid);
    text = g_strdup_printf(_("%s wants to join this group."), nick);
    ng_core_add_roster(jid);
    ng_db_member_jid_add(jid, nick);
    g_free(nick);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    ng_core_send_subscribed_message(jid);

}
static void ng_bot_core_subscribed_cb(NGCore *core, const gchar *jid,
    gpointer data)
{
    gchar *nick, *text;
    if(jid==NULL) return;
    nick = ng_utils_generate_new_nick(jid);
    text = g_strdup_printf(_("%s has been added to this group."), nick);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    g_free(nick);
}

static void ng_bot_core_unsubscribe_request_cb(NGCore *core,
    const gchar *jid, gpointer data)
{
    gchar *nick, *text;
    if(!ng_db_member_get_nick_name(jid, &nick))
        nick = ng_utils_generate_new_nick(jid);
    text = g_strdup_printf("%s has left from this group, good bye!", nick);
    g_free(nick);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    ng_core_remove_roster(jid);
    ng_db_member_jid_remove(jid);
    if(bot_member_table!=NULL)
        g_hash_table_remove(bot_member_table, jid);
    ng_core_send_unsubscribed_message(jid);
}

static void ng_bot_core_unsubscribed_cb(NGCore *core, const gchar *jid,
    gpointer data)
{

}

static void ng_bot_member_data_free(NGBotMemberData *member_data)
{
    if(member_data==NULL) return;
    g_free(member_data->jid);
    g_free(member_data->subscription);
    g_free(member_data->resource);
    g_free(member_data->status);
    g_free(member_data);
}

void ng_bot_init()
{
    bot_member_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)ng_bot_member_data_free);
    ng_core_signal_connect("message",
        G_CALLBACK(ng_bot_core_message_cb), NULL);
    ng_core_signal_connect("receive-roster",
        G_CALLBACK(ng_bot_core_receive_roster_cb), NULL);
    ng_core_signal_connect("buddy-presence",
        G_CALLBACK(ng_bot_core_buddy_presence_cb), NULL);
    ng_core_signal_connect("buddy-unavailable",
        G_CALLBACK(ng_bot_core_buddy_unavailable), NULL);
    ng_core_signal_connect("subscribe-request",
        G_CALLBACK(ng_bot_core_subscribe_request_cb), NULL);
    ng_core_signal_connect("subscribed",
        G_CALLBACK(ng_bot_core_subscribed_cb), NULL);
    ng_core_signal_connect("unsubscribe-request",
        G_CALLBACK(ng_bot_core_unsubscribe_request_cb), NULL);
    ng_core_signal_connect("unsubscribed",
        G_CALLBACK(ng_bot_core_unsubscribed_cb), NULL);
}

void ng_bot_exit()
{
    if(bot_member_table!=NULL)
        g_hash_table_destroy(bot_member_table);
}

gchar *ng_bot_get_nick_by_jid(const gchar *jid, const gchar *nick)
{
    gchar *rnick = NULL;
    gchar *tmp = NULL;
    if(nick==NULL || (nick!=NULL && strlen(nick)==0))
    {
        if(ng_db_member_get_nick_name(jid, &tmp))
        {
            if(tmp!=NULL)
            {
                rnick = g_strdup(tmp);
                g_free(tmp);
            }
        }
    }
    else
        rnick = g_strdup(nick);
    if(rnick==NULL)
    {
        rnick = ng_utils_generate_new_nick(jid);
    }
    return rnick;
}

void ng_bot_broadcast(const gchar *from, const gchar *message)
{
    NGDbMemberData db_member_data = {0};
    gchar *sender = NULL;
    GHashTableIter iter;
    NGBotMemberData *bot_member_data;
    gboolean stopped;
    gint64 stop_time;
    gint64 real_time;
    gchar *text;
    if(bot_member_table==NULL || message==NULL) return;
    real_time = ng_uitls_get_real_time();
    if(from!=NULL)
    {
        if(g_utf8_strlen(message, -1)>280)
        {
            ng_core_send_message(from, _("The message is too long!"));
            return;
        }
        if(!ng_db_member_jid_get_data(from, &db_member_data))
        {
            ng_core_send_message(from, _("You are not a member yet."));
            return;
        }
        if(db_member_data.banned)
        {
            if(db_member_data.ban_time >= real_time)
            {
                ng_core_send_message(from, _("You have been banned."));
                ng_db_member_data_free(&db_member_data);
                return;
            }
            else
                ng_db_member_set_banned(from, FALSE, 0);
        }
        ng_db_member_set_message_count(from, db_member_data.msg_count+1,
            db_member_data.msg_size+g_utf8_strlen(message, -1));
        sender = ng_bot_get_nick_by_jid(from, db_member_data.nick);
        ng_db_member_data_free(&db_member_data);
    }
    g_hash_table_iter_init(&iter, bot_member_table);
    //g_printf("Message: %s\n", message);
    text = g_strdup_printf("[%s] %s", sender, message);
    while(g_hash_table_iter_next(&iter, NULL, (gpointer *)&bot_member_data))
    {
        if(bot_member_data==NULL) continue;
        if(sender!=NULL)
        {
            if(g_strcmp0(bot_member_data->jid, from)==0) continue;
        }
        if(!bot_member_data->online) continue;
        if(ng_db_member_get_stopped(bot_member_data->jid, &stopped,
            &stop_time) && stopped)
        {
            if(stop_time<real_time)
            {
                ng_db_member_set_stopped(bot_member_data->jid, FALSE, 0);
            }
            else
                continue;
        }
        if(sender!=NULL)
            ng_core_send_message(bot_member_data->jid, text);
        else
            ng_core_send_message(bot_member_data->jid, message);
    }
    if(from!=NULL)
        ng_db_log_add_message(from, text);
    g_free(text);
    g_free(sender);
}

GHashTable *ng_bot_get_member_table()
{
    return bot_member_table;
}

const NGBotMemberData *ng_bot_get_member_data(const gchar *jid)
{
    NGBotMemberData *member_data = NULL;
    if(bot_member_table==NULL || jid==NULL) return NULL;
    member_data = g_hash_table_lookup(bot_member_table, jid);
    return member_data;
}

void ng_bot_member_data_remove(const gchar *jid)
{
    if(bot_member_table==NULL || jid==NULL) return;
    g_hash_table_remove(bot_member_table, jid);
}


