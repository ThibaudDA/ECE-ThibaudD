// ==== Bibliothèques ====
#include <WiFi.h>
#include <Wire.h>
#include <math.h>

// ==== Réseau Wi-Fi ====
const char* ssid = "///////";
const char* password = "///////";
WiFiServer server(80);

// ==== Broches moteurs ====
#define IN1_G 17
#define IN2_G 16
#define IN1_D 18
#define IN2_D 19
#define EN_G 4
#define EN_D 23

// ==== Broches encodeurs ====
#define ENC_G_CH_A 32
#define ENC_G_CH_B 33
#define ENC_D_CH_A 27
#define ENC_D_CH_B 14

#define LED_WIFI 26  // GPIO relié à la LED
#define LED_WIFI2 25  // GPIO pour la LED à éteindre après connexion

// ==== Configuration I2C ====
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const uint8_t LIS3MDL_ADDRESS = 0x1E;

// ==== Valeurs de calibration ====
bool boussole = false;
bool boussoleDone = false;
const float hardIronX = 365.50;
const float hardIronY = -3174.00;
const float softIronScale = 1.03;
const float targetAngle = 0.0;    // Nord comme cible
const float tolerance = 15.0;     // Tolérance de ±15°
const int maxSpeed = 80;         // Vitesse max PWM (0-255)
const int minSpeed = 50;          // Vitesse min quand proche de la cible
const float slowDownAngle = 45.0; // Angle où on réduit la vitesse

// ==== Constantes ====
const float TICK_TO_CM = (28.274 / 4000.0) * 4.0;
const int VITESSE_BASE = 100;
const int FREIN_PWM = 70;
const int FREIN_DUREE = 50;

// ==== Variables ====
volatile long tickG = 0;
volatile long tickD = 0;
bool mouvementTermine = true;
float distanceCible = 0;
int sensRotation = 0;

// ==== Structure PID ====
struct PID {
  float Kp, Ki, Kd;
  float sommeErreur;
  float erreurPrecedente;

  PID(float p, float i, float d) : Kp(p), Ki(i), Kd(d), sommeErreur(0), erreurPrecedente(0) {}

  float calculer(float erreur) {
    sommeErreur += erreur;
    float deltaErreur = erreur - erreurPrecedente;
    erreurPrecedente = erreur;
    return Kp * erreur + Ki * sommeErreur + Kd * deltaErreur;
  }
};
PID pidTrajectoire(1.5, 0.0, 0.8);
PID pidRotation(0.8, 0.0, 1.2);

// ==== Interruptions ====
void IRAM_ATTR handleEncoderG() { tickG++; }
void IRAM_ATTR handleEncoderD() { tickD++; }

