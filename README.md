# SO-ADAS-19
Advanced Driver Assistance Systems

03.01.2019

CAMBIAMENTI FATTI :

   - ECU: 

      - Da front a steer lungo la socket vengono passati oltre a destra e sinistra anche i valori delle velocità      
      - Ho ripulito un po' il codice ordinando i metodi
      
   - STEER:
    
      - Riceve o un valore o Destra e Sinistra, dunque scriverà NO ACTION, DESTRA o SINISTRA.
      
PROBLEMI:

   - Lanciando il codice di andre rimangono tanti processi aperti
   - HMI non si chiude mai da sola
   - Steer funziona apparentemente sia con la read bloccante ch enon bloccante
    
