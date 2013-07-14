#include <config.h>
#include <stdlib.h>
#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include "ng-main.h"

/**
 * Modified by Mike Manilone <crtmike@gmx.us>
 */

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    ng_main_run(&argc, &argv);
    ng_main_exit();
    return 0;
}

