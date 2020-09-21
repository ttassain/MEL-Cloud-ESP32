# MEL Cloud Demo

Petit programme qui permet de controler les splits de ma clim d'un seul endroit

## ESP-32

### OLED Module d'affichage 128X64 I2C SSD1306

| GPIO | Fonction | Description |
| --- | ----------- | ----------- |
| 3.3V | VDD |  |
| GND | GND |  |
| 22 | SCL |  |
| 21 | SDA |  |

### Boutons

| GPIO | Fonction | Description |
| --- | ----------- | ----------- |
| 13 | UP | Haut|
| 12 | DOWN | Bas |
| 32 | RIGHT | Droite |
| 33 | LEFT | Gauche |
| 14 | OK | TODO : Pour le moment ça refresh |
| 26 | SET | +1 a la valeur |
| 25 | CLEAR | -1 a la valeur |

### Leds

| GPIO | Fonction | Description |
| --- | ----------- | ----------- |
| 02 | RED | Appel API en cours |
| 15 | GREEN | Prêt |

### Autres

| GPIO | Fonction | Description |
| --- | ----------- | ----------- |
| 04 | BUZZER | |

## Author

TASSAIN Thierry

## License

MEL Cloud Demo is licensed under the Apache v2 License. See the LICENSE file for more info.