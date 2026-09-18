# Le contrat : l'anneau et les 256 octets

Copie de travail de `GpuPacket.cs` et `SharedRing.cs` (emotion-calculator). **La source de
vérité est là-bas.** Relevé le 18 septembre 2026.

## L'anneau partagé

Fichier `/dev/shm/emotion-emulator`, mappé en lecture. Petit-boutien.

| décalage | taille | contenu |
|---|---|---|
| 0 | u32 | magie `0x454D5230` (« EMR0 ») |
| 4 | i32 | capacité (puissance de deux, nombre de cases) |
| 8 | i32 | taille d'une case : 256 |
| 64 | i64 | curseur d'écriture, sur sa propre ligne de cache |
| 128 | i64 | curseur de lecture (informatif, écrit par un lecteur) |
| 192 | — | première case ; case k à `192 + (k & (capacité − 1)) × 256` |

Le producteur écrit la case puis avance le curseur : la case `écrit − 1` est complète par
construction. **Constater qu'une case est valide ne suffit pas** : entre la vérification et
la fin de la copie, le producteur peut avoir fait un tour complet. On relit le curseur après
la copie et on jette la copie s'il a dépassé de plus d'un tour. Cette course a dormi des mois
dans le moteur et n'est apparue que le jour où le message a grossi ; elle se corrige quand
on l'écrit, pas quand on la voit.

## Les 256 octets d'une image

| décalage | type | nom | sens |
|---|---|---|---|
| 4 | u32 | séquence | numéro d'image, pour dédupliquer et compter les trous |
| 8 | i64 | temps | ms depuis le départ du moteur |
| 16 | f32 | niveau | énergie globale, 0 à 1 |
| 20 | f32 | bpm | tempo publié ; **0 quand le moteur ne sait pas** (ne rien dire plutôt que dire faux) |
| 24 | f32 | phase | position dans la mesure de quatre temps, 0 à 1 ; **0 tant que le « 1 » est inconnu** |
| 40 | u8 | frappes | bit 0 kick, bit 1 clap, bit 2 charley |
| 47 | u8 | nouveauté | 0 à 255 |
| 48–59 | 12 × u8 | bandes | énergie par bande, grave à aigu (douze octets, plus douze flottants) |
| 60 | u8 | relais | bits 0–1 platine qui joue, bit 2 fondu en cours, bits 4–5 platine qui entre |
| 61 | u8 | accord de gamme | accord du chromagramme avec la gamme de la fiche |
| 62–95 | — | libres | |
| 96–98 | 3 × u8 | voix | grave, médium, aigu |
| 99 | u8 | coups | bit 0 coup grave |
| 100–102 | 3 × u8 | timbre | brillance (centroïde), ouverture, densité |
| 103 | u8 | temps | 0 à 3 dans la mesure, ≥ 4 si inconnu |
| 106 | u8 | montée | |
| 107 | u8 | structure | bit 0 rupture |
| 117 | u8 | phase du temps | position dans le temps courant, **toujours remplie** : c'est elle qui nourrit l'horloge |
| 118 | u8 | grille sûre | à quel point le « 1 » est établi |
| 119 | u8 | accord | la période confirmée par une voie indépendante ; **c'est le verrou de l'horloge**, pas 118 |
| 120 | u8 | sources actives | nombre de cases publiées, reste compris (≤ 8) |
| 121 | i8 | pitch | tempo mesuré rapporté au tempo appris au cue, en quarts de pour cent |
| 122–125 | 2 × u16 | motifs 6 et 7 | |
| 126 | u8 | caractères 6 et 7 | un quartet chacun |
| 127 | u8 | degrés 6 et 7 | un quartet chacun |
| 128–191 | 8 × 8 o | **sources** | voir ci-dessous |
| 192 | f32 | bpm attendu | |
| 200 | u8 | dérive vue | |
| 201–203 | 3 × u8 | caractères 0 à 5 | un quartet par source, source paire dans le quartet bas ; 0 frappe, 15 tient |
| 204 | f32 | bpm annoncé | |
| 208 | u8 | annonce | |
| 209–211 | 3 × u8 | degrés 0 à 5 | un quartet par source ; 0 tonique … 6, 7 hors gamme, 15 inconnu |
| 216–231 | 8 × 2 o | enveloppes | par source : piqué, tenue |
| 232–239 | 8 × u8 | retraits | depuis combien de temps la source s'est tue, en seizièmes de seconde |
| 240 | u8 | motif global | période en mesures, 0 si inconnue |
| 241 | u8 | motif sûr | |
| 242 | u8 | bande du motif | |
| 243–254 | 6 × u16 | motifs 0 à 5 | seize bits par source, une case par double croche : où la source monte dans la mesure |
| 255 | u8 | verrou | bit 0 les sonorités sont sues ; bit 1 le rythme est su |

### Le mot d'une source (8 octets, à `128 + rang × 8`)

| octet | nom | sens |
|---|---|---|
| 0 | niveau | 0 à 255, rapporté à la crête propre de la source |
| 1 | hauteur | 0 à 255 |
| 2 | drapeaux | bit 0 frappe ; bits 1–3 dominance (8 crans : part de la source qui est vraiment à elle) ; **bits 4–5 platine** (1, 2 ; 3 le reste partagé ; 0 inconnu) |
| 3 | nom | code de forme / nom publié |
| 4 | entendu | 0 à 255, maturité |
| 5 | netteté | 0 à 255 |
| 6 | brillance | |
| 7 | forme | octet de forme (commande le motif quand la fiche n'a rien dit) |

**Le reste** est toujours la dernière case active (`actives − 1`). Son numéro change à
l'accueil et au retrait — voir « un numéro de case n'est pas une identité ».
