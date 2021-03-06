#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* *************************************************************************** *
 *                       Stub IO routines for our sample
 * *************************************************************************** */

void io_open(void);
void io_close(void);

int pico_putchar(int c);
int pico_puts(const char* s);


/* *************************************************************************** *
 *                   Tiny sample shell using msh routines.
 * *************************************************************************** */

#include "picoshell.h"
#include "picoshell_termesc.h"

#define PUTS_BLUE_BACK(charp) \
{ \
    pico_puts(TERMESC_BACK_BLUE); \
    pico_puts(charp); \
    pico_puts(TERMESC_FONT_NORMAL);\
}


#define PUTS_GREEN_BACK(charp) \
{ \
    pico_puts(TERMESC_BACK_GREEN); \
    pico_puts(charp); \
    pico_puts(TERMESC_FONT_NORMAL);\
}

/*
 * Prototypes for our commands
 */
msh_declare_command( help );
msh_declare_command( rgb );

const msh_command_entry my_commands[] = {
    msh_define_command( help ),
    msh_define_command( rgb ),
    MSH_COMMAND_TERMINATOR
};

/*
 * Define some original commands over built-ins
 *     We have to define cmd_help by yourself...
 */

msh_define_help( help, "display help for available commands",
        "Usage: help [command]\n"
        "    Displays help for 'command', or all commands and their\n"
        "    short descriptions.\n" );
int cmd_help(int argc, const char** argv)
{
    if ( argc == 1) {
        msh_print_cmdlist(my_commands);
        msh_print_cmdlist(msh_builtin_commands);
    }
    else
    {
        const char* usage;
        usage = msh_get_command_usage(my_commands, argv[1]);

        if ( usage == NULL ) {
            usage = msh_get_command_usage(msh_builtin_commands, argv[1]);
        }
        if ( usage == NULL ) {
            pico_puts("No such command: '");
            pico_puts(argv[1]);
            pico_puts("'\n");
        }
        else
        {
            pico_puts(usage);
        }
    }
    return 0;
}


msh_define_help( rgb, "Set RGB LED color / brightness",
        "Usage: rgb 255 255 255  # 0 to 255 pwm\n"
        "       rgb 999          # 0-9 exponential scale\n");
int cmd_rgb(int argc, const char** argv)
{
    if ( argc == 2 ) {
        if (strlen(argv[1]) < 3) {
            pico_puts("Error: need three digits\n");
            return 1;
        }
        int r_pow = argv[1][0] - '0';
        int g_pow = argv[1][1] - '0';
        int b_pow = argv[1][2] - '0';

        int r = (r_pow == 0) ? 0 : 1 << (r_pow - 1);
        int g = (g_pow == 0) ? 0 : 1 << (g_pow - 1);
        int b = (b_pow == 0) ? 0 : 1 << (b_pow - 1);
        
        led_rgb( (r < 255) ? r : 255,
                 (g < 255) ? g : 255,
                 (b < 255) ? b : 255 );
    }
    else if ( argc == 4 ) {
        int r = atoi(argv[1]);
        int g = atoi(argv[2]);
        int b = atoi(argv[3]);
        led_rgb(r, g, b);
    } else {
        pico_puts("Error: need exactly 1, or 3 arguments.");
    }
    return 0;
}



/*
 * main() just loopingly read input line, parse, and execute.
 */

int shell(void)
{
    char  linebuf[MSH_CMDLINE_CHAR_MAX];

    io_open();
    msh_set_prompt("LED> ");


    /*
     * Loop for reading line (forever).
     */
    int   argc;
    char* argv[MSH_CMDARGS_MAX];
    char  argbuf[MSH_CMDLINE_CHAR_MAX];
    while ( 1 ) {
        /*
         * Read a input line
         */
        if (  msh_get_cmdline(linebuf) == 0 ) {
            /* got empty input */
            continue;
        }

        /*
         * Loop for parse line and executing commands.
         */
        const char *linebufp = linebuf;
        while ( 1 ) {
            int i;
            const char* ret_parse;
            int ret_command;

            ret_parse = msh_parse_line(linebufp, argbuf, &argc, argv);

            if ( ret_parse == NULL ) {
                pico_puts("Syntax error\n");
                break; /* discard this line */
            }
            if ( strlen(argv[0]) <= 0 ) {
                break; /* empty input line */
            }
            pico_puts("\n");

            ret_command = msh_do_command(my_commands, argc, (const char**)argv);
            if ( ret_command < 0 ) {
                /* If the command not found amoung my_commands[], search the
                 * buildin */
                ret_command =
                    msh_do_command(msh_builtin_commands, argc, (const char**)argv);
            }
            if ( ret_command < 0 ) {
                pico_puts("command not found: \'");
                pico_puts(argv[0]);
                pico_puts("'\n");
            }

            /*
             * Do we have more sentents remained in linebuf
             * separated by a ';' or a '\n' ?
             */
            if ( ret_parse == linebufp ) {
                /* No, we don't */
                break;
            } else {
                /* Yes, we have. We have to parse rest of lines,
                 * which begins with char* ret_parse; */
                linebufp = ret_parse;
            }
        }
    }

    io_close();
    return 0;
}
