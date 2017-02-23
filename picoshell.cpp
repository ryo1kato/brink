#include <string.h>
#include <stdio.h>

#include "picoshell.h"
#include "picoshell_config.h"
#include "picoshell_termesc.h"


/* ***************************************************************************
 *                cmdedit help strings (displayed by 'shellhelp')
 * ***************************************************************************/
#ifdef MSH_CONFIG_HELP_KEYBIND
#ifdef MSH_CONFIG_LINEEDIT
#    define MSH_HELP_LINEEDIT \
        " * Minimal Emacs-like line editting. (Ctrl+F,B,E,A)\n" \
        "    Ctrl-F  Cursor right     ('F'orward)\n"  \
        "    Ctrl-B  Cursor left      ('B'ackward)\n" \
        "    Ctrl-A  Cursor line head ('A'head)\n"  \
        "    Ctrl-E  Cursor line tail ('E'nd)\n"
#else
#   define MSH_HELP_LINEEDIT ""
#endif

#ifdef MSH_CONFIG_CMDHISTORY
#    define MSH_HELP_CMDHISTORY \
        " * Command-line history. (Ctrl+P,N)\n" \
        "    Ctrl-P  Previous history ('P'revious)\n" \
        "    Ctrl-N  Next history     ('N'ext)\n"
#else
#   define MSH_HELP_CMDHISTORY ""
#   define MSH_HELP_CMDHISTORY_KEYS ""
#endif

#ifdef MSH_CONFIG_CLIPBOARD
#    define MSH_HELP_CLIPBOARD  \
        " * Cut & paste. (Ctrl+K,W,Y)\n" \
        "    Ctrl-K  Cut strings after the cursor to clipboard  ('K'ill)\n" \
        "    Ctrl-W  Cut a word before the cursor to clipboard  ('W'ord)\n" \
        "    Ctrl-Y  Paste clipboard content to cursor position ('Y'ank)\n"
#else
#    define MSH_HELP_CLIPBOARD  ""
#endif

#define MSH_CMDEDIT_HELP_DESCRIPTION \
    "* Basic keybinds\n" \
    "    Ctrl-H  Backspace\n" \
    "    Ctrl-D  Delete\n" \
    "    Ctrl-L  Clear screen\n" \
    "    Ctrl-C  Discard line\n" \
    "    Ctrl-U  Kill whole line\n" \
    MSH_HELP_LINEEDIT \
    MSH_HELP_CMDHISTORY \
    MSH_HELP_CLIPBOARD
#endif


/* ***************************************************************************
 *                         the builtin commands
 * ***************************************************************************/
#ifdef MSH_CONFIG_HELP_KEYBIND
static int cmd_shellhelp(int argc, const char** argv)
{
    pico_puts(MSH_CMDEDIT_HELP_DESCRIPTION);
    return 0;
}
#endif


static int cmd_echo(int argc, const char** argv)
{
    int i;
    if ( argc < 1 ) {
        return 0;
    }
    for ( i = 1;  i < argc;  i++ ) {
        pico_puts(argv[i]);
        pico_putchar(' ');
    }
    pico_putchar('\n');
    return 0;
}


/* ***************************************************************************
 *                          command registration
 * ***************************************************************************/

const msh_command_entry msh_builtin_commands[] = {
#ifdef MSH_CONFIG_HELP_KEYBIND
    { "shellhelp", cmd_shellhelp,
        "display help for keybinds of commandline editting",
        "No further help available.\n",
    },
#endif

    { "echo", cmd_echo,
#ifdef MSH_CONFIG_HELP
        "echo all arguments separated by a whitespace",
        "Usage: echo [string ...]\n"
#endif
    },

    MSH_COMMAND_TERMINATOR
};


/*
 * Search built-in command entry.
 */
static const msh_command_entry *
find_command_entry(const msh_command_entry* cmdlist, const char* name)
{
    int i = 0;
    while ( cmdlist[i].name != NULL ) {
        if ( strcmp(cmdlist[i].name, name) == 0 ) {
            return &cmdlist[i];
        } else {
            i++;
        }
    }
    return NULL;
}


