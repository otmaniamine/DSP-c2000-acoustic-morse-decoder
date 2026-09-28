% Programme MATLAB pour convertir du texte en code Morse et générer un signal carré
clear all;
close all;
clc;

% Définition du dictionnaire Morse (caractères français supportés)
morseDict = containers.Map('KeyType', 'char', 'ValueType', 'char');
morseDict('A') = '.-';
morseDict('B') = '-...';
morseDict('C') = '-.-.';
morseDict('D') = '-..';
morseDict('E') = '.';
morseDict('F') = '..-.';
morseDict('G') = '--.';
morseDict('H') = '....';
morseDict('I') = '..';
morseDict('J') = '.---';
morseDict('K') = '-.-';
morseDict('L') = '.-..';
morseDict('M') = '--';
morseDict('N') = '-.';
morseDict('O') = '---';
morseDict('P') = '.--.';
morseDict('Q') = '--.-';
morseDict('R') = '.-.';
morseDict('S') = '...';
morseDict('T') = '-';
morseDict('U') = '..-';
morseDict('V') = '...-';
morseDict('W') = '.--';
morseDict('X') = '-..-';
morseDict('Y') = '-.--';
morseDict('Z') = '--..';
morseDict('0') = '-----';
morseDict('1') = '.----';
morseDict('2') = '..---';
morseDict('3') = '...--';
morseDict('4') = '....-';
morseDict('5') = '.....';
morseDict('6') = '-....';
morseDict('7') = '--...';
morseDict('8') = '---..';
morseDict('9') = '----.';
morseDict(' ') = '/';
morseDict('É') = '..-..';
morseDict('È') = '.-..-';
morseDict('Ê') = '-..-';
morseDict('À') = '.--.-';
morseDict('Ç') = '-.-..';
morseDict('Ù') = '..--';
morseDict('Û') = '..--';
morseDict('Î') = '..-.';
morseDict('Ô') = '---.';

% Paramètres du signal
dotDuration = 0.1;  % durée d'un point en secondes
dashDuration = 3 * dotDuration;  % durée d'un tiret (3 fois la durée d'un point)
symbolSpacing = dotDuration;  % espace entre symboles (même caractère)
letterSpacing = 3 * dotDuration;  % espace entre lettres
wordSpacing = 7 * dotDuration;  % espace entre mots
samplingFreq = 1000;  % fréquence d'échantillonnage en Hz pour le signal de base

% Paramètres audio
audioToneFreq = 800;  % fréquence du son pour le code Morse (Hz)
audioSamplingFreq = 44100;  % fréquence d'échantillonnage pour l'audio (Hz)

% Demander à l'utilisateur d'entrer du texte
inputText = input('Entrez votre texte à convertir en Morse: ', 's');
inputText = upper(inputText);  % Convertir en majuscules pour la correspondance avec le dictionnaire

% Convertir le texte en code Morse
morseCode = '';
for i = 1:length(inputText)
    if isKey(morseDict, inputText(i))
        morseCode = [morseCode morseDict(inputText(i)) ' '];
    else
        fprintf('Caractère non supporté: %s (ignoré)\n', inputText(i));
    end
end

% Afficher le code Morse
fprintf('Code Morse: %s\n', morseCode);

% Générer le signal carré
timeStep = 1/samplingFreq;
totalTime = 0;

% Calculer d'abord la durée totale estimée
for i = 1:length(morseCode)
    if morseCode(i) == '.'
        totalTime = totalTime + dotDuration;
    elseif morseCode(i) == '-'
        totalTime = totalTime + dashDuration;
    elseif morseCode(i) == ' '
        totalTime = totalTime + letterSpacing - symbolSpacing;  % Ajuster car on ajoute déjà symbolSpacing après chaque symbole
    elseif morseCode(i) == '/'
        totalTime = totalTime + wordSpacing - letterSpacing;  % Ajuster car on ajoute déjà letterSpacing après chaque lettre
    end
    
    % Ajouter l'espace entre les symboles (sauf pour les espaces et slashs)
    if morseCode(i) == '.' || morseCode(i) == '-'
        totalTime = totalTime + symbolSpacing;
    end
end

% Créer les vecteurs de temps et de signal
t = 0:timeStep:totalTime;
signal = zeros(size(t));

% Générer le signal
currentTime = 0;
signalInfo = struct('symbols', {}, 'startTimes', {}, 'durations', {}, 'values', {});
symbolCount = 0;

