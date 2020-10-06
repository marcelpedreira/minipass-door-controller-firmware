#include <16F877A.h>

#FUSES NOWDT                    
#FUSES NOBROWNOUT               
#FUSES NOLVP                    
#use delay(clock=4000000)

#define eeprom_address long int 
#define MODBUS_TYPE MODBUS_TYPE_SLAVE
#define MODBUS_SERIAL_TYPE MODBUS_RTU     
#define MODBUS_SERIAL_RX_BUFFER_SIZE 64
#define MODBUS_SERIAL_BAUD 9600
#define MODBUS_SERIAL_INT_SOURCE MODBUS_INT_RDA
#define MODBUS_ADDRESS 1
#define max_user_count 0x0F   //Este valor debe estar en 255(0xFF) en la version final
#define max_access_count 0x0F   //Este valor debe estar en 255(0xFF) en la version final
#define green_led_door_0 PIN_D1
#define green_led_door_1 PIN_A1
#define red_led_door_0 PIN_D2
#define red_led_door_1 PIN_A2
#define open_door_0 PIN_C5
#define open_door_1 PIN_A3
#define buzzer_door_0 PIN_C2
#define buzzer_door_1 PIN_A4
#define WIEGAND_37 1

//#define use_external_memory 1	//Se define si se va a usar memoria externa para la lista blanca 
//#define use_emulated_rtc 1	//Se define si se va a usar un rtc emulado 
	

int seconds;
int minutes;
int hours;
int date;
int mounth;
int year_high = 20;
int year_low;
short current_door;
int user;
int time_counter = 0;

#include <wiegand.c>
#include <modbus.c>
#include <EEPROMmemory.c>
#include <rtcDS1307.c>
//#include <LCD2.c>

EEPROM_ADDRESS address = 0;
EEPROM_ADDRESS access_address = 2;   //direccion corriente de la memoria de accesos
BYTE access_event_count = 0;
int16 mbus_data[16];

//Funcion que obtiene la direccion en la red del controlador seteada a traves de jumpers
int get_modbus_address()
{
	int temp = input_a();
	temp &= 0b00001111;
	return temp;
}

//Funcion que crea la data que hay que pasar como respuesta al master
void regist_data(int index, int offset)
{
   int high, low, i;
   for(i = 0; i < offset; i++){
      high = read_regist_memory(index);
      index++;
      low = read_regist_memory(index);
      index++;
      mbus_data[i] = make16(high, low);
   }
}

//Funcion que crea la data que hay que pasar como respuesta al master
void conf_data(int index, int offset)
{
   int high, low, i;
   for(i = 0; i < offset; i++){
      high = read_white_memory(index);
      index++;
      low = read_white_memory(index);
      index++;
      mbus_data[i] = make16(high, low);
   }
}

//Funcion que limpia los registros
void clean_registers()
{
	write_regist_memory(0, 0);
	write_regist_memory(1, 0);
	access_address = 2;
	access_event_count = 0;
}

//Funcion que limpia los usuarios
void clean_white_memory()
{
   write_white_memory(0, 0);
   write_white_memory(1, 0);
}

//Funcion que desbloquea la puerta por un tiempo
void handle_door()
{
	if(current_door){   //Puerta 1
      output_high(green_led_door_1);
	  output_low(red_led_door_1);
	  output_high(open_door_1);
   }
   else{ //Puerta 0
      output_high(green_led_door_0);
      output_low(red_led_door_0);
      output_high(open_door_0);
   }
   enable_interrupts(int_timer1);
   set_timer1(3036);
}

//Acciones al producirse un acceso
void regist()
{
   //Registrando el acceso
   real_time(minutes, hours, date, mounth, year_low);

   write_regist_memory(access_address, user);
   access_address++;
   write_regist_memory(access_address, minutes);
   access_address++;
   write_regist_memory(access_address, hours);
   access_address++;
   write_regist_memory(access_address, date);
   access_address++;
	write_regist_memory(access_address, mounth);
   access_address++;
   write_regist_memory(access_address, year_high);
   access_address++;
	write_regist_memory(access_address, year_low);
   access_address++;
   write_regist_memory(access_address, current_door);
   access_address++;
   
   real_time(minutes, hours, date, mounth, year_low);
   
   access_event_count++;
   if(access_address == access_event_count * 8 - 1){
      access_address = 2;
	  access_event_count = 0;
   }
   write_regist_memory(1, access_event_count);
}

//Funcion que busca un codigo dentro de la lista blanca
short find_user(int &facCode, long &userCode)
{
   int read;
   int FacilityCode;
   long IdCardCode;
   int access;
   address = 2;               
         
   //Aqui primero hay que actualizar el userCount leyendo la posicion 1 de la memoria
   int user_count = read_white_memory(1); 
   
   //Recorriendo la memoria de codigos
   for(user = 0; user < user_count; user++){ 
      IdCardCode = 0;
      //Obteniendo el FacilityCode (1er byte del bloque que ocupa el codigo en la memoria I2C)
      FacilityCode = read_white_memory(address);
      address++;
      //Obteniendo el UserCode (Sgtes 2 bytes del bloque que ocupa el codigo en la memoria I2C)
      read = read_white_memory(address);    
      address++;
      IdCardCode = read * 256;
      read = read_white_memory(address);    
      address++;
      IdCardCode += read;
      //Obteniendo la politica de acceso (Ultimo byte del bloque que ocupa el codigo en la memoria I2C)
      access = read_white_memory(address);
      address++;
      //Comparando 
      if(facCode == FacilityCode && userCode == IdCardCode){
		switch(access){
			case 1:
				if(current_door == 0)
					return 1;
				break;
			case 2:
				if(current_door == 1)
					return 1;
				break;
			case 3:
				return 1;
		}   
	  }
	}
	user = 254;	//Acceso denegado
	return 0;
}

