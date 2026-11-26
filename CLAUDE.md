# TPO Informática II — SCAI (Sistema de Control de Acceso Inteligente)

> Documento de contexto para desarrollar el firmware. Leerlo completo antes de escribir código.
> Lo que figura como **PENDIENTE** todavía no lo decidió el grupo: no lo inventes, preguntá.

---

## 1. Contexto

- Trabajo Práctico Obligatorio de **Informática II** (UTN, cursada 2026).
- Equipo de estudiantes. El código lo van a **defender en un coloquio**, así que tiene que ser claro, comentado y entendible. No sirve el código "mágico".
- Placa: **LPC845-BRK** (NXP LPC845, ARM Cortex-M0+, 30 MHz, 64 KB de flash, 16 KB de RAM, alimentada a 3,3 V), más la placa de expansión **Infotrónic** de la cátedra (aporta el LCD, el buzzer y el RS485).
- Entorno: **MCUXpresso IDE**, lenguaje **C++** (gnu++11) sobre la librería de la cátedra, bare-metal, sin RTOS. Ver 1.1.
- Proyecto: `R2053-2026/` (proyecto MCUXpresso). Fuentes en `R2053-2026/src/`.

### 1.1 Decisión: base C++ de la cátedra (26/11/2026)

El grupo decidió construir sobre la librería C++ de la cátedra (Ing. Trujillo) que ya está en `src/` y **no** reescribir en C99. Esto ajusta las reglas de la sección 4:

- **Capas:** las carpetas numeradas de la cátedra (`02-gpio`, `06-timer`, `08-teclado`, `10-lcd`, `12-USART`, `firmware/`...) son la capa de drivers y primitivas. Lo nuevo del SCAI va en `src/aplicacion/` (máquinas de estado, lógica de acceso, protocolo con la PC) y en carpetas nuevas con el mismo estilo de nombres cuando sea una primitiva nueva (PN532, servo, reloj).
- **Inicialización de la placa:** `aplicacion/inicializarInfotronic.cpp` cumple el rol de `Board_Init()`. Los pines están todos en `aplicacion/pinout.h` (cumple el rol de `board/pinout.h`).
- **Drivers nuevos del SCAI:** `13-i2c`, `14-rtc` (DS3231), `15-servo` (SCT), `16-reed` (PINT), `17-buzzer`. Cada carpeta nueva hay que agregarla a las rutas de include del `.cproject`.
- **Compilar fuera del IDE:** toolchain en `C:\MCUXpressoIDE_25.6.136\ide\plugins\com.nxp.mcuxpresso.tools.win32_*\tools\bin`, script de link `R2053-2026/Debug/Completo_01_Debug.ld`.
- **Timers:** se usa la clase `timer` de la cátedra (resolución mínima de 100 ms con la base `timer::DEC`) y la colección `timers`, cuyo `TmrEvent()` se llama en el `main` loop para que los handlers corran fuera de la ISR.
- **Memoria dinámica:** la librería usa `new`, `std::vector` y `std::list`. Se acepta **solo durante la inicialización**; nunca se reserva memoria dentro del `while(1)` ni en una ISR.
- **Objetos que se enganchan al SysTick** (`timer`, `Led` y otras clases derivadas de `Callback`): se agregan a `g_CallbackList`. No pueden ser variables globales de otro archivo, porque C++ no garantiza el orden de inicialización entre archivos. Se crean dentro de `main()` antes de `InicializarInfotronic()`, o en `inicializarInfotronic.cpp`.
- **Nombres:** se respeta el estilo de la librería (`TimerStart`, `SetPin`...). Los nombres nuevos siguen en español.
- **Flash:** el proyecto base ya ocupa unos 44 KB de los 64 KB. Hay que vigilar el tamaño en cada etapa.

## 2. Qué es el sistema

Es un control de acceso para un edificio de oficinas. Un **nodo de puerta** lee una credencial NFC, la valida según su nivel de acceso, su sector y la hora, y abre o rechaza el acceso. Cada evento queda registrado con fecha y hora y se envía a una PC, donde corre una app en Qt.

### 2.1 Niveles de acceso

| Nivel | Lo habilita | Acceso | Particularidad |
|---|---|---|---|
| `VISITANTE` | Recepción | Edificio y áreas comunes | Un solo uso: se deshabilita al salir |
| `GENERAL_UN_USO` | Recepción | Edificio, su sector y baño | Requiere tener un `GENERAL` previo; se deshabilita al salir |
| `GENERAL` | RRHH | Edificio, su sector y baño | Empleado estándar |
| `ALTO` | RRHH | Lo mismo que `GENERAL` más el **área segura** | Gerencia, IT, jefaturas |

