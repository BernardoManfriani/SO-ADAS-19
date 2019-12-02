# SO-ADAS-19
Advanced Driver Assistance Systems

Done 1:
- Merge braches: funzioni di creazione per ogni sensore (modularità). Nessun processo zombie.
- Creazioni librerie: socketManager, fileManager

Done 2:
2.	Aggiunta lib per fare wait(NULL) in throttleControl.c
5.	Sposto la creazione di ECU_CLIENT e ECU_SERVER: da else metodo creaSensori() -> al metodo start(), sotto createSensori() e createAttuatori()
6.	Rimosso ciclo printf() da sensori
7.	Creare variabile globale(?) per char *argv[2]
8.	Aggiunto in ecu.c i vari pid dei processi che si occuperanno di fare da ECU-CLIENT per gli attuatori
9.	Aggiunto in ecu.c il metodo createEcuClients() che crea i processi descritti in (8.)
10.	Rinominati nomi metodi creazione sensori e attuatori (mi confendevano)
12. -------------------------   ATTENZIONE a tc. Sembra che ogni tantoo rimane un processo attivo -------------------
13. Creato metodo template createComponente() creazione sensori e attuatori
14.	PORCO DIO alcune volte ECU SERVER legge dal socket dai sensori, altre no. Voglio bestemmiare. STARE ATTENTI 
15. Eliminato fork() all'interno ddel while(1) nei metodi createServer() (ecu e attuatori)
16. Aggiunto e modificato in socketManager connectClient() - faccio un do{connect()}while(result == -1){}
17. ------------------------------- steerByWire non legge nulla da socket, come se non si blocasse su la read e uscisse subito
