#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <event.h>
#include <malloc.h>
#include <sage.h>
#include <stdint.h>
#include <ctype.h>
#include <rand.h>

static void run_command(char a[]);

void print_hello() {
	putchar('H');
	putchar('e');
	putchar('l');
	putchar('l');
	putchar('o');
	putchar(' ');
	putchar('w');
	putchar('o');
	putchar('r');
	putchar('l');
	putchar('d');
	putchar('!');
	putchar('\n');
}

void print_addr(uint8_t *addr) {
	char hex[16];
	uint64_t num = (uint64_t)addr;
	for (int i = 0;i < 16;i++) {
		uint8_t nibble = (num >> (60 - (i * 4))) & 0xF;
		if (nibble < 10) {
			hex[i] = '0' + nibble;
		}
		else {
			hex[i] = 'a' + (nibble - 10);
		}
	}
	putchar('0');
	putchar('x');
	for (int i = 0;i < 16;i++) {
		putchar(hex[i]);
	}
	putchar('\n');
}

void print_str(char *str) {
	for (int i = 0;str[i] != '\0';i++) {
		putchar(str[i]);
	}
	putchar('\n');
}

void main(void) {
	// init_malloc(heap, sizeof(heap) / 2);
	uint64_t time = get_time();
	printf("Got time: %d\n", time);
	srand(time);
	printf("Got random: %d\n", rand());
	printf("Got random: %d\n", rand());
	printf("Got random: %d\n", rand());
	printf("Got random: %d\n", rand());
	printf("Got random: %d\n", rand());
	Rectangle dims;
	screen_get_dims(&dims);
	printf("Got screen dims: %d, %d, %d, %d\n", dims.x, dims.y, dims.width, dims.height);
	char c;
	char command[256];
	printf("It's me, Adam's in userspace!\n");

	int at = 0;
	printf("Welcome to the console in user space. Please type 'help' for help.\n");
	printf("~> ");
	do {
        c = getchar();
		if (c == '\r' || c == '\n') {
			command[at] = '\0';
			printf("\n");
			run_command(command);
			printf("~> ");
			at = 0;
		}
		else if (c == '\b' || c == 127) {
			// Backspace or "delete"
			if (at > 0) {
				// Erase character 
				printf("\b \b");
				at--;
			}
		}
		else if (c == 0x1B) {
			// Escape sequence
			char esc1 = getchar();
			char esc2 = getchar();
			if (esc1 == 0x5B) {
				switch (esc2) {
					case 0x41:
						printf("UP\n");
					break;
					case 0x42:
						printf("DOWN\n");
					break;
					case 0x43:
						printf("RIGHT\n");
					break;
					case 0x44:
						printf("LEFT\n");
					break;
				}
			}
		}
		else if (c == 4) {
			// EOF
			printf("\nSee ya!\n");
			break;
		}
		else if (c != 255) {
			if (at < 255) {
				command[at++] = c;
				// Echo it back out to the user
				putchar(c);
			}
		}
		else {
			sleep(10);
		}
	} while (1);
}


