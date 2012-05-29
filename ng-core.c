/*
 * NekoGroup Core Module Declaration
 *
 * ng-core.c
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

#include "ng-core.h"
#include "ng-common.h"
#include "ng-main.h"
#include "ng-utils.h"
#include "ng-marshal.h"

/**
 * SECTION: ng-core
 * @Short_description: The XMPP communication core
 * @Title: NGCore
 * @Include: ng-core.h
 *
 * The #NGCore is a class which commnicates with the XMPP server.
 * The core uses libloudmouth as its backend.
 */

#define NG_CORE_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    NG_CORE_TYPE, NGCorePrivate)

typedef struct NGCorePrivate
{
    LmConnection *connection;
    gchar *jid;
    gchar *server;
    guint16 port;
    gboolean use_ssl;
}NGCorePrivate;

typedef struct NGCoreConnectionPrivate
{
    gchar *jid;
    gchar *pass;
    gboolean result;
}NGCoreConnectonPrivate;

enum
{
    SIGNAL_MESSAGE,
    SIGNAL_SUBSCRIBE_REQUEST,
    SIGNAL_SUBSCRIBED,
    SIGNAL_UNSUBSCRIBE_REQUEST,
    SIGNAL_UNSUBSCRIBED,
    SIGNAL_BUDDY_UNAVAILABLE,
    SIGNAL_RECEIVE_ROSTER,
    SIGNAL_BUDDY_PRESENCE,
    SIGNAL_LAST
};

static GObject *core_instance = NULL;
static gpointer ng_core_parent_class = NULL;
static gint core_signals[SIGNAL_LAST] = {0};