/*
 * Find a command 'argv[0]' from cmdlist (using find_command_entry())
 * and executes it.
 */
int msh_do_command(const msh_command_entry* cmdlist, int argc, const char** argv)
{
    const msh_command_entry* cmd_entry;

    if ( argc < 1 ) {
        return -1;
    }

    cmd_entry = find_command_entry(cmdlist, argv[0]);

    if ( cmd_entry != NULL ) {
        return ( cmd_entry->func(argc, argv) );
    } else {
        /*
        pico_puts("command not found: ");
        pico_puts(argv[0]);
        pico_puts("\n");
        */
        return -1;
    }
}

#ifdef MSH_CONFIG_HELP
void msh_print_cmdlist(const msh_command_entry* cmdlist)
{
    int i, j;
    const int indent = 10;

    i = 0;
    while ( cmdlist[i].name != NULL ) {
            pico_puts("    ");
            pico_puts(cmdlist[i].name);
            for (j = indent - strlen(cmdlist[i].name);  j > 0;  j--) {
                pico_putchar(' ');
            }
            pico_puts("- ");
            if ( cmdlist[i].description != NULL ) {
                pico_puts(cmdlist[i].description);
                pico_puts("\n");
            } else {
                pico_puts("(No description available)\n");
            }
        i++;
    }
    return;
}


const char* msh_get_command_usage(const msh_command_entry* cmdlist, const char* cmdname)
{
    const msh_command_entry* cmd_entry;

    cmd_entry = find_command_entry(cmdlist, cmdname);
    if ( cmd_entry == NULL ) {
        return NULL; /* No such command */
    } else if ( cmd_entry->usage == NULL ) {
        return "No help available.\n";
    } else {
        return cmd_entry->usage;
    }
}
#else
void msh_print_cmdlist(const msh_command_entry* cmdlist) { /* do nothing */ }
const char* msh_get_command_usage(const msh_command_entry* cmdlist, const char* cmdname)
{
    return "No help available.\n";
}
#endif /*MSH_CONFIG_HELP*/
#include "history.h"
#ifdef MSH_CONFIG_CMDHISTORY
#include <string.h>
#include <stdbool.h>

/*
 * Easy, we have only one history, the global history.
 */
static char history[MSH_CMD_HISTORY_MAX][MSH_CMDLINE_CHAR_MAX];
static bool histfull;
static int  histlast;


void history_append(const char* line)
{
    /* If a given string is too long or zero-length, just ignore.*/
    int len = strlen(line);
    if ( len >= MSH_CMDLINE_CHAR_MAX || len <= 0) {
        return;
    }

    strcpy(history[histlast], line);

    if ( histlast >= MSH_CMD_HISTORY_MAX - 1 ) {
        histfull = true;
        histlast = 0;
    } else {
        histlast++;
    }
}


const char* history_get(int histnum)
{
    if ( ! histfull ) {
        if ( histnum >= histlast ) {
            return NULL;
        }
    }
    else
    if ( histnum > MSH_CMD_HISTORY_MAX - 1 || histnum < 0 ) {
        return NULL;
    }

    if ( histlast > histnum ) {
        return ( history[histlast - histnum - 1] );
    } else {
        return history[ MSH_CMD_HISTORY_MAX - (histnum - histlast)  - 1];
    }
}



#ifdef TEST
#include "test.h"
int main(void)
{
    return 0;
}
#endif
#endif/*MSH_CONFIG_CMDHISTORY*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "picoshell_termesc.h"
#include "history.h"


#ifdef MSH_CONFIG_ENABLE_BELL
#define ring_terminal_bell() pico_putchar('\a');
#else
#define ring_terminal_bell() /* disable */
#endif



/*
 * cmdline_t class and its methods (in subtle Object Oriented flavor).
 */
typedef struct cmdline_struct {
    char buf[MSH_CMDLINE_CHAR_MAX];
    int  pos;     /* cursor position (start at 1 orignin, 0 means empty line) */
    int  linelen; /* length of input char of line EXCLUDING trailing null */
#   ifdef MSH_CONFIG_CLIPBOARD
    /* The buffer used for Cut&Paste */
    char clipboard[MSH_CMDLINE_CHAR_MAX];
#   endif
} cmdline_t;