static void run_command(char command[]) {
	uint8_t buf[1024];
	if (!strcmp(command, "help")) {
		printf("Help is coming!\n");
	}
	else if (!strcmp(command, "exit")) {
		printf("Exitting!\n");
		exit();
	}
	else if (!strcmp(command, "pwd")) {
		printf("pwd\n");

		get_env("CWD", (char*)buf);

		printf("%.1024s\n", buf);
	}
	else if (!strcmp(command, "getpid")) {
		printf("getpid: %d\n", get_pid());
	}
	else if (!strcmp(command, "nextpid")) {
		static int n = -1;
		if (n == -1 || n >= 0x500) {
			n = get_pid();
		}
		printf("nextpid: %d\n", n = next_pid(n));
	}
	else if (!strcmp(command, "draw")) {
		// Draw a square
		Pixel test[0x400] = {0};
		Rectangle rect;
		for (int i=0;i<400; i++) {
			rect.x = rand() % 0x100 + 0x100;
			rect.y = rand() % 0x100 + 0x100;
			rect.width = 0x20;
			rect.height = 0x20;

			test[0].r = rand();
			test[0].g = rand();
			test[0].b = rand();
			test[0].a = 0xFF;
			for (uint32_t i = 1; i < sizeof(test) / sizeof(test[0]); i++) {
				test[i] = test[0];
			}

			screen_draw_rect(test, &rect, 1, 1);
			screen_flush(&rect);
		}
	}
	else if (!strncmp(command, "ls", 2)) {
		// Get the path from the command
		char path[256] = {0};
		int i, j;
		for (i = 2; command[i] != '\0' && i < 256; i++) {
			if (isspace(command[i])) {
				continue;
			} else {
				break;
			}
		}

		for (j=0; command[i] != '\0' && i < 256; i++, j++) {
			path[j] = command[i];
		}
		path[j] = '\0';

		printf("ls: %s\n", path);
		char buf[1024] = {0};
		path_list_dir(path, buf, sizeof(buf), true);
		printf("Result:\n%.1024s\n", buf);
	}
	else if (!strncmp(command, "isdir", 5)) {
		char path[256] = {0};
		int i, j;
		for (i = 5; command[i] != '\0' && i < 256; i++) {
			if (isspace(command[i])) {
				continue;
			} else {
				break;
			}
		}

		for (j=0; command[i] != '\0' && i < 256; i++, j++) {
			path[j] = command[i];
		}
		path[j] = '\0';

		printf("isdir: %s\n", path);
		printf("Result: %d\n", path_is_dir(path));
	}
	else if (!strncmp(command, "isfile", 5)) {
		char path[256];
		int i, j;
		for (i = 5; command[i] != '\0' && i < 256; i++) {
			if (isspace(command[i])) {
				continue;
			} else {
				break;
			}
		}

		for (j=0; command[i] != '\0' && i < 256; i++, j++) {
			path[j] = command[i];
		}
		path[j] = '\0';

		printf("isfile: %s\n", path);
		printf("Result: %d\n", path_is_file(path));
	}
	else if (!strncmp(command, "exists", 6)) {
		char path[256];
		int i, j;
		for (i = 6; command[i] != '\0' && i < 256; i++) {
			if (isspace(command[i])) {
				continue;
			} else {
				break;
			}
		}

		for (j=0; command[i] != '\0' && i < 256; i++, j++) {
			path[j] = command[i];
		}
		path[j] = '\0';

		printf("isfile: %s\n", path);
		printf("Result: %d\n", path_exists(path));
	}
	else if (!strncmp(command, "spawn", 5)) {
		char path[256];
		int i, j;
		for (i = 5; command[i] != '\0' && i < 256; i++) {
			if (isspace(command[i])) {
				continue;
			} else {
				break;
			}
		}

		for (j=0; command[i] != '\0' && i < 256; i++, j++) {
			path[j] = command[i];
		}
		path[j] = '\0';

		// printf("isfile: %s\n", path);
		// printf("Result: %d\n", path_exists(path));
		int pid = spawn_process(path);
		if (pid < 0) {
			printf("Failed to spawn process.\n");
		}
		else {
			printf("Spawned process %d.\n", pid);
		}
	}
	else if (!strncmp(command, "pidgetenv", 9)) {
		// Get the PID from the command
		int pid, i;
		pid = 0;
		printf("pid=%d\n", pid);
		printf("pidgetenv: %s\n", command);
		for (i = 9; command[i] != '\0' && i < 256; i++) {
			printf("command[%d] = %c\n", i, command[i]);
			if (isdigit(command[i])) {
				pid *= 10;
				pid += command[i] - '0';
			} else if (isspace(command[i])) {
				continue;
			} else {
				printf("Invalid PID\n");
				return;
			}
		}

		printf("pidgetenv: %d\n", pid_get_env(pid, "CWD", (char*)buf));
		printf("%.1024s\n", buf);
	}
	else if (!strncmp(command, "pidputenv", 9)) {
		// Get the PID from the command
		int pid, i;
		pid = 0;
		printf("pid=%d\n", pid);
		printf("pidputenv: %s\n", command);
		for (i=9; isspace(command[i]); i++);

		for (; isdigit(command[i]) && i < 256; i++) {
			printf("command[%d] = %c\n", i, command[i]);
			pid *= 10;
			pid += command[i] - '0';
		}

		if (command[i] != ' ') {
			printf("Invalid PID\n");
			return;
		}

		// Get the symbol after the PID
		for (; isspace(command[i]); i++);
		char *name = &command[i];

		printf("pidputenv: %d\n", pid_put_env(pid, "CWD", name));
		printf("%.1024s\n", buf);
	}
	else if (!strcmp(command, "cd")) {
		printf("cd\n");
		put_env("CWD", "~");
	}
	else if (!strcmp(command, "gev")) {
		struct virtio_input_event events[64];
		int num = get_events(events, 64);
		for (int i = 0;i < num;i++) {
			printf("INPUT: [0x%02x:0x%02x:0x%08x]\n", events[i].type, events[i].code, events[i].value);
		}
		printf("Processed %d events.\n", num);
	}
	else if (command[0] != '\0') {
		printf("Unknown command '%s'.\n", command);
	}
}