### 2.2 Sector (atributo independiente del nivel)

- `OFICINA` o `DEPOSITO`. Define qué área se habilita por defecto.
- Caso particular: se pueden habilitar **los dos sectores** en una misma credencial (por ejemplo, un supervisor). Implementarlo como flags de bits, no como un enum excluyente.

### 2.3 Flags y alarmas

- Llegada tarde: ingreso después del horario configurado (por ejemplo, 09:15).
- Intento fuera de la franja horaria permitida.
- Credencial deshabilitada o vencida.
- **3 intentos fallidos consecutivos**: se bloquea el lector durante N segundos y se genera un evento.
- **Puerta forzada**: el reed detecta la puerta abierta sin un acceso válido previo.
- **Puerta abierta demasiado tiempo**: el reed sigue abierto después de T segundos.
- Acceso cruzado entre sectores no autorizado.
- Antipassback: no se permiten dos entradas seguidas sin una salida en el medio.

---

## 3. Alcance

### 3.1 Núcleo mínimo (es lo que hay que hacer primero)

| Componente | Función | Periférico del LPC845 |
|---|---|---|
| Lector NFC **PN532** | Leer el UID de la tarjeta o el celular | **USART en modo HSU, 115200 baudios** |
| Teclado matricial 4x4 | PIN para entrar al modo administrador | GPIO (barrido con SysTick) |
| LCD 16x2 HD44780 (de la Infotrónic) | Mensajes al usuario | GPIO, modo 4 bits, RW a GND |
| Servo **SG90** | Abrir y cerrar la puerta a escala | **SCTimer como PWM**, 50 Hz, pulso de 1–2 ms |
| Reed switch **MC-38** | Saber si la puerta está abierta o cerrada | GPIO + **PINT** (interrupción por flanco) |
| LEDs verde y rojo | Acceso aceptado o rechazado | GPIO |
| Buzzer (de la Infotrónic) | Tonos distintos para aceptado y rechazado | GPIO (o SCTimer) |
| Enlace con la PC | Eventos y comandos | **USART0** (puente USB-VCOM de la BRK) |
| Reloj | Fecha y hora de cada evento | **RTC DS3231 por I2C1** (decidido el 26/11/2026), puesto en hora desde la PC (`$TIME`) |
| WiFi | Enlace con la PC (versión final) | **ESP8266 / ESP-01 por USART2**, comandos AT (decidido el 26/11/2026) |

### 3.2 Extras (solo cuando el núcleo esté funcionando)

- Huella dactilar **AS608** (UART), como segundo factor para el área segura.
- (El DS3231 y el ESP8266 pasaron al núcleo: ver 3.1.)
- WiFi con **ESP8266 / ESP-01** (UART, comandos AT), o **RS485** (MAX485, UART) hacia la Infotrónic.
- MicroSD (SPI) para guardar el log de eventos.
- PIR (GPIO), MQ-2 (ADC, con divisor a 3,3 V), LDR (ADC), SW-420 (GPIO/PINT), DHT22, botones de emergencia (PINT).
- Más máquinas de estado: esclusa del cuarto seguro, molinete, edificio en general.

### 3.3 Periféricos

- **Vistos en la cursada:** GPIO, SysTick, ADC, DAC, UART, interrupción externa (en el LPC845 se llama PINT), RTC.
- **No vistos (cuentan como desafío):** PWM/SCTimer, I2C, SPI, DMA.
- El LPC845 **no tiene** RTC, USB, CAN, Ethernet ni I2S.
- **Prioridad:** cuando haya opción, usar un periférico visto en la cursada (por ejemplo, el PN532 por UART y no por I2C).

### 3.4 Asignación de USARTs (el LPC845 tiene 5)

| USART | Uso |
|---|---|
| USART0 | PC (VCOM USB de la BRK) |
| USART1 | PN532 (HSU) |
| USART2 | ESP8266 o RS485 (extra) |
| USART3 | AS608 (extra) |
| USART4 | Libre |

### 3.5 Pines

**PROVISORIO:** hay una propuesta en `R2053-2026/src/aplicacion/pinout.h` (cada pin dudoso dice `[VERIFICAR]`). Falta confirmarla con los esquemáticos de la LPC845-BRK y de la Infotrónic. El pin del buzzer es un marcador (PIO0_0) hasta ver el esquemático.

