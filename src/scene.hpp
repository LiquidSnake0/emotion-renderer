// La composition : une scene par platine, a partir des cases signees. Port de ce que fait
// outils/fenetre.py dans emotion-emulator (registres / etiquette_scene / _suivre_les_rangs).
// Rien ne dessine ici : on calcule des rectangles et des etats, le rendu les prend.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <tuple>
#include <vector>

#include "paquet.hpp"

namespace emotion {

struct Rect { float x = 0, y = 0, l = 0, h = 0; bool vide() const { return l <= 0 || h <= 0; } };

struct Composition {
    std::array<Rect, 3> scenes{};                 // [1], [2] ; [0] inutilise
    std::array<Rect, 8> cases{};                  // par rang ; vide si la case n'est pas dessinee
    std::vector<Rect> cadresVides;                // la grille de lecture
    std::array<float, 3> ouverture{0, 1, 0};
    std::array<float, 3> faderVu{0, 0, 0};        // niveau moyen des cases de la platine, lisse
};

class Scenes {
public:
    static constexpr float Gouttiere = 14, HautCase = 124, EtiquetteH = 18, InterRang = 12;
    static constexpr int Colonnes = 4, Rangs = 2;

    // A APPELER UNE FOIS PAR IMAGE D'ANALYSE (paquet nouveau), pas par image de rendu.
    void nouvellePaquet(const Paquet& p) { suivreLesRangs(p); }

    // A appeler a chaque image de rendu : les grandeurs lissees avancent avec dt.
    void pas(const Paquet& p, float dt) {
        bool presentes[3] = {false, false, false};
        float sommes[3] = {0, 0, 0}; int n[3] = {0, 0, 0};
        if (p.platineJoue == 1 || p.platineJoue == 2) presentes[p.platineJoue] = true;
        if (p.fonduEnCours && (p.platineEntre == 1 || p.platineEntre == 2)) presentes[p.platineEntre] = true;
        for (int r = 0; r < p.actives; ++r) {
            const int t = p.sources[r].platine;
            if (t == 1 || t == 2) { presentes[t] = true; sommes[t] += p.sources[r].niveau * gains_[r]; n[t]++; }
        }
        if (!presentes[1] && !presentes[2]) presentes[1] = true;   // un paquet sans relais : P1 seule
        for (int k = 1; k <= 2; ++k) {
            const float cible = presentes[k] ? 1.0f : 0.0f;
            ouverture_[k] += (cible - ouverture_[k]) * std::min(1.0f, dt * 2.5f);
            if (std::fabs(ouverture_[k] - cible) < 0.004f) ouverture_[k] = cible;
            const float v = n[k] ? sommes[k] / n[k] : 0.0f;
            faderVu_[k] += (v - faderVu_[k]) * std::min(1.0f, dt * 6.0f);
        }
    }

