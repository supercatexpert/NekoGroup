/*
 * Command Module Declaration
 *
 * ng-cmd.c
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

/**
 * Modified by Mike Manilone <crtmike@gmx.us>
 */

#include <time.h>
#include "ng-cmd.h"
#include "ng-common.h"
#include "ng-core.h"
#include "ng-db.h"
#include "ng-bot.h"
#include "ng-config.h"
#include "ng-utils.h"
#include "ng-main.h"

static void ng_cmd_help(const gchar *jid)
{
    GString *result_string;
    NGDbUserPrivilegeLevel level = NG_DB_USER_PRIVILEGE_NORMAL;
    ng_db_member_get_privilege_level(jid, &level);
    result_string = g_string_new("");
    g_string_append(result_string, _("Normal Command Manual:\n"));
    g_string_append(result_string, _("-help		Show this help message.\n"));
    g_string_append(result_string, _("-nick		Show your nick, change your "
        "nick if used with an argument.\n"));
    g_string_append(result_string, _("-info		Show the profile of the "
        "given member.\n"));
    g_string_append(result_string, _("-users		Show the member list "
        "of this group, can be used with a key word.\n"));
    g_string_append(result_string, _("-online		Show the online members, "
        "can be used with a key word.\n"));
    g_string_append(result_string, _("-log			Show the message log, "
        "can be used with two arguments: time (with unit d,h,m,s) or "
        "number.\n"));
    g_string_append(result_string, _("-ping		Ping the group.\n"));
    g_string_append(result_string, _("-about		Show the about message "
        "of NekoGroup.\n"));
    g_string_append(result_string, _("-stop		Stop receiving messages from "
        "this group, should be used with time limit (unit: d,h,m,s), you can "
        "continue receiving messages by using 0 as the argument.\n"));
    g_string_append(result_string, _("-pm			Send private message to "
        "another member.\n"));
    g_string_append(result_string, _("-quit		Leave from this group.\n"));
    g_string_append(result_string, _("-me			/me from IRC.\n"));
    g_string_append(result_string, _("-say			Say anything!\n"));
    if(level>=NG_DB_USER_PRIVILEGE_POWER)
    {
        g_string_append(result_string, _("\n\nPower User Command Manual:\n"));
        g_string_append(result_string, _("-add		Add new members to this "
            "group.\n"));
        g_string_append(result_string, _("-ban		Ban a member.\n"));
        g_string_append(result_string, _("-release	Release a member from "
            "banned state.\n"));
        g_string_append(result_string, _("-kick		Kick a member from this "
            "group.\n"));
        g_string_append(result_string, _("-title		Set the title "
            "(status) of this group.\n"));
        g_string_append(result_string, _("-shutdown	Shutdown this group "
            "(only root user can do this).\n"));
    }
    ng_core_send_message(jid, result_string->str);
    g_string_free(result_string, TRUE);
}

static void ng_cmd_ping(const gchar *jid)
{
    time_t local_time;
    struct tm *time_ptr;
    const NGConfigServerData *conf_server_data;
    gchar *text;
    if(jid==NULL) return;
    local_time = time(NULL);
    conf_server_data = ng_config_get_server_data();
    if(conf_server_data!=NULL)
        local_time+=conf_server_data->timezone*60;
    time_ptr = localtime(&local_time);
    text = g_strdup_printf(_("Pong at %s"), g_strdelimit(asctime(time_ptr),
        "\n", '\0'));
    ng_core_send_message(jid, text);
    g_free(text);
}

static void ng_cmd_about(const gchar *jid)
{
    const gchar *text = _("About NekoGroup\n"
        "A group chatting robot based on GLib and libloudmouth.\n"
        "Version: 0.1.0, build date: 2012-05-29\n"
        "Copyright (C) 2012 - SuperCat, license: GPL v3.");
    ng_core_send_message(jid, text);
}

