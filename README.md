# SO-ADAS-19
Advanced Driver Assistance Systems

2.	Aggiunta lib per fare wait(NULL) in throttleControl.c
5.	Sposto la creazione di ECU_CLIENT e ECU_SERVER: da else metodo creaSensori() -> al metodo start(), sotto createSensori() e createAttuatori()
6.	Rimosso ciclo printf() da sensori
7.	Creare variabile globale(?) per char *argv[2]
8.	Aggiunto in ecu.c i vari pid dei processi che si occuperanno di fare da ECU-CLIENT per gli attuatori
9.	Aggiunto in ecu.c il metodo createEcuClients() che crea i processi descritti in (8.)
10.	Rinominati nomi metodi creazione sensori e attuatori (mi confendevano)
12. -------------------------   ATTENZIONE a tc. Sembra che ogni tantoo rimane un processo attivo -------------------
13. Creato metodo template createComponente() creazione sensori e attuatori
14.	PORCO DIO alcune volte ECU SERVER legge dal socket dai sensori, altre no. Voglio bestemmiare. CONTROLLARE 
15. Eliminato fork() all'interno ddel while(1) nei metodi createServer() (ecu e attuatori)
16. Aggiunto e modificato in socketManager connectClient() - faccio un do{connect()}while(result == -1){}

---------------------------------------------------------------------------------------------------------------------------------------

-	Aggiungo sensore ffr -> ecu server legge suo dato e scrive su pipe a processo ffrDataManager -> quest'ultimo controlla che ci siano quella 		  		sequenza di caratteri e in tal caso manda PERICOLO a bbw
- Tolto else da crea server, processo figlio si occupa del primo sensore e il padre continua ad eseguire cicli for per altri sensori
- creo/modifico metodo ffrDataManager
- inizializzo pipe,  apro e chiudo lati pipe in creaServers e sensoriDataManager
- forse MAX_DATA_SIZE = 20 non è sufficiente. ffr legge e spedisce a ecu 24 byte
- SONO BLOCCATO: i manager in ecu rimangono bloccati nel while(read su pipe)


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


----------------- 29/12/2019 ----------------------
- FAR FUNZIONARE INDIPENDENTEMENTE FFR
FFR:
	il componente complessivamente funziona, ci sono pero dei problemi a cui stare attenti. Quando viene codificato 
	un valore maligno in quello che legge ffr viene alzato un pericolo e tutto si ferma fino a che non si inserisce
	nuovamente INIZIO.
	A ripetere piu volte questo giro(anche una volta o poche), ECU_SERVER aspetta indefinitivamente (o tanto tempo) di
	leggere qualcosa da ffr.
	Sembra quindi che FFR non riesc piu a leggere dal file; non so se perchè ci sono errori nella lettura o non riesce
	ad aprire il file. Per
	questo proposito è probabile che non riesca perchè il file rimane aperto (nessuno lo chiude perchè il processo
	viene interrotto prima) e 
	fopen ha problemi ad aprirlo. BHO!
- PROVARE BRAKE BY WIRE CON SOCKET (SCRIVERE ANCHE THROTTLE CONTROL), SCRIVERE IN INPUT ESCLUSIVAMENTE NUMERI

----------------- 30/12/2019 ----------------------
- BBW e TC funzionano quando sono insieme. Va bene se stampano NO ACTION anche quando il server non è stato inizializzato?

	
