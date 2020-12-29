#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <HCSR04.h>

#define TRIGGER_PIN  2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum 
#define samples 10

UltraSonicDistanceSensor distanceSensor(2, 3); 
int averageMeasure = 0;

char On = '*'; //on
short prenderBomba = -1, levelAnterior=0, temporizador=0, contador=0;
boolean Flag = 0, sms = 0, llamada = 0; 
const byte Piloto = 5;
short response = 0;

volatile uint32_t tiempo, tiempoPasado, tiempoDeLanzamientoMsn=0, tiempoPasado2=0;

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
  char NumeroTelefonicoUsuario2[11];
  char NumeroTelefonicoServer[11];

  int levelMAX, levelMIN;
  unsigned int intervaloEsperaON; //intervalo en minutos
  unsigned int intervaloEsperaOFF; //intervalo en minutos


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
  


  configuracion();
  
  for(byte i = 1;  i <= 2; i++){
      msnInicio(i);
      delay(10000);
  }
  
 



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
      else if(datoMensaje.indexOf("*@")>0)
      {  
        inicio = datoMensaje.indexOf("*@");
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
      else if((datoMensaje.indexOf("*t")>0))
      {
        inicio = datoMensaje.indexOf("*t");
      }
      else if(datoMensaje.indexOf("*Level?")>0){
        enviarMSNtxt("Nivel: " + String(average()), Configuracion.NumeroTelefonicoUsuario);
        delay(6000);
        enviarMSNtxt("Nivel: " + String(average()), Configuracion.NumeroTelefonicoUsuario2);
      }

      else if(datoMensaje.indexOf("*sta=void")>0){
        sms=0;
      }

      else if(datoMensaje.indexOf("*sta=full")>0){
        sms=1;
      }
      /*
      else if(datoMensaje.indexOf("*BAT?")>0){
        mySerial.println("AT+CBC");
        
      }

      else if(datoMensaje.indexOf("CBC:?")>0){
        enviarMSNtxt("Bateria: " + datoMensaje, Configuracion.NumeroTelefonicoUsuario);
        delay(10000);
        enviarMSNtxt("Bateria: " + datoMensaje, Configuracion.NumeroTelefonicoUsuario2);
        delay(3000);
      }
      */
      

     

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

      else if(datoMensaje.startsWith("*@", inicio) && datoMensaje.endsWith("*!")){
        
        String x =  datoMensaje.substring(inicio + 2, fin);
        Serial.print("extraccion: "); Serial.println(x); 
        x.toCharArray(Configuracion.NumeroTelefonicoUsuario2, 33);
        
        Serial.print("Cambiar el numero de usuario a: ");
        Serial.write(Configuracion.NumeroTelefonicoUsuario2);
        GuardarEnEEPROM();
        
      }

      else if(datoMensaje.startsWith("*T", inicio) && datoMensaje.endsWith("*!")){
        
        String x =  datoMensaje.substring(inicio + 2, fin); 
        Serial.println("Received: " + x);

        if(x.toInt() < 180 && x.toInt()>= 1){
          Configuracion.intervaloEsperaON = x.toInt();
          Serial.print("Estableciendo nivel intervalo de lamada seguridad a: ");
          Serial.println(Configuracion.intervaloEsperaON);

          GuardarEnEEPROM();
        }

      
        
        
      }

      else if(datoMensaje.startsWith("*t", inicio) && datoMensaje.endsWith("*!")){
        
        String x =  datoMensaje.substring(inicio + 2, fin); 
        Serial.println("Received: " + x);

        if(x.toInt() < 180 && x.toInt()>= 1){
          Configuracion.intervaloEsperaOFF = x.toInt();
          Serial.print("Estableciendo nivel intervalo de lamada seguridad a: ");
          Serial.println(Configuracion.intervaloEsperaOFF);

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

        txt = "admin: ";
        txt += Configuracion.NumeroTelefonicoUsuario;
        txt += ", ";

        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);

        txt += Configuracion.NumeroTelefonicoUsuario2;
        txt += "\n";

        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        
        txt += "Ch server: ";
        txt += Configuracion.NumeroTelefonicoServer;
        txt += "\n";
        
        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);

        txt += "Min. Sensor: "; 
        txt += Configuracion.levelMIN;
        txt += "\n";
        
        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        
        txt += "Max. Sensor: ";
        txt += Configuracion.levelMAX;
        txt += "\n";

        txt += "Stop(ON, off): ";
        txt += Configuracion.intervaloEsperaON;
        txt += ",";
        txt += Configuracion.intervaloEsperaOFF;
        txt += "\n";
        
        enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        delay(10000);
        enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario2);


      }

      else if(datoMensaje.startsWith("!!!ON", inicio)){
        Serial.println( "Iniciando Sensado");
        enviarMSNtxt("Iniciando el Sensado", Configuracion.NumeroTelefonicoUsuario);
        delay(20000);
        enviarMSNtxt("Iniciando el Sensado", Configuracion.NumeroTelefonicoUsuario2);
        Flag = 1;
        delay(20000);
      }
      else if(datoMensaje.startsWith("!!#OFF", inicio)){
        Serial.println("Finalizando Sensado");
        Flag = 0;
        enviarMSNtxt("Terminando el sensado", Configuracion.NumeroTelefonicoUsuario);
        delay(20000);
        enviarMSNtxt("Terminando el sensado", Configuracion.NumeroTelefonicoUsuario2);
        delay(20000);
      }

         
      datoMensaje="";
    

  }

  /*

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

  if( llamada==1 &&  tiempo - tiempoPasado > 8000){
    //colgar si se hizo una llamada
    mySerial.println("ATH");
    llamada = 0;
    
  }

  */

}

