#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>


#define clientsFile "clients"
#define compteursFile "compteur"
#define facturesFile "factures.dat"
#define usersFile "users.dat"


//
////
//////
// dealing with clients: display, ajout, supression, modification:
typedef struct {
    int id_client;
    char nom[50];
    char prenom[50];
    char adresse[100];
    char email[50];
    char telephone[30];
} Client;
typedef struct {
    char matricule[20];   // Ex: "CPT-8845-X". C'est le numéro de série écrit sur l'appareil.
    int id_client;        // CLÉ ÉTRANGÈRE : Le lien vers le propriétaire.
    int type;             // 1 = 2 Fils (Monophasé), 2 = 4 Fils (Triphasé). Cela change le prix de l'abonnement.
    int index_actuel;
}Compteur;
typedef struct {
    int id_facture;
    int id_client;
    char date_emission[20];
    int index_ancien;
    int index_nouveau;
    int consommation;

    // --- Gestion Financière 
    float montant_ht;       // Hors Taxe
    float montant_tva;      // TVA (14%)
    float montant_total;    // Net à payer (TTC)

    // --- Gestion des Paiements ---
    int estPaye;            // 0 = Non, 1 = Oui
    char mode_paiement[20];
    char date_paiement[20];
} Facture;
typedef struct {
    char username[20];
    char password[20];
    int role; // 1 = ADMIN (Tout faire), 2 = AGENT (Ajouter/Facturer seulement)
} User;

int estToutChiffres(const char* chaine) {
    if (strlen(chaine) == 0) return 0; // Vide
    for (int i = 0; chaine[i] != '\0'; i++) {
        if (!isdigit(chaine[i])) return 0; // Trouvé un non-chiffre
    }
    return 1;
}

// Vérifie si une chaine contient uniquement des lettres et espaces (pour Nom/Prénom)
int estToutLettres(const char* chaine) {
    if (strlen(chaine) == 0) return 0;
    for (int i = 0; chaine[i] != '\0'; i++) {
        if (!isalpha(chaine[i]) && chaine[i] != ' ' && chaine[i] != '-') return 0;
    }
    return 1;
}

// Vérifie format basique email (contient '@' et '.')
int estEmailValide(const char* email) {
    return (strchr(email, '@') != NULL && strchr(email, '.') != NULL);
}

// Vérifie téléphone (chiffres, +, espaces)
int estTelValide(const char* tel) {
    if (strlen(tel) < 8) return 0; // Trop court
    for (int i = 0; tel[i] != '\0'; i++) {
        if (!isdigit(tel[i]) && tel[i] != '+' && tel[i] != ' ') return 0;
    }
    return 1;
}

//void lireChaine(char* chaine, int longueur) {
//    char* posEntree = NULL;
//    if (fgets(chaine, longueur, stdin) != NULL) {
//        posEntree = strchr(chaine, '\n');
//        if (posEntree != NULL) {
//            *posEntree = '\0';
//        }
//    }
//}

// Remplacez tout le bloc de votre ancienne fonction par ceci :
void lireChaine(char* chaine, int longueur) {
    // On lit la chaîne
    if (fgets(chaine, longueur, stdin) != NULL) {
        // Astuce moderne pour enlever le \n :
        // strcspn renvoie l'index du premier '\n' trouvé.
        // On remplace ce caractère par '\0' (fin de chaîne).
        chaine[strcspn(chaine, "\n")] = '\0';
    }
    else {
        // Sécurité si fgets échoue (ex: erreur de lecture)
        chaine[0] = '\0'; // On rend la chaîne vide
    }
}

Client* getAllClients(int *count) {

    FILE* fichier = fopen(clientsFile, "rb");

    if (fichier == NULL) {
        *count = 0;
        return NULL;
    }
    fseek(fichier, 0, SEEK_END); 
    long fileSize = ftell(fichier); 
    rewind(fichier); 

    *count = (int)(fileSize / sizeof(Client));

    if (*count == 0) {
        fclose(fichier);
        return NULL;
    }

    Client* clientList = (Client*)malloc((*count) * sizeof(Client));

    if (clientList == NULL) {
        printf("Memory allocation failed!\n");
        fclose(fichier);
        *count = 0;
        return NULL;
    }

    fread(clientList, sizeof(Client), *count, fichier);

    fclose(fichier);
    return clientList;
}

int clientExiste(int idToCheck) {
    FILE* fichier = fopen(clientsFile, "rb");
    Client c;

    if (fichier == NULL) {
        return 0;
    }

    while (fread(&c, sizeof(Client), 1, fichier)) {
        if (c.id_client == idToCheck) {
            fclose(fichier);
            return 1; 
        }
    }

    fclose(fichier);
    return 0; 
}