static LmHandlerResult ng_core_message_handle_func(LmMessageHandler *handler,
    LmConnection *connection, LmMessage *message, gpointer data)
{
    LmMessageNode *node;
    LmMessageNode *body_node;
    const gchar *from, *body, *type;
    gchar *jid = NULL;
    if(core_instance==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    node = lm_message_get_node(message);
    if(node==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;    
    body_node = lm_message_node_get_child(node, "body");
    if(body_node==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    from = lm_message_node_get_attribute(node, "from");
    type = lm_message_node_get_attribute(node, "type");
    body = lm_message_node_get_value(body_node);
    jid = ng_utils_get_jid_from_address(from);
    if(jid==NULL) jid = g_strdup(from);
    if(from==NULL || body==NULL) 
    {
        g_free(jid);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    if(g_strcmp0(jid, lm_connection_get_jid(connection))==0)
    {
        g_free(jid);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    g_signal_emit(core_instance, core_signals[SIGNAL_MESSAGE], 0,
        jid, type, body);
    g_free(jid);
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static LmHandlerResult ng_core_presence_handle_func(LmMessageHandler *handler,
    LmConnection *connection, LmMessage *message, gpointer data)
{
    LmMessageNode *node, *status_node;
    const gchar *type, *from, *status;
    gchar *jid, *resource;
    if(core_instance==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    node = lm_message_get_node(message);
    if(node==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;    
    type = lm_message_node_get_attribute(node, "type");
    from = lm_message_node_get_attribute(node, "from");
    jid = ng_utils_get_jid_from_address(from);
    if(jid==NULL) jid = g_strdup(from);
    if(g_strcmp0(jid, lm_connection_get_jid(connection))==0)
    {
        g_free(jid);
        return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    }
    resource = ng_utils_get_resource_from_address(from);
    if(type!=NULL && from!=NULL)
    {
        if(g_strcmp0(type, "subscribe")==0)
        {
            g_signal_emit(core_instance,
                core_signals[SIGNAL_SUBSCRIBE_REQUEST], 0, jid);
        }
        else if(g_strcmp0(type, "subscribed")==0)
        {
            g_signal_emit(core_instance,
                core_signals[SIGNAL_SUBSCRIBED], 0, jid);
        }
        else if(g_strcmp0(type, "unsubscribe")==0)
        {
            g_signal_emit(core_instance,
                core_signals[SIGNAL_UNSUBSCRIBE_REQUEST], 0, jid);
        }
        else if(g_strcmp0(type, "unsubscribed")==0)
        {
            g_signal_emit(core_instance,
                core_signals[SIGNAL_UNSUBSCRIBED], 0, jid);
        }
        else if(g_strcmp0(type, "unavailable")==0)
        {
            g_signal_emit(core_instance,
                core_signals[SIGNAL_BUDDY_UNAVAILABLE], 0, jid);
        }
        else g_print("Presence type: %s found.\n", type);
    }
    if(type==NULL && from!=NULL)
    {
        status_node = lm_message_node_get_child(node, "status");
        if(status_node!=NULL)
            status = lm_message_node_get_value(status_node);
        else
            status = NULL;
        g_signal_emit(core_instance,
            core_signals[SIGNAL_BUDDY_PRESENCE], 0, jid, resource, status);
    }
    
    g_free(jid);
    g_free(resource);
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static gboolean ng_core_send_pong_message(LmConnection *connection,
    const gchar *server, const gchar *id, GError **error)
{
    gboolean flag;
    LmMessage *reply_msg;
    reply_msg = lm_message_new(server, LM_MESSAGE_TYPE_IQ);
    lm_message_node_set_attributes(reply_msg->node, "type", "result",
        "id", id, NULL);
    flag = lm_connection_send(connection, reply_msg, error);
    lm_message_unref(reply_msg);
    return flag;
}

static LmHandlerResult ng_core_iq_handle_func(LmMessageHandler *handler,
    LmConnection *connection, LmMessage *message, gpointer data)
{
    LmMessageNode *node;
    LmMessageNode *ping_node, *query_node, *item_node;
    const gchar *id, *from, *jid, *subscription;
    const gchar *self_jid;
    //const gchar *type;
    GError *error = NULL;
    node = lm_message_get_node(message);
    if(node==NULL) return LM_HANDLER_RESULT_REMOVE_MESSAGE;
    //type = lm_message_node_get_attribute(node, "type");
    from = lm_message_node_get_attribute(node, "from");
    id = lm_message_node_get_attribute(node, "id");
    ping_node = lm_message_node_get_child(node, "ping");
    query_node = lm_message_node_get_child(node, "query");
    self_jid = lm_connection_get_jid(connection);
    
    if(ping_node!=NULL && from!=NULL)
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
            "Got ping message from %s.", from);
        if(id==NULL) id = "ping";
        if(!ng_core_send_pong_message(connection, from, id, &error))
        {
            g_warning("Failed to send pong message: %s", error->message);
            g_error_free(error);
        }
        else
        {
            g_message("Replied pong to %s", from);
        }
    }
    if(query_node!=NULL)
        item_node = lm_message_node_get_child(query_node, "item");
    else item_node = NULL;
    if(item_node!=NULL)
    {
        item_node = lm_message_node_get_child(query_node, "item");
        for(;item_node!=NULL;item_node=item_node->next)
        {
            jid = lm_message_node_get_attribute(item_node, "jid");
            if(jid==NULL) continue;
            if(g_strcmp0(self_jid, jid)==0) continue;
            subscription = lm_message_node_get_attribute(item_node,
                "subscription");
            g_signal_emit(core_instance,
                core_signals[SIGNAL_RECEIVE_ROSTER], 0, jid, subscription);
        }
    }
    return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void ng_core_connection_close_cb(LmConnection *connection,
    LmDisconnectReason reason, gpointer data)
{
    g_message("Disconnected from server, reason: %d", reason);
    ng_main_quit();
}

static void ng_core_finalize(GObject *object)
{
    NGCorePrivate *priv = NG_CORE_GET_PRIVATE(NG_CORE(object));
    if(priv->connection!=NULL)
    {
        lm_connection_close(priv->connection, NULL);
        lm_connection_unref(priv->connection);
        priv->connection = NULL;
    }
    
    G_OBJECT_CLASS(ng_core_parent_class)->finalize(object);
}

static GObject *ng_core_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(core_instance!=NULL) return core_instance;
    retval = G_OBJECT_CLASS(ng_core_parent_class)->constructor
        (type, n_construct_params, construct_params);
    core_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&core_instance);
    return retval;
}

static void ng_core_class_init(NGCoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    ng_core_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = ng_core_finalize;
    object_class->constructor = ng_core_constructor;
    g_type_class_add_private(klass, sizeof(NGCorePrivate));
    
    core_signals[SIGNAL_MESSAGE] = g_signal_new("message",
        NG_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(NGCoreClass,
        message), NULL, NULL, ng_marshal_VOID__STRING_STRING_STRING,
        G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, NULL);
        
    core_signals[SIGNAL_SUBSCRIBE_REQUEST] = g_signal_new(
        "subscribe-request", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, subscribe_request), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

    core_signals[SIGNAL_SUBSCRIBED] = g_signal_new(
        "subscribed", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, subscribed), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

    core_signals[SIGNAL_UNSUBSCRIBE_REQUEST] = g_signal_new(
        "unsubscribe-request", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, unsubscribe_request), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    core_signals[SIGNAL_UNSUBSCRIBED] = g_signal_new(
        "unsubscribed", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, unsubscribed), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    core_signals[SIGNAL_BUDDY_UNAVAILABLE] = g_signal_new(
        "buddy-unavailable", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, buddy_unavailable), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    core_signals[SIGNAL_RECEIVE_ROSTER] = g_signal_new(
        "receive-roster", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, receive_roster), NULL, NULL,
        ng_marshal_VOID__STRING_STRING, G_TYPE_NONE, 2, G_TYPE_STRING,
        G_TYPE_STRING, NULL);
        
    core_signals[SIGNAL_BUDDY_PRESENCE] = g_signal_new(
        "buddy-presence", NG_CORE_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(NGCoreClass, buddy_presence), NULL, NULL,
        ng_marshal_VOID__STRING_STRING_STRING, G_TYPE_NONE, 3,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, NULL);
}

static void ng_core_instance_init(NGCore *core)
{

}

GType ng_core_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo core_info = {
        .class_size = sizeof(NGCoreClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)ng_core_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(NGCore),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)ng_core_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("NGCore"), &core_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

gint ng_core_init(const gchar *server, guint16 port, gboolean use_ssl,
    const gchar *uid, const gchar *user, const gchar *pass)
{
    NGCorePrivate *priv = NULL;
    LmConnection *connection;
    GError *error = NULL;
    LmMessageHandler *handler;
    LmSSL *ssl;
    LmMessage *msg;
    LmMessageNode *query_node;
    if(server==NULL || uid==NULL)
        return -1;
    connection = lm_connection_new(server);
    if(connection==NULL)
        return -2;
    if(port==0)
    {
        if(use_ssl)
            port = LM_CONNECTION_DEFAULT_PORT_SSL;
        else
            port = LM_CONNECTION_DEFAULT_PORT;
    }
    lm_connection_set_jid(connection, uid);
    lm_connection_set_port(connection, port);
    lm_connection_set_keep_alive_rate(connection, 15);
    handler = lm_message_handler_new(ng_core_message_handle_func,
        NULL, NULL);
    lm_connection_register_message_handler(connection, handler,
        LM_MESSAGE_TYPE_MESSAGE, LM_HANDLER_PRIORITY_NORMAL);
    lm_message_handler_unref(handler);
    handler = lm_message_handler_new(ng_core_presence_handle_func,
        NULL, NULL);
    lm_connection_register_message_handler(connection, handler,
        LM_MESSAGE_TYPE_PRESENCE, LM_HANDLER_PRIORITY_NORMAL);
    lm_message_handler_unref(handler);
    handler = lm_message_handler_new(ng_core_iq_handle_func,
        NULL, NULL);
    lm_connection_register_message_handler(connection, handler,
        LM_MESSAGE_TYPE_IQ, LM_HANDLER_PRIORITY_NORMAL);
    lm_message_handler_unref(handler);
    lm_connection_set_disconnect_function(connection,
        ng_core_connection_close_cb, NULL, NULL);
    if(use_ssl)
    {
        ssl = lm_ssl_new(NULL, NULL, NULL, NULL);
        lm_ssl_use_starttls(ssl, TRUE, FALSE);
        lm_connection_set_ssl(connection, ssl);
        lm_ssl_unref(ssl);
    }

    if(!lm_connection_open_and_block(connection, &error))
    {
        g_warning("Cannot connect to server %s: %s", server,
            error->message);
        lm_connection_unref(connection);
        g_error_free(error);
        error = NULL;
        return -3;
    }
    g_message("Connected to the XMPP server.");
    
    if(!lm_connection_authenticate_and_block(connection, user, pass,
        "bot", &error))
    {
        g_warning("Failed to authenticate %s", error->message);
        lm_connection_unref(connection);
        g_error_free(error);
        error = NULL;
        return -4;
	}
    
    g_message("Authenticated successfully.");
    
    core_instance = g_object_new(NG_CORE_TYPE, NULL);
    priv = NG_CORE_GET_PRIVATE(core_instance);
    priv->connection = connection;

    msg = lm_message_new(NULL, LM_MESSAGE_TYPE_MESSAGE);
    lm_message_node_add_child(msg->node, "body", " ");
    lm_connection_send(priv->connection, msg, NULL);
    lm_message_unref(msg);

    /* Send presence message. */
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_AVAILABLE);
    lm_connection_send(connection, msg, NULL);
    lm_message_unref(msg);
    
    /* Query for the roster with nested groups. */
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
        LM_MESSAGE_SUB_TYPE_GET);
    query_node = lm_message_node_add_child(msg->node, "query", NULL);
    lm_message_node_set_attribute(query_node, "xmlns", "jabber:iq:roster");
    lm_connection_send(connection, msg, NULL);
    lm_message_unref(msg);
    
    return 0;
}

void ng_core_exit()
{
    if(core_instance!=NULL) g_object_unref(core_instance);
    core_instance = NULL;
}

gulong ng_core_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(core_instance==NULL) return 0;
    return g_signal_connect(core_instance, name, callback, data);
}

void ng_core_signal_disconnect(gulong handler_id)
{
    if(core_instance==NULL) return;
    g_signal_handler_disconnect(core_instance, handler_id);
}

gboolean ng_core_send_message(const gchar *jid, const gchar *message)
{
    NGCorePrivate *priv = NULL;
    LmMessage *reply_msg;
    gboolean flag;
    if(core_instance==NULL || jid==NULL || message==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    reply_msg = lm_message_new(jid, LM_MESSAGE_TYPE_MESSAGE);
    lm_message_node_add_child(reply_msg->node, "body", message);
    flag = lm_connection_send(priv->connection, reply_msg, NULL);
    lm_message_unref(reply_msg);
    return flag;
}

gboolean ng_core_send_subscribe_request(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    gboolean flag;
    LmMessage *reply_msg;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    reply_msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_SUBSCRIBE);
    flag = lm_connection_send(priv->connection, reply_msg, NULL);
    lm_message_unref(reply_msg);
    return flag;
}

gboolean ng_core_send_subscribed_message(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    gboolean flag;
    LmMessage *reply_msg;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    reply_msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_SUBSCRIBED);
    flag = lm_connection_send(priv->connection, reply_msg, NULL);
    lm_message_unref(reply_msg);
    return flag;
}

gboolean ng_core_send_unsubscribe_request(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    gboolean flag;
    LmMessage *reply_msg;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    reply_msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_UNSUBSCRIBE);
    flag = lm_connection_send(priv->connection, reply_msg, NULL);
    lm_message_unref(reply_msg);
    return flag;
}

