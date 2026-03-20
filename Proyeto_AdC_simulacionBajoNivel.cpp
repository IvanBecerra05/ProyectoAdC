/*
 * ============================================================================================================================
 * SIMULADOR CPU x86 - LISTA EN MEMORIA SIMULADA
 * Proyecto: Arquitectura de Computadoras
 * Modelo Von Neumann - Arquitectura Intel x86
 * 
 * Equipo: Carlos Ivan Becerra Quintero - 25110102
 * Jared Omar Hernandez Flores - 25110107
 * Perla Dayane Raygoza Pineda - 25110132
 * Andre Rodriguez Arias - 25110095
 * Diego Falcón Montoya - 25110090
 * Bárbara Thalia García Nuño - 25110091
 * Mónica Rotarescu Castillo - 25110100
 * ============================================================================================================================
 *
 * DESCRIPCION GENERAL:
 * El programa realiza cuatro operaciones fundamentales:
 *   1. Permite al usuario ingresar datos (cadenas de caracteres) que quedan almacenados
 *      como dato pendiente hasta recibir un comando.
 *   2. Almacena los datos en la RAM simulada a partir de la direccion 0x0310,
 *      donde cada elemento ocupa 16 bytes consecutivos (cadena ASCII + terminador 0x00).
 *   3. Muestra todos los elementos almacenados en la lista recorriendo la RAM.
 *   4. Consulta un elemento especifico de la lista usando su indice numerico.
 *
 * ============================================================================================================================
 * CODIGO DE ALTO NIVEL EQUIVALENTE
 * ============================================================================================================================
 *
 *   #include <iostream>
 *   #include <cstring>
 *   using namespace std;
 *
 *   char lista[256][16];
 *   int total = 0;
 *
 *   void interrupcion_teclado(char* buffer) {
 *       cin.getline(buffer, 16);
 *   }
 *
 *   int main() {
 *       char pendiente[16];
 *       char entrada[16];
 *       memset(pendiente, 0, sizeof(pendiente));
 *
 *       cout << "COMANDOS: '@'=guardar  '#'=leer  '$'=mostrar todo  '!'=salir\n\n";
 *
 *       while (true) {
 *           interrupcion_teclado(entrada);
 *
 *           if (strcmp(entrada, "@") == 0) {
 *               strcpy(lista[total], pendiente);
 *               total++;
 *               memset(pendiente, 0, sizeof(pendiente));
 *
 *           } else if (strcmp(entrada, "#") == 0) {
 *               int idx = atoi(pendiente);
 *               cout << "lista[" << idx << "] = \"" << lista[idx] << "\"\n";
 *
 *           } else if (strcmp(entrada, "$") == 0) {
 *               for (int i = 0; i < total; i++)
 *                   cout << "lista[" << i << "] = \"" << lista[i] << "\"\n";
 *
 *           } else if (strcmp(entrada, "!") == 0) {
 *               break;
 *
 *           } else {
 *               strcpy(pendiente, entrada);
 *           }
 *       }
 *       return 0;
 *   }
 *
 * ============================================================================================================================
 * ARQUITECTURA DEL SIMULADOR
 * ============================================================================================================================
 *
 * Este programa implementa un simulador de CPU basada en arquitectura x86
 * siguiendo el modelo de John von Neumann. Representa los componentes basicos:
 *
 *   CPU          : Registros de proposito general, pila, flujo, indice y flags.
 *   Memoria RAM  : Arreglo int RAM[65536]. Conceptualmente 65536 bytes (1 posicion = 1 byte).
 *   MAR / MBR    : Registros internos de Von Neumann. TODO acceso a RAM pasa por ellos.
 *   Unidad de Control (UC) : El switch(EIP) identifica y despacha cada instruccion.
 *   ALU          : Funciones MOV, ADD, MUL, DEC, CMP, XCHG y saltos.
 *   Stack        : Crece hacia abajo desde 0xFFFF. PUSH resta 4, POP suma 4.
 *
 * CICLO DE INSTRUCCION (Von Neumann):
 *   Fetch  : int eip_actual = EIP    (la UC lee la direccion apuntada por EIP)
 *   Decode : switch(EIP)             (la UC identifica que instruccion ejecutar)
 *   Execute: case correspondiente    (la ALU/UC ejecuta la microoperacion)
 *
 * ACCESO A MEMORIA: Todo acceso usa MAR y MBR sin excepcion.
 *   Lectura : MAR = addr  ->  MBR = RAM[MAR]  ->  dest = MBR
 *   Escritura: MAR = addr  ->  MBR = valor    ->  RAM[MAR] = MBR
 *
 * ============================================================================================================================
 * MAPA DE MEMORIA RAM (65536 posiciones, 1 byte c/u)
 * ============================================================================================================================
 *
 *   0x0000 - 0x000F  :  Reservado. EBP = 0 al inicio, esta zona nunca se usa para datos.
 *   0x0010 - 0x010F  :  CHAR MAP. Tabla de 256 posiciones hardcodeada antes de iniciar.
 *                        Cada posicion guarda su propio indice: RAM[0x0010 + ascii] = ascii.
 *                        Simula la ROM del controlador de teclado.
 *   0x0110 - 0x020F  :  BUFFER. Zona donde INT_TECLADO deposita la entrada del usuario via MAR/MBR.
 *   0x0210 - 0x030F  :  PENDING. Ultimo dato ingresado, esperando el comando '@' o '#'.
 *   0x0310 - 0x7FFF  :  LIST. Zona de almacenamiento de la lista.
 *                        Cada elemento ocupa 16 bytes: cadena ASCII terminada en 0x00.
 *   0x8000 - ...     :  Programa principal. EIP inicia en 0x8000.
 *   0xA000 - ...     :  INT_TECLADO. Handler de lectura del teclado.
 *   0xB000 - ...     :  INT_MEMCPY. Handler de copia de memoria byte a byte.
 *   0xC000 - ...     :  INT_PRINT. Handler de salida a consola.
 *   0xFFFF           :  Tope del stack. ESP inicia aqui y crece hacia abajo.
 *
 * ============================================================================================================================
 * INTERRUPCIONES
 * ============================================================================================================================
 *
 *   0xA000  INT_TECLADO : Lee cadena del teclado. Deposita en BUFFER via CHAR_MAP y MAR/MBR.
 *   0xB000  INT_MEMCPY  : Copia ELEM_SIZE bytes de RAM[ESI] a RAM[EDI] via MAR/MBR.
 *                         Se detiene al encontrar terminador 0x00. Simula REP MOVSB de x86.
 *   0xC000  INT_PRINT   : Imprime cadena en RAM[EAX] char por char via MAR/MBR.
 *                         Simula INT 21h funcion 09h de DOS / syscall write de Linux.
 *
 * ============================================================================================================================
 * TABLA DE INSTRUCCIONES
 * ============================================================================================================================
 *
 *  | DIRECCION | INSTRUCCION          | INTERPRETACION
 *  |===========================================================================================================================
 *  |  PROGRAMA PRINCIPAL (main) --- inicia en 0x8000
 *  |  BUCLE PRINCIPAL: Lee la entrada del usuario y detecta si es comando o dato.
 *  |===========================================================================================================================
 *  |  0x8000   | CALL 0xA000          | Llama a INT_TECLADO para leer la entrada del usuario.
 *  |           |                      | CALL = PUSH(dir_retorno) + JMP(destino).
 *  |           |                      |  -> ESP -= 4.
 *  |           |                      |  -> MAR = ESP, MBR = 0x8001, RAM[MAR] = MBR.
 *  |           |                      |  -> EIP = 0xA000.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8001   | MOV EAX,[0x0110]     | EAX = RAM[BUFFER_BASE].
 *  |           |                      | Lee el primer caracter de la entrada via MAR/MBR.
 *  |           |                      | Determina si la entrada es un comando o un dato.
 *  |           |                      |  -> MAR = 0x0110, MBR = RAM[MAR], EAX = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8002   | CMP EAX,'@'          | Compara EAX con '@' (CMD_SAVE = 0x40).
 *  |           |                      | La ALU hace EAX - 0x40 internamente.
 *  |           |                      | ZF = 1 si son iguales. ZF = 0 si no.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8003   | JE 0x8020            | Si ZF == 1: EIP = 0x8020 (BLOQUE SAVE).
 *  |           |                      | Si ZF == 0: EIP = 0x8004 (sigue comparando).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8004   | CMP EAX,'#'          | Compara EAX con '#' (CMD_READ = 0x23).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8005   | JE 0x8030            | Si ZF == 1: EIP = 0x8030 (BLOQUE READ).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8006   | CMP EAX,'$'          | Compara EAX con '$' (CMD_SHOW = 0x24).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8007   | JE 0x8040            | Si ZF == 1: EIP = 0x8040 (BLOQUE SHOW ALL).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8008   | CMP EAX,'!'          | Compara EAX con '!' (CMD_EXIT = 0x21).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8009   | JE 0x8060            | Si ZF == 1: EIP = 0x8060 (BLOQUE EXIT).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x800A   | MOV ESI,0x0110       | ESI = BUFFER_BASE.
 *  |           |                      | Apunta al inicio del buffer de entrada (fuente de la copia).
 *  |           |                      | ESI es el Source Index: registro dedicado a punteros
 *  |           |                      | fuente en operaciones de cadenas de la arquitectura x86.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x800B   | MOV EDI,0x0210       | EDI = PENDING_BASE.
 *  |           |                      | Apunta al inicio del buffer pendiente (destino de la copia).
 *  |           |                      | EDI es el Destination Index: registro dedicado a punteros
 *  |           |                      | destino en operaciones de cadenas de la arquitectura x86.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x800C   | CALL 0xB000          | Llama a INT_MEMCPY para copiar BUFFER -> PENDING.
 *  |           |                      | El dato queda en PENDING esperando el comando '@' o '#'.
 *  |           |                      |  -> MAR = ESP, MBR = 0x800D, RAM[MAR] = MBR.
 *  |           |                      |  -> EIP = 0xB000.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x800D   | JMP 0x8000           | Salto incondicional al inicio del bucle principal.
 *  |           |                      | El programa vuelve a esperar la siguiente entrada.
 *  |===========================================================================================================================
 *  |  BLOQUE SAVE --- 0x8020
 *  |  Guarda el dato pendiente en la siguiente celda libre de la lista.
 *  |  EBX apunta siempre a la siguiente celda libre (inicia en 0x0310 y avanza
 *  |  0x10 bytes por cada elemento guardado). EDX lleva la cuenta total.
 *  |===========================================================================================================================
 *  |  0x8020   | MOV ESI,0x0210       | ESI = PENDING_BASE (fuente: el dato pendiente).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8021   | MOV EDI,EBX          | EDI = EBX (destino: la siguiente celda libre de la lista).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8022   | CALL 0xB000          | Llama a INT_MEMCPY para copiar PENDING -> RAM[EBX].
 *  |           |                      | El dato queda guardado en la lista en RAM.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8023   | ADD EBX,0x10         | EBX = EBX + 16. Avanza al siguiente slot libre.
 *  |           |                      | Cada elemento ocupa 16 bytes (15 chars + terminador 0x00).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8024   | ADD EDX,1            | EDX = EDX + 1. Incrementa el contador de elementos.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8025   | MOV [0x0210],0       | RAM[PENDING_BASE] = 0x00.
 *  |           |                      | Limpia PENDING escribiendo el terminador en su primera
 *  |           |                      | posicion via MAR/MBR. Evita que el mismo dato se guarde
 *  |           |                      | dos veces si se presiona '@' sin ingresar un dato nuevo.
 *  |           |                      |  -> MAR = 0x0210, MBR = 0, RAM[MAR] = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8026   | JMP 0x8000           | Regresa al bucle principal.
 *  |===========================================================================================================================
 *  |  BLOQUE READ --- 0x8030
 *  |  Convierte el dato pendiente (cadena numerica) a entero via PARSE_NUM inline.
 *  |  Calcula la direccion del elemento: LIST_BASE + (indice * ELEM_SIZE).
 *  |  Muestra el elemento via INT_PRINT.
 *  |
 *  |  PARSE_NUM (loop inline 0x8032 - 0x803A):
 *  |  ECX acumula el numero. Por cada digito en PENDING:
 *  |    EAX = char leido de RAM[ESI].
 *  |    Si EAX == 0 (terminador): fin del loop -> salta a 0x803B.
 *  |    EAX = EAX - 48 ('0') = valor del digito.
 *  |    XCHG EAX, ECX: EAX <- acumulador, ECX <- digito actual.
 *  |      (necesario porque MUL en x86 opera SIEMPRE sobre EAX de forma implicita)
 *  |    MUL 10: EAX = EAX * 10. Resultado SIEMPRE en EAX (arquitectura x86 real).
 *  |    ADD ECX, EAX: ECX = digito + acumulador*10. ECX es el nuevo acumulador.
 *  |    ESI += 1. JMP 0x8032 (repite el loop).
 *  |===========================================================================================================================
 *  |  0x8030   | MOV ECX,0            | ECX = 0. Inicializa el acumulador del numero.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8031   | MOV ESI,0x0210       | ESI = PENDING_BASE. Puntero al inicio de la cadena.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8032   | MOV EAX,[ESI]        | EAX = RAM[ESI]. Lee el char actual via MAR/MBR.  <- PARSE_LOOP
 *  |           |                      |  -> MAR = ESI, MBR = RAM[MAR], EAX = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8033   | CMP EAX,0            | Compara EAX con el terminador 0x00.
 *  |           |                      | ZF = 1 si es fin de cadena.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8034   | JE 0x803B            | Si ZF == 1 (terminador): fin del PARSE_LOOP -> 0x803B.
 *  |           |                      | Si ZF == 0: continua procesando el digito.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8035   | ADD EAX,-48          | EAX = EAX - 48. Convierte char ASCII a valor de digito.
 *  |           |                      | Ejemplo: ASCII '3' = 51, 51 - 48 = 3.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8036   | XCHG EAX,ECX         | Intercambia EAX y ECX.
 *  |           |                      | EAX <- acumulador actual (listo para MUL).
 *  |           |                      | ECX <- digito actual (guardado para el ADD posterior).
 *  |           |                      | Necesario porque MUL en x86 real usa EAX de forma
 *  |           |                      | implicita: no existe "MUL ECX, 10", solo "MUL operando"
 *  |           |                      | y el resultado SIEMPRE queda en EAX.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8037   | MUL 10               | EAX = EAX * 10.
 *  |           |                      | Desplaza el acumulador una posicion decimal.
 *  |           |                      | Ejemplo: acumulador = 1 (del digito anterior) -> EAX = 10.
 *  |           |                      | Un solo operando, resultado SIEMPRE en EAX (x86 real).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8038   | ADD ECX,EAX          | ECX = ECX + EAX = digito + acumulador*10.
 *  |           |                      | Ejemplo: ECX = 3, EAX = 10 -> ECX = 13.
 *  |           |                      | ECX queda como nuevo acumulador del numero.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8039   | ADD ESI,1            | ESI += 1. Avanza al siguiente caracter de PENDING.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803A   | JMP 0x8032           | Salto incondicional al inicio del PARSE_LOOP.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803B   | MOV EAX,0x10         | EAX = ELEM_SIZE (16). Base del calculo de offset.
 *  |           |                      | Se carga en EAX porque MUL opera sobre EAX de forma
 *  |           |                      | implicita: MUL ECX = EAX * ECX, resultado en EAX.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803C   | MUL ECX              | EAX = EAX * ECX = 16 * indice = offset en bytes.
 *  |           |                      | Calcula cuantos bytes hay desde LIST_BASE hasta el elemento.
 *  |           |                      | Ejemplo: indice 2 -> EAX = 16 * 2 = 32 = 0x20.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803D   | ADD EAX,0x0310       | EAX = LIST_BASE + offset = direccion exacta del elemento.
 *  |           |                      | Ejemplo: indice 2 -> EAX = 0x0310 + 0x20 = 0x0330.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803E   | CALL 0xC000          | Llama a INT_PRINT para mostrar el elemento en RAM[EAX].
 *  |           |                      |  -> MAR = ESP, MBR = 0x803F, RAM[MAR] = MBR.
 *  |           |                      |  -> EIP = 0xC000.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x803F   | JMP 0x8000           | Regresa al bucle principal.
 *  |===========================================================================================================================
 *  |  BLOQUE SHOW ALL --- 0x8040
 *  |  Muestra todos los elementos de la lista.
 *  |  ECX = copia de EDX para no destruir el contador original.
 *  |  EAX = puntero de lectura que avanza ELEM_SIZE por iteracion.
 *  |===========================================================================================================================
 *  |  0x8040   | MOV ECX,EDX          | ECX = EDX. Copia el contador de elementos al bucle.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8041   | MOV EAX,0x0310       | EAX = LIST_BASE. Puntero de lectura al inicio de la lista.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8042   | CMP ECX,0            | Compara ECX con 0.  <- SHOW_LOOP
 *  |           |                      | ZF = 1 si lista vacia o se terminaron los elementos.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8043   | JE 0x8048            | Si ZF == 1 (fin): salta a 0x8048 (sale del bucle).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8044   | CALL 0xC000          | Llama a INT_PRINT para mostrar el elemento en RAM[EAX].
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8045   | ADD EAX,0x10         | EAX += 16. Avanza al siguiente elemento de la lista.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8046   | DEC ECX              | ECX--. Un elemento menos por mostrar.
 *  |           |                      | ZF = 1 SOLO si ECX llego a 0.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8047   | JNZ 0x8042           | Si ZF == 0 (quedan elementos): EIP = 0x8042 (repite).
 *  |           |                      | Si ZF == 1 (ECX llego a 0)   : EIP = 0x8048 (fin).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0x8048   | JMP 0x8000           | Regresa al bucle principal.
 *  |===========================================================================================================================
 *  |  BLOQUE EXIT --- 0x8060
 *  |===========================================================================================================================
 *  |  0x8060   | HLT                  | Halt. Detiene el ciclo de instruccion.
 *  |           |                      | HLT_flag = true. El while principal deja de iterar.
 *  |===========================================================================================================================
 *  |  INT_TECLADO --- 0xA000
 *  |  Rutina de Servicio de Interrupcion del teclado.
 *  |  Equivalente a INT 21h en DOS o syscall read en Linux.
 *  |  Por cada caracter tecleado:
 *  |    MAR = CHAR_MAP_BASE + ascii_del_char  (apunta al char en la tabla de mapeo)
 *  |    MBR = RAM[MAR]                        (bus de datos copia el valor)
 *  |    RAM[BUFFER_BASE + i] = MBR            (deposita el char en el buffer)
 *  |  Resultado: cadena en RAM[BUFFER_BASE] terminada en 0x00.
 *  |  Convencion x86: prologo PUSH EBP / MOV EBP,ESP
 *  |                  epilogo MOV ESP,EBP / POP EBP / RET
 *  |===========================================================================================================================
 *  |  0xA000   | PUSH EBP             |  -> ESP -= 4.
 *  |           |                      |  -> MAR = ESP, MBR = EBP, RAM[MAR] = MBR.
 *  |           |                      | Guarda el EBP del llamador en la pila.
 *  |           |                      | Sin este PUSH, al retornar EBP quedaria corrupto.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xA001   | MOV EBP,ESP          | EBP = ESP. Establece el nuevo marco de pila.
 *  |           |                      | Variables locales se referenciarian como [EBP-4], [EBP-8].
 *  |           |                      | Este handler no tiene locales, pero el prologo se respeta.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xA002   | INT LEER_TECLADO     | INTERRUPCION DE HARDWARE. El periferico entrega la cadena.
 *  |           |                      | La CPU pausa su flujo normal.
 *  |           |                      | Por cada caracter la UC busca su valor en CHAR_MAP via MAR/MBR
 *  |           |                      | y lo deposita en BUFFER_BASE:
 *  |           |                      |   MAR = CHAR_MAP_BASE + ascii_del_char.
 *  |           |                      |   MBR = RAM[MAR].
 *  |           |                      |   MAR = BUFFER_BASE + i.
 *  |           |                      |   RAM[MAR] = MBR.
 *  |           |                      | Al final escribe el terminador 0x00.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xA003   | MOV ESP,EBP          | ESP = EBP. Epilogo: restaura ESP (destruye locales si hubiera).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xA004   | POP EBP              |  -> MAR = ESP, MBR = RAM[MAR], EBP = MBR, ESP += 4.
 *  |           |                      | Restaura el EBP del llamador desde la pila.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xA005   | RET                  |  -> MAR = ESP, MBR = RAM[MAR], EIP = MBR, ESP += 4.
 *  |           |                      | Saca la dir. de retorno del stack e inyecta en EIP.
 *  |           |                      | EIP vuelve siempre a 0x8001.
 *  |===========================================================================================================================
 *  |  INT_MEMCPY --- 0xB000
 *  |  Copia ELEM_SIZE bytes de RAM[ESI] a RAM[EDI] byte a byte via MAR/MBR.
 *  |  Simula la instruccion REP MOVSB (Move String Byte con repeticion) de x86.
 *  |  ESI = direccion fuente, EDI = direccion destino.
 *  |  Se detiene al encontrar el terminador 0x00 o al agotar ECX.
 *  |  Convencion x86: prologo/epilogo estandar.
 *  |===========================================================================================================================
 *  |  0xB000   | PUSH EBP             | Prologo: guarda EBP del llamador.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB001   | MOV EBP,ESP          | Prologo: establece nuevo marco de pila.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB002   | MOV ECX,0x10         | ECX = 16 (ELEM_SIZE). Contador de bytes a copiar.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB003   | MOV EAX,[ESI]        | EAX = RAM[ESI]. Lee el byte fuente via MAR/MBR.  <- MEMCPY_LOOP
 *  |           |                      |  -> MAR = ESI, MBR = RAM[MAR], EAX = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB004   | MOV [EDI],EAX        | RAM[EDI] = EAX. Escribe el byte destino via MAR/MBR.
 *  |           |                      |  -> MAR = EDI, MBR = EAX, RAM[MAR] = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB005   | CMP EAX,0            | Compara EAX con el terminador 0x00.
 *  |           |                      | ZF = 1 si se acaba de copiar el terminador.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB006   | JE 0xB00C            | Si ZF == 1 (terminador copiado): salta al epilogo.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB007   | ADD ESI,1            | ESI += 1. Avanza el puntero fuente al siguiente byte.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB008   | ADD EDI,1            | EDI += 1. Avanza el puntero destino al siguiente byte.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB009   | DEC ECX              | ECX--. Un byte menos por copiar.
 *  |           |                      | ZF = 1 si ECX llego a 0 (slot completo).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB00A   | JNZ 0xB003           | Si ZF == 0 (quedan bytes y no hay terminador): repite.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB00B   | MOV [EDI],0          | RAM[EDI] = 0x00. Garantiza terminador en la ultima posicion.
 *  |           |                      |  -> MAR = EDI, MBR = 0, RAM[MAR] = MBR.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB00C   | MOV ESP,EBP          | Epilogo: restaura ESP.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB00D   | POP EBP              | Epilogo: restaura EBP del llamador.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xB00E   | RET                  | Epilogo: regresa al llamador (0x800D, 0x8023 o 0x803F segun contexto).
 *  |===========================================================================================================================
 *  |  INT_PRINT --- 0xC000
 *  |  Muestra en consola la cadena en RAM[EAX] char por char via MAR/MBR.
 *  |  Simula INT 21h funcion 09h (print string) de DOS, o syscall write de Linux.
 *  |  EAX = direccion base del elemento a imprimir en RAM.
 *  |  Calcula el indice como: (EAX - LIST_BASE) / ELEM_SIZE.
 *  |  Convencion x86: prologo/epilogo estandar.
 *  |===========================================================================================================================
 *  |  0xC000   | PUSH EBP             | Prologo: guarda EBP del llamador.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xC001   | MOV EBP,ESP          | Prologo: establece nuevo marco de pila.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xC002   | INT PRINT_STR        | INTERRUPCION DE SALIDA. Muestra la cadena en RAM[EAX].
 *  |           |                      | Por cada char del elemento:
 *  |           |                      |   MAR = EAX + i, MBR = RAM[MAR].
 *  |           |                      |   Si MBR == 0: fin de cadena.
 *  |           |                      |   Imprime (char)MBR en consola.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xC003   | MOV ESP,EBP          | Epilogo: restaura ESP.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xC004   | POP EBP              | Epilogo: restaura EBP del llamador.
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  0xC005   | RET                  | Epilogo: regresa al llamador (0x8045 o 0x803F segun contexto).
 *  |  -------------------------------------------------------------------------------------------------------------------------
 *  |  FIN DEL PROGRAMA.
 *  |  EDX = cantidad total de elementos guardados en la lista.
 *
 * ============================================================================================================================
 * FLUJO COMPLETO DE EJECUCION - EJEMPLO CON 3 DATOS
 * ============================================================================================================================
 * El usuario ingresara: "hola", "mundo", "cpu".
 * Luego mostrara toda la lista, consultara el indice 1, y saldra.
 *
 * ============================================================================================================================
 * ENTRADA 1: DATO "hola"
 * ============================================================================================================================
 * EIP = 0x8000.
 * INSTRUCCION: CALL 0xA000
 *   ESP -= 4. MAR = ESP, MBR = 0x8001, RAM[MAR] = MBR. EIP = 0xA000.
 *
 * INT_TECLADO:
 *   PUSH EBP: EBP (= 0) se guarda en pila. ESP -= 4.
 *   MOV EBP,ESP: EBP = ESP. Nuevo marco de pila.
 *   INT LEER_TECLADO: el usuario escribe "hola".
 *   La CPU busca cada char en CHAR_MAP via MAR/MBR:
 *     MAR = 0x0010 + 'h' (104) = 0x0078, MBR = 104. MAR = 0x0110. RAM[0x0110] = 104.
 *     MAR = 0x0010 + 'o' (111) = 0x007F, MBR = 111. MAR = 0x0111. RAM[0x0111] = 111.
 *     MAR = 0x0010 + 'l' (108) = 0x007C, MBR = 108. MAR = 0x0112. RAM[0x0112] = 108.
 *     MAR = 0x0010 + 'a' (97)  = 0x0071, MBR = 97.  MAR = 0x0113. RAM[0x0113] = 97.
 *     MAR = 0x0010 + 0   (0)   = 0x0010, MBR = 0.   MAR = 0x0114. RAM[0x0114] = 0. (terminador)
 *   BUFFER[0x0110...0x0114] = "hola\0".
 *   MOV ESP,EBP / POP EBP / RET: epilogo estandar. EIP = 0x8001.
 *
 * DE VUELTA EN EL BUCLE PRINCIPAL:
 *   MOV EAX,[0x0110]: MAR=0x0110, MBR=104 ('h'). EAX = 104.
 *   CMP/JE 0x8002-0x8009: EAX (104) != '@','#','$','!'. ZF=0. Ningun salto.
 *   MOV ESI,0x0110 / MOV EDI,0x0210 / CALL 0xB000: copia BUFFER -> PENDING.
 *
 * INT_MEMCPY (copia "hola" de BUFFER a PENDING):
 *   ECX = 16. MEMCPY_LOOP:
 *     Lee 'h' de RAM[0x0110]. Escribe en RAM[0x0210]. CMP 'h',0: ZF=0. ESI++, EDI++. DEC ECX.
 *     ... (repite para 'o', 'l', 'a') ...
 *     Lee 0x00. Escribe 0x00 en PENDING. CMP 0,0: ZF=1. JE epilogo.
 *   PENDING[0x0210...0x0214] = "hola\0". RET -> EIP = 0x800D.
 *   JMP 0x8000. "hola" queda pendiente.
 *
 * ============================================================================================================================
 * ENTRADA 2: COMANDO "@" - GUARDAR "hola"
 * ============================================================================================================================
 * INT_TECLADO lee "@". BUFFER[0x0110] = 64. EAX = 64.
 * CMP EAX,'@': ZF=1. JE 0x8020: EIP = 0x8020 (BLOQUE SAVE).
 *
 * BLOQUE SAVE:
 *   MOV ESI,0x0210: fuente = PENDING ("hola").
 *   MOV EDI,EBX: EBX = 0x0310 (LIST_BASE). EDI = 0x0310.
 *   CALL 0xB000: INT_MEMCPY copia "hola" de PENDING a LIST[0].
 *     RAM[0x0310...0x0314] = "hola\0".
 *   ADD EBX,0x10: EBX = 0x0320. Siguiente slot libre.
 *   ADD EDX,1: EDX = 1. La lista tiene 1 elemento.
 *   MOV [0x0210],0: RAM[0x0210] = 0. PENDING se limpia.
 *   JMP 0x8000.
 *
 * ============================================================================================================================
 * ENTRADAS 3-4: "mundo" + "@"  /  ENTRADAS 5-6: "cpu" + "@"
 * ============================================================================================================================
 * El mismo proceso se repite:
 *   "mundo" -> PENDING -> MEMCPY -> LIST[1] en 0x0320. EBX = 0x0330. EDX = 2.
 *   "cpu"   -> PENDING -> MEMCPY -> LIST[2] en 0x0330. EBX = 0x0340. EDX = 3.
 *
 * Estado de la lista en RAM:
 *   0x0310...0x031F : "hola\0"
 *   0x0320...0x032F : "mundo\0"
 *   0x0330...0x033F : "cpu\0"
 *
 * ============================================================================================================================
 * ENTRADA 7: COMANDO "$" - MOSTRAR TODA LA LISTA
 * ============================================================================================================================
 * EAX = 36 ('$'). JE 0x8040: EIP = 0x8040 (BLOQUE SHOW ALL).
 *
 * BLOQUE SHOW ALL:
 *   MOV ECX,EDX: ECX = 3. MOV EAX,0x0310.
 *   SHOW_LOOP - Iteracion 1:
 *     CMP ECX,0: ZF=0. CALL 0xC000: imprime "hola". ADD EAX,0x10: EAX=0x0320.
 *     DEC ECX: ECX=2, ZF=0. JNZ 0x8042: repite.
 *   SHOW_LOOP - Iteracion 2:
 *     CALL 0xC000: imprime "mundo". ADD EAX,0x10: EAX=0x0330.
 *     DEC ECX: ECX=1, ZF=0. JNZ: repite.
 *   SHOW_LOOP - Iteracion 3:
 *     CALL 0xC000: imprime "cpu". ADD EAX,0x10: EAX=0x0340.
 *     DEC ECX: ECX=0, ZF=1. JNZ: ZF=1, NO salta. Fin del bucle.
 *   JMP 0x8000.
 *
 * ============================================================================================================================
 * ENTRADA 8: DATO "1" (indice a consultar)
 * ============================================================================================================================
 * INT_TECLADO lee "1". BUFFER[0x0110] = 49 ('1'). EAX = 49.
 * No coincide con ningun comando. MEMCPY copia "1" a PENDING.
 * PENDING[0x0210] = 49, PENDING[0x0211] = 0x00.
 *
 * ============================================================================================================================
 * ENTRADA 9: COMANDO "#" - LEER lista[1]
 * ============================================================================================================================
 * EAX = 35 ('#'). JE 0x8030: EIP = 0x8030 (BLOQUE READ).
 *
 * BLOQUE READ - PARSE_NUM:
 *   MOV ECX,0. MOV ESI,0x0210.
 *   PARSE_LOOP - Iteracion 1:
 *     MOV EAX,[0x0210]: EAX = 49 ('1'). CMP EAX,0: ZF=0.
 *     ADD EAX,-48: EAX = 1 (valor del digito).
 *     XCHG EAX,ECX: EAX = 0 (acumulador), ECX = 1 (digito).
 *     MUL 10: EAX = 0 * 10 = 0. (resultado en EAX, x86 real)
 *     ADD ECX,EAX: ECX = 1 + 0 = 1. (nuevo acumulador)
 *     ADD ESI,1: ESI = 0x0211.
 *     JMP 0x8032.
 *   PARSE_LOOP - Iteracion 2:
 *     MOV EAX,[0x0211]: EAX = 0 (terminador). CMP EAX,0: ZF=1.
 *     JE 0x803B: fin del PARSE_LOOP. ECX = 1 (el indice buscado).
 *   CALCULO DE DIRECCION:
 *     MOV EAX,0x10: EAX = 16.
 *     MUL ECX: EAX = 16 * 1 = 16 = 0x10. (offset)
 *     ADD EAX,0x0310: EAX = 0x0310 + 0x10 = 0x0320. (direccion de lista[1])
 *   CALL 0xC000: INT_PRINT muestra RAM[0x0320] = "mundo". Imprime: lista[1] = "mundo".
 *   RET -> EIP = 0x803F. JMP 0x8000.
 *
 * ============================================================================================================================
 * ENTRADA 10: COMANDO "!" - SALIR
 * ============================================================================================================================
 * EAX = 33 ('!'). JE 0x8060: EIP = 0x8060.
 * HLT: HLT_flag = true. El while principal deja de iterar.
 * El programa finaliza mostrando:
 *   Elementos en lista : 3
 *   Ciclos de reloj    : N
 *
 * ============================================================================================================================
 * INICIALIZACION DE VARIABLES
 * ============================================================================================================================
 *
 *   int RAM[65536] = {0};      // RAM simulada: 65536 posiciones de 1 byte cada una.
 *
 *   // Registros de proposito general
 *   int EAX = 0;   // Accumulator: resultados, primer char del buffer, puntero de lectura.
 *   int EBX = 0;   // Base: puntero a la siguiente celda libre de la lista (inicia en LIST_BASE).
 *   int ECX = 0;   // Counter: contador de bucles y acumulador de PARSE_NUM.
 *   int EDX = 0;   // Data: contador de elementos guardados en la lista.
 *
 *   // Registros de pila y flujo
 *   int ESP = 0;   // Stack Pointer: tope de la pila (crece hacia abajo desde 0xFFFF).
 *   int EBP = 0;   // Base Pointer: ancla del marco de funcion. Inicia en 0 (sin frame previo).
 *   int EIP = 0;   // Instruction Pointer: direccion de instruccion actual. Inicia en 0x8000.
 *
 *   // Registros de indice para operaciones de cadenas
 *   int ESI = 0;   // Source Index: direccion fuente para INT_MEMCPY.
 *   int EDI = 0;   // Destination Index: direccion destino para INT_MEMCPY.
 *
 *   // Registros internos de memoria (Von Neumann)
 *   int MAR = 0;   // Memory Address Register: direccion a leer o escribir en RAM.
 *   int MBR = 0;   // Memory Buffer Register: dato en transito hacia o desde RAM.
 *
 *   // Bandera
 *   int ZF  = 0;   // Zero Flag: 1 si el resultado de la ultima operacion fue exactamente 0.
 *
 *   bool HLT_flag = false;
 *   int ciclo = 1;
 *
 * ============================================================================================================================
 * FUNCIONES DE VISUALIZACION
 * ============================================================================================================================
 *
 *   void imprimir_encabezado()
 *     Imprime la fila de titulos de la tabla de ejecucion.
 *     Se llama al inicio y cada vez que INT_TECLADO recibe una nueva entrada,
 *     para separar visualmente cada iteracion del bucle.
 *
 *   void imprimir_fila(int eip, const char* inst)
 *     Imprime una fila de la tabla con el estado completo del CPU en ese ciclo.
 *     Los registros se muestran en hexadecimal via (unsigned short) + %04X.
 *     ZF y TOPE se muestran en decimal via %d.
 *     TOPE = RAM[ESP]: el valor almacenado actualmente en el tope de la pila.
 *
 * ============================================================================================================================
 * HARDWARE ALU Y UNIDAD DE CONTROL (Microoperaciones)
 * ============================================================================================================================
 *
 *   MOV(dest, valor)     : dest = valor. Carga inmediato en registro. No modifica ZF. EIP++.
 *   MOV_REG(dest, src)   : dest = src. Copia registro a registro. No modifica ZF. EIP++.
 *   MOV_MEM(dest, addr)  : MAR=addr, MBR=RAM[MAR], dest=MBR. Lectura de RAM via MAR/MBR. EIP++.
 *   ADD(dest, valor)     : dest += valor. ZF=1 si resultado==0. EIP++.
 *   XCHG(a, b)           : Intercambia a y b mediante variable temporal. No modifica ZF. EIP++.
 *                          Instruccion x86 real. Usada para preparar EAX antes de MUL.
 *   MUL(valor)           : EAX *= valor. Resultado SIEMPRE en EAX (x86 real).
 *                          Un solo operando. ZF=1 si resultado==0. EIP++.
 *   DEC(dest)            : dest--. ZF=1 si resultado==0. No modifica CF (igual que x86). EIP++.
 *   CMP(a, b)            : ZF = (a==b)?1:0. No guarda resultado (igual que x86 real). EIP++.
 *   JE(addr)             : Si ZF==1: EIP=addr. Si ZF==0: EIP++. (Jump if Equal/Zero)
 *   JNZ(addr)            : Si ZF==0: EIP=addr. Si ZF==1: EIP++. (Jump if Not Zero)
 *   JMP(addr)            : EIP=addr. Salto incondicional.
 *   PUSH(valor)          : ESP-=4. MAR=ESP, MBR=valor, RAM[MAR]=MBR. EIP++.
 *   POP_VAL()            : MAR=ESP, MBR=RAM[MAR], v=MBR, ESP+=4. EIP++. Retorna v.
 *   CALL(dest, ret)      : ESP-=4. MAR=ESP, MBR=ret, RAM[MAR]=MBR. EIP=dest.
 *                          Guarda dir. de retorno en stack y salta al destino.
 *   RET_OP()             : MAR=ESP, MBR=RAM[MAR], EIP=MBR, ESP+=4.
 *                          Saca dir. de retorno del stack e inyecta en EIP.
 *   HLT_OP()             : HLT_flag=true. Detiene el ciclo de instruccion.
 */

