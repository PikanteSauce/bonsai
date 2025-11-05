/************** Logique d'arrosage pour le bonsai/la plante  **************/

/* 
  1. Implémenter une horloge pour n'arroser qu'une ou deux fois par jour
  2. Si humidité < Seuil bas => arroser la plante
    a. Mettre en place un hystérésis ou bien une quantité d'eau prédéfinie
  3. Implémentation mode automatique
      Arrosage des plantes par lecture humidité et décision système
      Lecture des capterus toutes les 1 minutes
  4. Implémentation mode manuel 
      Activer arrosage 
      Visualisation niveau d'humidité
  5. Répartir serveur sur un core et l'autre core avec les tâches (capteurs actionneurs sur l'autres
    => VOIR freeeRTOS AMP et SMP
    FreeRTOS ports that support Symmetric Multi Processing (SMP) allow different tasks to run simultaneously on
    multiple cores of the same CPU. For these ports, you may specify which core a task will run on by using
    functions with "Affinity" in the name



















*/
