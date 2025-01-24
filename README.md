# Poker Stats Tracker

Ce programme permet de suivre et d'analyser les statistiques des joueurs en temps réel lors de parties de poker sur PokerStars. Il extrait les données des fichiers d'historique des mains et affiche des métriques clés pour chaque joueur à la table.

## Fonctionnalités principales

- **Suivi des statistiques en temps réel** : Le programme lit les fichiers d'historique des mains et met à jour les statistiques des joueurs à chaque nouvelle main.
- **Métriques clés** :
  - **VPIP** (Voluntarily Put Money In Pot) : Pourcentage de mains où le joueur a mis de l'argent dans le pot volontairement.
  - **PFR** (Pre-Flop Raise) : Pourcentage de mains où le joueur a relancé avant le flop.
  - **RFI** (Raise First In) : Pourcentage de mains où le joueur a été le premier à relancer avant le flop.
  - **3Bet** : Pourcentage de mains où le joueur a relancé une relance pré-flop.
- **Affichage coloré** : Les statistiques sont affichées avec des couleurs pour une meilleure lisibilité.
- **Gestion des joueurs** : Les joueurs sont ajoutés ou retirés automatiquement en fonction de leur présence à la table.
- **Support multi-tables** : Le programme peut gérer plusieurs tables simultanément en suivant les changements de table.

## Utilisation

1. **Sélection de la plateforme** : Le programme supporte actuellement PokerStars.
2. **Sélection du dossier du joueur** : Choisissez le dossier contenant les historiques de mains de PokerStars.
3. **Affichage des résultats** : Les statistiques des joueurs sont mises à jour et affichées toutes les 3 secondes.

## Prérequis

- Système d'exploitation Windows.
- Accès aux fichiers d'historique des mains de PokerStars (généralement situés dans `C:\Users\<user>\AppData\Local\PokerStars.FR\HandHistory\<player_folder>`).

## Compilation

Pour compiler le programme, utilisez un compilateur C compatible avec Windows (comme GCC ou MSVC) et assurez-vous d'inclure les bibliothèques nécessaires.

```bash
gcc main.c -o poker_stats_tracker.exe
```

## Exécution

Exécutez le programme généré (`poker_stats_tracker.exe`) et suivez les instructions à l'écran pour sélectionner la plateforme et le dossier du joueur.

## Remarques

- Le programme est conçu pour être utilisé en temps réel pendant une session de poker.
- Assurez-vous que les fichiers d'historique des mains sont accessibles et à jour.

---

Ce programme est un outil utile pour les joueurs de poker qui souhaitent analyser les tendances et les comportements des autres joueurs à la table.
