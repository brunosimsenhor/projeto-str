// Para resfriar considerar Chiller de serpentina

// Incluir biblitoecas
#include <Arduino_FreeRTOS.h>
#include "DHT.h"
#include <queue.h>
#include <timers.h>

// Definições gerais
#define BAUDRATE_SERIAL                   9600
#define config_USE_TIMERS                 1
#define configSUPPORT_DYNAMIC_ALLOCATION  1

// Define a quantidade de ticks
#define WAITING_QUEUE_TIME        (TickType_t) 10
#define AQUISITION_TIMER_IN_TICKS pdMS_TO_TICKS(500)
#define CONTROL_TIMER_IN_TICKS    pdMS_TO_TICKS(1000)
#define ACTUATOR_TIMER_IN_TICKS   pdMS_TO_TICKS(1000)

// Definições - DHT11
#define DHTPIN  2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

// Definições Leds
#define LED_AZUL 3
#define LED_VERDE 4
#define LED_VERMELHO 5

// Definições Resfriamento e Aquecimento
#define AQUECIMENTO 6
#define RESFRIAMENTO 7

// Filas
QueueHandle_t fila_dht11_temperatura;
QueueHandle_t fila_led_azul;
QueueHandle_t fila_led_verde;
QueueHandle_t fila_led_vermelho;
QueueHandle_t fila_aquecimento;
QueueHandle_t fila_resfriamento;

// Tarefas de aquisição de dados
void task_dht11_temperatura();

// Tarefas de controle
void task_controle();

// Tarefas de atuação
// Tarefas dos leds
void task_led_azul();
void task_led_vermelho();
void task_led_verde();
// Tarefa de aquecimento
void task_aquecimento();
// Tarefa de resfriamento
void task_resfriamento();

// Timer de aquisição de dados
TimerHandle_t dht11_timer;
// Timer de controle
TimerHandle_t controle_timer;
// Timers de atuação
TimerHandle_t led_azul_timer, led_verde_timer, led_vermelho_timer, aquecimento_timer, resfriamento_timer;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Inicializar serial
  Serial.begin(BAUDRATE_SERIAL);
  Serial.println("setup...");

  // Inicializar dht11
  dht.begin();

  // Saidas dos LEDs
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(AQUECIMENTO, OUTPUT);
  pinMode(RESFRIAMENTO, OUTPUT);

  // Criação das filas (queues)
  fila_dht11_temperatura  = xQueueCreate(1, sizeof(float));
  fila_led_azul           = xQueueCreate(1, sizeof(int));
  fila_led_verde          = xQueueCreate(1, sizeof(int));
  fila_led_vermelho       = xQueueCreate(1, sizeof(int));
  fila_aquecimento        = xQueueCreate(1, sizeof(int));
  fila_resfriamento       = xQueueCreate(1, sizeof(int));

  // Verifica se as filas foram corretamente criadas
  if (fila_dht11_temperatura == NULL ||
      fila_led_azul == NULL ||
      fila_led_verde == NULL ||
      fila_led_vermelho == NULL ||
      fila_aquecimento == NULL ||
      fila_resfriamento == NULL) {
      Serial.println("A fila nao foi criada.");
      Serial.println("O programa nao pode continuar");

      while(1) {}
  }

  // Tarefas de aquisição de dados
  dht11_timer         = xTimerCreate("dht11",         AQUISITION_TIMER_IN_TICKS,  pdTRUE, (void *) 0, task_dht11_temperatura);

  // Tarefas de controle
  controle_timer      = xTimerCreate("controle",      CONTROL_TIMER_IN_TICKS,     pdTRUE, (void *) 0, task_controle);

  // Tarefas de atuação
  led_azul_timer      = xTimerCreate("led_azul",      ACTUATOR_TIMER_IN_TICKS,    pdTRUE, (void *) 0, task_led_azul);
  led_verde_timer     = xTimerCreate("led_verde",     ACTUATOR_TIMER_IN_TICKS,    pdTRUE, (void *) 0, task_led_verde);
  led_vermelho_timer  = xTimerCreate("led_vermelho",  ACTUATOR_TIMER_IN_TICKS,    pdTRUE, (void *) 0, task_led_vermelho);
  aquecimento_timer   = xTimerCreate("aquecimento",   ACTUATOR_TIMER_IN_TICKS,    pdTRUE, (void *) 0, task_aquecimento);
  resfriamento_timer  = xTimerCreate("resfriamento",  ACTUATOR_TIMER_IN_TICKS,    pdTRUE, (void *) 0, task_resfriamento);

  // Verificando timers
  if (dht11_timer == NULL) {
    Serial.println("nao criou os timers de aquisicao");
    Serial.println("O programa nao pode continuar");

    while (1) {}
  }

  // Verificando timers
  if (controle_timer == NULL ||
      led_azul_timer == NULL) {
    Serial.println("nao criou os timers de controle");
    Serial.println("O programa nao pode continuar");

    while (1) {}
  }

  
  // Verificando timers
  if (led_azul_timer == NULL ||
      led_verde_timer == NULL ||
      led_vermelho_timer == NULL ||
      aquecimento_timer == NULL ||
      resfriamento_timer == NULL) {
    Serial.println("nao criou os timers de atuacao");
    Serial.println("O programa nao pode continuar");

    while (1) {}
  }

  // Iniciando os timers
  if (xTimerStart(dht11_timer, 0) != pdPASS ||
      xTimerStart(controle_timer, 0) != pdPASS ||
      xTimerStart(led_azul_timer, 0) != pdPASS ||
      xTimerStart(led_verde_timer, 0) != pdPASS ||
      xTimerStart(led_vermelho_timer, 0) != pdPASS ||
      xTimerStart(aquecimento_timer, 0) != pdPASS ||
      xTimerStart(resfriamento_timer, 0) != pdPASS) {
    Serial.println("nao startou os timers");
    Serial.println("O programa nao pode continuar");

    while (1) {}
  }

  Serial.println("comecando...");

  // A partir deste momento, o scheduler de tarefas entra em ação e as tarefas executam
}

