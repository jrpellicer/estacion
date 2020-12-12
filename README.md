# Estación Meteorológica

Proyecto de estación meteorológica con NodeMCU, sensor DHT22 y Raspberry Pi

## Objetivo del proyecto

Creación de una estación meteorológica conectada a la red con la posibilidad de acceder a sus datos remotamente vía web. Para ello, se implementa un pequeño sistema IoT doméstico con sensores de temperatura y humedad conectados por wifi, que almacena las lecturas obtenidas en una base de datos ubicada en una Raspberry Pi y que a su vez también actúa de servidor web.

El paso de los datos desde el sensor hasta la Raspberry Pi se hace mediante el protocolo MQTT. Los mensajes MQTT recibidos vía WiFi en la Raspberry se procesan mediante Node-RED y se almacenan en una BBDD MySQL, estando disponibles para ser accedidos por el servidor web Apache también ubicado en la Raspberry Pi.

Todos los servicios aojados en la Raspberry Pi están contenerizados con Docker.

## Elementos necesarios

Por un lado está el dispositivo IoT que se encarga de medir y enviar vía WiFi los datos de temperatura y humedad; y por otro lado el servidor que se encarga de recibir esos datos, procesarlos, almacenarlos y servirlos vía web.

Para el dispositivo IoT se utiliza una placa de desarrollo NodeMCU basada en el microcontrlador ESP8266 con tarjeta WiFi. A esa placa de desarrollo se conecta un sensor de temperatura y humedad DHT22.

El servidor es una Raspberry Pi con Raspbian instalado y configurado.

Dispositivo cliente:
- Placa NodeMCU v3 con ESP8266
- NodeMCU Breadboard (opcional). Placa para facilitar la conexión entre el NodeMCU y los componentes.
- Sensor DHT22. Sensor de temperatura y humedad.
- Alimentador MicroUSB.
- 3 Cables puente para conectar el sensor DHT22 al NodeMCU

Servidor:
- Raspberry Pi 4 model B.
- Tarjeta SD.
- Alimentador MicroUSB.

## Montaje de los componentes de la estación

El NodeMCU trabaja con voltajes de 3,3v (a diferencia de Arduino que trabaja a 5v). Según el datasheet del DHT22, este sensor trabaja entre voltajes de 3,3V y 5,5V. El sensor tiene 3 patillas: una de alimentación, otra de tierra (GND) y la de datos. No es necesaria ninguna resistencia intermedia y se puede conectar directamente a la placa.

El pin de alimentación (+) del sensor se conecta a uno de salida de 3,3V del NodeMCU, el de GND (-) a uno de GND, y por último, el pin central de datos se conecta a un pin GPIO de la placa. En este caso se ha conectado al **pin D2**. Es importante elegir bien el pin para la programación posterior de la placa.

Aunque la placa trabaja a 3,3V, para alimentarla se puede utilizar un cargador microUSB de 5V, como los de cualquier móvil. La propia placa tiene un regulador de tensión en elpuerto USB que la baja a 3,3V.

## Programación de la placa

Para la programción del NodeMCU hay que configurar el IDE de Arduino indicando que la tarjeta a utilizar es la `NodeMCU 1.0 (ESP-12E Module)`.

Para que aparezca esta tarjeta se debe instalar en el *Gestor de Tarjetas* la placa ESP8266.

Si no apareciera este tipo de tarjeta, habría que modificar las *Preferencias* del IDE de Arduino y en el campo de *Gestor de URLs Adicionales de Tarjetas* introducir la siguiente URL: <https://arduino.esp8266.com/stable/package_esp8266com_index.json>

Se compila el programa *estacion.ino* y se pasa a la placa.

**Importante:**. En el programa *estacion.ino* hay que cambiar la dirección IP del Broker MQTT.

## Preparación de la Raspberry Pi

Es necesaria una Raspberry Pi con Raspbian configurado con una IP estática y Docker instalado.

### Instalación del Broker MQTT

