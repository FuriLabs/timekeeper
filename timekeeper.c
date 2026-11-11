
/**
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#define RTC_SYS_FILE "/sys/class/rtc/rtc0/since_epoch"
#define DEFAULT_DATA_DIR "/data/time/"
#define OFFSET_FILENAME "timekeep"

static gchar *
get_data_dir(void)
{
    const gchar *env_dir = g_getenv("TIMEKEEPER_DATA_DIR");
    return g_strdup(env_dir ? env_dir : DEFAULT_DATA_DIR);
}

static gchar *
get_offset_file_path(void)
{
    g_autofree gchar *data_dir = get_data_dir();
    return g_build_filename(data_dir, OFFSET_FILENAME, NULL);
}

static gboolean
read_int_from_file(const gchar *filename, gint64 *value)
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *contents = NULL;
    gchar *endptr = NULL;

    if (!g_file_get_contents(filename, &contents, NULL, &error)) {
        g_warning("Failed to read file %s: %s", filename, error->message);
        return FALSE;
    }

    g_strstrip(contents);

    errno = 0;
    gint64 parsed = g_ascii_strtoll(contents, &endptr, 10);

    if (errno == ERANGE || endptr == contents || *endptr != '\0') {
        g_warning("Invalid number format in file %s", filename);
        return FALSE;
    }

    *value = parsed;
    return TRUE;
}

static gint
write_int_to_file(const gchar *filename, gint64 value)
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *data_dir = get_data_dir();

    if (g_mkdir_with_parents(data_dir, 0755) != 0) {
        g_warning("Cannot create data directory: %s", g_strerror(errno));
        return -1;
    }

    g_autofree gchar *contents = g_strdup_printf("%" G_GINT64_FORMAT, value);

    if (!g_file_set_contents(filename, contents, -1, &error)) {
        g_warning("Failed to write file %s: %s", filename, error->message);
        return -1;
    }

    return 0;
}

static gboolean
read_since_epoch(gint64 *value)
{
    return read_int_from_file(RTC_SYS_FILE, value);
}

static gint
write_offset(gint64 offset)
{
    g_autofree gchar *offset_file = get_offset_file_path();
    return write_int_to_file(offset_file, offset);
}

static gboolean
read_offset(gint64 *value)
{
    g_autofree gchar *offset_file = get_offset_file_path();
    return read_int_from_file(offset_file, value);
}

static gint64
get_current_time(void)
{
    return g_get_real_time() / G_USEC_PER_SEC;
}

static gint
store_time_offset(void)
{
    gint64 current_time = 0;
    gint64 epoch_since = 0;
    gint64 offset = 0;
    gint ret = -1;

    current_time = get_current_time();
    if (current_time <= 0) {
        g_warning("Failed to get current time");
        return -1;
    }

    if (!read_since_epoch(&epoch_since) || epoch_since < 0) {
        g_warning("Failed to read epoch_since");
        return -1;
    }

    offset = current_time - epoch_since;

    g_debug("Time now is %" G_GINT64_FORMAT, current_time);
    g_debug("Epoch since is %" G_GINT64_FORMAT, epoch_since);
    g_debug("Offset is %" G_GINT64_FORMAT, offset);

    ret = write_offset(offset);
    if (ret == 0)
        g_debug("Offset file written successfully");
    else
        g_warning("Failed to write offset file!");

    return ret;
}

static gint
restore_time_offset(void)
{
    struct timeval tv;
    gint64 time_offset = 0;
    gint64 epoch_since = 0;
    gint64 final_time = 0;
    gint ret = -1;

    if (!read_offset(&time_offset)) {
        g_warning("Failed to read time offset");
        return -1;
    }

    if (!read_since_epoch(&epoch_since) || epoch_since < 0) {
        g_warning("Failed to read epoch_since");
        return -1;
    }

    final_time = epoch_since + time_offset;

    if (final_time < 0) {
        g_warning("Calculated time is negative");
        return -1;
    }

    g_debug("Offset is %" G_GINT64_FORMAT, time_offset);
    g_debug("Epoch since is %" G_GINT64_FORMAT, epoch_since);
    g_debug("Final time is %" G_GINT64_FORMAT, final_time);

    tv.tv_sec = (time_t)final_time;
    tv.tv_usec = 0;

    if ((gint64)tv.tv_sec != final_time) {
        g_warning("Restored time cannot be represented by time_t");
        return -1;
    }

    ret = settimeofday(&tv, NULL);
    if (ret != 0)
        g_warning("Failed to set time! Are you root? %s", g_strerror(errno));
    else
        g_debug("Time restored!");

    return ret;
}

static void
print_usage(const gchar *program_name)
{
    g_print("Usage: %s store|restore\n", program_name);
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (g_strcmp0(argv[1], "restore") == 0)
        return restore_time_offset();
    else if (g_strcmp0(argv[1], "store") == 0)
        return store_time_offset();

    print_usage(argv[0]);
    return 1;
}
