/*
 * Command Module Declaration
 *
 * command.c
 * This file is part of <NekoGroup>
 *
 * Copyright (C) 2011 - SuperCat, license: GPL v3
 *
 * <NekoGroup> is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * <NekoGroup> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "command.h"
#include "core.h"
#include "log.h"

static void ng_command_list_print_help(PurpleConvIm *conv_im)
{
    const gchar *text = "Command list:<br/>"
        "-help\t\t\t\t- print this help<br/>"
        "-online\t\t\t\t- show the users online now<br/>"
        "-list\t\t\t\t- list all users<br/>"
        "-nick\t\t\t\t- see your nick name<br/>"
        "-nick newnick\t\t\t- set your name to newnick<br/>"
        "-talk user message\t- talk with another user privately<br/>"
        "-history [line=40]\t\t- see the history chatting log<br/>"
        "-ping\t\t\t\t- check whether this group is still alive<br/>"
        "-about\t\t\t\t- about this group chatting program<br/>";
    purple_conv_im_send(conv_im, text);
}

static void ng_command_list_user(PurpleAccount *account,
    PurpleConvIm *conv_im)
{
    GSList *list, *foreach;
    PurpleBuddy *buddy;
    gchar *chat_name;
    guint count = 0;
    GString *result_str;
    const gchar *group_name;
    result_str = g_string_new("User list:<br/>");
    list = purple_find_buddies(account, NULL);
    for(foreach=list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        buddy = foreach->data;
        if(buddy==NULL) continue;
        chat_name = ng_core_get_buddy_chat_name(buddy, NULL);
        group_name = purple_group_get_name(purple_buddy_get_group(buddy));
        if(PURPLE_BUDDY_IS_ONLINE(buddy))
            g_string_append_printf(result_str, "[*");
        else
            g_string_append_printf(result_str, "[-");
        if(g_strcmp0(group_name, "admin")==0)
            g_string_append_printf(result_str, "#]");
        else
            g_string_append_printf(result_str, "-]");
        g_string_append_printf(result_str, " %s<br/>", chat_name);
        g_free(chat_name);
        count++;
    }
    g_slist_free(list);
    g_string_append_printf(result_str, "Total %u user(s)", count);
    purple_conv_im_send(conv_im, result_str->str);
    g_string_free(result_str, TRUE);
}

static void ng_command_list_online(PurpleAccount *account,
    PurpleConvIm *conv_im)
{
    GSList *list, *foreach;
    PurpleBuddy *buddy;
    gchar *chat_name;
    guint count = 0;
    GString *result_str;
    const gchar *group_name;
    result_str = g_string_new("Online user list:<br/>");
    list = purple_find_buddies(account, NULL);
    for(foreach=list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        buddy = foreach->data;
        if(buddy==NULL) continue;
        if(!PURPLE_BUDDY_IS_ONLINE(buddy)) continue;
        chat_name = ng_core_get_buddy_chat_name(buddy, NULL);
        group_name = purple_group_get_name(purple_buddy_get_group(buddy));
        if(g_strcmp0(group_name, "admin")==0)
            g_string_append_printf(result_str, "<b>%s</b><br/>", chat_name);
        else
            g_string_append_printf(result_str, "%s<br/>", chat_name);
        g_free(chat_name);
        count++;
    }
    g_slist_free(list);
    g_string_append_printf(result_str, "Total %u user(s)", count);
    purple_conv_im_send(conv_im, result_str->str);
    g_string_free(result_str, TRUE);
}

static void ng_command_config_nick(PurpleAccount *account,
    PurpleConvIm *conv_im, const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    PurpleBuddy *buddy;
    PurpleBuddy *buddy_check;
    const gchar *nick_name;
    gchar *result;
    gchar **cmd_list;
    gchar *old_name;
    const gchar *new_name;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    if(name==NULL) return;
    buddy = purple_find_buddy(account, name);
    if(buddy==NULL) return;
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL)
    {
        if(cmd_list[1]==NULL || *cmd_list[1]=='\0')
        {
            nick_name = purple_buddy_get_alias_only(buddy);
            if(nick_name!=NULL)
                result = g_strdup_printf("Your nick name is: %s", nick_name);
            else
                result = g_strdup_printf("Your nick name is not set yet!");
            purple_conv_im_send(conv_im, result);
            g_free(result);
        }
        else
        {
            new_name = g_strstrip(cmd_list[1]);
            buddy_check = ng_core_get_buddy_by_nick(account, new_name);
            if(buddy_check==NULL)
            {
                old_name = ng_core_get_buddy_chat_name(buddy, NULL);
                purple_blist_alias_buddy(buddy, new_name);
                serv_alias_buddy(buddy);
                result = g_strdup_printf("%s tried to set his/her name to: %s",
                    old_name, new_name);
                g_free(old_name);
                ng_log_command_log(result);
                g_free(result);
            }
            else
            {
                if(buddy_check==buddy)
                    result = g_strdup_printf("Please do not change nick name "
                        "repeatly!");
                else
                    result = g_strdup_printf("Your nick name has been used, "
                        "please try another one.");
                purple_conv_im_send(conv_im, result);
                g_free(result);
            }
        }
    }
    g_strfreev(cmd_list);
}

static void ng_command_list_print_about(PurpleConvIm *conv_im)
{
    const gchar *text = "About NekoGroup<br/>"
        "A group chatting robot based on libpurple.<br/>"
        "Version: 0.0.1, build date: 2011-11-28<br/>"
        "Copyright (C) 2011 - SuperCat, license: GPL v3.";
    purple_conv_im_send(conv_im, text);
}

static void ng_command_list_history(PurpleAccount *account,
    PurpleConvIm *conv_im, const gchar *command)
{
    GList *history_list, *foreach = NULL;
    gchar **cmd_list = NULL;
    guint i;
    guint line = 40;
    gchar *text;
    GString *result_str;
    guint length;
    const PurpleConversation *conv;
    const gchar *name;
    PurpleBuddy *buddy;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    if(name==NULL) return;
    buddy = purple_find_buddy(account, name);
    if(buddy==NULL)
    {
        purple_conv_im_send(conv_im, "You should be added by administartors "
            "first!");
        return;
    }
    history_list = ng_log_get_history();
    if(history_list==NULL) return;
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL)
    {
        result_str = g_string_new("Latest ");
        if(cmd_list[1]!=NULL && *cmd_list[1]!='\0')
            sscanf(cmd_list[1], "%u", &line);
        if(line<5) line = 5;
        g_string_append_printf(result_str, "%u history lines:<br/>", line);
        length = g_list_length(history_list);
        if(length>line)
            foreach = g_list_nth(history_list, length - line);
        else
            foreach = history_list;
        if(foreach==NULL) foreach = history_list;
        for(i=0;foreach!=NULL && i<line;
            foreach=g_list_next(foreach), i++)
        {
            text = foreach->data;
            if(text==NULL) continue;
            g_string_append_printf(result_str, "%s<br/>", text);
        }
        g_string_append_printf(result_str, "%u/%u history line(s) printed.",
            i, length);
        purple_conv_im_send(conv_im, result_str->str);
        g_string_free(result_str, TRUE);
    }
    g_strfreev(cmd_list);
}

static void ng_command_talk_private(PurpleAccount *account,
    PurpleConvIm *conv_im, const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    gchar *nick;
    gchar **cmd_list = NULL;
    gchar *user, *message;
    PurpleBuddy *buddy;
    gboolean flag = FALSE;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    if(name==NULL) return;
    buddy = purple_find_buddy(account, name);
    if(buddy==NULL) return;
    nick = ng_core_get_buddy_chat_name(buddy, name);
    cmd_list = g_strsplit(command, " ", 3);
    if(cmd_list[0]!=NULL && cmd_list[1]!=NULL && strlen(cmd_list[1])>0 &&
        cmd_list[2]!=NULL && strlen(cmd_list[2])>0)
    {
        user = g_strstrip(cmd_list[1]);
        message = g_strdup_printf("(Private)[%s]: %s", nick,
            g_strstrip(cmd_list[2]));
        buddy = ng_core_get_buddy_by_nick(account, user);
        if(buddy==NULL)
           buddy = purple_find_buddy(account, user);
        if(buddy!=NULL)
        {
            ng_core_talk_to_buddy(account, buddy, message);
            flag = TRUE;
        }
    }
    g_free(nick);
    g_strfreev(cmd_list);
    if(!flag)
    {
        purple_conv_im_send(conv_im, "Failed to send private message!");
    }
}

void ng_command_add_user(PurpleAccount *account, PurpleConvIm *conv_im,
    const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    gchar *realname;
    gchar *new_user = NULL;
    gchar **cmd_list;
    PurpleBuddy *new_buddy;
    gint level = 0;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    realname = g_strdup(name);
    realname = g_strdelimit(realname, "/", '\0');
    level = ng_core_get_conversation_privilege(account, realname);
    g_free(realname);
    if(level<=0)
    {
        purple_conv_im_send(conv_im, "You are not a privileged user!");
        return;
    }
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL && cmd_list[1]!=NULL && *cmd_list[1]!='\0')
        new_user = g_strstrip(cmd_list[1]);
    if(new_user!=NULL)
        new_user = g_strdup(new_user);
    g_strfreev(cmd_list);
    if(new_user==NULL) return;
    new_buddy = purple_buddy_new(account, new_user, NULL);
    if(new_buddy==NULL) return;
    purple_blist_add_buddy(new_buddy, NULL, NULL, NULL);
    purple_account_add_buddy(account, new_buddy);
    purple_blist_request_add_buddy(account, new_user, NULL, NULL);
}

void ng_command_kick_user(PurpleAccount *account, PurpleConvIm *conv_im,
    const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    const gchar *victim_realname;
    PurpleBuddy *victim_buddy;
    gchar *realname;
    gchar **cmd_list;
    gchar *victim_user = NULL;
    gint level = 0, victim_level = 0;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    realname = g_strdup(name);
    realname = g_strdelimit(realname, "/", '\0');
    level = ng_core_get_conversation_privilege(account, realname);
    if(level<=0)
    {
        purple_conv_im_send(conv_im, "You are not a privileged user!");
        g_free(realname);
        return;
    }
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL && cmd_list[1]!=NULL && *cmd_list[1]!='\0')
        victim_user = g_strstrip(cmd_list[1]);
    if(victim_user!=NULL)
        victim_user = g_strdup(victim_user);
    g_strfreev(cmd_list);
    if(victim_user==NULL)
    {
        purple_conv_im_send(conv_im, "You need a target!");
        g_free(realname);
        g_free(victim_user);
        return;
    }
    victim_buddy = ng_core_get_buddy_by_nick(account, victim_user);
    if(victim_buddy==NULL)
    {
        purple_conv_im_send(conv_im, "Cannot find target user");
        g_free(realname);
        g_free(victim_user);
        return;
    }
    if(level>=2)
    {
        victim_realname = purple_buddy_get_name(victim_buddy);
        if(g_strcmp0(victim_realname, realname)==0)
        {
            purple_conv_im_send(conv_im, "Don not try to kick yourself, "
                "dear root user?");
            g_free(realname);
            g_free(victim_user);
            return;
        }
        purple_account_remove_buddy(account, victim_buddy, NULL);
    }
    if(level==1)
    {
        victim_realname = purple_buddy_get_name(victim_buddy);
        victim_level = ng_core_get_conversation_privilege(account,
            victim_realname);
        if(victim_level>=2)
        {
            purple_conv_im_send(conv_im, "How dare you to kick root user?");
            g_free(realname);
            g_free(victim_user);
            return;
        }
        else if(victim_level==1)
        {
            purple_conv_im_send(conv_im, "You cannot kick other privileged "
                "users.");
            g_free(realname);
            g_free(victim_user);
            return;
        }
        purple_account_remove_buddy(account, victim_buddy, NULL);
    }
    g_free(realname);
    g_free(victim_user);
}

void ng_command_upgrade_user(PurpleAccount *account, PurpleConvIm *conv_im,
    const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    gchar *realname;
    gchar *user = NULL;
    gchar **cmd_list;
    PurpleBuddy *buddy = NULL;
    PurpleGroup *group;
    gint level = 0;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    realname = g_strdup(name);
    realname = g_strdelimit(realname, "/", '\0');
    level = ng_core_get_conversation_privilege(account, realname);
    g_free(realname);
    if(level<=1)
    {
        purple_conv_im_send(conv_im, "You are not a root user!");
        return;
    }
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL && cmd_list[1]!=NULL && *cmd_list[1]!='\0')
    {
        user = g_strstrip(cmd_list[1]);
        buddy = ng_core_get_buddy_by_nick(account, user);
        if(buddy==NULL) buddy = purple_find_buddy(account, user);
    }
    g_strfreev(cmd_list);
    if(buddy==NULL) return;
    group = purple_find_group("admin");
    if(group==NULL)
    {
        group = purple_group_new("admin");
        purple_blist_add_group(group, NULL);
    }
    if(group==NULL) return;
    purple_blist_add_buddy(buddy, NULL, group, NULL);
}

void ng_command_downgrade_user(PurpleAccount *account, PurpleConvIm *conv_im,
    const gchar *command)
{
    const PurpleConversation *conv;
    const gchar *name;
    gchar *realname;
    gchar *user = NULL;
    gchar **cmd_list;
    PurpleBuddy *buddy = NULL;
    PurpleGroup *group;
    gint level = 0;
    conv = purple_conv_im_get_conversation(conv_im);
    if(conv==NULL) return;
    name = purple_conversation_get_name(conv);
    realname = g_strdup(name);
    realname = g_strdelimit(realname, "/", '\0');
    level = ng_core_get_conversation_privilege(account, realname);
    g_free(realname);
    if(level<=1)
    {
        purple_conv_im_send(conv_im, "You are not a root user!");
        return;
    }
    cmd_list = g_strsplit(command, " ", 2);
    if(cmd_list[0]!=NULL && cmd_list[1]!=NULL && *cmd_list[1]!='\0')
    {
        user = g_strstrip(cmd_list[1]);
        buddy = ng_core_get_buddy_by_nick(account, user);
    }
    g_strfreev(cmd_list);
    if(buddy==NULL) return;
    group = purple_find_group("user");
    if(group==NULL)
    {
        group = purple_group_new("user");
        purple_blist_add_group(group, NULL);
    }
    if(group==NULL) return;
    purple_blist_add_buddy(buddy, NULL, group, NULL);
}


gboolean ng_command_use(PurpleAccount *account, PurpleConvIm *conv_im,
    const gchar *command)
{
    if(conv_im==NULL || command==NULL) return FALSE;
    if(g_strcmp0(command, "-help")==0)
    {
        ng_command_list_print_help(conv_im);
        return TRUE;
    }
    else if(strncmp(command, "-nick", 5)==0)
    {
        ng_command_config_nick(account, conv_im, command);
        return TRUE;
    }
    else if(g_strcmp0(command, "-online")==0)
    {
        ng_command_list_online(account, conv_im);
        return TRUE;
    }
    else if(g_strcmp0(command, "-list")==0)
    {
        ng_command_list_user(account, conv_im);
        return TRUE;
    }
    else if(strncmp(command, "-history", 8)==0)
    {
        ng_command_list_history(account, conv_im, command);
        return TRUE;
    }

    else if(strncmp(command, "-talk", 5)==0)
    {
        ng_command_talk_private(account, conv_im, command);
        return TRUE;
    }
    else if(g_strcmp0(command, "-ping")==0)
    {
        purple_conv_im_send(conv_im, "Pong!");
        return TRUE;
    }
    else if(strncmp(command, "-add", 4)==0)
    {
        ng_command_add_user(account, conv_im, command);
        return TRUE;
    }
    else if(strncmp(command, "-kick", 5)==0)
    {
        ng_command_kick_user(account, conv_im, command);
        return TRUE;
    }
    else if(strncmp(command, "-upgrade", 8)==0)
    {
        ng_command_upgrade_user(account, conv_im, command);
        return TRUE;
    }
    else if(strncmp(command, "-downgrade", 10)==0)
    {
        ng_command_downgrade_user(account, conv_im, command);
        return TRUE;
    }
    else if(g_strcmp0(command, "-about")==0)
    {
        ng_command_list_print_about(conv_im);
        return TRUE;
    }
    return FALSE;
}