void addClient() {
    Client c;
    char buffer[100]; // Buffer temporaire pour lire les entrées
    int valide = 0;   // Flag pour les boucles de validation

    FILE* fichier = fopen(clientsFile, "ab"); // On ouvre à la fin seulement si tout est OK
    if (fichier == NULL) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    printf("\n--- Nouveau Client (Saisie securisee) ---\n");

    // --- 1. Saisie de l'ID (Entier, Positif, Unique, Pas de lettres) ---
    do {
        printf("ID Client (Entier positif unique) : ");
        lireChaine(buffer, 100); // On lit tout comme du texte d'abord

        // A. Vérifier si c'est numérique
        if (!estToutChiffres(buffer)) {
            printf(">> Erreur : L'ID doit contenir uniquement des chiffres.\n");
            continue;
        }

        // B. Convertir en int
        int idTemp = atoi(buffer);

        // C. Vérifier si positif
        if (idTemp <= 0) {
            printf(">> Erreur : L'ID doit etre strictement positif.\n");
            continue;
        }

        // D. Vérifier si existe déjà
        // Note: Assurez-vous que la fonction clientExiste ouvre/ferme le fichier correctement en interne
        if (clientExiste(idTemp)) {
            printf(">> Erreur : L'ID %d existe deja. Choisissez-en un autre.\n", idTemp);
            continue;
        }

        // Si tout est bon
        c.id_client = idTemp;
        valide = 1;

    } while (!valide);


    // --- 2. Saisie du Nom (Lettres seulement) ---
    valide = 0;
    do {
        printf("Nom : ");
        lireChaine(c.nom, 50);
        if (estToutLettres(c.nom)) {
            valide = 1;
        }
        else {
            printf(">> Erreur : Le nom ne doit contenir que des lettres.\n");
        }
    } while (!valide);


    // --- 3. Saisie du Prénom (Lettres seulement) ---
    valide = 0;
    do {
        printf("Prenom : ");
        lireChaine(c.prenom, 50);
        if (estToutLettres(c.prenom)) {
            valide = 1;
        }
        else {
            printf(">> Erreur : Le prenom ne doit contenir que des lettres.\n");
        }
    } while (!valide);


    // --- 4. Adresse (Pas de validation stricte, on accepte tout) ---
    printf("Adresse : ");
    lireChaine(c.adresse, 100);


    // --- 5. Email (Validation @ et .) ---
    valide = 0;
    do {
        printf("Email : ");
        lireChaine(c.email, 50);
        if (estEmailValide(c.email)) {
            valide = 1;
        }
        else {
            printf(">> Erreur : Format email invalide (doit contenir '@' et '.').\n");
        }
    } while (!valide);


    // --- 6. Téléphone (Chiffres et + seulement) ---
    valide = 0;
    do {
        printf("Telephone : ");
        lireChaine(c.telephone, 30);
        if (estTelValide(c.telephone)) {
            valide = 1;
        }
        else {
            printf(">> Erreur : Numero invalide (chiffres uniquement).\n");
        }
    } while (!valide);

    // --- Ecriture dans le fichier ---
    // Note: Le fichier a été ouvert au début, mais si clientExiste l'ouvre aussi, 
    // il faut faire attention aux conflits.
    // Le mieux est d'ouvrir le fichier pour l'écriture SEULEMENT MAINTENANT.
    fclose(fichier); // On ferme le test d'ouverture du début

    fichier = fopen(clientsFile, "ab"); // On réouvre pour écrire
    if (fichier != NULL) {
        fwrite(&c, sizeof(Client), 1, fichier);
        printf("\n[SUCCES] Client ajoute avec succes !\n");
        fclose(fichier);
    }
    else {
        perror("Erreur finale d'ecriture");
    }

    printf("\n");
}

void modifyClient() {
    int idRecherche, trouve = 0;
    Client c;
    char buffer[100]; // Pour lire l'ID proprement
    char tempBuffer[100]; // Pour lire les nouveaux champs avant validation
    int valide = 0;   // Pour les boucles

    FILE* fichier = fopen(clientsFile, "rb+"); // Lecture et Ecriture

    if (fichier == NULL) {
        printf("Erreur : Impossible d'ouvrir le fichier (il n'existe peut-etre pas).\n");
        return;
    }

    // --- 1. Saisie sécurisée de l'ID à rechercher ---
    do {
        printf("\nEntrez l'ID du client a modifier : ");
        lireChaine(buffer, 100);

        if (!estToutChiffres(buffer)) {
            printf(">> Erreur : L'ID doit etre un nombre entier.\n");
        }
        else {
            idRecherche = atoi(buffer);
            if (idRecherche <= 0) {
                printf(">> Erreur : L'ID doit etre positif.\n");
            }
            else {
                break; // ID valide, on sort de la boucle
            }
        }
    } while (1);


    // --- 2. Recherche et Modification ---
    while (fread(&c, sizeof(Client), 1, fichier)) {
        if (c.id_client == idRecherche) {
            trouve = 1;
            printf("\n--- Client trouve ---\n");
            printf("Nom actuel    : %s\n", c.nom);
            printf("Prenom actuel : %s\n", c.prenom);
            printf("---------------------\n");
            printf("Veuillez saisir les nouvelles informations :\n");

            // --- A. Nouveau Nom ---
            valide = 0;
            do {
                printf("Nouveau Nom : ");
                lireChaine(tempBuffer, 50);
                if (estToutLettres(tempBuffer)) {
                    strcpy(c.nom, tempBuffer); // On valide et on copie
                    valide = 1;
                }
                else {
                    printf(">> Erreur : Le nom ne doit contenir que des lettres.\n");
                }
            } while (!valide);

            // --- B. Nouveau Prénom ---
            valide = 0;
            do {
                printf("Nouveau Prenom : ");
                lireChaine(tempBuffer, 50);
                if (estToutLettres(tempBuffer)) {
                    strcpy(c.prenom, tempBuffer);
                    valide = 1;
                }
                else {
                    printf(">> Erreur : Le prenom ne doit contenir que des lettres.\n");
                }
            } while (!valide);

            // --- C. Nouvelle Adresse (Pas de validation stricte) ---
            printf("Nouvelle Adresse : ");
            lireChaine(c.adresse, 100);

            // --- D. Nouvel Email ---
            valide = 0;
            do {
                printf("Nouvel Email : ");
                lireChaine(tempBuffer, 50);
                if (estEmailValide(tempBuffer)) {
                    strcpy(c.email, tempBuffer);
                    valide = 1;
                }
                else {
                    printf(">> Erreur : Email invalide (doit contenir '@' et '.').\n");
                }
            } while (!valide);

            // --- E. Nouveau Téléphone ---
            valide = 0;
            do {
                printf("Nouveau Telephone : ");
                lireChaine(tempBuffer, 30);
                if (estTelValide(tempBuffer)) {
                    strcpy(c.telephone, tempBuffer);
                    valide = 1;
                }
                else {
                    printf(">> Erreur : Telephone invalide (chiffres uniquement).\n");
                }
            } while (!valide);


            // --- 3. Écriture des modifications ---

            // Reculer le curseur pour écraser l'ancien enregistrement
            fseek(fichier, -((long)sizeof(Client)), SEEK_CUR);

            // Écrire la structure mise à jour
            fwrite(&c, sizeof(Client), 1, fichier);

            printf("\n[SUCCES] Modification enregistree.\n");
            break; // Important : on sort du while car on a trouvé et modifié
        }
    }

    if (!trouve) {
        printf("\nAucun client trouve avec l'ID %d.\n", idRecherche);
    }

    fclose(fichier);

    printf("\n");
}