static gboolean ng_cmd_member_add(const gchar *jid, gint number, gchar **list)
{
    gint i;
    gchar *text;
    NGDbUserPrivilegeLevel level;
    if(jid==NULL || number<1 || list==NULL) return FALSE;
    if(list[0]==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    for(i=0;i<number && list[i]!=NULL;i++)
    {
        ng_core_send_subscribe_request(list[i]);
        text = g_strdup_printf(_("Trying to add new member: %s"), list[i]);
        ng_core_send_message(jid, text);
        g_free(text);
    }
    return TRUE;
}

static gboolean ng_cmd_member_renick(const gchar *jid, const gchar *nick)
{
    gchar *old_nick;
    gchar *text;
    gint64 time_limit;
    time_t timel;
    struct tm *time_ptr;
    NGDbMemberData db_member_data = {0};
    const NGConfigServerData *conf_server_data;
    const NGConfigNormalData *conf_normal_data;
    gunichar *ucs4_string;
    glong ucs4_string_len = 0;
    glong i;
    if(jid==NULL) return FALSE;
    if(nick!=NULL)
    {
        if(g_utf8_strlen(nick, -1)>32)
        {
            ng_core_send_message(jid, _("The nick should be less than 32 "
                "characters."));
            return TRUE;
        }
        ucs4_string = g_utf8_to_ucs4(nick, -1, NULL, &ucs4_string_len, NULL);
        if(ucs4_string!=NULL)
        {
            for(i=0;i<ucs4_string_len;i++)
            {
                switch(g_unichar_get_script(ucs4_string[i]))
                {
                    case G_UNICODE_SCRIPT_COMMON:
                    case G_UNICODE_SCRIPT_HAN:
                    case G_UNICODE_SCRIPT_HIRAGANA:
                    case G_UNICODE_SCRIPT_KATAKANA:
                    case G_UNICODE_SCRIPT_LATIN:
                        break;
                    default:
                        ng_core_send_message(jid, _("The nick contains "
                            "invalid characters!"));
                        g_free(ucs4_string);
                        return TRUE;
                        break;
                }
            }
            g_free(ucs4_string);
        }
        if(ng_db_member_nick_exist(nick))
        {
            ng_core_send_message(jid, _("This nick is already used."));
            return TRUE;
        }
        if(!ng_db_member_jid_get_data(jid, &db_member_data))
            return FALSE;
        conf_normal_data = ng_config_get_normal_data();
        time_limit = db_member_data.rename_time + 
            conf_normal_data->renick_timelimit;
        if(db_member_data.rename_time>0 && time_limit>ng_uitls_get_real_time())
        {
            conf_server_data = ng_config_get_server_data();
            timel = time_limit/1000;
            if(conf_server_data!=NULL)
                timel+=conf_server_data->timezone*60;
            time_ptr = localtime(&timel);
            text = g_strdup_printf(_("Your re-nick skill is cooling down, "
                "retry this skill after time: %s"), g_strdelimit(
                asctime(time_ptr), "\n", '\0'));
            ng_core_send_message(jid, text);
            g_free(text);
            ng_db_member_data_free(&db_member_data);
            return TRUE;
        }
        if(ng_db_member_set_nick(jid, nick))
        {
            text = g_strdup_printf(_("%s has changed the nick to: %s"),
                db_member_data.nick, nick);
            ng_bot_broadcast(NULL, text);
            g_free(text);
        }
        ng_db_member_data_free(&db_member_data);
    }
    else
    {
        if(ng_db_member_get_nick_name(jid, &old_nick))
        {
            text = g_strdup_printf(_("Your nick is: %s"), old_nick);
            ng_core_send_message(jid, text);
            g_free(text);
            g_free(old_nick);
        }
        else
            ng_core_send_message(jid, _("Your nick is not set."));
    }
    return TRUE;
}

static gboolean ng_cmd_member_info(const gchar *jid, const gchar *target)
{
    gchar *target_jid = NULL;
    const NGBotMemberData *bot_member_data;
    const NGConfigServerData *conf_server_data;
    NGDbMemberData db_member_data = {0};
    NGDbUserPrivilegeLevel level;
    time_t timel;
    struct tm *time_ptr;
    gint64 real_time;
    GString *result_string;
    gchar *tmp;
    if(!ng_db_member_nick_get_jid(target, &target_jid))
    {
        if(!ng_db_member_jid_exist(target))
        {
            ng_core_send_message(jid, _("Target member does not exist!"));
            return TRUE;
        }
        else
            target_jid = g_strdup(target);
    }
    if(!ng_db_member_jid_get_data(target_jid, &db_member_data))
    {
        ng_core_send_message(jid, _("Target member does not exist in "
            "the database!"));
        g_free(target_jid);
        return TRUE;
    }
    real_time = ng_uitls_get_real_time();
    conf_server_data = ng_config_get_server_data();
    bot_member_data = ng_bot_get_member_data(target_jid);
    result_string = g_string_new("");
    if(ng_db_member_get_privilege_level(jid, &level))
    {
        if(level>=NG_DB_USER_PRIVILEGE_POWER)
        {
            g_string_append_printf(result_string, _("JID: %s\n"),
                db_member_data.jid);
        }
    }
    g_string_append_printf(result_string, _("Nick: %s\n"), db_member_data.nick);
    timel = db_member_data.rename_time/1000;
    if(conf_server_data!=NULL)
        timel+=conf_server_data->timezone*60;
    time_ptr = localtime(&timel);
    g_string_append_printf(result_string, ngettext(
        _("Nick has been changed %lld time, the last modified time is %s\n"),
        _("Nick has been changed %lld times, the last modified time is %s\n"),
        (long long int)db_member_data.rename_freq),
        (long long int)db_member_data.rename_freq,
        g_strdelimit(asctime(time_ptr), "\n", '\0'));
    g_string_append_printf(result_string, ngettext(_("%lld message, "),
        _("%lld messages, "), (long long int)db_member_data.msg_count),
        (long long int)db_member_data.msg_count);
    g_string_append_printf(result_string, ngettext(_("%lld character\n"),
        _("%lld characters\n"), (long long int)db_member_data.msg_size),
        (long long int)db_member_data.msg_size);
    if(db_member_data.stopped)
    {
        if(db_member_data.stop_time<real_time)
        {
            ng_db_member_set_stopped(db_member_data.jid, FALSE, 0);
            g_string_append_printf(result_string, _("Not stopped\n"));
        }
        else
        {
            timel = db_member_data.stop_time/1000;
            if(conf_server_data!=NULL)
                timel+=conf_server_data->timezone*60;
            time_ptr = localtime(&timel);
            g_string_append_printf(result_string, _("Stopped, until %s\n"),
                g_strdelimit(asctime(time_ptr), "\n", '\0'));  
        }
    }
    else
        g_string_append_printf(result_string, _("Not stopped\n"));
    if(db_member_data.banned)
    {
        if(db_member_data.ban_time<real_time)
        {
            ng_db_member_set_banned(db_member_data.jid, FALSE, 0);
            g_string_append_printf(result_string, _("Not banned\n"));
        }
        else
        {
            timel = db_member_data.ban_time/1000;
            if(conf_server_data!=NULL)
                timel+=conf_server_data->timezone*60;
            time_ptr = localtime(&timel);
            g_string_append_printf(result_string, _("Banned, until %s\n"),
                g_strdelimit(asctime(time_ptr), "\n", '\0'));  
        }
    }
    else
        g_string_append_printf(result_string, _("Not banned\n"));
    timel = db_member_data.join_time/1000;
    if(conf_server_data!=NULL)
        timel+=conf_server_data->timezone*60;
    time_ptr = localtime(&timel);
    g_string_append_printf(result_string, _("Join time: %s\n"),
        g_strdelimit(asctime(time_ptr), "\n", '\0'));  
    g_string_append_printf(result_string, _("Allow private message: %s\n"),
        db_member_data.allow_pm? "Yes": "No");
    switch(db_member_data.privilege)
    {
        case NG_DB_USER_PRIVILEGE_ROOT:
            tmp = "Root";
            break;
        case NG_DB_USER_PRIVILEGE_POWER:
            tmp = "Power User";
            break;
        default:
            tmp = "Normal Member";
            break;
    }
    g_string_append_printf(result_string, _("Privilege: %s"),
        tmp);
    if(bot_member_data!=NULL)
    {
        if(bot_member_data->resource!=NULL)
        {
            g_printf("Resource: %s\n", bot_member_data->resource);
            g_string_append_printf(result_string,
                _("\nOnline resource: [%s]"), bot_member_data->resource);
        }
    }
    ng_core_send_message(jid, result_string->str);
    g_string_free(result_string, TRUE);
    ng_db_member_data_free(&db_member_data);
    g_free(target_jid);
    return TRUE;
}

static gint ng_cmd_member_list_sort(gconstpointer a, gconstpointer b)
{
    const NGDbMemberData *data1, *data2;
    data1 = *(const NGDbMemberData **)a;
    data2 = *(const NGDbMemberData **)b;
    return (gint)(data1->msg_count - data2->msg_count);
}

static gboolean ng_cmd_member_list(const gchar *jid, const gchar *key)
{
    GHashTable *table;
    guint count = 0;
    gint i;
    GString *result_string;
    GHashTableIter iter;
    GPtrArray *db_data_array;
    NGDbMemberData *db_member_data;
    if(jid==NULL) return FALSE;
    result_string = g_string_new("");
    if(key!=NULL)
        table = ng_db_member_search(key);
    else
        table = ng_db_member_get_table();
    if(key!=NULL)
        g_string_append_printf(result_string, _("The users who's "
            "nicks contain \'%s\':\n"), key);
    else
        g_string_append_printf(result_string, _("All users:\n"));
    if(table!=NULL)
    {
        g_hash_table_iter_init(&iter, table);
        db_data_array = g_ptr_array_new();
        while(g_hash_table_iter_next(&iter, NULL,
            (gpointer *)&db_member_data))
        {
            if(db_member_data==NULL) continue;
            g_ptr_array_add(db_data_array, db_member_data);
            count++;
        }
        g_ptr_array_sort(db_data_array, ng_cmd_member_list_sort);
        for(i=0;i<db_data_array->len;i++)
        {
            db_member_data = g_ptr_array_index(db_data_array, i);
            g_string_append_printf(result_string, "* %s (N=%lld, C=%lld)\n",
                db_member_data->nick,
                (long long int)db_member_data->msg_count,
                (long long int)db_member_data->msg_size);
        }
        g_ptr_array_free(db_data_array, TRUE);
        g_hash_table_destroy(table);
    }
    g_string_append_printf(result_string,
        ngettext(_("%u user listed"), _("%u users listed"), count), count);
    ng_core_send_message(jid, result_string->str);
    g_string_free(result_string, TRUE);
    return TRUE;
}

static gboolean ng_cmd_member_online(const gchar *jid, const gchar *key)
{
    GHashTable *table;
    GHashTableIter iter;
    guint count = 0;
    gint64 real_time;
    NGBotMemberData *bot_member_data;
    GString *result_string;
    NGDbMemberData db_member_data = {0};
    if(jid==NULL) return FALSE;
    table = ng_bot_get_member_table();
    if(table==NULL) return TRUE;
    g_hash_table_iter_init(&iter, table);
    result_string = g_string_new("");
    real_time = ng_uitls_get_real_time();
    if(key!=NULL)
        g_string_append_printf(result_string, _("The online users who's "
            "nicks contain \'%s\':\n"), key);
    else
        g_string_append_printf(result_string, _("All online users:\n"));
    while(g_hash_table_iter_next(&iter, NULL, (gpointer *)&bot_member_data))
    {
        if(bot_member_data==NULL) continue;
        if(!bot_member_data->online) continue;
        memset(&db_member_data, 0, sizeof(NGDbMemberData));
        if(!ng_db_member_jid_get_data(bot_member_data->jid, &db_member_data))
            continue;
        if(key!=NULL)
        {
            if(g_strstr_len(db_member_data.nick, -1, key)==NULL)
            {
                ng_db_member_data_free(&db_member_data);
                continue;
            }
        }
        g_string_append_printf(result_string, "* %s ", db_member_data.nick);
        if(db_member_data.stopped)
        {
            if(db_member_data.stop_time<real_time)
            {
                ng_db_member_set_stopped(db_member_data.jid, FALSE, 0);
            }
            else
            {
                g_string_append(result_string, _("<Stopped> "));
            }
        }
        if(bot_member_data->status!=NULL)
        {
            g_string_append_printf(result_string, "(%s) ",
                bot_member_data->status);
        }
        g_string_append_c(result_string, '\n');
        count++;
        ng_db_member_data_free(&db_member_data);
    }
    g_string_append_printf(result_string,
        ngettext(_("%u user listed"), _("%u users listed"), count), count);
    ng_core_send_message(jid, result_string->str);
    g_string_free(result_string, TRUE);
    return TRUE;
}

static gboolean ng_cmd_member_pm(const gchar *jid, const gchar *target,
    gint argc, gchar **argv)
{
    gchar *target_jid = NULL;
    GString *result_string;
    gboolean allow_pm = FALSE;
    gchar *nick = NULL;
    gint i;
    if(!ng_db_member_nick_get_jid(target, &target_jid))
    {
        if(!ng_db_member_jid_exist(target))
        {
            ng_core_send_message(jid, _("Target member does not exist!"));
            return TRUE;
        }
        else
            target_jid = g_strdup(target);
    }
    if(!ng_db_member_get_allow_pm(target_jid, &allow_pm) || !allow_pm)
    {
        ng_core_send_message(jid, _("Target member does not want to "
            "receive private message!"));
        g_free(target_jid);
        return TRUE;
    }
    if(!ng_db_member_get_nick_name(jid, &nick))
        nick = ng_utils_generate_new_nick(jid);
    result_string = g_string_new("");
    g_string_append_printf(result_string, _("(P.M.) [%s] "), nick);
    g_free(nick);
    for(i=0;i<argc && argv[i]!=NULL;i++)
    {
        g_string_append_printf(result_string, "%s ", argv[i]);
    }
    ng_core_send_message(target_jid, result_string->str);
    g_free(target_jid);
    g_string_free(result_string, TRUE);
    return TRUE;
}

static gboolean ng_cmd_member_view_log(const gchar *jid, const gchar *cond1,
    const gchar *cond2)
{
    GList *list;
    GList *foreach;
    time_t timel;
    struct tm *time_ptr;
    gint64 search_time = 3600000;
    guint search_num = 50;
    gint n;
    gint num;
    gchar unit;
    GString *result_string;
    NGDbLogData *log_data;
    gchar time_buf[32];
    const NGConfigServerData *conf_server_data;
    if(jid==NULL) return FALSE;
    if(cond1!=NULL || cond2!=NULL)
    {
        search_time = 0;
        search_num = 0;
    }
    if(cond1!=NULL)
    {
        n = sscanf(cond1, "%d%c", &num, &unit);
        if(n==1)
            search_num = num;
        else if(n==2)
        {
            switch(unit)
            {
                case 'd':
                    search_time = 86400 * (gint64)num * 1000;
                    break;
                case 'h':
                    search_time = 3600 * (gint64)num * 1000;
                    break;
                case 'm':
                    search_time = 60 * (gint64)num * 1000;
                    break;
                case 's':
                    search_time = (gint64)num * 1000;
                    break;
                default:
                    ng_core_send_message(jid, _("Wrong time format."));
                    return TRUE;
                    break;
            }
        }
    }
    if(cond2!=NULL)
    {
        n = sscanf(cond2, "%d%c", &num, &unit);
        if(n==1)
            search_num = num;
        else if(n==2)
        {
            switch(unit)
            {
                case 'd':
                    search_time = 86400 * (gint64)num * 1000;
                    break;
                case 'h':
                    search_time = 3600 * (gint64)num * 1000;
                    break;
                case 'm':
                    search_time = 60 * (gint64)num * 1000;
                    break;
                case 's':
                    search_time = (gint64)num * 1000;
                    break;
                default:
                    ng_core_send_message(jid, _("Wrong time format."));
                    return TRUE;
                    break;
            }
        }
    }    
    list = ng_db_log_get_data(search_time, search_num);
    if(list==NULL)
    {
        ng_core_send_message(jid, _("No message log matched the "
            "search condition."));
        return TRUE;
    }
    result_string = g_string_new("");
    conf_server_data = ng_config_get_server_data();
    for(foreach=list;foreach!=NULL;foreach=g_list_next(foreach))
    {
        log_data = foreach->data;
        if(log_data==NULL) continue;
        timel = log_data->time/1000;
        if(conf_server_data!=NULL)
            timel+=conf_server_data->timezone*60;
        time_ptr = localtime(&timel);
        memset(time_buf, 0, 32);
        strftime(time_buf, 32, "%H:%M:%S", time_ptr);
        g_string_append_printf(result_string, "\n%s %s",
            time_buf, log_data->message);
        ng_db_log_data_destroy(log_data);
        foreach->data = NULL;
    }
    g_list_free(list);
    ng_core_send_message(jid, result_string->str);
    g_string_free(result_string, TRUE);
    return TRUE;
}

gboolean ng_cmd_member_quit(const gchar *jid)
{
    gchar *text;
    gchar *nick = NULL;
    if(jid==NULL) return FALSE;
    if(!ng_db_member_get_nick_name(jid, &nick))
        nick = ng_utils_generate_new_nick(jid);
    text = g_strdup_printf("%s has left from this group, good bye!", nick);
    g_free(nick);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    ng_bot_member_data_remove(jid);
    ng_db_member_jid_remove(jid);
    ng_core_remove_roster(jid);
    ng_core_send_unsubscribe_request(jid);
    return TRUE;
}

static gboolean ng_cmd_member_stop(const gchar *jid, const gchar *length)
{
    gchar *text;
    gint64 time_length;
    gint num;
    gchar unit;
    const NGConfigServerData *conf_server_data;
    time_t timel;
    struct tm *time_ptr;
    gint n;
    if(jid==NULL || length==NULL) return FALSE;
    n = sscanf(length, "%d%c", &num, &unit);
    if(n==1 && num==0)
    {
        ng_db_member_set_stopped(jid, FALSE, 0);
        ng_core_send_message(jid,
            _("You have been released from stop state."));
        return TRUE;
    }
    else if(n!=2)
    {
        ng_core_send_message(jid, _("Wrong time format."));
        return TRUE;
    }
    switch(unit)
    {
        case 'd':
            time_length = 86400 * (gint64)num * 1000;
            break;
        case 'h':
            time_length = 3600 * (gint64)num * 1000;
            break;
        case 'm':
            time_length = 60 * (gint64)num * 1000;
            break;
        case 's':
            time_length = (gint64)num * 1000;
            break;
        default:
            ng_core_send_message(jid, _("Wrong time format."));
            return TRUE;
            break;
    }
    if(time_length==0)
    {
        ng_db_member_set_stopped(jid, FALSE, 0);
        ng_core_send_message(jid,
            _("You have been released from stop state."));
        return TRUE;
    }
    if(time_length>(gint64)86400*365*2*1000)
    {
        ng_core_send_message(jid,
            _("Why do you try to stop for more than 2 years?"));
        return TRUE;
    }
    ng_db_member_set_stopped(jid, TRUE, time_length);
    conf_server_data = ng_config_get_server_data();
    timel = time(NULL) + time_length/1000;
    if(conf_server_data!=NULL)
        timel+=conf_server_data->timezone*60;
    time_ptr = localtime(&timel);
    text = g_strdup_printf(_("You have been stopped until %s"),
        g_strdelimit(asctime(time_ptr), "\n", '\0'));
    ng_core_send_message(jid, text);
    g_free(text);
    return TRUE;
}

static gboolean ng_cmd_member_ban(const gchar *jid, const gchar *victim,
    const gchar *length)
{
    gchar *text;
    NGDbUserPrivilegeLevel level, victim_level;
    gchar *target_jid = NULL;
    gint64 time_length;
    gint num;
    gchar unit;
    gint n;
    const NGConfigServerData *conf_server_data;
    time_t timel;
    struct tm *time_ptr;
    if(jid==NULL || victim==NULL || length==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    if(!ng_db_member_nick_get_jid(victim, &target_jid))
    {
        if(!ng_db_member_jid_exist(victim))
        {
            ng_core_send_message(jid, _("Target member does not exist!"));
            return TRUE;
        }
        else
            target_jid = g_strdup(victim);
    }
    if(!ng_db_member_get_privilege_level(target_jid, &victim_level))
    {
        g_free(target_jid);
        return FALSE;
    }
    if(victim_level==NG_DB_USER_PRIVILEGE_ROOT)
    {
        ng_core_send_message(jid, _("Root users cannot be banned!"));
        g_free(target_jid);
        return TRUE;
    }
    if(level==NG_DB_USER_PRIVILEGE_POWER &&
        victim_level>=NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("You cannot ban power users or "
            "root users!"));
        g_free(target_jid);
        return TRUE;
    }
    n = sscanf(length, "%d%c", &num, &unit);
    if(n==1 && num==0)
    {
        ng_db_member_set_banned(target_jid, FALSE, 0);
        text = g_strdup_printf(_("%s has been released from banned state."),
            target_jid);
        ng_bot_broadcast(NULL, text);
        g_free(text);
        g_free(target_jid);
        return TRUE;
    }
    else if(n!=2)
    {
        ng_core_send_message(jid, _("Wrong time format."));
        g_free(target_jid);
        return TRUE;
    }
    switch(unit)
    {
        case 'd':
            time_length = 86400 * (gint64)num * 1000;
            break;
        case 'h':
            time_length = 3600 * (gint64)num * 1000;
            break;
        case 'm':
            time_length = 60 * (gint64)num * 1000;
            break;
        case 's':
            time_length = (gint64)num * 1000;
            break;
        default:
            ng_core_send_message(jid, _("Wrong time format."));
            g_free(target_jid);
            return TRUE;
            break;
    }
    if(time_length==0)
    {
        ng_db_member_set_banned(target_jid, FALSE, 0);
        text = g_strdup_printf(_("%s has been released from banned state."),
            victim);
        ng_bot_broadcast(NULL, text);
        g_free(text);
        g_free(target_jid);
        return TRUE;
    }
    if(time_length>(gint64)86400*365*2*1000)
    {
        ng_core_send_message(jid,
            _("Why do you try to ban the victim for more than 2 years?"));
        g_free(target_jid);
        return TRUE;
    }
    ng_db_member_set_banned(target_jid, TRUE, time_length);
    conf_server_data = ng_config_get_server_data();
    timel = time(NULL) + time_length/1000;
    if(conf_server_data!=NULL)
        timel+=conf_server_data->timezone*60;
    time_ptr = localtime(&timel);
    text = g_strdup_printf(_("%s has been banned until %s"), victim,
        g_strdelimit(asctime(time_ptr), "\n", '\0'));
    ng_bot_broadcast(NULL, text);
    g_free(text);
    g_free(target_jid);
    return TRUE;
}

static gboolean ng_cmd_member_release(const gchar *jid, const gchar *target)
{
    gchar *text;
    NGDbUserPrivilegeLevel level;
    gchar *target_jid = NULL;
    if(jid==NULL || target==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    if(!ng_db_member_nick_get_jid(target, &target_jid))
    {
        if(!ng_db_member_jid_exist(target))
        {
            ng_core_send_message(jid, _("Target member does not exist!"));
            return TRUE;
        }
        else
            target_jid = g_strdup(target);
    }
    ng_db_member_set_banned(target_jid, FALSE, 0);
    text = g_strdup_printf(_("%s has been released from banned state."),
        target);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    g_free(target_jid);
    return TRUE;
}

static gboolean ng_cmd_set_group_title(const gchar *jid, const gchar *title)
{
    NGDbUserPrivilegeLevel level;
    if(jid==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    if(title==NULL) title = "";
    ng_core_set_title(title);
    return TRUE;
}

static gboolean ng_cmd_member_kick(const gchar *jid, const gchar *victim)
{
    gchar *text;
    NGDbUserPrivilegeLevel level, victim_level;
    gchar *target_jid = NULL;
    if(jid==NULL || victim==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    if(!ng_db_member_nick_get_jid(victim, &target_jid))
    {
        if(!ng_db_member_jid_exist(victim))
        {
            ng_core_send_message(jid, _("Target member does not exist!"));
            return TRUE;
        }
        else
            target_jid = g_strdup(victim);
    }
    if(!ng_db_member_get_privilege_level(target_jid, &victim_level))
    {
        g_free(target_jid);
        return FALSE;
    }
    if(victim_level==NG_DB_USER_PRIVILEGE_ROOT)
    {
        ng_core_send_message(jid, _("Root users cannot be kicked!"));
        g_free(target_jid);
        return TRUE;
    }
    if(level==NG_DB_USER_PRIVILEGE_POWER &&
        victim_level>=NG_DB_USER_PRIVILEGE_POWER)
    {
        ng_core_send_message(jid, _("You cannot kick power users or "
            "root users!"));
        g_free(target_jid);
        return TRUE;
    }
    text = g_strdup_printf("%s was kicked from the group!", victim);
    ng_bot_broadcast(NULL, text);
    g_free(text);
    ng_bot_member_data_remove(target_jid);
    ng_db_member_jid_remove(target_jid);
    ng_core_remove_roster(target_jid);
    ng_core_send_unsubscribe_request(target_jid);
    g_free(target_jid);
    return TRUE;
}

static gboolean ng_cmd_shutdown(const gchar *jid)
{
    NGDbUserPrivilegeLevel level;
    if(jid==NULL) return FALSE;
    if(!ng_db_member_get_privilege_level(jid, &level)) return FALSE;
    if(level<NG_DB_USER_PRIVILEGE_ROOT)
    {
        ng_core_send_message(jid, _("Permission denied."));
        return TRUE;
    }
    ng_bot_broadcast(NULL, _("This group is going to shut down!"));
    ng_core_send_unavailable_message();
    ng_main_quit_loop();
    return TRUE;
}

static gboolean ng_cmd_me(const gchar *jid, const gchar*message)
{
    gchar *tosend, *nick;
    ng_db_member_get_nick_name(jid, &nick);
    tosend = g_strdup_printf(" * %s %s", nick, message);
    ng_bot_broadcast(NULL, tosend);
    g_free(nick);
    g_free(tosend);
    return TRUE;
}

gboolean ng_cmd_exec(const gchar *jid, const gchar *command)
{
    gint argc = 0;
    gchar **argv = NULL;
    gchar *nick;
    gboolean flag = FALSE;
    if(!g_shell_parse_argv(command, &argc, &argv, NULL))
        return FALSE;
    g_printf("%s used command: \'%s\'\n", jid, command);
    if(argc<1)
    {
        g_strfreev(argv);
        return FALSE;
    }
    if(g_strcmp0(argv[0], "-nick")==0)
    {
        if(argc>1)
        {
            nick = g_strdup(argv[1]);
            flag = ng_cmd_member_renick(jid, g_strstrip(nick));
            g_free(nick);
        }
        else
            flag = ng_cmd_member_renick(jid, NULL);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-info")==0)
    {
        if(argc!=1 && argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        if(argc==1)
            flag = ng_cmd_member_info(jid, jid);
        else
            flag = ng_cmd_member_info(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-users")==0)
    {
        if(argc!=1 && argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        if(argc==1)
            flag = ng_cmd_member_list(jid, NULL);
        else
            flag = ng_cmd_member_list(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-online")==0)
    {
        if(argc!=1 && argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        if(argc==1)
            flag = ng_cmd_member_online(jid, NULL);
        else
            flag = ng_cmd_member_online(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-log")==0)
    {
        if(argc==1)
        {
            flag = ng_cmd_member_view_log(jid, NULL, NULL);
            g_strfreev(argv);
            return flag;
        }
        else if(argc==2)
        {
            flag = ng_cmd_member_view_log(jid, argv[1], NULL);
            g_strfreev(argv);
            return flag;
        }
        else if(argc==3)
        {
            flag = ng_cmd_member_view_log(jid, argv[1], argv[2]);
            g_strfreev(argv);
            return flag;
        }
        else
        {
            g_strfreev(argv);
            return FALSE;
        }
    }
    else if(g_strcmp0(argv[0], "-help")==0)
    {
        ng_cmd_help(jid);
        g_strfreev(argv);
        return TRUE;
    }
    else if(g_strcmp0(argv[0], "-ping")==0)
    {
        ng_cmd_ping(jid);
        g_strfreev(argv);
        return TRUE;
    }
    else if(g_strcmp0(argv[0], "-about")==0)
    {
        ng_cmd_about(jid);
        g_strfreev(argv);
        return TRUE;
    }
    else if(g_strcmp0(argv[0], "-stop")==0)
    {
        if(argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        flag = ng_cmd_member_stop(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-pm")==0)
    {
        if(argc<3)
        {
            g_strfreev(argv);
            return FALSE;
        }
        flag = ng_cmd_member_pm(jid, argv[1], argc-2, argv+2);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-quit")==0)
    {
        flag = ng_cmd_member_quit(jid);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-add")==0)
    {
        flag = ng_cmd_member_add(jid, argc-1, argv+1);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-ban")==0)
    {
        if(argc!=3)
        {
            g_strfreev(argv);
            return FALSE;
        }
        flag = ng_cmd_member_ban(jid, argv[1], argv[2]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-release")==0)
    {
        if(argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        flag = ng_cmd_member_release(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-kick")==0)
    {
        if(argc!=2)
        {
            g_strfreev(argv);
            return FALSE;
        }
        flag = ng_cmd_member_kick(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-title")==0)
    {
        if(argc==1)
            flag = ng_cmd_set_group_title(jid, NULL);
        else if(argc==2)        
            flag = ng_cmd_set_group_title(jid, argv[1]);
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-me")==0)
    {
        if(argc>=2)
        {
            guint i;
            GString *message;
            message = g_string_new("");
            for(i = 1; i<argc; i++)
            {
                g_string_append(message, argv[i]);
                g_string_append_c(message, ' ');
            }
            flag = ng_cmd_me(jid, message->str);
            g_string_free(message, TRUE);
        }
        else
        {
            flag = ng_cmd_me(jid, _("did nothing."));
        }
        g_strfreev(argv);
        return flag;
    }
    else if(g_strcmp0(argv[0], "-say")==0)
    {
        GString *message;
        guint i;
        message = g_string_new("");
        for(i = 1; i < argc; i++)
        {
            g_string_append(message, argv[i]);
            g_string_append_c(message, ' ');
        }
        ng_bot_broadcast(jid, message->str);
        g_string_free(message, TRUE);
        g_strfreev(argv);
        return TRUE;
    }
    else if(g_strcmp0(argv[0], "-shutdown")==0)
    {
        flag = ng_cmd_shutdown(jid);
        g_strfreev(argv);
        return flag;
    }
    g_strfreev(argv);
    return FALSE;
}






