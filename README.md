# MixWaves Squeeze

Compresseur de studio stéréo à détecteur, avec l'interface et les fonctions d'un compresseur de studio classique.

## Commandes
| Commande | Rôle |
|---|---|
| Input | Gain d'entrée (0 à +36 dB). Le seuil est fixe : plus on monte Input, plus on comprime |
| Attack | 0,05 ms à 150 ms |
| Release | 50 ms à 3,5 s |
| Output | Gain de sortie (-20 à +20 dB) |
| Mix | Mélange signal sec / signal comprimé (compression parallèle) |
| RATIO (bouton) | 1:1, 2:1, 3:1, 4:1, 6:1, 10:1, Opto, 20:1, Nuke |
| DETECTOR (bouton) | Filtre passe-haut dans le détecteur, bosse de présence, ou les deux |
| Link | Liaison stéréo du détecteur |
| AUDIO (bouton) | Distorsion Dist 2 (harmoniques paires), Dist 3 (harmoniques impaires), ou les deux |
| HP (Audio) | Passe-haut sur le signal audio |
| HR | Headroom : adoucit la distorsion |
| BY PASS / POWER | Contournement et marche / arrêt |
| LED | Réduction de gain (1 à 26 dB), REDLINE et 1% THD pour le niveau de sortie |

Les LED du détecteur, de Link, de Dist 2 et de Dist 3 sont aussi cliquables.

## Compiler sur GitHub
1. Crée un dépôt GitHub et envoie tous ces fichiers (y compris le dossier `.github`).
2. Onglet **Actions** : le build démarre automatiquement (Windows, macOS, Linux).
3. Quand c'est vert, télécharge le VST3 dans **Artifacts**, en bas de la page du build.

## Installer
- Windows : copie le dossier `.vst3` dans `C:\Program Files\Common Files\VST3`
- macOS : `~/Library/Audio/Plug-Ins/VST3` (ou `Components` pour l'AU)

## Compiler en local
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