void deleteClient() {
    int idToDelete;
    int trouve = 0;
    Client c;
    char buffer[100]; // Pour la saisie sécurisée

    // --- 1. Saisie sécurisée de l'ID ---
    // On boucle tant que l'utilisateur ne donne pas un ID valide
    do {
        printf("\nEntrez l'ID du client a supprimer : ");
        lireChaine(buffer, 100);

        if (!estToutChiffres(buffer)) {
            printf(">> Erreur : L'ID doit etre un nombre entier.\n");
        }
        else {
            idToDelete = atoi(buffer);
            if (idToDelete <= 0) {
                printf(">> Erreur : L'ID doit etre positif.\n");
            }
            else {
                break; // ID valide, on peut continuer
            }
        }
    } while (1);

    // --- 2. Ouverture des fichiers ---
    FILE* fichier = fopen(clientsFile, "rb");

    // Petite astuce : si le fichier original n'existe pas, inutile de créer un temp
    if (fichier == NULL) {
        printf("Erreur : Fichier clients introuvable ou vide.\n");
        return;
    }

    FILE* temp = fopen("temp.dat", "wb");
    if (temp == NULL) {
        printf("Erreur : Impossible de creer le fichier temporaire.\n");
        fclose(fichier); // Important : on n'oublie pas de fermer le premier fichier
        return;
    }

    // --- 3. Copie sélective ---
    // On copie tout dans 'temp' SAUF le client qui a l'ID à supprimer
    while (fread(&c, sizeof(Client), 1, fichier)) {
        if (c.id_client != idToDelete) {
            fwrite(&c, sizeof(Client), 1, temp);
        }
        else {
            trouve = 1;
            // On affiche qui on vient de supprimer pour confirmation visuelle
            printf(">> Suppression du client : %s %s (ID: %d)\n", c.nom, c.prenom, c.id_client);
        }
    }

    // --- 4. Fermeture et Remplacement ---
    fclose(fichier);
    fclose(temp);

    if (trouve) {
        // Suppression de l'ancien fichier
        if (remove(clientsFile) == 0) {
            // Renommage du temporaire en "clients"
            if (rename("temp.dat", clientsFile) == 0) {
                printf("[SUCCES] La suppression a ete effectuee.\n");
            }
            else {
                printf("[ERREUR] Impossible de renommer le fichier temporaire.\n");
            }
        }
        else {
            printf("[ERREUR] Impossible de supprimer l'ancien fichier clients.\n");
        }
    }
    else {
        // Si on n'a rien trouvé, on supprime le fichier temporaire qui ne sert à rien
        remove("temp.dat");
        printf("Aucun client trouve avec l'ID %d. Aucune modification faite.\n", idToDelete);
    }


    printf("\n");
}

void displayClients() {
    int nombreClients = 0;

    // 1. On récupère le tableau complet via la fonction
    Client* liste = getAllClients(&nombreClients);

    // 2. Vérification si vide ou erreur
    if (liste == NULL) {
        printf("\nAucun fichier client trouve ou liste vide.\n");
        return;
    }

    printf("\n--- Liste des Clients (%d trouves) ---\n", nombreClients);
    printf("%-5s %-15s %-15s %-20s %-20s %-15s\n", "ID", "Nom", "Prenom", "Adresse", "Email", "Tel");
    printf("----------------------------------------------------------------------------------------------------\n");

    // 3. On parcourt le TABLEAU (et non plus le fichier)
    for (int i = 0; i < nombreClients; i++) {
        printf("%-5d %-15s %-15s %-20s %-20s %-15s\n",
            liste[i].id_client,
            liste[i].nom,
            liste[i].prenom,
            liste[i].adresse,
            liste[i].email,
            liste[i].telephone);
    }

    // 4. IMPORTANT : On libère la mémoire allouée par malloc dans getAllClients
    free(liste);


    printf("\n");
}
//end of client crud operations
//////
////
//


//
////
//////
//start of compteur function:

