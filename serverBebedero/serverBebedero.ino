#include <EEPROM.h>
#include <SoftwareSerial.h>



char On = '*'; //on
char Off = '#'; //off
boolean Flag = 0, sms = 0;
const byte Piloto = 5;

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(8, 7); //SIM800L Tx & Rx is connected to Arduino #8 & #7

String datoMensaje = "";

// ****************************
//                            *
//          EEPROM            *
//                            *
// ****************************
struct Settings{
  char NumeroTelefonicoUsuario[11];
  char NumeroTelefonicoServer[11];

  int levelMAX, levelMIN;

};

Settings Configuracion;

void setup() {
  //Delay para evitar las fluctuaciones de corriente que evitan que funcione correctamente
  delay(4000);
  pinMode(Piloto, OUTPUT);
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  LecturaDeEEPROM();
  //GuardarEnEEPROM();

  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);
  
  Serial.println("Initializing...");
  


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
  
  Serial.print("numero del server "); Serial.println(Configuracion.NumeroTelefonicoUsuario); 
  //String mensage = "AT+CMGS=\"" + String(Configuracion.NumeroTelefonicoUsuario) + "\"";
  //mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  //updateSerial();
  
  //mensage = "Sistema de bebedero Medidor de nivel encendido, enviando set de encendido al Server";
  //mySerial.print(mensage); //text content
  //updateSerial();
  
  //mySerial.write(26);



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

      byte inicio=0, fin;
      
      if(datoMensaje.indexOf("**")>0)
      {  
        inicio = datoMensaje.indexOf("**");
      }
      else if(datoMensaje.indexOf("*#")>0)
      {  
        inicio = datoMensaje.indexOf("*#");
      }
      else if(datoMensaje.indexOf("*?")>0)
      {  
        inicio = datoMensaje.indexOf("*?");
      }
      else if(datoMensaje.indexOf("*$")>0)
      {  
        inicio = datoMensaje.indexOf("*$");
      }
      else if(datoMensaje.indexOf("*%?")>0)
      {  
        inicio = datoMensaje.indexOf("*%?");
      }
      else if(datoMensaje.indexOf("!!!ON")>0)
      {  
        inicio = datoMensaje.indexOf("!!!ON");
      }
      else if(datoMensaje.indexOf("!!#OFF")>0)
      {  
        inicio = datoMensaje.indexOf("!!#OFF");
      }
      fin = datoMensaje.indexOf("*!");
      

      if(datoMensaje.startsWith("**", inicio) && datoMensaje.endsWith("*!")){
        String x =  datoMensaje.substring(inicio + 2, fin);
        Serial.print("extraccion: "); Serial.println(x); 
        x.toCharArray(Configuracion.NumeroTelefonicoUsuario, 33);
        
        Serial.print("Cambiar el numero de usuario a: ");
        Serial.write(Configuracion.NumeroTelefonicoUsuario);
        GuardarEnEEPROM();
        
      }
      
      else if(datoMensaje.startsWith("*#", inicio) && datoMensaje.endsWith("*!")){
        
        String x =  datoMensaje.substring(inicio + 2, fin);
        Serial.print("extraccion: "); Serial.println(x); 
        x.toCharArray(Configuracion.NumeroTelefonicoServer, 33);
        
        Serial.print("Cambiar el numero de usuario a: ");
        Serial.write(Configuracion.NumeroTelefonicoServer);
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

        //enviarMSNtxt("Joder Tio", Configuracion.NumeroTelefonicoUsuario);
        
        String txt;

        txt = "Usuario Notificaciones: ";
        txt += Configuracion.NumeroTelefonicoUsuario;
        txt += "\n";

        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        
        txt += "Canal Servidor: ";
        txt += Configuracion.NumeroTelefonicoServer;
        txt += "\n";
        
        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);

        txt += "Nivel Minimo Sensor: "; 
        txt += Configuracion.levelMIN;
        txt += "\n";
        
        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        
        txt += "Nivel Maximo Sensor: ";
        txt += Configuracion.levelMAX;
        txt += "\n";
        
        enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);


      }

      else if(datoMensaje.startsWith("!!!ON"), inicio){
        Serial.println( "Iniciando Sensado");
        enviarMSNtxt("Iniciando el Sensado", Configuracion.NumeroTelefonicoUsuario);
        Flag = 1;
      }
      else if(datoMensaje.startsWith("!!#OFF"), inicio){
        Serial.println("Finalizando Sensado");
        Flag = 0;
        enviarMSNtxt("Terminando el sensado", Configuracion.NumeroTelefonicoUsuario);
      }

      
      datoMensaje="";
    

  }

}

void LecturaDeEEPROM(){
  Serial.println("Obteniendo datos de EPROM");
  //Settings MiObjetoResultado;
  EEPROM.get(0, Configuracion);
  
  //Configuracion = MiObjetoResultado;

  Serial.println(Configuracion.NumeroTelefonicoUsuario);
  Serial.println(Configuracion.NumeroTelefonicoServer);
  Serial.println(Configuracion.levelMIN);
  Serial.println(Configuracion.levelMAX);
}

void GuardarEnEEPROM(){
    Serial.println("Guardando en EEPROM");
    Settings Presets;

    Presets = Configuracion;
      
  
    EEPROM.put(0, Presets);
    Serial.print("Tamaño final del objeto = ");
    Serial.println(sizeof(Presets));
}


void enviarMSNtxt(String txt, char telefono[]){
  String mensage = "AT+CMGS=\"" + String(telefono) + "\"";
  mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();

  mySerial.print(txt); //text content
  updateSerial();
  
  mySerial.write(26);

}