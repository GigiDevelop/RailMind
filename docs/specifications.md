# Specifiche Provvisorie

## Architettura Generale

- Diorama ferroviario con locomotive intelligenti.
- Binari alimentati a 18 V DC stabile.
- I binari forniscono alimentazione, non comandi di marcia.
- Direzione, velocita, accelerazione e frenata sono calcolate a bordo locomotiva.
- Comunicazione radio tramite ESP-NOW.
- Centrale principale su PC/server con CPU i9.
- Gateway iniziale basato su ESP32-S2.
- Possibile evoluzione a tre gateway S2:
  - trenini
  - luci
  - attivita diorama e scambi

## Locomotiva

La locomotiva contiene due ESP:

- ESP32-S3: cervello della locomotiva.
- ESP32-C3: attuatore locale per motore e luci.

### ESP32-S3

- Alimentato in standby dai 18 V dei binari tramite alimentatore dedicato.
- Comunica con la centrale tramite ESP-NOW.
- Riceve comando di accensione dalla centrale.
- Avvia il motore elettrico della dinamo.
- Controlla il segnale di alimentazione principale stabile.
- Abilita il C3 quando il sistema principale e pronto.
- Legge il sensore posizione IR sotto la locomotiva.
- Gestisce lookup table velocita/posizione.
- Calcola direzione, velocita target, accelerazione e frenata.
- Invia comandi operativi al C3 tramite I2C.
- Invia telemetria al server.

### ESP32-C3

- Riceve comandi dall'S3 tramite I2C.
- Controlla il ponte H a MOSFET per il motore di trazione.
- Gestisce luci frontali e posteriori.
- Gestisce eventuali rele o uscite accessorie locali.
- Riporta stato, fault e misure all'S3.

## Priorita S3

Essendo dual core, l'S3 separa le attivita:

- Alta priorita:
  - lettura posizione
  - calcolo movimento
  - controllo velocita target
  - comandi al C3
- Priorita inferiore:
  - ESP-NOW
  - telemetria
  - diagnostica
  - aggiornamenti configurazione
  - logiche accessorie

## Alimentazione

- Ponte H trazione alimentato direttamente dai 18 V dei binari.
- Elettronica principale alimentata da piccola dinamo collegata a motore elettrico.
- L'S3 resta alimentato in standby dai binari.

Sequenza di accensione:

1. Binari presenti a 18 V.
2. S3 acceso in standby.
3. Centrale invia comando di accensione.
4. S3 abilita motore dinamo.
5. S3 attende alimentazione principale stabile.
6. S3 abilita C3 e periferiche.
7. C3 gestisce motore trazione, luci e attuatori.

## Marker IR

- Marker ogni 20 cm sui binari.
- Ogni marker usa un ATtiny13.
- Trasmissione IR continua modulata a 38 kHz.
- Velocita dati: 4800 baud.
- Pacchetto:

```text
AA 55 ID_LOW ID_HIGH CRC8
```

- ID a 16 bit, little-endian.
- CRC8 per validazione.
- Il pacchetto dura circa 10,4 ms in seriale 8N1.
- Due pacchetti richiedono circa 20,8 ms.
- Obiettivo zona utile IR: almeno 2 cm.

## Numerazione Marker

- Numerazione tramite linea fisica di chaining.
- Ogni modulo ha:
  - CHAIN_IN
  - CHAIN_OUT
  - GND comune
  - eventuale linea dati/configurazione
- Alla prima configurazione ogni ATtiny salva il proprio ID in EEPROM.
- Se EEPROM contiene ID valido, il normale avvio non lo sovrascrive.

## Reset Marker

- Reset globale per rinumerare tutta la linea.
- Reset locale per sostituire un marker.
- Reset protetto contro attivazioni accidentali.
- Possibili protezioni:
  - pin fisico
  - ponticello
  - sequenza speciale su linea dati
  - comando mantenuto per alcuni secondi

## Server Centrale

- Riceve telemetria da locomotive e gateway.
- Visualizza stato live su console.
- Espone interfaccia web locale.
- Gestisce percorsi del treno.
- Gestisce scambi.
- Gestisce luci case/strade in base all'ora del giorno.
- Mantiene configurazioni e storico eventi.

