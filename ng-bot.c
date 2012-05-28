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
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#include "ng-bot.h"
#include "ng-common.h"
#include "ng-core.h"
#include "ng-db.h"
#include "ng-cmd.h"

#define NG_BOT_MEMBER_NAME_SALT "NekoSalt"

static GHashTable *bot_member_table = NULL;

static const gchar *ng_bot_markup_unescape_entity(const gchar *text,
    gint *length)
{
    const gchar *pln;
    gint len, pound;
    gchar temp[2];

    if (!text || *text != '&')
        return NULL;

    #define IS_ENTITY(s)  (!g_ascii_strncasecmp(text, s, (len = sizeof(s) - 1)))

    if(IS_ENTITY("&amp;"))
        pln = "&";
    else if(IS_ENTITY("&lt;"))
        pln = "<";
    else if(IS_ENTITY("&gt;"))
        pln = ">";
    else if(IS_ENTITY("&nbsp;"))
        pln = " ";
    else if(IS_ENTITY("&copy;"))
        pln = "\302\251";
    else if(IS_ENTITY("&quot;"))
        pln = "\"";
    else if(IS_ENTITY("&reg;"))
        pln = "\302\256";
    else if(IS_ENTITY("&apos;"))
        pln = "\'";
    else if(*(text+1) == '#' &&
        (sscanf(text, "&#%u%1[;]", &pound, temp) == 2 ||
        sscanf(text, "&#x%x%1[;]", &pound, temp) == 2) &&
        pound != 0)
    {
        static gchar buf[7];
        gint buflen = g_unichar_to_utf8((gunichar)pound, buf);
        buf[buflen] = '\0';
        pln = buf;

        len = (*(text+2) == 'x' ? 3 : 2);
        while(isxdigit((gint) text[len])) len++;
        if(text[len] == ';') len++;
    }
    else
        return NULL;

    if (length)
        *length = len;
    return pln;
}

static gchar *ng_bot_unescape_html(const gchar *html)
{
    GString *ret;
    const gchar *c = html;

    if (html == NULL)
        return NULL;

    ret = g_string_new("");
    while (*c)
    {
        gint len;
        const gchar *ent;

        if((ent = ng_bot_markup_unescape_entity(c, &len))!=NULL)
        {
            g_string_append(ret, ent);
            c += len;
        }
        else if (!strncmp(c, "<br>", 4))
        {
            g_string_append_c(ret, '\n');
            c += 4;
        }
        else
        {
            g_string_append_c(ret, *c);
            c++;
        }
    }
    return g_string_free(ret, FALSE);
}