static cmdline_t CmdLine;
static int  bCmdLineInitialized;

/** cmdline_clear()
 * Empty the cmdline buffer, but not displayed line.
 */
static void
cmdline_clear( cmdline_t* pcmdline )
{
    memset(pcmdline->buf, '\0', MSH_CMDLINE_CHAR_MAX);
    pcmdline->pos     = 0;
    pcmdline->linelen = 0;
}

/** cmdline_init()
 * Initialize cmdline_t object. Call this before fast use.
 */
static void
cmdline_init( cmdline_t* pcmdline )
{
    cmdline_clear( pcmdline );
#   ifdef MSH_CONFIG_CLIPBOARD
    memset(pcmdline->clipboard, '\0', MSH_CMDLINE_CHAR_MAX);
#   endif
}


static char* prompt_string = MSH_CMD_PROMPT;
void msh_set_prompt(char* str)
{
    prompt_string = str;
}

/* ************************************************************************* *
 *     Basic Line Edit Functions (Enabled regardless of MSH_CONFIG_LINEEDIT)
 * ************************************************************************* */

/** cmdline_kill()
 * Ctrl-U of standard terminal
 */
static void
cmdline_kill( cmdline_t* pcmdline )
{
    int i;
    for ( i = 0;  i < pcmdline->pos;  i++ ) {
        pico_putchar('\b');
    }
    for ( i = 0;  i < pcmdline->linelen;  i++ ) {
        pico_putchar(' ');
    }
    for ( i = 0;  i < pcmdline->linelen;  i++ ) {
        pico_putchar('\b');
    }
    cmdline_clear( pcmdline );
}


/** cmdline_set()
 * Discard current cmdline and set it to specified string
 */
#ifdef MSH_CONFIG_CMDHISTORY
static void
cmdline_set( cmdline_t* pcmdline, const char* str )
{
    int len;

    cmdline_kill(pcmdline);
    len = strlen(str);
    strcpy( pcmdline->buf, str );
    pico_puts( str );
    pcmdline->pos     = len;
    pcmdline->linelen = len;
}
#endif

/** cmdline_insert_char()
 * Insert (or append) a charactor 'c' at current cursor position.
 */
static int
cmdline_insert_char( cmdline_t* pcmdline, unsigned char c )
{
    /* Check if the line buffer can hold another one char */
    if ( pcmdline->linelen >= MSH_CMDLINE_CHAR_MAX - 1 ) {
        /* buffer is full */
        ring_terminal_bell();
        return 0;
    }

    pico_putchar(c);
    /* Is cursor at the end of the cmdline ? */
    if ( pcmdline->pos == pcmdline->linelen ) {
        /* just append */
        pcmdline->buf[ pcmdline->pos ] = c;
    } else {
        /* slide the strings after the cursor to the right */
        int i;
        pico_puts(  & pcmdline->buf[ pcmdline->pos ]  );
        for (i = pcmdline->linelen;  i > pcmdline->pos;  i--) {
            pcmdline->buf[ i ] = pcmdline->buf[ i - 1 ];
            pico_putchar('\b');
        }
        pcmdline->buf[ pcmdline->pos ] = c;
    }
    pcmdline->pos++;
    pcmdline->linelen++;
    pcmdline->buf[ pcmdline->linelen ] = '\0'; /* just for safty */
    return 1;
}


/** cmdline_backspace()
 * Delete a charactor at left of the cursor and
 * slide rest of strings to the left.
 */
