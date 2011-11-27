/*
 * Core Header Declaration
 *
 * core.h
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

#ifndef HAVE_NG_CORE_H
#define HAVE_NG_CORE_H

#include <purple.h>
#include <glib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <glib/gprintf.h>

G_BEGIN_DECLS

typedef struct NGCoreAccountData {
    PurpleAccount *account;
    gchar *root;
    gchar *title;
}NGCoreAccountData;

gboolean ng_core_init(gint *argc, gchar **argv[]);
gboolean ng_core_account_init();
const gchar *ng_core_get_ui_id();
PurpleConversation *ng_core_get_conversation(PurpleAccount *account,
    const PurpleBuddy *buddy);
gchar *ng_core_get_buddy_chat_name(PurpleBuddy *buddy, const gchar *sender);
PurpleBuddy *ng_core_get_buddy_by_nick(PurpleAccount *account,
    const gchar *nick);
void ng_core_do_broadcast(PurpleAccount *account, PurpleConversation *conv,
    const gchar *message);
void ng_core_talk_to_buddy(PurpleAccount *account, PurpleBuddy *buddy,
    const gchar *message);
NGCoreAccountData *ng_core_get_account_data(PurpleAccount *account);
gint ng_core_get_conversation_privilege(PurpleAccount *account,
    const gchar *name);

G_END_DECLS

#endif