// ==== Fonctions moteurs ====
void setMoteurs(int pwmG, int pwmD) {
  digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
  digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
  analogWrite(EN_D, constrain(pwmD, 0, 255));
  analogWrite(EN_G, constrain(pwmG, 0, 255));
}
void freinageActif() {
  digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
  digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
  analogWrite(EN_D, FREIN_PWM);
  analogWrite(EN_G, FREIN_PWM);
  delay(FREIN_DUREE);
}
void freinageActifav() {
  digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
  digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
  analogWrite(EN_D, FREIN_PWM);
  analogWrite(EN_G, FREIN_PWM);
  delay(FREIN_DUREE-10);
}
void stop_moteurs() {
  digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, LOW);
  digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, LOW);
  analogWrite(EN_D, 0); analogWrite(EN_G, 0);
}
void resetTicks() {
  tickG = 0;
  tickD = 0;
}
void avancer(int distance_cm) {
  resetTicks();
  distanceCible = distance_cm;
  mouvementTermine = false;
  while (!mouvementTermine) {
    float moyenneDist = ((float)tickG + (float)tickD) / 2.0 * TICK_TO_CM;
    // moyenneDist = moyenne des ticks × cm/tick = distance parcourue en cm
    float erreurTicks = (float)tickG - (float)tickD;
    float correction = pidTrajectoire.calculer(erreurTicks);
    int pwmG = VITESSE_BASE - correction;
    int pwmD = VITESSE_BASE + correction;
    setMoteurs(pwmG, pwmD);
    if (moyenneDist >= distanceCible) {
      freinageActif();
      stop_moteurs();
      mouvementTermine = true;
    }
  }
}
void avancer_mm(int distance_mm) {
  resetTicks();
  distanceCible = distance_mm / 10.0; // on convertit en cm
  mouvementTermine = false;
  while (!mouvementTermine) {
    float moyenneDist = ((float)tickG + (float)tickD) / 2.0 * TICK_TO_CM;
    float erreurTicks = (float)tickG - (float)tickD;
    float correction = pidTrajectoire.calculer(erreurTicks);
    int pwmG = VITESSE_BASE - correction;
    int pwmD = VITESSE_BASE + correction;
    setMoteurs(pwmG, pwmD);
    if (moyenneDist >= distanceCible) {
      freinageActif();
      stop_moteurs();
      mouvementTermine = true;
    }
  }
}
void rotationPrecis(float angleDegre, bool gauche) {
  resetTicks();
  float angleRad = angleDegre * PI / 180.0;
  float distanceRoue = (angleRad * 9.0) / 2.0; // empattement = 9 cm
  long ticksRotationCible = distanceRoue / TICK_TO_CM;

  mouvementTermine = false;
  while (!mouvementTermine) {
    long erreur = tickD + tickG; // même sens pour les deux roues
    float correction = pidRotation.calculer(erreur);
    int pwm = constrain(VITESSE_BASE - correction, 80, 120);

    // Rotation sur place : une roue avance, l'autre recule
    if (gauche) {
      digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);  // droite avance
      digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);  // gauche recule
    } else {
      digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);  // droite recule
      digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);  // gauche avance
    }
    analogWrite(EN_D, pwm+20);
    analogWrite(EN_G, pwm+20);

    long moyenneTicks = (abs(tickG) + abs(tickD)) / 2;
    if (moyenneTicks >= ticksRotationCible) {
      freinageActif();
      stop_moteurs();
      mouvementTermine = true;
    }
  }
}
void tourner() {
  const int V1 = 35;   // très lent
  const int V2 = 75;   // moyen
  const int V3 = 90;   // rapide
  const int V4 = 105;  // très rapide
  const int Vbis = 65;

  if (sensRotation == 2) {  // DROITE
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH);  digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V3); analogWrite(EN_D, V1);
    delay(500);
    
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH);  digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V2); analogWrite(EN_D, V1);
    delay(500);
    
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    analogWrite(EN_G, Vbis); analogWrite(EN_D, V1+10);
    delay(500);
    
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    analogWrite(EN_G, V3); analogWrite(EN_D, V2);
    delay(1000);

    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    analogWrite(EN_G, V3); analogWrite(EN_D, V2+5);
    delay(700);
  }

  else if (sensRotation == 1) {  // GAUCHE
    digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V4); analogWrite(EN_D, V4);
    delay(150);

    digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, 0); analogWrite(EN_D, V3);
    delay(250);

    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V1); analogWrite(EN_D, V3);
    delay(125);

    avancer_mm(2);

    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V1); analogWrite(EN_D, V3);
    delay(125);

    avancer_mm(2);

    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V1); analogWrite(EN_D, V3);
    delay(125);

    avancer_mm(3);

    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    analogWrite(EN_G, V1+10); analogWrite(EN_D, V3);
    delay(125);
  }

  freinageActif();
  stop_moteurs();
  sensRotation = 0;
}


void escalier() {
  rotationPrecis(5, false);
  rotationPrecis(10, true);
  rotationPrecis(5, false);
  avancer(20);
  sensRotation = 1;
  tourner(); // gauche
  sensRotation = 2;
  tourner(); // droite
  rotationPrecis(5, false);
  rotationPrecis(10, true);
  rotationPrecis(5, false);
}