    Composition composer(const Paquet& p, float x, float y, float largeur) const {
        Composition c;
        c.ouverture = ouverture_; c.faderVu = faderVu_;
        float o1 = ouverture_[1], o2 = ouverture_[2];
        if (o1 + o2 < 1e-3f) o1 = 1.0f;
        const bool deux = std::min(o1, o2) > 0.02f;
        const float dispo = largeur - (deux ? Gouttiere : 0.0f);
        const float l1 = dispo * o1 / (o1 + o2), l2 = dispo - l1;
        c.scenes[1] = {x, y, l1, EtiquetteH + Rangs * (HautCase + InterRang)};
        c.scenes[2] = {x + l1 + (deux ? Gouttiere : 0.0f), y, l2, c.scenes[1].h};
        const float yGrille = y + EtiquetteH;

        // A quelle scene va chaque case : le tag vient du moteur ; sans tag, la scene ouverte.
        const int ouverte = o1 >= o2 ? 1 : 2;
        std::array<std::vector<int>, 3> parScene; std::vector<int> partagees;
        for (int r = 0; r < p.actives; ++r) {
            const int t = p.sources[r].platine;
            if (t == 3) partagees.push_back(r); else parScene[(t == 1 || t == 2) ? t : ouverte].push_back(r);
        }
        float largCase[3] = {0, 0, 0};
        for (int k = 1; k <= 2; ++k) if (c.scenes[k].l >= 2.0f) largCase[k] = (c.scenes[k].l - (Colonnes - 1) * Gouttiere) / Colonnes;

        // La place du reste partage, calculee avant la grille : a cheval sur la frontiere.
        Rect reste{};
        if (!partagees.empty() && deux && largCase[1] > 0 && largCase[2] > 0) {
            const float lr = (largCase[1] + largCase[2]) / 2;
            reste = {c.scenes[2].x - Gouttiere / 2 - lr / 2, yGrille + HautCase + InterRang, lr, HautCase};
        }
        for (int k = 1; k <= 2; ++k) {
            if (largCase[k] <= 0) continue;
            const float larg = largCase[k];
            for (int i = 0; i < Rangs * Colonnes; ++i) {
                Rect cadre{c.scenes[k].x + (i % Colonnes) * (larg + Gouttiere), yGrille + (i / Colonnes) * (HautCase + InterRang), larg, HautCase};
                const bool sousLeReste = !reste.vide() && cadre.y == reste.y && cadre.x < reste.x + reste.l && cadre.x + larg > reste.x;
                if (!sousLeReste) c.cadresVides.push_back(cadre);
            }
            if (larg <= 40) continue;
            for (size_t i = 0; i < parScene[k].size() && i < size_t(Rangs * Colonnes); ++i)
                c.cases[parScene[k][i]] = {c.scenes[k].x + (i % Colonnes) * (larg + Gouttiere), yGrille + (i / Colonnes) * (HautCase + InterRang), larg, HautCase};
        }
        for (size_t j = 0; j < partagees.size(); ++j) {
            const int r = partagees[j];
            if (!reste.vide()) { c.cases[r] = reste; continue; }
            const int k = ouverte;
            if (largCase[k] <= 40) continue;
            const size_t i = parScene[k].size() + j;
            if (i >= size_t(Rangs * Colonnes)) continue;
            c.cases[r] = {c.scenes[k].x + (i % Colonnes) * (largCase[k] + Gouttiere), yGrille + (i / Colonnes) * (HautCase + InterRang), largCase[k], HautCase};
        }
        return c;
    }

    // Le gain local d'une case (un reglage du rendu, jamais dans le paquet).
    float gain(int r) const { return gains_[r]; }
    void gain(int r, float v) { gains_[r] = std::clamp(v, 0.0f, 1.0f); }

private:
    // LE FADER SUIT L'INSTRUMENT, PAS LE NUMERO DE LA CASE. A l'accueil le reste passe de la
    // case 4 a la case 8 ; ce qui etait attache a une case se reporte par identite : le reste
    // vers le reste, la i-eme case d'une platine vers la i-eme case de la meme platine.
    using Cle = std::tuple<int, int>;   // (platine, ordinal) ; (-1, 0) pour le reste
    static Cle cle(const Paquet& p, int r) {
        if (r == p.actives - 1) return {-1, 0};
        int ord = 0;
        for (int k = 0; k < r; ++k) if (p.sources[k].platine == p.sources[r].platine) ++ord;
        return {p.sources[r].platine, ord};
    }
    void suivreLesRangs(const Paquet& p) {
        std::array<int, 8> tags{};
        for (int r = 0; r < p.actives; ++r) tags[r] = p.sources[r].platine;
        if (!precValide_ || precActives_ == 0) { memoriser(p, tags); return; }
        if (precActives_ == p.actives && tags == precTags_) return;
        std::map<Cle, float> anciens;
        for (int r = 0; r < precActives_; ++r) anciens[clePrec(r)] = gains_[r];
        std::array<float, 8> neufs; neufs.fill(1.0f);
        for (int r = 0; r < p.actives; ++r) {
            auto it = anciens.find(cle(p, r));
            neufs[r] = it == anciens.end() ? 1.0f : it->second;
        }
        gains_ = neufs;
        memoriser(p, tags);
    }
    Cle clePrec(int r) const {
        if (r == precActives_ - 1) return {-1, 0};
        int ord = 0;
        for (int k = 0; k < r; ++k) if (precTags_[k] == precTags_[r]) ++ord;
        return {precTags_[r], ord};
    }
    void memoriser(const Paquet& p, const std::array<int, 8>& tags) { precValide_ = true; precActives_ = p.actives; precTags_ = tags; }

    std::array<float, 3> ouverture_{0, 1, 0};
    std::array<float, 3> faderVu_{0, 0, 0};
    std::array<float, 8> gains_{1, 1, 1, 1, 1, 1, 1, 1};
    bool precValide_ = false; int precActives_ = 0; std::array<int, 8> precTags_{};
};

}  // namespace emotion
