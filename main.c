#include <stdlib.h>
#include <glib.h>
#include "ng-main.h"

int main(int argc, char *argv[])
{
    ng_main_run(&argc, &argv);
    ng_main_exit();
    return 0;
}

