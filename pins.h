// Definición de constantes para el keypad
const byte KEYPAD_ROWS = 4; /**< Número de filas en el teclado matricial */
const byte KEYPAD_COLS = 4; /**< Número de columnas en el teclado matricial */
const byte ROW_PINS[KEYPAD_ROWS] = {22, 24, 26, 28}; /**< Pines de fila del teclado matricial */
const byte COL_PINS[KEYPAD_COLS] = {30, 32, 34, 36}; /**< Pines de columna del teclado matricial */
const char KEYS[KEYPAD_ROWS][KEYPAD_COLS] = { /**< Matriz de teclas en el teclado matricial */
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'.', '0', '=', '/'}
};

// Definición de pines para LEDs RGB
const int RED_PIN = 8; /**< Pin para el LED rojo */
const int GREEN_PIN = 7; /**< Pin para el LED verde */
const int BLUE_PIN = 6; /**< Pin para el LED azul */

// Definición de pines para el sensor DHT22, el sensor de luz  y el buzzer
#define DHTPIN 40 /**< Pin para el sensor DHT22 */
#define DHTTYPE DHT22 /**< Tipo de sensor DHT22  */
#define PHOTOCELLPIN A0 /**< Pin para el sensor de luz */
#define BUZZERPIN 9 /**< Pin para el buzzer */

// Definición de pines para la pantalla LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2; /**< Pines para la pantalla LCD */