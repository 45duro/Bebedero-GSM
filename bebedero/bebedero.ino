#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <HCSR04.h>

#define TRIGGER_PIN  2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum 
#define samples 5

UltraSonicDistanceSensor distanceSensor(2, 3); 
int averageMeasure = 0;

char On = '*'; //on
char Off = '#'; //off
boolean Flag = 0, sms = 0, llamada = 0; 
const byte Piloto = 5;
short response = 0;

unsigned int tiempo, tiempoPasado, tiempoDeLanzamientoMsn=0;

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
  unsigned int intervaloEspera;

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

  mySerial.println("AT+CLIP=1"); // Mostrar quien llama
  updateSerial();
  
  Serial.print("numero del server "); Serial.println(Configuracion.NumeroTelefonicoUsuario); 
  String mensage = "AT+CMGS=\"" + String(Configuracion.NumeroTelefonicoUsuario) + "\"";
  mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  
  mensage = "Sistema de bebedero Medidor de nivel encendido, enviando set de encendido al Server";
  mySerial.print(mensage); //text content
  updateSerial();
  
  mySerial.write(26);



}

void loop()
{
  updateSerial();
  Estado();
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
      else if(datoMensaje.indexOf("&&&ON_BOMBA*!")>0)
      { 
        response = 1; 
        inicio = datoMensaje.indexOf("&&&ON_BOMBA*!");
      }
      else if(datoMensaje.indexOf("&&&OFF_BOMBA*!")>0)
      { 
        response = 2; 
        inicio = datoMensaje.indexOf("&&&OFF_BOMBA*!");
      }
      else if(datoMensaje.indexOf("NO CARRIER")>0)
      { 
        mySerial.println("ATH");
      }
      else if((datoMensaje.indexOf("*T")>0))
      {
        inicio = datoMensaje.indexOf("*T");
      }
      else if(datoMensaje.indexOf("*Level?")>0){
        enviarMSNtxt("Nivel: " + String(average()), Configuracion.NumeroTelefonicoUsuario);
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

      else if(datoMensaje.startsWith("*T", inicio) && datoMensaje.endsWith("*!")){
        
        String x =  datoMensaje.substring(inicio + 2, fin); 
        Serial.println("Received: " + x);

        if(x.toInt() < 60000 && x.toInt()> 10000){
          Configuracion.intervaloEspera = x.toInt();
          Serial.print("Estableciendo nivel intervalo de lamada seguridad a: ");
          Serial.println(Configuracion.intervaloEspera);

          GuardarEnEEPROM();
        }
        
        
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

      else if(datoMensaje.startsWith("!!!ON", inicio)){
        Serial.println( "Iniciando Sensado");
        enviarMSNtxt("Iniciando el Sensado", Configuracion.NumeroTelefonicoUsuario);
        delay(5000);
        Flag = 1;
      }
      else if(datoMensaje.startsWith("!!#OFF", inicio)){
        Serial.println("Finalizando Sensado");
        Flag = 0;
        enviarMSNtxt("Terminando el sensado", Configuracion.NumeroTelefonicoUsuario);
      }

         
      datoMensaje="";
    

  }

  tiempo = millis();
  //Serial.print("  respo: "); Serial.print(response);
  //Serial.print("  t: "); Serial.print(tiempo); Serial.print(" Fot: "); Serial.print(tiempoDeLanzamientoMsn); Serial.print(" inter "); Serial.println(intervaloEspera);
  
  if(response == -1 && (tiempo - tiempoDeLanzamientoMsn > Configuracion.intervaloEspera)){
    //llame
    Serial.println("Estoy ingresando a llamar");
    mySerial.println("ATD" + String(Configuracion.NumeroTelefonicoServer)+ ";");
    llamada = 1;
    response= 0;
    tiempoPasado = tiempo;
  }

  if( llamada==1 &&  tiempo - tiempoPasado > 7000){
    //colgar si se hizo una llamada
    mySerial.println("ATH");
    llamada = 0;
    
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

int average (){
  for(byte i = 0; i < samples; i++){
    delay(100);
    averageMeasure += distanceSensor.measureDistanceCm();     
  }
  
  int medidaFinal = averageMeasure/samples;
  averageMeasure = 0;
  return medidaFinal;
}




void Estado(){
  if (Flag == 1){
    int Sensor = average();
    Serial.print("Sensor: ");
    Serial.println(Sensor);
    if ( (Sensor > 130) and (Sensor < Configuracion.levelMAX ) and (sms == 0) ){
            Serial.println("Nivel Bajo");
            enviarMSNtxt("@@1*!", Configuracion.NumeroTelefonicoServer);
            delay(500);
            tiempoDeLanzamientoMsn = millis();
            sms = 1;
            analogWrite(Piloto,200);
            response = -1;
            
          }

    if ( (Sensor > 10) and (Sensor < Configuracion.levelMIN ) and (sms == 1)  ){
           Serial.println("Nivel Alto");
           enviarMSNtxt("@#0*!", Configuracion.NumeroTelefonicoServer);
           delay(500);
           tiempoDeLanzamientoMsn = millis();
           sms = 0;
           analogWrite(Piloto,0);
           response = -1;
         }

  }
}