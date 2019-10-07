# Proyecto Open-Boiler
Este proyecto apunta a reemplazar la placa de control electrónica de una caldera combinada hogareña Roca Victoria 20/20 por una alternativa de hardware y software abierto. Es posible que a futuro se también se extienda su aplicación a otros modelos y fabricantes.

## Historia detrás del proyecto
Compré esta caldera “Roca Victoria 20/20 T” en el año 2002 y funciono a la perfección durante aproximadamente 5 años, luego aparecieron los primeros problemas con los sensores de control. Primero fue la sonda de ionización, que detecta cuando el quemador está encendido. Con el tiempo empezó a fallar también el caudalímetro de gases de escape. Es interesante mencionar que la falla del primer sensor deja la caldera en error esperando un reset, por lo que si esto te pasa en medio de una noche fría de invierno cuando dormís, por la mañana te levantás en el expreso trans-Siberiano.

El otro sensor, que verifica si el extractor de humos de combustión genera presión en el tubo de salida, tiene la mala costumbre de fallar justo cuando estas tomando un agradable baño tibio, con lo cual el agua fría de golpe al apagarse la caldera con error te hace cambiar la nota que estabas cantando en la ducha por un grito desesperado digno de una película de terror clase B. Es así que seguí reemplazando los sensores, que no son baratos, regularmente cada 2 o tres años.

Buscando en diversos foros y sitios de internet, llegué en principio a la conclusión de que este modelo de caldera era realmente una pesadilla y que debía desembolsar una importante suma de dinero para para reemplazarla y lograr tranquilidad. Pero luego tuve la certeza de que nada garantiza que una caldera nueva no empiece con los mismos problemas en unos 5 años ya que, averiguando un poco, los sensores de estos equipos parecen ser un estándar que, detalle más, detalle menos, todos los fabricantes instalan. Así que seguí “emparchando” los problemas de sensores a medida que surgían hasta que, un día de julio de 2019 la placa de control dijo “basta” y debía ser reemplazada, o bien comprar una caldera nueva. Solo la placa costaba la nada despreciable suma de 330 dólares donde yo vivo, una caldera nueva unos 1500 dólares aprox.

Antes de tirarla, decidí desarmar una vez mas a ver que podía hacer. La verdad es que más allá de la antipatía que me generaba una caldera que me había dejado sin ACS ni calefacción, pude apreciar en detalle que la parte mecánica, de hecho, estaba muy bien construida. Materiales nobles, acero inoxidable, cobre, buenas terminaciones. Ni una gota de agua perdiendo por ningún lado y ni siquiera una mancha de óxido en 16 años. Es así que, harto de la electrónica y los sensores de esta caldera, me decidí a intentar reemplazar la placa por una hecha por mí, y así poder tomar el control de que componentes instalar y, sobre todo, del firmware, para poder decidir cuándo y de qué manera la caldera debe detenerse con un error irrecuperable, o bien informar y seguir adelante cuando no es algo tan grave. La idea es poder asegurar que cuando se detecta un problema, es realmente un problema (falta de gas, llama apagada por el viento, etc.) y no una falla del sensor que lo detecta.

## Haciéndose amigo de la caldera
Lo primero a considerar, es comenzar a ver la caldera, ya no como una caja negra (o más bien blanca en este caso), sino como un conjunto de entradas y salidas que a las que hay que aplicarles lógica para que funcionen orquestadamente. Este modelo en particular está compuesto de la siguiente manera:

## Entradas:
1. Sensor de temperatura de agua caliente sanitaria “ACS”: Termistor NTC Honeywell T7335D (10 KΩ a 25°C).
2. Sensor de temperatura de calefacción central “CC”: Termistor NTC Honeywell T7335D (10 KΩ a 25°C).
3. Ajuste de temperatura de ACS: Potenciómetro PCB Piher de 10 kΩ.
4. Ajuste de temperatura de CC: Potenciómetro PCB Piher de 10 kΩ.
5. Ajuste de modo del sistema: Potenciómetro PCB Piher de 10 kΩ.
6. Sensor de llama: Sonda de ionización -> se reemplaza por sensor de luz infrarroja KY-026.
7. Sensor de flujo de aire de escape: Presostato 12-15 mm ca -> inicialmente se conserva con la posibilidad de ser anulado por software. En un futuro se reemplazará por un sensor barométrico Bosch BMP280.
8. Sensor de sobrecalentamiento: Termostato Campini Ty60R de 105 °C con reposición manual.

## Salidas:
1. Válvula de gas de seguridad: Solenoide 12 VCC, 31 Ω.
2. Válvula de gas de 7.000 kcal/h: Solenoide 12 VCC, 31 Ω.
3. Válvula de gas de 12.000 kcal/h: Solenoide 12 VCC, 31 Ω.
4. Válvula de gas de 20.000 kcal/h: Solenoide 12 VCC, 31 Ω.
5. Extractor de gases de combustión: Electroventilador 220 VAC.
6. Bomba de agua de calefacción: Electrobomba 220 VAC, 3 velocidades.
7. Encendedor electrónico a chispas: Salida 18.000 V, Entradas de 12 y 5  VCC.
