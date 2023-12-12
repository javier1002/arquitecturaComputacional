//  Alvaro Arroyo - Edinson Oviedo Martinez - Cristian Camilo Viveros
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>
#include "AsyncTaskLib.h"
#include "StateMachineLib.h"
#include "DHT.h"
#include "pins.h"
#include "const.h"

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); /**< Objeto para controlar una pantalla LCD */
DHT dht(DHTPIN, DHTTYPE); /**< Objeto para el sensor de temperatura y humedad  */
Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS); /**< Objeto para el keypad */
Servo buzzer; /**< Objeto para controlar un servo motor utilizado como buzzer */

enum LoginState
{
  ReadPass = 0, /**< Estado de lectura de la contraseña */
  CorrectPass = 1, /**< Estado de contraseña correcta */
  IncorrectPass = 2, /**< Estado de contraseña incorrecta */
  LockedSystem = 3, /**< Estado de sistema bloqueado */
  LightMonitor = 4, /**< Estado de monitoreo de luz */
  LightAlarm = 5, /**< Estado de alarma de luz */
  TemperatureMonitor = 6, /**< Estado de monitoreo de temperatura */
  MaxTemperature = 7 /**< Estado de temperatura máxima alcanzada */
};

enum Input
{
  Read = 0,           // Estado: Leer entrada
  Correct = 1,        // Estado: Contraseña correcta
  Incorrect = 2,      // Estado: Contraseña incorrecta
  LockedSys = 3,      // Estado: Sistema bloqueado
  LightMon = 4,       // Estado: Monitoreo de luz
  MinimalLight = 5,   // Estado: Luz mínima
  TempMonitor = 6,    // Estado: Monitoreo de temperatura
  MaxTemp = 7,        // Estado: Temperatura máxima
};

StateMachine stateMachine(8, 12);  // Máquina de estados con 8 estados y 12 transiciones
Input currentInput;                // Variable para almacenar la entrada actual

//define la lógica de la máquina de estados para el sistema de monitoreo, la máquina de estados tiene ocho estados:
void setupStateMachine(){
  stateMachine.AddTransition(ReadPass, CorrectPass, []() { return currentInput == Correct; });
  stateMachine.AddTransition(ReadPass, IncorrectPass, []() { return currentInput == Incorrect; });

  stateMachine.AddTransition(IncorrectPass, ReadPass, []() { return currentInput == Read; });
  stateMachine.AddTransition(IncorrectPass, LockedSystem, []() { return currentInput == LockedSys; });

  stateMachine.AddTransition(LockedSystem, ReadPass, []() { return currentInput == Read; });

  stateMachine.AddTransition(CorrectPass, LightMonitor, []() { return currentInput == LightMon; });

  stateMachine.AddTransition(LightMonitor, LightAlarm, []() { return currentInput == MinimalLight; });
  stateMachine.AddTransition(LightMonitor, TemperatureMonitor, []() { return currentInput == TempMonitor; });

  stateMachine.AddTransition(LightAlarm, LightMonitor, []() { return currentInput == LightMon; });
  
  stateMachine.AddTransition(TemperatureMonitor, LightMonitor, []() { return currentInput == LightMon; });
  stateMachine.AddTransition(TemperatureMonitor, MaxTemperature, []() { return currentInput == MaxTemp; });
  
  stateMachine.AddTransition(MaxTemperature, TemperatureMonitor, []() { return currentInput == TempMonitor; });

  //ESTADOS:
  //ReadPass: El sistema está esperando la entrada del usuario (contraseña).
  //CorrectPass: La contraseña ingresada es correcta.
  //IncorrectPass: La contraseña ingresada es incorrecta.
  //LockedSys: El sistema está bloqueado debido a intentos fallidos de contraseña.
  //LightMon: El sistema está monitoreando el nivel de luz.
  //MinimalLight: Se detecta una luz mínima.
  //MaxTemp: Se detecta una temperatura máxima.
  stateMachine.SetOnEntering(ReadPass, readPass);
  stateMachine.SetOnEntering(CorrectPass, provideAccess);
  stateMachine.SetOnEntering(IncorrectPass, incorrectPassword);
  stateMachine.SetOnEntering(LockedSystem, lockedSystem);
  stateMachine.SetOnEntering(LightMonitor, lightMonitor);
  stateMachine.SetOnEntering(LightAlarm, lightAlarm);
  stateMachine.SetOnEntering(TemperatureMonitor, tempMonitor);
  stateMachine.SetOnEntering(MaxTemperature, tempAlarm);
}