#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

// =========================================================================
// ZONAS DE MEMORIA
// =========================================================================
#define CHAR_MAP_BASE 0x0010
#define BUFFER_BASE 0x0110
#define PENDING_BASE 0x0210
#define LIST_BASE 0x0310
#define ELEM_SIZE 0x0010 // 16 bytes por elemento

// Codigos ASCII de los comandos
#define CMD_SAVE 0x40 // '@'
#define CMD_READ 0x23 // '#'
#define CMD_EXIT 0x21 // '!'
#define CMD_SHOW 0x24 // '$'

// =========================================================================
// 1. ARQUITECTURA (CPU + RAM)
// =========================================================================
int RAM[65536] = {0};

// Registros de proposito general
int EAX = 0; // Accumulator: resultados, datos temporales, puntero de lectura
int EBX = 0; // Base: puntero a la siguiente celda libre de la lista
int ECX = 0; // Counter: contador de bucles y conversion numerica
int EDX = 0; // Data: contador de elementos guardados en la lista

// Registros de pila y flujo
int ESP = 0; // Stack Pointer: tope de la pila (crece hacia abajo)
int EBP = 0; // Base Pointer: ancla del marco de funcion
int EIP = 0; // Instruction Pointer: direccion de instruccion actual

// Registros de segmento para INT_MEMCPY
int ESI = 0; // Source Index: direccion fuente para la copia
int EDI = 0; // Destination Index: direccion destino para la copia

