#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Configuration du relais
#define RELAY_PIN 23

// Configuration NTP (fuseau horaire et serveur)
// Décalage en secondes (ex: GMT+1 = 3600, ajustez selon votre région)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// URL de l'API Flask (adaptez avec l'IP de votre PC)
const char* serverUrl = "http://10.26.9.19:5000/api/schedules/1";

unsigned long lastTimeCheck = 0;
const long interval = 10000; // Vérification toutes les 10 secondes

void setup() {
  Serial.begin(115200);

  // Configuration de la broche du relais
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Initialisation de WiFiManager pour la configuration du réseau
  WiFiManager wifiManager;
  
  // Décommentez la ligne ci-dessous si vous souhaitez réinitialiser les paramètres Wi-Fi enregistrés à chaque démarrage pour tester
  // wifiManager.resetSettings();

  // Si la connexion échoue, un point d'accès "P-RingMaster_Config" est créé
  if (!wifiManager.autoConnect("P-RingMaster_Config")) {
    Serial.println("Échec de la connexion et timeout atteints.");
    ESP.restart();
  }

  Serial.println("Connecté au Wi-Fi avec succès !");

  // Démarrage du client NTP
  timeClient.begin();
}

void loop() {
  // Mise à jour de l'heure NTP
  timeClient.update();
  
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Heure actuelle : ");
  Serial.println(formattedTime);

  unsigned long currentMillis = millis();
  
  // Interroger le serveur Flask à intervalle régulier
  if (currentMillis - lastTimeCheck >= interval) {
    lastTimeCheck = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        String payload = http.getString();
        Serial.println("Réponse du serveur reçue :");
        Serial.println(payload);

        // Analyse du JSON reçu
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          bool triggerRelay = doc["trigger"] | false;
          if (triggerRelay) {
            Serial.println("Activation du relais (Sonnerie) !");
            digitalWrite(RELAY_PIN, HIGH);
            delay(3000); // Durée de la sonnerie : 3 secondes
            digitalWrite(RELAY_PIN, LOW);
          }
        } else {
          Serial.println("Erreur lors de l'analyse du JSON");
        }
      } else {
        Serial.print("Erreur de connexion HTTP, code : ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("WiFi déconnecté");
    }
  }

  delay(1000);
}