// Vérifie si un matricule existe déjà (1 = existe, 0 = n'existe pas)
int compteurExiste(char* matriculeCheck) {
    FILE* fichier = fopen(compteursFile, "rb");
    Compteur cpt;

    // Si le fichier n'existe pas encore, pas de doublon possible
    if (fichier == NULL) return 0;

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        if (strcmp(cpt.matricule, matriculeCheck) == 0) {
            fclose(fichier);
            return 1; // Trouvé !
        }
    }
    fclose(fichier);
    return 0;
}

// Vérifie si un client possède déjà un compteur (1 = Oui, 0 = Non)
int clientAUnCompteur(int idClientCheck) {
    FILE* fichier = fopen(compteursFile, "rb");
    Compteur cpt;

    // Si le fichier n'existe pas, personne n'a de compteur
    if (fichier == NULL) return 0;

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        if (cpt.id_client == idClientCheck) {
            fclose(fichier);
            return 1; // Il a déjà un compteur !
        }
    }

    fclose(fichier);
    return 0; // Il n'en a pas
}

void ajouterCompteur() {
    Compteur cpt;
    int idClientTemp;
    char buffer[100];

    printf("\n--- Enregistrement d'un Nouveau Compteur ---\n");

    // --- 1. Lier au client (Validation double : Existe + Pas de compteur) ---
    do {
        printf("ID du Client proprietaire : ");
        lireChaine(buffer, 100);

        if (!estToutChiffres(buffer)) {
            printf(">> Erreur : L'ID doit etre un nombre.\n");
            continue;
        }

        idClientTemp = atoi(buffer);

        // A. Est-ce que le client existe dans la base 'clients' ?
        if (!clientExiste(idClientTemp)) {
            printf(">> Erreur : Le client ID %d n'existe pas.\n", idClientTemp);
            continue;
        }

        // B. (NOUVEAU) Est-ce qu'il a déjà un compteur ?
        if (clientAUnCompteur(idClientTemp)) {
            printf(">> Erreur : Le client ID %d possede DEJA un compteur.\n", idClientTemp);
            printf(">> Impossible d'en ajouter un deuxieme pour le moment.\n");
            // On boucle pour redemander un autre ID
            continue;
        }

        // Si on arrive ici, tout est bon (Client existe ET n'a pas de compteur)
        cpt.id_client = idClientTemp;
        printf(">> Client valide : ID %d (Nouveau contrat)\n", idClientTemp);
        break;

    } while (1);

    // --- La suite reste identique à avant ---

    // 2. Saisie du Matricule
    do {
        printf("Matricule du compteur (ex: CPT-001) : ");
        lireChaine(cpt.matricule, 20);

        if (strlen(cpt.matricule) == 0) {
            printf(">> Erreur : Vide.\n");
        }
        else if (compteurExiste(cpt.matricule)) {
            printf(">> Erreur : Le matricule '%s' existe deja !\n", cpt.matricule);
        }
        else {
            break;
        }
    } while (1);

    // 3. Type
    do {
        printf("Type (1=Mono, 2=Tri) : ");
        lireChaine(buffer, 100);
        if (estToutChiffres(buffer)) {
            cpt.type = atoi(buffer);
            if (cpt.type == 1 || cpt.type == 2) break;
        }
    } while (1);

    // 4. Index
    do {
        printf("Index initial : ");
        lireChaine(buffer, 100);
        if (estToutChiffres(buffer)) {
            cpt.index_actuel = atoi(buffer);
            if (cpt.index_actuel >= 0) break;
        }
    } while (1);

    // 5. Sauvegarde
    FILE* fichier = fopen(compteursFile, "ab");
    if (fichier != NULL) {
        fwrite(&cpt, sizeof(Compteur), 1, fichier);
        printf("\n[SUCCES] Compteur ajoute pour le client %d !\n", cpt.id_client);
        fclose(fichier);
    }
}

void supprimerCompteur() {
    char matriculeSuppr[20];
    int trouve = 0;
    Compteur cpt;

    printf("\n--- Suppression Compteur ---\n");
    printf("Entrez le Matricule exact du compteur : ");
    lireChaine(matriculeSuppr, 20);

    FILE* fichier = fopen(compteursFile, "rb");
    FILE* temp = fopen("temp_cpt.dat", "wb");

    if (fichier == NULL || temp == NULL) {
        printf("Erreur système fichiers.\n");
        if (fichier) fclose(fichier);
        return;
    }

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        // Si ce n'est PAS celui qu'on veut supprimer, on le copie
        if (strcmp(cpt.matricule, matriculeSuppr) != 0) {
            fwrite(&cpt, sizeof(Compteur), 1, temp);
        }
        else {
            trouve = 1; // On l'a trouvé, donc on ne le copie pas (suppression)
            printf("Compteur %s (Client %d) supprime.\n", cpt.matricule, cpt.id_client);
        }
    }

    fclose(fichier);
    fclose(temp);

    if (trouve) {
        remove(compteursFile);
        rename("temp_cpt.dat", compteursFile);
        printf("[SUCCES] Base de donnees mise a jour.\n");
    }
    else {
        remove("temp_cpt.dat");
        printf("[INFO] Matricule introuvable, aucune suppression faite.\n");
    }
}

