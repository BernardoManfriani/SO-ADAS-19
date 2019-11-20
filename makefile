# prova e allFiles eseguono lo stesso comando
prova: hmi.c ecu.c
	gcc hmi.c -o hmi
	gcc ecu.c -o ecu

prova2: hmi.c ecu.c
	gcc hmi.c -o hmi
	gcc ecu.c -o ecu
	gcc frontWindshieldCamera.c -o fwc

allFiles:
	gcc hmi.c -o hmi
	gcc ecu.c socketManager.c -o ecu
	gcc frontWindshieldCamera.c socketManager.c fileManager.c -o fwc
	gcc forwardFacingRadar.c socketManager.c -o ffr
	gcc blindSpot.c socketManager.c -o bs
	gcc parkAssist.c socketManager.c -o pa
	gcc surroundViewCameras.c socketManager.c -o svc
	gcc throttleControl.c socketManager.c fileManager.c -o tc
