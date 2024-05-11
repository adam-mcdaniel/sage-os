#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <event.h>
#include <stdbool.h>

// State of the shift key
static bool shift;

static void run_command(char a[]);
static char conv_key(int code);
static int get_next_key(struct virtio_input_event events[], int at, int num, char *ret);

void main(void)
{
    char c;
    char command[25];
    unsigned int at = 0;
    struct virtio_input_event events[128];
    int atev = 0;
    int sizeev = 0;

    printf("~> ");
    do
    {
        while (atev >= sizeev)
        {
            sizeev = get_events(events, sizeof(events));
            atev = 0;
            sleep(10);
        }
        c = 0xFF;
        atev = get_next_key(events, atev, sizeev, &c);

        if (c != 0 && c != 255)
        {
            if (c == '\r' || c == '\n')
            {
                command[at] = '\0';
                putchar('\n');
                run_command(command);
                at = 0;
                printf("\n~> ");
            }
            else if (c == '\b') {
                if (at > 0) 
                {
                    putchar('\b');
                    putchar(' ');
                    putchar('\b');
                    at -= 1;
                }
                else 
                {
                    putchar('\a');
                }
            }
            else if (at < (sizeof(command) - 1))
            {
                putchar(c);
                command[at++] = c;
            }
        }
    } while (1);
}

static void run_command(char command[])
{
    if (!strcmp(command, "help"))
    {
        printf("Help is coming!\n");
    }
    else if (command[0] != '\0')
    {
        printf("Unknown command '%s'.\n", command);
    }
}

static char keys[] = {
    [KEY_A] = 'a',
    [KEY_B] = 'b',
    [KEY_C] = 'c',
    [KEY_D] = 'd',
    [KEY_E] = 'e',
    [KEY_F] = 'f',
    [KEY_G] = 'g',
    [KEY_H] = 'h',
    [KEY_I] = 'i',
    [KEY_J] = 'j',
    [KEY_K] = 'k',
    [KEY_L] = 'l',
    [KEY_M] = 'm',
    [KEY_N] = 'n',
    [KEY_O] = 'o',
    [KEY_P] = 'p',
    [KEY_Q] = 'q',
    [KEY_R] = 'r',
    [KEY_S] = 's',
    [KEY_T] = 't',
    [KEY_U] = 'u',
    [KEY_V] = 'v',
    [KEY_W] = 'w',
    [KEY_X] = 'x',
    [KEY_Y] = 'y',
    [KEY_Z] = 'z',
    [KEY_0] = '0',
    [KEY_1] = '1',
    [KEY_2] = '2',
    [KEY_3] = '3',
    [KEY_4] = '4',
    [KEY_5] = '5',
    [KEY_6] = '6',
    [KEY_7] = '7',
    [KEY_8] = '8',
    [KEY_9] = '9',
    [KEY_ENTER] = '\n',
    [KEY_BACKSPACE] = '\b',
    [KEY_SPACE] = ' '
};

static const int num_keys = sizeof(keys) / sizeof(keys[0]);

static char num_shift_keys[] = {
    ')', 
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '('
};

static char conv_key(int code)
{
    char ret = 0;

    if (code < num_keys)
    {
        ret = keys[code];
        if (shift) 
        {
            if (ret >= 'a' && ret <= 'z')
            {
                ret -= 32;
            }
            else if (ret >= '0' && ret <= '9')
            {
                ret = num_shift_keys[ret - '0'];
            }
        }
    }

    return ret;
}

static int get_next_key(struct virtio_input_event events[], int at, int num, char *ret)
{
    int i;
    struct virtio_input_event *ev;
    for (i = at; i < num; i++)
    {
        ev = events + i;
        if (ev->type == EV_KEY)
        {
            if (ev->code == KEY_RIGHTSHIFT || ev->code == KEY_LEFTSHIFT)
            {
                shift = !!ev->value;
            }
            else if (ev->value == 1)
            {
                *ret = conv_key(ev->code);
                break;
            }
        }
    }
    return i + 1;
}
