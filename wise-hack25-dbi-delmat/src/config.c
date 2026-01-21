#define _GNU_SOURCE
/*
 * WiSe Hack'25 - Configuration Management
 * File: src/config.c
 */

 #include "config.h"
 #include "utils.h"
 #include <stdlib.h>
 #include <string.h>
  #include <strings.h>
 #include <limits.h>
 
 /* ========== GLOBAL CONFIGURATION ========== */
 
 static engine_config_t g_config = {
     .hotspot_multiplier = 2.0,
     .min_calls = 50,
     .analysis_interval = 500,
     .enable_memory_tracking = true,
     .enable_branch_tracking = false,
     .verbose_logging = false,
     .allow_external_patch = false,
     .max_functions = 64,
     .memory_threshold_multiplier = 1.5,
     .call_window_size = 1000,
     .hotspot_cooldown_ms = 10000,
     .log_to_file = false,
     .profile_mode = PROFILE_MODE_LIGHT,
     .adaptation_strategy = ADAPT_STRATEGY_CONSERVATIVE
 };
 
 static void *g_config_lock = NULL;
 
 /* ========== ENVIRONMENT PARSING ========== */
 
 /**
  * @brief Parse boolean from environment variable
  */
 static bool parse_bool(const char *env_value, bool default_value) {
     if (!env_value) {
         return default_value;
     }
     
     if (strcmp(env_value, "1") == 0 ||
         strcasecmp(env_value, "true") == 0 ||
         strcasecmp(env_value, "yes") == 0 ||
         strcasecmp(env_value, "on") == 0) {
         return true;
     }
     
     if (strcmp(env_value, "0") == 0 ||
         strcasecmp(env_value, "false") == 0 ||
         strcasecmp(env_value, "no") == 0 ||
         strcasecmp(env_value, "off") == 0) {
         return false;
     }
     
     return default_value;
 }
 
 /**
  * @brief Parse double from environment variable
  */
 static double parse_double(const char *env_value, double default_value, 
                           double min_val, double max_val) {
     if (!env_value) {
         return default_value;
     }
     
     char *endptr;
     double value = strtod(env_value, &endptr);
     
     if (endptr == env_value || *endptr != '\0') {
         LOG_WARN("Invalid double value: %s, using default: %.2f", 
                 env_value, default_value);
         return default_value;
     }
     
     // Clamp to valid range
     if (value < min_val) {
         LOG_WARN("Value %.2f below minimum %.2f, clamping", value, min_val);
         value = min_val;
     } else if (value > max_val) {
         LOG_WARN("Value %.2f above maximum %.2f, clamping", value, max_val);
         value = max_val;
     }
     
     return value;
 }
 
 /**
  * @brief Parse integer from environment variable
  */
 static int parse_int(const char *env_value, int default_value, 
                     int min_val, int max_val) {
     if (!env_value) {
         return default_value;
     }
     
     char *endptr;
     long value = strtol(env_value, &endptr, 10);
     
     if (endptr == env_value || *endptr != '\0') {
         LOG_WARN("Invalid integer value: %s, using default: %d", 
                 env_value, default_value);
         return default_value;
     }
     
     // Check bounds
     if (value < min_val) {
         LOG_WARN("Value %ld below minimum %d, clamping", value, min_val);
         value = min_val;
     } else if (value > max_val) {
         LOG_WARN("Value %ld above maximum %d, clamping", value, max_val);
         value = max_val;
     }
     
     return (int)value;
 }
 
 /**
  * @brief Parse profile mode from environment variable
  */
 static profile_mode_t parse_profile_mode(const char *env_value) {
     if (!env_value) {
         return PROFILE_MODE_LIGHT;
     }
     
     if (strcasecmp(env_value, "none") == 0) {
         return PROFILE_MODE_NONE;
     } else if (strcasecmp(env_value, "light") == 0) {
         return PROFILE_MODE_LIGHT;
     } else if (strcasecmp(env_value, "full") == 0) {
         return PROFILE_MODE_FULL;
     } else if (strcasecmp(env_value, "aggressive") == 0) {
         return PROFILE_MODE_AGGRESSIVE;
     }
     
     LOG_WARN("Unknown profile mode: %s, using light", env_value);
     return PROFILE_MODE_LIGHT;
 }
 
 /**
  * @brief Parse adaptation strategy from environment variable
  */
 static adaptation_strategy_t parse_adaptation_strategy(const char *env_value) {
     if (!env_value) {
         return ADAPT_STRATEGY_CONSERVATIVE;
     }
     
     if (strcasecmp(env_value, "conservative") == 0) {
         return ADAPT_STRATEGY_CONSERVATIVE;
     } else if (strcasecmp(env_value, "balanced") == 0) {
         return ADAPT_STRATEGY_BALANCED;
     } else if (strcasecmp(env_value, "aggressive") == 0) {
         return ADAPT_STRATEGY_AGGRESSIVE;
     } else if (strcasecmp(env_value, "adaptive") == 0) {
         return ADAPT_STRATEGY_ADAPTIVE;
     }
     
     LOG_WARN("Unknown adaptation strategy: %s, using conservative", env_value);
     return ADAPT_STRATEGY_CONSERVATIVE;
 }
 
 /* ========== CONFIGURATION API ========== */
 
 /**
  * @brief Initialize configuration system
  */
 void config_init(void) {
     if (!g_config_lock) {
         g_config_lock = dr_mutex_create();
     }
     
     // Set default values
     memset(&g_config, 0, sizeof(g_config));
     g_config.hotspot_multiplier = 2.0;
     g_config.min_calls = 50;
     g_config.analysis_interval = 500;
     g_config.enable_memory_tracking = true;
     g_config.enable_branch_tracking = false;
     g_config.verbose_logging = false;
     g_config.allow_external_patch = false;
     g_config.max_functions = 64;
     g_config.memory_threshold_multiplier = 1.5;
     g_config.call_window_size = 1000;
     g_config.hotspot_cooldown_ms = 10000;
     g_config.log_to_file = false;
     g_config.profile_mode = PROFILE_MODE_LIGHT;
     g_config.adaptation_strategy = ADAPT_STRATEGY_CONSERVATIVE;
     
     LOG_DEBUG("Configuration system initialized");
 }
 
 /**
  * @brief Load configuration from environment variables
  */
 void config_from_env(void) {
     dr_mutex_lock(g_config_lock);
     
     const char *env_value;
     
     // Hotspot detection
     env_value = getenv("DBI_HOTSPOT_MULT");
     g_config.hotspot_multiplier = parse_double(env_value, 2.0, 1.0, 10.0);
     
     // Minimum calls before analysis
     env_value = getenv("DBI_MIN_CALLS");
     g_config.min_calls = parse_int(env_value, 50, 1, 10000);
     
     // Analysis interval
     env_value = getenv("DBI_ANALYSIS_INTERVAL");
     g_config.analysis_interval = parse_int(env_value, 500, 10, 60000);
     
     // Memory tracking
     env_value = getenv("DBI_TRACK_MEMORY");
     g_config.enable_memory_tracking = parse_bool(env_value, true);
     
     // Branch tracking (bonus feature)
     env_value = getenv("DBI_TRACK_BRANCHES");
     g_config.enable_branch_tracking = parse_bool(env_value, false);
     
     // Verbose logging
     env_value = getenv("DBI_VERBOSE");
     g_config.verbose_logging = parse_bool(env_value, false);
     
     // External patch allowance
     env_value = getenv("DBI_ALLOW_EXTERNAL");
     g_config.allow_external_patch = parse_bool(env_value, false);
     
     // Maximum functions to track
     env_value = getenv("DBI_MAX_FUNCTIONS");
     g_config.max_functions = parse_int(env_value, 64, 1, 1024);
     
     // Memory threshold multiplier
     env_value = getenv("DBI_MEMORY_THRESHOLD_MULT");
     g_config.memory_threshold_multiplier = parse_double(env_value, 1.5, 0.5, 5.0);
     
     // Call window size
     env_value = getenv("DBI_CALL_WINDOW");
     g_config.call_window_size = parse_int(env_value, 1000, 10, 100000);
     
     // Hotspot cooldown
     env_value = getenv("DBI_COOLDOWN_MS");
     g_config.hotspot_cooldown_ms = parse_int(env_value, 10000, 0, 600000);
     
     // Log to file
     env_value = getenv("DBI_LOG_FILE");
     g_config.log_to_file = (env_value != NULL && env_value[0] != '\0');
     
     // Profile mode
     env_value = getenv("DBI_PROFILE_MODE");
     g_config.profile_mode = parse_profile_mode(env_value);
     
     // Adaptation strategy
     env_value = getenv("DBI_ADAPTATION_STRATEGY");
     g_config.adaptation_strategy = parse_adaptation_strategy(env_value);
     
     dr_mutex_unlock(g_config_lock);
     
     config_dump();
 }
 
 /**
  * @brief Get current configuration
  */
 engine_config_t* config_get(void) {
     return &g_config;
 }
 
 /**
  * @brief Dump configuration to log
  */
 void config_dump(void) {
     dr_mutex_lock(g_config_lock);
     
     LOG_INFO("=== DBI Engine Configuration ===");
     LOG_INFO("Hotspot multiplier:      %.2f", g_config.hotspot_multiplier);
     LOG_INFO("Minimum calls:           %d", g_config.min_calls);
     LOG_INFO("Analysis interval:       %d ms", g_config.analysis_interval);
     LOG_INFO("Memory tracking:         %s", 
              g_config.enable_memory_tracking ? "ENABLED" : "DISABLED");
     LOG_INFO("Branch tracking:         %s", 
              g_config.enable_branch_tracking ? "ENABLED" : "DISABLED");
     LOG_INFO("Verbose logging:         %s", 
              g_config.verbose_logging ? "ENABLED" : "DISABLED");
     LOG_INFO("External patches:        %s", 
              g_config.allow_external_patch ? "ALLOWED" : "DENIED");
     LOG_INFO("Max functions:           %d", g_config.max_functions);
     LOG_INFO("Memory threshold mult:   %.2f", g_config.memory_threshold_multiplier);
     LOG_INFO("Call window size:        %d", g_config.call_window_size);
     LOG_INFO("Hotspot cooldown:        %d ms", g_config.hotspot_cooldown_ms);
     LOG_INFO("Log to file:             %s", 
              g_config.log_to_file ? "YES" : "NO");
     
     // Profile mode string
     const char *profile_str = "UNKNOWN";
     switch (g_config.profile_mode) {
         case PROFILE_MODE_NONE: profile_str = "NONE"; break;
         case PROFILE_MODE_LIGHT: profile_str = "LIGHT"; break;
         case PROFILE_MODE_FULL: profile_str = "FULL"; break;
         case PROFILE_MODE_AGGRESSIVE: profile_str = "AGGRESSIVE"; break;
     }
     LOG_INFO("Profile mode:            %s", profile_str);
     
     // Adaptation strategy string
     const char *adapt_str = "UNKNOWN";
     switch (g_config.adaptation_strategy) {
         case ADAPT_STRATEGY_CONSERVATIVE: adapt_str = "CONSERVATIVE"; break;
         case ADAPT_STRATEGY_BALANCED: adapt_str = "BALANCED"; break;
         case ADAPT_STRATEGY_AGGRESSIVE: adapt_str = "AGGRESSIVE"; break;
         case ADAPT_STRATEGY_ADAPTIVE: adapt_str = "ADAPTIVE"; break;
     }
     LOG_INFO("Adaptation strategy:     %s", adapt_str);
     LOG_INFO("================================");
     
     dr_mutex_unlock(g_config_lock);
 }
 
 /**
  * @brief Calculate dynamic threshold based on average calls
  */
 double config_get_dynamic_threshold(double avg_calls) {
     dr_mutex_lock(g_config_lock);
     double threshold = avg_calls * g_config.hotspot_multiplier;
     dr_mutex_unlock(g_config_lock);
     
     return threshold;
 }
 
 /**
  * @brief Calculate memory threshold based on average memory operations
  */
 uint64_t config_get_memory_threshold(double avg_mem_ops) {
     dr_mutex_lock(g_config_lock);
     uint64_t threshold = (uint64_t)(avg_mem_ops * g_config.memory_threshold_multiplier);
     dr_mutex_unlock(g_config_lock);
     
     return threshold;
 }
 
 /**
  * @brief Check if function should be considered for adaptation
  */
 bool config_should_adapt_function(uint64_t call_count, uint64_t memory_ops, 
                                   double avg_calls, double avg_memory) {
     dr_mutex_lock(g_config_lock);
     
     bool should_adapt = false;
     
     // Check based on adaptation strategy
     switch (g_config.adaptation_strategy) {
         case ADAPT_STRATEGY_CONSERVATIVE:
             // Conservative: only adapt if both thresholds are exceeded
             should_adapt = (call_count > avg_calls * 2.5) && 
                           (memory_ops > avg_memory * 2.0);
             break;
             
         case ADAPT_STRATEGY_BALANCED:
             // Balanced: adapt if either threshold is significantly exceeded
             should_adapt = (call_count > avg_calls * 2.0) || 
                           (memory_ops > avg_memory * 1.8);
             break;
             
         case ADAPT_STRATEGY_AGGRESSIVE:
             // Aggressive: adapt quickly
             should_adapt = (call_count > avg_calls * 1.5) || 
                           (memory_ops > avg_memory * 1.5);
             break;
             
         case ADAPT_STRATEGY_ADAPTIVE:
             // Adaptive: adjust based on current load
             {
                 double load_factor = (double)call_count / avg_calls;
                 double memory_factor = (double)memory_ops / avg_memory;
                 
                 // Weighted combination
                 double score = (load_factor * 0.7) + (memory_factor * 0.3);
                 should_adapt = (score > 2.0);
             }
             break;
             
         default:
             should_adapt = (call_count > avg_calls * 2.0);
             break;
     }
     
     dr_mutex_unlock(g_config_lock);
     return should_adapt;
 }
 
 /**
  * @brief Get overhead factor based on profile mode
  */
 double config_get_overhead_factor(void) {
     dr_mutex_lock(g_config_lock);
     
     double factor = 1.0;
     
     switch (g_config.profile_mode) {
         case PROFILE_MODE_NONE:
             factor = 0.1;  // Minimal overhead
             break;
         case PROFILE_MODE_LIGHT:
             factor = 0.5;  // Light overhead
             break;
         case PROFILE_MODE_FULL:
             factor = 1.0;  // Normal overhead
             break;
         case PROFILE_MODE_AGGRESSIVE:
             factor = 2.0;  // Higher overhead, more data
             break;
     }
     
     dr_mutex_unlock(g_config_lock);
     return factor;
 }
 
 /**
  * @brief Check if verbose logging is enabled
  */
 bool config_is_verbose(void) {
     dr_mutex_lock(g_config_lock);
     bool verbose = g_config.verbose_logging;
     dr_mutex_unlock(g_config_lock);
     
     return verbose;
 }
 
 /**
  * @brief Check if memory tracking is enabled
  */
 bool config_track_memory(void) {
     dr_mutex_lock(g_config_lock);
     bool track = g_config.enable_memory_tracking;
     dr_mutex_unlock(g_config_lock);
     
     return track;
 }
 
 /**
  * @brief Check if branch tracking is enabled
  */
 bool config_track_branches(void) {
     dr_mutex_lock(g_config_lock);
     bool track = g_config.enable_branch_tracking;
     dr_mutex_unlock(g_config_lock);
     
     return track;
 }
 
 /**
  * @brief Get analysis interval in milliseconds
  */
 int config_get_analysis_interval(void) {
     dr_mutex_lock(g_config_lock);
     int interval = g_config.analysis_interval;
     dr_mutex_unlock(g_config_lock);
     
     return interval;
 }
 
 /**
  * @brief Get maximum number of functions to track
  */
 int config_get_max_functions(void) {
     dr_mutex_lock(g_config_lock);
     int max_funcs = g_config.max_functions;
     dr_mutex_unlock(g_config_lock);
     
     return max_funcs;
 }
 
 /**
  * @brief Get hotspot cooldown period in milliseconds
  */
 int config_get_cooldown_ms(void) {
     dr_mutex_lock(g_config_lock);
     int cooldown = g_config.hotspot_cooldown_ms;
     dr_mutex_unlock(g_config_lock);
     
     return cooldown;
 }
 
 /**
  * @brief Check if external patching is allowed
  */
 bool config_allow_external_patch(void) {
     dr_mutex_lock(g_config_lock);
     bool allow = g_config.allow_external_patch;
     dr_mutex_unlock(g_config_lock);
     
     return allow;
 }
 
 /**
  * @brief Update configuration dynamically
  */
 bool config_update(const char *key, const char *value) {
     if (!key || !value) {
         LOG_ERROR("Invalid parameters for config_update");
         return false;
     }
     
     dr_mutex_lock(g_config_lock);
     bool updated = false;
     
     if (strcmp(key, "hotspot_multiplier") == 0) {
         g_config.hotspot_multiplier = parse_double(value, 2.0, 1.0, 10.0);
         updated = true;
     } else if (strcmp(key, "min_calls") == 0) {
         g_config.min_calls = parse_int(value, 50, 1, 10000);
         updated = true;
     } else if (strcmp(key, "analysis_interval") == 0) {
         g_config.analysis_interval = parse_int(value, 500, 10, 60000);
         updated = true;
     } else if (strcmp(key, "enable_memory_tracking") == 0) {
         g_config.enable_memory_tracking = parse_bool(value, true);
         updated = true;
     } else if (strcmp(key, "enable_branch_tracking") == 0) {
         g_config.enable_branch_tracking = parse_bool(value, false);
         updated = true;
     } else if (strcmp(key, "verbose_logging") == 0) {
         g_config.verbose_logging = parse_bool(value, false);
         updated = true;
     } else if (strcmp(key, "profile_mode") == 0) {
         g_config.profile_mode = parse_profile_mode(value);
         updated = true;
     } else if (strcmp(key, "adaptation_strategy") == 0) {
         g_config.adaptation_strategy = parse_adaptation_strategy(value);
         updated = true;
     } else {
         LOG_WARN("Unknown configuration key: %s", key);
     }
     
     if (updated) {
         LOG_INFO("Configuration updated: %s = %s", key, value);
     }
     
     dr_mutex_unlock(g_config_lock);
     return updated;
 }
 
 /**
  * @brief Reset configuration to defaults
  */
 void config_reset(void) {
     dr_mutex_lock(g_config_lock);
     
     // Store current verbose setting to avoid spamming logs
     bool was_verbose = g_config.verbose_logging;
     g_config.verbose_logging = false;
     
     // Reset to defaults
     g_config.hotspot_multiplier = 2.0;
     g_config.min_calls = 50;
     g_config.analysis_interval = 500;
     g_config.enable_memory_tracking = true;
     g_config.enable_branch_tracking = false;
     g_config.allow_external_patch = false;
     g_config.max_functions = 64;
     g_config.memory_threshold_multiplier = 1.5;
     g_config.call_window_size = 1000;
     g_config.hotspot_cooldown_ms = 10000;
     g_config.log_to_file = false;
     g_config.profile_mode = PROFILE_MODE_LIGHT;
     g_config.adaptation_strategy = ADAPT_STRATEGY_CONSERVATIVE;
     
     // Restore verbose setting
     g_config.verbose_logging = was_verbose;
     
     dr_mutex_unlock(g_config_lock);
     
     LOG_INFO("Configuration reset to defaults");
 }
 
 /**
  * @brief Export configuration to string
  */
 char* config_export(void) {
     dr_mutex_lock(g_config_lock);
     
     char *config_str = str_format(
         "DBI_HOTSPOT_MULT=%.2f\n"
         "DBI_MIN_CALLS=%d\n"
         "DBI_ANALYSIS_INTERVAL=%d\n"
         "DBI_TRACK_MEMORY=%d\n"
         "DBI_TRACK_BRANCHES=%d\n"
         "DBI_VERBOSE=%d\n"
         "DBI_ALLOW_EXTERNAL=%d\n"
         "DBI_MAX_FUNCTIONS=%d\n"
         "DBI_MEMORY_THRESHOLD_MULT=%.2f\n"
         "DBI_CALL_WINDOW=%d\n"
         "DBI_COOLDOWN_MS=%d\n"
         "DBI_PROFILE_MODE=%d\n"
         "DBI_ADAPTATION_STRATEGY=%d",
         g_config.hotspot_multiplier,
         g_config.min_calls,
         g_config.analysis_interval,
         g_config.enable_memory_tracking ? 1 : 0,
         g_config.enable_branch_tracking ? 1 : 0,
         g_config.verbose_logging ? 1 : 0,
         g_config.allow_external_patch ? 1 : 0,
         g_config.max_functions,
         g_config.memory_threshold_multiplier,
         g_config.call_window_size,
         g_config.hotspot_cooldown_ms,
         g_config.profile_mode,
         g_config.adaptation_strategy
     );
     
     dr_mutex_unlock(g_config_lock);
     return config_str;
 }
 
 /**
  * @brief Import configuration from string
  */
 bool config_import(const char *config_str) {
     if (!config_str) {
         return false;
     }
     
     // Parse line by line
     char *copy = str_duplicate(config_str);
     if (!copy) {
         return false;
     }
     
     char *saveptr = NULL;
     char *line = strtok_r(copy, "\n", &saveptr);
     bool any_updated = false;
     
     while (line) {
         // Skip comments and empty lines
         if (line[0] == '#' || line[0] == '\0') {
             line = strtok_r(NULL, "\n", &saveptr);
             continue;
         }
         
         // Parse key=value
         char *equals = strchr(line, '=');
         if (equals) {
             *equals = '\0';
             char *key = line;
             char *value = equals + 1;
             
             if (config_update(key, value)) {
                 any_updated = true;
             }
         }
         
         line = strtok_r(NULL, "\n", &saveptr);
     }
     
     dr_global_free(copy, 0);
     
     if (any_updated) {
         LOG_INFO("Configuration imported successfully");
         config_dump();
     }
     
     return any_updated;
 }
 
 /**
  * @brief Cleanup configuration system
  */
 void config_cleanup(void) {
     if (g_config_lock) {
         dr_mutex_destroy(g_config_lock);
         g_config_lock = NULL;
     }
     
     LOG_DEBUG("Configuration system cleaned up");
 }