for i = 1:length(morseCode)
    if morseCode(i) == '.'
        % Définir les indices pour ce point
        startIndex = round(currentTime/timeStep) + 1;
        duration = dotDuration;
        endIndex = round((currentTime + duration)/timeStep);
        if endIndex > length(signal)
            endIndex = length(signal);
        end
        
        % Mettre le signal à 1 pour la durée du point
        signal(startIndex:endIndex) = 1;
        
        % Stocker les informations du symbole
        symbolCount = symbolCount + 1;
        signalInfo(symbolCount).symbols = '.';
        signalInfo(symbolCount).startTimes = currentTime;
        signalInfo(symbolCount).durations = duration;
        signalInfo(symbolCount).values = 1;
        
        currentTime = currentTime + dotDuration + symbolSpacing;
        
    elseif morseCode(i) == '-'
        % Définir les indices pour ce tiret
        startIndex = round(currentTime/timeStep) + 1;
        duration = dashDuration;
        endIndex = round((currentTime + duration)/timeStep);
        if endIndex > length(signal)
            endIndex = length(signal);
        end
        
        % Mettre le signal à 1 pour la durée du tiret
        signal(startIndex:endIndex) = 1;
        
        % Stocker les informations du symbole
        symbolCount = symbolCount + 1;
        signalInfo(symbolCount).symbols = '-';
        signalInfo(symbolCount).startTimes = currentTime;
        signalInfo(symbolCount).durations = duration;
        signalInfo(symbolCount).values = 1;
        
        currentTime = currentTime + dashDuration + symbolSpacing;
        
    elseif morseCode(i) == ' '
        % Espace entre lettres (on a déjà ajouté symbolSpacing après le dernier symbole)
        additionalSpacing = letterSpacing - symbolSpacing;
        currentTime = currentTime + additionalSpacing;
        
        % Stocker les informations de l'espace entre lettres
        symbolCount = symbolCount + 1;
        signalInfo(symbolCount).symbols = ' ';
        signalInfo(symbolCount).startTimes = currentTime - additionalSpacing;
        signalInfo(symbolCount).durations = additionalSpacing;
        signalInfo(symbolCount).values = 0;
        
    elseif morseCode(i) == '/'
        % Espace entre mots (on a déjà ajouté letterSpacing après la dernière lettre)
        additionalSpacing = wordSpacing - letterSpacing;
        currentTime = currentTime + additionalSpacing;
        
        % Stocker les informations de l'espace entre mots
        symbolCount = symbolCount + 1;
        signalInfo(symbolCount).symbols = '/';
        signalInfo(symbolCount).startTimes = currentTime - additionalSpacing;
        signalInfo(symbolCount).durations = additionalSpacing;
        signalInfo(symbolCount).values = 0;
    end
end

% Afficher le signal
figure;
subplot(2,1,1);
plot(t, signal, 'LineWidth', 2);
title('Signal Carré du Code Morse');
xlabel('Temps (s)');
ylabel('Amplitude');
grid on;
ylim([-0.1, 1.1]);

% Générer le signal audio
audioFreq = 800;  % Fréquence du son (Hz) - 800Hz est standard pour le code Morse
audioSamplingFreq = 44100;  % Fréquence d'échantillonnage standard pour l'audio
audioTimeStep = 1/audioSamplingFreq;
audioTime = 0:audioTimeStep:totalTime;
audioSignal = zeros(size(audioTime));

% Générer le son - une onde sinusoïdale modulée par le signal carré
for i = 1:length(audioTime)
    t_current = audioTime(i);
    idx = floor(t_current / timeStep) + 1;
    if idx <= length(signal)
        if signal(idx) == 1
            audioSignal(i) = sin(2*pi*audioFreq*t_current);
        end
    end
end

% Normaliser le signal audio
audioSignal = 0.9 * audioSignal / max(abs(audioSignal));

% Afficher le signal audio
subplot(2,1,2);
plot(audioTime(1:min(10000, length(audioTime))), audioSignal(1:min(10000, length(audioSignal))), 'LineWidth', 1);
title('Signal Audio du Code Morse (premières 10,000 échantillons)');
xlabel('Temps (s)');
ylabel('Amplitude');
grid on;

% Jouer le son
sound(audioSignal, audioSamplingFreq);

% Sauvegarder le signal audio dans un fichier WAV pour une utilisation future
audiowrite('morse_audio.wav', audioSignal, audioSamplingFreq);

% Afficher le tableau des informations du signal
fprintf('\nTableau des informations du signal:\n');
fprintf('Symbole\tTemps début\tDurée\tValeur\n');
for i = 1:length(signalInfo)
    fprintf('%s\t%.3f\t\t%.3f\t%d\n', signalInfo(i).symbols, signalInfo(i).startTimes, ...
        signalInfo(i).durations, signalInfo(i).values);
end

% Créer un tableau pour l'exportation vers C
morseData = struct('morse_code', morseCode, 'dot_duration', dotDuration, ...
    'dash_duration', dashDuration, 'symbol_spacing', symbolSpacing, ...
    'letter_spacing', letterSpacing, 'word_spacing', wordSpacing, ...
    'signal_data', signalInfo);

% Sauvegarder les données pour une utilisation ultérieure en C
save('morse_data.mat', 'morseData', 'audioSignal', 'audioSamplingFreq');

% Exportation des données du signal au format CSV pour faciliter l'importation en C
timeVector = t';
signalVector = signal';
csvData = [timeVector, signalVector];
csvwrite('morse_signal.csv', csvData);

% Exporter un échantillon du signal audio au format CSV pour référence
audioSample = audioSignal(1:10:min(44100, length(audioSignal)));  % Prendre un échantillon réduit
audioTimeStep = audioTime(1:10:min(44100, length(audioTime)));
audioCSV = [audioTimeStep', audioSample'];
csvwrite('morse_audio_sample.csv', audioCSV);

fprintf('\nLes données ont été sauvegardées dans morse_data.mat, morse_signal.csv et morse_audio.wav\n');
fprintf('Le signal audio a été sauvegardé dans morse_audio.wav (pour transmission via jack casque)\n');
fprintf('Vous pouvez maintenant utiliser ces fichiers pour le traitement en C sur votre carte DSP\n');