void rotationSurPlace(int degres, bool sensHoraire) {
  const float EMPATTEMENT = 9.5; // Distance entre les roues en cm
  const float DISTANCE_PAR_DEGRE = (PI * EMPATTEMENT) / 360.0; // Distance en cm par degré de rotation
  
  float distanceTotale = DISTANCE_PAR_DEGRE * degres;
  long ticksCible = distanceTotale / TICK_TO_CM;
  
  resetTicks();
  mouvementTermine = false;
  
  // Vitesse identique pour les deux roues
  const int vitesse = 100; // PWM entre 0-255
  
  while(!mouvementTermine) {
    if(sensHoraire) {
      // Rotation horaire: gauche avance, droite recule
      digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
      digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
    } else {
      // Rotation anti-horaire: droite avance, gauche recule
      digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
      digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    }
    
    analogWrite(EN_G, vitesse);
    analogWrite(EN_D, vitesse);
    
    // On vérifie la moyenne des ticks absolus
    long moyenneTicks = (abs(tickG) + abs(tickD)) / 2;
    if(moyenneTicks >= ticksCible) {
      //freinageActif();
      stop_moteurs();
      mouvementTermine = true;
      
    }
  }
}


void tracerCercleRayonParam(float rayonFeutre, bool sensHoraire) {
    const float EMPATTEMENT = 8.0;           // cm entre roues
    const float DIST_CENTRE_FEUTRE = 13.0;   // cm du centre roues au stylo
    const float TICK_TO_CM = (PI * 9.5) / 4000.0; // roue 9,5 cm → ≈0.00746 cm/tick
    const int PWM_EXT = 100;                 // Vitesse extérieure (ajustable)

    // Calcul du rayon pour le centre du robot pour obtenir le cercle voulu au feutre
    float rayonCentre = rayonFeutre - DIST_CENTRE_FEUTRE;

if (rayonFeutre < 13) {
  
 // PREMIERE PARTIE : Côté gauche (recule vers la gauche)
  digitalWrite(IN1_G, LOW); 
  digitalWrite(IN2_G, HIGH);
  digitalWrite(IN1_D, LOW); 
  digitalWrite(IN2_D, HIGH);
  
  // Progression plus fluide des vitesses
  analogWrite(EN_G, 120); analogWrite(EN_D, 40); delay(40);
  analogWrite(EN_G, 115); analogWrite(EN_D, 45); delay(40);
  analogWrite(EN_G, 110); analogWrite(EN_D, 50); delay(40);
  analogWrite(EN_G, 105); analogWrite(EN_D, 55); delay(40);
  analogWrite(EN_G, 100); analogWrite(EN_D, 60); delay(40);
  analogWrite(EN_G, 95);  analogWrite(EN_D, 65); delay(40);
  analogWrite(EN_G, 90);  analogWrite(EN_D, 70); delay(40);
  analogWrite(EN_G, 85);  analogWrite(EN_D, 75); delay(40);
  analogWrite(EN_G, 80);  analogWrite(EN_D, 80); delay(40);
  analogWrite(EN_G, 75);  analogWrite(EN_D, 85); delay(40);
  analogWrite(EN_G, 70);  analogWrite(EN_D, 90); delay(40);
  analogWrite(EN_G, 65);  analogWrite(EN_D, 95); delay(40);
  analogWrite(EN_G, 60);  analogWrite(EN_D, 100); delay(40);
  analogWrite(EN_G, 55);  analogWrite(EN_D, 105); delay(40);
  analogWrite(EN_G, 50);  analogWrite(EN_D, 110); delay(40);
  analogWrite(EN_G, 45);  analogWrite(EN_D, 115); delay(40);

   stop_moteurs();
  
  // DEUXIEME PARTIE : Côté droit (avance vers la droite)
  digitalWrite(IN1_G, HIGH); 
  digitalWrite(IN2_G, LOW);
  digitalWrite(IN1_D, HIGH); 
  digitalWrite(IN2_D, LOW);
  
  // Continuation fluide
  analogWrite(EN_D, 40);  analogWrite(EN_G, 120); delay(40);
  analogWrite(EN_D, 45);  analogWrite(EN_G, 115); delay(40);
  analogWrite(EN_D, 50);  analogWrite(EN_G, 110); delay(40);
  analogWrite(EN_D, 55);  analogWrite(EN_G, 105); delay(40);
  analogWrite(EN_D, 60);  analogWrite(EN_G, 100); delay(40);
  analogWrite(EN_D, 65);  analogWrite(EN_G, 95); delay(40);
  analogWrite(EN_D, 70);  analogWrite(EN_G, 90); delay(40);
  analogWrite(EN_D, 75);  analogWrite(EN_G, 85); delay(40);
  analogWrite(EN_D, 80);  analogWrite(EN_G, 80); delay(40);
  analogWrite(EN_D, 85);  analogWrite(EN_G, 75); delay(40);
  analogWrite(EN_D, 90);  analogWrite(EN_G, 70); delay(40);
  analogWrite(EN_D, 95);  analogWrite(EN_G, 65); delay(40);
  analogWrite(EN_D, 100); analogWrite(EN_G, 60); delay(40);
  analogWrite(EN_D, 105); analogWrite(EN_G, 55); delay(40);
  analogWrite(EN_D, 110); analogWrite(EN_G, 50); delay(40);
  analogWrite(EN_D, 115); analogWrite(EN_G, 45); delay(40);
 
  // Arrêt final
  stop_moteurs();
  //freinageActif();


  
}



    else if (rayonFeutre == 13) {
        // Rayon trop petit ou nul, rotation sur place
        rotationSurPlace(360, sensHoraire);
        stop_moteurs();
        return;
    }


    
else {
    float perimetreCentre = 2.0 * PI * rayonCentre;
    long ticksCible = perimetreCentre / TICK_TO_CM;

    // Calcul des rayons pour vitesses roues
    float rayonInt = rayonCentre - EMPATTEMENT / 2.0;
    float rayonExt = rayonCentre + EMPATTEMENT / 2.0;
    float rapport = rayonInt / rayonExt;
    int PWM_INT = PWM_EXT * rapport;

    resetTicks();
    mouvementTermine = false;

    Serial.print("Rayon stylo : "); Serial.println(rayonFeutre);
    Serial.print("Rayon centre robot : "); Serial.println(rayonCentre);
    Serial.print("Ticks cible : "); Serial.println(ticksCible);
    Serial.print("PWM intérieur : "); Serial.println(PWM_INT);
    Serial.print("PWM extérieur : "); Serial.println(PWM_EXT);

    while (!mouvementTermine) {
        digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
        digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);

        if (sensHoraire) {
            analogWrite(EN_D, PWM_EXT); // Droite = extérieur
            analogWrite(EN_G, PWM_INT); // Gauche = intérieur
        } else {
            analogWrite(EN_D, PWM_INT); // Droite = intérieur
            analogWrite(EN_G, PWM_EXT); // Gauche = extérieur
        }

        long moyenneTicks = (abs(tickG) + abs(tickD)) / 2;
        if (moyenneTicks >= ticksCible) {
            freinageActif();
            stop_moteurs();
            mouvementTermine = true;
        }
    }
    }
}






