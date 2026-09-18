# Emotion Renderer

**Le mur.** Ce que le public voit pendant un set de vinyles : une projection qui suit le son
qui sort réellement des enceintes, composée à partir de ce que le moteur d'analyse publie —
jamais à partir d'un fichier, jamais à partir d'une commande.

Troisième pièce d'un ensemble de trois, et la dernière à être construite :

| pièce | rôle | parle à |
|---|---|---|
| [crate](https://github.com/LiquidSnake0/crate) | la base du bac : BPM joué, clé transposée, couleur, enchaînements possibles | emotion-emulator, par HTTP |
| [emotion-emulator](https://github.com/LiquidSnake0/emotion-emulator) | l'oreille : écoute le master et le cue, sépare les sources, suit le tempo, publie 256 octets par image | ce dépôt, par mémoire partagée |
| **emotion-renderer** | l'œil : lit les 256 octets et compose l'image, une scène par platine | le rétroprojecteur |

Ce dépôt est né le 18 septembre 2026, **avant le matériel**. Il porte tout ce que les deux
autres pièces ont déjà appris et décidé pour lui, pour qu'au premier branchement il n'y ait
rien à redécouvrir. Ce qu'il ne porte pas encore : une seule image rendue sur un GPU.

## Ce qui est déjà décidé, et pourquoi

**Le renderer ne mélange pas deux images, il compose des cases signées.** Le moteur publie
huit cases, chacune marquée de sa platine (1, 2, ou 3 pour le reste partagé). Le rendu met
tout ce qui est P1 dans la scène de gauche, P2 à droite, le reste à cheval sur la frontière.
Quand un disque entre, sa scène s'ouvre pendant que l'autre se resserre ; quand il sort, sa
scène se ferme. **P1 est toujours à gauche de P2**, quelle que soit celle qui joue : rien ne
saute de côté au retrait. C'est mesuré et illustré dans emotion-emulator, sur son mock
(`outils/fenetre.py`), qui est la référence de ce que ce dépôt doit rendre.

**Toute ouverture ou fermeture est une grandeur lissée, jamais un état.** Un cadre qui
passerait de zéro à la moitié de l'écran en une image serait un saut ; l'œil suit le saut et
perd la musique. Même règle pour le curseur de mesure et pour tout repère : on corrige une
fraction, on ne recale jamais d'un coup.

**Un numéro de case n'est pas une identité.** À l'accueil du second disque, le reste passe de
la case 4 à la case 8 et la case 4 devient un gabarit de B. Tout ce que le rendu attache à
une case — un fader, une couleur, un état de geste — doit suivre l'instrument, pas le numéro :
le reste vers le reste, la i-ème case d'une platine vers la i-ème case de la même platine.

**Le son donne le mouvement, la base donne le caractère.** Attaques, énergie, tempo,
hauteurs viennent du signal. Famille de couleur, roue Camelot, pochette viennent du crate,
via le moteur. Le rendu ne détecte rien : il lit.

**Le geste vient du caractère de la source, pas de son nom.** Le GPU « n'a que faire de si
c'est un piano ou un tambour, il a besoin d'information pour modifier des formes ». Par case
il reçoit quand ça frappe, comment ça tient, comment ça enfle, où dans la mesure, quel
degré de la gamme — et le mock a fixé un geste par caractère : la boule du boom et le
triangle du tchak pour le reste, des vagues pour ce qui tient, une corde pour ce qui est
pincé, une onde d'impact pour ce qui frappe.

**Le geste est annoncé par le motif, confirmé par la frappe.** Le bit de frappe arrive 60 à
80 ms après l'attaque ; le motif de la source (seize cases par mesure) est en avance. Quand
la mesure entre dans une case que le motif allume, le geste part ; la frappe qui suit dans
les 150 ms confirme ; une frappe sans annonce déclenche quand même, en retard.

**Le préparé n'atteint jamais le mur.** Ce qui se cale au casque ne se voit pas : sinon le
public verrait la coulisse.

**On jette, on ne bloque jamais.** Une image de 21 ms arrivée en retard n'a aucune valeur.
Le rendu lit la dernière case publiée de l'anneau et ignore ce qu'il a raté.

**Le budget est de 40 ms bout en bout**, le seuil où l'œil cesse de lier une image au son.
L'analyse en prend 21. Le rendu prédit ce qui est périodique (le kick, par une horloge
verrouillée sur la grille publiée) et ne prédit jamais ce qui ne l'est pas.

**Aucun réseau entre le moteur et le rendu.** Le navigateur a été retiré d'emotion-emulator
pour cette raison : la page recevait par WebSocket ce que le rendu lira sur PCIe. Ici le
contrat est un anneau sans verrou en mémoire partagée, 256 octets par image, 1,5 µs à
l'écriture.

## Le contrat

Les 256 octets et l'anneau sont décrits octet par octet dans [`docs/contrat.md`](docs/contrat.md).
Ils viennent de `GpuPacket.cs` et `SharedRing.cs` d'emotion-emulator ; **la source de vérité
est là-bas**, ce fichier en est la copie de travail, à relire à chaque changement de paquet.
La composition des scènes et les gestes sont dans [`docs/scene.md`](docs/scene.md).

## Ce qui existe dans ce dépôt

```
src/anneau.hpp    lire l'anneau partagé sans verrou (et jeter une case relue pendant sa copie)
src/paquet.hpp    décoder les 256 octets, avec les décalages du contrat
src/scene.hpp     composer une scène par platine : ouvertures lissées, cases par tag, reste à cheval
src/main.cpp      la sonde : imprime ce que le rendu recevrait, image par image
```

```sh
make                          # g++ seul, aucune dépendance
./bin/emotion-renderer --sonde 10        # dix secondes de paquets, depuis /dev/shm/emotion-emulator
./outils/rejouer.sh <fondu.pak> 10       # rejoue un fondu enregistré par emotion-emulator, et la sonde le lit
```

**Rien ne dessine encore.** La sonde est là pour prouver une chose avant tout le reste : que
ce dépôt lit exactement ce que le moteur écrit, y compris pendant un relais entre deux
disques. C'est le même contrat que `outils/fenetre.py`, et il se vérifie sur les fondus
enregistrés (`~/.cache/emotion-emulator/relais/*.pak`), rejoués par la sonde du moteur.

## Ce qui n'est pas décidé

- **La bibliothèque graphique.** OpenGL est le choix par défaut sur Linux avec une GTX 1080 ;
  Vulkan n'apporte rien ici tant qu'une image se rend en moins d'une milliseconde. CUDA
  n'est pas nécessaire : la séparation des sources reste sur le CPU, dans le moteur.
- **Le vocabulaire visuel final.** Le mock rend en caractères et en gestes simples, par
  choix de mesure. Le mur pourra rendre autrement, à condition de garder ce que le mock a
  appris : ce qui distingue deux motifs n'est pas leur tracé, c'est leur composition — où la
  matière se trouve dans la case et comment elle bouge.
- **Le matériel.** Le DJ branchera un DDJ-FLX4 pour relier le crate au moteur avant de
  brancher ce rendu. Avec le FLX4, les faders et l'EQ sortent en MIDI : le rendu n'aura pas
  à deviner le fader.

## Feuille de route

1. Relecture d'emotion-emulator, finalisation du crate, DDJ-FLX4 branché — rien ici avant.
2. La sonde lit l'anneau en direct pendant un set rejoué, sans perdre une image.
3. Une fenêtre OpenGL qui rend la composition de `scene.hpp`, comparée image par image au mock.
4. Le mur.