static int
cmdline_backspace( cmdline_t* pcmdline )
{
    if ( pcmdline->pos <= 0 ) {
        ring_terminal_bell();
        return 0;
    }
    pico_putchar('\b');
    /* Is cursor at the end of the cmdline ? */
    if ( pcmdline->pos == pcmdline->linelen ) {
        pico_putchar(' ');
        pico_putchar('\b');
    } else {
        int i;
        /* slide the characters after cursor position to the left */
        for ( i = pcmdline->pos;  i < pcmdline->linelen;  i++ ) {
            pcmdline->buf[i-1] = pcmdline->buf[i];
            pico_putchar( pcmdline->buf[i] );
        }
        pico_putchar(' ');
        /* put the cursor to its orignlal position */
        /* +1 in for () is for pico_putchar(' ') in the previous line */
        for ( i = pcmdline->pos;  i < pcmdline->linelen + 1;  i++ ) {
            pico_putchar('\b');
        }
    }
    pcmdline->buf[ pcmdline->linelen - 1 ] = '\0';
    pcmdline->pos--;
    pcmdline->linelen--;
    return 1;
}


/** cmdline_delete()
 * Delete a charactor on the cursor and slide rest of strings to the left.
 * Cursor position doesn't change, unlike cmdline_backspace().
 */
static int
cmdline_delete( cmdline_t* pcmdline )
{
    if ( pcmdline->linelen <= pcmdline->pos ) {
        /* No more charactors to delete.
         * i.e, cursor is the rightmost pos of the line.  */
        ring_terminal_bell();
        return 0;
    }
    else
    {
        int i;
        /* slide the chars on and after cursor position to the left */
        for ( i = pcmdline->pos;  i < pcmdline->linelen - 1;  i++ ) {
            pcmdline->buf[i] = pcmdline->buf[ i + 1 ];
            pico_putchar( pcmdline->buf[i] );
        }
        pico_putchar(' ');
        /* put the cursor to its orignlal position */
        for ( i = pcmdline->pos;  i < pcmdline->linelen;  i++ ) {
            pico_putchar('\b');
        }

    }
    pcmdline->buf[ pcmdline->linelen - 1 ] = '\0';
    pcmdline->linelen--;
    return 1;
}



/* ************************************************************************* *
 *     Line Edit Functions
 * ************************************************************************* */
#ifdef MSH_CONFIG_LINEEDIT
/** cmdline_cursor_left()
 ** cmdline_cursor_right()
 * move a cursor on the comdline left and right;
 */
static int
cmdline_cursor_left( cmdline_t* pcmdline )
{
    if ( pcmdline->pos > 0 ) {
        pico_putchar('\b');
        pcmdline->pos--;
        return 1;
    }
    else
    {
        ring_terminal_bell();
        return 0;
    }
}

static int
cmdline_cursor_right( cmdline_t* pcmdline )
{
    if ( pcmdline->pos < pcmdline->linelen ) {
        pico_putchar( pcmdline->buf[pcmdline->pos++] );
        return 1;
    }
    else
    {
        ring_terminal_bell();
        return 0;
    }
}

/** cmdline_cursor_linehead()
 ** cmdline_cursor_linetail()
 * Move the cursor to head/tail of the line.
 */
static void
cmdline_cursor_linehead( cmdline_t* pcmdline )
{
    while ( pcmdline->pos > 0 ) {
        pico_putchar('\b');
        pcmdline->pos--;
    }
}

static void
cmdline_cursor_linetail( cmdline_t* pcmdline )
{
    while ( pcmdline->pos < pcmdline->linelen ) {
        pico_putchar( pcmdline->buf[pcmdline->pos++] );
    }
}

#ifdef MSH_CONFIG_CLIPBOARD
/** cmdline_yank()
 * Insert clipboard string into current cursor pos
 */
static void
cmdline_yank( cmdline_t* pcmdline )
{
    if ( strlen(pcmdline->clipboard) == 0 ) {
        /* no string in the clipboard */
        ring_terminal_bell();
    } else {
        int i = 0;
        while ( pcmdline->clipboard[ i ] != '\0'
                && cmdline_insert_char( pcmdline, pcmdline->clipboard[i] ) )
        {
                i++;
        }
    }
}

/** cmdline_killtail()
 * kill characters on and right of the cursor and
 * copy them into the clipboard buffer.
 */
