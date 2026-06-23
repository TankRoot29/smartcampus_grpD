#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
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

// ===== TOPICS MQTT (Convention SmartCampus CI) =====
#define TOPIC_CAPTEURS "ufhb/grp_D/capteurs/parking"
#define TOPIC_ALERTES  "ufhb/grp_D/alertes"
#define TOPIC_STATUT   "ufhb/grp_D/statut"
#define TOPIC_EVENEMENTS "ufhb/grp_D/evenements"
#define TOPIC_IDS      "ufhb/grp_D/ids/alertes" // 🛡️ Nouveau topic pour l'IDS physique

// ============================================================
// ===== CONSTANTES ===========================================
// ============================================================
const int PLACES_MAX = 10;
const int DISTANCE_SEUIL = 150;
const unsigned long INTERVALLE_ENVOI = 5000;  // 5 secondes
const unsigned long TEMPS_OUVERTURE = 3000;    // 3 secondes

// ============================================================
// ===== OBJETS ===============================================
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo barriereEntree;
Servo barriereSortie;
WiFiClient espClient;
PubSubClient client(espClient);
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

  char buffer[256];
  serializeJson(doc, buffer);
  
  Serial.print("🚨 ALERTE SYSTEME: ");
  Serial.println(buffer);
  client.publish(TOPIC_ALERTES, buffer);
}

// 🛡️ FONCTION IDS : Alerte d'intrusion physique
void envoyerAlerteIDS(const char* anomalie, float distanceInterception) {
  StaticJsonDocument<256> doc;
  doc["alerte"] = "INTRUSION_DETECTEE";
  doc["anomalie"] = anomalie;
  doc["distance_cm"] = round(distanceInterception * 10) / 10.0;
  doc["places_libres"] = placesLibres;
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  
  Serial.print("🛡️ [IDS ALERT]: ");
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
  while (!client.connected()) {
    Serial.print("🔄 MQTT...");
    String clientId = "ESP32_SmartCampus_GrpD_" + String(random(0, 10000));
    
    if (client.connect(clientId.c_str(), TOPIC_STATUT, 1, true, "Hors-ligne")) {
      Serial.println(" ✅ connecté !");
      client.publish(TOPIC_STATUT, "En-ligne", true);
    } else {
      Serial.print(" ❌ échec, rc=");
      Serial.print(client.state());
      Serial.println(" - nouvelle tentative dans 5s");
      delay(5000);
    }
  }
}

// ============================================================
// ===== SETUP ================================================
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n========================================");
  Serial.println("🚗 SMARTCAMPUS CI - PARKING (+ IDS PROTECTION)");
  Serial.println("========================================");
  
  // Initialisation des broches
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
  
  // DHT22
  dht.begin();
  
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" SMARTCAMPUS CI ");
  lcd.setCursor(0, 1);
  lcd.print("  PARKING IOT   ");

  // WiFi et MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  reconnecterMQTT();
  
  // Affichage final
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Places: 10");
  lcd.setCursor(0, 1);
  lcd.print("Disponible");
  
  Serial.println("✅ Système Edge prêt !");
  Serial.println("========================================\n");
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
  
  // Lecture des distances aux barrières
  float distanceEntree = lireDistanceCM(PIN_TRIG_ENTREE, PIN_ECHO_ENTREE);
  float distanceSortie = lireDistanceCM(PIN_TRIG_SORTIE, PIN_ECHO_SORTIE);

  // === GESTION DE LA BARRIÈRE D'ENTRÉE ===
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
      
      Serial.print("✅ Places restantes: ");
      Serial.println(placesLibres);
      
    } else {
      // 🛡️ CRITIQUE IDS : Tentative d'intrusion (Parking Plein + Forçage détecté au capteur)
      Serial.println("\n⚠️ [IDS CRITIQUE] : Tentative de forçage du portail d'entrée !");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCES REFUSE !");
      lcd.setCursor(0, 1);
      lcd.print("INTRUSION PORTAIL");
      
      // Envoi de l'alerte IDS structurée
      envoyerAlerteIDS("Forcage de barriere d'entree (Parking Satire)", distanceEntree);
      envoyerAlerteImmediate("FRAUDE_ENTREE", "Tentative d'entree sur parking plein");
      
      // Déclenchement de l'alarme matérielle locale
      for(int i = 0; i < 4; i++) {
        digitalWrite(PIN_BUZZER, HIGH);
        digitalWrite(PIN_LED_ROUGE, HIGH);
        delay(100);
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_LED_ROUGE, LOW);
        delay(100);
      }
    }
  }
  
  if (distanceEntree >= DISTANCE_SEUIL) {
    voitureDevantEntree = false;
  }

  // === GESTION DE LA BARRIÈRE DE SORTIE ===
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
      
      Serial.print("✅ Places restantes: ");
      Serial.println(placesLibres);
    } else {
      // 🛡️ SCÉNARIO IDS SUPPLÉMENTAIRE : Anomalie à la sortie (Une voiture passe alors que le parking est déjà vide)
      envoyerAlerteIDS("Forcage de barriere de sortie / Detection suspecte", distanceSortie);
    }
  }
  
  if (distanceSortie >= DISTANCE_SEUIL) {
    voitureDevantSortie = false;
  }

  // === FERMETURE AUTOMATIQUE DES BARRIÈRES ===
  if (barriereEntreeOuverte && maintenant > tempsFermetureEntree) {
    barriereEntree.write(0);
    barriereEntreeOuverte = false;
    Serial.println("🔒 Barrière entrée fermée");
    lcd.clear();
  }
  
  if (barriereSortieOuverte && maintenant > tempsFermetureSortie) {
    barriereSortie.write(0);
    barriereSortieOuverte = false;
    Serial.println("🔒 Barrière sortie fermée");
    lcd.clear();
  }

  // === ENVOI PÉRIODIQUE ===
  if (maintenant - dernierEnvoiPeriodique >= INTERVALLE_ENVOI) {
    dernierEnvoiPeriodique = maintenant;
    
    float temperature = dht.readTemperature();
    float humidite = dht.readHumidity();
    
    envoyerDonneesPeriodiques(temperature, humidite);
    
    if (!isnan(temperature) && temperature > 35.0) {
      envoyerAlerteImmediate("TEMP_ELEVEE", "Temperature > 35°C");
    }
    if (!isnan(humidite) && humidite > 80.0) {
      envoyerAlerteImmediate("HUMIDITE_ELEVEE", "Humidite > 80%");
    }
  }

  // === AFFICHAGE LCD STANDARD ===
  if (!barriereEntreeOuverte && !barriereSortieOuverte) {
    if (maintenant - dernierAffichageLCD >= 500) {
      dernierAffichageLCD = maintenant;
      
      lcd.setCursor(0, 0);
      lcd.print("Places: ");
      lcd.print(placesLibres);
      lcd.print("  ");
      
      lcd.setCursor(0, 1);
      if (placesLibres == 0) {
        lcd.print("PARKING PLEIN");
        digitalWrite(PIN_LED_ROUGE, HIGH);
        digitalWrite(PIN_LED_VERTE, LOW);
      } else {
        lcd.print("Disponible    ");
        digitalWrite(PIN_LED_VERTE, HIGH);
        digitalWrite(PIN_LED_ROUGE, LOW);
      }
    }
  }

  delay(50);
}