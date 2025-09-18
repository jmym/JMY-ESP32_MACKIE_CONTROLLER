Controlador MIDI Mackie para ESP32-S3
📋 Descripción del Proyecto
Este proyecto implementa un controlador MIDI tipo Mackie Universal Control (MCU) basado en el ESP32-S3-DevKitC-1-N16R8. El sistema incluye una interfaz gráfica con pantalla TFT, control de encoders, gestión MIDI USB y almacenamiento en tarjeta SD.

🛠 Hardware Utilizado
Tarjeta de Desarrollo
ESP32-S3-DevKitC-1-N16R8

Microcontrolador ESP32-S3 dual-core

16MB flash, 8MB PSRAM

USB-C para programación y comunicación MIDI

WiFi y Bluetooth LE 5.0

Pantalla TFT
ST7796S - Pantalla de 4.0" 480x320 píxeles

Conexiones:

TFT_CS → GPIO10

TFT_DC → GPIO7

TFT_RST → GPIO6

TFT_BL → GPIO5 (control de retroiluminación)

TFT_SCLK → GPIO12 (SPI SCK)

TFT_MOSI → GPIO11 (SPI MOSI)

TFT_MISO → GPIO13 (SPI MISO)

Tarjeta SD
Interface SPI

Conexiones:

SD_CS → GPIO4

SD_SCLK → GPIO12 (compartido con TFT)

SD_MOSI → GPIO11 (compartido con TFT)

SD_MISO → GPIO13 (compartido con TFT)

Expansores MCP23017
Cuatro chips MCP23017 para manejar 32 entradas/salidas:

MCP1 - Encoders de Volumen (Dirección 0x20)
Interruptores: GPIO1 (INT_MCP1_A) y GPIO2 (INT_MCP1_B)

Controla 16 encoders (8 bancos × 2 encoders por canal)

MCP2 - Encoders de Pan (Dirección 0x21)
Interruptores: GPIO3 (INT_MCP2_A) y GPIO8 (INT_MCP2_B)

Controla 16 encoders adicionales

MCP3 - Switches (Dirección 0x22)
16 switches para mute/solo de canales

MCP4 - Botones y Encoder de Navegación (Dirección 0x23)
Botones: Play, Stop, Rec, Bank Up, Bank Down

Encoder de navegación:

ENC_NAV_A_PIN → Pin 8

ENC_NAV_B_PIN → Pin 9

ENC_NAV_SW_PIN → Pin 10

📋 Especificaciones Técnicas
Características Principales
16 encoders rotativos con push-button

16 switches táctiles

5 botones de transporte y navegación

Pantalla TFT de 4.0" con interfaz gráfica

Almacenamiento de presets en SD

Comunicación MIDI USB

Soporte para MTC (MIDI Time Code)

VU meters en tiempo real

Configuración MIDI
4 bancos de 8 canales cada uno

Soporte para Control Change, Note On/Off y Pitch Bend

Sincronización bidireccional con DAW

Paleta de colores Studio One 7

🎛 Funcionalidades
Control de Audio
Control de volumen y pan para 8 canales simultáneos

Mute y solo por canal

VU meters visuales

Nombres de pista desde DAW

Transporte
Control de reproducción (Play, Stop, Record)

Navegación por tiempo (Jog wheel)

Visualización de tiempo MTC

Interfaz de Usuario
Menú configurable con encoder de navegación

Salvapantallas con información de estado

Configuración visual de encoders

Testeo de hardware integrado

📁 Estructura del Proyecto
text
ESP32_MACKIE_CONTROLLER/
├── Config.h              # Configuración global y estructuras
├── DisplayManager.h/cpp  # Gestión de pantalla TFT
├── EncoderManager.h/cpp  # Gestión de encoders
├── HardwareManager.h/cpp # Control de MCP23017
├── MidiManager.h/cpp     # Comunicación MIDI USB
├── MenuManager.h/cpp     # Sistema de menús
├── FileManager.h/cpp     # Gestión de SD card
├── SystemManager.h/cpp   # Gestión del sistema
├── Strings.h            # Cadenas de texto
└── ESP32_MACKIE_CONTROLLER.ino # Sketch principal
⚙️ Configuración
Pines Críticos
cpp
// Evitar GPIOs de strapping:
// GPIO0, GPIO2, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12
Instalación
Clonar el repositorio

Abrir con Arduino IDE o PlatformIO

Instalar dependencias:

Adafruit ST7796S Library

Adafruit MCP23017 Library

Adafruit GFX Library

Compilar y subir a ESP32-S3

🎨 Características de Software
Optimizaciones
Actualización parcial de pantalla

Debouncing hardware/software

Aceleración de encoders

Gestión de energía eficiente

Buffer MIDI optimizado

Comunicación
MIDI USB nativo con ESP32-S3

SysEx para comunicación con Studio One

MTC para sincronización temporal

Feedback visual inmediato

🔧 Mantenimiento
Diagnóstico
Test integrado de hardware

Monitorización de memoria

Logs en tarjeta SD

Estadísticas MIDI

Actualización
Sistema de presets versionado

Backup automático de configuración

Recuperación de fallos

📸 Vista de Hardware
text
+------------------------------------------+
| ESP32-S3 DevKitC-1                       |
|   [USB-C] [TFT] [SD] [MCP1] [MCP2]       |
|   [MCP3] [MCP4]                          |
+------------------------------------------+
📞 Soporte
Para issues y contribuciones, consultar el repositorio GitHub del proyecto.

📄 Licencia
Este proyecto está bajo licencia MIT. Ver archivo LICENSE para detalles.

Nota: Este controlador es compatible con la mayoría de DAWs que soportan protocolo Mackie Control Universal, con optimizaciones específicas para Studio One 7.