static void
cmdline_killtail( cmdline_t* pcmdline )
{
    int i;
    if ( pcmdline->pos == pcmdline->linelen ) {
        /* nothing to kill */
        ring_terminal_bell();
    }

    /* copy chars on and right of the cursor to the clipboar */
    strcpy( pcmdline->clipboard, &pcmdline->buf[pcmdline->pos] );

    /* erase chars on and right of the cursor on terminal */
    for ( i = pcmdline->pos;  i < pcmdline->linelen; i++ ) {
        pico_putchar(' ');
    }
    for ( i = pcmdline->pos;  i < pcmdline->linelen; i++ ) {
        pico_putchar('\b');
    }

    /* erase chars on and right of the cursor in buf */
    pcmdline->buf[pcmdline->pos] = '\0';
    pcmdline->linelen = pcmdline->pos;
}
/** cmdline_killword()
 * kill a (part of) word on and left of the cursor and
 * copy them into the clipboard buffer.
 */
static void
cmdline_killword( cmdline_t* pcmdline )
{
    int i, j;
    if ( pcmdline->pos == 0 ) {
        ring_terminal_bell();
        return ;
    }
    /* search backward for a word to kill */
    i = 0;
    while( i < pcmdline->pos
           && pcmdline->buf[ pcmdline->pos - i - 1 ] == ' ' ) {
        i++;
    }
    while( i < pcmdline->pos
           && pcmdline->buf[ pcmdline->pos - i - 1 ] != ' ' ) {
        i++;
    }

    /* copy the word to clipboard */
    j = 0;
    while ( j < i ) {
        pcmdline->clipboard[ j ] = pcmdline->buf[ pcmdline->pos - i + j ];
        j++;
    }
    pcmdline->clipboard[ j ] = '\0';

    /* kill the word */
    j = 0;
    while ( j < i ) {
        cmdline_backspace( pcmdline );
        j++;
    }
}

#endif /*MSH_CONFIG_CLIPBOARD*/
#endif/*MSH_CONFIG_LINEEDIT*/




/*
 * Input a char at the current cursor position.
 * Or move cursor, retrieve command history etc, if Ctrl-X
 *
 * Returns false (=0) when input terminated (by Enter)
 */
#ifdef MSH_CONFIG_CMDHISTORY
    /*
     * Current active history number
     *   Notice: unlinke 'histnum' in history.c, zero value for this histnum means
     *   CURRENT LINE, not the first history. (offset -1)
     */
    static int histnum;
    /* temporary buffer to hold current line */
    char curline[MSH_CMDLINE_CHAR_MAX];
    /* temporary buffer to hold history */
    const char* histline;
