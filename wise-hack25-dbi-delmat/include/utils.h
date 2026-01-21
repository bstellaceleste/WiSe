#include <stdint.h>
#ifndef UTILS_H
#define UTILS_H

#include "dr_api.h"

/* Logging levels */
#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARN    1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_TRACE   4

/* Timer structure */
typedef struct {
    uint64_t start_time;
    uint64_t total_time;
    uint64_t call_count;
} wise_timer_t;

/* Initialization */
void utils_init(void);
void utils_cleanup(void);

/* Logging */
void log_init(int level);
void log_message(int level, const char *format, ...);
void log_hexdump(int level, const char *label, const void *data, size_t len);
void log_set_level(int level);

#define LOG_ERROR(...)   log_message(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(...)    log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(...)    log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...)   log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_TRACE(...)   log_message(LOG_LEVEL_TRACE, __VA_ARGS__)

/* String utilities */
char* str_duplicate(const char *src);
bool str_ends_with(const char *str, const char *suffix);
bool str_starts_with(const char *str, const char *prefix);
char* str_format(const char *format, ...);
char* str_to_lower(const char *str);

/* Address utilities */
app_pc addr_from_symbol(const char *module_name, const char *symbol);
const char* addr_to_symbol(app_pc addr);
bool addr_in_module(app_pc addr, const char *module_name);
int64_t addr_calculate_offset(app_pc from, app_pc to);
bool addr_is_readable(app_pc addr, size_t size);

/* Thread-safe utilities */
void* ts_malloc(size_t size);
void ts_free(void *ptr);
void ts_strdup(const char **dest, const char *src);
uint64_t ts_atomic_increment(uint64_t *counter);
uint64_t ts_atomic_decrement(uint64_t *counter);

/* Timing utilities */
void timer_start(wise_timer_t *timer);
void timer_stop(wise_timer_t *timer);
double timer_average(wise_timer_t *timer);
double timer_average_ms(wise_timer_t *timer);
void timer_reset(wise_timer_t *timer);
uint64_t get_timestamp_us(void);
uint64_t get_timestamp_ms(void);
void sleep_us(uint64_t microseconds);
void sleep_ms(uint64_t milliseconds);

/* Hashing utilities */
uint32_t hash_string(const char *str);
uint32_t hash_address(app_pc addr);

/* Memory utilities */
bool safe_memcpy(void *dest, const void *src, size_t n);
bool safe_memset(void *dest, int c, size_t n);
void secure_zero(void *ptr, size_t len);

/* Formatting utilities */
const char* format_size(uint64_t bytes);
const char* format_time_us(uint64_t microseconds);
const char* format_percent(double value);

/* Error handling */
const char* get_last_error(void);
void set_last_error(const char *error);
bool check_condition(bool condition, const char *message, ...);

/* File utilities */
char* file_read_all(const char *filename, size_t *out_size);
bool file_exists(const char *filename);

/* Assertion macro */
#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        LOG_ERROR("Assertion failed at %s:%d: %s", __FILE__, __LINE__, msg); \
    } \
} while(0)

#endif /* UTILS_H */