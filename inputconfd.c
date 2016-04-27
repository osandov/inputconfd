/*
 * Copyright (C) 2016 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <getopt.h>
#include <libudev.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *progname = "inputconfd";

static bool has_nonzero_property(struct udev_device *device,
				 const char *property)
{
	const char *value;

	value = udev_device_get_property_value(device, property);
	if (value == NULL)
		return false;

	return atoi(value) != 0;
}

static bool is_keyboard(struct udev_device *device)
{
	return has_nonzero_property(device, "ID_INPUT_KEYBOARD");
}

static bool is_mouse(struct udev_device *device)
{
	return has_nonzero_property(device, "ID_INPUT_MOUSE");
}

static void run_cmd(const char *cmd, const char *label)
{
	int ret;

	ret = system(cmd);
	if (ret == -1) {
		fprintf(stderr, "could not run %s command\n",
			label);
	} else if (WIFEXITED(ret)) {
		ret = WEXITSTATUS(ret);
		if (ret != 0) {
			fprintf(stderr, "%s command exited with status %d\n",
				label, ret);
		}
	} else {
		fprintf(stderr, "%s command exited abnormally\n", label);
	}
}

static void usage(bool error)
{
	fprintf(error ? stderr : stdout,
		"usage: %s [--keyboard=CMD] [--mouse=CMD]\n"
		"Listen to udev events and run a command when a keyboard or mouse is plugged in\n"
		"\n"
		"Hooks:\n"
		"  -k, --keyboard=CMD    command to run when keyboard is plugged in (either this\n"
		"                        or --mouse must be given)\n"
		"  -m, --mouse=CMD       command to run when mouse is plugged in (either this or\n"
		"                        --keyboard must be given)\n"
		"\n"
		"Miscellaneous:\n"
		"  -h, --help            display this help message and exit\n",
		progname);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	struct option long_options[] = {
		{"keyboard", required_argument, NULL, 'k'},
		{"mouse", required_argument, NULL, 'm'},
		{"help", no_argument, NULL, 'h'},
	};
	char *keyboard_cmd = NULL;
	char *mouse_cmd = NULL;
	struct udev *udev;
	struct udev_monitor *monitor;
	struct pollfd pollfd;
	int status = EXIT_FAILURE;
	int ret;

	if (argc > 0)
		progname = argv[0];

	for (;;) {
		int c;

		c = getopt_long(argc, argv, "kmh", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'k':
			keyboard_cmd = strdup(optarg);
			if (!keyboard_cmd) {
				perror("strdup");
				goto out;
			}
			break;
		case 'm':
			mouse_cmd = strdup(optarg);
			if (!mouse_cmd) {
				perror("strdup");
				goto out;
			}
			break;
		case 'h':
			free(keyboard_cmd);
			free(mouse_cmd);
			usage(false);
		default:
			free(keyboard_cmd);
			free(mouse_cmd);
			usage(true);
		}
	}
	if (optind != argc || (!keyboard_cmd && !mouse_cmd))
		usage(true);

	if (keyboard_cmd)
		run_cmd(keyboard_cmd, "keyboard");
	if (mouse_cmd)
		run_cmd(mouse_cmd, "mouse");

	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "failed to allocate a new udev context\n");
		goto out;
	}

	monitor = udev_monitor_new_from_netlink(udev, "udev");
	if (!monitor) {
		fprintf(stderr, "failed to allocate udev monitor\n");
		goto out_udev;
	}

	ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);
	if (ret != 0) {
		fprintf(stderr, "failed to add udev subsystem filter\n");
		goto out_udev_monitor;
	}

	ret = udev_monitor_enable_receiving(monitor);
	if (ret != 0) {
		fprintf(stderr, "failed to enable udev monitor\n");
		goto out_udev_monitor;
	}

	pollfd.fd = udev_monitor_get_fd(monitor);
	pollfd.events = POLLIN;
	for (;;) {
		struct udev_device *device;
		bool run_keyboard = false;
		bool run_mouse = false;

		pollfd.revents = 0;
		ret = poll(&pollfd, 1, -1);
		if (ret == -1) {
			perror("poll");
			goto out_udev_monitor;
		}

		if (!(pollfd.revents & POLLIN))
			continue;

		while ((device = udev_monitor_receive_device(monitor))) {
			if (strcmp(udev_device_get_action(device), "add") == 0) {
				if (is_keyboard(device))
					run_keyboard = true;
				if (is_mouse(device))
					run_mouse = true;
			}
			udev_device_unref(device);
		}

		if (run_keyboard && keyboard_cmd)
			run_cmd(keyboard_cmd, "keyboard");
		if (run_mouse && mouse_cmd)
			run_cmd(mouse_cmd, "mouse");
	}

	status = EXIT_SUCCESS;
out_udev_monitor:
	udev_monitor_unref(monitor);
out_udev:
	udev_unref(udev);
out:
	free(keyboard_cmd);
	free(mouse_cmd);
	return status;
}
