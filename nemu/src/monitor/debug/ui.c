#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
  char *arg = strtok(NULL, " ");
  if (arg != NULL)
  {
    cpu_exec(atoi(arg));
  }
  else
  {
    cpu_exec(1);
  }
  return 0;
}

static int cmd_info(char *args) {
    char *arg = strtok(NULL, " ");
    if (!arg) {
        printf("args error: No args\n");
        return 0;
    }

    if (strlen(arg) != 1 || (*arg != 'r' && *arg != 'w')) {
        printf("args error: Invalid argument\n");
        return 0;
    }

    switch (*arg) {
        case 'r': {
            int i;
            printf("32-bit registers:\n");
            for (i = 0; i < 8; i++) {
                printf("  %-3s = 0x%08x\n", regsl[i], reg_l(i));
            }
            printf("Instruction pointer:\n  %-3s = 0x%08x\n", "eip", cpu.eip);
            printf("16-bit registers:\n");
            for (i = 0; i < 8; i++) {
                printf("  %-3s = 0x%04x\n", regsw[i], reg_w(i));
            }
            printf("8-bit registers:\n");
            for (i = 0; i < 8; i++) {
                printf("  %-3s = 0x%02x\n", regsb[i], reg_b(i));
            }
            break;
        }
        case 'w':
            Log("TODO");
            // info_watchpoint();
            break;
        default:
            printf("args error: Unknown arg\n");
            return 0;
    }
    return 0;
}

static int cmd_x(char *args) {
    char *num_str = strtok(NULL, " ");
    char *addr_str = strtok(NULL, " ");

    if (!num_str || !addr_str) {
        printf("Usage: x N EXPR\n"
               "  N: number of consecutive 4-byte words to display\n"
               "  EXPR: hexadecimal starting address (e.g. 0x100000)\n");
        return 0;
    }

    int num_words = atoi(num_str);
    uint32_t addr = strtoul(addr_str, NULL, 16);

    for (int i = 0; i < num_words; i++) {
        uint32_t value = vaddr_read(addr + i * 4, 1);
        printf("0x%08x: 0x%08x\n", addr + i * 4, value);
    }

    return 0;
}

static struct
{
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    /* TODO: Add more commands */
    {"si", "Execute N instructions step by step, default N=1", cmd_si},
    {"info", "Print regs' status with arg r, checkpoint informations with arg w", cmd_info},
	{ "x", "Scan the consecutive 4N bytes from Address expr", cmd_x},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode)
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  while (1)
  {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
