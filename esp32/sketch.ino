#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ============================================================
// ===== CONFIGURATION DES PINS ===============================
// ============================================================
#define PIN_TRIG_ENTREE 5
#define PIN_ECHO_ENTREE 18
#define PIN_TRIG_SORTIE 19
#define PIN_ECHO_SORTIE 23
#define PIN_LED_VERTE   2
#define PIN_LED_ROUGE   4
#define PIN_BUZZER      12
#define PIN_SERVO_E     13
#define PIN_SERVO_S     14
#define PIN_DHT         15

#define DHTTYPE DHT22

// ============================================================
// ===== CONFIGURATION RÉSEAU =================================
// ============================================================
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 8883;

// Topics MQTT
#define TOPIC_CAPTEURS   "ufhb/grp_D/capteurs/parking"
#define TOPIC_ALERTES    "ufhb/grp_D/alertes"
#define TOPIC_STATUT     "ufhb/grp_D/statut"
#define TOPIC_EVENEMENTS "ufhb/grp_D/evenements"
#define TOPIC_IDS        "ufhb/grp_D/ids/alertes"

// ============================================================
// ===== CONSTANTES ===========================================
// ============================================================
const int PLACES_MAX = 10;
const int DISTANCE_SEUIL = 150;
const unsigned long INTERVALLE_ENVOI = 5000;
const unsigned long TEMPS_OUVERTURE = 3000;

// ============================================================
// ===== OBJETS ===============================================
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo barriereEntree;
Servo barriereSortie;
WiFiClientSecure espClientSec;
PubSubClient client(espClientSec);
DHT dht(PIN_DHT, DHTTYPE);

// ============================================================
// ===== VARIABLES ============================================
// ============================================================
int placesLibres = PLACES_MAX;
bool voitureDevantEntree = false;
bool voitureDevantSortie = false;
bool barriereEntreeOuverte = false;
bool barriereSortieOuverte = false;
unsigned long tempsFermetureEntree = 0;
unsigned long tempsFermetureSortie = 0;
unsigned long dernierEnvoiPeriodique = 0;
unsigned long dernierAffichageLCD = 0;

// ============================================================
// ===== FONCTIONS ============================================
// ============================================================
float lireDistanceCM(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  long duree = pulseIn(pinEcho, HIGH, 30000);
  if (duree == 0) return 400.0;
  return duree * 0.034 / 2;
}

void setup_wifi() {
  delay(10);
  Serial.println("\n📶 Connexion Wi-Fi...");
  WiFi.begin(ssid, password);
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 20) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi connecté !");
    Serial.print("📌 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Échec Wi-Fi !");
  }
}

void envoyerDonneesPeriodiques(float temp, float hum) {
  StaticJsonDocument<256> doc;
  doc["places_libres"] = placesLibres;
  doc["temperature"] = isnan(temp) ? 0.0 : round(temp * 10) / 10.0;
  doc["humidite"] = isnan(hum) ? 0.0 : round(hum * 10) / 10.0;
  doc["statut_parking"] = (placesLibres == 0) ? "PLEIN" : "DISPONIBLE";
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  Serial.print("📤 MQTT Periodique: ");
  Serial.println(buffer);
  client.publish(TOPIC_CAPTEURS, buffer, true);
}

void envoyerAlerteImmediate(const char* type, const char* message) {
  StaticJsonDocument<256> doc;
  doc["type"] = type;
  doc["message"] = message;
  doc["places_libres"] = placesLibres;
  doc["timestamp"] = millis();
  
  // Ajoute les valeurs actuelles
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp)) doc["temperature"] = round(temp * 10) / 10.0;
  if (!isnan(hum)) doc["humidite"] = round(hum * 10) / 10.0;

  char buffer[256];
  serializeJson(doc, buffer);
  
  Serial.print("🚨 ALERTE MÉTÉO: ");
  Serial.println(buffer);
  client.publish(TOPIC_ALERTES, buffer);
}

void envoyerAlerteIDS(const char* anomalie, float distanceInterception) {
  StaticJsonDocument<256> doc;
  doc["alerte"] = "INTRUSION_DETECTEE";
  doc["anomalie"] = anomalie;
  doc["distance_cm"] = round(distanceInterception * 10) / 10.0;
  doc["places_libres"] = placesLibres;
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  Serial.print("🛡️ IDS ALERT: ");
  Serial.println(buffer);
  client.publish(TOPIC_IDS, buffer);
}

void envoyerEvenement(const char* evenement) {
  StaticJsonDocument<200> doc;
  doc["evenement"] = evenement;
  doc["places_libres"] = placesLibres;
  doc["timestamp"] = millis();

  char buffer[200];
  serializeJson(doc, buffer);
  client.publish(TOPIC_EVENEMENTS, buffer);
}

void reconnecterMQTT() {
  if (client.connected()) return;

  static unsigned long dernierEssai = 0;
  if (millis() - dernierEssai < 5000) return;
  dernierEssai = millis();

  Serial.print("🔄 Connexion MQTT...");
  String clientId = "ESP32_SmartCampus_GrpD_" + String(random(0, 10000));
  
  if (client.connect(clientId.c_str(), TOPIC_STATUT, 1, true, "Hors-ligne")) {
    Serial.println(" ✅ connecté !");
    client.publish(TOPIC_STATUT, "En-ligne", true);
  } else {
    Serial.print(" ❌ rc=");
    Serial.println(client.state());
  }
}

