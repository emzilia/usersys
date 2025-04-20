//#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <systemd/sd-bus.h>

#define _cleanup_(f) __attribute__((cleanup(f)))
#define DESTINATION "org.freedesktop.systemd1"
#define PATH        "/org/freedesktop/systemd1"
#define INTERFACE   "org.freedesktop.systemd1.Manager"

typedef struct UserSysStatus {
	char **         unit;
	int             status;
} UserSysStatus;

typedef struct UserSysContents {
	const char*	path;
	char**		service_units;
	char**		timer_units;
	int		service_size;
	int             timer_size;
	UserSysStatus*  statuses;
} UserSysContents;

static int log_error(int error, const char *message) {
	fprintf(stderr, "%s: %s\n", message, strerror(-error));
	return error;
}

const char* get_usersys_path() {
	char* home = getpwuid(getuid())->pw_dir;
	char* usersys_path = "/.config/systemd/user/";
	strcat(home, usersys_path);
	return home;
}

void init_usersyscontents(UserSysContents* path, int service_count, int timer_count) {
	path->service_size = service_count;
	path->service_units = malloc((service_count + 1) * sizeof(char*));
	if (path->service_units == NULL) {
		fprintf(stderr, "Error: Unable to allocate memory for struct\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < service_count; ++i) {
		path->service_units[i] = malloc((64 + 1) * sizeof(char*));
		if (path->service_units[i] == NULL) {
			fprintf(stderr, "Error: Unable to allocate memory for struct members\n");
			exit(EXIT_FAILURE);
		}
	}
	path->timer_size = timer_count;
	path->timer_units = malloc((timer_count + 1) * sizeof(char*));
	if (path->timer_units == NULL) {
		fprintf(stderr, "Error: Unable to allocate memory for struct\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < timer_count; ++i) {
		path->timer_units[i] = malloc((64 + 1) * sizeof(char*));
		if (path->timer_units[i] == NULL) {
			fprintf(stderr, "Error: Unable to allocate memory for struct members\n");
			exit(EXIT_FAILURE);
		}
	}
}

void init_usersysstatuses(UserSysContents* path) {
	int count = 0;
	count += path->service_size;
	count += path->timer_size;
	
}

UserSysContents get_units(UserSysContents* path) {
	int service_count = 0;
	int timer_count = 0;
	struct dirent* entry;
	char* file_ext;

	DIR* dir = opendir(path->path);
	
	if (dir == NULL) {
		fprintf(stderr, "Error: Unable to open path for copy\n");
		exit(EXIT_FAILURE);
	}
	
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
	
		file_ext = strrchr(entry->d_name, '.');
		if (file_ext != NULL) {
			if (!strcmp(file_ext, ".service"))
				++service_count;
			if (!strcmp(file_ext, ".timer"))
				++timer_count;
		}

	}
	closedir(dir);

	init_usersyscontents(path, service_count, timer_count);
	
	dir = opendir(path->path);
	if (dir == NULL) {
		fprintf(stderr, "Error: Unable to open path for copy\n");
		exit(EXIT_FAILURE);
	}
	int i = 0;
	int j = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

		file_ext = strrchr(entry->d_name, '.');
		if (file_ext != NULL) {
			if (!strcmp(file_ext, ".service"))
			    strcpy(path->service_units[i++], entry->d_name);
			if (!strcmp(file_ext, ".timer"))
			    strcpy(path->timer_units[j++], entry->d_name);
		}
	}
	closedir(dir);

	return *path;
}

void free_elements(UserSysContents* path) {
	for (int i = 0; i < path->service_size; ++i)
		free(path->service_units[i]);
	free(path->service_units);
	for (int i = 0; i < path->timer_size; ++i)
		free(path->timer_units[i]);
	free(path->timer_units);
}

sd_bus_message* get_unit_status(sd_bus *bus, sd_bus_error error, sd_bus_message *reply, char *unit) {
	int r = sd_bus_call_method(
		bus, 
		DESTINATION, 
		PATH, 
		INTERFACE, 
		"GetUnitFileState", 
		&error, 
		&reply, 
		"s",
		unit
		
	);
	if (r < 0) log_error(r, "GetUnitFileState call failed");
	return reply;
}

UserSysContents usersys_contents;

int main(int argc, char **argv) {
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
        _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
	_cleanup_(sd_bus_message_unrefp) sd_bus_message *reply = NULL;
	int r;

	usersys_contents.path = get_usersys_path();
	usersys_contents = get_units(&usersys_contents);

	r = sd_bus_open_user(&bus);
	if (r < 0) return log_error(r, "Failed to acquire bus");

	reply = get_unit_status(bus, error, reply, "gitstatus.timer");

	const char *ans;
	r = sd_bus_message_read(reply, "s", &ans);
	if (r < 0) return log_error(r, "Failed to read reply");

	printf("Unit path is \"%s\".\n", ans);

	for (int i = 0; i < usersys_contents.service_size; ++i) {
		printf("%s\n", usersys_contents.service_units[i]);
	}
	
	for (int i = 0; i < usersys_contents.timer_size; ++i) {
		printf("%s\n", usersys_contents.timer_units[i]);
	}

	free_elements(&usersys_contents);
	
	exit(EXIT_SUCCESS);
}
