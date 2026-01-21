/*
 * WiSe Hack'25 - Utilities Implementation
 * File: src/utils.c
 * CORRECTED VERSION - Fixed dr_safe_read() and deprecated API calls
 */

 #include "utils.h"
 #include "config.h"
 #include <stdarg.h>
 #include <string.h>
 #include <time.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <sys/time.h>
 #include <ctype.h>
 
 
 /* ========== INTERNAL DATA STRUCTURES ========== */
 
 typedef struct {
     int log_level;
     FILE *log_file;
     bool log_to_stderr;
     bool log_to_file;
     char log_filename[256];
     void *log_lock;
 } logging_context_t;
 
 static logging_context_t g_log_context = {
     .log_level = LOG_LEVEL_INFO,
     .log_file = NULL,
     .log_to_stderr = true,
     .log_to_file = false,
     .log_filename = {0},
     .log_lock = NULL
 };
 
 static char *g_log_level_names[] = {
     "ERROR",
     "WARN",
     "INFO",
     "DEBUG",
     "TRACE"
 };
 
 static char *g_log_level_colors[] = {
     "\033[1;31m",  // RED for ERROR
     "\033[1;33m",  // YELLOW for WARN
     "\033[1;32m",  // GREEN for INFO
     "\033[1;36m",  // CYAN for DEBUG
     "\033[1;35m"   // MAGENTA for TRACE
 };
 
 /* ========== LOGGING IMPLEMENTATION ========== */
 
 /**
  * @brief Initialize logging system
  */
 void log_init(int level) {
     if (!g_log_context.log_lock) {
         g_log_context.log_lock = dr_mutex_create();
     }
     
     g_log_context.log_level = level;
     
     engine_config_t *config = config_get();
     if (config && config->verbose_logging) {
         g_log_context.log_level = LOG_LEVEL_DEBUG;
     }
     
     // Check for log file environment variable
     const char *log_file = getenv("DBI_LOG_FILE");
     if (log_file && log_file[0] != '\0') {
         strncpy(g_log_context.log_filename, log_file, 
                 sizeof(g_log_context.log_filename) - 1);
         g_log_context.log_filename[sizeof(g_log_context.log_filename) - 1] = '\0';
         
         g_log_context.log_file = fopen(log_file, "a");
         if (g_log_context.log_file) {
             g_log_context.log_to_file = true;
             setbuf(g_log_context.log_file, NULL);  // Unbuffered
             fprintf(stderr, "[INFO] Logging to file: %s\n", log_file);
         } else {
             fprintf(stderr, "[ERROR] Failed to open log file: %s (%s)\n", 
                    log_file, strerror(errno));
         }
     }
     
     fprintf(stderr, "[INFO] Logging initialized (level: %s)\n", 
            g_log_level_names[g_log_context.log_level]);
 }
 
 /**
  * @brief Internal logging function with formatting
  */
 static void log_internal(int level, const char *format, va_list args) {
     if (level > g_log_context.log_level) {
         return;
     }
     
     dr_mutex_lock(g_log_context.log_lock);
     
     // Get timestamp
     struct timeval tv;
     struct tm tm_info;
     char timestamp[32];
     
     gettimeofday(&tv, NULL);
     localtime_r(&tv.tv_sec, &tm_info);
     strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);
     
     // Get process/thread info
     process_id_t pid = dr_get_process_id();
     thread_id_t tid = dr_get_thread_id(dr_get_current_drcontext());
     
     // Format the message
     char message[2048];
     vsnprintf(message, sizeof(message), format, args);
     
     // Log to stderr (colored)
     if (g_log_context.log_to_stderr) {
         fprintf(stderr, "%s[%s.%03ld][PID:%d:TID:%d][%s]%s %s\n",
                 g_log_level_colors[level],
                 timestamp, tv.tv_usec / 1000,
                 (int)pid, (int)tid,
                 g_log_level_names[level],
                 "\033[0m",  // Reset color
                 message);
     }
     
     // Log to file (uncolored)
     if (g_log_context.log_to_file && g_log_context.log_file) {
         fprintf(g_log_context.log_file, 
                 "[%s.%03ld][PID:%d:TID:%d][%s] %s\n",
                 timestamp, tv.tv_usec / 1000,
                 (int)pid, (int)tid,
                 g_log_level_names[level],
                 message);
         fflush(g_log_context.log_file);
     }
     
     dr_mutex_unlock(g_log_context.log_lock);
 }
 
 /**
  * @brief Log a message with variable arguments
  */
 void log_message(int level, const char *format, ...) {
     va_list args;
     va_start(args, format);
     log_internal(level, format, args);
     va_end(args);
 }
 
 /**
  * @brief Hex dump utility
  */
 void log_hexdump(int level, const char *label, const void *data, size_t len) {
     if (level > g_log_context.log_level) {
         return;
     }
     
     if (!data || len == 0) {
         log_message(level, "%s: (null or empty)", label);
         return;
     }
     
     const unsigned char *bytes = (const unsigned char *)data;
     char line[128];
     char ascii[17];
     size_t i, j;
     
     log_message(level, "%s (%zu bytes):", label, len);
     
     for (i = 0; i < len; i += 16) {
         snprintf(line, sizeof(line), "  %04zx: ", i);
         
         // Hex bytes
         for (j = 0; j < 16; j++) {
             if (i + j < len) {
                 snprintf(line + strlen(line), 
                         sizeof(line) - strlen(line),
                         "%02x ", bytes[i + j]);
             } else {
                 strcat(line, "   ");
             }
             
             if (j == 7) {
                 strcat(line, " ");
             }
         }
         
         strcat(line, " ");
         
         // ASCII representation
         for (j = 0; j < 16; j++) {
             if (i + j < len) {
                 unsigned char c = bytes[i + j];
                 ascii[j] = (c >= 32 && c < 127) ? c : '.';
             } else {
                 ascii[j] = ' ';
             }
         }
         ascii[j] = '\0';
         strcat(line, ascii);
         
         log_message(level, "%s", line);
     }
 }
 
 void log_set_level(int level) {
    dr_mutex_lock(g_log_context.log_lock);
    g_log_context.log_level = level;
    dr_mutex_unlock(g_log_context.log_lock);
    LOG_INFO("Log level updated to %s", g_log_level_names[level]);
}

 /**
  * @brief Cleanup logging system
  */
 void log_cleanup(void) {
     if (g_log_context.log_lock) {
         dr_mutex_lock(g_log_context.log_lock);
         
         if (g_log_context.log_file) {
             fclose(g_log_context.log_file);
             g_log_context.log_file = NULL;
         }
         
         g_log_context.log_to_file = false;
         g_log_context.log_filename[0] = '\0';
         
         dr_mutex_unlock(g_log_context.log_lock);
         dr_mutex_destroy(g_log_context.log_lock);
         g_log_context.log_lock = NULL;
     }
 }
 
 /* ========== STRING UTILITIES ========== */
 
 /**
  * @brief Duplicate a string with allocation
  */
 char* str_duplicate(const char *src) {
     if (!src) {
         return NULL;
     }
     
     size_t len = strlen(src) + 1;
     char *dst = (char *)dr_global_alloc(len);
     if (dst) {
         memcpy(dst, src, len);
     }
     
     return dst;
 }
 
 /**
  * @brief Check if string ends with suffix
  */
 bool str_ends_with(const char *str, const char *suffix) {
     if (!str || !suffix) {
         return false;
     }
     
     size_t str_len = strlen(str);
     size_t suffix_len = strlen(suffix);
     
     if (suffix_len > str_len) {
         return false;
     }
     
     return strcmp(str + str_len - suffix_len, suffix) == 0;
 }
 
 /**
  * @brief Check if string starts with prefix
  */
 bool str_starts_with(const char *str, const char *prefix) {
     if (!str || !prefix) {
         return false;
     }
     
     size_t prefix_len = strlen(prefix);
     if (prefix_len == 0) {
         return true;
     }
     
     return strncmp(str, prefix, prefix_len) == 0;
 }
 
 /**
  * @brief Format string with allocation
  */
 char* str_format(const char *format, ...) {
     va_list args;
     va_start(args, format);
     
     // First pass to get required size
     va_list args_copy;
     va_copy(args_copy, args);
     int needed = vsnprintf(NULL, 0, format, args_copy);
     va_end(args_copy);
     
     if (needed < 0) {
         va_end(args);
         return NULL;
     }
     
     // Allocate and format
     char *buffer = (char *)dr_global_alloc(needed + 1);
     if (!buffer) {
         va_end(args);
         return NULL;
     }
     
     vsnprintf(buffer, needed + 1, format, args);
     va_end(args);
     
     return buffer;
 }
 
 /**
  * @brief Convert string to lowercase
  */
 char* str_to_lower(const char *str) {
     if (!str) {
         return NULL;
     }
     
     size_t len = strlen(str);
     char *lower = (char *)dr_global_alloc(len + 1);
     if (!lower) {
         return NULL;
     }
     
     for (size_t i = 0; i < len; i++) {
         lower[i] = tolower((unsigned char)str[i]);
     }
     lower[len] = '\0';
     
     return lower;
 }
 
 /* ========== ADDRESS UTILITIES ========== */
 
 /**
  * @brief Get address from symbol name
  */
 app_pc addr_from_symbol(const char *module_name, const char *symbol) {
     if (!symbol) {
         return NULL;
     }
     
     // Iterate through all loaded modules
     module_data_t *mod = dr_lookup_module_by_name(module_name ? module_name : dr_get_application_name());
     if (mod) {
         app_pc addr = (app_pc)dr_get_proc_address(mod->handle, symbol);
         if (addr) {
             dr_free_module_data(mod);
             return addr;
         }
         dr_free_module_data(mod);
     }
     
     // If not found in specific module, search all modules
     dr_module_iterator_t *iter = dr_module_iterator_start();
     while (dr_module_iterator_hasnext(iter)) {
         mod = dr_module_iterator_next(iter);
         
         if (!module_name || strstr(dr_module_preferred_name(mod), module_name)) {
             app_pc addr = (app_pc)dr_get_proc_address(mod->handle, symbol);
             if (addr) {
                 dr_free_module_data(mod);
                 dr_module_iterator_stop(iter);
                 return addr;
             }
         }
         
         dr_free_module_data(mod);
     }
     dr_module_iterator_stop(iter);
     
     return NULL;
 }
 
 /**
  * @brief Get symbol name for address
  */
 const char* addr_to_symbol(app_pc addr) {
     module_data_t *mod = dr_lookup_module(addr);
     if (!mod) {
         return NULL;
     }
     
     const char *mod_name = dr_module_preferred_name(mod);
     
     // Store in thread-local buffer
     static __thread char buffer[256];
     snprintf(buffer, sizeof(buffer), "%s+0x%lx", 
              mod_name, (unsigned long)(addr - mod->start));
     
     dr_free_module_data(mod);
     return buffer;
 }
 
 /**
  * @brief Check if address is in module
  */
 bool addr_in_module(app_pc addr, const char *module_name) {
     if (!addr || !module_name) {
         return false;
     }
     
     module_data_t *mod = dr_lookup_module(addr);
     if (!mod) {
         return false;
     }
     
     bool result = strstr(dr_module_preferred_name(mod), module_name) != NULL;
     dr_free_module_data(mod);
     
     return result;
 }
 
 /**
  * @brief Calculate relative offset between two addresses
  */
 int64_t addr_calculate_offset(app_pc from, app_pc to) {
     if (!from || !to) {
         return 0;
     }
     
     return (int64_t)(to - from);
 }
 
 /**
  * @brief Check if pointer is readable
  * FIXED: Correct dr_safe_read() call
  */
 bool addr_is_readable(app_pc addr, size_t size) {
     if (!addr || size == 0) {
         return false;
     }
     
     // Try to read requested bytes
     uint8_t *test = dr_global_alloc(size);
     if (!test) {
         return false;
     }
     
     size_t bytes_read = 0;
     bool result = dr_safe_read(addr, size, test, &bytes_read) && (bytes_read == size);
     
     dr_global_free(test, size);
     return result;
 }
 
 /* ========== THREAD-SAFE UTILITIES ========== */
 
 /**
  * @brief Thread-safe memory allocation
  */
 void* ts_malloc(size_t size) {
     static void *alloc_lock = NULL;
     
     if (!alloc_lock) {
         alloc_lock = dr_mutex_create();
     }
     
     dr_mutex_lock(alloc_lock);
     void *ptr = dr_global_alloc(size);
     dr_mutex_unlock(alloc_lock);
     
     return ptr;
 }
 
 /**
  * @brief Thread-safe memory free
  */
 void ts_free(void *ptr) {
     static void *free_lock = NULL;
     
     if (!ptr) {
         return;
     }
     
     if (!free_lock) {
         free_lock = dr_mutex_create();
     }
     
     dr_mutex_lock(free_lock);
     dr_global_free(ptr, 0);  // Size 0 means we don't track it
     dr_mutex_unlock(free_lock);
 }
 
 /**
  * @brief Thread-safe string duplicate
  */
 void ts_strdup(const char **dest, const char *src) {
     static void *str_lock = NULL;
     
     if (!str_lock) {
         str_lock = dr_mutex_create();
     }
     
     dr_mutex_lock(str_lock);
     
     // Free previous if exists
     if (*dest) {
         dr_global_free((void *)*dest, 0);
         *dest = NULL;
     }
     
     // Duplicate new string
     if (src) {
         *dest = str_duplicate(src);
     }
     
     dr_mutex_unlock(str_lock);
 }
 
 /**
  * @brief Thread-safe atomic counter increment
  */
 uint64_t ts_atomic_increment(uint64_t *counter) {
     static void *counter_lock = NULL;
     
     if (!counter) {
         return 0;
     }
     
     if (!counter_lock) {
         counter_lock = dr_mutex_create();
     }
     
     dr_mutex_lock(counter_lock);
     uint64_t result = ++(*counter);
     dr_mutex_unlock(counter_lock);
     
     return result;
 }
 
 /**
  * @brief Thread-safe atomic counter decrement
  */
 uint64_t ts_atomic_decrement(uint64_t *counter) {
     static void *counter_lock = NULL;
     
     if (!counter) {
         return 0;
     }
     
     if (!counter_lock) {
         counter_lock = dr_mutex_create();
     }
     
     dr_mutex_lock(counter_lock);
     uint64_t result = --(*counter);
     dr_mutex_unlock(counter_lock);
     
     return result;
 }
 
 /* ========== TIMING UTILITIES ========== */
 
 /**
  * @brief Get microsecond timestamp (replacement for dr_get_micros)
  */
 static inline uint64_t get_micros(void) {
     struct timeval tv;
     gettimeofday(&tv, NULL);
     return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
 }
 
 /**
  * @brief Start timer
  */
 void timer_start(wise_timer_t *timer) {
     if (timer) {
         timer->start_time = get_micros();
     }
 }
 
 /**
  * @brief Stop timer and update statistics
  */
 void timer_stop(wise_timer_t *timer) {
     if (timer && timer->start_time > 0) {
         uint64_t end_time = get_micros();
         timer->total_time += (end_time - timer->start_time);
         timer->call_count++;
         timer->start_time = 0;
     }
 }
 
 /**
  * @brief Get average time in microseconds
  */
 double timer_average(wise_timer_t *timer) {
     if (!timer || timer->call_count == 0) {
         return 0.0;
     }
     
     return (double)timer->total_time / timer->call_count;
 }
 
 /**
  * @brief Get average time in milliseconds
  */
 double timer_average_ms(wise_timer_t *timer) {
     return timer_average(timer) / 1000.0;
 }
 
 /**
  * @brief Reset timer statistics
  */
 void timer_reset(wise_timer_t *timer) {
     if (timer) {
         timer->start_time = 0;
         timer->total_time = 0;
         timer->call_count = 0;
     }
 }
 
 /**
  * @brief Get current timestamp in microseconds
  */
 uint64_t get_timestamp_us(void) {
     return get_micros();
 }
 
 /**
  * @brief Get current timestamp in milliseconds
  */
 uint64_t get_timestamp_ms(void) {
     return get_micros() / 1000;
 }
 
 /**
  * @brief Sleep for microseconds
  */
 void sleep_us(uint64_t microseconds) {
     dr_sleep((int)(microseconds / 1000));  // dr_sleep takes milliseconds
 }
 
 /**
  * @brief Sleep for milliseconds
  */
 void sleep_ms(uint64_t milliseconds) {
     dr_sleep((int)milliseconds);
 }
 
 /* ========== HASHING UTILITIES ========== */
 
 /**
  * @brief Simple hash function for strings
  */
 uint32_t hash_string(const char *str) {
     if (!str) {
         return 0;
     }
     
     // djb2 hash algorithm
     uint32_t hash = 5381;
     int c;
     
     while ((c = *str++)) {
         hash = ((hash << 5) + hash) + c;  // hash * 33 + c
     }
     
     return hash;
 }
 
 /**
  * @brief Hash function for addresses
  */
 uint32_t hash_address(app_pc addr) {
     // Simple hash for addresses
     uintptr_t ptr = (uintptr_t)addr;
     return (uint32_t)(ptr ^ (ptr >> 32));
 }
 
 /* ========== MEMORY UTILITIES ========== */
 
 /**
  * @brief Safe memory copy with bounds checking
  */
 bool safe_memcpy(void *dest, const void *src, size_t n) {
     if (!dest || !src || n == 0) {
         return false;
     }
     
     // Check if source is readable
     if (!addr_is_readable((app_pc)src, n)) {
         log_message(LOG_LEVEL_ERROR, "Source memory not readable: %p (%zu bytes)", src, n);
         return false;
     }
     
     memcpy(dest, src, n);
     return true;
 }
 
 /**
  * @brief Safe memory set with bounds checking
  */
 bool safe_memset(void *dest, int c, size_t n) {
     if (!dest || n == 0) {
         return false;
     }
     
     memset(dest, c, n);
     return true;
 }
 
 /**
  * @brief Zero out sensitive memory
  */
 void secure_zero(void *ptr, size_t len) {
     if (ptr && len > 0) {
         memset(ptr, 0, len);
         
         // Compiler barrier to prevent optimization
         __asm__ volatile("" : : "r"(ptr) : "memory");
     }
 }
 
 /* ========== FORMATTING UTILITIES ========== */
 
 /**
  * @brief Format size in human readable form
  */
 const char* format_size(uint64_t bytes) {
     static __thread char buffer[32];
     const char *units[] = {"B", "KB", "MB", "GB", "TB"};
     int unit = 0;
     double size = bytes;
     
     while (size >= 1024 && unit < 4) {
         size /= 1024;
         unit++;
     }
     
     snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
     return buffer;
 }
 
 /**
  * @brief Format time in human readable form
  */
 const char* format_time_us(uint64_t microseconds) {
     static __thread char buffer[32];
     
     if (microseconds < 1000) {
         snprintf(buffer, sizeof(buffer), "%llu us", 
                 (unsigned long long)microseconds);
     } else if (microseconds < 1000000) {
         snprintf(buffer, sizeof(buffer), "%.2f ms", microseconds / 1000.0);
     } else {
         snprintf(buffer, sizeof(buffer), "%.2f s", microseconds / 1000000.0);
     }
     
     return buffer;
 }
 
 /**
  * @brief Format percentage
  */
 const char* format_percent(double value) {
     static __thread char buffer[16];
     snprintf(buffer, sizeof(buffer), "%.2f%%", value * 100.0);
     return buffer;
 }
 
 /* ========== ERROR HANDLING ========== */
 
 /**
  * @brief Get last error string
  */
 const char* get_last_error(void) {
     return strerror(errno);
 }
 
 /**
  * @brief Set last error
  */
 void set_last_error(const char *error) {
     if (error) {
         log_message(LOG_LEVEL_ERROR, "Error: %s", error);
     }
 }
 
 /**
  * @brief Check condition and log error if false
  */
 bool check_condition(bool condition, const char *message, ...) {
     if (!condition) {
         va_list args;
         va_start(args, message);
         
         char formatted[512];
         vsnprintf(formatted, sizeof(formatted), message, args);
         
         log_message(LOG_LEVEL_ERROR, "Condition failed: %s", formatted);
         
         va_end(args);
     }
     
     return condition;
 }
 
 /* ========== FILE UTILITIES ========== */
 
 /**
  * @brief Read entire file into buffer
  */
 char* file_read_all(const char *filename, size_t *out_size) {
     if (!filename) {
         return NULL;
     }
     
     FILE *file = fopen(filename, "rb");
     if (!file) {
         log_message(LOG_LEVEL_ERROR, "Failed to open file: %s", filename);
         return NULL;
     }
     
     // Get file size
     fseek(file, 0, SEEK_END);
     long size = ftell(file);
     fseek(file, 0, SEEK_SET);
     
     if (size <= 0) {
         fclose(file);
         return NULL;
     }
     
     // Allocate and read
     char *buffer = (char *)dr_global_alloc(size + 1);
     if (!buffer) {
         fclose(file);
         return NULL;
     }
     
     size_t read_size = fread(buffer, 1, size, file);
     fclose(file);
     
     if (read_size != (size_t)size) {
         dr_global_free(buffer, size + 1);
         return NULL;
     }
     
     buffer[size] = '\0';
     
     if (out_size) {
         *out_size = size;
     }
     
     return buffer;
 }
 
 /**
  * @brief Check if file exists
  */
 bool file_exists(const char *filename) {
     if (!filename) {
         return false;
     }
     
     FILE *file = fopen(filename, "r");
     if (file) {
         fclose(file);
         return true;
     }
     
     return false;
 }
 
 /* ========== INITIALIZATION ========== */
 
 /**
  * @brief Initialize utilities subsystem
  */
 void utils_init(void) {
     // Initialize logging
     log_init(LOG_LEVEL_INFO);
     
     log_message(LOG_LEVEL_DEBUG, "Utilities subsystem initialized");
 }
 
 /**
  * @brief Cleanup utilities subsystem
  */
 void utils_cleanup(void) {
     log_cleanup();
 }