// ============================================================
// ===== SETUP ================================================
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);   // Important pour Wokwi

  Serial.println("\n========================================");
  Serial.println("🚗 SMARTCAMPUS CI - PARKING");
  Serial.println("========================================");

  // Broches
  pinMode(PIN_TRIG_ENTREE, OUTPUT);
  pinMode(PIN_ECHO_ENTREE, INPUT);
  pinMode(PIN_TRIG_SORTIE, OUTPUT);
  pinMode(PIN_ECHO_SORTIE, INPUT);
  pinMode(PIN_LED_VERTE, OUTPUT);
  pinMode(PIN_LED_ROUGE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_VERTE, HIGH);
  digitalWrite(PIN_LED_ROUGE, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // Servos
  barriereEntree.attach(PIN_SERVO_E);
  barriereEntree.write(0);
  barriereSortie.attach(PIN_SERVO_S);
  barriereSortie.write(0);

  dht.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" SMARTCAMPUS CI ");
  lcd.setCursor(0, 1);
  lcd.print("  PARKING IOT   ");

  setup_wifi();
  espClientSec.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  reconnecterMQTT();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Places: 10");
  lcd.setCursor(0, 1);
  lcd.print("Disponible");

  Serial.println("✅ Système prêt !");
}

// ============================================================
// ===== LOOP =================================================
// ============================================================
void loop() {
  if (!client.connected()) {
    reconnecterMQTT();
  }
  client.loop();

  unsigned long maintenant = millis();
  
  float distanceEntree = lireDistanceCM(PIN_TRIG_ENTREE, PIN_ECHO_ENTREE);
  float distanceSortie = lireDistanceCM(PIN_TRIG_SORTIE, PIN_ECHO_SORTIE);

  // === GESTION ENTRÉE ===
  if (distanceEntree < DISTANCE_SEUIL && !voitureDevantEntree) {
    voitureDevantEntree = true;
    
    if (placesLibres > 0) {
      Serial.println("\n🚗 VOITURE À L'ENTRÉE !");
      digitalWrite(PIN_BUZZER, HIGH);
      delay(150);
      digitalWrite(PIN_BUZZER, LOW);
      
      placesLibres--;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Entree active");
      lcd.setCursor(0, 1);
      lcd.print("Places: ");
      lcd.print(placesLibres);
      
      barriereEntree.write(90);
      barriereEntreeOuverte = true;
      tempsFermetureEntree = maintenant + TEMPS_OUVERTURE;
      
      envoyerEvenement("entree_vehicule");
    } else {
      Serial.println("\n⚠️ [IDS] Tentative d'intrusion !");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCES REFUSE !");
      lcd.setCursor(0, 1);
      lcd.print("INTRUSION PORTAIL");
      
      envoyerAlerteIDS("Forcage entree (Plein)", distanceEntree);
      envoyerAlerteImmediate("FRAUDE_ENTREE", "Parking plein");
    }
  }
  if (distanceEntree >= DISTANCE_SEUIL) voitureDevantEntree = false;

  // === GESTION SORTIE ===
  if (distanceSortie < DISTANCE_SEUIL && !voitureDevantSortie) {
    voitureDevantSortie = true;
    
    if (placesLibres < PLACES_MAX) {
      Serial.println("\n🚗 VOITURE À LA SORTIE !");
      digitalWrite(PIN_BUZZER, HIGH);
      delay(150);
      digitalWrite(PIN_BUZZER, LOW);
      
      placesLibres++;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bonne route !");
      lcd.setCursor(0, 1);
      lcd.print("Places: ");
      lcd.print(placesLibres);
      
      barriereSortie.write(90);
      barriereSortieOuverte = true;
      tempsFermetureSortie = maintenant + TEMPS_OUVERTURE;
      
      envoyerEvenement("sortie_vehicule");
    } else {
      envoyerAlerteIDS("Anomalie sortie", distanceSortie);
    }
  }
  if (distanceSortie >= DISTANCE_SEUIL) voitureDevantSortie = false;

  // Fermeture barrières
  if (barriereEntreeOuverte && maintenant > tempsFermetureEntree) {
    barriereEntree.write(0);
    barriereEntreeOuverte = false;
    Serial.println("🔒 Barrière entrée fermée");
  }
  if (barriereSortieOuverte && maintenant > tempsFermetureSortie) {
    barriereSortie.write(0);
    barriereSortieOuverte = false;
    Serial.println("🔒 Barrière sortie fermée");
  }

  // === ENVOI PÉRIODIQUE & DETECTION SEUILS ENVIRONNEMENTAUX ===
  if (maintenant - dernierEnvoiPeriodique >= INTERVALLE_ENVOI) {
    dernierEnvoiPeriodique = maintenant;
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    envoyerDonneesPeriodiques(temp, hum);

    // 💡 Déclenchement des alertes météo en cas de dépassement des seuils
    if (!isnan(temp) && temp > 35.0) {
      envoyerAlerteImmediate("TEMP_ELEVEE", "Temperature critique > 35C");
    }
    if (!isnan(hum) && hum > 80.0) {
      envoyerAlerteImmediate("HUMIDITE_ELEVEE", "Humidite critique > 80%");
    }
  }

  delay(50);
}