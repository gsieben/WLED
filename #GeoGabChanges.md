# Zusätzliche Files
platformio_overwirte.ini

# Änderungen am Code

## wled.h
einige werte verändert. Die meisten Änderungen in my_config.h als variable hinterlegt

## User-Mods
### Multi Relay

### Lichtsensor BH1750

Zeile 20

  if (HomeAssistantDiscovery) _createMqttSensor(F("Brightness"), mqttLuminanceTopic, F("Illuminance"), F("lx"));    // GeoGab Changes Remove space form " lx"



### Umwletsensor BMW680X
Keine änderungen da mein Code

### LD2410
Zeile 215:  
    const char LD2410Usermod::_name[]    PROGMEM = "LD2410Usermod"; 
    ->
    const char LD2410Usermod::_name[]    PROGMEM = "LD2410";    

