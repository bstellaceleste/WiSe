# WiSe Hack'25 - Adaptive Engine

*Author:* Lyse Voukeng
## 1. Introduction
Ce projet est ma soumission pour le hackathon *WISE HACK'25*. Il s'agit d'un moteur d'instrumentation binaire dynamique (DBI) construit avec Intel Pin.
L'objectif est de créer un moteur capable d'analyser un programme en cours d'exécution, d'identifier les zones de code critiques ("Hotspots") et de rediriger dynamiquement l'exécution vers des versions optimisées de ces zones, le tout sans jamais arrêter le programme cible.
Mon approche vise la *généralisation* et la *robustesse*, en évitant toute forme de "hardcoding" pour garantir la performance sur le binaire mystère final. Surtout etant donne les contraintes fourni dans les documents de la competition.
Ce projet présente un **moteur d'instrumentation dynamique** conçu pour l'optimisation logicielle en temps réel. Dans un contexte industriel où l'arrêt de service est proscrit (ex: plateformes de *Mobile Money*), ce moteur permet d'analyser le comportement d'un binaire et de le modifier "à chaud" (*Hot-Patching*) pour améliorer ses performances sans recompiler ni redémarrer l'application.

## 2. Architecture du Système

Le moteur est structuré de manière modulaire pour répondre aux critères de rigueur du jury :
* **Module d'Analyse (Phase 1) :** Surveillance de la fréquence d'appel et mesure précise de l'activité de la pile (*Stack Pointer*) via le registre `RSP`.
* **Module d'Adaptation (Phase 2) :** Réécriture dynamique du prologue des fonctions cibles en mémoire vive.
* **Module de Contrôle :** Interface agnostique permettant le traitement de n'importe quel binaire ELF x86_64, incluant le **Binaire Mystère**.

## 3. Prérequis

Pour garantir une compilation et une exécution sans erreur, les éléments suivants sont requis :
* **Système d'exploitation :** Linux x86_64 (obligatoire pour les appels `mprotect` et l'assembleur inline).
* **Compilateur :** `g++` supportant le standard C++17.
* **Outils :** `make` pour l'automatisation du build.

## 4. Installation

L'installation est simplifiée au maximum pour répondre aux contraintes du jury :
```bash
# Cloner le dépôt et entrer dans le dossier
git clone [https://github.com/lysevoukeng2019-beep/wise-hack25-adaptive-engine]
cd adaptive-engine

# Compiler l'intégralité du projet via le Makefile
make
