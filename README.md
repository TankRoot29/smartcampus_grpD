# 🚗 IoT Parking Monitoring System

![IoT](https://img.shields.io/badge/IoT-ESP32-blue)
![MQTT](https://img.shields.io/badge/Protocol-MQTT-green)
![Prometheus](https://img.shields.io/badge/Monitoring-Prometheus-orange)
![Grafana](https://img.shields.io/badge/Dashboard-Grafana-yellow)
![Docker](https://img.shields.io/badge/Deployment-Docker-blue)

## 📖 Overview

**IoT Parking Monitoring System** est une solution complète de supervision d'un parking intelligent basée sur les technologies IoT, MQTT et l'observabilité moderne.

Le système détecte automatiquement l'entrée et la sortie des véhicules grâce à un ESP32 équipé de capteurs ultrasoniques. Les données sont transmises en temps réel via MQTT puis collectées par Prometheus avant d'être visualisées dans Grafana à travers un tableau de bord interactif.

L'objectif principal est de démontrer la mise en œuvre d'une chaîne IoT complète allant du capteur jusqu'à la supervision temps réel.

---

## 🏗️ Architecture

```text
┌──────────────────┐
│ HC-SR04 Sensors  │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ ESP32 Controller │
│ Barrier Control  │
└────────┬─────────┘
         │ MQTT
         ▼
┌──────────────────┐
│ HiveMQ Broker    │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Python Exporter  │
│ MQTT → Metrics   │
└────────┬─────────┘
         │ HTTP /metrics
         ▼
┌──────────────────┐
│ Prometheus       │
└────────┬─────────┘
         │ PromQL
         ▼
┌──────────────────┐
│ Grafana          │
│ Dashboard        │
└──────────────────┘
```

---

## ✨ Features

### 🚘 Smart Vehicle Detection

* Détection des véhicules à l'entrée et à la sortie.
* Ouverture automatique de la barrière.
* Gestion du trafic en temps réel.

### 🚦 Visual & Audio Signaling

* LED verte : accès autorisé.
* LED rouge : parking complet.
* Buzzer de signalisation.

### 🅿️ Parking Capacity Management

* Capacité maximale configurable.
* Comptage automatique des places disponibles.
* Blocage des nouvelles entrées lorsque le parking est saturé.

### 📡 MQTT Communication

* Transmission légère et temps réel.
* Architecture découplée.
* Compatible avec tout broker MQTT standard.

### 📊 Real-Time Monitoring

* Export des métriques au format Prometheus.
* Visualisation Grafana.
* Alertes visuelles par seuils.

---

## 🛠️ Technologies Used

| Category         | Technology     |
| ---------------- | -------------- |
| Embedded System  | ESP32          |
| Sensors          | HC-SR04        |
| Actuator         | Servo Motor    |
| Communication    | MQTT           |
| Broker           | HiveMQ         |
| Backend          | Python         |
| Monitoring       | Prometheus     |
| Visualization    | Grafana        |
| Containerization | Docker         |
| Orchestration    | Docker Compose |
| Simulation       | Wokwi          |

---

## 📂 Project Structure

```text
project-root/
│
├── mqtt_exporter/
│   ├── exporter.py
│   └── requirements.txt
│
├── prometheus/
│   └── prometheus.yml
│
├── docker-compose.yml
├── sketch.ino
└── README.md
```

---

## ⚙️ Installation

### 1. Clone Repository

```bash
git clone https://github.com/TankRoot29/Projet-IoT---parking-iot-service.git

cd Projet-IoT---parking-iot-service
```

### 2. Start Infrastructure

```bash
docker compose up --build
```

Services disponibles :

| Service          | URL                           |
| ---------------- | ----------------------------- |
| Grafana          | http://localhost:3000         |
| Prometheus       | http://localhost:9090         |
| Metrics Endpoint | http://localhost:8000/metrics |

---

## 🔧 ESP32 Configuration

### GPIO Mapping

| Component             | GPIO |
| --------------------- | ---- |
| HC-SR04 Entry Trigger | 5    |
| HC-SR04 Entry Echo    | 18   |
| HC-SR04 Exit Trigger  | 19   |
| HC-SR04 Exit Echo     | 23   |
| Servo Motor           | 13   |
| Buzzer                | 12   |
| Green LED             | 2    |
| Red LED               | 4    |

### Required Library

```text
PubSubClient
```

L'ESP32 se connecte automatiquement au réseau Wokwi puis au broker MQTT HiveMQ.

---

## 📊 Grafana Dashboard

### Data Source

Configurez Prometheus :

```text
http://prometheus:9090
```

### Main Metric

```promql
parking_places_libres
```

### Recommended Visualization

* Gauge Panel
* Maximum : 10
* Green : 7–10
* Yellow : 3–6
* Red : 0–2

---

## 🔍 Educational & Networking Concepts

Ce projet illustre plusieurs concepts essentiels :

### IoT Architecture

* Communication machine-to-machine (M2M)
* Edge Computing
* IoT Telemetry

### Network Engineering

* MQTT Publish/Subscribe
* Découplage des composants
* Communication temps réel

### Observability

* Monitoring
* Metrics Collection
* Data Visualization
* Time-Series Databases

### DevOps

* Docker
* Docker Compose
* Infrastructure as Code

---

## 🚀 Future Improvements

* Authentification MQTT sécurisée (TLS).
* Gestion multi-parkings.
* Notifications Telegram ou WhatsApp.
* Alertes Prometheus.
* Application mobile.
* Historisation des données dans InfluxDB.

---

## 📸 Screenshots

Ajoutez ici :

* Simulation Wokwi
* Dashboard Grafana
* Interface Prometheus
* Architecture du projet

---

## 👨‍💻 Author

**AGBENONZAN Kossivi Jacques Junior**
**KONE Kpantieri**
**HORO Désiré**

Licence 3 Informatique
UFR Mathématiques et Informatique
Université Félix Houphouët-Boigny

---

## 📜 License

Projet académique réalisé dans le cadre du module **Création de Services Réseaux et IoT**.