//Interrupcion externa utilizada para el rex     
#int_ext
void intRB0_handler()
{
   current_door = 0;
   user = 255;	//REX
   regist();
   handle_door();
}
    
//Interrupcion por timer1 utilizada para volver a bloquear la puerta despues de producirse un accceso
#int_timer1
void timer1()
{
	if(time_counter < 5)
		time_counter++;
	else{
		time_counter = 0;
   		if(current_door){   //Puerta 1
      		output_low(green_led_door_1);
	  		output_high(red_led_door_1);
	  		output_low(open_door_1);
   		}
   		else{ //Puerta 0
      		output_low(green_led_door_0);
      		output_high(red_led_door_0);
      		output_low(open_door_0);
   		}
   		disable_interrupts(int_timer1);
	}
}

void main() 
{
  int facCode; 
  long userCode = 0;
  //int MODBUS_ADDRESS = get_modbus_address();

  modbus_init();
  wiegand_init();
  //lcd_init();
  
  clean_registers();
  clean_white_memory();

  minutes = 8;
  hours = 8; 
  date = 9; 
  mounth = 1; 
  year_low = 14;
  rtc_init(minutes, hours, date, mounth, year_low);

  write_white_memory(0, 0);
  write_white_memory(1, 1);
  write_white_memory(2, 1);
  write_white_memory(3, 0x5B);
  write_white_memory(4, 0xA0);
  write_white_memory(5, 3);
                     
  //Inicializando las salidas 
  output_low(green_led_door_0);
  output_high(red_led_door_0);
  /*output_low(open_door_0);
  output_low(green_led_door_1);
  output_high(red_led_door_1);
  output_low(open_door_1);
  output_low(buzzer_door_0);
  output_low(buzzer_door_1);*/
    
  setup_adc_ports(NO_ANALOGS);
  port_b_pullups(true);
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
  
  //Habilitando interrupciones
  enable_interrupts(int_ext);
  ext_int_edge(H_TO_L);
  enable_interrupts(global);
  
  while (TRUE){
  
      if(receive_wiegand()){
         current_door = get_door();
         if(current_door){
            output_high(buzzer_door_1);
            delay_ms(500);
            output_low(buzzer_door_1);
         }
         else{
            output_high(buzzer_door_0);
            delay_ms(500);
            output_low(buzzer_door_0);
         }
         
         get_code(facCode, userCode);
         if(find_user(facCode, userCode))
			handle_door();
		 regist();
      }
         
      if(modbus_kbhit()){
         delay_us(50);
         //check address against our address, 0 is broadcast
         if((modbus_rx.address == MODBUS_ADDRESS) || modbus_rx.address == 0)
         {
            switch(modbus_rx.func)
            {
			   case FUNC_WRITE_SINGLE_COIL:	//Esta funcion se utiliza para limpiar los registros
				  clean_registers();
				  modbus_write_single_coil_rsp(MODBUS_ADDRESS,modbus_rx.data[1],((int16)(modbus_rx.data[2]))<<8);
				  break;
			   case FUNC_READ_HOLDING_REGISTERS:	//Esta funcion sera utilizada para leer la lista blanca
                  conf_data(modbus_rx.data[1],modbus_rx.data[3]);
                  modbus_read_holding_registers_rsp(MODBUS_ADDRESS,(modbus_rx.data[3]*2),mbus_data);
                  break;
               case FUNC_READ_INPUT_REGISTERS:	//Esta funcion sera utilizada para leer los registros de accesos producidos
                  regist_data(modbus_rx.data[1],modbus_rx.data[3]);
                  modbus_read_input_registers_rsp(MODBUS_ADDRESS,(modbus_rx.data[3]*2),mbus_data);
				  break;
               case FUNC_WRITE_MULTIPLE_REGISTERS:	//Esta funcion sera utilizada para actualizar la lista blanca asi como la fecha y hora
				  //Actualizando la hora 
				  minutes = modbus_rx.data[5];
				  hours = modbus_rx.data[6];
				  date = modbus_rx.data[7];
				  mounth = modbus_rx.data[8];
				  year_high = modbus_rx.data[9];
				  year_low = modbus_rx.data[10];
				  rtc_init(minutes, hours, date, mounth, year_low); 
				  //Actualizando la memoria de lista blanca
				  address = modbus_rx.data[1] * 2; //un registro modbus contiene dos registros de memoria de codigos
				  //Poniendo la cantidad de usuarios de codigo
				  write_white_memory(address, modbus_rx.data[11]);
                  address++;
                  write_white_memory(address, modbus_rx.data[12]);
                  address++;
				  //Poniendo los codigos
				  int i;
				  for(i = 13; i < modbus_rx.data[4]; i++){
                     write_white_memory(address, modbus_rx.data[i]);
                     address++;
                  }
                  modbus_write_multiple_registers_rsp(MODBUS_ADDRESS, make16(modbus_rx.data[0],modbus_rx.data[1]), make16(modbus_rx.data[2],modbus_rx.data[3]));
                  break; 
               default:    //We don't support the function, so return exception
                  modbus_exception_rsp(MODBUS_ADDRESS,modbus_rx.func,ILLEGAL_FUNCTION);
            }
         }
      }
      /*real_time(minutes, hours, date, mounth, year_low);
      printf(lcd_putc, "\f%u/%u/%u\n%u:%u:%u", date, mounth, year_low, hours, minutes, seconds);
      real_time(minutes, hours, date, mounth, year_low);
      delay_ms(1000);*/
  }
}
