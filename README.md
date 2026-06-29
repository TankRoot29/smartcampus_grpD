# 🚗 IoT Smart Parking & Edge Security Monitoring System

[![ESP32](https://img.shields.io/badge/ESP32-IoT-blue.svg?style=flat-square&logo=espressif)](https://wokwi.com/projects/467281479121231873)
[![MQTT](https://img.shields.io/badge/MQTT-HiveMQ-green.svg?style=flat-square&logo=mqtt)](https://www.hivemq.com/)
[![Node-RED](https://img.shields.io/badge/Node--RED-ETL-red.svg?style=flat-square&logo=node-red)](https://nodered.org/)
[![InfluxDB](https://img.shields.io/badge/InfluxDB_2.7-TimeSeries-blue.svg?style=flat-square&logo=influxdb)](https://www.influxdata.com/)
[![Grafana](https://img.shields.io/badge/Grafana-Monitoring-orange.svg?style=flat-square&logo=grafana)](https://grafana.com/)
[![Docker](https://img.shields.io/badge/Docker-Containerized-blue.svg?style=flat-square&logo=docker)](https://www.docker.com/)

---

## 📖 Présentation

**IoT Smart Parking & Edge Security Monitoring System** est une solution IoT de bout en bout pour la gestion automatisée, la supervision environnementale et la sécurisation physique (Edge IDS) d’un parking connecté.

Le cœur matériel repose sur un microcontrôleur **ESP32** (simulé sur [Wokwi](https://wokwi.com/projects/467281479121231873)) configuré en mode **Edge Computing** : il gère en temps réel les capteurs de présence ultrasoniques, les barrières d'accès (servomoteurs), l'affichage local (LCD I2C) et un système d'alerte sonore et visuel. 

Les données de télémétrie et de sécurité sont encapsulées au format **JSON**, puis publiées via le protocole **MQTT** vers un broker cloud. Un pipeline **Node-RED** hébergé localement extrait, valide et formate ces flux avant de les insérer dans une base de données temporelle **InfluxDB**. Enfin, un tableau de bord **Grafana** offre une observabilité complète du système (occupation du parking, conditions environnementales et détection d'intrusions).

---

## 🎯 Objectifs du Projet

* **Automatiser** le comptage des véhicules et le contrôle d'accès au parking (barrières intelligentes).
* **Superviser** en temps réel l'occupation et les conditions environnementales (température et humidité via DHT22).
* **Sécuriser** le site grâce à un **Edge IDS (Intrusion Detection System)** capable de détecter localement des fraudes ou forçages.
* **Orchestrer** une infrastructure de supervision conteneurisée via **Docker Compose** pour un déploiement agile.
* **Maîtriser** la chaîne complète IoT : Matériel $\rightarrow$ Transport Réseau $\rightarrow$ Pipeline ETL $\rightarrow$ Stockage Temporel $\rightarrow$ Visualisation.

---

## 🏗️ Architecture Détaillée du Système

Le diagramme suivant illustre l'architecture distribuée et le flux de données du système :

```text
  ┌────────────────────────────────────────────────────────┐
  │                   ESP32 (Simulé Wokwi)                 │
  │                                                        │
  │   [HC-SR04 Entrée] ──┐                     ┌── [LCD]   │
  │   [HC-SR04 Sortie] ──┼───► [Edge IDS] ─────┼── [LEDs]  │
  │   [DHT22 Météo]    ──┘                     └── [Buzzer]│
  │                                                        │
  │   [Servomoteur Entrée] ◄───────────────────────────┐   │
  │   [Servomoteur Sortie] ◄───────────────────────────┘   │
  └───────────────────────────┬────────────────────────────┘
                              │
                              │ MQTT (JSON)
                              │ Port 1883
                              ▼
  ┌────────────────────────────────────────────────────────┐
  │                 HiveMQ Cloud Public Broker             │
  │                   (broker.hivemq.com)                  │
  └───────────────────────────┬────────────────────────────┘
                              │
                              │ MQTT sur TLS (Port 8883)
                              ▼
  ┌────────────────────────────────────────────────────────┐
  │                 CONTAINERS DOCKER COMPOSE              │
  │                                                        │
  │  ┌──────────────┐      ┌──────────────┐  ┌──────────┐  │
  │  │   Node-RED   │ ───► │   InfluxDB   │  │ Mosquitto│  │
  │  │ (ETL Flow)   │      │  (Bucket v2) │  │  (Local) │  │
  │  └──────┬───────┘      └──────┬───────┘  └──────────┘  │
  │         │                     │                        │
  │         │                     ▼                        │
  │         │              ┌──────────────┐                │
  │         └─────────────►│   Grafana    │                │
  │                        │ (Dashboards) │                │
  │                        └──────────────┘                │
  └────────────────────────────────────────────────────────┘
```

> [!NOTE]  
> **Hybridation Cloud / Local :** Dans notre implémentation, l'ESP32 simulé en ligne sur Wokwi transmet ses données au broker public HiveMQ. Le pipeline Node-RED conteneurisé en local y souscrit via une connexion sécurisée (TLS - Port 8883) pour intégrer les métriques dans notre base InfluxDB locale. Un broker Mosquitto local sécurisé est également provisionné dans Docker Compose pour des déploiements 100% sur réseau local.

---

## ✨ Fonctionnalités et Logique Edge

### 🚘 Gestion et Contrôle d'Accès du Parking
* **Entrée :** Détection automatique d'un véhicule à l'entrée par capteur ultrasonique $\rightarrow$ Activation sonore $\rightarrow$ Décrémentation des places libres $\rightarrow$ Ouverture du servomoteur d'entrée ($90^\circ$) pendant 3 secondes $\rightarrow$ Fermeture automatique.
* **Sortie :** Détection d'un véhicule sortant par capteur ultrasonique $\rightarrow$ Incrémentation des places libres $\rightarrow$ Ouverture du servomoteur de sortie ($90^\circ$) pendant 3 secondes $\rightarrow$ Fermeture automatique.
* **Affichage Local :** Écran LCD 16x2 actualisé en temps réel indiquant le nombre de places libres et l'état ("Disponible" ou "PARKING PLEIN").

### 🚦 Signalisation & Alertes Locales
* **LED Verte :** Allumée tant qu'il reste au moins une place disponible.
* **LED Rouge :** Allumée dès que le parking atteint sa capacité maximale (10 places).
* **Buzzer :** Émet un bip court lors des validations d'accès et s'active en mode d'alarme lors des anomalies.

### 🌡️ Supervision Environnementale
* Le capteur **DHT22** mesure périodiquement la température et l'humidité ambiantes.
* Envoi des données toutes les 5 secondes via MQTT.
* Génération automatique d'alertes instantanées si $T > 35^\circ\text{C}$ ou $H > 80\%$.

### 🛡️ Edge Intrusion Detection System (Edge IDS)
L'ESP32 intègre une intelligence locale pour identifier les fraudes physiques à la barrière sans attendre le traitement serveur :
1. **Tentative d'accès sur parking plein :** Si une voiture se présente devant le capteur d'entrée alors que le nombre de places libres est égal à $0$ $\rightarrow$ La barrière d'entrée reste fermée $\rightarrow$ Alarme visuelle et sonore locale (clignotement rapide LED Rouge + Buzzer pulsé) $\rightarrow$ Publication immédiate d'une alerte IDS sur le topic `ufhb/grp_D/ids/alertes` et technique sur `ufhb/grp_D/alertes`.
2. **Anomalie à la sortie :** Si une présence est détectée à la sortie alors que le parking est déjà réputé vide (`placesLibres == PLACES_MAX`) $\rightarrow$ Le système identifie une incohérence de comptage ou un forçage suspect de la barrière de sortie $\rightarrow$ Publication immédiate d'une alerte IDS.

---

## 📂 Structure du Projet

```text
Projet-IoT---parking-iot-service/
│
├── captures/                  # Captures d'écran et médias (Grafana, Node-RED, Wokwi)
├── docker/                    # Fichiers de configuration de la pile de services
│   ├── influxdb/
│   │   └── .env               # Configuration de l'initialisation automatique d'InfluxDB v2
│   ├── mosquitto/
│   │   └── config/
│   │       ├── mosquitto.conf # Fichier de configuration du broker Mosquitto local
│   │       └── passwd         # Fichier d'authentification des utilisateurs Mosquitto
│   ├── nodered/
│   │   ├── flows.json         # Définition des flux ETL Node-RED
│   │   ├── flows_cred.json    # Identifiants chiffrés des flux
│   │   └── settings.js        # Paramètres d'exécution de l'instance Node-RED
│   └── docker-compose.yml     # Orchestration multi-conteneurs de l'infrastructure
│
├── esp32/                     # Code source et documentation matérielle
│   ├── diagram.json           # Configuration du circuit électronique sur Wokwi
│   ├── library.txt            # Liste des dépendances de bibliothèques Wokwi
│   ├── lien-wokwi.txt         # Lien d'accès direct au simulateur en ligne
│   └── sketch.ino             # Code C++ Arduino principal de l'ESP32
│
├── grafana/                   # Configurations et modèles de Dashboards exportés
├── rapport/                   # Documents de présentation et rapport final (.md)
└── README.md                  # Documentation principale
```

---

## 📡 Topologie MQTT & Payloads

### 1. Topics de Communication
Tous les topics respectent la structure hiérarchique recommandée pour le projet :

| Catégorie | Topic MQTT | Payload Type | Description |
| :--- | :--- | :--- | :--- |
| **Télémétrie Parking** | `ufhb/grp_D/capteurs/parking` | JSON | Places libres, température, humidité et statut. |
| **Alertes Techniques** | `ufhb/grp_D/alertes` | JSON | Alertes météo ($T$, $H$) ou anomalies système. |
| **Statut Système** | `ufhb/grp_D/statut` | Chaîne | Statut LWT de l'ESP32 (`En-ligne` / `Hors-ligne`). |
| **Événements Parking** | `ufhb/grp_D/evenements` | JSON | Historique des accès (`entree_vehicule` / `sortie_vehicule`). |
| **Alertes IDS** | `ufhb/grp_D/ids/alertes` | JSON | Signalement immédiat d'une intrusion ou fraude physique. |

### 2. Exemples de Payloads JSON

* **Télémétrie périodique :**
  ```json
  {
    "places_libres": 8,
    "temperature": 26.5,
    "humidite": 64.2,
    "statut_parking": "DISPONIBLE",
    "timestamp": 125000
  }
  ```

* **Alerte IDS (Intrusion à l'entrée) :**
  ```json
  {
    "alerte": "INTRUSION_DETECTEE",
    "anomalie": "Forcage de barriere d'entree (Parking Satire)",
    "distance_cm": 45.2,
    "places_libres": 0,
    "timestamp": 248600
  }
  ```

---

## 🔌 Raccordement Matériel (ESP32 Pinout)

| Broche ESP32 | Direction | Composant | Description |
| :---: | :---: | :--- | :--- |
| **GPIO 5** | Sortie | HC-SR04 Entrée (Trigger) | Impulsion ultrason d'entrée |
| **GPIO 18** | Entrée | HC-SR04 Entrée (Echo) | Lecture du temps de retour entrée |
| **GPIO 19** | Sortie | HC-SR04 Sortie (Trigger) | Impulsion ultrason de sortie |
| **GPIO 23** | Entrée | HC-SR04 Sortie (Echo) | Lecture du temps de retour sortie |
| **GPIO 13** | Sortie | Servomoteur Entrée (PWM) | Contrôle barrière d'accès entrée |
| **GPIO 14** | Sortie | Servomoteur Sortie (PWM) | Contrôle barrière d'accès sortie |
| **GPIO 15** | Entrée/Sortie | DHT22 (Data) | Capteur de température & humidité |
| **GPIO 12** | Sortie | Buzzer | Alarme sonore locale |
| **GPIO 2** | Sortie | LED Verte | Indicateur places disponibles |
| **GPIO 4** | Sortie | LED Rouge | Indicateur parking complet / Alarme |
| **I2C SDA (GPIO 21)** | E/S | LCD 16x2 (SDA) | Ligne de données écran LCD |
| **I2C SCL (GPIO 22)** | E/S | LCD 16x2 (SCL) | Ligne d'horloge écran LCD |

---

## 🚀 Guide de Déploiement

### Prérequis
* [Docker Desktop](https://www.docker.com/products/docker-desktop/) installé et actif.
* Un accès Internet (pour la simulation Wokwi et le broker public HiveMQ).

### Étape 1 : Cloner le Répertoire
```bash
git clone https://github.com/TankRoot29/Projet-IoT---parking-iot-service.git
cd Projet-IoT---parking-iot-service/docker
```

### Étape 2 : Lancer l'Infrastructure Docker Compose
Démarrez l'ensemble des conteneurs (Mosquitto, InfluxDB, Node-RED, Grafana) en arrière-plan :
```bash
docker compose up --build -d
```

### Étape 3 : Accéder aux Interfaces de Services

| Service | Adresse Locale | Identifiants par défaut |
| :--- | :--- | :--- |
| **Node-RED** | [http://localhost:1880](http://localhost:1880) | *Sans authentification par défaut* |
| **InfluxDB** | [http://localhost:8086](http://localhost:8086) | Utilisateur : `admin` / Password : `smartcampus2025` |
| **Grafana** | [http://localhost:3000](http://localhost:3000) | Utilisateur : `admin` / Password : `admin` |

### Étape 4 : Exécuter la Simulation Matérielle
1. Ouvrez le projet sur le simulateur en ligne : [Wokwi Smart Parking ESP32](https://wokwi.com/projects/467281479121231873).
2. Cliquez sur le bouton de démarrage de la simulation (**Play**).
3. Modifiez la distance sur les capteurs ultrasoniques **HC-SR04** pour simuler le passage de voitures (Entrée ou Sortie) et observez la réaction des barrières, du LCD et des données envoyées.

---

## 🔀 Pipeline de Traitement Node-RED

Le flux configuré dans Node-RED implémente une logique de routage robuste :
1. **Souscription :** Écoute globale sur le pattern `ufhb/grp_D/#` (HiveMQ public, port 8883 TLS).
2. **Filtrage (Switch Node) :** Aiguillage des messages selon le sous-topic spécifique.
3. **Analyse JSON (JSON Node) :** Conversion du payload de chaîne de caractères en objet JavaScript.
4. **Calculs & Formatage (Function Node) :** Restructuration des payloads, typage strict des données pour assurer l'indexation correcte.
5. **Routage Temporel :** Injection des données dans la base InfluxDB locale (Organisation : `ufhb`, Bucket : `smartcampus`) sous trois mesures distinctes :
   * `parking` (places libres, occupation, statut).
   * `alertes` (notifications environnementales et techniques).
   * `alertes_ids` (intrusions et tentatives de forçage).

---

## 📊 Tableau de bord Grafana (Observabilité)

Le Dashboard Grafana est structuré en trois zones fonctionnelles clés :
1. **Statut du Parking (Temps Réel) :**
   * Panel SingleStat indiquant le nombre de places libres de couleur changeante (Vert $\rightarrow$ Orange $\rightarrow$ Rouge).
   * Graphique historique d'occupation sur les dernières 24 heures.
2. **Surveillance Environnementale :**
   * Courbes de température ($^\circ\text{C}$) et d'humidité relative ($\%$) avec seuils visuels d'alerte.
3. **Console IDS (Sécurité physique) :**
   * Tableau récapitulatif des alarmes (Type de fraude, distance d'interception détectée par le capteur et timestamp précis).
   * Jauge du nombre total d'anomalies détectées par jour.

---

## 👨‍💻 Équipe Projet - Groupe D

* **AGBENONZAN Kossivi Jacques Junior** *(GitHub: [JunRoot29](https://github.com/JunRoot29))*
* **KONE Kpantieri**
* **HORO Désiré**

### 🎓 Cadre Académique
* **Université :** Université Félix Houphouët-Boigny – Abidjan, Côte d'Ivoire.
* **UFR :** Mathématiques et Informatique.
* **Classe :** Licence 3 Réseaux, Sécurité et Télécommunications (L3 RIST).
* **Module :** Création de Services Réseaux et IoT.
* **Superviseur Académique :** Dr. Konaté.

---

## 📜 Licence
Projet académique à but éducatif réalisé en 2026. Code source sous licence libre de diffusion académique.