void loop() {
  // No FreeRTOS tudo é feito pelas tarefas
}

// função para escrever nas filas dos LEDs
void switch_led(int azul, int verde, int vermelho) {
  xQueueOverwrite(fila_led_azul, (void *)&azul);
  xQueueOverwrite(fila_led_verde, (void *)&verde);
  xQueueOverwrite(fila_led_vermelho, (void *)&vermelho);
}

// função para escrever na fila do aquecimento
void switch_aquecimento(int flag) {
  xQueueOverwrite(fila_aquecimento, (void *)&flag);
}

// função para escrever na fila do resfriamento
void switch_resfriamento(int flag) {
  xQueueOverwrite(fila_resfriamento, (void *)&flag);
}

// Tarefas de aquisição de dados
// task_dht11_temperatura: tarefa responsável por fazer a leitura da temperatura por meio do sensor dht11
void task_dht11_temperatura(TimerHandle_t xTimer) {
  Serial.println("temperature gathering task is starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_temperatura;

  float temperatura = dht.readTemperature();

  // Insere leitura na fila */
  xQueueOverwrite(fila_dht11_temperatura, (void *)&temperatura);

  Serial.print("Temperatura atual: ");
  Serial.println(temperatura);

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_temperatura = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da task_dht11_temperatura:");
//  Serial.println(valor_high_water_mark_task_temperatura);
}


// Tarefa de controle
// dados considerando a levedura Lalvin 71B
// https://www.indupropil.com.br/levedura-lalvin-71b-5gr.html
void task_controle(TimerHandle_t xTimer) {
  Serial.println("control task starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_controle;
  float temperatura;

  xQueuePeek(fila_dht11_temperatura, &temperatura, WAITING_QUEUE_TIME);    

  Serial.print("Temperatura: ");
  Serial.println(temperatura);

  // Temperatura críticamente baixa => acionamento dos sistemas de refrigeração ou aquecimento
  if (temperatura < 15.0) {
    Serial.println("Temperatura críticamente baixa");

    switch_led(1, 0, 0);

    switch_aquecimento(1);
    switch_resfriamento(0);

  // Temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento
  } else if (temperatura >= 15.0 && temperatura <= 17.0) {
    Serial.println("Temperatura de atenção - baixa");

    switch_led(1, 1, 0);

    switch_aquecimento(1);
    switch_resfriamento(0);

  // Dentro da faixa de temperatura desejada
  } else if (temperatura > 17.0 && temperatura < 28.0) {
    Serial.println("Temperatura desejada");

    switch_led(0, 1, 0);

    switch_aquecimento(0);
    switch_resfriamento(0);

  // Temperatura de atenção, acionamento do sistema de refrigeração ou aquecimento
  } else if (temperatura >= 28.0 && temperatura <= 30.0) {
    Serial.println("Temperatura de atenção - alta");

    switch_led(0, 1, 1);

    switch_aquecimento(0);
    switch_resfriamento(1);

  // led vermelho = temperatura crítica, manutenção do acionamento dos sistemas de refrigeração ou aquecimento
  } else if (temperatura > 30.0) {
    Serial.println("Temperatura críticamente alta");

    switch_led(0, 0, 1);

    switch_aquecimento(0);
    switch_resfriamento(1);
  } else {
    Serial.println("Unknown state");
  }

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_controle = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_controle:");
//  Serial.println(valor_high_water_mark_task_controle);
}

// Tarefas de atuação
void task_led_azul(TimerHandle_t xTimer)
{
  Serial.println("blue led task starting");

  // Variavel para recuperar o High Water Mark da task */
//  UBaseType_t valor_high_water_mark_task_led_azul;
  int azul;

  // Recupera valor da  fila
  xQueuePeek(fila_led_azul, &azul, WAITING_QUEUE_TIME);

  digitalWrite(LED_AZUL, azul == 1 ? HIGH : LOW);

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_led_azul = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_led_azul:");
//  Serial.println(valor_high_water_mark_task_led_azul);
}

void task_led_verde(TimerHandle_t xTimer)
{
  Serial.println("green led task starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_led_verde;
  int verde;

  // Recupera valor da  fila
  xQueuePeek(fila_led_verde, &verde, WAITING_QUEUE_TIME);

  digitalWrite(LED_VERDE, verde == 1 ? HIGH : LOW);

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_led_verde = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_led_verde:");
//  Serial.println(valor_high_water_mark_task_led_verde);
}


void task_led_vermelho(TimerHandle_t xTimer)
{
  Serial.println("red led task starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_led_vermelho;
  int vermelho;

  // Recupera valor da  fila
  xQueuePeek(fila_led_vermelho, &vermelho, WAITING_QUEUE_TIME);

  digitalWrite(LED_VERMELHO, vermelho == 1 ? HIGH : LOW);

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_led_vermelho = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_led_vermelho:");
//  Serial.println(valor_high_water_mark_task_led_vermelho);
}

void task_resfriamento(TimerHandle_t xTimer)
{
  Serial.println("cooling task starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_resfriamento;
  int resfriamento;

  // Recupera valor da  fila
  xQueuePeek(fila_resfriamento, &resfriamento, WAITING_QUEUE_TIME);    

  digitalWrite(RESFRIAMENTO, resfriamento == 1 ? HIGH : LOW);

  // Obtem o High Water Mark da task atuaL
//  valor_high_water_mark_task_resfriamento = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_resfriamento:");
//  Serial.println(valor_high_water_mark_task_resfriamento);
}

void task_aquecimento(TimerHandle_t xTimer)
{
  Serial.println("heating task starting");

  // Variavel para recuperar o High Water Mark da task
//  UBaseType_t valor_high_water_mark_task_aquecimento;
  int aquecimento;

  // Recupera valor da  fila
  xQueuePeek(fila_aquecimento, &aquecimento, WAITING_QUEUE_TIME);

  digitalWrite(AQUECIMENTO, aquecimento == 1 ? HIGH : LOW);

  // Obtem o High Water Mark da task atual
//  valor_high_water_mark_task_aquecimento = uxTaskGetStackHighWaterMark(NULL);
//  Serial.print("High water mark (words) da valor_high_water_mark_task_aquecimento:");
//  Serial.println(valor_high_water_mark_task_aquecimento);
}
