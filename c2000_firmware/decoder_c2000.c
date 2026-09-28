/*
 * Décodeur Morse pour DSP C2000
 * 
 * Ce code reçoit un signal audio Morse depuis l'entrée ADC,
 * détecte les points, tirets et espaces, puis décode le message
 * en texte.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "DSP28x_Project.h"     // Inclure les fichiers d'en-tête spécifiques à C2000
 
 // Définitions et constantes
 #define BUFFER_SIZE         1024    // Taille du tampon pour les échantillons ADC
 #define SAMPLE_RATE         44100   // Fréquence d'échantillonnage (Hz)
 #define TONE_FREQ           800     // Fréquence attendue du signal Morse (Hz)
 #define THRESHOLD           2000    // Seuil pour la détection du signal (à ajuster selon votre ADC)
 #define MAX_MORSE_LENGTH    512     // Longueur maximale du code Morse
 #define MAX_TEXT_LENGTH     128     // Longueur maximale du texte décodé
 
 // Paramètres temporels (en secondes)
 #define DOT_DURATION        0.1     // Durée d'un point
 #define DASH_DURATION       0.3     // Durée d'un tiret
 #define DOT_DASH_THRESHOLD  0.2     // Seuil pour distinguer un point d'un tiret
 
 // Délais entre symboles (en secondes)
 #define SYMBOL_SPACE        0.1     // Espace entre symboles (point/tiret)
 #define LETTER_SPACE        0.3     // Espace entre lettres
 #define WORD_SPACE          0.7     // Espace entre mots
 #define LETTER_THRESHOLD    0.2     // Seuil pour distinguer un espace entre symboles et entre lettres
 #define WORD_THRESHOLD      0.5     // Seuil pour distinguer un espace entre lettres et entre mots
 
 // Variables globales
 Uint16 adcBuffer[BUFFER_SIZE];      // Tampon pour les échantillons ADC
 char morseBuffer[MAX_MORSE_LENGTH]; // Tampon pour le code Morse décodé
 char textBuffer[MAX_TEXT_LENGTH];   // Tampon pour le texte décodé
 Uint16 morseIndex = 0;              // Index dans le tampon Morse
 Uint16 textIndex = 0;               // Index dans le tampon texte
 
 // Structure pour les informations temporelles du signal
 typedef struct {
     float signalStart;      // Début du signal haut
     float signalEnd;        // Fin du signal haut
     float signalDuration;   // Durée du signal haut
     float gapStart;         // Début de l'espace
     float gapEnd;           // Fin de l'espace
     float gapDuration;      // Durée de l'espace
     int active;             // Indicateur de signal actif
 } SignalInfo;
 
 SignalInfo currentSignal;
 
 // Table de conversion Morse -> caractère
 typedef struct {
     char* morse;
     char character;
 } MorseCode;
 
 // Table de conversion du code Morse en caractères
 // Cette table contient les caractères standard et les caractères français
 const MorseCode morseTable[] = {
     {".-", 'A'},
     {"-...", 'B'},
     {"-.-.", 'C'},
     {"-..", 'D'},
     {".", 'E'},
     {"..-.", 'F'},
     {"--.", 'G'},
     {"....", 'H'},
     {"..", 'I'},
     {".---", 'J'},
     {"-.-", 'K'},
     {".-..", 'L'},
     {"--", 'M'},
     {"-.", 'N'},
     {"---", 'O'},
     {".--.", 'P'},
     {"--.-", 'Q'},
     {".-.", 'R'},
     {"...", 'S'},
     {"-", 'T'},
     {"..-", 'U'},
     {"...-", 'V'},
     {".--", 'W'},
     {"-..-", 'X'},
     {"-.--", 'Y'},
     {"--..", 'Z'},
     {"-----", '0'},
     {".----", '1'},
     {"..---", '2'},
     {"...--", '3'},
     {"....-", '4'},
     {".....", '5'},
     {"-....", '6'},
     {"--...", '7'},
     {"---..", '8'},
     {"----.", '9'},
     {"..-..","É"},     // Caractères accentués français
     {".-..-","È"},
     {"-..-", "Ê"},
     {".--.-","À"},
     {"-.-..","Ç"},
     {"..--", "Ù"},
     {"..-.","Î"},
     {"---.","Ô"},
     {"", ' '}          // Espace (représenté par un '/' en Morse)
 };
 
 #define MORSE_TABLE_SIZE (sizeof(morseTable) / sizeof(MorseCode))
 
 // Prototypes de fonctions
 void initSystem(void);
 void initADC(void);
 void sampleADC(void);
 void processSignal(void);
 void decodeMorse(void);
 char lookupMorseChar(const char* morseCode);
 void displayResult(void);
 void configureGPIO(void);
 void setupTimer(void);
 void sendToUART(char* text);
 
 // Fonction principale
 void main(void)
 {
     // Initialisation du système et des périphériques
     initSystem();
     initADC();
     configureGPIO();
     setupTimer();
     
     // Configuration UART pour afficher les résultats (si disponible)
     // InitUARTStdio(); // Fonction spécifique à votre configuration C2000
     
     // Initialisation des tampons
     memset(adcBuffer, 0, sizeof(adcBuffer));
     memset(morseBuffer, 0, sizeof(morseBuffer));
     memset(textBuffer, 0, sizeof(textBuffer));
     
     // Initialisation de la structure de signal
     currentSignal.active = 0;
     currentSignal.signalStart = 0;
     currentSignal.signalEnd = 0;
     currentSignal.signalDuration = 0;
     currentSignal.gapStart = 0;
     currentSignal.gapEnd = 0;
     currentSignal.gapDuration = 0;
     
     // Message de démarrage
     printf("Décodeur Morse pour DSP C2000 démarré\n");
     printf("En attente de signal...\n");
     
     // Boucle principale
     while(1)
     {
         // Échantillonnage ADC
         sampleADC();
         
         // Traitement du signal pour extraire le code Morse
         processSignal();
         
         // Décodage du Morse en texte
         decodeMorse();
         
         // Affichage du résultat
         displayResult();
         
         // Délai (pour éviter de surcharger le CPU)
         DELAY_US(1000); // Délai de 1ms
     }
 }
 
 // Initialisation du système
 void initSystem(void)
 {
     // Initialisation du système C2000
     InitSysCtrl();
     
     // Désactiver les interruptions CPU
     DINT;
     
     // Initialiser l'accès à la mémoire flash
     InitFlash();
     
     // Initialiser les interruptions PIE
     InitPieCtrl();
     IER = 0x0000;
     IFR = 0x0000;
     InitPieVectTable();
     
     // Activer les interruptions CPU
     EINT;
 }
 
 // Initialisation de l'ADC
 void initADC(void)
 {
     // Configuration de l'ADC pour le DSP C2000
     InitAdc();  // Fonction spécifique à C2000
     
     // Configuration du canal ADC (ajuster selon votre brochage)
     // Exemple pour ADCINA0 (canal 0)
     AdcRegs.ADCCTL1.bit.ADCREFSEL = 0;  // Référence interne
     AdcRegs.ADCCTL1.bit.ADCPWDN = 0;    // Allumer l'ADC
     AdcRegs.ADCCTL1.bit.ADCENABLE = 1;  // Activer l'ADC
     
     AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;   // Canal A0
     AdcRegs.ADCSOC0CTL.bit.ACQPS = 20;  // Temps d'acquisition (cycles)
     AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 1; // Déclenchement logiciel
     
     // Délai pour stabilisation de l'ADC
     DELAY_US(1000);
 }
 
 // Échantillonnage ADC
 void sampleADC(void)
 {
     static Uint16 bufferIndex = 0;
     
     // Démarrer la conversion ADC
     AdcRegs.ADCSOCFRC1.all = 0x0001;    // Forcer SOC0
     
     // Attendre la fin de la conversion
     while(AdcRegs.ADCINTFLG.bit.ADCINT1 == 0);
     
     // Effacer le flag d'interruption
     AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
     
     // Lire la valeur ADC
     adcBuffer[bufferIndex] = AdcResult.ADCRESULT0;
     
     // Incrémenter l'index du tampon
     bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
 }
 
 // Traitement du signal pour détecter les points, tirets et espaces
 void processSignal(void)
 {
     static float currentTime = 0;
     static Uint16 prevValue = 0;
     static int expectingLowSignal = 0;
     static int expectingHighSignal = 1;
     
     // Incrémenter le temps (basé sur la fréquence d'échantillonnage)
     currentTime += 1.0f / SAMPLE_RATE;
     
     // Lire la dernière valeur échantillonnée
     Uint16 currentValue = adcBuffer[BUFFER_SIZE - 1];
     
     // Détection de signal basée sur le seuil
     int signalDetected = (currentValue > THRESHOLD);
     
     // Détection des flancs (montant et descendant)
     if (!currentSignal.active && signalDetected && expectingHighSignal) {
         // Flanc montant détecté - début d'un signal (point ou tiret)
         currentSignal.active = 1;
         currentSignal.signalStart = currentTime;
         expectingHighSignal = 0;
         expectingLowSignal = 1;
     }
     else if (currentSignal.active && !signalDetected && expectingLowSignal) {
         // Flanc descendant détecté - fin d'un signal
         currentSignal.active = 0;
         currentSignal.signalEnd = currentTime;
         currentSignal.signalDuration = currentSignal.signalEnd - currentSignal.signalStart;
         currentSignal.gapStart = currentTime;
         expectingHighSignal = 0;
         expectingLowSignal = 0;
         
         // Classez le signal comme un point ou un tiret
         if (currentSignal.signalDuration < DOT_DASH_THRESHOLD) {
             // Point détecté
             if (morseIndex < MAX_MORSE_LENGTH - 1) {
                 morseBuffer[morseIndex++] = '.';
             }
             printf("Point détecté (%.3fs)\n", currentSignal.signalDuration);
         } else {
             // Tiret détecté
             if (morseIndex < MAX_MORSE_LENGTH - 1) {
                 morseBuffer[morseIndex++] = '-';
             }
             printf("Tiret détecté (%.3fs)\n", currentSignal.signalDuration);
         }
     }
     else if (!currentSignal.active && !signalDetected && !expectingHighSignal && !expectingLowSignal) {
         // Mesure de la durée de l'espace
         float gapDuration = currentTime - currentSignal.gapStart;
         
         // Détection de l'espace entre les lettres
         if (gapDuration > LETTER_THRESHOLD && gapDuration < WORD_THRESHOLD) {
             currentSignal.gapEnd = currentTime;
             currentSignal.gapDuration = gapDuration;
             expectingHighSignal = 1;
             
             // Ajouter un espace pour séparer les caractères Morse
             if (morseIndex < MAX_MORSE_LENGTH - 1) {
                 morseBuffer[morseIndex++] = ' ';
             }
             printf("Espace entre lettres détecté (%.3fs)\n", gapDuration);
         }
         // Détection de l'espace entre les mots
         else if (gapDuration >= WORD_THRESHOLD) {
             currentSignal.gapEnd = currentTime;
             currentSignal.gapDuration = gapDuration;
             expectingHighSignal = 1;
             
             // Ajouter une barre oblique pour représenter l'espace entre les mots
             if (morseIndex < MAX_MORSE_LENGTH - 1) {
                 morseBuffer[morseIndex++] = '/';
                 morseBuffer[morseIndex++] = ' ';
             }
             printf("Espace entre mots détecté (%.3fs)\n", gapDuration);
         }
         // Si la durée est suffisante pour considérer le prochain signal
         else if (gapDuration > SYMBOL_SPACE) {
             expectingHighSignal = 1;
         }
     }
     
     // Mettre à jour la valeur précédente
     prevValue = currentValue;
 }
 
 // Décodage du Morse en texte
 void decodeMorse(void)
 {
     // Si le tampon morse contient des données
     if (morseIndex > 0) {
         char tempMorse[32] = {0};  // Tampon temporaire pour un caractère morse
         int tempIndex = 0;         // Index dans le tampon temporaire
         
         // Réinitialiser le tampon texte
         memset(textBuffer, 0, sizeof(textBuffer));
         textIndex = 0;
         
         // Parcourir le tampon morse
         for (int i = 0; i <= morseIndex; i++) {
             if (morseBuffer[i] == '.' || morseBuffer[i] == '-') {
                 // Ajouter le point ou le tiret au tampon temporaire
                 if (tempIndex < 31) {
                     tempMorse[tempIndex++] = morseBuffer[i];
                 }
             }
             else if (morseBuffer[i] == ' ' || morseBuffer[i] == '\0') {
                 // Fin d'un caractère Morse
                 if (tempIndex > 0) {
                     tempMorse[tempIndex] = '\0';
                     char decodedChar = lookupMorseChar(tempMorse);
                     
                     // Ajouter le caractère décodé au tampon texte
                     if (textIndex < MAX_TEXT_LENGTH - 1) {
                         textBuffer[textIndex++] = decodedChar;
                     }
                     
                     // Réinitialiser le tampon temporaire
                     memset(tempMorse, 0, sizeof(tempMorse));
                     tempIndex = 0;
                 }
             }
             else if (morseBuffer[i] == '/') {
                 // Espace entre les mots
                 if (textIndex < MAX_TEXT_LENGTH - 1) {
                     textBuffer[textIndex++] = ' ';
                 }
             }
         }
         
         // Terminer la chaîne de texte
         textBuffer[textIndex] = '\0';
         
         // Réinitialiser le tampon morse pour le prochain message
         memset(morseBuffer, 0, sizeof(morseBuffer));
         morseIndex = 0;
     }
 }
 
 // Recherche d'un caractère dans la table Morse
 char lookupMorseChar(const char* morseCode)
 {
     for (int i = 0; i < MORSE_TABLE_SIZE; i++) {
         if (strcmp(morseTable[i].morse, morseCode) == 0) {
             return morseTable[i].character;
         }
     }
     
     // Caractère non trouvé
     return '?';
 }
 
 // Affichage du résultat
 void displayResult(void)
 {
     // Si le tampon texte contient des données
     if (textIndex > 0) {
         // Afficher le texte décodé
         printf("Texte décodé : %s\n", textBuffer);
         
         // Envoyer le texte via UART (si disponible)
         sendToUART(textBuffer);
     }
 }
 
 // Configuration des GPIO
 void configureGPIO(void)
 {
     // Configuration de la broche de sortie pour debug (LED)
     EALLOW;
     GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;   // GPIO0 comme GPIO
     GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;    // GPIO0 comme sortie
     GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Éteindre la LED
     EDIS;
 }
 
 // Configuration du timer
 void setupTimer(void)
 {
     // Configuration du Timer0 pour le timing précis
     // Cette fonction dépend de votre configuration C2000
     // InitCpuTimers();
     // ConfigCpuTimer(&CpuTimer0, 150, 1000); // Timer à 1ms
     // StartCpuTimer0();
 }
 
 // Envoi du texte via UART
 void sendToUART(char* text)
 {
     // Cette fonction dépend de votre configuration C2000
     // Exemple d'implémentation:
     
     if (text != NULL) {
         int i = 0;
         while (text[i] != '\0') {
             // Envoyer le caractère via UART
             // SciaSendByte(text[i]);
             i++;
         }
         
         // Envoyer un retour à la ligne
         // SciaSendByte('\r');
         // SciaSendByte('\n');
     }
 }
 
 // Interruption ADC (si nécessaire)
 // __interrupt void adcIsr(void)
 // {
 //     // Code d'interruption ADC...
 //     
 //     // Acquitter l'interruption
 //     AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
 //     PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
 // }