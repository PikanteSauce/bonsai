#include "utils.h"
#include "config.h"

float conversion(int valeur) {
    // conversion de la valeur brute entière vers voltage lu puis vers % d'humidite
    float volts = valeur * (VOLTAGE / MAX_RESOLUTION);  // valeur en volts
    float pourcentHumidite = volts * 100 / 3.3;
    return pourcentHumidite;
}

float filtreMedianeMoy(float* tab, float tolDistance) {
    /* tolerance à donnée en cm */
    float mediane = tab[TAILLE_TAB/2];
    float borneMin = mediane - tolDistance;
    float borneMax = mediane + tolDistance;
    float mesuresAddi = 0.0;
    int nbMesureAcceptee = 0;
    for (int i = 0; i < (TAILLE_TAB - 1); i++){
        if (tab[i] >= borneMin && tab[i] <= borneMax) {
            mesuresAddi += tab[i];
            nbMesureAcceptee ++;
        }    
    }
    if (nbMesureAcceptee == 0) {
        return 0;
    }
    else {
        return (mesuresAddi / nbMesureAcceptee);
    }  
}

// tri d'un tableau de float par ordre croissant de taile donnée
void trierTableau(float *tab, int taille) {
    int i, j;
    float temp;
    for (i = 0; i < taille - 1; i++) {
        for (j = 0; j < taille - i - 1; j++) {
            if (tab[j] > tab[j + 1]) {
                // Échange
                temp = tab[j];
                tab[j] = tab[j + 1];
                tab[j + 1] = temp;
            }
        }
    }
}