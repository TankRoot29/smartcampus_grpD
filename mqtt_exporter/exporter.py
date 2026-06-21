import os
import json
import time
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

# Récupération des variables d'environnement définies dans Docker Compose
INFLUX_URL = os.getenv("INFLUXDB_URL", "http://localhost:8086")
INFLUX_TOKEN = os.getenv("INFLUXDB_TOKEN", "MonSuperTokenSecretEtTresLongPourLAPI123456789")
INFLUX_ORG = os.getenv("INFLUXDB_ORG", "ufhb")
INFLUX_BUCKET = os.getenv("INFLUXDB_BUCKET", "parking_metrics")

MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC = "campus/parking/kossivi/places"

# Initialisation de la connexion à la base de données InfluxDB
print(" Connexion à la base InfluxDB...")
influx_client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)

def on_connect(client, userdata, flags, rc):
    print(f" Connecté au broker HiveMQ avec succès. Écoute du topic : {MQTT_TOPIC}")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        # Décodage de la charge utile JSON reçue de l'ESP32
        payload = msg.payload.decode('utf-8')
        data = json.loads(payload)
        
        # Extraction des différentes métriques
        places = int(data.get("places_libres", 10))
        evenement = int(data.get("evenement", 0))
        dist_e = float(data.get("dist_entree", 400.0))
        dist_s = float(data.get("dist_sortie", 400.0))

        # Structuration de la donnée en format InfluxDB (Line Protocol)
        point = Point("parking_status") \
            .tag("device", "esp32_campus") \
            .field("places_libres", places) \
            .field("evenement", evenement) \
            .field("distance_entree", dist_e) \
            .field("distance_sortie", dist_s)

        # Écriture immédiate dans le bucket InfluxDB
        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print(f" [INFLUXDB SAVE] -> Places: {places} | Evt: {evenement} | Dist_E: {dist_e}cm | Dist_S: {dist_s}cm")

    except json.JSONDecodeError:
        print(f" Erreur : Message reçu non conforme au format JSON : {msg.payload}")
    except Exception as e:
        print(f" Erreur lors de l'insertion InfluxDB : {e}")

def main():
    client = mqtt.Client("Python_Influx_Gateway")
    client.on_connect = on_connect
    client.on_message = on_message

    # Boucle de reconnexion robuste au broker MQTT
    while True:
        try:
            print(f" Connexion au broker MQTT public : {MQTT_BROKER}...")
            client.connect(MQTT_BROKER, 1883, 60)
            break
        except Exception as e:
            print(f" Échec de connexion au broker ({e}). Nouvel essai dans 5s...")
            time.sleep(5)

    client.loop_forever()

if __name__ == '__main__':
    main()