#endif
static int
cursor_inputchar( cmdline_t* pcmdline, unsigned char c )
{
    unsigned char input = c;

    /* QUICK HACK
     * Map escape sequences (Arrow keys) to other bind - work only for limited types of terminals
     */
    if (input == '\033' ) {
        char second, third;
        second = pico_getchar();
        third = pico_getchar();
        if ( second == '[' ) {
            switch (third) {
#ifdef MSH_CONFIG_HISTORY
            case 'A':
                input = MSH_KEYBIND_HISTPREV;
                break;
            case 'B':
                input = MSH_KEYBIND_HISTNEXT;
                break;
#endif
#ifdef MSH_CONFIG_LINEEDIT
            case 'C':
                input = MSH_KEYBIND_CURRIGHT;
                break;
            case 'D':
                input = MSH_KEYBIND_CURLEFT;
                break;
#endif
            default:
                ;
                /* do nothing */
            }
        } else {
            /* do nothing */
        }
    }


    switch (input) {
        /*
         * End of input if newline char.
         */
        case MSH_KEYBIND_ENTER:
            pico_putchar('\n');
            return 0;

        case '\t':
            /* tab sould be comverted to a space */
            cmdline_insert_char( pcmdline, ' ');
            break;

        case MSH_KEYBIND_DISCARD:
            cmdline_clear(pcmdline);
            pico_putchar('\n');
            return 0;

        case MSH_KEYBIND_BACKSPACE:
            cmdline_backspace(pcmdline);
            break;

        case MSH_KEYBIND_DELETE:
        case 0x7F: /* ASCII DEL.  Should be used as BS ?*/
            cmdline_delete(pcmdline);
            break;

        case MSH_KEYBIND_KILLLINE:
            cmdline_kill(pcmdline);
            break;

#ifdef MSH_CONFIG_LINEEDIT
        case MSH_KEYBIND_CLEAR:
            cmdline_cursor_linehead(pcmdline);
            pico_puts(TERMESC_CLEAR);
            pico_puts(prompt_string);
            cmdline_cursor_linetail(pcmdline);
            break;

        case MSH_KEYBIND_CURLEFT:
            cmdline_cursor_left(pcmdline);
            break;

        case MSH_KEYBIND_CURRIGHT:
            cmdline_cursor_right(pcmdline);
            break;

        case MSH_KEYBIND_LINEHEAD:
            cmdline_cursor_linehead(pcmdline);
            break;

        case MSH_KEYBIND_LINETAIL:
            cmdline_cursor_linetail(pcmdline);
            break;

#ifdef MSH_CONFIG_CLIPBOARD
        case MSH_KEYBIND_YANK:
            cmdline_yank(pcmdline);
            break;

        case MSH_KEYBIND_KILLTAIL:
            cmdline_killtail(pcmdline);
            break;

        case MSH_KEYBIND_KILLWORD:
            cmdline_killword(pcmdline);
            break;
#endif /*MSH_CONFIG_CLIPBOARD*/
#endif /*MSH_CONFIG_LINEEDIT*/

#ifdef MSH_CONFIG_CMDHISTORY
        case MSH_KEYBIND_HISTPREV:
            if ( histnum == 0 ) {
                /* save current line before overwrite with history */
                strcpy(curline, pcmdline->buf);
            }
            histline = history_get(histnum);
            if ( histline != NULL ) {
                cmdline_set(pcmdline, histline);
                histnum++;
            } else {
                ring_terminal_bell();
            }
            break;

        case MSH_KEYBIND_HISTNEXT:
            if ( histnum == 1 ) {
                histnum = 0;
                cmdline_set(pcmdline, curline);
            }
            else
            if ( histnum > 1 )  {
                histline = history_get(histnum-2);
                if ( histline != NULL ) {
                    cmdline_set(pcmdline, histline);
                    histnum--;
                } else {
                    ring_terminal_bell(); /* no newer hist */
                }
            } else {
                ring_terminal_bell(); /* invalid (negative) histnum value */
            }

            break;
#endif /*MSH_CONFIG_CMDHISTORY*/

        default:
            if ( isprint(c) ) {
                if ( pcmdline->pos  <  MSH_CMDLINE_CHAR_MAX - 1 ) {
                    cmdline_insert_char( pcmdline, c );
                }
            }
            break;
    }

    return 1 /*true*/;
}


int msh_get_cmdline(char* linebuf)
{
    if ( ! bCmdLineInitialized ) {
        cmdline_init( &CmdLine );
        bCmdLineInitialized = 1; /* true */
    } else {
        cmdline_clear( &CmdLine );
    }
    pico_puts(prompt_string);

    while ( cursor_inputchar( &CmdLine, pico_getchar() ) )
        ;

#ifdef MSH_CONFIG_CMDHISTORY
    history_append(CmdLine.buf);
    histnum = 0; /* reset active histnum */
#endif

    strcpy(linebuf, CmdLine.buf);
    return ( strlen( CmdLine.buf ) );
}



#ifdef TEST
#include "test.h"
int main(void)
{
    /*
     * Oh! Please write the test code!!
     */
    return 0;
}
#endif
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "picoshell_config.h"

typedef struct parse_state_struct {
    const char* readpos;
    char*       writepos;
} parse_state_t;


/*
 * E<0 : Syntax error type E (FIXME: no error types defined)
 *   0 : No characters to read (';' or '\0')
 * N>1 : read N chars
 *
 * If error is returned, state of parse_state_t may not be consistent
 * and should never re-used. (whole input line should be discarded.)
 */
