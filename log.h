/*
 * Log Header Declaration
 *
 * log.h
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

#ifndef HAVE_NG_LOG_H
#define HAVE_NG_LOG_H

#include <purple.h>
#include <glib.h>
#include <glib/gprintf.h>

G_BEGIN_DECLS

void ng_log_init();
void ng_log_write_public_message(const gchar *message);
void ng_log_command_log(const gchar *text);
GList *ng_log_get_history();

G_END_DECLS

#endif

