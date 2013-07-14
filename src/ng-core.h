/*
 * NekoGroup Core Header Declaration
 *
 * ng-core.h
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

#ifndef HAVE_NG_CORE_H
#define HAVE_NG_CORE_H

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <loudmouth/loudmouth.h>

G_BEGIN_DECLS

#define NG_CORE_TYPE (ng_core_get_type())
#define NG_CORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), NG_CORE_TYPE, \
    NGCore))
#define NG_CORE_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), NG_CORE_TYPE, \
    NGCoreClass))
#define NG_IS_CORE(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), NG_CORE_TYPE))
#define NG_IS_CORE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), NG_CORE_TYPE))
#define NG_CORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    NG_CORE_TYPE, NGCoreClass))

typedef struct _NGCore NGCore;
typedef struct _NGCoreClass NGCoreClass;

/**
 * NGCore:
 *
 * The communication core. The contents of the #NGCore structure are
 * private and should only be accessed via the provided API.
 */

struct _NGCore {
    /*< private >*/
    GObject parent;
};

/**
 * NGCoreClass:
 *
 * #NGCore class.
 */

struct _NGCoreClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*message)(NGCore *core, const gchar *from, const gchar *type,
        const gchar *body);
    void (*subscribe_request)(NGCore *core, const gchar *jid);
    void (*subscribed)(NGCore *core, const gchar *jid);
    void (*unsubscribe_request)(NGCore *core, const gchar *jid);
    void (*unsubscribed)(NGCore *core, const gchar *jid);
    void (*buddy_presence)(NGCore *core, const gchar *jid,
        const gchar *resource, const gchar *status);
    void (*buddy_unavailable)(NGCore *core, const gchar *jid);
    void (*receive_roster)(NGCore *core, const gchar *jid,
        const gchar *subscription);
};

/*< private >*/
GType ng_core_get_type();

/*< public >*/
gint ng_core_init(const gchar *server, guint16 port, gboolean use_ssl,
    const gchar *uid, const gchar *user, const gchar *pass);
void ng_core_exit();
gulong ng_core_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void ng_core_signal_disconnect(gulong handler_id);
gboolean ng_core_send_message(const gchar *jid, const gchar *message);
gboolean ng_core_send_subscribe_request(const gchar *jid);
gboolean ng_core_send_subscribed_message(const gchar *jid);
gboolean ng_core_send_unsubscribe_request(const gchar *jid);
gboolean ng_core_send_unsubscribed_message(const gchar *jid);
gboolean ng_core_set_title(const gchar *title);
gboolean ng_core_add_roster(const gchar *jid);
gboolean ng_core_remove_roster(const gchar *jid);
gboolean ng_core_send_unavailable_message();

G_END_DECLS

#endif

