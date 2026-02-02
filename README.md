# AgroClimIoT 🌱📡

AgroClimIoT es un sistema IoT para monitoreo agroclimático y de suelo, desarrollado sobre ESP32. Integra sensores ambientales y edáficos, ofrece visualización en tiempo real mediante un servidor web local y almacena los datos en Firebase Realtime Database.

---

## 🚜 Características principales

- Medición de temperatura y humedad ambiental
- Medición de presión atmosférica y altitud
- Medición de temperatura y humedad del suelo
- Dashboard web accesible desde el navegador
- Envío automático de datos a Firebase RTDB
- Alertas visuales mediante LEDs según umbrales
- Arquitectura lista para escalar a red de sensores Agro-IoT

---

## 🌡️ Variables monitoreadas

- Temperatura ambiente (°C)
- Humedad relativa (%)
- Presión atmosférica (hPa)
- Altitud estimada (m)
- Temperatura del suelo (°C)
- Humedad del suelo (%)

---

## 🧠 Hardware utilizado

- ESP32
- DHT11 (temperatura y humedad ambiental)
- BMP280 (presión, temperatura, altitud)
- DS18B20 (temperatura del suelo)
- Sensor de humedad de suelo HD-38
- LEDs para alertas

---

## 🔌 Mapa de pines

| Sensor / Elemento | Pin ESP32 |
|------------------|-----------|
| DHT11            | GPIO 26   |
| DS18B20          | GPIO 4    |
| HD-38 (analógico)| GPIO 33   |
| LED alerta temp suelo | GPIO 5 |
| LED alerta humedad suelo | GPIO 18 |

---

## 🌐 Dashboard web

El ESP32 levanta un servidor HTTP en el puerto **80**.  
Accede desde el navegador ingresando la IP asignada al dispositivo:

http://<IP_DEL_ESP32>/


Los datos se actualizan automáticamente cada 10 segundos.

---

## ☁️ Envío de datos a Firebase

Los datos se envían periódicamente a Firebase Realtime Database, permitiendo:

- Almacenamiento histórico
- Análisis posterior
- Integración con dashboards externos

---

## ⚠️ Configuración de credenciales

Por seguridad, **no subas credenciales al repositorio**.

Crea un archivo:

secrets.example.h

Incluye:
- WiFi SSID y contraseña
- Firebase API Key
- Firebase Database URL

---

## 🚦 Sistema de alertas

- LED 1: se activa si la temperatura del suelo ≥ 30 °C
- LED 2: se activa si la humedad del suelo ≤ 30 %

Los umbrales pueden ajustarse directamente en el código.

---

## 🛣️ Roadmap

- Soporte para múltiples nodos (red de sensores)
- Integración con LoRa / ESP-NOW
- Timestamp real para registros históricos
- Dashboard externo (Grafana / Power BI)
- Alertas remotas (correo / WhatsApp / Telegram)
- Calibración avanzada de sensores de suelo

---

## 📜 Licencia

Este proyecto se distribuye bajo la licencia MIT.

---

## 🤝 Contribuciones

Las contribuciones son bienvenidas.  
AgroClimIoT está pensado como una base abierta para investigación, educación y agricultura inteligente.

