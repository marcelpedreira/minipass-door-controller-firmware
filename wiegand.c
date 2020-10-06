//////////////////////////////////////////////////////////////////////////////////////////
////                                      wiegand.c                                   ////
////                                                                                  ////
////             Driver para receptor/transmisor por protocolo Wiegand                ////
////                                                                                  ////
//////////////////////////////////////////////////////////////////////////////////////////
////                                                                                  ////
////  Configuracion de HW:                                                            ////
////                                                                                  ////
////  Transmisor: PIN_A1 - DATA0                                                      ////
////              PIN_A2 - DATA1                                                      ////
////                                                                                  ////
////  Receptor:   PIN_B4 - DATA0                                                      ////
////              PIN_B5 - DATA1                                                      ////
////              PIN_B6 - DATA0                                                      ////
////              PIN_B7 - DATA1                                                      ////
////                                                                                  ////
////  Funciones:                                                                      ////
////                                                                                  ////
////  wiegand_init()                                                                  ////
////    - Inicializacion del sistema                                                  ////
////                                                                                  ////
////  Receptor:                                                                       ////
////  receive_wiegand()                                                               ////
////    - Devuelve true si ha arrivado algun codigo wiegand                           ////
////                                                                                  ////
////  get_code(int &facCode, long &userCode)                                          ////
////    - Devuelve el valor del ultimo codigo Wiegand arrivado en el facCode y el     ////
////      userCode                                                                    ////
////                                                                                  ////
////  get_door()                                                                      ////
////    - Devuelve la puerta correspondiente al lector por donde llego el codigo      ////
////      Wiegand                                                                     ////
////                                                                                  ////
////  Transmisor:                                                                     ////
////  send_wiegand(int facCode, long userCode)                                        ////
////    - Envia wiegand el codigo especificado                                        ////
////                                                                                  ////
////  Nota:                                                                           ////
////  Si se va a usar como transmisor, debe definirse la constante                    ////  
////  WIEGAND_TYPE_TRANSMITER                                                         ////
////                                                                                  ////
//////////////////////////////////////////////////////////////////////////////////////////

//Variables globales a usarse fundamentalmente en las subrutinas de interrupcion 
int facCode;
long userCode = 0;
int wiegand_counter = 0;   //contador de bits de codigo Wiegand 

#ifndef WIEGAND_37
#define CodeBits 26
#define FacilityCodeLenght 9
#define UserCodeLenght 25
#else
#define CodeBits 37
#define FacilityCodeLenght 17
#define UserCodeLenght 36
#endif

#ifndef WIEGAND_TYPE_TRANSMITER     //RECEPTOR por definicion

//#define CodeBits 26

short read_complete = 0;   //bandera de lectura terminada
int buffer[CodeBits];   //buffer del codigo Wiegand que llega
int reading;   //lectura del puerto de protocolo Wiegand
short door;

//Funcion para limpiar (poner en cero) el buffer de codigo 
void clean_buffer()
{
   int i;
   for(i = 0; i < CodeBits; i++) 
      buffer[i] = 0x00;
   wiegand_counter = 0;
}

//Funcion que obtiene el FacilityCode y el UserCode a partir del buffer de codigo
void code_update()
{
   int i;
   //Facility Code
   facCode = 0x00;
   long multiplo = 1;   //variable para las potencias de 2 para formar el numero a partir de los 0s y 1s del buffer
   for(i = 1; i < FacilityCodeLenght; i++){   //El FacilityCode esta en el primer byte del buffer 
      if(buffer[i] == 1)
         facCode += multiplo;
      multiplo *= 2;
   }
   //User Code
   userCode = 0x00;
   multiplo = 1;
   for(i = FacilityCodeLenght; i < UserCodeLenght; i++){   //El userCode esta en los restantes 16 bits del buffer
      if(buffer[i] == 1)
         userCode += multiplo;
      multiplo *= 2;
   }
}

//Funcion que chequea la paridad en el codigo Wiegand arribado
short check_parity()
{
   int sum = 0;   //para contar los 1s del codigo
   int i;
   //Comprobando 1ra mitad del codigo Wiegand
   for(i = 0; i < 13; i++){    
      if(buffer[i] == 1)
         sum++;
    }
   //sum += first_parity;
   //Comprobando paridad par
   if(sum & 1)
      return 0;
   
   sum = 0;   //reinicio el contador
   //Comprobando 2da mitad del codigo Wiegand
   for(i = 13; i < 26; i++){    
      if(buffer[i] == 1)
         sum++;
    }
   //sum += last_parity;
   //Comprobando paridad impar
   if(sum & 1)
      return 1;
   
   return 0;
}

