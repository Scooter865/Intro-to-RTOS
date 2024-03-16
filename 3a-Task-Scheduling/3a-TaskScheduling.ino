// The message to be printed and preempted
const char message[] = "The quick brown fox jumped over the lazy dog.";

// Handles for the 2 tasks
static TaskHandle_t task_h1 = NULL;
static TaskHandle_t task_h2 = NULL;

// Message printer task
void messagePrinter(void * param) {
  size_t msgLen = strlen(message);

  while (1) {
    Serial.println();
    for (size_t i = 0; i < msgLen; i++) {
      Serial.print(message[i]);
    }
    Serial.println();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Star printer task
void starPrinter(void * param) {
  while (1) {
    Serial.print('*');
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Star printer task

void setup() {
  Serial.begin(300);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  Serial.printf("\n-----Scheduling Example-----\n");

  xTaskCreate(messagePrinter, "Message Printer", 1024, NULL, 1, &task_h1);
  xTaskCreate(starPrinter, "Star Printer", 1024, NULL, 2, &task_h2);

}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskSuspend(task_h2);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  vTaskResume(task_h2);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

}