- Las funciones movibles (USART, SCT) se asignan con la **Switch Matrix (SWM)**.
- No usar los pines de SWD (PIO0_2 y PIO0_3) ni el de RESET (PIO0_5).
- Centralizar toda la asignación en un único archivo: `board/pinout.h`.

---

## 4. Reglas de código

1. **Nada bloqueante.** No se usan `delay()` ni loops de espera activa en la aplicación. Todo funciona con **máquinas de estado** y **timers por software** que avanzan con el tick de 1 ms.
2. **Las ISR se mantienen mínimas:** solo levantan un flag o cargan un buffer. La lógica va en el `main` loop.
3. Las variables que se comparten con una ISR se declaran `volatile`.
4. **Arquitectura en capas**: la aplicación nunca toca registros directamente.
   ```
   app/        -> máquinas de estado, lógica de acceso, protocolo con la PC
   primitivas/ -> teclado, lcd, timers por software, buffers UART, pn532, servo, reloj
   drivers/    -> gpio, systick, usart, pint, sct, swm (acceso a registros)
   board/      -> pinout.h, inicialización de la placa
   ```
5. **Una máquina de estados por archivo**, con un `switch(estado)` y un `enum` de estados. Cada una tiene una función `MdE_xxx(void)` que se llama en cada vuelta del `main`.
6. Nombres en español, consistentes: `Timer_Start(id, ms, callback)`, `LCD_Print(fila, col, txt)`, etc.
7. **Comentar en español**, explicando el porqué de las cosas, no solo el qué.
8. Sin memoria dinámica (`malloc`). Todos los buffers son estáticos y de tamaño fijo.
9. Cada módulo tiene su `.h` (interfaz pública) y su `.c`. Lo que es interno se declara `static`.

### Estructura del `main`

```c
int main(void) {
    Board_Init();      // reloj, SWM, GPIO, SysTick 1 ms, USARTs, PINT, SCT
    App_Init();
    while (1) {
        Timers_Run();      // descuenta y dispara los timers por software
        Teclado_Run();     // barrido + antirrebote
        PN532_Run();       // MdE de comunicación con el lector
        PC_Run();          // parser de las tramas que llegan de la PC
        MdE_Puerta();      // lógica principal
    }
}
```

---

## 5. Máquina de estados principal: puerta

| Estado | Qué hace | Transiciones |
|---|---|---|
| `REPOSO` | LCD: "Acerque tarjeta". Servo cerrado. | UID leído → `VALIDANDO` · tecla `*` → `PIN_ADMIN` · reed abierto → `ALARMA_FORZADA` |
| `VALIDANDO` | Busca el UID en la tabla y chequea nivel, sector, horario, antipassback y si está habilitado | válido → `ACCESO_OK` · inválido → `DENEGADO` |
| `ACCESO_OK` | LED verde, tono OK, abre el servo, registra el evento, arranca el timer de apertura (por ejemplo 5 s) | reed abierto → `PUERTA_ABIERTA` · timeout sin abrir → `CERRANDO` |
| `PUERTA_ABIERTA` | Espera a que se cierre la puerta | reed cerrado → `CERRANDO` · timeout de T s → evento "abierta prolongada" (sigue en este estado con aviso sonoro) |
| `CERRANDO` | Cierra el servo. Si la credencial es de un solo uso y era una salida, la deshabilita | → `REPOSO` |
| `DENEGADO` | LED rojo, tono de error, LCD "Acceso denegado", registra el intento, `fallidos++` | fallidos ≥ 3 → `BLOQUEADO` · tras 2 s → `REPOSO` |
| `BLOQUEADO` | Ignora el lector durante N s. Manda el evento a la PC | timeout → `REPOSO` (fallidos = 0) |
| `ALARMA_FORZADA` | Buzzer continuo y evento a la PC | PIN admin correcto o comando `$RST` de la PC → `REPOSO` |
| `PIN_ADMIN` | Pide el PIN de 4 dígitos por teclado | correcto → `MODO_ADMIN` · incorrecto o timeout → `REPOSO` |
| `MODO_ADMIN` | Alta rápida de una tarjeta: se acerca la tarjeta y se elige nivel y sector con el teclado | `#` confirma o timeout → `REPOSO` |

---

## 6. Modelo de datos

