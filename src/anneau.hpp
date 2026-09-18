// L'anneau partage d'emotion-calculator, cote lecteur. Sans verrou, sans attente : on lit la
// derniere case publiee, et l'on jette ce qu'on a rate — une image de 21 ms en retard n'a
// aucune valeur.
#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "paquet.hpp"

namespace emotion {

class Anneau {
public:
    static constexpr const char* CheminDefaut = "/dev/shm/emotion-emulator";
    static constexpr uint32_t Magie = 0x454D5230;   // « EMR0 »
    static constexpr int EnTete = 192;
    static constexpr int OffMagie = 0, OffCapacite = 4, OffTaille = 8, OffEcrit = 64;

    explicit Anneau(std::string chemin = CheminDefaut) : chemin_(std::move(chemin)) {}
    ~Anneau() { fermer(); }
    Anneau(const Anneau&) = delete;
    Anneau& operator=(const Anneau&) = delete;

    bool ouvert() const { return base_ != nullptr; }

    bool ouvrir() {
        if (base_) return true;
        int fd = ::open(chemin_.c_str(), O_RDONLY);
        if (fd < 0) return false;
        struct stat st{};
        if (fstat(fd, &st) != 0 || st.st_size < EnTete) { ::close(fd); return false; }
        void* m = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (m == MAP_FAILED) return false;
        taille_fichier_ = st.st_size;
        base_ = static_cast<const uint8_t*>(m);
        if (lire32(OffMagie) != Magie) { fermer(); return false; }
        capacite_ = static_cast<int32_t>(lire32(OffCapacite));
        taille_ = static_cast<int32_t>(lire32(OffTaille));
        if (capacite_ <= 0 || (capacite_ & (capacite_ - 1)) != 0 || taille_ != TaillePaquet) { fermer(); return false; }
        return true;
    }

    void fermer() {
        if (base_) munmap(const_cast<uint8_t*>(base_), taille_fichier_);
        base_ = nullptr;
    }

    // Combien de cases ont ete publiees depuis l'ouverture du moteur.
    int64_t publiees() const { return base_ ? lireEcrit() : 0; }

    // LA DERNIERE CASE PUBLIEE. Vrai si `out` a ete rempli avec une copie valide.
    //
    // Constater qu'une case est valide ne suffit pas : entre la lecture du curseur et la fin
    // de la copie, le producteur peut avoir fait un tour complet et reecrit la case. On relit
    // le curseur APRES la copie et l'on jette si la case copiee a ete depassee d'un tour.
    bool derniere(Paquet& out, uint32_t* sequence = nullptr) {
        if (!base_ && !ouvrir()) return false;
        const int64_t ecrit = lireEcrit();
        if (ecrit <= 0) return false;
        const int64_t rang = ecrit - 1;
        uint8_t copie[TaillePaquet];
        std::memcpy(copie, base_ + EnTete + (rang & (capacite_ - 1)) * taille_, TaillePaquet);
        std::atomic_thread_fence(std::memory_order_acquire);
        const int64_t apres = lireEcrit();
        if (apres - rang > capacite_) return false;      // la case a ete reecrite pendant la copie
        out = Paquet::decoder(copie);
        if (sequence) *sequence = out.sequence;
        return true;
    }

private:
    uint32_t lire32(int o) const { uint32_t v; std::memcpy(&v, base_ + o, 4); return v; }
    int64_t lireEcrit() const {
        int64_t v;
        std::memcpy(&v, base_ + OffEcrit, 8);   // l'ecrivain publie par Volatile.Write : une lecture alignee suffit
        return v;
    }

    std::string chemin_;
    const uint8_t* base_ = nullptr;
    size_t taille_fichier_ = 0;
    int32_t capacite_ = 0, taille_ = 0;
};

}  // namespace emotion