static int
read_token( parse_state_t*  pstate )
{
    int   readcount  = 0; /* FIXME counting is not correct */
    bool  is_squoted = false;
    bool  is_dquoted = false;

    while ( 1 ) {
        char ch = *pstate->readpos;

        if ( ch == '\0' ) {
            break; /* end of input */
        }
        readcount++;

        /*
         * We are in quote
         */
        if ( is_squoted || is_dquoted ) {
            if (   (is_squoted  &&  ch == '\'')
                || (is_dquoted  &&  ch == '"' )  )  {
                /*
                 * FIXME : For now, we treat dquote as squote.
                 */
                /* Closing quote */
                is_squoted = false;
                is_dquoted = false;
            } else {
                /* Read the char as it is. */
                *pstate->writepos++ = ch;
            }
        }
        /*
         * We are NOT in quote
         */
        else
        {
            /* quote */
            if ( ch == '\'' ) {
                is_squoted = true;
            }
            else
            if ( ch == '"' ) {
                is_dquoted = true;
            }

            /* Backslash (Escaping): read a char ahead */
            else
            if ( ch == MSH_CMD_ESCAPE_CHAR ) {
                /* read a char ahead */
                pstate->readpos++;
                ch = *pstate->readpos;
                if ( ! isprint((unsigned)ch) ) {
                    /* You can only escape normal chars */
                    return -1;
                } else {
                    readcount++;
                    *pstate->writepos++ = ch;
                }
            }

            /* A blank not in quote or not escaped, or a
             * command line separator (';') makes this argument done. */
            else
            if ( isspace((unsigned)ch)
                 || ch == MSH_CMD_SEP_CHAR )  {
                readcount--;
                break; /* end of current argument */
            }

            /* Normal chars */
            else
            if ( isprint((unsigned)ch) ) {
                *pstate->writepos++ = ch;
            }

            /* NON Printable (control code) char  */
            else
            {
                /*
                 * If you want to just Ignore invalid characters,
                 * comment-out the blow.
                 */
                return -1; /* Syntax error */
            }
        }
        pstate->readpos++;
    }

    /*
     * Finally, check if state is consistent,
     * I.e., check for no-closing quote etc.
     */
    if ( is_squoted || is_dquoted ) {
        return -1; /* Syntax error */
    } else {
        *pstate->writepos++ = '\0';
        return readcount;
    }
}

const char*
msh_parse_line(const char* cmdline, char* argvbuf, int* pargc, char** argv)
{
    /*
     * Prepare and initialize a parse_state_t.
     */
    parse_state_t state;
    state.readpos  = cmdline;
    state.writepos = argvbuf;

    *pargc = 0;
    argv[0] = argvbuf;

    while ( *state.readpos != '\0' ) {

        /*
         * Skip preceeding spaces or ';'
         */
        while ( isspace((unsigned)*state.readpos) ) {
            state.readpos++;
        }

        int ret = read_token( &state );

        /*
         * Syntax error
         */
        if ( ret < 0 ) {
            return NULL;
        }
        else
        /*
         * No more chars to read. Case I
         */
        if ( ret == 0 ) {
            switch ( *(state.readpos) ) {
                case '\0':
                    return cmdline;
                case MSH_CMD_SEP_CHAR:
                    return (state.readpos+1);
                default:
                    pico_puts("Fatal error in parse() \n");
                    return NULL;
            }
        }

        /*
         * Normal
         */
        else
        {
            (*pargc)++;
            char stopchar = *(state.readpos);
            /*
             * No more chars to read. Case II
             */
            if ( stopchar == '\0' ) {
                return cmdline;
            }

            /*
             * End of command by ';'.
             * Tell the caller where to restart.
             */
            if ( stopchar == MSH_CMD_SEP_CHAR ) {
                state.readpos++;/* skip ';' */
                return (state.readpos);
            }

            /*
             * read_token() stoped by a argument separator.
             */
            else
            if ( isspace((unsigned)stopchar) ) {
                argv[*pargc] = state.writepos;
                continue;
            }

            /*
             * Can't be!!
             */
            else {
                pico_puts("Fatal error in parse() ");
                return NULL;
            }
        }
    }

    /* Tell the caller that whole line has processed */
    return cmdline;
}





#ifdef TEST
#include "test.h"
int main(void)
{
    return 0;
}
#endif