//Funcion que actualiza el buffer del codigo Wiegand
void buffer_update(short value, short currentDoor)
{
    buffer[wiegand_counter] = value;
    wiegand_counter++;
    if(wiegand_counter == CodeBits){
        read_complete = 1;
        door = currentDoor;
        disable_interrupts(int_rb);
    }
}

short receive_wiegand()
{
   if(read_complete){
      //if(check_parity()){
         read_complete = 0;
         code_update();
         clean_buffer();
         enable_interrupts(int_rb);
         return 1;
      //}
   }
   return 0;
}

void get_code(int &_facCode, long &_userCode)
{
   _facCode = facCode;
   _userCode = userCode;
}

short get_door()
{
   return door;
}

//Interrupcion por cambio de estado en los pines RB4-RB7
#int_rb      
void ext_handler()
{
   reading = input_b();
   
   //Determinando el pin del puerto B por donde se produjo el pulso 
   switch(reading){
      case 0xEF:   //RB4 = data0 puerta0
         buffer_update(0, 0);
         break;      
      case 0xDF:   //RB5 = data1 puerta0
         buffer_update(1, 0);
         break;
      case 0xBF:   //RB6 = data0 puerta1
         buffer_update(0, 1);
         break;
      case 0x7F:   //RB7 = data1 puerta1
         buffer_update(1, 1);
         break;
   }
}

#else    //TRANSMISOR

long position = 1;   //para crear la potencia de 2 y determinar el numero con 1 en el bit que interesa
short first_parity;
short last_parity;

//Funcion que realiza el pulso de 50us en el data0 o data1 segun corresponda
void pulse(short pin)
{
    output_low(pin);
    delay_us(50);
    output_high(pin);
    wiegand_counter++;      //actualizando el contador de bits del codigo Wiegand
}

//Funcion que determina la posicion o bit del codigo (facCode o userCode) que toca y realiza el pulso
void position_pulse(long code)
{
    if(code & position)   //si el bit en turno es 1 hago pulso por el data1   
      pulse(PIN_A2);
    else   //si el bit en turno es 0 hago pulso por el data0
      pulse(PIN_A1);
        
    position *= 2;   //pasando el 1 al siguiente bit   
}

//Interrupcion por el timer0 cada 2ms para el protocolo Wiegand
#int_timer0
void timer0()
{
     if(wiegand_counter == 0){   
     //primer bit de paridad 
       if(first_parity)
         pulse(PIN_A2);
       else
         pulse(PIN_A1);   
     }
     
      else if(wiegand_counter < 9)   //nos encontramos en la seccion del facCode
         position_pulse(facCode);
         
      else if(wiegand_counter < 25){   //nos encontramos en la seccion del userCode
         if(wiegand_counter == 9)   //nos encontramos en el inicio de la seccion 
            position = 1;  //hay que reiniciar la posicion del bit a comparar
         position_pulse(userCode);
      }
      
      else{      //nos encontramos en el final del codigo, por tanto ya se termino de transmitir
      
         //ultimo bit de paridad 
         if(last_parity)
            pulse(PIN_A2);
         else
            pulse(PIN_A1);   
         
         disable_interrupts(INT_TIMER0);
         wiegand_counter = 0;
         position = 1;
      }
      set_timer0(6);   //recargando el timer
}

//Funcion que completa los bits de paridad del codigo Wiegand(modificar first_parity y last_parity)
void build_parity()
{
   long code;
   long i; 
   int parity = 0;
   first_parity = 0;
   last_parity = 1;
   
   //Obteniendo la paridad par de los primeros 12 bits (los 8 del facCode mas los 4 mas significativos del userCode)
   for(i = 1; i <= 128; i *= 2){
      if(i & facCode)
         parity++;
   }
   code = userCode >> 12;
   for(i = 1; i <= 16; i *= 2){
      if(i & code)
         parity++;
   }
   if(parity & 1)
      first_parity = 1;
      
   //Obteniendo la paridad impar de los ultimos 12 bits (12 bits menos significativos del userCode)   
   parity = 0;
   for(i = 1; i <= 2048; i *= 2){
      if(i & userCode)
         parity++;
   }
   if(parity & 1)
      last_parity = 0;
}

void send_wiegand(int _facCode, long _userCode)
{
   facCode = _facCode;
   userCode = _userCode;
   build_parity();
   enable_interrupts(INT_TIMER0);
}

#endif

void wiegand_init()
{
   #ifndef WIEGAND_TYPE_TRANSMITER
   clean_buffer();
   port_b_pullups(TRUE);
   enable_interrupts(int_rb);
   #else
   setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
   set_timer0(6);   //2ms
   wiegand_counter = 0;
   position = 1;
   #endif
   enable_interrupts(GLOBAL);
}