// Registros internos de memoria (Von Neumann)
int MAR = 0; // Memory Address Register
int MBR = 0; // Memory Buffer Register

// Bandera
int ZF = 0; // Zero Flag: 1 si resultado fue cero

bool HLT_flag = false;
int ciclo = 1;

// =========================================================================
// VISUALIZACION
// =========================================================================
void imprimir_encabezado()
{
    printf("------------------------------------------------------------------------------------------------------------------------------------\n");
    printf("CICLO | EIP    | INSTRUCCION            | EAX  | EBX  | ECX  | EDX  | ESI  | EDI  | EBP  | ESP  | MAR  | MBR  | ZF | TOPE\n");
    printf("------------------------------------------------------------------------------------------------------------------------------------\n");
}

void imprimir_fila(int eip, const char *inst)
{
    int tope = RAM[ESP];
    printf("%04d  | 0x%04X | %-22s | %04X | %04X | %04X | %04X | %04X | %04X | %04X | %04X | %04X | %04X |  %d | %d\n",
           ciclo, eip, inst,
           (unsigned short)EAX, (unsigned short)EBX,
           (unsigned short)ECX, (unsigned short)EDX,
           (unsigned short)ESI, (unsigned short)EDI,
           (unsigned short)EBP, (unsigned short)ESP,
           (unsigned short)MAR, (unsigned short)MBR,
           ZF, tope);
}

