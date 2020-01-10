# SO-ADAS-19
Advanced Driver Assistance Systems

03.01.2019

CAMBIAMENTI FATTI :

   - ECU: 

      - Da front a steer lungo la socket vengono passati oltre a destra e sinistra anche i valori delle velocità      
      - Ho ripulito un po' il codice ordinando i metodi
      
   - STEER:
    
      - Riceve o un valore o Destra e Sinistra, dunque scriverà NO ACTION, DESTRA o SINISTRA.
      
   - PARK
      - Dovrebbe funzionare anche se le stampe scazzano un po'. 
      - Non riesco a capire se passa la roba giusta alla ecu attraverso la socket
   - BLIND
      - Stessa zuppa di park.
      - Da testare meglio
      
PROBLEMI:

   - Lanciando il codice di andre rimangono tanti processi aperti
   - HMI non si chiude mai da sola
   - Steer funziona apparentemente sia con la read bloccante ch enon bloccante
   - Non esegue la exit a riga 44/45 dell'hmi.
   
    
ROBA FUNZIONANTE SUL MIO BRANCH:
   - Steer
   - Blind
   - Park
   
   
   
10.01.2019
ANNOTAZIONI:
   - kill(pid,SIGNAL) manda una signal ad un processo. Ma se questo processo non è in attesa viene ucciso. Potrebbe essere utlie per il progetto 