// ==== Setup ====
void setup() {
  Serial.begin(115200);

pinMode(LED_WIFI2, OUTPUT);
digitalWrite(LED_WIFI2, HIGH);  // Allumée au début
pinMode(LED_WIFI, OUTPUT);
digitalWrite(LED_WIFI, LOW); // LED éteinte au départ
  
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(IN1_D, OUTPUT); pinMode(IN2_D, OUTPUT);
  pinMode(IN1_G, OUTPUT); pinMode(IN2_G, OUTPUT);
  pinMode(EN_D, OUTPUT); pinMode(EN_G, OUTPUT);
  pinMode(ENC_G_CH_A, INPUT); pinMode(ENC_G_CH_B, INPUT);
  pinMode(ENC_D_CH_A, INPUT); pinMode(ENC_D_CH_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_G_CH_A), handleEncoderG, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_D_CH_A), handleEncoderD, RISING);
  // Configuration magnétomètre
  writeRegister(0x20, 0xFC);
  writeRegister(0x21, 0x00);
  writeRegister(0x22, 0x00);
  writeRegister(0x23, 0x0C);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
digitalWrite(LED_WIFI2, LOW);  // Éteindre la LED
digitalWrite(LED_WIFI, HIGH); // LED allumée après connexion

  server.begin();
  Serial.println(WiFi.localIP());
}

