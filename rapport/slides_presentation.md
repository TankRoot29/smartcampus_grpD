# 🚗 Présentation Projet : IoT Smart Parking & Edge Security
## *Système de Gestion et Supervision Sécurisée de Parking Connecté*

---

# 📑 Sommaire

1. Contexte & Problématique
2. Objectifs du Projet
3. Architecture Globale
4. Le Cœur Embarqué (ESP32)
5. Edge Intrusion Detection System (Edge IDS)
6. Protocole de Communication (MQTT)
7. Pipeline d'Intégration (Node-RED)
8. Stockage & Visualisation (InfluxDB & Grafana)
9. Déploiement DevOps (Docker)
10. Compétences Acquises & Perspectives
11. Conclusion

---

# 1. Contexte & Problématique

* **Urbanisation et Smart Campus :** Difficulté croissante pour trouver des places de parking disponibles en temps réel.
* **Sécurité Physique Insuffisante :** Risques d'intrusions, de forçage de barrières et de fraudes (ex: entrée forcée sur parking complet).
* **Conditions Environnementales :** Nécessité de superviser le microclimat du parking (prévention d'incendies ou d'inondations).
* **Besoin de Centralisation :** Centraliser les métriques éparses sur un tableau de bord unique d'observabilité.

---

# 2. Objectifs du Projet

* **Automatisation :** Gérer l'accès physique via des capteurs ultrasoniques et des barrières automatisées.
* **Sécurisation en périphérie (Edge Computing) :** Exécuter un algorithme de détection d'anomalies (IDS) directement sur l'ESP32.
* **Routage Fiable :** Transmettre et transformer les données via un bus MQTT sécurisé et un pipeline ETL Node-RED.
* **Stockage & Observabilité :** Historiser dans InfluxDB v2 et visualiser les métriques sur Grafana.
* **Industrialisation :** Containeriser l'ensemble des services via Docker Compose pour simplifier le déploiement.

---

# 3. Architecture Globale du Système

```text
  ┌────────────────────────────────────────────────────────┐
  │                   ESP32 (Simulé Wokwi)                 │
  │   [HC-SR04 / DHT22] ──► [Edge IDS] ──► [LCD / Buzzer]  │
  └───────────────────────────┬────────────────────────────┘
                              │ MQTT (Port 1883)
                              ▼
  ┌────────────────────────────────────────────────────────┐
  │               HiveMQ Cloud Public Broker               │
  └───────────────────────────┬────────────────────────────┘
                              │ MQTT sur TLS (Port 8883)
                              ▼
  ┌────────────────────────────────────────────────────────┐
  │               SERVICES LOCAUX DOCKER COMPOSE           │
  │  ┌──────────────┐      ┌──────────────┐  ┌──────────┐  │
  │  │   Node-RED   │ ───► │   InfluxDB   │  │ Mosquitto│  │
  │  │ (ETL Flow)   │      │  (Bucket v2) │  │  (Local) │  │
  │  └──────┬───────┘      └──────┬───────┘  └──────────┘  │
  │         └─────────────►│   Grafana    │                │
  │                        └──────────────┘                │
  └────────────────────────────────────────────────────────┘
```

---

# 4. Le Cœur Embarqué (ESP32)

* **Capteurs Utilisés :**
  * Deux capteurs ultrasoniques **HC-SR04** (gestion distincte Entrée et Sortie).
  * Un capteur thermo-hygrométrique **DHT22** (suivi environnemental).
* **Actionneurs et Affichage :**
  * Deux servomoteurs simulant l'ouverture/fermeture des barrières physiques.
  * Un écran LCD 16x2 I2C pour le guidage des conducteurs.
  * Des LEDs de signalisation (Verte = Libre, Rouge = Plein/Alerte).
  * Un Buzzer pour les confirmations sonores et les alarmes.
* **Environnement de Simulation :** Wokwi, permettant de valider le comportement matériel.

---

# 5. Edge Intrusion Detection System (Edge IDS)

> L'intelligence de sécurité est déportée au plus près des capteurs (Edge computing) pour des décisions en temps réel (< 50ms) sans dépendre du réseau.

### Scénarios de Détection d'Intrusions :
1. **Forçage Entrée (Parking Saturé) :** 
   * Présence détectée à l'entrée alors que `placesLibres == 0`.
   * *Action :* Barrière bloquée, alarme locale (clignotement LED rouge + buzzer pulsé) et publication d'une alerte IDS.
