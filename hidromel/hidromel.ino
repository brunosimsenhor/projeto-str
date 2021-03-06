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

/* Definições Resfriamento e Aquecimento*/
#define AQUECIMENTO 6
#define RESFRIAMENTO 7

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
  /* Inicializar serial */
  Serial.begin(BAUDRATE_SERIAL);

  /* Saidas dos LEDs */
//  pinMode(LED_VERMELHO, OUTPUT);
//  pinMode(LED_AZUL, OUTPUT);
//  pinMode(LED_VERDE, OUTPUT);
//  pinMode(LED_VERDE, OUTPUT);
//  pinMode(LED_VERDE, OUTPUT);

  /* Inicializar dht11 */
  dht.begin();

  /* Criação das filas (queues) */ 
  fila_dht11_temperatura  = xQueueCreate(1, sizeof(float));
  fila_led_azul           = xQueueCreate(1, sizeof(int));
  fila_led_verde          = xQueueCreate(1, sizeof(int));
  fila_led_vermelho       = xQueueCreate(1, sizeof(int));
  fila_aquecimento        = xQueueCreate(1, sizeof(int));
  fila_resfriamento       = xQueueCreate(1, sizeof(int));

  /* Verifica se as filas foram corretamente criadas */
  if ((fila_dht11_temperatura == NULL || fila_led_azul == NULL || fila_led_verde == NULL || fila_led_vermelho == NULL || fila_aquecimento == NULL || fila_resfriamento == NULL)) {
    Serial.println("erro nas filas");
    while(1){}
  }

  /* Criar tarefas */
  xTaskCreate(task_dht11_temperatura, (const portCHAR *)"dht11",            1024, NULL,  5, NULL);
  xTaskCreate(task_controle,          (const portCHAR *)"monitor_serial",   156,  NULL,  4,  NULL);
  xTaskCreate(task_led_azul,          (const portCHAR *)"led_azul",         156,  NULL,  2,  NULL);
  xTaskCreate(task_led_verde,         (const portCHAR *)"led_verde",        156,  NULL,  1,  NULL);
  xTaskCreate(task_led_vermelho,      (const portCHAR *)"led_vermelho",     156,  NULL,  2,  NULL);
  xTaskCreate(task_aquecimento,       (const portCHAR *)"led_aquecimento",  156,  NULL,  3,  NULL);
  xTaskCreate(task_resfriamento,      (const portCHAR *)"led_resfriamento", 156,  NULL,  3,  NULL);

  Serial.println("setup finalizado");
  /* A partir deste momento, o scheduler de tarefas entra em ação e as tarefas executam */
}

void loop() {
    /* No FreeRTOS tudo é feito pelas tarefas */
}

void switch_led(int azul, int verde, int vermelho) {
  xQueueOverwrite(fila_led_azul, (void *)&azul);
  xQueueOverwrite(fila_led_verde, (void *)&verde);
  xQueueOverwrite(fila_led_vermelho, (void *)&vermelho);
}


void switch_aquecimento(int flag) {
  xQueueOverwrite(fila_aquecimento, (void *)&flag);
}

void switch_resfriamento(int flag) {
  xQueueOverwrite(fila_resfriamento, (void *)&flag);
}


/* TAREFA DE SENSOR */
/* task_dht11_temperatura: tarefa responsável por fazer a leitura da temperatura por meio do sensor dht11 */
void task_dht11_temperatura( void *pvParameters )
{
  float temperatura = 0.0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );

  while(1) {
    temperatura = dht.readTemperature();
    /* Insere leitura na fila */
    xQueueOverwrite(fila_dht11_temperatura, (void *)&temperatura);
    vTaskDelay( 2000 / portTICK_PERIOD_MS );  /* Ler a temperatura a cada 2 segundos*/
    
    Serial.print("TEMPERATURA ATUAL: ");
    Serial.println(temperatura);
  }
}


/* TAREFA DE CONTROLE */
/* dados considerando a levedura Lalvin 71B */
/* https://www.indupropil.com.br/levedura-lalvin-71b-5gr.html */

