#include <EEPROM.h>
#include <SoftwareSerial.h>

# define ReleNegado true



boolean Flag = 0, sms = 0; //Flag es el estado de la bomba en el server
const byte Piloto = 13, bomba = 12;

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(8, 7); //SIM800L Tx & Rx is connected to Arduino #8 & #7

String datoMensaje = "";

//Para llevar un conteo y estar preguntando sobre los AT comand
volatile long tiempo=0, tiempo_pasado=0, intervalo = 36000000;

// ****************************
//                            *
//          EEPROM            *
//                            *
// ****************************
struct Settings{
  char NumeroTelefonicoUsuario[11];
  char NumeroTelefonicoCliente[11];
  char NumeroTelefonicoUsuario2[11];
  int levelMAX, levelMIN;  

};

Settings Configuracion;

void setup() {
  //Delay para evitar las fluctuaciones de corriente que evitan que funcione correctamente
  delay(5000);
  pinMode(Piloto, OUTPUT);
  pinMode(bomba, OUTPUT);
  //Por Seguridad apagado de bomba, evitando golpes de ariete etc
  out_Bomba(false);

  
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  LecturaDeEEPROM();
  //GuardarEnEEPROM();

  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);
  
  Serial.println("Initializing...");
  


  
  inicioSesion();
  

  enviarMSNtxt("!!!ON", Configuracion.NumeroTelefonicoCliente);
  updateSerial();
  
  
  //mySerial.write(26);



}

void loop()
{
  updateSerial();

  tiempo = millis();
  if(tiempo - tiempo_pasado > intervalo){
    inicioSesion();
    tiempo_pasado=tiempo;
  }

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
      else if(datoMensaje.indexOf("@@1*!")>0)
      {  
        inicio = datoMensaje.indexOf("@@1*!");
      }
      else if(datoMensaje.indexOf("@#0*!")>0)
      {  
        inicio = datoMensaje.indexOf("@#0*!");
      }
      else if(datoMensaje.indexOf("*@")>0)
      {  
        inicio = datoMensaje.indexOf("*@");
      }
      else if(datoMensaje.indexOf("+CLIP:")>0  && (datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoCliente))>0 || datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario))>0 || datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario2))>0))
      {
        Serial.println ("Reconociendo la llamada");
        mySerial.println("ATH");

        
        
        

        
        if(Flag == 0){
          Flag = 1;
          //Buscar quien llamó para enviarle el mensaje de confirmación
          if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoCliente))>0){
            enviarMSNtxt("Sistema encendido y dispensando agua", Configuracion.NumeroTelefonicoCliente);
          }
          else if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario))>0){
            enviarMSNtxt("Sistema encendido y dispensando agua", Configuracion.NumeroTelefonicoUsuario);
          }
          else if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario2))>0){
            enviarMSNtxt("Sistema encendido y dispensando agua", Configuracion.NumeroTelefonicoUsuario2);
          }
      
        }
        else if(Flag == 1){
          Flag = 0;
          //Buscar quien llamó para enviarle el mensaje de confirmación
          if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoCliente))>0){
            enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoCliente);
          }
          else if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario))>0){
            enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoUsuario);
          }
          else if(datoMensaje.indexOf(String(Configuracion.NumeroTelefonicoUsuario2))>0){
            enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoUsuario2);
          }
        }
      }
      else if(datoMensaje.indexOf("status")>0){
        if(Flag){
          enviarMSNtxt("Sistema encendido\ndispensando agua", Configuracion.NumeroTelefonicoUsuario);
          delay(6000);
          enviarMSNtxt("Sistema encendido\ndispensando agua", Configuracion.NumeroTelefonicoUsuario2);
          delay(6000);
          enviarMSNtxt("Sistema encendido\ndispensando agua", Configuracion.NumeroTelefonicoCliente);
        }
        else{
          enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoUsuario);
          delay(6000);
          enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoUsuario2);
          delay(6000);
          enviarMSNtxt("Sistema apagado\nNo agua", Configuracion.NumeroTelefonicoCliente);
        }
                  
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
        x.toCharArray(Configuracion.NumeroTelefonicoCliente, 33);
        
        Serial.print("Cambiar el numero de usuario a: ");
        Serial.write(Configuracion.NumeroTelefonicoCliente);
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
        txt += ", ";

        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);

        txt += Configuracion.NumeroTelefonicoUsuario2;
        txt += "\n";

        //enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario);
        
        txt += "Canal Cliente: ";
        txt += Configuracion.NumeroTelefonicoCliente;
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
        delay(10000);
        enviarMSNtxt(txt, Configuracion.NumeroTelefonicoUsuario2);


      }

      else if(datoMensaje.startsWith("@#0*!", inicio)){
        Serial.println( "APAGANDO BOMBA");
        out_Bomba(0);
        Flag = 0;
        enviarMSNtxt("&&&OFF_BOMBA*!", Configuracion.NumeroTelefonicoCliente);
      }

      else if(datoMensaje.startsWith("@@1*!", inicio)){
        Serial.println("ENCENDIENDO BOMBA");
        out_Bomba(1);
        Flag = 1;
        enviarMSNtxt("&&&ON_BOMBA*!", Configuracion.NumeroTelefonicoCliente);
      }

      if(Flag){
        out_Bomba(1);
      }
      else{
        out_Bomba(0);
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
  Serial.println(Configuracion.NumeroTelefonicoUsuario2);
  Serial.println(Configuracion.NumeroTelefonicoCliente);
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

void out_Bomba(boolean status)
{
  
  #if ReleNegado == true 
    if(status){
      digitalWrite(Piloto, 1);
      digitalWrite(bomba, 0);
    }
    else{
      digitalWrite(Piloto, 0);
      digitalWrite(bomba, 1);
    }
  #else
    if(status){
      digitalWrite(Piloto, 1);
      digitalWrite(bomba, 1);
    }
    else{
      digitalWrite(Piloto, 0);
      digitalWrite(bomba, 0);
    }

  #endif
  

}

void inicioSesion(){
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