El Broker a instalar en la Raspberry Pi es el Mosquitto. Existe en el Hub de Docker una versión válida para Raspberry:

    docker run --name mosquitto -d -p 1883:1883 -p 9001:9001 eclipse-mosquitto

El Topic a utilizar para enviar los datos desde el NodeMCU será:

    casa/estacion/sensor1

Pasando en el mismo mensaje la temperatura y la humedad registrada:

    casa/estacion/sensor1/"22.1,87.3"

### Instalación de MySQL

Para poder almacenar todas las lecturas del sensor en una base de datos,hay que instalar el gestor de base de datos MySQL. Se instala también en un contenedor de Docker.

Se crea el directorio necesario para que se almacenen los datos en la Raspberry Pi:

        mkdir ~/estacion/mysql/data -p

Se creamos y ejecuta el contenedor especificando que se monte un volumen para almacenar los datos en el directorio antes creado:

        docker run --name mysql -d -p 3306:3306 -e MYSQL_ROOT_PASSWORD=P@ssw0rd -v ~/estacion/mysql/data:/var/lib/mysql hypriot/rpi-mysql

Una vez está en ejecución, vamos hay que abrir un terminal para crear la base de datos y la tabla:

        docker exec -it mysql mysql -p

Se teclean los siguientes mandatos dentro del contenedor:

        create database meteo;

        use meteo;

        CREATE TABLE IF NOT EXISTS `mediciones` (\
        `fecha` TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'Fecha y hora de la lectura',\
        `sensor` varchar(50) NOT NULL COMMENT 'Nombre del sensor',\
        `temperatura` float COMMENT 'Temperatura',\
        `humedad` float COMMENT 'Humedad',\
        PRIMARY KEY (`fecha`,`sensor`),\
        KEY `sensor` (`sensor`)\
        ) COMMENT='Mediciones de temperatura';

        exit


### Instalación de NodeRED

NodeRED se instala también en un contenedor.

Como paso previo a la instalación hay que crear un directorio de trabajo para el almacenamiento persistente de los datos del contenedor:

        mkdir ~/estacion/node-red p

Se ejecuta el contenedor:

        docker run -d -p 1880:1880 --name nodered -v ~/estacion/node-red:/data nodered/node-red

Para acceder al entorno de trabajo, hay que abrir un navegador y acceder a la dirección de la Raspberry pi por el puerto 1880

Una vez dentro del entorno de NodeRed hay que instalar una extensión para poder incorporar un nodo que permita leer y escribir en una base de datos MySQL. Para ello se accede al menú y en la opción *Settings*, en la pestaña *Palette* hay que buscar en el recuadro la palabra *mysql* e instalar la librería.

Una vez instalada hay que importar el flow *estacion.json*.

**Importante:**
- Se debe entrar en el nodo de Mysql y cambiar la configuración con las credenciales de acceso y la dirección IP del servidor MySQL.
- Hay que configurar también el nodo MQTT para cambiar la dirección del Broker MQTT. En ambos casos hay que poner la IP de la Raspberry Pi

Finalmente, hay que pulsar sobre *Deploy*.

### Instalación de Apache y PHP

Se instala y ejecuta el servidor web Apachetambién en un contenedor de Docker.

Se crea el directorio necesario para almacenar la página web en una carpeta local de la Raspberry Pi.

        mkdir ~/estacion/htdocs

Se copia el fichero *index.php* dentro de ~/estacion/htdocs


**Importante:**. Con un editor de texto se debe editar el dichero *index.php* y cambiar la configuración con las credenciales de acceso y la dirección IP del servidor MySQL.

Se instala y ejecuta el contenedor:

        docker run --name apache -d -p 80:80 -v ~/estacion/htdocs:/var/www/html php:apache

Es necesario instalar la extensión de PHP para acceder a base de datos MySQL. Para ello hay que ejecutar 3 comandos dentro del contenedor. Se puede hacer desde fuera del contenedor con la función exec de Docker:

        docker exec -it apache docker-php-ext-install mysqli
        docker exec -it apache docker-php-ext-enable mysqli
        docker exec -it apache apachectl restart