int timeToChange = 0;
int ledTimer = 0;
bool ledActivated = false;

void getLight();
void activeLed();
void getTemperature();
void activateTemBuzzer();

//se encarga de gestionar el tiempo de espera para la transición a un nuevo estado en la máquina de estados.
void timeOut(Input toState){ 
  if(timeToChange == 0){
    currentInput = toState;
    return;
  }
  timeToChange--;
}
/**Estas tareas asíncronas se utilizan para leer los sensores de luz y temperatura, activar el LED y el buzzer,
y cambiar el estado de la máquina de estados en función del tiempo de espera.*/
AsyncTask light(1000, true, getLight);
AsyncTask temp(1000, true, getTemperature);

AsyncTask timeOutLM(1000, true, []() { timeOut(Input::TempMonitor); });
AsyncTask timeOutLA(1000, true, []() { timeOut(Input::LightMon); });
AsyncTask activeLedAlm(100, true, activeLed);

AsyncTask timeOutTM(1000, true, []() { timeOut(Input::LightMon); });
AsyncTask timeOutTA(1000, true, []() { timeOut(Input::TempMonitor); });
AsyncTask activateBuzzer(1000, true, activateTemBuzzer);

//configura los pines, inicializa el lcd los sensores Establece el estado inicial de la máquina de estados 
void setup() {
  pinMode(RED_PIN, OUTPUT); /**< Configura el pin del componente de color rojo como salida. */
  pinMode(GREEN_PIN, OUTPUT); /**< Configura el pin del componente de color verde como salida. */
  pinMode(BLUE_PIN, OUTPUT); /**< Configura el pin del componente de color azul como salida. */
  pinMode(BUZZERPIN, OUTPUT); /**< Configura el pin del zumbador como salida. */

  lcd.begin(16, 2); /**< Inicializa la pantalla LCD con un formato de 16 columnas por 2 filas. */
  dht.begin(); /**< Inicializa el sensor de temperatura y humedad DHT. */

  setupStateMachine(); /**< Realiza la configuración de la máquina de estados. */
  stateMachine.SetState(ReadPass, false, true); /**< Establece el estado inicial de la máquina de estados como ReadPass. */
}
/*verifica la contraseña y Estas tareas asíncronas se utilizan para leer los sensores de luz y temperatura,
 activar el LED y el buzzer de temperatura, y cambiar el estado de la máquina de estados en función del tiempo de espera.*/
void loop() {
  char key = keypad.getKey();
  if (key) {
    passwordIntput[intput] = key;
    lcd.print('*');
    intput++;
    if(intput == PASS_LENGTH){
      currentInput = verifyPassword(passwordIntput);
      intput = 0;
    }
  }
  stateMachine.Update();
  light.Update();
  timeOutLM.Update();
  timeOutLA.Update();
  activeLedAlm.Update();
  timeOutTM.Update();
  temp.Update();
  timeOutTA.Update();
  activateBuzzer.Update();
}
//se encarga de preparar la pantalla LCD para recibir la contraseña.
void readPass(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese la clave:");
  lcd.setCursor(0, 1);
}

/*Esta función compara la contraseña ingresada con la contraseña almacenada.
 * Si las contraseñas coinciden, devuelve el estado Correcto; de lo contrario, incrementa
 * el número de intentos y devuelve el estado Incorrecto o de bloqueo.*/
