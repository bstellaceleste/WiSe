# WiSe Hack'25 : Moteur d'Instrumentation Binaire Adaptatif (MIBA)

## 1. Introduction
Ce projet implémente un moteur capable d'analyser, de profiler et de modifier le comportement d'un programme binaire x86_64 en temps réel, sans interruption de service. L'objectif est de répondre à la problématique de la rigidité des logiciels monolithiques par l'usage de l'Instrumentation Binaire Dynamique (DBI).

## 2. Architecture Technique
Le moteur s'appuie sur le framework **Intel PIN 4.0**. Il fonctionne en s'insérant entre le processeur et le binaire cible.

### Phase 1 : Analyse et Profilage
Le moteur surveille chaque routine (`RTN`) chargée en mémoire :
- **Fréquence d'appel** : Un compteur global suit chaque exécution de fonction.
- **Activité Mémoire** : Nous analysons le flux d'instructions pour détecter les écritures dans la pile (`INS_IsStackWrite`), permettant d'identifier les fonctions gourmandes en ressources mémoire.

### Phase 2 : Hot-Patching & Adaptation
Lorsqu'une fonction est identifiée comme **Hotspot** (seuil par défaut : 500 appels), le moteur déclenche une réécriture dynamique :
- Utilisation de `RTN_ReplaceSignature` pour rediriger l'exécution vers une version optimisée.
- Préservation du contexte d'exécution pour garantir que le programme ne crash pas lors de la transition.



## 3. Généralisation (Binaire Mystère)
Le moteur a été conçu pour être totalement agnostique du binaire source :
- **Zéro Hardcoding** : Aucune fonction n'est ciblée par son nom. L'adaptation repose uniquement sur des métriques statistiques collectées en temps réel.
- **Relocalisation dynamique** : Gestion des adresses via les offsets relatifs pour supporter l'ASLR (Address Space Layout Randomization).

## 4. Guide d'Installation et Reproductibilité
### Prérequis
- Système : Kali Linux / Ubuntu x86_64
- Framework : Intel PIN 4.0

### Procédure de Build Rapide
```bash
# 1. Cloner le dépôt
git clone <URL_DU_DEPOT>
cd MyTool

# 2. Configurer le chemin PIN (Ajustez selon votre installation)
export PIN_ROOT=/chemin/vers/pin

# 3. Compiler et Lancer le test automatique
./build.sh
