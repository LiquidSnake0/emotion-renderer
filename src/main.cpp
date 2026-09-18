// emotion-renderer — pour l'instant, la sonde : ce que le rendu recevrait, image par image.
//
//   emotion-renderer --sonde [secondes]     lit /dev/shm/emotion-emulator et imprime
//
// Rien ne dessine encore. Avant la premiere image GL, une seule chose a prouver : que ce
// programme lit exactement ce que le moteur ecrit, y compris pendant un relais entre deux
// disques — le meme contrat que outils/fenetre.py d'emotion-emulator.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "anneau.hpp"
#include "paquet.hpp"
#include "scene.hpp"

using namespace emotion;

static int sonde(double secondes) {
    Anneau anneau;
    Scenes scenes;
    Paquet p;
    uint32_t derniere = 0; int lus = 0, trous = 0;
    auto depart = std::chrono::steady_clock::now(), precedent = depart;
    std::printf("sonde : %s, %.0f s\n", Anneau::CheminDefaut, secondes);
    while (true) {
        auto maintenant = std::chrono::steady_clock::now();
        const double ecoule = std::chrono::duration<double>(maintenant - depart).count();
        if (ecoule >= secondes) break;
        const float dt = std::chrono::duration<float>(maintenant - precedent).count();
        precedent = maintenant;
        Paquet q;
        if (anneau.derniere(q)) {
            if (q.sequence != derniere) {
                if (derniere && q.sequence > derniere + 1) trous += q.sequence - derniere - 1;
                derniere = q.sequence; p = q; ++lus;
                scenes.nouvellePaquet(p);
                std::string tags;
                for (int r = 0; r < p.actives; ++r) tags += std::to_string(p.sources[r].platine);
                std::printf("t=%7.2f s  seq %7u  bpm %6.1f  relais joue=%d fondu=%d entre=%d  cases=%d [%s]  ouv P1 %.2f P2 %.2f  fader vu %.2f/%.2f\n",
                            p.tempsMs / 1000.0, p.sequence, p.bpm, p.platineJoue, p.fonduEnCours, p.platineEntre, p.actives, tags.c_str(),
                            scenes.composer(p, 0, 0, 1180).ouverture[1], scenes.composer(p, 0, 0, 1180).ouverture[2],
                            scenes.composer(p, 0, 0, 1180).faderVu[1], scenes.composer(p, 0, 0, 1180).faderVu[2]);
            }
        }
        if (lus) scenes.pas(p, dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    std::printf("%d images lues, %d trou(s), anneau %s\n", lus, trous, anneau.ouvert() ? "ouvert" : "absent");
    return lus ? 0 : 1;
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "--sonde";
    if (mode == "--sonde") return sonde(argc > 2 ? std::atof(argv[2]) : 10.0);
    std::fprintf(stderr, "emotion-renderer --sonde [secondes]\n(le rendu GL n'existe pas encore : voir README)\n");
    return 2;
}