void updateIndexCompteur(int idClient, int nouvelIndex) {
    FILE* fichier = fopen(compteursFile, "rb+"); // Mode Lecture + Écriture
    Compteur cpt;

    if (fichier == NULL) {
        printf("[ERREUR] Impossible d'ouvrir le fichier compteurs.\n");
        return;
    }

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        if (cpt.id_client == idClient) {
            // Mise à jour de la valeur en mémoire
            cpt.index_actuel = nouvelIndex;

            // Retour en arrière du curseur
            fseek(fichier, -((long)sizeof(Compteur)), SEEK_CUR);

            // Écriture par dessus
            fwrite(&cpt, sizeof(Compteur), 1, fichier);

            // On peut décommenter la ligne suivante pour le debug, mais sinon c'est silencieux
            // printf("[INFO] Index compteur mis a jour a : %d\n", nouvelIndex);
            break;
        }
    }
    fclose(fichier);
}

// Remplit les variables pointées avec les infos du compteur du client donné
// Retourne 1 si trouvé, 0 sinon.
int getCompteurInfo(int idClient, char* matriculeDest, int* typeDest, int* indexDest) {
    FILE* fichier = fopen(compteursFile, "rb");
    Compteur cpt;

    if (fichier == NULL) return 0;

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        if (cpt.id_client == idClient) {
            // Copie des données vers les pointeurs de destination
            strcpy(matriculeDest, cpt.matricule);
            *typeDest = cpt.type;
            *indexDest = cpt.index_actuel;

            fclose(fichier);
            return 1; // Succès
        }
    }

    fclose(fichier);
    return 0; // Pas trouvé
}

void afficherCompteurs() {
    Compteur cpt;
    FILE* fichier = fopen(compteursFile, "rb");

    if (fichier == NULL) {
        printf("\n[INFO] Aucun compteur enregistre ou fichier introuvable.\n");
        return;
    }

    printf("\n--- Liste des Compteurs ---\n");
    printf("%-15s | %-10s | %-12s | %-10s\n", "Matricule", "ID Client", "Type", "Index");
    printf("--------------------------------------------------------------\n");

    while (fread(&cpt, sizeof(Compteur), 1, fichier)) {
        char typeStr[15];
        // On rend l'affichage plus lisible pour l'humain
        if (cpt.type == 1) strcpy(typeStr, "Monophase");
        else strcpy(typeStr, "Triphase");

        printf("%-15s | %-10d | %-12s | %-10d kWh\n",
            cpt.matricule, cpt.id_client, typeStr, cpt.index_actuel);
    }
    fclose(fichier);
}

// --- Fonction Menu pour les Compteurs ---
//void menuCompteurs() {
//    int choix;
//    // Variables pour le test de simulation (Option 4)
//    int idTest, typeTest, indexTest;
//    char matTest[20];
//
//    do {
//        printf("\n====================================\n");
//        printf("       GESTION DES COMPTEURS        \n");
//        printf("====================================\n");
//        printf("1. Ajouter un nouveau compteur\n");
//        printf("2. Afficher la liste des compteurs\n");
//        printf("3. Supprimer un compteur\n");
//        printf("4. [TEST] Simuler une lecture (Test getCompteurInfo)\n");
//        printf("0. Retour au menu principal / Quitter\n");
//        printf("------------------------------------\n");
//        printf("Votre choix : ");
//
//        if (scanf("%d", &choix) != 1) {
//            while (getchar() != '\n'); // Vider buffer si lettre tapée
//            continue;
//        }
//        while (getchar() != '\n'); // Consommer le \n
//
//        switch (choix) {
//        case 1:
//            ajouterCompteur();
//            break;
//        case 2:
//            afficherCompteurs();
//            break;
//        case 3:
//            supprimerCompteur();
//            break;
//        case 4:
//            // Ce test simule ce que fera le module Facture plus tard
//            printf("\n--- Test Simulation Lecture ---\n");
//            printf("Entrez un ID Client pour voir si on trouve son compteur : ");
//            scanf("%d", &idTest);
//
//            // Appel de la fonction "magique"
//            if (getCompteurInfo(idTest, matTest, &typeTest, &indexTest)) {
//                printf(">> SUCCES ! Compteur trouve.\n");
//                printf("   Matricule : %s\n", matTest);
//                printf("   Type      : %d (%s)\n", typeTest, (typeTest == 1) ? "Mono" : "Tri");
//                printf("   Index     : %d kWh\n", indexTest);
//            }
//            else {
//                printf(">> ECHEC : Aucun compteur trouve pour ce client.\n");
//            }
//            break;
//        case 0:
//            printf("Retour...\n");
//            break;
//        default:
//            printf("Choix invalide.\n");
//        }
//    } while (choix != 0);
//}

//////
////
//


