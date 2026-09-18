# La scène : ce que le rendu compose

Tiré de `emotion-calculator/outils/fenetre.py` (16–17 septembre 2026), qui est la référence.
Ce document dit **quoi**, `src/scene.hpp` dit **comment** en C++.

## Une scène par platine

- Deux scènes, P1 à gauche, P2 à droite. Chacune a une **ouverture** entre 0 et 1, lissée
  vers sa cible avec une constante de temps de 0,4 s (`o += (cible − o) · min(1, dt · 2,5)`),
  posée à la cible quand l'écart passe sous 0,004.
- La cible d'une platine vaut 1 si elle joue (bits 0–1 de l'octet 60), si elle entre pendant
  un fondu (bits 4–5, avec le bit 2), ou si une case active porte son tag. Aucune présente :
  P1 seule ouverte (un moteur d'avant le relais).
- Largeurs : la largeur disponible moins une gouttière si les deux sont ouvertes (> 0,02),
  répartie au prorata des ouvertures.
- Chaque scène a une grille de 4 colonnes × 2 rangs ; les cadres vides restent dessinés,
  c'est la grille de lecture. Les cases de la platine y prennent place dans l'ordre des rangs.
- Le **reste partagé** (tag 3) se dessine **à cheval sur la frontière**, sur le second rang, à
  la largeur moyenne des cases des deux scènes ; sa place est réservée avant de dessiner la
  grille, pour ne recouvrir aucun cadre. Une seule scène ouverte : il rejoint sa grille.
- Au-dessus de chaque scène : « P1 · joue / sort / entre », le pitch de la platine qui entre,
  et une barre = le niveau moyen des cases de la platine, lissé (constante 6 /s) — ce que le
  rendu **sait** du fader.

## Les cases et les gestes

- Le niveau dessiné est le niveau publié interpolé entre deux images d'analyse, fois le gain
  local de la case s'il y en a un ; les événements (frappe) ne s'interpolent jamais.
- Un geste par caractère (octet 201–203 / 126, quartet, 0 à 15 → 0 à 1) :
  - **le reste** (dernière case) : une **boule** qui grossit sur le boom (hauteur < 0,33), un
    **triangle** qui claque sur le tchak ;
  - **tient** (caractère > 0,8) : trois **vagues**, vitesse et amplitude selon le niveau ;
  - **pincé** (0,45 à 0,8) : une **corde** qui vibre au pincement, amortie en 0,45 s ;
  - **frappe** (< 0,45) : un point qui pèse le niveau et une **onde d'impact** de 0,35 s.
- Retombée de l'impulsion : `0,7 + 8 · (1 − tenue)` par seconde ; force de l'attaque
  `0,15 + 0,75 · piqué` ; vitesse de l'onde `3,4 − 2,2 · tenue`.
- **Annonce et confirmation.** Quand le curseur de mesure entre dans une case (1/16) que le
  motif de la source allume, le geste part ; un bit de frappe dans les 150 ms confirme sans
  redéclencher ; un bit sans annonce déclenche en retard. Pour le reste, chaque case de la
  mesure retient ce qu'elle a fait entendre (boom ou tchak).
- Dominance (bits 1–3 du drapeau) : la part extraite en vert, la part discrète en ambre,
  deux points en tête de case. Colorer dit ce qu'on sait ; couper déciderait à la place de l'œil.
- Titre de case : rang, nom, nature (frappé / pincé / tenu), degré (I … VII, · hors gamme) ;
  se coupe à la place que laisse le verdict, en lâchant les mots de la fin et en gardant le
  rang et la platine entiers.

## L'horloge et le curseur

- Le curseur de mesure avance seul à la cadence du temps (octet 117, qui s'enroule) et se
  laisse tirer à 12 % par image vers la phase publiée (24) quand le « 1 » est connu (103 < 4).
  Il ne recule jamais.
- L'horloge du kick se cale sur la phase du temps publiée, se verrouille quand l'accord (119)
  dépasse 0,5 (hystérésis : lâche à 0,25), et déclenche le kick en avance de l'avance réglée
  (30 ms par défaut). Verrouillée, elle prédit ; sinon la frappe détectée fait foi.

## Le report par identité

À chaque changement du nombre de cases ou des tags, tout ce qui est attaché à une case se
reporte : clé `("reste")` pour la dernière case, `(platine, ordinal dans la platine)` pour
les autres ; une case nouvelle part à l'état neutre. Vérifié sur un passage enregistré : un
gain de 0,3 posé sur le kick en case 4 se retrouve en case 8 pendant le chevauchement, puis
en case 5 après le retrait.
