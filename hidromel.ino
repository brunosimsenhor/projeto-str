/* Para resfriar considerar Chiller de serpentina */

/* Incluir biblitoecas */
#include <Arduino_FreeRTOS.h>
#include "DHT.h"
#include <task.h>
#include <queue.h>

/* Definições gerais */
#define BAUDRATE_SERIAL 9600

/* Definições - DHT11 */
#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/* Definições Leds*/
#define LED_AZUL 3
#define LED_VERDE 4
#define LED_VERMELHO 5


/* filas (queues) */
QueueHandle_t fila_dht11_temperatura;
QueueHandle_t fila_led_azul;
QueueHandle_t fila_led_verde;
QueueHandle_t fila_led_vermelho;
QueueHandle_t fila_aquecimento;
QueueHandle_t fila_resfriamento;

/* Define - tempo para aguardar tomada de controle da fila */
#define TEMPO_PARA_AGUARDAR_FILA (TickType_t ) 1000

/* TAREFAS DE SENSOR */
/* Tarefa de aquisição de dados */
void task_dht11_temperatura( void *pvParameters );

/* TAREFA DE CONTROLE*/
void task_controle( void *pvParameters );

/* TAREFA DE ATUAÇÃO */
/* Tarefas dos leds*/
void task_led_azul( void *pvParameters );
void task_led_vermelho( void *pvParameters );
void task_led_verde( void *pvParameters );
/* Tarefas aquecimento e resfriamento */
void task_aquecimento(void *pvParameters );
void task_resfriamento(void *pvParameters );



void setup() {

  /* Saidas dos LEDs */
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  
  /* Inicializar serial */
  Serial.begin(BAUDRATE_SERIAL);
  
  /* Inicializar dht11 */
  dht.begin();

  /* Criação das filas (queues) */ 
  fila_dht11_temperatura = xQueueCreate( 1, sizeof(float) );
  fila_led_azul = xQueueCreate( 1, sizeof(boolean) );
  fila_led_verde = xQueueCreate( 1, sizeof(boolean) );
  fila_led_vermelho = xQueueCreate( 1, sizeof(boolean) );
  fila_aquecimento = xQueueCreate( 1, sizeof(boolean) );  
  fila_resfriamento = xQueueCreate( 1, sizeof(boolean) );  

  
  /* Verifica se as filas foram corretamente criadas */
  if ( (fila_dht11_temperatura == NULL || fila_led_azul == NULL || fila_led_verde == NULL || fila_led_vermelho == NULL || fila_aquecimento == NULL || fila_resfriamento == NULL   ) )
  {
      Serial.println("A fila nao foi criada.");
      Serial.println("O programa nao pode continuar");
      while(1)
      {
        
      }
  }


  /* Criar tarefas */
  xTaskCreate(
      task_dht11_temperatura        /* Funcao que implementa a tarefa */
      ,  (const portCHAR *)"dht11"  /* Nome (para fins de debug, se necessário) */
      ,  1024                       /* Tamanho da stack (em words) reservada para essa tarefa */
      ,  NULL                       /* Parametros passados (nesse caso, não há) */
      ,  2                          /* Prioridade */
      ,  NULL );                    /* Handle da tarefa, opcional (nesse caso, não há) */


  xTaskCreate(
      task_monitor_serial        
      ,  (const portCHAR *)"monitor_serial"  
      ,  156                     
      ,  NULL                       
      ,  1                         
      ,  NULL );                    

  xTaskCreate(
      task_leds        
      ,  (const portCHAR *)"leds"  
      ,  156                     
      ,  NULL                       
      ,  1                         
      ,  NULL );                    



/* A partir deste momento, o scheduler de tarefas entra em ação e as tarefas executam */
}

void loop() {
    /* No FreeRTOS tudo é feito pelas tarefas */
}

/* task_dht11_temperatura: tarefa responsável por fazer a leitura da temperatura por meio do sensor dht11 */
void task_dht11_temperatura( void *pvParameters )
{

   float temperatura = 0.0;

   vTaskDelay( 2000 / portTICK_PERIOD_MS );

   while(1)
   {
      
      temperatura = dht.readTemperature();

      /* Insere leitura na fila */
      xQueueOverwrite(fila_dht11_temperatura, (void *)&temperatura);

      vTaskDelay( 2000 / portTICK_PERIOD_MS );  /* Ler a temperatura a cada 2 segundos*/
      
   }
   
}



/* task_monitor_serial: tarefa responsável por imprimir na tela os dados da fila (sensor dht11) */
void task_monitor_serial( void *pvParameters )
{
   float temperatura = 0.0;
   while(1)
   {
      xQueuePeek(fila_dht11_temperatura, &temperatura,TEMPO_PARA_AGUARDAR_FILA);    
      Serial.print(F("Temperatura: "));
      Serial.print(temperatura);
      Serial.println(F("°C "));
      vTaskDelay( 2000 / portTICK_PERIOD_MS ); /* Exibir a temperatura a cada 2 segundos*/
   } 
}





/* task_leds: tarefa responsável por acender os leds conforme a condição atual */
/* dados considerando a levedura Lalvin 71B */
/* https://www.indupropil.com.br/levedura-lalvin-71b-5gr.html */
void task_leds( void *pvParameters )
{
   float temperatura = 0.0;
   while(1)
   {
      xQueuePeek(fila_dht11_temperatura, &temperatura,TEMPO_PARA_AGUARDAR_FILA);    

/* led vermelho = temperatura crítica, manutenção do acionamento dos sistemas de refrigeração ou aquecimento */
      if(temperatura <= 15.0 ){
        digitalWrite(LED_VERMELHO, HIGH);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERDE, LOW);

      }

/* led amarelo = temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento */
      if(temperatura > 15.0 && temperatura <= 17.0) {
        digitalWrite(LED_VERMELHO, LOW);
        digitalWrite(LED_AMARELO, HIGH);
        digitalWrite(LED_VERDE, LOW);
      }

/* led verde = dentro da faixa de temperatura desejada */
      if(temperatura > 17.0 && temperatura < 28.0) {
        digitalWrite(LED_VERMELHO, LOW);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERDE, HIGH);
      }

/* led amarelo = temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento */
      if(temperatura >= 28.0 && temperatura <= 17.0) {
        digitalWrite(LED_VERMELHO, LOW);
        digitalWrite(LED_AMARELO, HIGH);
        digitalWrite(LED_VERDE, LOW);
      }


/* led vermelho = temperatura crítica, manutenção do acionamento dos sistemas de refrigeração ou aquecimento */
      if(temperatura >= 30.0){
        digitalWrite(LED_VERMELHO, HIGH);
        digitalWrite(LED_AMARELO, LOW);
        digitalWrite(LED_VERDE,LOW );
      } 
          vTaskDelay( 2000 / portTICK_PERIOD_MS ); /* Exibir a temperatura a cada 2 segundos*/

   }
}
