#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdint.h>
#include <stddef.h>
struct telemetria
{
    int16_t altura;
    int16_t temperatura;
    float radiacion;
    int16_t numero_comprobaciones;
    uint8_t Check_sum;
};
uint8_t Cks(  struct telemetria *aux);
void llamar_hilo(void *arg1, void *arg2, void *arg3);
void rti(void *arg1, void *arg2, void *arg3);
K_THREAD_DEFINE(rtos,2048,llamar_hilo,NULL,NULL,NULL,3,0,0);/*macro estática que reserva memoria y crea sus estructuras*/
K_THREAD_DEFINE(aux,2048,rti,NULL,NULL,NULL,0,0,0);/*prioridad maxima*/
K_SEM_DEFINE(semaforo,0,1);/*creamos un semaforo para dormir un proceso que revisa parametros y la rutina de emergencia funcione*/
K_MSGQ_DEFINE(comunicacion,sizeof(struct telemetria),20,4);/* 20 paquetes de 4  alineación memoria*/
void llamar_hilo(void *arg1, void *arg2, void *arg3){ /*Cuando llamo a una funcion tiene que ser con tres argumentos*/
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2); /*En este proyecto no se van a utilizar estas entradas de la funcion, para que el compilador nose queje UNUSED*/
    ARG_UNUSED(arg3);
    struct telemetria datos={0};/*inicializamos los valores a 0*/
    /*Este hilo va a comprobar los parametros  de vuelo*/
    while (1) {
        datos.numero_comprobaciones++;
        datos.altura = 10000 + (sys_rand16_get() % 2000);
        datos.temperatura = -35 + (sys_rand16_get() % 21);  /*Nos inventamos valores pero los acotamos para que tenga sentido*/
        datos.radiacion = (sys_rand32_get() % 500) / 100.0f;

        if(datos.altura> 11820|| datos.radiacion> 3){
            k_sem_give(&semaforo); /*Despierta RTI*/
        }
        datos.Check_sum=Cks(&datos);
        k_msgq_put(&comunicacion,&datos,K_NO_WAIT);
        k_msleep(1000); /*Cada 1 segundo hago la comprobación */
    }
}
void rti(void *arg1, void *arg2, void *arg3){
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2); 
    ARG_UNUSED(arg3);
    while (1)
    {
         k_sem_take(&semaforo,K_FOREVER);/*Al tener máxima prioridad empieza aqui y se duerme */
        /*El protocolo que sea..... hacemos  un print para que se vea por pantalla aunque en situcion real no se gasta tiempo en eso*/
        printf("Error saltamos RTI \n");
    }
    

}
uint8_t Cks( struct telemetria *aux){
    uint8_t res=0;
    aux->Check_sum =0;
    uint8_t *pt= (uint8_t*)aux; /* Cresamos un punteropara recorrer bytes dela estructura*/
    for(size_t i=0;i<sizeof(struct telemetria);i++){
        res+=pt[i];
    }
    return res;

}



int main(void)
{
/*  En este s.o el main es un hilo pero cuando se termina no importa los demas siguen activos, por buenas practicas de 
    programación espreferible crear un hilo diferente que se crea con su propia pila para fallos, el main es  para ver si 
    el sistema a cargado correctamente*/
    printf("Sistema en el main \n");
    struct telemetria imprimir;
    while(1){

        k_msgq_get(&comunicacion,&imprimir,K_FOREVER);
        uint8_t recibido= imprimir.Check_sum;
        if(recibido!=Cks(&imprimir)) {
            printf("Reenviar el mensaje \n");
            continue;
        }  


        printf("N-vez:%d | Altura:%d | Temperatur:%d | Radiacion:%.2f \n",imprimir.numero_comprobaciones,
            imprimir.altura,imprimir.temperatura,imprimir.radiacion);
    }
    
    return 0;

}