int verifyPassword(char passwordIntput[PASS_LENGTH]){
  int i = 0;
  while(!access && i < PASS_LENGTH){
    if(passwordIntput[i] != password[i]){
      tries++;
      return Input::Incorrect;
    }
    i++;
  }
  return Input::Correct;
}
 /* Esta función realiza las siguientes acciones:
  - Limpia la pantalla LCD.
  - Muestra un mensaje indicando que el sistema está bloqueado.
  - Establece el color de fondo de la pantalla en rojo.
  - Espera 5 segundos antes de volver al estado de lectura.
 */
void lockedSystem(){
  lcd.clear();
  lcd.print("Sistema");
  lcd.setCursor(1,1);
  lcd.print("bloqueado");
  setColor(255, 0, 0);
  delay(5000);
  currentInput = Input::Read;
}
/*Esta función realiza las siguientes acciones:
  - Introduce un pequeño retraso de 200 milisegundos.
  - Verifica si se han alcanzado el número máximo de intentos permitidos.
  - Si se supera, cambia el estado actual a Sistema Bloqueado y sale de la función.
  - Limpia la pantalla LCD.
  - Muestra un mensaje indicando que la contraseña es incorrecta.
  - Establece el color de fondo de la pantalla en azul.
  - Espera 500 milisegundos antes de volver al estado de lectura.
 */
void incorrectPassword(){
  delay(200); 
  if(tries >= MAX_TRIES){
    currentInput = Input::LockedSys;
    return;
  }
  lcd.clear();
  lcd.print("Clave incorrecta");
  setColor(0, 0, 255);
  delay(500);
  currentInput = Input::Read;
}

/*Esta función realiza las siguientes acciones:
 - Establece el color de fondo de la pantalla en verde.
 - Limpia la pantalla LCD.
 - Muestra un mensaje indicando que el acceso ha sido concedido.
 - Espera 500 milisegundos antes de cambiar al estado de monitoreo de luz.
 */
