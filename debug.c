/*
 * Debug Module Declaration
 *
 * debug.c
 * This file is part of <NekoGroup>
 *
 * Copyright (C) 2011 - Supengat, license: GPL v3
 *
 * <NekoGroup> is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * <NekoGroup> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MEngHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <NekoGroup>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <stdlib.h>
#include "debug.h"

/* Set this flag to TRUE to enable debug mode. */
gboolean debug_flag = DEBUG_MODE;


gboolean ng_debug_get_flag()
{
    return debug_flag;
}

void ng_debug_set_mode(gboolean mode)
{
    debug_flag = mode;
}

gint ng_debug_print(const gchar *format, ...)
{
    gint result;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    if(!debug_flag) return 0;
    result = g_vprintf(format, arg_ptr);
    return result;
}

gint ng_debug_perror(const gchar *format, ...)
{
    gint result;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    result = g_vfprintf(stderr, format, arg_ptr);
    return result;
}

gint ng_debug_pmsg(const gchar *format, ...)
{
    gint result;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    result = g_vfprintf(stdout, format, arg_ptr);
    return result;
}

gint ng_debug_module_pmsg(const gchar *module_name, const gchar *format,
    ...)
{
    gint result;
    gchar *new_format;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    new_format = g_strdup_printf("%s: %s\n", module_name, format);
    result = g_vfprintf(stdout, new_format, arg_ptr);
    g_free(new_format);
    return result;
}

gint ng_debug_module_print(const gchar *module_name, const gchar *format,
    ...)
{
    gint result;
    gchar *new_format;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    if(!debug_flag) return 0;
    new_format = g_strdup_printf("%s-DEBUG: %s\n", module_name, format);
    result = g_vfprintf(stdout, new_format, arg_ptr);
    g_free(new_format);
    return result;
}

gint ng_debug_module_perror(const gchar *module_name, const gchar *format,
    ...)
{
    gint result;
    gchar *new_format;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    new_format = g_strdup_printf("%s-ERROR: %s\n", module_name, format);
    result = g_vfprintf(stderr, new_format, arg_ptr);
    g_free(new_format);
    return result;
}