// ==== Loop ====
void loop() {
  while(boussole){
    float angle = getFilteredAngle();
    if(!isInTargetZone(angle)) {
      // Calcul de la vitesse adaptative
      float error = calculateError(angle);
      int currentSpeed = calculateAdaptiveSpeed(error);
      
      if(error > 0) {
        turnRight(currentSpeed);
      } else {
        turnLeft(currentSpeed);
      }
    } else {
      stop_moteurs();
      boussole = false;
      boussoleDone = true;
    }
  }
  if(boussoleDone){
    delay(300);
    rotationPrecis(10, false);
    stop_moteurs();
    delay(100);
    // avancer diag gauche
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    analogWrite(EN_G, 60); analogWrite(EN_D, 100);
    delay(300);
    stop_moteurs();
    delay(100);
    // reculer diag gauche
    digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
    digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
    analogWrite(EN_G, 100); analogWrite(EN_D, 60);
    delay(350);
    freinageActifav();
    stop_moteurs();
    delay(200);
    // tourner droite
    /*rotationPrecis(5, false);
    stop_moteurs();
    delay(100);
    // avancer 4cm
    avancer(4);
    // oscillation en reculant
    digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
    digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
    analogWrite(EN_G, 80); analogWrite(EN_D, 80);
    delay(50);*/
    /*for(int i = 0; i <= 40; i++){
      // Reculer
      digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
      digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
      analogWrite(EN_G, 60); analogWrite(EN_D, 60);
      delay(10);
      stop_moteurs();
      delay(400);
    
      float angle = 1 + i * 1.001;  // angle croissant de rotation
      rotationPrecis(angle, true);   // gauche
      freinageActifav();
      stop_moteurs();
      delay(300);
      rotationPrecis(angle * 2, false);  // droite plus fort
      freinageActifav();
      stop_moteurs();
      delay(300);
      rotationPrecis(angle * 1.1, true);   // gauche pour revenir
      freinageActifav();
      stop_moteurs();
      delay(300);
    }*/
    rotationPrecis(5, false);
    freinageActifav();
    digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
    digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
    analogWrite(EN_G, 70); analogWrite(EN_D, 70);
    delay(80);
    stop_moteurs();
    delay(200);
    for(int i = 0; i <= 7; i++){
      // tourner 45° droite
      rotationPrecis(43, false);
      freinageActifav();
      stop_moteurs();
      delay(200);
      // avancer reculer MEME DISTANCE
      digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
      digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW);
      analogWrite(EN_G, 80); analogWrite(EN_D, 80);
      delay(80);
      digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
      digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
      analogWrite(EN_G, 70); analogWrite(EN_D, 70);
      delay(80);
      freinageActifav();
      stop_moteurs();
      delay(200);
    }
    boussoleDone = false;
    stop_moteurs();
  }
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/AVANCER") != -1) {
      int index = request.indexOf("distance=");
      if (index != -1) {
        String param = request.substring(index + 9);
        int cm = param.toInt();
        if (cm > 0 && cm < 1000) {
          avancer(cm);
        }
      }
    }
    if (request.indexOf("/ESCALIER") != -1) {
      escalier();
    }
    if (request.indexOf("/BOUSSOLE") != -1) {
      boussole = true;
    }


/*
 if (request.indexOf("/ROTATION") != -1) {
      rotationSurPlace(360, false); // 360° anti-horaire
    }
*/


