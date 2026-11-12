PROYECTO: CONTROL ESP32 CON FLASK
================================

ESTRUCTURA
- app.py: servidor Flask que envía instrucciones HTTP a la ESP32.
- templates/index.html: página web con botones.
- static/css/estilos.css: estilos de la página.
- esp32/: proyecto PlatformIO para la placa ESP32.
- requirements.txt: dependencias de Python.

REQUISITOS
- Python instalado.
- VS Code.
- Extensión PlatformIO IDE para compilar y cargar el código de esp32/.
- ESP32 y computador conectados a la misma red Wi-Fi de 2.4 GHz.

PASOS

1. CONFIGURAR LA ESP32
   - Abre la carpeta esp32/ en VS Code con PlatformIO.
   - Abre esp32/src/main.cpp.
   - Cambia NOMBRE_WIFI y CONTRASENA_WIFI por los datos de tu red.
   - Carga el programa a la placa.
   - Abre el monitor serial a 115200 baudios.
   - Copia la dirección IP que imprime la ESP32.

2. CONFIGURAR FLASK
   - Abre la carpeta principal Control_ESP32 en VS Code.
   - En app.py, cambia ESP32_URL por la IP que mostró el monitor serial.
     Ejemplo: ESP32_URL = "http://192.168.1.105"

3. INSTALAR DEPENDENCIAS (PowerShell, desde la carpeta principal)
   python -m venv venv
   .\venv\Scripts\Activate.ps1
   pip install -r requirements.txt

4. INICIAR FLASK
   python app.py

5. ABRIR LA PÁGINA
   En el mismo computador abre:
   http://127.0.0.1:5000

   Pulsa Encender LED o Apagar LED.

SOLUCIÓN DE PROBLEMAS
- Si Flask no conecta, verifica que la IP en app.py sea la misma que muestra
  el monitor serial de la ESP32.
- Ambos equipos deben estar en la misma red local. Algunas redes de invitados
  aíslan los dispositivos entre sí.
- Permite Python/Flask en el firewall de Windows si aparece una solicitud.
- El LED integrado no está en GPIO 2 en todas las placas. Si no enciende,
  revisa el pin de LED de tu modelo o conecta un LED externo con resistencia.
- Esta configuración es para una red local; no expongas el servidor HTTP de
  la ESP32 directamente a Internet.
