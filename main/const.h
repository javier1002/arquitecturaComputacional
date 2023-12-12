const byte PASS_LENGTH = 4; /**< Longitud de la contraseña */
const byte MAX_TRIES = 3; /**< Número máximo de intentos permitidos */
boolean access = false; /**< Indicador de acceso*/
char password[] = "1357"; /**< Contraseña predefinida */
char passwordInput[PASS_LENGTH]; /**< Almacena la entrada de la contraseña */
int input = 0; /**< Almacena la entrada numérica */
int tries = 0; /**< Contador de intentos realizados */