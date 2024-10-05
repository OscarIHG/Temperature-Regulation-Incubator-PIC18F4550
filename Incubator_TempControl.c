/*
 * File:   Proyecto Final.c
 * Author: Oscar Iván
 *
 * Created on 30 de noviembre de 2021, 01:03 PM
 */

#include <xc.h>
#include"configuraciones.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

volatile uint8_t interrupcionAct = 0; //Interrupcion del timer0

//Configuramos el puerto serial para transmitir y recibir datos
void SerialBegin(int velocidad){
    RCSTAbits.SPEN = 1; //Configura en el puerto C el USART
    
    TRISCbits.RC6 = 0; //Declara en el puerto C el pin 6 como TX salida
    TRISCbits.RC7 = 1; // Declara en el puerto C el pin 7 como RX entrada
    
    //Transmision 
    TXSTAbits.TX9 = 0; //Transmisión de 8 bits
    TXSTAbits.TXEN = 1; //Transmisión habilitada
    TXSTAbits.SYNC = 0;// Modo asíncrono
    
    //Recepcion 
    RCSTAbits.SPEN = 1; // Configura los puertos RX y TX como seriales
    RCSTAbits.RX9 = 0; //Recepcion de 8 bits
    RCSTAbits.CREN = 1; // Habilita la recepción contínua
    TXSTAbits.BRGH = 1; //Ritmo de los badios a alta velocidad
    BAUDCONbits.BRG16 = 1; // Genera un ritmo de baudios en 16 bits
    
    switch(velocidad){
    case 9600:
    SPBRG = 207;
    break;
    case 19200:
    SPBRG = 103;
    break;
    case 57600:
    SPBRG = 34;
    break;
    case 115200:
    SPBRG = 16;
    break;
    default: SPBRG = 207;
    }   
}

//Conversion analogica/digital.
void configuraADC(){
    //Paso 1 - Configurar los voltajes de referencia para ADC
    ADCON1bits.VCFG0 = 0;
    ADCON1bits.VCFG1 = 0;
    //Paso 2 - Configurar cuantas entradas analogicas se usaran
    ADCON1bits.PCFG = 9; //6 entradas analogicas (1001)
    //Paso 3 - Configurar el formato de resultado
    ADCON2bits.ADFM = 1; //Derecha
    //Paso 4 - Configurar el tiempo de conversion (TAD)
    ADCON2bits.ACQT = 0; 
    //Paso 5 - Seleccionar la pre-escala para el oscilador para el ADC
    ADCON2bits.ADCS = 0; // Fosc/2
    //Paso 6 - Activar el modulo
    ADCON0bits.ADON = 1; 
}

//Obtiene la señal analógica del pin otorgado y lo guarda en un registro 
int analogRead(unsigned char canal){
    int resultado = 0;
    //Paso 1 - Seleccionar el canal
    ADCON0bits.CHS = canal;
    //Paso 2 - Iniciar la conversion
    ADCON0bits.GODONE = 1;
    //Paso 3 - Esperar a que termine la conversion
    while(ADCON0bits.GODONE == 1){
        }
    //Paso 4 - Agrupar el resultado
    resultado = (unsigned int)ADRESH << 8;
    resultado = resultado + ADRESL;
    //Paso 5 - Regresar el resultado
    return resultado;
}

void main(void){
    float lecturaADC = 0; //Variable que guarda la señal analógica
    double tempADC = 0; //Variable que guarda la conversión a °C
    OSCCON = 0b01110010;
    TRISB = 0b00000000; //Puerto B configurado como salidas
    SerialBegin(9600); //Se llama a la función para que se ejecute
    //Configurar timer 0 con cronometro
    //Paso 1 - Limpiar la bandera de la interrupción del timer 0
    INTCONbits.TMR0IF = 0;
    //Paso 2 - Habilitar banderas globales
    INTCONbits.GIE = 1;
    //Paso 3 - Habilitar banderas perifericas
    INTCONbits.PEIE = 1;
    //Paso 4 - activar la pre-escala
    T0CONbits.PSA = 0;
    //Paso 5 - Configurar la pre-escala del timer para generar retardo de 500 ms
    T0CONbits.T0PS0 = 0;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS2 = 1;
    TMR0H = 0xE1;
    TMR0L = 0x7B;
    //Paso 6 - configurar el oscilador del timer0
    T0CONbits.T0CS = 0;
    //Paso 7 - Habilitar el modo de funcionamiento del timer en cronometro de 16 bits
    T0CONbits.T08BIT = 0;
    //Paso 8 - Habilitar la interrupcion por desbordamiento
    INTCONbits.TMR0IE = 1;
    //Paso 9 - Habilitar el timer
    T0CONbits.TMR0ON = 1;
    configuraADC(); //Llamamos la funcion configuraADC
    while(1){
    //Se adquiere la señal analógica y se convierte a grados centígrados
    lecturaADC = analogRead(0);
    /* Aqui lo que se hace es volver a obtener el valor
     del voltaje que entrega el sensor, pues entre un valor de entre 0
     a 1023 */
    tempADC = (lecturaADC*0.00488)*100;
    /*En caso de que la bandera sea activada por la interrupción*/
    if (interrupcionAct == 1){
    interrupcionAct = 0;
    //Se muestra la temperatura en el monitor serial
    printf("La temperatura es de %.2f C\n\r", tempADC);
    /*Si la temperatura es menor que 36, se enciente
    el pin B4 y se apaga el B3 */
    if (tempADC < 36.00){
        LATBbits.LATB3 = 0;
        LATBbits.LATB4 = 1;
    }
    /*Si la temperatura es mayor a 37.5, se enciende el
    pin B3 y se apaga el B4*/
    else if (tempADC > 37.50){
        LATBbits.LATB4 = 0;
        LATBbits.LATB3 = 1;
    }
    /*Si la temperatura permanece en los parámetros, ambos
    pines permanecen apagados */
    else {
    LATBbits.LATB4 = 0;
    LATBbits.LATB3 = 0;
    }
    }
    }
    return;
}

void __interrupt(__high_priority) interrupcionPrincipal(void){
    /*Se activa la bandera y se reestablecen
    los valores para el timer0 del retardo de 500ms */
    if(INTCONbits.TMR0IF == 1){
    INTCONbits.TMR0IF = 0;
    interrupcionAct = 1;
    TMR0H = 0xE1;
    TMR0L = 0x7B;
    }
}
