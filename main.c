#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <systemd/sd-bus.h>

#define _cleanup_(f) __attribute__((cleanup(f)))
#define DESTINATION "org.freedesktop.systemd1"
#define PATH        "/org/freedesktop/systemd1"
#define INTERFACE   "org.freedesktop.systemd1.Manager"

typedef struct UserSysContents {
	const char*	path;
	char**		service_units;
	char**		timer_units;
	int		size;
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

	r = sd_bus_open_user(&bus);
	if (r < 0) return log_error(r, "Failed to acquire bus");

	reply = get_unit_status(bus, error, reply, "gitstatus.timer");

	const char *ans;
	r = sd_bus_message_read(reply, "s", &ans);
	if (r < 0) return log_error(r, "Failed to read reply");

	printf("Unit path is \"%s\".\n", ans);
	const char* usersys_path = get_usersys_path();
	printf("%s\n", usersys_path);

	return 0;
}