gboolean ng_core_send_unsubscribed_message(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    gboolean flag;
    LmMessage *reply_msg;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    reply_msg = lm_message_new_with_sub_type(jid, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_UNSUBSCRIBED);
    flag = lm_connection_send(priv->connection, reply_msg, NULL);
    lm_message_unref(reply_msg);
    return flag;
}

gboolean ng_core_set_title(const gchar *title)
{
    NGCorePrivate *priv = NULL;
    LmMessage *msg;
    gboolean flag;
    if(core_instance==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_AVAILABLE);
    if(title!=NULL)
        lm_message_node_add_child(msg->node, "status", title);
    flag = lm_connection_send(priv->connection, msg, NULL);
    lm_message_unref(msg);
    return flag;
}

gboolean ng_core_add_roster(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    LmMessage *msg;
    LmMessageNode *query_node, *item_node;
    gboolean flag;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
        LM_MESSAGE_SUB_TYPE_SET);
    lm_message_node_set_attribute(msg->node, "id", "adduser1");
    query_node = lm_message_node_add_child(msg->node, "query", NULL);
    lm_message_node_set_attribute(query_node, "xmlns", "jabber:iq:roster");
    item_node = lm_message_node_add_child(query_node, "item", NULL);
    lm_message_node_set_attribute(item_node, "jid", jid);
    lm_message_node_set_attribute(item_node, "subscription", "none");
    lm_connection_send(priv->connection, msg, NULL);
    lm_message_unref(msg);
    return flag;
}

