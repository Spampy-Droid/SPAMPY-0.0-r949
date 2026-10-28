SPAMPY — r923 con correzione arpeggiatore (identificativo FM-1_926)

Base esatta richiesta: 22spampy-su2.7.1-114-suoni-r923.fwsc.
SHA256 base: 41c2bbbc615efe7ce96dd165be9d921fdc0f68fe30618dd28fdc78c78c5905e7

Scorrendo i preset, ciascun suono carica il proprio arpeggiatore: DREAM ARP conserva il suo arp di fabbrica, i suoni senza arp lo spengono. Le impostazioni e le note trattenute dall’arp del suono precedente non si trasferiscono al successivo. Vale anche per i preset USER caricati dal banco. L’arp acceso manualmente non viene più mantenuto passando ad un altro preset; si applicano le impostazioni del suono scelto. Il KBD HOLD con arp spento mantiene il comportamento della r923.

114 suoni melodici più GM KIT, drum, volumi e motori audio della r923 invariati. Nessuna modifica importata dalle r924/r925. Le sole modifiche nei sorgenti del firmware sono ui.c e upreset.c; il nuovo identificativo e ABOUT distinguono questo aggiornamento dalla base.

Verifiche: riprodotto il difetto sulla base; corretti tutti i 115 caricamenti, arp attivo/spento, impostazioni, note trattenute e scorrimento rapido; un core e due core CHAOS=7; caricamento USER con record simulati; conservato il KBD HOLD. Audio dei 115 preset verificato bit per bit contro la r923 con arp disattivato. Caricamento/salvataggio dei 115 preset, 39 pagine UI, CRC del pacchetto, simulazione OTA e copertura di tutte le 568 funzioni da parte della protezione PC superati. Non provato fisicamente sull’FM-1.

Installare il file allegato e riselezionare un preset dal banco. Non è necessario cancellare progetti o preset USER. I progetti salvati mantengono il proprio stato arp quando vengono aperti; la correzione si applica al successivo cambio di suono.

Firmware: 22spampy-su2.7.1-114-suoni-r923-fix-arp-r926.fwsc
SHA256: 30960392789b252f88d36acf8507f4ef0752da695418124f3093ea5dff9e9fae
Applicazione: 521796 byte, 59768 byte liberi nello slot.