void provideAccess(){  
  setColor(0, 255, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acceso concedido");
  delay(500);
  currentInput = Input::LightMon;
}

 /*Esta función realiza las siguientes acciones:
  - Establece el tiempo para cambiar a otro estado en 3 segundos.
  - Establece el color de fondo de la pantalla en negro.
  - Inicia el monitoreo de luz.
  - Inicia el temporizador de tiempo de espera para el monitoreo de luz.
  - Detiene los temporizadores relacionados con el monitoreo de temperatura y alarma.
  - Detiene la activación de la alarma y el monitoreo de temperatura.
 */
void lightMonitor(){
  timeToChange = 3;
  setColor(0, 0, 0);
  light.Start();
  timeOutLM.Start();
  timeOutLA.Stop();
  timeOutTM.Stop();
  activeLedAlm.Stop();
  temp.Stop();
}

 /*Esta función realiza las siguientes acciones:
  - Limpia la pantalla LCD.
  - Lee el valor analógico del sensor de luz.
  - Si el nivel de luz es inferior a 40, cambia al estado de luz mínima.
  - Muestra en la pantalla LCD el nivel de luz y el tiempo restante para cambiar de estado.
 */
void getLight(){
  lcd.clear();
  int l = analogRead(PHOTOCELLPIN);
  if(l < 40){
    currentInput = Input::MinimalLight;
  }
  lcd.setCursor(0, 0);
  lcd.print("Luz:");
  lcd.setCursor(5, 0);
  lcd.print(l);
  lcd.setCursor(15, 1);
  lcd.print(timeToChange);
}


 /* Esta función realiza las siguientes acciones:
  - Establece el tiempo para cambiar a otro estado en 4 segundos.
  - Detiene el monitoreo de luz.
  - Detiene el temporizador de tiempo de espera para el monitoreo de luz.
  - Inicia el temporizador de tiempo de espera para la alarma de luz.
  - Inicia la activación de la alarma y establece el indicador de activación de LED en verdadero.
 */
void lightAlarm(){
  timeToChange = 4;
  light.Stop();
  timeOutLM.Stop();
  timeOutLA.Start();
  activeLedAlm.Start();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Luz Minima");
  ledActivated = true;
}


 /* Esta función realiza las siguientes acciones:
  - Si el LED está activado y el temporizador del LED es menor que 5, establece el color del fondo de la pantalla en rojo y
 aumenta el temporizador del LED.
  - Si el temporizador del LED alcanza 5, desactiva el LED, reinicia el temporizador del LED y sale de la función.
  - Si el LED no está activado y el temporizador del LED es menor que 2, establece el color del fondo de la pantalla en negro y
    aumenta el temporizador del LED.
  - Si el temporizador del LED alcanza 2, activa el LED, reinicia el temporizador del LED y sale de la función.
 */
void activeLed(){
  lcd.setCursor(15,1);
  lcd.print(timeToChange);
  if(ledActivated && ledTimer < 5){
    setColor(255, 0, 0);
    ledTimer++;
    return;
  }
  if(ledTimer == 5){
    ledActivated = false;
    ledTimer = 0;
    return;
  }
  if(!ledActivated && ledTimer < 2){
    setColor(0, 0, 0);
    ledTimer++;
    return;
  }
  if(ledTimer == 2){
    ledActivated = true;
    ledTimer = 0;
    return;
  }
}
/*Esta función realiza las siguientes acciones:
  - Establece el tiempo para cambiar a otro estado en 6 segundos.
  - Inicia el temporizador de tiempo de espera para el monitoreo de temperatura.
  - Inicia el monitoreo de temperatura.
  - Detiene los temporizadores relacionados con el monitoreo de luz y alarma.
  - Detiene la activación del zumbador.
 */
void tempMonitor(){
  timeToChange = 6;
  timeOutTM.Start();
  temp.Start();
  timeOutLM.Stop();
  light.Stop();
  timeOutTA.Stop();
  activateBuzzer.Stop();
}
/* Esta función realiza las siguientes acciones:
  - Limpia la pantalla LCD.
  - Lee la temperatura del sensor DHT22.
  - Si la temperatura es superior a 30 grados Celsius, cambia al estado de temperatura máxima.
  - Muestra en la pantalla LCD el valor de la temperatura y el tiempo restante para cambiar de estado.
 */
void getTemperature(){
  lcd.clear();
  float t = dht.readTemperature();
  if(t > 30 ){
    currentInput = Input::MaxTemp;
  }
  lcd.setCursor(0,0);
  lcd.print("Tem:");
  lcd.setCursor(4,0);
  lcd.print(t);
  lcd.print("C");
  lcd.setCursor(15, 1);
  lcd.print(timeToChange);
}
 /* Esta función realiza las siguientes acciones:
 * - Establece el tiempo para cambiar a otro estado en 5 segundos.
 * - Inicia el temporizador de tiempo de espera para la alarma de temperatura.
 * - Detiene los temporizadores relacionados con el monitoreo de temperatura.
 * - Detiene el monitoreo de temperatura.
 * - Inicia la activación del zumbador.
 */
void tempAlarm(){
  timeToChange = 5;
  timeOutTA.Start();
  timeOutTM.Stop();
  temp.Stop();
  activateBuzzer.Start();
}
/*Esta función realiza las siguientes acciones:
 * - Limpia la pantalla LCD.
 * - Muestra un mensaje indicando que se alcanzó la temperatura máxima.
 * - Activa el zumbador con una frecuencia de 262 Hz durante 500 milisegundos.
 */
void activateTemBuzzer(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temperatura maxima");
  lcd.setCursor(0,1);
  lcd.print("Alcanzada");
  tone(BUZZERPIN, 262, 500);
}
/* Esta función utiliza señales PWM para ajustar los niveles de los componentes de color
  (rojo, verde y azul) y establecer así el color de fondo de la pantalla LCD.*/
void setColor(int red, int green, int blue){
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}
