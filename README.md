# SO-ADAS-19
Advanced Driver Assistance Systems

------------------ 27/12/2019 -------------------
- modifiche brake by wire
	-aggiunto pid ecu
	-aggiunto SIGPARK
	-manageSocketData gestisce caso in cui riceve "PARCHEGGIO"
- modifiche ecu:
	-aggiunto metodo decodeFfrData
	-aggiunto pid hmi
	-qunado legge PERICOLO avverte ffr e avverte hmi
- modifiche hmi:
	-tolto break da metodo start, per poter gestire il caso in cui ci sia un pericolo
	-aggiunta una signal su SIGDANGER -> hmi chiude tutti i processi e aspetta che venga reinserito INIZIO
	-aggiunto setgpid() per poter uccidere tutti i processi creati in ECU in solo colpo (uccidendo il capogruppo ECU uccido tutti)
- VORREI INIZIARE A PENSARE COME GESTIRE LE DUE MODALITA DI AVVIO NORMALE E ARTIFICIALE


----------------- 28/12/2019 -----------------
- modifiche ecu
	-aggiunta variabile globale per tenere salvato la modalità di avvio del progetto
	-passare la modalità di avvio come argomento nelle exec dei processi che ne hanno realmente bisogno => add argomenti nei main di tali processi
	-aggiunto segnale define SIGPARK SIGUSR1
	-aggiunto metodo parkingHandler
	-aggiunta signal(SIGPARK, parkingHandler) appena dopo l'arrivo di SIGSTART
	-rimosso parametro da metodo startEcuSigHandler
	-rimosso metodo test
- modifiche ffr
	-aggiunto controllo modalità di avvio => apertura file diversa
- modifiche hmi
	-aggiunto segnale define SIGPARK SIGUSR1