void task_controle( void *pvParameters )
{
  float temperatura = 0.0;

  vTaskDelay( 2000 / portTICK_PERIOD_MS );

  while(1) {
    xQueuePeek(fila_dht11_temperatura, &temperatura,TEMPO_PARA_AGUARDAR_FILA);    
    
    /* Temperatura críticamente baixa => acionamento dos sistemas de refrigeração ou aquecimento */
    if(temperatura < 15.0 ){
      Serial.println("Temperatura críticamente baixa");
      switch_led(1, 0, 0);   
      switch_aquecimento(1);
      switch_resfriamento(0);  
    
    /* Temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento */
    } else if (temperatura >= 15.0 && temperatura <= 17.0) {
      Serial.println("Temperatura de atenção - baixa");
      switch_led(1, 1, 0);
      switch_aquecimento(1);
      switch_resfriamento(0);        
    
    /* Dentro da faixa de temperatura desejada */
    } else if (temperatura > 17.0 && temperatura < 28.0) {
      Serial.println("Temperatura desejada");        
      switch_led(0, 1, 0); 
      switch_aquecimento(0);
      switch_resfriamento(0);       
    
    /* Temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento */
    } else if (temperatura >= 28.0 && temperatura <= 30.0) {
      Serial.println("Temperatura de atenção - alta");
      switch_led(0, 1, 1);
      switch_aquecimento(0);
      switch_resfriamento(1);        
    
    /* led vermelho = temperatura crítica, manutenção do acionamento dos sistemas de refrigeração ou aquecimento */
    } else if (temperatura > 30.0){
      Serial.println("Temperatura críticamente alta");        
      switch_led(0, 0, 1);   
      switch_aquecimento(0);
      switch_resfriamento(1);
    
    /* desconhecido */
    } else {
      Serial.println("Estado desconhecido");
    }

    /* Ler a temperatura a cada 2 segundos */
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}   

/* TAREFAS DE ATUAÇÃO */
void task_led_azul( void *pvParameters )
{
  int azul = 0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );
  
  while(1) {    
    /* Recupera valor da  fila */
    xQueuePeek(fila_led_azul, &azul,TEMPO_PARA_AGUARDAR_FILA);
    Serial.println("AZUL =>");
    Serial.println(azul);

    digitalWrite(LED_AZUL, azul == 1 ? HIGH : LOW);

    /* Ler a temperatura a cada 2 segundos*/
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}


void task_led_verde( void *pvParameters )
{
  int verde = 0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );

  while(1) {    
    /* Recupera valor da  fila */
    xQueuePeek(fila_led_verde, &verde,TEMPO_PARA_AGUARDAR_FILA);    
    Serial.println("VERDE =>");
    Serial.println(verde);
    
    digitalWrite(LED_VERDE, verde == 1 ? HIGH : LOW);

    /* Ler a temperatura a cada 2 segundos*/
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}


void task_led_vermelho( void *pvParameters )
{
  int vermelho = 0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );
  
  while(1) {    
    /* Recupera valor da  fila */
    xQueuePeek(fila_led_vermelho, &vermelho,TEMPO_PARA_AGUARDAR_FILA);
    Serial.println("VERMELHO =>");
    Serial.println(vermelho);
    
    digitalWrite(LED_VERMELHO, vermelho == 1 ? HIGH : LOW);

    /* Ler a temperatura a cada 2 segundos */
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}




void task_resfriamento( void *pvParameters )
{
  int resfriamento = 0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );

  while(1) {
    /* Recupera valor da  fila */
    xQueuePeek(fila_resfriamento, &resfriamento,TEMPO_PARA_AGUARDAR_FILA);
    Serial.println("RESFRIAMENTO =>");
    Serial.println(resfriamento);

    digitalWrite(RESFRIAMENTO, resfriamento == 1 ? HIGH : LOW);
    
    /* Ler a temperatura a cada 2 segundos*/
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}


void task_aquecimento( void *pvParameters )
{
  int aquecimento = 0;
  vTaskDelay( 2000 / portTICK_PERIOD_MS );

  while(1) {
    /* Recupera valor da  fila */
    xQueuePeek(fila_aquecimento, &aquecimento,TEMPO_PARA_AGUARDAR_FILA);
    Serial.println("AQUECIMENTO =>");
    Serial.println(aquecimento);

    digitalWrite(AQUECIMENTO, aquecimento == 1 ? HIGH : LOW);

    /* Ler a temperatura a cada 2 segundos*/
    vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }
}
