#!/usr/bin/env python3
"""
Version avancée du moteur DBI avec métriques étendues
"""

import frida
import sys
import time
import threading
import json
from collections import defaultdict, deque

class AdvancedDBIEngine:
    def __init__(self, target_binary):
        self.target_binary = target_binary
        self.session = None
        self.script = None
        
        # Métriques avancées
        self.function_calls = defaultdict(int)
        self.memory_operations = defaultdict(lambda: {'reads': 0, 'writes': 0})
        self.call_history = deque(maxlen=1000)  # Historique des appels
        self.execution_times = defaultdict(list)
        
        # Seuils adaptatifs
        self.base_threshold = 30
        self.adaptive_threshold = self.base_threshold
        self.redirected_functions = set()
        
        # Statistiques
        self.start_time = time.time()
        self.total_redirections = 0
        
    def calculate_adaptive_threshold(self):
        """Calcul du seuil adaptatif basé sur l'activité globale"""
        if len(self.call_history) < 10:
            return self.base_threshold
            
        # Analyse de la fréquence d'appels récente
        recent_calls = list(self.call_history)[-50:]
        unique_functions = len(set(recent_calls))
        
        # Plus il y a de diversité, plus on augmente le seuil
        if unique_functions > 3:
            self.adaptive_threshold = min(self.base_threshold * 2, 100)
        else:
            self.adaptive_threshold = max(self.base_threshold // 2, 10)
            
        return self.adaptive_threshold
    
    def on_message(self, message, data):
        if message['type'] == 'send':
            payload = message['payload']
            
            if payload['type'] == 'function_call':
                func_name = payload['function']
                self.function_calls[func_name] += 1
                self.call_history.append(func_name)
                
                # Calcul du seuil adaptatif
                current_threshold = self.calculate_adaptive_threshold()
                
                # Détection de hotspot
                if (self.function_calls[func_name] >= current_threshold and 
                    func_name not in self.redirected_functions and
                    func_name == 'process_transaction'):
                    self.trigger_intelligent_redirection(func_name)
                    
            elif payload['type'] == 'memory_intensive':
                func_name = payload['function']
                operation = payload['operation']
                self.memory_operations[func_name][operation] += 1
                
            elif payload['type'] == 'performance_data':
                func_name = payload['function']
                exec_time = payload['time']
                self.execution_times[func_name].append(exec_time)
    
    def trigger_intelligent_redirection(self, func_name):
        """Redirection intelligente avec analyse de performance"""
        call_count = self.function_calls[func_name]
        threshold = self.adaptive_threshold
        
        print(f"\n🔥 [HOTSPOT CRITIQUE DÉTECTÉ]")
        print(f"   Fonction: {func_name}")
        print(f"   Appels: {call_count} (seuil: {threshold})")
        print(f"   Temps écoulé: {time.time() - self.start_time:.1f}s")
        
        # Analyse de performance si disponible
        if func_name in self.execution_times and self.execution_times[func_name]:
            avg_time = sum(self.execution_times[func_name]) / len(self.execution_times[func_name])
            print(f"   Temps moyen d'exécution: {avg_time:.2f}ms")
        
        print(f"🚀 [REDIRECTION ACTIVÉE] vers {func_name}_optimized")
        
        # Commande de redirection
        self.script.post({
            'type': 'redirect_function',
            'source': func_name,
            'target': f'{func_name}_optimized'
        })
        
        self.redirected_functions.add(func_name)
        self.total_redirections += 1
    
    def create_advanced_script(self):
        return """
        // Variables globales pour le monitoring
        var function_timers = {};
        var memory_access_count = 0;
        
        // Hook principal de process_transaction avec timing
        var process_transaction_addr = Module.findExportByName(null, "process_transaction");
        var process_transaction_optimized_addr = Module.findExportByName(null, "process_transaction_optimized");
        
        if (process_transaction_addr) {
            Interceptor.attach(process_transaction_addr, {
                onEnter: function(args) {
                    this.start_time = Date.now();
                    this.transaction_id = args[0].toInt32();
                    
                    send({
                        type: 'function_call',
                        function: 'process_transaction',
                        args: [this.transaction_id]
                    });
                },
                onLeave: function(retval) {
                    var exec_time = Date.now() - this.start_time;
                    send({
                        type: 'performance_data',
                        function: 'process_transaction',
                        time: exec_time
                    });
                }
            });
        }
        
        // Monitoring mémoire pour check_memory_integrity
        var check_memory_addr = Module.findExportByName(null, "check_memory_integrity");
        if (check_memory_addr) {
            Interceptor.attach(check_memory_addr, {
                onEnter: function(args) {
                    send({
                        type: 'function_call',
                        function: 'check_memory_integrity'
                    });
                    
                    // Simulation de détection d'accès mémoire intensif
                    memory_access_count++;
                    if (memory_access_count % 10 === 0) {
                        send({
                            type: 'memory_intensive',
                            function: 'check_memory_integrity',
                            operation: 'reads'
                        });
                    }
                }
            });
        }
        
        // Hook validate_logic avec détection de branches
        var validate_logic_addr = Module.findExportByName(null, "validate_logic");
        if (validate_logic_addr) {
            Interceptor.attach(validate_logic_addr, {
                onEnter: function(args) {
                    send({
                        type: 'function_call',
                        function: 'validate_logic'
                    });
                }
            });
        }
        
        // Gestionnaire de redirection
        recv(function(message) {
            if (message.type === 'redirect_function') {
                if (message.source === 'process_transaction' && process_transaction_optimized_addr) {
                    
                    // Remplacement complet de la fonction
                    Interceptor.replace(process_transaction_addr, new NativeCallback(function(id) {
                        console.log("⚡ [OPTIMIZED EXECUTION] ID:", id, "- Version rapide activée");
                        
                        // Appel de la version optimisée
                        var optimized_func = new NativeFunction(process_transaction_optimized_addr, 'int', ['int']);
                        var result = optimized_func(id);
                        
                        // Log périodique pour confirmer la redirection
                        if (id % 50 === 0) {
                            console.log("✅ [REDIRECT SUCCESS] Transaction", id, "traitée par version optimisée");
                        }
                        
                        return result;
                    }, 'int', ['int']));
                    
                    console.log("🎯 [REDIRECTION COMPLÈTE] process_transaction -> process_transaction_optimized");
                }
            }
        });
        """
    
    def start_advanced_monitoring(self):
        try:
            pid = frida.spawn([self.target_binary])
            self.session = frida.attach(pid)
            
            script_code = self.create_advanced_script()
            self.script = self.session.create_script(script_code)
            self.script.on('message', self.on_message)
            self.script.load()
            
            print(f"🚀 [DBI ENGINE AVANCÉ] Monitoring sur {self.target_binary}")
            print(f"📊 [SEUIL ADAPTATIF] Initial: {self.base_threshold}")
            sys.stdout.flush()  # Forcer l'affichage
            
            frida.resume(pid)
            self.start_advanced_metrics()
            
            # Mode automatique pour les tests
            if len(sys.argv) > 2 and sys.argv[2] == "--auto":
                print("\n🤖 [MODE AUTO] Test automatique - arrêt dans 30 secondes...")
                # Forcer l'affichage immédiat
                sys.stdout.flush()
                time.sleep(30)
            else:
                input("\n⏸️  [CONTRÔLE] Appuyez sur Entrée pour arrêter le monitoring...\n")
            
        except Exception as e:
            print(f"❌ [ERREUR CRITIQUE] {e}")
        finally:
            if self.session:
                self.session.detach()
    
    def start_advanced_metrics(self):
        def display_advanced_metrics():
            while True:
                time.sleep(3)
                if self.function_calls:
                    print(f"\n📈 === MÉTRIQUES TEMPS RÉEL (t={time.time()-self.start_time:.1f}s) ===")
                    
                    for func, count in sorted(self.function_calls.items(), key=lambda x: x[1], reverse=True):
                        status = " 🔄 [REDIRECTED]" if func in self.redirected_functions else ""
                        
                        # Calcul de la fréquence
                        freq = count / (time.time() - self.start_time) if time.time() - self.start_time > 0 else 0
                        
                        print(f"  {func}: {count} appels ({freq:.1f}/s){status}")
                        
                        # Affichage des métriques mémoire si disponibles
                        if func in self.memory_operations:
                            mem_ops = self.memory_operations[func]
                            if mem_ops['reads'] > 0 or mem_ops['writes'] > 0:
                                print(f"    💾 Mémoire: {mem_ops['reads']} lectures, {mem_ops['writes']} écritures")
                    
                    print(f"🎯 Seuil adaptatif actuel: {self.adaptive_threshold}")
                    print(f"🔄 Total redirections: {self.total_redirections}")
                    print("=" * 50)
        
        thread = threading.Thread(target=display_advanced_metrics, daemon=True)
        thread.start()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 advanced_dbi.py <binary> [--auto]")
        sys.exit(1)
    
    engine = AdvancedDBIEngine(sys.argv[1])
    engine.start_advanced_monitoring()