// =========================================================================
// CHAR MAP: hardcodea ASCII 0-255 en RAM antes de iniciar el programa.
// =========================================================================
void inicializar_char_map()
{
    for (int c = 0; c <= 255; c++)
        RAM[CHAR_MAP_BASE + c] = c;
}

// =========================================================================
// 2. HARDWARE ALU Y UNIDAD DE CONTROL (Microoperaciones)
// =========================================================================

// MOV: carga valor inmediato en registro.
void MOV(int &dest, int valor)
{
    dest = valor;
    EIP++;
}

// MOV_REG: copia registro a registro.
void MOV_REG(int &dest, int src)
{
    dest = src;
    EIP++;
}

// MOV_MEM: carga valor de memoria (RAM[addr]) a registro via MAR/MBR.
void MOV_MEM(int &dest, int addr)
{
    MAR = addr;
    MBR = RAM[MAR];
    dest = MBR;
    EIP++;
}

// ADD: suma. ZF=1 si resultado==0.
void ADD(int &dest, int valor)
{
    dest += valor;
    ZF = (dest == 0) ? 1 : 0;
    EIP++;
}

// XCHG: intercambia dos registros. Instruccion x86 real.
void XCHG(int &a, int &b)
{
    int tmp = a;
    a = b;
    b = tmp;
    EIP++;
}