static gchar *ng_bot_markup_strip_html(const gchar *str)
{
    gint i, j, k, entlen;
    gboolean visible = TRUE;
    gboolean closing_td_p = FALSE;
    gchar *str2;
    const gchar *cdata_close_tag = NULL, *ent;
    gchar *href = NULL;
    int href_st = 0;

    if(!str)
        return NULL;

    str2 = g_strdup(str);

    for(i = 0, j = 0; str2[i]; i++)
    {
        if(str2[i] == '<')
        {
            if(cdata_close_tag)
            {
                if(g_ascii_strncasecmp(str2 + i, cdata_close_tag,
                        strlen(cdata_close_tag)) == 0)
                {
                    i += strlen(cdata_close_tag) - 1;
                    cdata_close_tag = NULL;
                }
                continue;
            }
            else if(g_ascii_strncasecmp(str2 + i, "<td", 3) == 0 &&
                closing_td_p)
            {
                str2[j++] = '\t';
                visible = TRUE;
            }
            else if(g_ascii_strncasecmp(str2 + i, "</td>", 5) == 0)
            {
                closing_td_p = TRUE;
                visible = FALSE;
            }
            else
            {
                closing_td_p = FALSE;
                visible = TRUE;
            }

            k = i + 1;

            if(g_ascii_isspace(str2[k]))
                visible = TRUE;
            else if(str2[k])
            {
                while(str2[k] && str2[k] != '<' && str2[k] != '>')
                {
                    k++;
                }

                if(g_ascii_strncasecmp(str2 + i, "<a", 2) == 0 &&
                    g_ascii_isspace(str2[i+2]))
                {
                    gint st;
                    gint end;
                    gchar delim = ' ';

                    for(st = i + 3; st < k; st++)
                    {
                        if (g_ascii_strncasecmp(str2+st, "href=", 5) == 0)
                        {
                            st += 5;
                            if (str2[st] == '"' || str2[st] == '\'')
                            {
                                delim = str2[st];
                                st++;
                            }
                            break;
                        }
                    }

                    for(end = st; end < k && str2[end] != delim; end++)
                    {
                       
                    }

                    if(st < k)
                    {
                        char *tmp;
                        g_free(href);
                        tmp = g_strndup(str2 + st, end - st);
                        href = ng_bot_unescape_html(tmp);
                        g_free(tmp);
                        href_st = j;
                    }
                }

                else if(href != NULL && g_ascii_strncasecmp(str2 + i,
                    "</a>", 4) == 0)
                {

                    size_t hrlen = strlen(href);

                    if((hrlen != j - href_st ||
                         strncmp(str2 + href_st, href, hrlen)) &&
                        (hrlen != j - href_st + 7 ||
                         strncmp(str2 + href_st, href + 7, hrlen - 7)))
                    {
                        str2[j++] = ' ';
                        str2[j++] = '(';
                        g_memmove(str2 + j, href, hrlen);
                        j += hrlen;
                        str2[j++] = ')';
                        g_free(href);
                        href = NULL;
                    }
                }

                else if((j && (g_ascii_strncasecmp(str2 + i, "<p>", 3) == 0
                              || g_ascii_strncasecmp(str2 + i, "<tr", 3) == 0
                              || g_ascii_strncasecmp(str2 + i, "<hr", 3) == 0
                              || g_ascii_strncasecmp(str2 + i, "<li", 3) == 0
                              || g_ascii_strncasecmp(str2 + i, "<div", 4) == 0))
                 || g_ascii_strncasecmp(str2 + i, "<br", 3) == 0
                 || g_ascii_strncasecmp(str2 + i, "</table>", 8) == 0)
                {
                    str2[j++] = '\n';
                }

                else if(g_ascii_strncasecmp(str2 + i, "<script", 7) == 0)
                {
                    cdata_close_tag = "</script>";
                }
                else if(g_ascii_strncasecmp(str2 + i, "<style", 6) == 0)
                {
                    cdata_close_tag = "</style>";
                }

                i = (str2[k] == '<' || str2[k] == '\0')? k - 1: k;
                continue;
            }
        }
        else if(cdata_close_tag)
        {
            continue;
        }
        else if(!g_ascii_isspace(str2[i]))
        {
            visible = TRUE;
        }
        if(str2[i] == '&' && (ent = ng_bot_markup_unescape_entity(str2 + i,
            &entlen)) != NULL)
        {
            while (*ent)
                str2[j++] = *ent++;
            i += entlen - 1;
            continue;
        }
        if(visible)
            str2[j++] = g_ascii_isspace(str2[i])? ' ': str2[i];
    }

    g_free(href);
    str2[j] = '\0';
    return str2;
}

static void ng_bot_core_message_cb(NGCore *core, const gchar *from,
    const gchar *type, const gchar *body, gpointer data)
{
    gchar *text;
    if(g_strcmp0(type, "chat")!=0)
    {
        return;
    }
    text = ng_bot_markup_strip_html(body);
    if(text==NULL) text = g_strdup(body);
    if(text==NULL) return;
    if(g_str_has_prefix(text, "-"))
    {
        /* Commands! */
        if(!ng_cmd_exec(from, text))
        {
            ng_core_send_message(from, _("Invalid command!"));
        }
        g_free(text);
        return;
    }
    ng_bot_broadcast(from, text);
    g_free(text);
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
    nick = ng_bot_generate_new_nick(jid);
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
    nick = ng_bot_generate_new_nick(jid);
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
        nick = ng_bot_generate_new_nick(jid);
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

gchar *ng_bot_generate_new_nick(const gchar *jid)
{
    GChecksum *checksum;
    gchar *short_name;
    gchar *rnick;
    checksum = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(checksum, (const guchar *)jid, strlen(jid));
    g_checksum_update(checksum, (const guchar *)NG_BOT_MEMBER_NAME_SALT,
        strlen(NG_BOT_MEMBER_NAME_SALT));
    short_name = ng_core_get_shortname(jid);
    rnick = g_strdup_printf("%s@%8.8s", short_name,
        g_checksum_get_string(checksum));
    g_free(short_name);
    g_checksum_free(checksum);
    return rnick;
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
        rnick = ng_bot_generate_new_nick(jid);
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
    real_time = g_get_real_time()/1000;
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
        {
            text = g_strdup_printf("[%s] %s", sender, message);
            ng_core_send_message(bot_member_data->jid, text);
            g_free(text);
        }
        else
            ng_core_send_message(bot_member_data->jid, message);
    }
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


