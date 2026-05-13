# RailMind

RailMind e un sistema per diorama ferroviario smart con locomotive intelligenti,
marker IR sui binari, comunicazione ESP-NOW, gateway dedicati e server centrale
di supervisione.

L'obiettivo e separare alimentazione, controllo, posizione e supervisione:
i binari forniscono energia stabile, mentre ogni locomotiva calcola localmente
direzione, velocita e comportamento in base ai dati ricevuti dalla centrale e
alla posizione letta sul tracciato.

## Componenti

- `server/`: applicazione centrale Python/FastAPI.
- `firmware/s3/`: firmware ESP32-S3 a bordo locomotiva.
- `firmware/c3/`: firmware ESP32-C3 a bordo locomotiva.
- `firmware/attiny13/`: firmware marker IR sui binari.
- `docs/`: specifiche tecniche e note di progetto.

## Architettura Generale

- Binari alimentati a 18 V DC stabile.
- I binari sono usati come alimentazione, non come comando.
- La locomotiva calcola a bordo direzione, velocita, accelerazione e frenata.
- Comunicazione locomotiva-centrale tramite ESP-NOW.
- Server centrale su PC con CPU i9.
- Gateway iniziale basato su ESP32-S2.
- Possibile evoluzione a tre gateway separati:
  - trenini
  - luci
  - attivita diorama e scambi

## Locomotiva

Ogni locomotiva contiene due microcontrollori principali.

### ESP32-S3

L'ESP32-S3 e il cervello della locomotiva.

- Resta alimentato in standby dai 18 V dei binari tramite alimentatore dedicato.
- Comunica con la centrale tramite ESP-NOW.
- Riceve il comando di accensione.
- Avvia il motore elettrico della dinamo.
- Verifica il segnale di alimentazione principale stabile.
- Abilita il C3 e le periferiche quando il sistema e pronto.
- Legge il sensore posizione IR sotto la locomotiva.
- Gestisce la lookup table velocita/posizione.
- Calcola direzione, velocita target, accelerazione e frenata.
- Invia comandi operativi al C3 tramite I2C.
- Invia telemetria al server.

Essendo dual core, l'S3 separa le attivita:

- task ad alta priorita per posizione, movimento e comandi motore
- task a priorita inferiore per ESP-NOW, telemetria, diagnostica e configurazione

### ESP32-C3

L'ESP32-C3 e il controllore attuativo locale.

- Riceve comandi dall'S3 tramite I2C.
- Controlla il ponte H a MOSFET per il motore di trazione.
- Gestisce luci frontali e posteriori.
- Gestisce eventuali rele o uscite accessorie.
- Riporta stato, fault e misure all'S3.

## Alimentazione Locomotiva

- Ponte H trazione alimentato direttamente dai 18 V dei binari.
- Elettronica principale alimentata da una piccola dinamo collegata a un motore elettrico.
- S3 alimentato in standby direttamente dai binari.

Sequenza prevista:

1. I binari forniscono 18 V.
2. L'S3 si accende in standby.
3. La centrale invia il comando di accensione.
4. L'S3 abilita il motore della dinamo.
5. L'S3 attende alimentazione principale stabile.
6. L'S3 abilita C3 e periferiche.
7. Il C3 gestisce motore trazione, luci e attuatori.

## Marker IR

I marker di posizione sono distribuiti lungo il tracciato ogni 20 cm.

- Microcontrollore marker: ATtiny13.
- Trasmissione IR continua.
- Portante IR: 38 kHz.
- Velocita dati: 4800 baud.
- Pacchetto:

```text
AA 55 ID_LOW ID_HIGH CRC8
```

- ID marker a 16 bit, little-endian.
- CRC8 per validazione.
- Pacchetto da 5 byte, circa 10,4 ms in seriale 8N1.
- Obiettivo: almeno due letture complete durante il passaggio della locomotiva.
- Zona utile IR consigliata: almeno 2 cm.

## Numerazione Marker

La numerazione dei marker avviene tramite linea fisica di chaining.

Ogni modulo marker prevede:

- `CHAIN_IN`
- `CHAIN_OUT`
- `GND` comune
- eventuale linea dati/configurazione

Alla prima configurazione ogni ATtiny salva il proprio ID in EEPROM e abilita il
modulo successivo. Se l'EEPROM contiene gia un ID valido, il normale avvio non
lo sovrascrive.

E prevista una logica di reset:

- reset globale per rinumerare tutta la linea
- reset locale per sostituire un marker
- protezione contro reset accidentali tramite pin, ponticello o sequenza speciale

## Server Centrale

Il backend e pensato come applicazione web locale:

- Python
- FastAPI
- WebSocket per aggiornamenti live
- dashboard web responsive
- database locale da introdurre nelle prossime fasi

Funzioni previste:

- ricezione telemetria da locomotive e gateway
- visualizzazione live dello stato
- gestione percorsi
- gestione scambi
- gestione luci case/strade in funzione dell'ora del giorno
- storico eventi e diagnostica
- invio comandi verso gateway ESP32-S2

## Telemetria Prevista

Ogni locomotiva dovra inviare almeno:

- ID locomotiva
- stato alimentazione
- stato dinamo
- `power good`
- marker IR corrente
- direzione
- velocita target
- velocita stimata
- duty cycle PWM
- temperatura motore
- temperatura radiatore MOSFET
- stato luci
- errori sensore posizione
- errori comunicazione S3-C3
- errori ESP-NOW
- stato emergenza/freno/stop

## Interfaccia Web

La dashboard iniziale include:

- menu laterale responsive
- hamburger menu su mobile
- stato connessione WebSocket
- pannello locomotive
- pannello scambi
- pannello regole luci
- pannello eventi

## Obiettivi Prima Versione

1. Ricevere telemetria locomotiva sul server.
2. Visualizzare stato live via console e WebSocket.
3. Definire protocollo base tra server, gateway e locomotiva.
4. Preparare firmware separati per S3, C3 e ATtiny13.
5. Implementare marker IR a 4800 baud con pacchetto `AA 55 ID_LOW ID_HIGH CRC8`.

## Idee Preliminari

- Aggiungere firmware per gateway ESP32-S2.
- Introdurre mappa grafica del tracciato nel frontend.
- Gestire scambi con blocchi di percorso e interlocking.
- Aggiungere simulazione giorno/notte per luci case, strade e stazioni.
- Salvare configurazioni e telemetria in SQLite nella prima fase.
- Aggiungere profili di marcia per ogni locomotiva.
- Implementare modalita diagnostica per testare marker IR e comunicazione.
- Aggiungere una vista manutenzione per temperature, fault e stato alimentazione.

## Avvio Server

```bash
cd server
pip install -r requirements.txt
uvicorn app.main:app --reload
```

Poi aprire:

```text
http://localhost:8000
```

