/*
 * Core Module Declaration
 *
 * core.c
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

#include "core.h"
#include "command.h"
#include "log.h"
#include "debug.h"

#define NG_CORE_UI_ID "NekoGroup"
#define NG_CORE_CUSTOM_PLUGIN_PATH ""
#define NG_CORE_PLUGIN_SAVE_PREF "/purple/user/plugins/saved"
#define PURPLE_GLIB_READ_COND (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

static gchar module_name[] = "Core";
static gchar *ng_core_program_dir = NULL;
static GKeyFile *ng_core_config = NULL;
static GSList *ng_core_account_data = NULL;

typedef struct PurpleGLibIOClosure {
    PurpleInputFunction function;
    guint result;
    gpointer data;
}PurpleGLibIOClosure;

typedef struct PurpleAccountRequestInfo {
    PurpleAccountRequestType type;
    PurpleAccount *account;
    gpointer ui_handle;
    gchar *user;
    gpointer userdata;
    PurpleAccountRequestAuthorizationCb auth_cb;
    PurpleAccountRequestAuthorizationCb deny_cb;
    guint ref;
}PurpleAccountRequestInfo;

static gboolean ng_core_purple_glib_io_invoke(GIOChannel *source,
    GIOCondition condition, gpointer data)
{
    PurpleGLibIOClosure *closure = data;
    PurpleInputCondition purple_cond = 0;
    if(condition & PURPLE_GLIB_READ_COND)
        purple_cond |= PURPLE_INPUT_READ;
    if(condition & PURPLE_GLIB_WRITE_COND)
        purple_cond |= PURPLE_INPUT_WRITE;
    closure->function(closure->data, g_io_channel_unix_get_fd(source),
        purple_cond);
    return TRUE;
}

static guint ng_core_glib_input_add(gint fd, PurpleInputCondition condition,
    PurpleInputFunction function, gpointer data)
{
    PurpleGLibIOClosure *closure;
    GIOChannel *channel;
    GIOCondition cond = 0;
    closure = g_new0(PurpleGLibIOClosure, 1);
    closure->function = function;
    closure->data = data;
    if(condition & PURPLE_INPUT_READ)
        cond |= PURPLE_GLIB_READ_COND;
    if(condition & PURPLE_INPUT_WRITE)
        cond |= PURPLE_GLIB_WRITE_COND;
    channel = g_io_channel_unix_new(fd);
    closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
        ng_core_purple_glib_io_invoke, closure, g_free);
    g_io_channel_unref(channel);
    return closure->result;
}

static PurpleEventLoopUiOps ng_core_glib_eventloops =
{
    .timeout_add = g_timeout_add,
    .timeout_remove = g_source_remove,
    .input_add = ng_core_glib_input_add,
    .input_remove = g_source_remove,
    .input_get_error = NULL,
    #if GLIB_CHECK_VERSION(2, 14, 0)
        .timeout_add_seconds = g_timeout_add_seconds,
    #else
        .timeout_add_secounds = NULL,
    #endif
};

static void ng_core_network_disconnected()
{
    ng_debug_module_perror(module_name, "This machine has been disconnected "
        "from the internet!");
}

static void ng_core_report_disconnect_reason(PurpleConnection *gc, 
    PurpleConnectionError reason, const gchar *text)
{
    PurpleAccount *account = purple_connection_get_account(gc);
    ng_debug_module_perror(module_name, "Connection disconnected: \"%s\" "
        "(%s)\n >Error: %d\n  >Reason: %s",
        purple_account_get_username(account),
        purple_account_get_protocol_id(account), reason, text);
}

static PurpleConnectionUiOps ng_core_connection_uiops =
{
    .connect_progress = NULL,
    .connected = NULL,
    .disconnected = NULL,
    .notice = NULL,
    .report_disconnect = NULL,
    .network_connected = NULL,
    .network_disconnected = ng_core_network_disconnected,
    .report_disconnect_reason = ng_core_report_disconnect_reason
};

static void ng_core_ui_init()
{
    purple_connections_set_ui_ops(&ng_core_connection_uiops);
}

static PurpleCoreUiOps ng_core_uiops =
{
    .ui_prefs_init = NULL,
    .debug_ui_init = NULL,
    .ui_init = ng_core_ui_init,
    .quit = NULL,
    .get_ui_info = NULL,
};

static void ng_core_signed_on(PurpleConnection *gc)
{
    PurpleAccount *account = purple_connection_get_account(gc);
    ng_debug_module_pmsg(module_name, "Account connected: \"%s\" (%s)",
        purple_account_get_username(account),
        purple_account_get_protocol_id(account));
}

static void ng_core_received_im_msg(PurpleAccount *account, gchar *sender,
    gchar *message, PurpleConversation *conv, PurpleMessageFlags flags)
{
    PurpleBuddy *buddy;
    PurpleConvIm *im;
    gchar *return_msg;
    gchar *chat_name;
    gchar *tmp1, *tmp2, *msg;
    if(conv==NULL)
        conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, sender);
    purple_conversation_set_logging(conv, FALSE);
    if(message==NULL) return;
    tmp1 = purple_markup_strip_html(message);
    if(tmp1==NULL) return;
    tmp2 = purple_markup_escape_text(tmp1, -1);
    if(tmp2==NULL) return;
    msg = purple_strdup_withhtml(tmp2);
    g_free(tmp2);
    if(msg==NULL) return;
    if(msg!=NULL && *msg=='-')
    {
        im = purple_conversation_get_im_data(conv);
        if(!ng_command_use(account, im, msg))
            purple_conv_im_send(im, "Invalid command!");
        g_free(msg);
        return;
    }
    buddy = purple_find_buddy(account, sender);
    chat_name = ng_core_get_buddy_chat_name(buddy, sender);
    return_msg = g_strdup_printf("[%s]: %s", chat_name, msg);
    g_free(msg);
    g_free(chat_name);
    ng_log_write_public_message(return_msg);
    ng_core_do_broadcast(account, conv, return_msg);
    g_free(return_msg);
}

static void ng_core_buddy_added(PurpleBuddy *buddy)
{
    const gchar *group_name;
    gchar *short_name;
    gchar *text;
    gchar *nick;
    PurpleAccount *account;
    PurpleGroup *group;
    account = purple_buddy_get_account(buddy);
    if(account==NULL) return;
    short_name = g_strdup(purple_buddy_get_name(buddy));
    if(short_name==NULL) short_name = g_strdup("unknown user");
    g_strdelimit(short_name, "@", '\0');
    group = purple_buddy_get_group(buddy);
    group_name = purple_group_get_name(group);
    if(g_strcmp0(group_name, "admin")==0)
    {
        nick = ng_core_get_buddy_chat_name(buddy, short_name);
        text = g_strdup_printf("User %s was upgraded to be an administrator!",
            nick);
        g_free(nick);
    }
    else if(g_strcmp0(group_name, "user")==0)
    {
        nick = ng_core_get_buddy_chat_name(buddy, short_name);
        text = g_strdup_printf("User %s was downgraded to be a normal user!",
            nick);
        g_free(nick);
    }
    else
        text = g_strdup_printf("User %s was added to this group", short_name);
    g_free(short_name);
    ng_core_do_broadcast(account, NULL, text);
    g_free(text);
}

static void ng_core_buddy_removed(PurpleBuddy *buddy)
{
    gchar *name;
    gchar *text;
    PurpleAccount *account;
    account = purple_buddy_get_account(buddy);
    if(account==NULL) return;
    name = ng_core_get_buddy_chat_name(buddy, NULL);;
    if(name==NULL) name = g_strdup("unknown user");
    text = g_strdup_printf("User %s was kicked from this group", name);
    g_free(name);
    ng_core_do_broadcast(account, NULL, text);
    g_free(text);
}

static void ng_core_buddy_aliased(PurpleBlistNode *node,
    const gchar *old_alias)
{
    const gchar *new_name = NULL;
    gchar *old_name = NULL;
    gchar *text;
    PurpleBuddy *buddy;
    PurpleAccount *account;
    if(node==NULL) return;
    if(purple_blist_node_get_type(node)!=PURPLE_BLIST_BUDDY_NODE) return;
    buddy = PURPLE_BUDDY(node);
    account = purple_buddy_get_account(buddy);
    if(account==NULL) return;
    if(old_alias==NULL)
    {
        old_name = g_strdup(purple_buddy_get_name(buddy));
        if(old_name!=NULL) old_name = g_strdelimit(old_name, "@", '\0');
    }
    else
        old_name = g_strdup(old_alias);
    new_name = purple_buddy_get_alias_only(buddy);
    text = g_strdup_printf("%s set his/her name to: %s", old_name, new_name);
    if(old_name!=NULL) g_free(old_name);
    ng_core_do_broadcast(account, NULL, text);
    g_free(text);
}

static void ng_core_connect_signals()
{
    static gint handle;
    purple_signal_connect(purple_connections_get_handle(), "signed-on",
        &handle, PURPLE_CALLBACK(ng_core_signed_on), NULL);
    purple_signal_connect(purple_conversations_get_handle(), "received-im-msg",
        &handle, PURPLE_CALLBACK(ng_core_received_im_msg), NULL);
    purple_signal_connect(purple_blist_get_handle(), "buddy-added",
        &handle, PURPLE_CALLBACK(ng_core_buddy_added), NULL);
    purple_signal_connect(purple_blist_get_handle(), "buddy-removed",
        &handle, PURPLE_CALLBACK(ng_core_buddy_removed), NULL);
    purple_signal_connect(purple_blist_get_handle(), "blist-node-aliased",
        &handle, PURPLE_CALLBACK(ng_core_buddy_aliased), NULL);
}

static gint ng_core_account_auth_requested_cb(PurpleAccount *account,
    const gchar *user)
{
    gchar *text;
    ng_debug_module_pmsg(module_name, "User %s wants to join group %s.",
        user, purple_account_get_username(account));
    text = g_strdup_printf("User: %s joined this group, requesting "
        "authorization...", user);
    ng_core_do_broadcast(account, NULL, text);
    g_free(text);
    return 1;
}

static gchar *ng_core_get_program_dir(const gchar *argv0)
{
    gchar *data_dir = NULL;
    gchar *bin_dir = NULL;
    gchar *exec_path = NULL;
    char full_path[PATH_MAX];
    #ifdef G_OS_UNIX
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
    #endif
    #ifdef G_OS_WIN32
        bzero(full_path, PATH_MAX);
        GetModuleFileName(NULL, full_path, PATH_MAX);
        bin_dir = g_path_get_dirname(full_path);
    #endif
    if(bin_dir!=NULL)
    {
        data_dir = g_strdup(bin_dir);
    }
    #ifdef G_OS_UNIX
        if(data_dir==NULL)
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
            g_free(exec_path);
            exec_path = realpath(data_dir, full_path);
            if(exec_path!=NULL)
                bin_dir = g_path_get_dirname(exec_path);
            else
                bin_dir = NULL;
            if(bin_dir!=NULL)
            {
                data_dir = g_strdup(bin_dir);
                if(!g_file_test(data_dir, G_FILE_TEST_IS_DIR))
                {
                    g_free(data_dir);
                    data_dir = g_strdup(bin_dir);
                }
                g_free(bin_dir);
            }
        }
    #endif
    if(data_dir==NULL) data_dir = g_get_current_dir();
    return data_dir;
}

gboolean ng_core_init(gint *argc, gchar **argv[])
{
    gboolean flag = FALSE;
    gchar *keyfile_path;
    gchar *data_path;
    ng_core_program_dir = ng_core_get_program_dir(*argv[0]);
    data_path = g_build_filename(ng_core_program_dir, "data", NULL);
    purple_util_set_user_dir(data_path);
    g_free(data_path);
    purple_debug_set_enabled(FALSE);
    purple_core_set_ui_ops(&ng_core_uiops);
    purple_eventloop_set_ui_ops(&ng_core_glib_eventloops);
    purple_plugins_add_search_path(NG_CORE_CUSTOM_PLUGIN_PATH);
    if(!purple_core_init(NG_CORE_UI_ID))
    {
        ng_debug_module_perror(module_name,
            "libpurple initialization failed! Abort!");
        return FALSE;
    }
    purple_set_blist(purple_blist_new());
    purple_blist_load();
    purple_prefs_load();
    purple_plugins_load_saved(NG_CORE_PLUGIN_SAVE_PREF);
    purple_pounces_load();
    ng_debug_module_pmsg(module_name, "libpurple initialized. "
        "Running version %s.", purple_core_get_version());
    ng_core_config = g_key_file_new();
    if(ng_core_program_dir!=NULL)
    {
        keyfile_path = g_build_filename(ng_core_program_dir, "data",
            "config.ini", NULL);
        flag = g_key_file_load_from_file(ng_core_config, keyfile_path,
            G_KEY_FILE_NONE, NULL);
        g_free(keyfile_path);
    }
    if(!flag)
    {
        ng_debug_module_perror(module_name,
            "Cannot load configure file! Abort!");
        return FALSE;
    }
    ng_core_connect_signals();
    return TRUE;
}

gboolean ng_core_account_init()
{
    NGCoreAccountData *account_data;
    gsize length;
    guint count = 0;
    guint i;
    gchar **groups;
    PurpleAccount *account = NULL;
    PurpleSavedStatus *status;
    gchar *id, *pass, *protocol, *title, *root, *server;
    gboolean req_tls;
    gint port;
    groups = g_key_file_get_groups(ng_core_config, &length);
    if(groups==NULL) return FALSE;
    for(i=0;groups[i]!=NULL;i++)
    {
        if(g_str_has_prefix(groups[i], "Account"))
        {
            protocol = g_key_file_get_string(ng_core_config, groups[i],
                "Protocol", NULL);
            id = g_key_file_get_string(ng_core_config, groups[i],
                "ID", NULL);
            pass = g_key_file_get_string(ng_core_config, groups[i],
                "Password", NULL);
            server = g_key_file_get_string(ng_core_config, groups[i],
                "Server", NULL);
            req_tls = g_key_file_get_boolean(ng_core_config, groups[i],
                "RequireTLS", NULL);
            port = g_key_file_get_boolean(ng_core_config, groups[i],
                "Port", NULL);
            if(protocol!=NULL && strlen(protocol)>0 && id!=NULL &&
                strlen(id)>0 && pass!=NULL && strlen(pass)>0)
            {
                account = purple_account_new(id, protocol);
                purple_account_set_password(account, pass);
                if(port>0 && port<65535)
                    purple_account_set_int(account, "port", port);
                if(req_tls)
                    purple_account_set_bool(account, "require_tls", TRUE);
                if(server!=NULL)
                    purple_account_set_string(account, "server", server);
                purple_accounts_add(account);
                purple_account_set_enabled(account, ng_core_get_ui_id(),
                    TRUE);
                purple_signal_connect(purple_accounts_get_handle(),
                    "account-authorization-requested", account,
                    PURPLE_CALLBACK(ng_core_account_auth_requested_cb),
                    NULL);
                account_data = g_new0(NGCoreAccountData, 1);
                count++;
                status = purple_savedstatus_get_default();
                purple_savedstatus_set_idleaway(FALSE);
                purple_savedstatus_set_type(status, PURPLE_STATUS_AVAILABLE);
                title = g_key_file_get_string(ng_core_config, groups[i],
                    "Title", NULL);
                root = g_key_file_get_string(ng_core_config, groups[i],
                    "Root", NULL);
                if(title!=NULL && strlen(title)>0)
                    purple_savedstatus_set_message(status, title);
                else
                    purple_savedstatus_set_message(status, "");
                purple_savedstatus_activate_for_account(status, account);
                account_data->account = account;
                if(title!=NULL && strlen(title)>0)
                    account_data->title = g_strdup(title);
                if(root!=NULL && strlen(root)>0)
                    account_data->root = g_strdup(root);
                ng_core_account_data = g_slist_append(ng_core_account_data,
                    account_data);
                if(title!=NULL) g_free(title);
                if(root!=NULL) g_free(root);
            }
            if(protocol!=NULL) g_free(protocol);
            if(id!=NULL) g_free(id);
            if(pass!=NULL) g_free(pass);
            if(server!=NULL) g_free(server);
        }
    }
    if(count>=1)
    {
        ng_debug_module_pmsg(module_name, "Accounts loaded, total: %u", count);
        return TRUE;
    }
    ng_debug_module_perror(module_name, "There is no available accounts!");
    return FALSE;
}

const gchar *ng_core_get_ui_id()
{
    return NG_CORE_UI_ID;
}

PurpleConversation *ng_core_get_conversation(PurpleAccount *account,
    const PurpleBuddy *buddy)
{
    PurpleConversation *conv;
    const gchar *name;
    if(!PURPLE_BUDDY_IS_ONLINE(buddy)) return NULL;
    name = purple_buddy_get_name(buddy);
    conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, name,
        account);
    if(conv==NULL)
        conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, name);
    return conv;
}

gchar *ng_core_get_buddy_chat_name(PurpleBuddy *buddy, const gchar *sender)
{
    const gchar *nick_name = NULL;
    gchar *short_name;
    if(buddy==NULL && sender==NULL) return NULL;
    if(buddy!=NULL)
        nick_name = purple_buddy_get_alias_only(buddy);
    if(nick_name!=NULL)
        short_name = g_strdup(nick_name);
    else
    {
        if(buddy!=NULL) 
            short_name = g_strdup(purple_buddy_get_name(buddy));
        else
            short_name = g_strdup(sender);
        g_strdelimit(short_name, "@", '\0');
    }
    return short_name;
}

PurpleBuddy *ng_core_get_buddy_by_nick(PurpleAccount *account,
    const gchar *nick)
{
    PurpleBuddy *buddy;
    const gchar *nick_name;
    GSList *list, *foreach;
    gboolean flag = FALSE;
    list = purple_find_buddies(account, NULL);
    for(foreach=list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        buddy = foreach->data;
        if(buddy==NULL) continue;
        nick_name = purple_buddy_get_alias_only(buddy);
        if(nick_name!=NULL && g_strcmp0(nick_name, nick)==0)
        {
            flag = TRUE;
            break;
        }
    }
    g_slist_free(list);
    if(!flag) buddy = NULL;
    return buddy;
}

void ng_core_do_broadcast(PurpleAccount *account, PurpleConversation *conv,
    const gchar *message)
{
    GSList *buddy_list, *foreach;
    PurpleConversation *new_conv;
    PurpleBuddy *buddy;
    PurpleConvIm *im;
    if(account==NULL || message==NULL) return;
    buddy_list = purple_find_buddies(account, NULL);
    for(foreach=buddy_list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        buddy = foreach->data;
        if(buddy==NULL) continue;
        new_conv = ng_core_get_conversation(account, buddy);
        if(new_conv!=NULL && new_conv!=conv)
        {
            im = purple_conversation_get_im_data(new_conv);
            purple_conv_im_send(im, message);
        }
    }
    g_slist_free(buddy_list);
}

void ng_core_talk_to_buddy(PurpleAccount *account, PurpleBuddy *buddy,
    const gchar *message)
{
    PurpleConversation *conv;
    PurpleConvIm *im;
    if(account==NULL || buddy==NULL || message==NULL) return;
    conv = ng_core_get_conversation(account, buddy);
    if(conv==NULL) return;
    im = purple_conversation_get_im_data(conv);
    purple_conv_im_send(im, message);
}

NGCoreAccountData *ng_core_get_account_data(PurpleAccount *account)
{
    GSList *foreach;
    NGCoreAccountData *account_data = NULL;
    for(foreach=ng_core_account_data;foreach!=NULL;
        foreach=g_slist_next(foreach))
    {
        account_data = foreach->data;
        if(account_data!=NULL && account_data->account==account)
            return account_data;
    }
    return NULL;
}

gint ng_core_get_conversation_privilege(PurpleAccount *account,
    const gchar *name)
{
    NGCoreAccountData *account_data;
    PurpleGroup *group;
    PurpleBuddy *buddy;
    const gchar *rootname;
    if(account==NULL || name==NULL) return 0;
    account_data = ng_core_get_account_data(account);
    if(account_data==NULL) return 0;
    rootname = account_data->root;
    if(g_strcmp0(rootname, name)==0) return 2;
    group = purple_find_group("admin");
    if(group==NULL) return 0;
    buddy = purple_find_buddy_in_group(account, name, group);
    if(buddy!=NULL) return 1;
    return 0;
}

