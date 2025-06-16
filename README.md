
ESP32 Web Server - Monitorizare Temperatură, Inundație și Control LED
===============================================================================

Acest proiect pentru ESP32 creează o interfață web în rețea locală care permite:

 - Monitorizarea temperaturii prin senzor analogic conectat la pinul 32
 - Detectarea inundațiilor prin senzor conectat la pinul 33
 - Controlul unui LED prin buton fizic sau interfață web
 - Trimiterea automată a alertelor către un Logic App în Azure
 - Trimiterea și salvarea ultimelor 10 mesaje text
 - Afișarea ultimelor 10 alerte de inundație cu oră și valoare brută

-------------------------------------------------------------------------------
PINOUT ESP32
-------------------------------------------------------------------------------
 - GPIO 14  -> Buton fizic
 - GPIO 27  -> LED (ieșire)
 - GPIO 32  -> Senzor temperatură (analogic)
 - GPIO 33  -> Senzor inundație (analogic)

-------------------------------------------------------------------------------
CONFIGURARE Wi-Fi
-------------------------------------------------------------------------------
Înlocuiește în cod datele rețelei tale:

  const char* ssid = "Nume_Retea";
  const char* password = "Parola_Retea";

-------------------------------------------------------------------------------
ALERTE LOGIC APP (AZURE)
-------------------------------------------------------------------------------
Dacă senzorul de inundație depășește valoarea 3000, se trimite automat
un mesaj JSON către un webhook Logic App:

  URL: const char* logicAppUrl = "https://...";
  Body JSON: { "event": "flood", "value": <valoare_senzor> }

-------------------------------------------------------------------------------
INTERFAȚA WEB
-------------------------------------------------------------------------------
 - Status LED: Pornit / Oprit
 - Buton fizic: APĂSAT / NEAPĂSAT
 - Temperatura (°C)
 - Valoare brută senzor apă
 - Trimitere mesaj text (max. 100 caractere)
 - Afișare mesaje salvate
 - Afișare alerte inundație salvate

Pagina se actualizează automat la fiecare secundă cu cele mai noi date.

-------------------------------------------------------------------------------
MEMORARE ȘI PERSISTENȚĂ
-------------------------------------------------------------------------------
Sunt salvate în memorie non-volatilă (Preferences):

 - msg0 - msg9      = ultimele 10 mesaje trimise de utilizator
 - alert0 - alert9  = ultimele 10 alerte de inundație cu oră și valoare

Mesajele pot fi șterse manual din interfață.

-------------------------------------------------------------------------------
INSTRUCȚIUNI DE PORNIRE
-------------------------------------------------------------------------------
1. Încarcă codul pe ESP32 folosind Arduino IDE sau PlatformIO.
2. Conectează ESP32 la rețeaua Wi-Fi definită.
3. Deschide Monitorul Serial și copiază adresa IP afișată.
4. Accesează acea adresă IP din browser (ex: http://192.168.1.123).
5. Interacționează cu interfața web (status, LED, mesaje, alerte).

-------------------------------------------------------------------------------
OBSERVAȚII
-------------------------------------------------------------------------------
 - Pragul pentru detecția de inundație este 3000 (poate fi ajustat).
 - Temperatura este citită brut și estimată (necalibrată).
 - Interfața web este compatibilă cu browserul pe PC sau telefon.


