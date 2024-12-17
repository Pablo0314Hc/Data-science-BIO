#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED, en píxeles
#define SCREEN_HEIGHT 64 // Altura de la pantalla OLED, en píxeles

// Declaración para una pantalla SSD1306 conectada a I2C (pines SDA, SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 4     // Pin donde está conectado el sensor DHT
#define DHTTYPE DHT11     

DHT dht(DHTPIN, DHTTYPE);

// Pines para los LEDs y dispositivos
const int humledverde = 5;
const int humledrojo = 6;
const int templedverde = 8;
const int templedrojo = 7;
const int luces = 3;
const int humificador = 2;

// Variables para los límites de temperatura y humedad (sin valores iniciales)
float tempLowerLimit;
float tempUpperLimit;
float humLowerLimit;
float humUpperLimit;

// Variable para indicar si se han recibido los límites
boolean limitsReceived = false;

// Variables para la comunicación serial
String datosRecibidos = "";
boolean datosCompletos = false;

void setup() {
  Serial.begin(115200);
  pinMode(templedverde, OUTPUT);
  pinMode(templedrojo, OUTPUT);
  pinMode(humledverde, OUTPUT);
  pinMode(humledrojo, OUTPUT);
  pinMode(luces, OUTPUT);
  pinMode(humificador, OUTPUT);

  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // Si falla la inicialización de la pantalla OLED
    for (;;); // Bucle infinito
  }
  delay(1000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop() {
  // Lectura y procesamiento de datos seriales
  while (Serial.available()) {
    char inChar = (char)Serial.read();

    // Procesar comandos 'A' y 'B'
    if (inChar == 'A') {
      enviarMediciones();
    } else if (inChar == 'B') {
      apagarComponentes();
    } else {
      // Acumular datos para los límites
      datosRecibidos += inChar;
      if (inChar == '\n') {
        datosCompletos = true;
      }
    }
  }

  if (datosCompletos) {
    procesarDatos(datosRecibidos);
    datosRecibidos = "";
    datosCompletos = false;
  }

  // Esperamos 1 segundo entre lecturas
  delay(1000);

  // Leemos la temperatura y humedad del sensor DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Verificamos si la lectura es válida
  if (isnan(t) || isnan(h)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  // Actualizamos la pantalla OLED
  actualizarPantalla(t, h);

  // Solo controlamos los dispositivos si ya hemos recibido los límites
  if (limitsReceived) {
    controlarDispositivos(t, h);
  } else {
    // Apagamos los dispositivos hasta recibir los límites
    apagarComponentes();

    // Mensaje opcional en el monitor serial
    Serial.println("Esperando límites desde LabVIEW...");
  }
}

// Función para actualizar la pantalla OLED
void actualizarPantalla(float t, float h) {
  display.clearDisplay();

  // TEMPERATURA
  display.setTextSize(1.9);
  display.setCursor(0, 0);
  display.print("Temperatura: ");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167); // Carácter para el símbolo de grado
  display.setTextSize(2);
  display.print("C");

  // HUMEDAD
  display.setTextSize(1.9);
  display.setCursor(0, 35);
  display.print("Humedad: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(h);
  display.print(" %");
  display.display();
}

// Función para controlar LEDs y dispositivos
void controlarDispositivos(float t, float h) {
  // Control de LEDs de temperatura
  if (t >= tempLowerLimit && t <= tempUpperLimit) {
    digitalWrite(templedverde, HIGH);  // Encender LED verde
    digitalWrite(templedrojo, LOW);    // Apagar LED rojo
  } else {
    digitalWrite(templedverde, LOW);   // Apagar LED verde
    digitalWrite(templedrojo, HIGH);   // Encender LED rojo
  }

  // Control de LEDs de humedad
  if (h >= humLowerLimit && h <= humUpperLimit) {
    digitalWrite(humledverde, HIGH);   // Encender LED verde
    digitalWrite(humledrojo, LOW);     // Apagar LED rojo
  } else {
    digitalWrite(humledverde, LOW);    // Apagar LED verde
    digitalWrite(humledrojo, HIGH);    // Encender LED rojo
  }

  // Control de luces (FOCOS 127VAC)
  if (t <= tempUpperLimit) {
    digitalWrite(luces, LOW);  // Apagar luces
  } else {
    digitalWrite(luces, HIGH); // Encender luces
  }

  // Control del humificador
  if (h <= humUpperLimit) {
    digitalWrite(humificador, LOW);  // Apagar humificador
  } else {
    digitalWrite(humificador, HIGH); // Encender humificador
  }
}

// Función para procesar los datos recibidos por serial
void procesarDatos(String datos) {
  // Separar los datos usando comas
  int indice1 = datos.indexOf(',');
  int indice2 = datos.indexOf(',', indice1 + 1);
  int indice3 = datos.indexOf(',', indice2 + 1);

  if (indice1 > 0 && indice2 > 0 && indice3 > 0) {
    String tempUpperStr = datos.substring(0, indice1);
    String tempLowerStr = datos.substring(indice1 + 1, indice2);
    String humUpperStr = datos.substring(indice2 + 1, indice3);
    String humLowerStr = datos.substring(indice3 + 1, datos.length() - 1); // Excluimos '\n'

    tempUpperLimit = tempUpperStr.toFloat();
    tempLowerLimit = tempLowerStr.toFloat();
    humUpperLimit = humUpperStr.toFloat();
    humLowerLimit = humLowerStr.toFloat();

    limitsReceived = true; // Indicamos que ya hemos recibido los límites
  }
}

// Función para enviar las mediciones del sensor
void enviarMediciones() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Verificamos si la lectura es válida
  if (isnan(t) || isnan(h)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  // Enviamos las mediciones por el puerto serial
  Serial.print(t);
  Serial.print("/");
  Serial.print(h);
  Serial.println("\t");
}

// Función para apagar todos los componentes
void apagarComponentes() {
  digitalWrite(templedrojo, LOW); 
  digitalWrite(templedverde, LOW);
  digitalWrite(humledrojo, LOW);
  digitalWrite(humledverde, LOW);
  digitalWrite(luces, LOW);
  digitalWrite(humificador, LOW);
}
