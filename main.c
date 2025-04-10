#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <systemd/sd-bus.h>

#define _cleanup_(f) __attribute__((cleanup(f)))
#define DESTINATION "org.freedesktop.systemd1"
#define PATH        "/org/freedesktop/systemd1"
#define INTERFACE   "org.freedesktop.systemd1.Manager"

static int log_error(int error, const char *message) {
	fprintf(stderr, "%s: %s\n", message, strerror(-error));
	return error;
}

sd_bus_message* list_user_units(sd_bus *bus, sd_bus_error error, sd_bus_message *reply) {
	int r = sd_bus_call_method(
		bus, 
		DESTINATION, 
		PATH, 
	        INTERFACE, 
		"ListUnits", 
	        &error, 
		&reply, 
		"a" 
	);
	if (r < 0) log_error(r, "ListUnits call failed");
	return reply;
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

int main(int argc, char **argv) {
	_cleanup_(sd_bus_flush_close_unrefp) sd_bus *bus = NULL;
	_cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
	_cleanup_(sd_bus_message_unrefp) sd_bus_message *reply = NULL;
	int r;

	r = sd_bus_open_user(&bus);
	if (r < 0) return log_error(r, "Failed to acquire bus");

	reply = get_unit_status(bus, error, reply, "gitstatus.service");

	const char *ans;
	r = sd_bus_message_read(reply, "s", &ans);
	if (r < 0) return log_error(r, "Failed to read reply");

	printf("Unit path is \"%s\".\n", ans);

	return 0;
}
