/**
 * ESP32 Mesh Node — ESPLab
 * Libraries:
 *   - painlessMesh by Coopdis et al. (Library Manager: "painlessMesh")
 *   - AsyncTCP by esphome/ESPAsync (Library Manager: "AsyncTCP" for ESP32)
 *
 * Mesh config:
 *   prefix:   "ESPLab"
 *   password: "engr33223"
 *   port:     5555
 */

#include <Arduino.h>
#include <painlessMesh.h>
#ifdef ESP32
#include <AsyncTCP.h> // painlessMesh uses this under the hood on ESP32
#endif

// ----- Mesh config -----
#define MESH_PREFIX "ESPLab"
#define MESH_PASSWORD "engr33223"
#define MESH_PORT 5555

// ----- Globals -----
painlessMesh mesh;
Scheduler userSched; // (TaskScheduler is bundled with painlessMesh)

// forward decl
void sendMessage();

// every 5 seconds, broadcast our status
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

// ----- Callbacks -----
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("[RX] from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[NET] New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("[NET] Topology changed. Nodes: %s\n",
                mesh.subConnectionJson().c_str());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("[TIME] Adjusted by %d ms\n", offset);
}

// ----- Periodic broadcast -----
void sendMessage() {
  String msg = String("Hello from Faith, Isabel and Joe");
  mesh.sendBroadcast(msg);
  Serial.printf("[TX] %s\n", msg.c_str());
}

// ----- Setup / Loop -----
void setup() {
  Serial.begin(9600);
  delay(200);

  // Optional: enable some logs during bring-up
  // mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // Initialize mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userSched, MESH_PORT);

  // Register callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Schedule our periodic broadcast
  userSched.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.printf("[BOOT] Node %u up, joining mesh \"%s\" on port %d...\n",
                mesh.getNodeId(), MESH_PREFIX, MESH_PORT);
}

void loop() {
  // Keep mesh + scheduler updated
  mesh.update();
}