void LecturaDeEEPROM(){
  Serial.println("Obteniendo datos de EPROM");
  //Settings MiObjetoResultado;
  EEPROM.get(0, Configuracion);
  
  //Configuracion = MiObjetoResultado;

  Serial.println(Configuracion.NumeroTelefonicoUsuario);
  Serial.println(Configuracion.NumeroTelefonicoUsuario2);
  Serial.println(Configuracion.NumeroTelefonicoServer);
  Serial.println(Configuracion.levelMIN);
  Serial.println(Configuracion.levelMAX);
  Serial.println(Configuracion.intervaloEsperaON);
  Serial.println(Configuracion.intervaloEsperaOFF);
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
    Serial.print(Sensor);

    Serial.print("  levelAnt: ");
    Serial.print(levelAnterior);

    Serial.print("  temporizador: ");
    Serial.print(temporizador);

    Serial.print("  sms: ");
    Serial.println(sms);

    tiempo = millis();
    
    if ( (Sensor < 160) and (Sensor > Configuracion.levelMAX ) and (sms == 0) ){

            prenderBomba=1;
            sms = 1;
            tiempoPasado = tiempo;
            temporizador = 1;
            levelAnterior=Sensor;

            //MAntenimiento descomentar para el proximo mantenimiento
            //contador = 0;
          }

    if ( (Sensor > 35) and (Sensor < Configuracion.levelMIN ) and (sms == 1)  ){
            
            prenderBomba =0;
            sms = 0;
            tiempoPasado = tiempo;
            temporizador = 2;
            levelAnterior=Sensor;
            //Mantenimiento descomentar para el proximo mantenimiento
            //contador = 0;
         }


    if(prenderBomba==1){
      Serial.println("Nivel Bajo");
      llamadaServer();
      
      analogWrite(Piloto,200);
      prenderBomba=-1;
    }
    else if(prenderBomba==0){
      Serial.println("Nivel Alto");
      llamadaServer();
      
      analogWrite(Piloto,0);
      prenderBomba=-1;
      
    }

    
    if(((levelAnterior - Sensor)<-10) and (temporizador==1) and ((tiempo - tiempoPasado) > minToMilis(Configuracion.intervaloEsperaON))){
        Serial.println("Entré en el reintento para encender");
        prenderBomba=1;
        contador++;

        if (contador>=5){
          enviarMSNtxt("La bomba no enciende despues de 5 intentos revisar",Configuracion.NumeroTelefonicoUsuario);
          delay(8000);
          enviarMSNtxt("La bomba no enciende despues de 5 intentos revisar",Configuracion.NumeroTelefonicoUsuario2);
          contador=0;
          temporizador=0;
        }
        levelAnterior=Sensor;
        tiempoPasado=tiempo;

        
    }

    if(((levelAnterior-Sensor)>2) and (temporizador==2) and ((tiempo - tiempoPasado) > minToMilis(Configuracion.intervaloEsperaOFF))){
        Serial.println("Entré en el reintento apagar");
        prenderBomba=0;
        contador++;

        if (contador>=7){
          enviarMSNtxt("La bomba no enciende despues de 7 intentos revisar",Configuracion.NumeroTelefonicoUsuario);
          delay(8000);
          enviarMSNtxt("La bomba no enciende despues de 7 intentos revisar",Configuracion.NumeroTelefonicoUsuario2);
          contador=0;
          temporizador=0;
        }
        levelAnterior=Sensor;
        tiempoPasado=tiempo;
    }

    
    if((tiempo - tiempoPasado2) > minToMilis(120)){
        configuracion();
        tiempoPasado2=tiempo;
    }
  }
}


void msnInicio(byte numberUser){
  String mensage;
  switch (numberUser)
  {
    case 1:
      Serial.print("numero del Usuario1 "); Serial.println(Configuracion.NumeroTelefonicoUsuario); 
      mensage = "AT+CMGS=\"" + String(Configuracion.NumeroTelefonicoUsuario) + "\"";
      mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
      updateSerial();
      
      mensage = "Sistema de bebedero Medidor de nivel encendido, enviando set de encendido al Server";
      mySerial.print(mensage); //text content
      updateSerial();
      
      mySerial.write(26);
    break;

    case 2:
      Serial.print("numero del Usuario2 "); Serial.println(Configuracion.NumeroTelefonicoUsuario2); 
      mensage = "AT+CMGS=\"" + String(Configuracion.NumeroTelefonicoUsuario2) + "\"";
      mySerial.println(mensage);//change ZZ with country code and xxxxxxxxxxx with phone number to sms
      updateSerial();
      
      mensage = "Sistema de bebedero Medidor de nivel encendido, enviando set de encendido al Server";
      mySerial.print(mensage); //text content
      updateSerial();
      
      mySerial.write(26);
    break;
  
  }
    
}


void configuracion(){
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
}

void llamadaServer(){
  
  Serial.println("Estoy ingresando a llamar");
  mySerial.println("ATD" + String(Configuracion.NumeroTelefonicoServer)+ ";");
  
  delay(9000);
  
  //Colgar
  mySerial.println("ATH");
}

uint32_t minToMilis(unsigned int x){
  return x*60000;
}
