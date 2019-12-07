# C_backdoor
Une backdoor démonisée écrite en C, fontionne sous Linux

### Ce que j'ai appris ###
  - Comment faire un daemon
  
  -Gérer les sockets en C
  
  -Gérer des process en C (via popen)
  
### Comment fontionne-t-elle ###
  - Changez la constante PORT qui correspond au port d'écoute de la backdoor
  
  - Compilez le code
  
  - Lancez le 
  
  - Connectez vous à celle-ci via un utilitaire réseau (netcat fera parfaitement l'affaire)
  
### Mettre fin à une connexion ###
  - La commande "end" permet d'arrêter de communiquer avec la backdoor (mais si vous fermez brusquement la connexion la backdoor ne bug pas)
  
  - La commande "shutdown_backdoor" permet d'arrêter la backdoor

Comme d'habitude, récupérez et améliorez ce code afin qu'il vive longtemps ^^
Si vous souhaitez me joindre :
  -aiglematth@protonmail.com
