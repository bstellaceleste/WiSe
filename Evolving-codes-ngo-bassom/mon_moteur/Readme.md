# WiSe Hack'25 - Moteur d'Instrumentation Adaptatif

## Description
Moteur d'instrumentation binaire basé sur Intel PIN capable de :
1. Profiler la fréquence d'appel des fonctions.
2. Détecter l'activité intensive de la pile (Stack).
3. Effectuer un Hot-Patching dynamique (Phase 2) via `RTN_ReplaceSignature` sans arrêter le binaire.

## Installation & Build
1. Extraire Intel PIN dans `~/Bureau/wise_hack/`.
2. Placer les fichiers du projet dans `$PIN_ROOT/source/tools/MyTool/`.
3. Rendre le script exécutable : `chmod +x build.sh`.
4. Lancer : `./build.sh`.

## Fonctionnement
- **Phase 1** : Le moteur compte chaque instruction d'écriture pile.
- **Phase 2** : Au-delà de 500 appels, la fonction est remplacée dynamiquement par une version optimisée.
- **Généralisation** : Le moteur utilise l'API RTN de PIN pour s'adapter à n'importe quel binaire ELF x86_64.