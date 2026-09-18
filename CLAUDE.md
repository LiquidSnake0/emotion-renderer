# Emotion Renderer

L'œil du système : lit les 256 octets qu'emotion-calculator publie dans `/dev/shm/emotion-emulator`
et compose l'image projetée. Troisième pièce après crate et emotion-calculator ; **on n'y touche
qu'après la relecture d'e-e, la finalisation du crate et le branchement du DDJ-FLX4** — c'est
l'ordre que le DJ a fixé le 17 septembre 2026.

## La source de vérité est ailleurs

- Le contrat (paquet, anneau) vit dans `emotion-calculator/src/Emotion.Signal/GpuPacket.cs` et
  `SharedRing.cs`. `docs/contrat.md` et `src/paquet.hpp` en sont des copies : **relire les
  deux à chaque changement du paquet**, et ne jamais inventer un octet ici.
- Le rendu de référence est `emotion-calculator/outils/fenetre.py`. Ce qu'il fait, ce dépôt
  doit le faire pareil avant de faire mieux.
- Les règles du domaine sont dans le `CLAUDE.md` d'emotion-calculator (le son donne le
  mouvement, la base le caractère ; le préparé n'atteint jamais le mur ; on jette, on ne
  bloque jamais ; ne rien dire plutôt que dire faux ; les enums partent par leur nom).

## Décisions à ne pas défaire sans en parler

- Une scène par platine, composée sur le **tag de platine** des cases, jamais sur
  « joue / entre » (qui changent de sens au retrait). P1 à gauche de P2, toujours.
- Toute ouverture, fermeture, ou déplacement de repère est **lissé** ; rien ne saute.
- **Un numéro de case n'est pas une identité** : reporter par identité (le reste vers le
  reste, la i-ème case d'une platine vers la i-ème) tout ce qui s'attache à une case.
- Une impulsion se consomme **une fois par image d'analyse**, jamais par image de rendu
  (47 images par seconde côté signal, 60 à 120 côté écran).
- Le geste est annoncé par le motif, confirmé par la frappe ; un descripteur s'interpole
  entre deux images d'analyse, un événement jamais.
- Aucun réseau. L'anneau, et rien d'autre.
- Le réglage appartient à ce qui affiche (avance, gain d'une case) ; il n'entre pas dans le
  paquet.

## Façon de travailler

Mesurer avant de construire, et écrire la mesure dans le commit. Une chose à la fois, et
demander avant de coder quand deux voies existent. Pas de dépendance sans le dire : la sonde
se compile avec `g++` seul, et c'est voulu. Commits petits, en français sans accents, sans
aucune signature d'IA. Le dépôt est public et c'est une vitrine : le README défend chaque
choix, y compris ce qui n'est pas décidé.
