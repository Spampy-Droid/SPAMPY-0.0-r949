SPAMPY — correzione crash PROPHET r918

File: spampy-su2.7.1-prophet5-fix-crash-r918.fwsc
Identificativo FM-1_918; ABOUT: 10.0 / P5 B1.

Corretto un errore della prima integrazione: i kernel Prophet erano copiati in RAM e chiamati correttamente, ma si trovavano oltre il limite RAM autorizzato dalla protezione hardware del program counter. Il salto in queste funzioni può provocare la schermata SPAMPY CRASH. Nella r918 il codice è collocato all'inizio della regione RAM copiata (.data) e il limite PC comprende esplicitamente la fine di questi kernel. Le protezioni restano attive.

Regressione verificata sul binario ELF
Build precedente: p5_samples, p5_mod_samples, p5_dc_samples fuori dalle finestre PC.
Nuova build: tutte le 544 funzioni compilate coperte; zero funzioni escluse. Il controllo viene ora eseguito anche ad ogni compilazione.
CRC del pacchetto e dei componenti validi; simulazione OTA superata. Codice DSP, patch, 57 suoni precedenti, drum e loader identici nei sorgenti; i test audio e di integrazione precedenti restano applicabili perché non è cambiata la logica audio.

Non è stata eseguita una prova fisica sull'FM-1. Questa corregge un difetto confermato del binario; se il crash si ripresenta, una foto completa della schermata, inclusi PC/DBG e gli altri codici, è necessaria per individuare eventuali altri problemi.

Eseguire un backup prima di installare. Non usare la vecchia build FM-1_917. Per tornare alla base senza Prophet usare spampy-su2.7.1-57-suoni-ripristino.fwsc già fornito nella chat.

I cinque preset nativi restano: STABBY BRASS, FANTASTIC FIVE, TAARI CHOIR, REEDY STRING PA, BORN IN THE 80'. Nessun cambiamento al loro suono è stato introdotto con questa correzione.
