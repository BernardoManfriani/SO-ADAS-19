allFiles:
	gcc hmi.c -o hmi
	gcc ecu.c socketManager.c fileManager.c -o ecu
	gcc frontWindshieldCamera.c socketManager.c fileManager.c -o fwc
	gcc blindSpot.c socketManager.c fileManager.c -o bs
	gcc parkAssist.c socketManager.c fileManager.c -o pa
	gcc surroundViewCameras.c socketManager.c fileManager.c -o svc
	gcc steerByWire.c socketManager.c fileManager.c -o sbw
	gcc brakeByWire.c socketManager.c fileManager.c -o bbw
	gcc throttleControl.c socketManager.c fileManager.c -o tc
	gcc forwardFacingRadar.c socketManager.c fileManager.c -o fvc
	
