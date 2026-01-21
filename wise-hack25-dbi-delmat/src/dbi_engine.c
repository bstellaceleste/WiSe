/*
 * WiSe Hack'25 - Main DBI Engine Implementation
 * File: src/dbi_engine.c
 * CORRECTED VERSION - Fixed deprecated DynamoRIO APIs
 */
 #define _GNU_SOURCE
 #include "dr_api.h"
 #include "drmgr.h"
 #include "drwrap.h"
 #include "drutil.h"
 #include "dbi_engine.h"
 #include "config.h"
 #include "hotpatch.h"
 #include "utils.h"
 #include <signal.h>
 #include <string.h>
 #include <math.h>
 #include <stdio.h>
 #include <sys/time.h>
 
 /* ========== MICROSECOND TIMESTAMP HELPER ========== */
 static inline uint64_t get_micros(void) {
     struct timeval tv;
     gettimeofday(&tv, NULL);
     return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
 }
 
 /* ========== GLOBAL DATA STRUCTURES ========== */
 
 static global_stats_t *g_stats = NULL;
 static bool g_engine_running = false;
 static void *g_engine_lock = NULL;
 static void *g_analysis_thread = NULL;
 
 /* ========== STATISTICS MANAGEMENT ========== */
 
 static void pre_call_hook(void *wrapcxt, void **user_data) {
    (void)user_data;  // Ignore user_data pour l'instant
    app_pc func_addr = drwrap_get_func(wrapcxt);
    
    // Log seulement pour les premières occurrences
    static uint64_t total_calls = 0;
    total_calls++;
    
    if (total_calls % 50 == 0) {
        LOG_DEBUG("Function call #%llu @ %p", 
                 (unsigned long long)total_calls, func_addr);
    }
    
    profile_count_call(func_addr);
}

 /**
  * @brief Initialize global statistics
  */
 static void stats_init(void) {
     if (!g_stats) {
         g_stats = (global_stats_t *)dr_global_alloc(sizeof(global_stats_t));
         memset(g_stats, 0, sizeof(global_stats_t));
         g_stats->lock = dr_mutex_create();
         
         LOG_DEBUG("Statistics system initialized");
     }
 }
 
 /**
  * @brief Find function by address (internal, assumes lock held)
  */
 static function_stats_t* find_function_by_addr(app_pc addr) {
     if (!g_stats || !addr) {
         return NULL;
     }
     
     for (int i = 0; i < g_stats->func_count; i++) {
         if (g_stats->functions[i].start_addr == addr) {
             return &g_stats->functions[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Find function by name (internal, assumes lock held)
  */
 static function_stats_t* find_function_by_name(const char *name) {
     if (!g_stats || !name) {
         return NULL;
     }
     
     for (int i = 0; i < g_stats->func_count; i++) {
         if (g_stats->functions[i].name && 
             strcmp(g_stats->functions[i].name, name) == 0) {
             return &g_stats->functions[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Get or create function statistics entry
  */
 function_stats_t* profile_get_or_create_function(app_pc addr, const char *name) {
     if (!addr) {
         return NULL;
     }
     
     dr_mutex_lock(g_stats->lock);
     
     // Check if function already exists
     function_stats_t *func = find_function_by_addr(addr);
     if (func) {
         dr_mutex_unlock(g_stats->lock);
         return func;
     }
     
     // Check if we've reached the maximum
     engine_config_t *config = config_get();
     if (g_stats->func_count >= config->max_functions) {
         LOG_WARN("Maximum function limit reached (%d), cannot track: %s", 
                 config->max_functions, name ? name : "unknown");
         dr_mutex_unlock(g_stats->lock);
         return NULL;
     }
     
     // Create new entry
     func = &g_stats->functions[g_stats->func_count];
     memset(func, 0, sizeof(function_stats_t));
     
     func->name = name ? strdup(name) : strdup("unknown");
     func->start_addr = addr;
     func->last_call_time = get_micros();
     func->is_redirected = false;
     func->is_candidate = false;
     
     g_stats->func_count++;
     
     LOG_DEBUG("Now tracking function: %s @ %p (total: %d)", 
              func->name, addr, g_stats->func_count);
     
     dr_mutex_unlock(g_stats->lock);
     return func;
 }
 
 /**
  * @brief Increment function call count
  */
 void profile_count_call(app_pc func_addr) {
     if (!func_addr) return;
     
     dr_mutex_lock(g_stats->lock);
     
     function_stats_t *func = find_function_by_addr(func_addr);
     if (!func) {
         func = profile_get_or_create_function(func_addr, "unknown");
         if (!func) {
             dr_mutex_unlock(g_stats->lock);
             return;
         }
     }
     
     // Update statistics
     func->call_count++;
     func->last_call_time = get_micros();
     g_stats->total_calls++;
     
     // Check for hotspot candidate
     engine_config_t *config = config_get();
     if (!func->is_candidate && g_stats->total_calls >= (uint64_t)config->min_calls) {
         double avg_calls = (double)g_stats->total_calls / g_stats->func_count;
         if (func->call_count > avg_calls * 1.5) {
             func->is_candidate = true;
             LOG_INFO("Hotspot candidate detected: %s (calls: %llu, avg: %.1f)", 
                     func->name, (unsigned long long)func->call_count, avg_calls);
         }
     }
     
     dr_mutex_unlock(g_stats->lock);
 }
 
 /**
  * @brief Count memory operations for a function
  */
 void profile_count_memory(app_pc func_addr, uint num_ops) {
     if (!func_addr || num_ops == 0) {
         return;
     }
     
     dr_mutex_lock(g_stats->lock);
     
     function_stats_t *func = find_function_by_addr(func_addr);
     if (!func) {
         dr_mutex_unlock(g_stats->lock);
         return;
     }
     
     func->memory_ops += num_ops;
     g_stats->total_memory_ops += num_ops;
     
     dr_mutex_unlock(g_stats->lock);
 }
 
 /**
  * @brief Dump all statistics to log
  */
 void profile_dump_stats(void) {
     if (!g_stats || g_stats->func_count == 0) {
         LOG_INFO("No statistics collected yet");
         return;
     }
     
     dr_mutex_lock(g_stats->lock);
     
     LOG_INFO("=== PROFILING STATISTICS ===");
     LOG_INFO("Total functions tracked: %d", g_stats->func_count);
     LOG_INFO("Total calls: %llu", (unsigned long long)g_stats->total_calls);
     LOG_INFO("Total memory operations: %llu", (unsigned long long)g_stats->total_memory_ops);
     LOG_INFO("Analysis cycles: %llu", (unsigned long long)g_stats->analysis_cycles);
     LOG_INFO("");
     
     // Calculate averages
     double avg_calls = g_stats->func_count > 0 ? 
                       (double)g_stats->total_calls / g_stats->func_count : 0;
     double avg_memory = g_stats->func_count > 0 ? 
                        (double)g_stats->total_memory_ops / g_stats->func_count : 0;
     
     LOG_INFO("Average calls per function: %.1f", avg_calls);
     LOG_INFO("Average memory ops per function: %.1f", avg_memory);
     LOG_INFO("");
     
     // Display function details
     for (int i = 0; i < g_stats->func_count; i++) {
         function_stats_t *f = &g_stats->functions[i];
         
         double call_ratio = avg_calls > 0 ? (double)f->call_count / avg_calls : 0;
         double memory_ratio = avg_memory > 0 ? (double)f->memory_ops / avg_memory : 0;
         
         LOG_INFO("Function: %s", f->name);
         LOG_INFO("  Address: %p", f->start_addr);
         LOG_INFO("  Calls: %llu (%.1fx avg)", (unsigned long long)f->call_count, call_ratio);
         LOG_INFO("  Memory ops: %llu (%.1fx avg)", (unsigned long long)f->memory_ops, memory_ratio);
         LOG_INFO("  Status: %s", 
                 f->is_redirected ? "OPTIMIZED" : 
                 f->is_candidate ? "HOTSPOT CANDIDATE" : "MONITORING");
         
         if (f->optimized_addr) {
             LOG_INFO("  Optimized version: %p", f->optimized_addr);
         }
         
         if (f->last_call_time > 0) {
             uint64_t now = get_micros();
             uint64_t time_since = (now - f->last_call_time) / 1000;
             LOG_INFO("  Last called: %llu ms ago", (unsigned long long)time_since);
         }
         
         LOG_INFO("");
     }
     
     LOG_INFO("=============================");
     
     dr_mutex_unlock(g_stats->lock);
 }
 
 /* ========== HOTSPOT DETECTION ========== */
 
 /**
  * @brief Check if a function is a hotspot
  */
  bool adaptation_is_hotspot(function_stats_t *func) {
    if (!func || func->is_redirected) {
        return false;
    }
    
    // PAS DE LOCK - Le thread d'analyse a déjà locké
    engine_config_t *config = config_get();
    
    if (g_stats->total_calls < (uint64_t)config->min_calls) {
        return false;
    }
    
    double avg_calls = (double)g_stats->total_calls / g_stats->func_count;
    double avg_memory = (double)g_stats->total_memory_ops / g_stats->func_count;
    
    uint64_t now = get_micros();
    uint64_t time_since_last = (now - func->last_call_time) / 1000;
    bool is_active = time_since_last < (uint64_t)config->hotspot_cooldown_ms;
    
    bool should_adapt = config_should_adapt_function(
        func->call_count, func->memory_ops, avg_calls, avg_memory
    );
    
    bool is_hotspot = should_adapt && is_active;
    
    if (is_hotspot && config_is_verbose()) {
        LOG_DEBUG("Hotspot analysis for %s:", func->name);
        LOG_DEBUG("  Calls: %llu (avg: %.1f)", 
                 (unsigned long long)func->call_count, avg_calls);
    }
    
    return is_hotspot;
}
 
 /**
  * @brief Find optimized version of a function
  */
 app_pc adaptation_find_optimized(const char *original_name) {
     if (!original_name) {
         return NULL;
     }
     
     // Construct optimized function name
     char opt_name[256];
     snprintf(opt_name, sizeof(opt_name), "%s_optimized", original_name);
     
     // Search in all loaded modules using iterator
     dr_module_iterator_t *iter = dr_module_iterator_start();
     while (dr_module_iterator_hasnext(iter)) {
         module_data_t *mod = dr_module_iterator_next(iter);
         
         app_pc addr = (app_pc)dr_get_proc_address(mod->handle, opt_name);
         if (addr) {
             LOG_DEBUG("Found optimized function %s in %s @ %p", 
                      opt_name, dr_module_preferred_name(mod), addr);
             dr_free_module_data(mod);
             dr_module_iterator_stop(iter);
             return addr;
         }
         
         dr_free_module_data(mod);
     }
     dr_module_iterator_stop(iter);
     
     LOG_DEBUG("Optimized function %s not found", opt_name);
     return NULL;
 }
 
 /**
  * @brief Redirect a function to its optimized version
  */
 bool adaptation_redirect_function(function_stats_t *func) {
     if (!func || func->is_redirected) {
         return false;
     }
     
     // Find optimized version if not already done
     if (!func->optimized_addr) {
         func->optimized_addr = adaptation_find_optimized(func->name);
         if (!func->optimized_addr) {
             LOG_WARN("No optimized version found for %s", func->name);
             return false;
         }
     }
     
     LOG_INFO("Redirecting %s: %p -> %p", 
              func->name, func->start_addr, func->optimized_addr);
     
     // Apply hotpatch
     if (hotpatch_redirect(func->start_addr, func->optimized_addr)) {
         func->is_redirected = true;
         
         LOG_INFO("Successfully redirected %s", func->name);
         
         if (config_is_verbose()) {
             LOG_DEBUG("Redirect details:");
             LOG_DEBUG("  Original calls: %llu", (unsigned long long)func->call_count);
             LOG_DEBUG("  Memory operations: %llu", (unsigned long long)func->memory_ops);
             LOG_DEBUG("  Analysis cycles before adaptation: %llu", 
                      (unsigned long long)g_stats->analysis_cycles);
         }
         
         return true;
     }
     
     LOG_ERROR("Failed to redirect %s", func->name);
     return false;
 }
 
 /* ========== ANALYSIS THREAD ========== */
 
 /**
  * @brief Periodic analysis and adaptation thread
  */

static thread_id_t g_analysis_thread_id = 0;  // 0 = invalid/not running

/**
 * @brief Thread d'analyse asynchrone
 * Signature : void (au lieu de void*) pour correspondre à dr_create_client_thread
 */
 static void analysis_thread_func(void *arg) {
    (void)arg;
    engine_config_t *config = config_get();
    char func_name_buffer[256]; // Buffer local pour sécuriser le nom

    LOG_INFO("Analysis thread started");

    while (g_engine_running) {
        dr_thread_yield(); // Cède le passage
        dr_sleep(config->analysis_interval);

        if (!g_engine_running) break;

        dr_mutex_lock(g_stats->lock);
        g_stats->analysis_cycles++;
        
        for (int i = 0; i < g_stats->func_count; i++) {
            function_stats_t *func = &g_stats->functions[i];
            
            // Ignorer si déjà patché
            if (func->is_redirected) continue;
            
            // --- LOGIQUE DE DÉTECTION HOTSPOT ---
            bool is_hot = false;
            uint64_t current_calls = func->call_count;

            if (g_stats->func_count <= 1) {
                // Si une seule fonction : on compare juste au seuil minimum
                is_hot = (current_calls >= (uint64_t)config->min_calls);
            } else {
                // Si plusieurs fonctions : on applique le multiplicateur de moyenne
                double avg_calls = (double)g_stats->total_calls / g_stats->func_count;
                is_hot = (current_calls >= (uint64_t)config->min_calls &&
                          current_calls > (avg_calls * config->hotspot_multiplier));
            }

            if (is_hot) {
                // Étape 1 : Recherche de la version optimisée
                if (!func->optimized_addr && func->name) {
                    // Sauvegarde du nom et déverrouillage pour éviter les deadlocks
                    strncpy(func_name_buffer, func->name, sizeof(func_name_buffer) - 1);
                    func_name_buffer[sizeof(func_name_buffer) - 1] = '\0';
                    
                    dr_mutex_unlock(g_stats->lock);
                    app_pc opt_addr = adaptation_find_optimized(func_name_buffer);
                    dr_mutex_lock(g_stats->lock);
                    
                    func->optimized_addr = opt_addr;
                }
                
                // Étape 2 : Application du Hotpatch
                if (func->optimized_addr) {
                    app_pc start_addr = func->start_addr;
                    app_pc target_addr = func->optimized_addr;

                    // On relâche le verrou global pendant l'écriture mémoire du patch
                    dr_mutex_unlock(g_stats->lock);
                    bool success = hotpatch_redirect(start_addr, target_addr);
                    dr_mutex_lock(g_stats->lock);
                    
                    if (success) {
                        func->is_redirected = true;
                        LOG_INFO("🔥 ADAPTED: %s (calls: %llu)", 
                                func->name, (unsigned long long)current_calls);
                    }
                }
            }
        }
        dr_mutex_unlock(g_stats->lock);
    }

    LOG_INFO("Analysis thread terminated");
}

static void start_analysis_thread(void) {
    if (g_analysis_thread_id != 0) {
        LOG_WARN("Analysis thread already running");
        return;
    }

    thread_id_t tid = dr_create_client_thread(analysis_thread_func, NULL);
    if (tid == 0) {
        LOG_ERROR("Failed to create analysis thread");
    } else {
        g_analysis_thread_id = tid;
        LOG_INFO("Analysis thread launched (TID: %lu)", (unsigned long)tid);
    }
}

/* Pas besoin de stop_analysis_thread → le thread vérifie g_engine_running */
 
 /**
  * @brief Stop the analysis thread
  */
 static void stop_analysis_thread(void) {
     if (!g_analysis_thread) {
         return;
     }
     
     LOG_DEBUG("Stopping analysis thread...");
     
     // Signal thread to stop
     g_engine_running = false;
     
     // Wait for thread to finish (using sleep workaround)
     dr_sleep(1000);  // Wait 1 second for thread to finish
     g_analysis_thread = NULL;
     
     LOG_DEBUG("Analysis thread stopped");
 }
 
 /* ========== INSTRUMENTATION CALLBACKS ========== */
 


 /**
  * @brief Module load callback
  */
  void module_load_callback(void *drcontext, const module_data_t *info, bool loaded) {
    (void)drcontext; // Supprime le warning unused parameter
    (void)loaded;    // Supprime le warning unused parameter

    // 1. On ne cible que les modules qui nous intéressent
    const char *mod_name = dr_module_preferred_name(info);
    
    if (mod_name != NULL && strstr(mod_name, "wise_") != NULL) {
        LOG_INFO("Module cible détecté : %s (Base: %p)", mod_name, info->start);

        // 2. Chercher l'adresse de la fonction 'process_transaction'
        app_pc func_addr = (app_pc)dr_get_proc_address(info->handle, "process_transaction");
        
        if (func_addr != NULL) {
            // 3. Poser le "wrapper" (hook)
            if (drwrap_wrap(func_addr, pre_call_hook, NULL)) {
                LOG_INFO("✅ Hook posé sur 'process_transaction' à l'adresse %p", func_addr);
                
             
                profile_get_or_create_function(func_addr, "process_transaction");
            } else {
                LOG_ERROR("❌ Échec de la pose du hook sur 'process_transaction'");
            }
        } else {
            LOG_WARN("Symbole 'process_transaction' non trouvé dans %s", mod_name);
        }

        // 4. Chercher la version optimisée
        app_pc opt_addr = (app_pc)dr_get_proc_address(info->handle, "process_transaction_optimized");
        if (opt_addr != NULL) {
            LOG_INFO("Version optimisée détectée à l'adresse %p", opt_addr);
        }
    }
}
 

  /**
 * @brief Callback d'instrumentation (Phase de traduction)
 * Rôle : Préparer le code pour DynamoRIO. 
 * On laisse cette fonction vide pour éviter tout overhead inutile lors de la traduction.
 */
dr_emit_flags_t instrumentation_callback(void *drcontext, void *tag, instrlist_t *bb, 
    instr_t *inst, bool for_trace, 
    bool translating, void *user_data) {
(void)drcontext; (void)tag; (void)bb; (void)inst;
(void)for_trace; (void)translating; (void)user_data;

// Ne rien faire ici permet de garder des performances maximales (Phase 1).
return DR_EMIT_DEFAULT;
}
 /* ========== ENGINE MANAGEMENT ========== */
 
 /**
  * @brief Initialize the DBI engine
  */
 void engine_init(void) {
     if (g_engine_running) {
         LOG_WARN("Engine already initialized");
         return;
     }
     
     // Initialize subsystems
     utils_init();
     config_init();
     hotpatch_init();
     
     // Load configuration
     config_from_env();
     
     if (config_get()->verbose_logging) {
        log_set_level(LOG_LEVEL_DEBUG);
    }
     // Initialize statistics
     stats_init();
     
     // Create engine lock
     if (!g_engine_lock) {
         g_engine_lock = dr_mutex_create();
     }
     
     // Set running flag
     g_engine_running = true;
     
     // Start analysis thread
    start_analysis_thread();
     
     LOG_INFO("WiSe Hack'25 DBI Engine initialized");
     LOG_INFO("PID: %d, Process: %s", 
              dr_get_process_id(), 
              dr_get_application_name());
     
     if (config_is_verbose()) {
         profile_dump_stats();
     }
 }
 

 static dr_signal_action_t signal_event(void *drcontext, dr_siginfo_t *info) {
    (void)drcontext;
    if (info->sig == SIGINT) {
        LOG_INFO("SIGINT reçu - sortie propre demandée");
        profile_dump_stats();
        hotpatch_list_patches();
        hotpatch_rollback_all();
        dr_exit_process(0);  // Sortie propre → appelle engine_exit()
        return DR_SIGNAL_SUPPRESS;
    }
    return DR_SIGNAL_DELIVER;
}
 /**
  * @brief Cleanup the DBI engine
  */
 void engine_exit(void) {
     if (!g_engine_running) {
         return;
     }
     
     LOG_INFO("DBI Engine shutdown initiated");
     
     // Stop analysis thread
     stop_analysis_thread();
     
     // Final statistics dump
     LOG_INFO("=== FINAL STATISTICS ===");
     profile_dump_stats();
     hotpatch_list_patches();
     
     // Calculate overhead
     if (g_stats && g_stats->analysis_cycles > 0) {
         double overhead_factor = config_get_overhead_factor();
         LOG_INFO("Estimated overhead: %.1f%%", overhead_factor * 100);
     }
     
     // Cleanup subsystems
     drwrap_exit();
     hotpatch_exit();
     config_cleanup();
     utils_cleanup();
     
     // Cleanup statistics
     if (g_stats) {
         for (int i = 0; i < g_stats->func_count; i++) {
             if (g_stats->functions[i].name) {
                 dr_global_free((void *)g_stats->functions[i].name, 0);
             }
         }
         
         dr_mutex_destroy(g_stats->lock);
         dr_global_free(g_stats, sizeof(global_stats_t));
         g_stats = NULL;
     }
     
     // Cleanup engine lock
     if (g_engine_lock) {
         dr_mutex_destroy(g_engine_lock);
         g_engine_lock = NULL;
     }
     
     g_engine_running = false;
     
     LOG_INFO("DBI Engine shutdown complete");
 }
 
 /* Additional functions omitted for brevity - they remain the same */
 bool engine_is_running(void) { return g_engine_running; }
 global_stats_t* engine_get_stats(void) { return g_stats; }
 bool engine_force_adaptation(const char *func_name) {
     if (!func_name || !g_stats) return false;
     dr_mutex_lock(g_stats->lock);
     function_stats_t *func = find_function_by_name(func_name);
     if (!func) {
         LOG_ERROR("Function %s not found", func_name);
         dr_mutex_unlock(g_stats->lock);
         return false;
     }
     if (func->is_redirected) {
         LOG_WARN("Function %s already redirected", func_name);
         dr_mutex_unlock(g_stats->lock);
         return true;
     }
     bool success = adaptation_redirect_function(func);
     dr_mutex_unlock(g_stats->lock);
     return success;
 }
 bool engine_rollback_all(void) {
     LOG_INFO("Rolling back all adaptations");
     bool success = hotpatch_rollback_all();
     if (success && g_stats) {
         dr_mutex_lock(g_stats->lock);
         for (int i = 0; i < g_stats->func_count; i++) {
             g_stats->functions[i].is_redirected = false;
         }
         dr_mutex_unlock(g_stats->lock);
         LOG_INFO("All adaptations rolled back successfully");
     }
     return success;
 }
 function_stats_t* engine_get_function(const char *func_name) {
     if (!func_name || !g_stats) return NULL;
     dr_mutex_lock(g_stats->lock);
     function_stats_t *func = find_function_by_name(func_name);
     dr_mutex_unlock(g_stats->lock);
     return func;
 }
 function_stats_t* engine_get_function_by_addr(app_pc addr) {
     if (!addr || !g_stats) return NULL;
     dr_mutex_lock(g_stats->lock);
     function_stats_t *func = find_function_by_addr(addr);
     dr_mutex_unlock(g_stats->lock);
     return func;
 }
 
 /* ========== DYNAMORIO ENTRY POINT ========== */
 
 DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
     (void)id;  // Unused
     
     dr_set_client_name("WiSe Hack'25 Adaptive DBI Engine",
                       "https://github.com/wise-hack-25/adaptive-dbi");
     
     LOG_DEBUG("DBI Engine starting with %d arguments", argc);
     for (int i = 0; i < argc; i++) {
         LOG_DEBUG("Arg[%d]: %s", i, argv[i]);
     }
     
     // Initialize DynamoRIO managers
     if (!drmgr_init()) {
         LOG_ERROR("Failed to initialize DynamoRIO manager");
         return;
     }
     
     if (!drwrap_init()) {
         LOG_ERROR("Failed to initialize drwrap");
         drmgr_exit();
         return;
     }
     
     // Register event callbacks - USING DRMGR APIs
     drmgr_register_thread_init_event(NULL);
     drmgr_register_thread_exit_event(NULL);
     drmgr_register_bb_instrumentation_event(NULL, instrumentation_callback, NULL);
     drmgr_register_module_load_event(module_load_callback);
     drmgr_register_signal_event(signal_event);
     
     // Initialize and start the engine
     engine_init();

     
     // Register exit event
    
     drmgr_register_exit_event(engine_exit);
     
     LOG_INFO("=== WiSe Hack'25 DBI Engine Ready ===");
     LOG_INFO("Instrumentation active - monitoring application");
     LOG_INFO("Press Ctrl+C to stop and see final statistics");
     LOG_INFO("======================================");
 }