/*
 * NekoGroup Bot Kernel Header Declaration
 *
 * ng-bot.h
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

#ifndef HAVE_NG_BOT_H
#define HAVE_NG_BOT_H

#include <glib.h>
#include <ctype.h>

G_BEGIN_DECLS

typedef struct NGBotMemberData NGBotMemberData;

struct NGBotMemberData {
    gchar *jid;
    gchar *subscription;
    gchar *resource;
    gchar *status;
    gboolean online;
};

void ng_bot_init();
void ng_bot_exit();
gchar *ng_bot_generate_new_nick(const gchar *jid);
gchar *ng_bot_get_nick_by_jid(const gchar *jid, const gchar *nick);
void ng_bot_broadcast(const gchar *from, const gchar *message);
GHashTable *ng_bot_get_member_table();
const NGBotMemberData *ng_bot_get_member_data(const gchar *jid);
void ng_bot_member_data_remove(const gchar *jid);

G_END_DECLS

#endif