// MUL: en x86 real MUL solo recibe UN operando.
// Multiplica EAX por ese operando. Resultado SIEMPRE en EAX.
void MUL(int valor)
{
    EAX *= valor;
    ZF = (EAX == 0) ? 1 : 0;
    EIP++;
}

// DEC: decrementa. ZF=1 si resultado==0. DEC no modifica CF en x86 real.
void DEC(int &dest)
{
    dest--;
    ZF = (dest == 0) ? 1 : 0;
    EIP++;
}

// CMP: compara a y b. ZF=1 si iguales. No guarda resultado (igual que x86).
void CMP(int a, int b)
{
    ZF = (a == b) ? 1 : 0;
    EIP++;
}

// JE: salta si ZF==1 (Jump if Equal / Jump if Zero).
void JE(int addr) { EIP = (ZF == 1) ? addr : EIP + 1; }

// JNZ: salta si ZF==0 (Jump if Not Zero).
void JNZ(int addr) { EIP = (ZF == 0) ? addr : EIP + 1; }

// JMP: salto incondicional.
void JMP(int addr) { EIP = addr; }

// PUSH: ESP-=4, RAM[ESP]=valor via MAR/MBR.
void PUSH(int valor)
{
    ESP -= 4;
    MAR = ESP;
    MBR = valor;
    RAM[MAR] = MBR;
    EIP++;
}

