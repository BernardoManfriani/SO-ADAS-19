# prova e allFiles eseguono lo stesso comando
prova: hmi.c ecu.c
	gcc hmi.c -o hmi
	gcc ecu.c -o ecu

allFiles:
	gcc hmi.c -o hmi
	gcc ecu.c -o ecu