gboolean ng_core_remove_roster(const gchar *jid)
{
    NGCorePrivate *priv = NULL;
    LmMessage *msg;
    LmMessageNode *query_node, *item_node;
    gboolean flag;
    if(core_instance==NULL || jid==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
        LM_MESSAGE_SUB_TYPE_SET);
    lm_message_node_set_attribute(msg->node, "id", "removeuser1");
    query_node = lm_message_node_add_child(msg->node, "query", NULL);
    lm_message_node_set_attribute(query_node, "xmlns", "jabber:iq:roster");
    item_node = lm_message_node_add_child(query_node, "item", NULL);
    lm_message_node_set_attribute(item_node, "jid", jid);
    lm_message_node_set_attribute(item_node, "subscription", "remove");
    lm_connection_send(priv->connection, msg, NULL);
    lm_message_unref(msg);
    return flag;
}

gboolean ng_core_send_unavailable_message()
{
    NGCorePrivate *priv = NULL;
    LmMessage *msg;
    gboolean flag;
    if(core_instance==NULL) return FALSE;
    priv = NG_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return FALSE;
    msg = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_PRESENCE,
        LM_MESSAGE_SUB_TYPE_UNAVAILABLE);
    flag = lm_connection_send(priv->connection, msg, NULL);
    lm_message_unref(msg);
    return flag;
}