if (request.indexOf("/CERCLE") != -1) {
    int index = request.indexOf("rayon=");
    if (index != -1) {
        String param = request.substring(index + 6);
        float rayon = param.toFloat();
        tracerCercleRayonParam(rayon, true); // Appel systématique, gestion interne
    }
}

    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html\n");
    client.println();
    client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Robot PID</title>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; background-color: #f4f4f4; padding: 20px; text-align: center; }");
    client.println("h1 { color: linear-gradient(0.25turn, #007BFF, #000000, #FF0000); }");
    client.println("form { background: white; padding: 15px; margin: 10px auto; border-radius: 10px; box-shadow: 0 2px 6px rgba(0,0,0,0.1); width: 300px; }");
    client.println("label { display: block; margin-bottom: 8px; font-weight: bold; }");
    client.println("input[type='number'] { width: 80%; padding: 8px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 5px; }");
    client.println("input[type='submit'] { background-color: #007BFF; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; transition: 0.2s; }");
    client.println("input[type='submit']:hover { background-color: #888888; transform: scale(0.9); transition: 0.2s; }");
    client.println("</style>");
    client.println("</head><body>");
    client.println("<h1>Robot PID Wi-Fi</h1>");
    client.println("<form action='/AVANCER' method='GET'>");
    client.println("<label>Distance (cm): </label><input type='number' name='distance' min='1' max='1000'>");
    client.println("<input type='submit' value='Avancer'>");
    client.println("</form>");
    client.println("<form action='/ESCALIER' method='GET'>");
    client.println("<input type='submit' value='Parcours Escalier'>");
    client.println("</form>");
client.println("<form action='/CERCLE' method='GET'>");
client.println("<label>Rayon du cercle (2-20 cm): </label>");
client.println("<input type='number' name='rayon' min='2' max='20' step='0.5' >");
client.println("<input type='submit' value='Tracer Cercle'>");
client.println("</form>");
    client.println("<form action='/BOUSSOLE' method='GET'>");
    client.println("<input type='submit' value='Rose des vents'>");
    client.println("</form>");


    
    client.println("</body></html>");
    client.stop();
  }
}



float getFilteredAngle() {
  int16_t x, y, z;
  readRawValues(&x, &y, &z);
  float x_cal = -(x - hardIronX) * softIronScale;
  float y_cal = (y - hardIronY);
  float angle = atan2(x_cal, -y_cal) * 180.0 / M_PI;
  return angle < 0 ? angle + 360 : angle;
}

bool isInTargetZone(float angle) {
  return (angle >= 335.0 || angle <= 15.0); // Zone élargie pour stabilité
}

float calculateError(float angle) {
  // Calcul l'erreur angulaire la plus courte
  float error = 0;
  if(angle > 180) {
    error = 360 - angle; // Tourne à droite
  } else {
    error = -angle;      // Tourne à gauche
  }
  return error;
}

int calculateAdaptiveSpeed(float error) {
  // Réduction progressive de la vitesse
  float absError = fabs(error);
  if(absError < slowDownAngle) {
    return map(absError, 0, slowDownAngle, minSpeed, maxSpeed);
  }
  return maxSpeed;
}

void turnRight(int speed) {
  // roue gauche avance, roue droite recule
  digitalWrite(IN1_G, HIGH); digitalWrite(IN2_G, LOW); 
  digitalWrite(IN1_D, LOW); digitalWrite(IN2_D, HIGH);
  analogWrite(EN_G, speed);
  analogWrite(EN_D, speed);
}

void turnLeft(int speed) {
  // roue droite avance, roue gauche recule
  digitalWrite(IN1_G, LOW); digitalWrite(IN2_G, HIGH);
  digitalWrite(IN1_D, HIGH); digitalWrite(IN2_D, LOW);
  analogWrite(EN_G, speed);
  analogWrite(EN_D, speed);
}



void readRawValues(int16_t* x, int16_t* y, int16_t* z) {
  Wire.beginTransmission(LIS3MDL_ADDRESS);
  Wire.write(0x28);
  Wire.endTransmission(false);
  Wire.requestFrom(LIS3MDL_ADDRESS, 6);
  
  *x = (int16_t)(Wire.read() | Wire.read() << 8);
  *y = (int16_t)(Wire.read() | Wire.read() << 8);
  *z = (int16_t)(Wire.read() | Wire.read() << 8);
}

void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(LIS3MDL_ADDRESS);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

