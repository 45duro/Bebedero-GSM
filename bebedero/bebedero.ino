#include <EEPROM.h>
#include <SoftwareSerial.h>

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(8, 7); //SIM800L Tx & Rx is connected to Arduino #8 & #7


String numeroApoyo;
String servidor;
String datoMensaje;
String MensajeRecibido;

// ****************************
//                            *
//          EEPROM            *
//                            *
// ****************************
struct Settings{
  String NumeroTelefonicoUsuario;
  String NumeroTelefonicoServer;

  int levelMAX, levelMIN;

} Configuracion;

void setup() {
  //Delay para evitar las fluctuaciones de corriente que evitan que funcione correctamente
  delay(3000);
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  
  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);
  
  Serial.println("Initializing...");
  LecturaDeEEPROM();


  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CSQ"); //Prueba de calidad de la señal, el rango de valores es 0-31, 31 es el mejor
  updateSerial();
  mySerial.println("AT+CCID"); //Lea la información de la SIM para confirmar si la SIM está conectada
  updateSerial();
  mySerial.println("AT+CREG?"); //Comprueba si se ha registrado en la red.
  updateSerial();

  mySerial.println("AT+CFUN=1"); // Configuring TEXT mode
  updateSerial();
  
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  
  mySerial.println("AT+CMGR=?"); // Configuring TEXT mode
  updateSerial();
  
  mySerial.println("AT+CNMI=2,2"); // Configuring TEXT mode
  updateSerial();
  
  /*
  String mensage = "AT+CMGS=\"" + Configuracion.NumeroTelefonicoUsuario + "\"";
  mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  
  mensage = "Sistema de bebedero Medidor de nivel encendido, enviando set de encendido al Server";
  mySerial.print(mensage); //text content
  updateSerial();
  
  mySerial.write(26);

  */

}

void loop()
{
  updateSerial();
}

void updateSerial()
{
  delay(500);
  while (Serial.available())
  {
      mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }

  if(mySerial.available())
  {
      datoMensaje = mySerial.readString();

      datoMensaje.trim();
      Serial.println (datoMensaje);//Forward what Software Serial received to Serial Port

      int inicio=0, fin;
      
      if(datoMensaje.indexOf("**")>0)
      {  
        inicio = datoMensaje.indexOf("**");
      }
      if(datoMensaje.indexOf("*#")>0)
      {  
        inicio = datoMensaje.indexOf("*#");
      }
      if(datoMensaje.indexOf("*?")>0)
      {  
        inicio = datoMensaje.indexOf("*?");
      }
      if(datoMensaje.indexOf("*$")>0)
      {  
        inicio = datoMensaje.indexOf("*$");
      }
      if(datoMensaje.indexOf("*%?")>0)
      {  
        inicio = datoMensaje.indexOf("*%?");
      }
      fin = datoMensaje.indexOf("*!");
      

      if(datoMensaje.startsWith("**", inicio) && datoMensaje.endsWith("*!")){
        
        Configuracion.NumeroTelefonicoUsuario = datoMensaje.substring(inicio + 2, fin);
        Serial.println("Cambiar el numero de usuario a: " + Configuracion.NumeroTelefonicoUsuario);
        GuardarEnEEPROM();
        
      }
      else if(datoMensaje.startsWith("*#", inicio) && datoMensaje.endsWith("*!")){
        
        Configuracion.NumeroTelefonicoServer = datoMensaje.substring(inicio + 2, fin);
        Serial.println("Cambiar el numero de server a: " + Configuracion.NumeroTelefonicoServer);
        GuardarEnEEPROM();
        
      }

      else if(datoMensaje.startsWith("*?", inicio) && datoMensaje.endsWith("*!")){
        String min = datoMensaje.substring(inicio + 2, fin);
        Serial.println("Received: " + min);
        
        if(min.toInt() < 200 && min.toInt()> 2){
          Configuracion.levelMIN = min.toInt();
          Serial.print("Estableciendo nivel minimo en: ");
          Serial.println(Configuracion.levelMIN);

          GuardarEnEEPROM();
        }
        
      }

      else if(datoMensaje.startsWith("*$", inicio) && datoMensaje.endsWith("*!")){
        String max = datoMensaje.substring(inicio + 2, fin);
        Serial.println("Received: " + max);
        if(max.toInt() < 200 && max.toInt()> 2){
          Configuracion.levelMAX = max.toInt();
          Serial.print("Estableciendo nivel maximo en: ");
          Serial.println(Configuracion.levelMAX);

          GuardarEnEEPROM();
        }
        
        
      }

      else if(datoMensaje.startsWith("*%?", inicio)){
        Serial.print("Ver datos en EEPROM");
        LecturaDeEEPROM();

        String txt;

        txt = "Usuario Notificaciones: ";
        txt += Configuracion.NumeroTelefonicoUsuario;
        txt += "\n";
        txt += "Canal Servidor: ";
        txt += Configuracion.NumeroTelefonicoServer;
        txt += "\n";
        txt += "Nivel Minimo Sensor: "; 
        txt += Configuracion.levelMIN;
        txt += "\n";
        txt += "Nivel Maximo Sensor: ";
        txt += Configuracion.levelMAX;
        txt += "\n";

        enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);

      }

      datoMensaje="";

  }

}

void LecturaDeEEPROM(){
  Serial.println("Obteniendo datos de EPROM");
  Settings MiObjetoResultado;
  EEPROM.get(0, MiObjetoResultado);
  
  Configuracion = MiObjetoResultado;
}

void GuardarEnEEPROM(){
    Serial.println("Guardando en EEPROM");
    Settings Presets;

    Presets = Configuracion;
      
  
    EEPROM.put(0,Presets);
    Serial.print("Tamaño final del objeto = ");
    Serial.println(sizeof(Presets));
}


void enviarMSNtxt(String txt, String telefono){
  String mensage = "AT+CMGS=\"" + telefono + "\"";
  mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();

  mySerial.print(txt); //text content
  updateSerial();
  
  mySerial.write(26);

}