2. **Forçage Sortie (Parking Vide) :**
   * Présence détectée à la sortie alors que le parking est déjà réputé vide (`placesLibres == PLACES_MAX`).
   * *Action :* Envoi d'une alerte IDS pour suspicion d'accès frauduleux ou incohérence de comptage.

---

# 6. Protocole de Communication (MQTT)

* **Découplage et Temps Réel :** Modèle Publish/Subscribe idéal pour les réseaux IoT contraints.
* **Topics Hiérarchisés (Convention SmartCampus) :**
  * Télémétrie : `ufhb/grp_D/capteurs/parking` (places, température, humidité)
  * Événements : `ufhb/grp_D/evenements` (entrées/sorties)
  * Alertes IDS : `ufhb/grp_D/ids/alertes` (tentatives d'intrusion)
  * Statut (LWT) : `ufhb/grp_D/statut` (état en/hors ligne de l'ESP32)
* **Format des Données :** Payloads structurés en JSON pour faciliter le parsing ETL.

---

# 7. Pipeline d'Intégration (Node-RED)

* **Rôle d'ETL (Extract, Transform, Load) :**
  * Récupération des flux MQTT du broker HiveMQ Cloud.
  * Filtrage logique via des nœuds *Switch*.
  * Validation structurelle et conversion des types de données (String $\rightarrow$ Number).
  * Routage vers les bases de données correspondantes.
* **Pipeline de Sécurité Dédié :**
  * Traitement à part des alertes IDS pour archivage immédiat et prioritaire dans InfluxDB.

---

# 8. Stockage & Visualisation (InfluxDB & Grafana)

### InfluxDB v2 (Base Temporelle) :
* **Organisation :** `ufhb` | **Bucket :** `smartcampus`.
* Trois *measurements* principales : `parking`, `alertes`, `alertes_ids`.
* Avantage : Optimisée pour les requêtes à haute fréquence temporelle.

### Grafana (Supervision Professionnelle) :
* **Panels Temps Réel :** Niveau de places disponibles avec alertes de couleur.
* **Historiques :** Évolution temporelle de l'occupation du parking et des conditions météo ($T$ & $H$).
* **Console IDS :** Table de journalisation des intrusions recensées pour le personnel de sécurité.

---

# 9. Déploiement DevOps (Docker)

* **Fichier unique `docker-compose.yml` :** Gestion et isolation de l'infrastructure logicielle complète.
* **Services orchestrés :**
  * **Node-RED** (Traitement ETL)
  * **InfluxDB 2.7** (Stockage temporel avec auto-initialisation via variables d'environnement)
  * **Grafana** (Visualisation)
  * **Mosquitto** (Broker local optionnel pré-configuré pour la sécurité)
* **Persistance des Données :** Volumes Docker nommés pour assurer la sauvegarde des bases de données lors des redémarrages.

---

# 10. Compétences Acquises & Perspectives

### Compétences Démontrées :
* Développement C++ embarqué sous architecture ESP32.
* Protocoles IoT et réseaux (MQTT, TCP/IP, sérialisation JSON).
* Conception de bases de données temporelles et requêtes Flux.
* Administration système conteneurisée (Docker Compose).
* Principes d'observabilité et de cybersécurité physique (Edge IDS).

### Perspectives d'Évolutions :
* Chiffrement des communications MQTT via TLS sur l'ESP32.
* Intégration d'une caméra IP pour vérification visuelle par IA (reconnaissance de plaques d'immatriculation).
* Notifications d'alertes instantanées par Telegram ou WhatsApp.

---

# 11. Conclusion

* **Solution Fonctionnelle et Stable :** Répond entièrement au cahier des charges académique et professionnel.
* **Sécurité par le Edge :** L'IDS embarqué démontre l'intérêt de traiter les menaces localement pour limiter le temps de réaction.
* **Prêt pour la Production :** Grâce à l'architecture conteneurisée, la solution est facilement déployable sur un serveur sur site (Raspberry Pi) ou dans le cloud.

---

# 💬 Questions & Réponses

### Merci pour votre attention !

* **Auteurs :** 
  * AGBENONZAN Kossivi Jacques Junior
  * KONE Kpantieri
  * HORO Désiré
* **Superviseur :** Dr. Konaté
* **Université :** Université Félix Houphouët-Boigny - Abidjan
* **Lien Simulateur :** [Wokwi Smart Parking Project](https://wokwi.com/projects/467281479121231873)
