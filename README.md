# SO-ADAS-19
Advanced Driver Assistance Systems

Done 1:
- Merge braches: funzioni di creazione per ogni sensore (modularità). Nessun processo zombie.
- Creazioni librerie: socketManager, fileManager

Done 2:
- Aggiunta lib per fare wait(NULL) in throttleControl.c
- Sposto la creazione di ECU_CLIENT e ECU_SERVER: da else metodo creaSensori() -> al metodo start(), sotto createSensori() e createAttuatori()
- Rimosso ciclo printf() da sensori
- Creare variabile globale(?) per char *argv[2]
- Aggiunto in ecu.c i vari pid dei processi che si occuperanno di fare da ECU-CLIENT per gli attuatori
- Aggiunto in ecu.c il metodo createEcuClients() che crea i processi descritti in (8.)
- Rinominati nomi metodi creazione sensori e attuatori (mi confendevano)
- ATTENZIONE a tc. Sembra che ogni tanto rimanga un processo attivo
- Creato metodo template createComponente() creazione sensori e attuatori
- PORCO DIO alcune volte ECU SERVER legge dal socket dai sensori, altre no. Voglio bestemmiare. STARE ATTENTI 
- Eliminato fork() all'interno ddel while(1) nei metodi createServer() (ecu e attuatori)
- Aggiunto e modificato in socketManager connectClient() - faccio un do{connect()}while(result == -1){}
- PROBLEMA DA SOLVERE: steerByWire non legge nulla da socket, come se non si blocasse su la read e uscisse subito
