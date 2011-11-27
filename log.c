/*
 * Log Module Declaration
 *
 * log.c
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

#include "log.h"
#include "debug.h"

#define NG_LOG_HISTORY_MAX_LENGTH 1024

static gchar group_name[] = "Log";
static GQueue *ng_log_history_queue = NULL;

void ng_log_init()
{
    ng_log_history_queue = g_queue_new();
}

void ng_log_write_public_message(const gchar *message)
{
    gpointer old_data;
    gchar *text = g_strdup_printf("(%s) %s",
        purple_utf8_strftime("%H:%M:%S", NULL), message);
    if(g_queue_get_length(ng_log_history_queue)>=NG_LOG_HISTORY_MAX_LENGTH)
    {
        old_data = g_queue_pop_tail(ng_log_history_queue);
        g_free(old_data);
    }
    g_queue_push_tail(ng_log_history_queue, text);
    ng_debug_module_pmsg(group_name, "Chat: %s", text);
}

GList *ng_log_get_history()
{
    return g_queue_peek_head_link(ng_log_history_queue);
}

void ng_log_command_log(const gchar *text)
{
    ng_debug_module_pmsg(group_name, "Command: (%s) %s",
        purple_utf8_strftime("%H:%M:%S", NULL), text);
}

