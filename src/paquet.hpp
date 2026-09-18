// Les 256 octets d'une image, tels qu'emotion-calculator les publie. Copie de travail de
// GpuPacket.cs : la source de verite est la-bas, et docs/contrat.md porte la meme table.
#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace emotion {

constexpr int TaillePaquet = 256;

// Les decalages, nommes comme dans outils/fenetre.py d'emotion-calculator.
namespace P {
constexpr int Sequence = 4, Temps = 8, Niveau = 16, Bpm = 20, Phase = 24, Frappes = 40, Nouveaute = 47;
constexpr int Bandes = 48, Relais = 60, AccordGamme = 61;
constexpr int VoixBas = 96, VoixMed = 97, VoixHaut = 98, VoixCoups = 99;
constexpr int Centroide = 100, Ouverture = 101, Densite = 102, Beat = 103, Montee = 106, Structure = 107;
constexpr int PhaseTemps = 117, GrilleSure = 118, Accord = 119, SourcesActives = 120, Pitch = 121;
constexpr int Motifs6 = 122, Caracteres3 = 126, Degres3 = 127;
constexpr int Sources = 128, SourcePas = 8;
constexpr int BpmAttendu = 192, DeriveVue = 200, Caracteres = 201, BpmAnnonce = 204, Annonce = 208, Degres = 209;
constexpr int Enveloppes = 216, EnveloppePas = 2, Retraits = 232, Motif = 240, Motifs = 243, Verrou = 255;
}  // namespace P
namespace S {
constexpr int Niveau = 0, Hauteur = 1, Drapeaux = 2, Nom = 3, Entendu = 4, Nettete = 5, Brillance = 6, Forme = 7;
constexpr int DominanceDecalage = 1, PlatineDecalage = 4;
}  // namespace S

struct Source {
    float niveau = 0, hauteur = 0, dominance = 0, entendu = 0, nettete = 0, pique = 0, tenue = 0, retrait = 0, caractere = 0;
    bool frappe = false;
    int platine = 0;   // 1, 2 ; 3 le reste partage ; 0 inconnu
    int nom = 0, forme = 0, degre = 15;
    uint16_t motif = 0;
};

struct Paquet {
    uint32_t sequence = 0;
    int64_t tempsMs = 0;
    float niveau = 0, bpm = 0, phase = 0, bpmAttendu = 0, bpmAnnonce = 0;
    bool kick = false, clap = false, charley = false, coupGrave = false, rupture = false, annonce = false;
    float nouveaute = 0, montee = 0, deriveVue = 0, phaseTemps = 0, grilleSure = 0, accord = 0, accordGamme = 0;
    float brillance = 0, ouverture = 0, densite = 0;
    std::array<float, 3> voix{};
    std::array<float, 12> bandes{};
    int beat = 7;                  // 0..3 ; >= 4 quand le « 1 » est inconnu
    int actives = 0;
    bool verrouSonorites = false, verrouRythme = false;
    float pitch = 1.0f;            // tempo mesure / tempo appris au cue
    int platineJoue = 0, platineEntre = 0;
    bool fonduEnCours = false;
    int motifGlobal = 0; float motifSur = 0; int motifBande = 0;
    std::array<Source, 8> sources{};

    static Paquet decoder(const uint8_t* b) {
        Paquet p;
        auto u8 = [&](int o) { return b[o] / 255.0f; };
        auto f32 = [&](int o) { float v; std::memcpy(&v, b + o, 4); return v; };
        auto u16 = [&](int o) { uint16_t v; std::memcpy(&v, b + o, 2); return v; };
        std::memcpy(&p.sequence, b + P::Sequence, 4);
        std::memcpy(&p.tempsMs, b + P::Temps, 8);
        p.niveau = f32(P::Niveau); p.bpm = f32(P::Bpm); p.phase = f32(P::Phase);
        p.bpmAttendu = f32(P::BpmAttendu); p.bpmAnnonce = f32(P::BpmAnnonce);
        p.kick = b[P::Frappes] & 1; p.clap = b[P::Frappes] & 2; p.charley = b[P::Frappes] & 4;
        p.coupGrave = b[P::VoixCoups] & 1; p.rupture = b[P::Structure] & 1; p.annonce = b[P::Annonce] != 0;
        p.nouveaute = u8(P::Nouveaute); p.montee = u8(P::Montee); p.deriveVue = u8(P::DeriveVue);
        p.phaseTemps = u8(P::PhaseTemps); p.grilleSure = u8(P::GrilleSure); p.accord = u8(P::Accord);
        p.accordGamme = u8(P::AccordGamme);
        p.brillance = u8(P::Centroide); p.ouverture = u8(P::Ouverture); p.densite = u8(P::Densite);
        p.voix = {u8(P::VoixBas), u8(P::VoixMed), u8(P::VoixHaut)};
        for (int i = 0; i < 12; ++i) p.bandes[i] = u8(P::Bandes + i);
        p.beat = b[P::Beat];
        p.actives = b[P::SourcesActives] > 8 ? 8 : b[P::SourcesActives];
        p.verrouSonorites = b[P::Verrou] & 1; p.verrouRythme = b[P::Verrou] & 2;
        p.pitch = 1.0f + static_cast<int8_t>(b[P::Pitch]) / 400.0f;
        p.platineJoue = b[P::Relais] & 3; p.fonduEnCours = b[P::Relais] & 4; p.platineEntre = (b[P::Relais] >> 4) & 3;
        p.motifGlobal = b[P::Motif]; p.motifSur = u8(P::Motif + 1); p.motifBande = b[P::Motif + 2];
        for (int r = 0; r < 8; ++r) {
            Source& s = p.sources[r];
            const int o = P::Sources + r * P::SourcePas;
            s.niveau = u8(o + S::Niveau); s.hauteur = u8(o + S::Hauteur);
            s.frappe = b[o + S::Drapeaux] & 1;
            s.dominance = ((b[o + S::Drapeaux] >> S::DominanceDecalage) & 7) / 7.0f;
            s.platine = (b[o + S::Drapeaux] >> S::PlatineDecalage) & 3;
            s.nom = b[o + S::Nom]; s.entendu = u8(o + S::Entendu); s.nettete = u8(o + S::Nettete); s.forme = b[o + S::Forme];
            s.pique = u8(P::Enveloppes + r * P::EnveloppePas); s.tenue = u8(P::Enveloppes + r * P::EnveloppePas + 1);
            s.retrait = b[P::Retraits + r] / 16.0f;
            // Les six premieres sources ont leurs mots a 243, 201 et 209 ; les deux dernieres,
            // arrivees avec le relais, vivent a 122, 126 et 127.
            s.motif = r < 6 ? u16(P::Motifs + 2 * r) : u16(P::Motifs6 + 2 * (r - 6));
            const int oCar = r < 6 ? P::Caracteres + r / 2 : P::Caracteres3;
            const int oDeg = r < 6 ? P::Degres + r / 2 : P::Degres3;
            s.caractere = ((b[oCar] >> (4 * (r % 2))) & 0xF) / 15.0f;
            s.degre = (b[oDeg] >> (4 * (r % 2))) & 0xF;
        }
        return p;
    }
};

}  // namespace emotion