// POP_VAL: MBR=RAM[ESP] via MAR/MBR, ESP+=4.
int POP_VAL()
{
    MAR = ESP;
    MBR = RAM[MAR];
    int v = MBR;
    ESP += 4;
    EIP++;
    return v;
}

// CALL: guarda dir. retorno en stack via MAR/MBR, salta al destino.
void CALL(int dest, int ret)
{
    ESP -= 4;
    MAR = ESP;
    MBR = ret;
    RAM[MAR] = MBR;
    EIP = dest;
}

// RET_OP: saca dir. retorno del stack via MAR/MBR -> EIP.
void RET_OP()
{
    MAR = ESP;
    MBR = RAM[MAR];
    EIP = MBR;
    ESP += 4;
}

// HLT_OP: detiene el ciclo de instruccion.
void HLT_OP() { HLT_flag = true; }

// =========================================================================
// 3. CICLO DE INSTRUCCION (Fetch - Decode - Execute)
// =========================================================================
int main()
{

    inicializar_char_map();

    EAX = 0;
    EBX = LIST_BASE;
    ECX = 0;
    EDX = 0;
    ESI = 0;
    EDI = 0;
    ESP = 0xFFFF;
    EBP = 0;
    EIP = 0x8000;
    MAR = 0;
    MBR = 0;
    ZF = 0;
    HLT_flag = false;
    ciclo = 1;

    printf("=== SIMULADOR CPU x86 - LISTA EN MEMORIA SIMULADA ===\n");
    printf("Char map : 0x0010-0x010F | Buffer  : 0x0110-0x020F\n");
    printf("Pending  : 0x0210-0x030F | Lista   : 0x0310-0x7FFF\n");
    printf("Programa : 0x8000        | INT_TECLADO : 0xA000\n");
    printf("INT_MEMCPY : 0xB000      | INT_PRINT   : 0xC000\n\n");
    printf("COMANDOS: '@'=guardar '#'=leer '$'=mostrar todo '!'=salir\n\n");
    imprimir_encabezado();

    while (!HLT_flag)
    {
        int eip_actual = EIP;

        switch (EIP)
        {

            // =================================================================
            // PROGRAMA PRINCIPAL --- 0x8000
            // Bucle principal: lee entrada, detecta comando o dato.
            // =================================================================

        case 0x8000:
            // Lee la entrada del usuario via INT_TECLADO.
            // CALL guarda 0x8001 en stack, salta a 0xA000.
            CALL(0xA000, 0x8001);
            imprimir_fila(eip_actual, "CALL 0xA000");
            break;

        case 0x8001:
            // Lee el primer char del BUFFER via MAR/MBR a EAX.
            // Determina si la entrada es un comando o un dato.
            MOV_MEM(EAX, BUFFER_BASE);
            imprimir_fila(eip_actual, "MOV EAX,[0x0110]");
            break;

        case 0x8002:
            // Compara EAX con '@' (CMD_SAVE).
            CMP(EAX, CMD_SAVE);
            imprimir_fila(eip_actual, "CMP EAX,'@'");
            break;

        case 0x8003:
            // Si es '@' -> BLOQUE SAVE (0x8020).
            JE(0x8020);
            imprimir_fila(eip_actual, "JE 0x8020");
            break;

        case 0x8004:
            // Compara EAX con '#' (CMD_READ).
            CMP(EAX, CMD_READ);
            imprimir_fila(eip_actual, "CMP EAX,'#'");
            break;

        case 0x8005:
            // Si es '#' -> BLOQUE READ (0x8030).
            JE(0x8030);
            imprimir_fila(eip_actual, "JE 0x8030");
            break;

        case 0x8006:
            // Compara EAX con '$' (CMD_SHOW).
            CMP(EAX, CMD_SHOW);
            imprimir_fila(eip_actual, "CMP EAX,'$'");
            break;

        case 0x8007:
            // Si es '$' -> BLOQUE SHOW ALL (0x8040).
            JE(0x8040);
            imprimir_fila(eip_actual, "JE 0x8040");
            break;

        case 0x8008:
            // Compara EAX con '!' (CMD_EXIT).
            CMP(EAX, CMD_EXIT);
            imprimir_fila(eip_actual, "CMP EAX,'!'");
            break;

        case 0x8009:
            // Si es '!' -> BLOQUE EXIT (0x8060).
            JE(0x8060);
            imprimir_fila(eip_actual, "JE 0x8060");
            break;

        case 0x800A:
            // Es un DATO: copia BUFFER -> PENDING via INT_MEMCPY.
            // ESI = fuente (BUFFER_BASE), EDI = destino (PENDING_BASE).
            MOV(ESI, BUFFER_BASE);
            imprimir_fila(eip_actual, "MOV ESI,0x0110");
            break;

        case 0x800B:
            MOV(EDI, PENDING_BASE);
            imprimir_fila(eip_actual, "MOV EDI,0x0210");
            break;

        case 0x800C:
            // CALL INT_MEMCPY: copia ESI -> EDI (ELEM_SIZE bytes via MAR/MBR).
            CALL(0xB000, 0x800D);
            imprimir_fila(eip_actual, "CALL 0xB000");
            break;

        case 0x800D:
            // Regresa al inicio del bucle principal.
            JMP(0x8000);
            imprimir_fila(eip_actual, "JMP 0x8000");
            break;

            // =================================================================
            // BLOQUE SAVE --- 0x8020
            // Copia PENDING -> slot actual (EBX) via INT_MEMCPY.
            // Avanza EBX al siguiente slot libre e incrementa EDX.
            // =================================================================

        case 0x8020:
            // ESI = PENDING_BASE (fuente del dato a guardar).
            MOV(ESI, PENDING_BASE);
            imprimir_fila(eip_actual, "MOV ESI,0x0210");
            break;

        case 0x8021:
            // EDI = EBX (destino: siguiente celda libre de la lista).
            MOV_REG(EDI, EBX);
            imprimir_fila(eip_actual, "MOV EDI,EBX");
            break;

        case 0x8022:
            // CALL INT_MEMCPY: copia el dato pendiente al slot de la lista.
            CALL(0xB000, 0x8023);
            imprimir_fila(eip_actual, "CALL 0xB000");
            break;

        case 0x8023:
            // EBX += ELEM_SIZE: avanza al siguiente slot libre.
            ADD(EBX, ELEM_SIZE);
            imprimir_fila(eip_actual, "ADD EBX,0x10");
            break;

        case 0x8024:
            // EDX += 1: incrementa el contador de elementos guardados.
            ADD(EDX, 1);
            imprimir_fila(eip_actual, "ADD EDX,1");
            break;

        case 0x8025:
            // Limpia PENDING: escribe 0x00 en la primera posicion via MAR/MBR.
            MAR = PENDING_BASE;
            MBR = 0;
            RAM[MAR] = MBR;
            EIP++;
            imprimir_fila(eip_actual, "MOV [0x0210],0");
            break;

        case 0x8026:
            JMP(0x8000);
            imprimir_fila(eip_actual, "JMP 0x8000");
            break;

            // =================================================================
            // BLOQUE READ --- 0x8030
            // Convierte PENDING a indice entero via PARSE_NUM (loop inline).
            // Calcula la direccion: LIST_BASE + (indice * ELEM_SIZE).
            // Muestra el elemento via INT_PRINT.
            //
            // PARSE_NUM inline:
            //   ECX = 0 (acumulador del numero)
            //   Por cada char en PENDING:
            //     EAX = char - '0'  (digito)
            //     ECX = ECX * 10 + EAX
            // =================================================================

        case 0x8030:
            // Inicializa ECX = 0 (acumulador del numero).
            MOV(ECX, 0);
            imprimir_fila(eip_actual, "MOV ECX,0");
            break;

        case 0x8031:
            // MAR = PENDING_BASE, inicializa ESI como puntero de lectura.
            MOV(ESI, PENDING_BASE);
            imprimir_fila(eip_actual, "MOV ESI,0x0210");
            break;

        // PARSE_LOOP: etiqueta en 0x8032
        case 0x8032:
            // Lee el char actual de PENDING via MAR/MBR.
            MAR = ESI;
            MBR = RAM[MAR];
            EAX = MBR;
            EIP++;
            imprimir_fila(eip_actual, "MOV EAX,[ESI]");
            break;

        case 0x8033:
            // Compara EAX con 0 (terminador). ZF=1 si fin de cadena.
            CMP(EAX, 0);
            imprimir_fila(eip_actual, "CMP EAX,0");
            break;

        case 0x8034:
            // Si terminador -> fin del PARSE_LOOP, salta a 0x803B.
            JE(0x803B);
            imprimir_fila(eip_actual, "JE 0x803B");
            break;

        case 0x8035:
            // EAX = EAX - '0': convierte char ASCII a digito entero.
            ADD(EAX, -(int)'0');
            imprimir_fila(eip_actual, "ADD EAX,-48");
            break;

        case 0x8036:
            // XCHG EAX, ECX: EAX <- acumulador, ECX <- digito actual.
            // Necesario para que MUL opere sobre EAX.
            XCHG(EAX, ECX);
            imprimir_fila(eip_actual, "XCHG EAX,ECX");
            break;

        case 0x8037:
            // MUL 10: EAX = EAX * 10. Un solo operando, resultado en EAX. x86 real.
            MUL(10);
            imprimir_fila(eip_actual, "MUL 10");
            break;

        case 0x8038:
            // ECX = ECX + EAX: digito + (acumulador*10) = nuevo acumulador en ECX.
            ADD(ECX, EAX);
            imprimir_fila(eip_actual, "ADD ECX,EAX");
            break;

        case 0x8039:
            // ESI += 1: avanza al siguiente char de PENDING.
            ADD(ESI, 1);
            imprimir_fila(eip_actual, "ADD ESI,1");
            break;

        case 0x803A:
            // Regresa al inicio del PARSE_LOOP.
            JMP(0x8032);
            imprimir_fila(eip_actual, "JMP 0x8032");
            break;

        case 0x803B:
            // Fin del PARSE_LOOP. ECX = indice entero.
            // EAX = ELEM_SIZE (base del calculo de offset).
            MOV(EAX, ELEM_SIZE);
            imprimir_fila(eip_actual, "MOV EAX,0x10");
            break;

        case 0x803C:
            // MUL ECX: EAX = EAX * ECX = offset en bytes. Un operando, resultado en EAX.
            MUL(ECX);
            imprimir_fila(eip_actual, "MUL ECX");
            break;

        case 0x803D:
            // EAX = LIST_BASE + offset = direccion exacta del elemento.
            ADD(EAX, LIST_BASE);
            imprimir_fila(eip_actual, "ADD EAX,0x0310");
            break;

        case 0x803E:
            // CALL INT_PRINT: muestra el elemento en RAM[EAX] via MAR/MBR.
            CALL(0xC000, 0x803F);
            imprimir_fila(eip_actual, "CALL 0xC000");
            break;

        case 0x803F:
            JMP(0x8000);
            imprimir_fila(eip_actual, "JMP 0x8000");
            break;

            // =================================================================
            // BLOQUE SHOW ALL --- 0x8040
            // Muestra todos los elementos de la lista.
            // ECX = EDX (copia del contador para no destruir EDX).
            // EAX = puntero de lectura, avanza ELEM_SIZE por iteracion.
            // =================================================================

        case 0x8040:
            // ECX = EDX: contador del bucle de muestra.
            MOV_REG(ECX, EDX);
            imprimir_fila(eip_actual, "MOV ECX,EDX");
            break;

        case 0x8041:
            // EAX = LIST_BASE: puntero de lectura al inicio de la lista.
            MOV(EAX, LIST_BASE);
            imprimir_fila(eip_actual, "MOV EAX,0x0310");
            break;

        // SHOW_LOOP: etiqueta en 0x8042
        case 0x8042:
            // Compara ECX con 0. Si lista vacia o fin -> sale del bucle.
            CMP(ECX, 0);
            imprimir_fila(eip_actual, "CMP ECX,0");
            break;

        case 0x8043:
            // Si ECX == 0 -> fin del SHOW_LOOP, regresa al bucle principal.
            JE(0x8048);
            imprimir_fila(eip_actual, "JE 0x8048");
            break;

        case 0x8044:
            // CALL INT_PRINT: muestra el elemento actual en RAM[EAX].
            CALL(0xC000, 0x8045);
            imprimir_fila(eip_actual, "CALL 0xC000");
            break;

        case 0x8045:
            // EAX += ELEM_SIZE: avanza al siguiente elemento.
            ADD(EAX, ELEM_SIZE);
            imprimir_fila(eip_actual, "ADD EAX,0x10");
            break;

        case 0x8046:
            // ECX--: un elemento menos por mostrar.
            DEC(ECX);
            imprimir_fila(eip_actual, "DEC ECX");
            break;

        case 0x8047:
            // Si ZF==0 (quedan elementos): regresa a SHOW_LOOP.
            JNZ(0x8042);
            imprimir_fila(eip_actual, "JNZ 0x8042");
            break;

        case 0x8048:
            JMP(0x8000);
            imprimir_fila(eip_actual, "JMP 0x8000");
            break;

            // =================================================================
            // BLOQUE EXIT --- 0x8060
            // =================================================================

        case 0x8060:
            // HLT: detiene el ciclo de instruccion.
            HLT_OP();
            imprimir_fila(eip_actual, "HLT");
            break;

            // =================================================================
            // INT_TECLADO --- 0xA000
            // Simula la ISR del teclado (INT 21h en DOS / syscall read en Linux).
            // Proceso:
            //   1. Lee la cadena del usuario.
            //   2. Por cada char: MAR = CHAR_MAP_BASE + ascii,
            //                     MBR = RAM[MAR],
            //                     RAM[BUFFER_BASE + i] = MBR.
            //   3. Escribe terminador 0x00 al final.
            // Resultado: cadena en RAM[BUFFER_BASE] terminada en 0x00.
            // Convencion x86: prologo PUSH EBP / MOV EBP,ESP
            //                 epilogo MOV ESP,EBP / POP EBP / RET
            // =================================================================

        case 0xA000:
            // Prologo: guarda EBP del llamador en el stack via MAR/MBR.
            PUSH(EBP);
            imprimir_fila(eip_actual, "PUSH EBP");
            break;

        case 0xA001:
            // Prologo: establece nuevo marco de pila.
            MOV_REG(EBP, ESP);
            imprimir_fila(eip_actual, "MOV EBP,ESP");
            break;

        case 0xA002:
        {
            // INTERRUPCION: el teclado entrega la cadena al CPU.
            // Cada char es buscado en CHAR_MAP via MAR/MBR y
            // depositado en BUFFER_BASE.
            printf("--------------------------------------------------------------------------\n");
            printf(">>> INT TECLADO >>> ");
            fflush(stdout);
            char input[ELEM_SIZE + 1];
            memset(input, 0, sizeof(input));
            cin.getline(input, sizeof(input));
            int len = (int)strlen(input);
            if (len >= ELEM_SIZE)
                len = ELEM_SIZE - 1;
            for (int i = 0; i < len; i++)
            {
                unsigned char c = (unsigned char)input[i];
                MAR = CHAR_MAP_BASE + c;
                MBR = RAM[MAR];
                MAR = BUFFER_BASE + i;
                RAM[MAR] = MBR;
            }
            MAR = CHAR_MAP_BASE;
            MBR = RAM[MAR];
            MAR = BUFFER_BASE + len;
            RAM[MAR] = MBR;
            EIP++;
            imprimir_encabezado();
            imprimir_fila(eip_actual, "INT LEER_TECLADO");
            break;
        }

        case 0xA003:
            // Epilogo: restaura ESP.
            MOV_REG(ESP, EBP);
            imprimir_fila(eip_actual, "MOV ESP,EBP");
            break;

        case 0xA004:
            // Epilogo: restaura EBP del llamador.
            EBP = POP_VAL();
            imprimir_fila(eip_actual, "POP EBP");
            break;

        case 0xA005:
            // Epilogo: saca dir. retorno del stack -> EIP.
            RET_OP();
            imprimir_fila(eip_actual, "RET");
            break;

            // =================================================================
            // INT_MEMCPY --- 0xB000
            // Copia ELEM_SIZE bytes de RAM[ESI] a RAM[EDI] via MAR/MBR.
            // Simula REP MOVSB (Move String Byte con repeticion) de x86.
            // Registros: ESI = fuente, EDI = destino.
            // ECX se usa internamente como contador del loop de copia.
            // Se detiene antes si encuentra terminador 0x00.
            // Prologo/Epilogo estandar x86.
            // =================================================================

        case 0xB000:
            PUSH(EBP);
            imprimir_fila(eip_actual, "PUSH EBP");
            break;

        case 0xB001:
            MOV_REG(EBP, ESP);
            imprimir_fila(eip_actual, "MOV EBP,ESP");
            break;

        case 0xB002:
            // ECX = ELEM_SIZE: contador de bytes a copiar.
            MOV(ECX, ELEM_SIZE);
            imprimir_fila(eip_actual, "MOV ECX,0x10");
            break;

        // MEMCPY_LOOP: etiqueta en 0xB003
        case 0xB003:
            // Lee byte fuente: MAR=ESI, MBR=RAM[MAR].
            MAR = ESI;
            MBR = RAM[MAR];
            EAX = MBR;
            EIP++;
            imprimir_fila(eip_actual, "MOV EAX,[ESI]");
            break;

        case 0xB004:
            // Escribe byte destino: MAR=EDI, RAM[MAR]=EAX via MBR.
            MAR = EDI;
            MBR = EAX;
            RAM[MAR] = MBR;
            EIP++;
            imprimir_fila(eip_actual, "MOV [EDI],EAX");
            break;

        case 0xB005:
            // Compara EAX con 0 (terminador). ZF=1 si fin de cadena.
            CMP(EAX, 0);
            imprimir_fila(eip_actual, "CMP EAX,0");
            break;

        case 0xB006:
            // Si terminador -> fin del MEMCPY_LOOP.
            JE(0xB00C);
            imprimir_fila(eip_actual, "JE 0xB00C");
            break;

        case 0xB007:
            // ESI += 1: avanza puntero fuente.
            ADD(ESI, 1);
            imprimir_fila(eip_actual, "ADD ESI,1");
            break;

        case 0xB008:
            // EDI += 1: avanza puntero destino.
            ADD(EDI, 1);
            imprimir_fila(eip_actual, "ADD EDI,1");
            break;

        case 0xB009:
            // DEC ECX: un byte menos por copiar.
            DEC(ECX);
            imprimir_fila(eip_actual, "DEC ECX");
            break;

        case 0xB00A:
            // Si ZF==0 (ECX no llego a 0): regresa a MEMCPY_LOOP.
            JNZ(0xB003);
            imprimir_fila(eip_actual, "JNZ 0xB003");
            break;

        case 0xB00B:
            // Garantiza terminador en la ultima posicion del slot destino.
            MAR = EDI;
            MBR = 0;
            RAM[MAR] = MBR;
            EIP++;
            imprimir_fila(eip_actual, "MOV [EDI],0");
            break;

        case 0xB00C:
            // Epilogo.
            MOV_REG(ESP, EBP);
            imprimir_fila(eip_actual, "MOV ESP,EBP");
            break;

        case 0xB00D:
            EBP = POP_VAL();
            imprimir_fila(eip_actual, "POP EBP");
            break;

        case 0xB00E:
            RET_OP();
            imprimir_fila(eip_actual, "RET");
            break;

            // =================================================================
            // INT_PRINT --- 0xC000
            // Muestra en consola la cadena en RAM[EAX] char por char via MAR/MBR.
            // Simula INT 21h funcion 09h (print string) de DOS,
            // o syscall write de Linux.
            // EAX = direccion base del elemento a imprimir.
            // Calcula el indice como: (EAX - LIST_BASE) / ELEM_SIZE.
            // Prologo/Epilogo estandar x86.
            // =================================================================

        case 0xC000:
            PUSH(EBP);
            imprimir_fila(eip_actual, "PUSH EBP");
            break;

        case 0xC001:
            MOV_REG(EBP, ESP);
            imprimir_fila(eip_actual, "MOV EBP,ESP");
            break;

        case 0xC002:
        {
            // Imprime el elemento en RAM[EAX] char a char via MAR/MBR.
            int idx = (EAX - LIST_BASE) / ELEM_SIZE;
            printf("  lista[%d] = \"", idx);
            for (int i = 0; i < ELEM_SIZE; i++)
            {
                MAR = EAX + i;
                MBR = RAM[MAR];
                if (MBR == 0)
                    break;
                printf("%c", (char)MBR);
            }
            printf("\"\n");
            EIP++;
            imprimir_fila(eip_actual, "INT PRINT_STR");
            break;
        }

        case 0xC003:
            MOV_REG(ESP, EBP);
            imprimir_fila(eip_actual, "MOV ESP,EBP");
            break;

        case 0xC004:
            EBP = POP_VAL();
            imprimir_fila(eip_actual, "POP EBP");
            break;

        case 0xC005:
            RET_OP();
            imprimir_fila(eip_actual, "RET");
            break;

        default:
            printf("ERROR: Violacion de segmento. EIP=0x%04X\n", EIP);
            HLT_flag = true;
            break;
        }

        ciclo++;
    }

    printf("--------------------------------------------------------------------------\n");
    printf("=== EJECUCION FINALIZADA ===\n");
    printf("Elementos en lista : %d\n", EDX);
    printf("Ciclos de reloj    : %d\n", ciclo - 1);
    system("pause");
    return 0;
}