// =========================================================
//            MODULE FACTURATION & PAIEMENTS
// =========================================================
// 
// Fonction 1 : Générer un fichier texte (Impression)
void genererFacturePDF(int idFacture) {
    Facture f;
    int trouve = 0;

    FILE* fic = fopen(facturesFile, "rb");
    if (fic == NULL) return;

    while (fread(&f, sizeof(Facture), 1, fic)) {
        if (f.id_facture == idFacture) {
            trouve = 1;
            break;
        }
    }
    fclose(fic);

    if (!trouve) { printf(">> Erreur : Facture introuvable pour l'export.\n"); return; }

    char nomFichier[50];
    sprintf(nomFichier, "Facture_%d.txt", f.id_facture);

    FILE* sortie = fopen(nomFichier, "w");
    if (sortie == NULL) { printf(">> Erreur creation fichier texte.\n"); return; }

    fprintf(sortie, "****************************************\n");
    fprintf(sortie, "         FACTURE D'ELECTRICITE          \n");
    fprintf(sortie, "****************************************\n\n");
    fprintf(sortie, "Facture N : %d\n", f.id_facture);
    fprintf(sortie, "Date      : %s\n", f.date_emission);
    fprintf(sortie, "Client ID : %d\n", f.id_client);
    fprintf(sortie, "----------------------------------------\n");
    fprintf(sortie, "Index Ancien  : %d kWh\n", f.index_ancien);
    fprintf(sortie, "Index Nouveau : %d kWh\n", f.index_nouveau);
    fprintf(sortie, "CONSOMMATION  : %d kWh\n", f.consommation);
    fprintf(sortie, "----------------------------------------\n");
    fprintf(sortie, "Montant H.T.  : %10.2f DH\n", f.montant_ht);
    fprintf(sortie, "T.V.A (14%%)   : %10.2f DH\n", f.montant_tva);
    fprintf(sortie, "----------------------------------------\n");
    fprintf(sortie, "NET A PAYER   : %10.2f DH\n", f.montant_total);
    fprintf(sortie, "========================================\n");

    if (f.estPaye) {
        fprintf(sortie, "STATUT : PAYE le %s\n", f.date_paiement);
        fprintf(sortie, "MODE   : %s\n", f.mode_paiement);
    }
    else {
        fprintf(sortie, "STATUT : NON PAYE\n");
    }
    fprintf(sortie, "****************************************\n");

    printf("\n[SUCCES] La facture a ete exportee dans '%s' !\n", nomFichier);
    fclose(sortie);
}

// Fonction 2 : Logique Mathématique (Sans calcul de remise)
void calculerFactureDetaillee(Facture* f, int typeCompteur) {
    float tva_rate = 0.14; // 14%
    // Redevance fixe : 20DH pour Mono, 40DH pour Tri
    float redevance = (typeCompteur == 1) ? 20.0 : 40.0;
    float total_conso = 0.0;

    // --- Calcul par Tranches ---
    if (f->consommation <= 100) {
        total_conso = f->consommation * 0.90;
    }
    else if (f->consommation <= 200) {
        total_conso = (100 * 0.90) + ((f->consommation - 100) * 1.10);
    }
    else {
        total_conso = (100 * 0.90) + (100 * 1.10) + ((f->consommation - 200) * 1.50);
    }

    // --- Totaux ---
    f->montant_ht = total_conso + redevance;
    f->montant_tva = f->montant_ht * tva_rate;
    f->montant_total = f->montant_ht + f->montant_tva;
}

// Fonction 3 : Créer une facture
void ajouterFacture() {
    Facture f;
    char buffer[100];
    int idClient;
    char matriculeCpt[20];
    int typeCpt, ancienIndex;

    printf("\n--- Etablir une Nouvelle Facture ---\n");

    // 1. Identification
    printf("ID du Client a facturer : ");
    lireChaine(buffer, 100);
    if (!estToutChiffres(buffer)) { printf(">> ID Invalide.\n"); return; }
    idClient = atoi(buffer);

    // 2. Récupération auto des infos Compteur
    if (getCompteurInfo(idClient, matriculeCpt, &typeCpt, &ancienIndex) == 0) {
        printf(">> ERREUR : Ce client n'existe pas ou n'a pas de compteur.\n");
        return;
    }

    printf(">> Client et Compteur (%s) trouves.\n", matriculeCpt);
    printf(">> Ancien Index (auto) : %d kWh\n", ancienIndex);

    // 3. Saisie Nouvel Index
    do {
        printf("Nouvel Index releve : ");
        lireChaine(buffer, 100);
        if (!estToutChiffres(buffer)) continue;

        f.index_nouveau = atoi(buffer);

        if (f.index_nouveau < ancienIndex) {
            printf(">> Erreur : Le nouvel index ne peut pas etre inferieur a l'ancien (%d) !\n", ancienIndex);
        }
        else {
            break;
        }
    } while (1);

    // 4. Remplissage structure
    f.id_client = idClient;
    f.id_facture = rand() % 9000 + 1000;
    f.index_ancien = ancienIndex;
    f.consommation = f.index_nouveau - f.index_ancien;

    // Initialisation Paiement
    f.estPaye = 0;
    strcpy(f.mode_paiement, "N/A");
    strcpy(f.date_paiement, "N/A");

    printf("Date emission (ex: 15/12/2025) : ");
    lireChaine(f.date_emission, 20);

    // 5. Calculs Financiers
    calculerFactureDetaillee(&f, typeCpt);

    printf("\n--- Resume Financier ---\n");
    printf("Conso : %d kWh\n", f.consommation);
    printf("Total TTC : %.2f DH\n", f.montant_total);

    // 6. Sauvegarde
    FILE* fic = fopen(facturesFile, "ab");
    if (fic != NULL) {
        fwrite(&f, sizeof(Facture), 1, fic);
        fclose(fic);

        // 7. Mise à jour Compteur
        updateIndexCompteur(idClient, f.index_nouveau);

        printf("[SUCCES] Facture enregistree.\n");

        printf("Voulez-vous generer le fichier facture maintenant ? (1=Oui, 0=Non) : ");
        lireChaine(buffer, 10);
        if (atoi(buffer) == 1) {
            genererFacturePDF(f.id_facture);
        }

    }
    else {
        perror("Erreur sauvegarde");
    }
}