```c
#define MAX_USUARIOS 32
#define UID_MAX_LEN  7          // los UID de NFC tienen 4 o 7 bytes

typedef enum { VISITANTE, GENERAL_UN_USO, GENERAL, ALTO } nivel_t;

#define SECTOR_OFICINA  (1u << 0)
#define SECTOR_DEPOSITO (1u << 1)

typedef struct {
    uint8_t  uid[UID_MAX_LEN];
    uint8_t  uid_len;
    char     nombre[16];
    nivel_t  nivel;
    uint8_t  sectores;      // flags SECTOR_*
    uint8_t  habilitado;    // 0/1
    uint8_t  adentro;       // para el antipassback: 1 si está dentro
} usuario_t;
```

- La tabla se guarda en RAM. Al arrancar, la PC la puede recargar con `$ADD`.
- Persistencia en flash mediante IAP: **PENDIENTE / extra**.

## 7. Protocolo con la PC (USART0, 115200 8N1, ASCII)

Formato de las tramas: `$CMD,campo1,campo2,...\n`. Cada comando que manda la PC recibe una respuesta `$ACK,CMD` o `$NAK,CMD,motivo`.

**De la PC al LPC845**

| Trama | Acción |
|---|---|
| `$TIME,dd/mm/aaaa,hh:mm:ss` | Pone en hora el reloj por software |
| `$ADD,UIDHEX,nombre,nivel,sectores` | Da de alta o modifica un usuario |
| `$DEL,UIDHEX` | Da de baja un usuario |
| `$LIST` | Lista los usuarios (`$USR,...` por cada uno, y `$END` al final) |
| `$OPEN` | Apertura remota |
| `$RST` | Resetea la alarma o el bloqueo |
| `$CFG,hora_tarde,hh:mm` | Configura la hora a partir de la cual se considera llegada tarde |

**Del LPC845 a la PC**

| Trama | Cuándo |
|---|---|
| `$EVT,dd/mm/aaaa hh:mm:ss,UIDHEX,nombre,ENTRADA\|SALIDA,OK\|DENEGADO,motivo` | En cada intento de acceso |
| `$ALM,dd/mm/aaaa hh:mm:ss,FORZADA\|ABIERTA\|BLOQUEO\|TARDE` | En cada alarma o flag |

## 8. PN532 por HSU, en resumen

- Hay que configurar los pines de selección del módulo en modo **HSU (UART)**, a 115200 baudios.
- Para despertarlo, se manda primero un preámbulo largo: `0x55 0x55 0x00 0x00 0x00 ...`.
- Formato de la trama: `00 00 FF LEN LCS D4 CMD [datos] DCS 00`. El PN532 contesta con un ACK `00 00 FF 00 FF 00` y después con la respuesta (`D5 CMD+1 ...`).
- Secuencia: `SAMConfiguration (0x14)` → `InListPassiveTarget (0x4A, 1 target, 106 kbps tipo A)` → del resultado se extrae el UID.
- Implementarlo como una **máquina de estados no bloqueante**: enviar → esperar ACK → esperar respuesta → timeout.

## 9. Plan de trabajo (en orden, probando cada etapa antes de seguir)

1. Proyecto base y blinky con SysTick a 1 ms, más los timers por software.
2. GPIO, teclado 4x4 con antirrebote y LCD 16x2.
3. USART0 con la PC: buffers circulares por interrupción, eco y parser de `$...`.
4. Reloj por software y el comando `$TIME`.
5. PN532: leer el UID y mostrarlo en el LCD y por la PC.
6. Reed switch con PINT.
7. Servo con SCTimer en modo PWM.
8. Máquina de estados de la puerta con la tabla de usuarios y las validaciones.
9. Flags, alarmas y modo administrador.
10. Extras, según el tiempo que quede.

## 10. Decisiones PENDIENTES del grupo

- [ ] Pinout definitivo (esquemáticos de la BRK y de la Infotrónic). Hay propuesta provisoria en `pinout.h`.
- [ ] Máquina de estados de la puerta (sección 5 es la propuesta original; el grupo todavía no la definió).
- [x] DS3231 (26/11/2026).
- [x] WiFi con ESP8266 (26/11/2026). La USART0 por USB queda para depurar.
- [x] PN532 por UART en modo HSU (26/11/2026).
- [ ] Si la huella (AS608) entra en el núcleo o queda como extra.
- [ ] Persistencia de la tabla de usuarios (IAP en flash o recarga desde la PC).
- [ ] Una sola puerta o varias (entrada/salida con dos lectores, cuarto seguro).
- [ ] Requisitos del template de la cátedra (`template_TPO_2026.docx`).
