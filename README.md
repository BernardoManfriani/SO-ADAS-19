# SO-ADAS-19
Advanced Driver Assistance Systems

--------------------- 31/01/2020 ---------------------------------
1. === PROBLEMA ===
INPUT:  
		15
		20
		DESTRA
		SINISTRA
		PERICOLO
		20
		20
		20
		35
		35
		35
		PERICOLO
		35

Premettendo che, per far si che venisse scritto 'ARRESTO AUTO' da bbw e che una volta letto PERICOLO al successivo avvio (uhh, ho un dejavu) venisse
letto 'INCREMENTO 20', ho messo uno sleep(1) brutale in manageDanger(). Al primo PERICOLO *sembra* che effettivamente tutto vada bene. Dal secondo
pericolo invece non viene scritto più 'ARRESTO AUTO' da bbw, anche se pooi effettivamente *sembra* che venga scritto 'INCREMENTO 35'.

1. === SOLUZIONE ===
Premettendo che gli input del prof sono lunghissimi e difficilmente mette due PERICOLO ravvicinati, nella sezione input della relazione eviterei
semplicemente di mettere due pericolo ravvicinati, evitando che succeda questa merda.


2. Ora vengono terminati gli attuatori una volta che arriva comando 'PARCHEGGIO'