// Fonction 4 : Payer une facture (Reste inchangée mais je la remets pour être complet)
void payerFacture() {
    int idFac, trouve = 0;
    Facture f;
    FILE* fichier = fopen(facturesFile, "rb+");

    if (fichier == NULL) {
        printf("Aucune facture a payer.\n");
        return;
    }

    printf("\n--- Encaissement Facture ---\n");
    printf("Entrez l'ID de la facture a payer : ");
    char buffer[20];
    lireChaine(buffer, 20);
    idFac = atoi(buffer);

    while (fread(&f, sizeof(Facture), 1, fichier)) {
        if (f.id_facture == idFac) {
            trouve = 1;

            if (f.estPaye == 1) {
                printf(">> Cette facture est DEJA payee (le %s par %s).\n", f.date_paiement, f.mode_paiement);
                break;
            }

            printf(">> Facture trouvee. Montant a payer : %.2f DH\n", f.montant_total);

            printf("Mode de paiement :\n1. Especes\n2. Carte Bancaire\n3. Cheque\nChoix : ");
            int choixPaiement;
            lireChaine(buffer, 10);
            choixPaiement = atoi(buffer);

            if (choixPaiement == 1) strcpy(f.mode_paiement, "Especes");
            else if (choixPaiement == 2) strcpy(f.mode_paiement, "Carte Bancaire");
            else strcpy(f.mode_paiement, "Cheque");

            printf("Date du paiement (JJ/MM/AAAA) : ");
            lireChaine(f.date_paiement, 20);

            f.estPaye = 1;

            fseek(fichier, -((long)sizeof(Facture)), SEEK_CUR);
            fwrite(&f, sizeof(Facture), 1, fichier);

            printf("[SUCCES] Paiement enregistre !\n");
            break;
        }
    }
    if (!trouve) printf("Facture introuvable.\n");
    fclose(fichier);
}

void afficherFactures() {
    Facture f;
    FILE* fic = fopen(facturesFile, "rb");

    if (fic == NULL) {
        printf("Aucune facture enregistree.\n");
        return;
    }

    printf("\n--- Historique des Factures ---\n");
    printf("%-5s | %-5s | %-12s | %-8s | %-10s | %-8s\n", "ID", "Clt", "Date", "Conso", "Montant", "Etat");
    printf("-------------------------------------------------------------------\n");

    while (fread(&f, sizeof(Facture), 1, fic)) {
        printf("%-5d | %-5d | %-12s | %-5d kWh | %-7.2f DH | %s\n",
            f.id_facture, f.id_client, f.date_emission, f.consommation, f.montant_total,
            (f.estPaye ? "PAYE" : "NON"));
    }
    fclose(fic);
}
//////
////
//

// permessions and roles

//////
////
//


void ajouterUtilisateur() {
    User u;
    char buffer[10];
    printf("\n- - - - - - - - - - - - - - - - - - - - - - -\n");
    printf("           Creation Nouvel Utilisateur\n");
    printf("- - - - - - - - - - - - - - - - - - - - - - -\n");

    printf("Nom d'utilisateur : ");
    lireChaine(u.username, 20);
    printf("Mot de passe : ");
    lireChaine(u.password, 20);

    do {
        printf("Role (1=Admin, 2=Agent) : ");
        lireChaine(buffer, 10);
        u.role = atoi(buffer);
    } while (u.role != 1 && u.role != 2);

    FILE* f = fopen(usersFile, "ab");
    fwrite(&u, sizeof(User), 1, f);
    fclose(f);
    printf("[SUCCES] Utilisateur %s ajoute.\n", u.username);
}


void header(char* headerTitle) {
    system("cls");
    printf("\n- - - - - - - - - - - - - - - - - - - - - - -\n");
    printf("           %s \n",headerTitle);
    printf("- - - - - - - - - - - - - - - - - - - - - - -\n\n");
}

void backToMenu(char * messageToShow) {
    printf("%s",messageToShow);
    _getch();
}

void gestionClientsMenueScreen(int userRole) {
    header("GESTION DES CLIENTS");

    printf("\t   [1] Ajouter un nouveau client\n");
    printf("\t   [2] Afficher la liste des clients\n");
    printf("\t   [3] Modifier les informations\n");

    // Only show this if user is Admin
    if (userRole == 1) {
        printf("\t   [4] [ADMIN] Supprimer un client\n");
    }

    printf("\n\t   [0] Retour au menu principal\n");
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
    printf("\tVotre choix : ");
}

void gestionClientsMainFunction(int userRole) {
    int choix = -1; // Initialize to -1 so the loop starts
    char buffer[10];
    int maxOption = (userRole == 1) ? 4 : 3;

    do {
        // A. Clear and Show Menu
        system("cls");
        gestionClientsMenueScreen(userRole);

        // B. Get Input
        lireChaine(buffer, 10);

        // --- VALIDATION PHASE ---

        // Check 1: Empty Input
        if (buffer[0] == '\0') {
            printf("\n\t>> ERREUR : Aucune saisie detectee.\n");
            system("pause");
            continue; // Go back to start of loop (re-draw menu)
        }

        // Check 2: Numeric only
        int isNumeric = 1;
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (!isdigit(buffer[i])) {
                isNumeric = 0;
                break;
            }
        }

        if (isNumeric == 0) {
            printf("\a\n\t>> ERREUR : Saisie invalide. Entrez un NOMBRE.\n");
            system("pause");
            continue; // Go back to start of loop
        }

        // Check 3: Range
        choix = atoi(buffer);
        if (choix < 0 || choix > maxOption) {
            printf("\a\n\t>> ERREUR : Choix invalide (0 - %d).\n", maxOption);
            system("pause");
            continue; // Go back to start of loop
        }

        // --- EXECUTION PHASE ---

        switch (choix) {
        case 1:
            system("cls");
            addClient();
            break;
        case 2:
            system("cls");
            displayClients();
            break;
        case 3:
            system("cls");
            modifyClient();
            break;
        case 4:
            // Security check is technically redundant here because maxOption 
            // prevents non-admins from entering 4, but good for safety.
            if (userRole == 1) {
                system("cls");
                deleteClient();
            }
            break;
        case 0:
            // Do nothing. The loop condition (choix != 0) will handle the exit.
            break;
        }
        if (choix != 0) {
            backToMenu("Press any key to go back to GESTION DES CLIENTS MENU...");
        }

    } while (choix != 0); // Keep looping until user explicitly types 0
}

int loginSystem() {
    User uInput, uFile;
    int tentatives = 0;
    int connecte = 0; // 0 = Echec, Sinon renvoie le Rôle (1 ou 2)

    header("Login Screen");


    do {
        printf("Entrer userName : ");
        lireChaine(uInput.username, 20);
        printf("Entrer Password : ");
        lireChaine(uInput.password, 20); // Note: En console standard, le mot de passe s'affiche. 
        // Pour le cacher (****), il faut des bibliothèques spécifiques (conio.h).

        FILE* f = fopen(usersFile, "rb");
        int found = 0;

        while (fread(&uFile, sizeof(User), 1, f)) {
            if (strcmp(uInput.username, uFile.username) == 0 && strcmp(uInput.password, uFile.password) == 0) {
                connecte = uFile.role; // On récupère le rôle (1 ou 2)
                found = 1;
                break;
            }
        }
        fclose(f);

        if (found) {
           // printf("\n>> Connexion reussie ! Bienvenue %s.\n", uInput.username);
            return connecte; // On renvoie le rôle
        }
        else {
            tentatives++;
            printf(">> Erreur : Identifiants incorrects. (%d/3 essais)\n", tentatives);
        }

    } while (tentatives < 3);

    printf(">> Trop d'erreurs. Fermeture du programme.\n");
    return 0; // Echec total
}

int main() {
    int choixPrincipal, sousChoix;
    char buffer[10];
    int userRole = 0; // Stockera le rôle de la personne connectée

    // 1. LA SECURITE D'ABORD
    userRole = loginSystem();

    if (userRole == 0) {
        return 0; // On quitte si le login a échoué
    }

    // Initialisation random
    srand(time(NULL));

    do {
        system("cls");
        printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("\t\tSYSTEME DE FACTURATION (%s)\n", (userRole == 1) ? "ADMIN" : "AGENT");
        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
        printf("        [1] Gestion des Clients\n");
        printf("        [2] Gestion des Compteurs\n");
        printf("        [3] Gestion des Factures & Paiements\n");

        // OPTION RESERVEE AUX ADMINS
        if (userRole == 1) {
            printf("        [4] ADMINISTRATION (Gestion Utilisateurs)\n");
        }

        printf("        [0] Quitter\n");
        printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");

        // Dynamic prompt to show the correct range depending on user role
        if (userRole == 1) {
            printf("CHOOSE WHAT DO YOU WANT ? [0 TO 4] ? ");
        }
        else {
            printf("CHOOSE WHAT DO YOU WANT ? [0 TO 3] ? ");
        }

        lireChaine(buffer, 10);
        if (!estToutChiffres(buffer)) continue;
        choixPrincipal = atoi(buffer);

        switch (choixPrincipal) {
        case 1: // CLIENTS
            system("cls");
            gestionClientsMainFunction(userRole);
            break;

        case 2: // COMPTEURS
            header("Compteurs Menu");
            printf("1. Ajouter\n2. Afficher\n");
            if (userRole == 1) printf("3. [ADMIN] Supprimer\n");

            printf("Choix : ");
            lireChaine(buffer, 10);
            sousChoix = atoi(buffer);

            if (sousChoix == 1) ajouterCompteur();
            else if (sousChoix == 2) afficherCompteurs();
            else if (sousChoix == 3) {
                if (userRole == 1) supprimerCompteur();
                else printf(">> ACCES REFUSE.\n");
            }
            break;

        case 3: // FACTURATION
            // Tout le monde peut facturer et encaisser
            printf("\n--- MENU FACTURATION ---\n");
            header("Facturation Menu");
            printf("1. Etablir une facture\n2. Enregistrer un Paiement\n3. Historique\n4. Re-Imprimer\nChoix : ");
            lireChaine(buffer, 10);
            sousChoix = atoi(buffer);
            if (sousChoix == 1) ajouterFacture();
            else if (sousChoix == 2) payerFacture();
            else if (sousChoix == 3) afficherFactures();
            else if (sousChoix == 4) {
                printf("ID Facture : "); lireChaine(buffer, 20);
                genererFacturePDF(atoi(buffer));
            }
            break;

        case 9: // MENU ADMINISTRATION
            if (userRole == 1) {
                ajouterUtilisateur();
            }
            else {
                printf(">> Choix invalide.\n");
            }
            break;

        case 0: printf("Deconnexion...\n"); break;
        default: printf("Choix invalide.\n");
        }
    } while (choixPrincipal != 0); 



    return 0;
}