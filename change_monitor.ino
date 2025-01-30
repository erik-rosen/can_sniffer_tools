#include "variant.h"
#include <due_can.h>

#define Serial SerialUSB
#define MAX_CAN_IDS 256  // Maximum unique CAN IDs to track

const unsigned long BASELINE_DURATION = 30000; // 30 seconds
const unsigned long MODIFICATION_DURATION = 10000; // 10 seconds
unsigned long startTime;

struct CAN_ID_Data {
    uint32_t id;
    uint8_t baseline_mask[8];
    uint8_t modification_mask[8];
    uint8_t prev_value[8];
    bool active;
    bool initialized;
};

CAN_ID_Data can_data_list[MAX_CAN_IDS];
int can_data_count = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Can0.begin(CAN_BPS_125K);
    Can1.begin(CAN_BPS_125K);
    
    for (int filter = 0; filter < 3; filter++) {
        Can0.setRXFilter(filter, 0, 0, true);
        Can1.setRXFilter(filter, 0, 0, true);
    }
    for (int filter = 3; filter < 7; filter++) {
        Can0.setRXFilter(filter, 0, 0, false);
        Can1.setRXFilter(filter, 0, 0, false);
    }
    
    startTime = millis();
}

void loop() {
    static bool baselinePrinted = false;
    static bool modificationPrinted = false;
    static bool monitoringPrinted = false;
    unsigned long currentTime = millis();
    CAN_FRAME frame;

    if (!baselinePrinted){
      Serial.print("Recording baseline!");
      Serial.println();
      baselinePrinted = true;
    }

    if (((currentTime - startTime) > BASELINE_DURATION ) && !modificationPrinted){
      Serial.print("Entering modification phase");
      Serial.println();
      modificationPrinted = true;
    }

    if (((currentTime - startTime) > (BASELINE_DURATION + MODIFICATION_DURATION)) && !modificationPrinted){
      Serial.print("Modification masks:");
      Serial.println();
      printModificationMasks();
      Serial.print("Entering monitoring phase");
      Serial.println();
      monitoringPrinted = true;
    }
  
    if (Can0.available() > 0) {
        Can0.read(frame);
        processFrame(frame, currentTime);
    }
    if (Can1.available() > 0) {
        Can1.read(frame);
        processFrame(frame, currentTime);
    }
}

CAN_ID_Data* findOrCreateCANData(uint32_t id) {
    for (int i = 0; i < can_data_count; i++) {
        if (can_data_list[i].id == id) {
            return &can_data_list[i];
        }
    }
    if (can_data_count < MAX_CAN_IDS) {
        can_data_list[can_data_count].id = id;
        memset(can_data_list[can_data_count].baseline_mask, 0x00, 8);
        memset(can_data_list[can_data_count].modification_mask, 0x00, 8);
        memset(can_data_list[can_data_count].prev_value, 0x00, 8);
        can_data_list[can_data_count].active = true;
        can_data_list[can_data_count].initialized = false;
        return &can_data_list[can_data_count++];
    }
    return nullptr;
}

void processFrame(CAN_FRAME &frame, unsigned long currentTime) {
    CAN_ID_Data* data = findOrCreateCANData(frame.id);
    if (!data) return;

    if (currentTime - startTime < BASELINE_DURATION) {
        if(!data->initialized){ //Sets previous value to the current value if it is the first message
          for (int i = 0; i < 8; i++) {
            data->prev_value[i] = frame.data.bytes[i];
          }
          data->initialized = true;
          return;
        }
        for (int i = 0; i < 8; i++) {
            data->baseline_mask[i] |= (data->prev_value[i] ^ frame.data.bytes[i]);
            data->prev_value[i] = frame.data.bytes[i];
        }
    } else if (currentTime - startTime < BASELINE_DURATION + MODIFICATION_DURATION) {
        for (int i = 0; i < 8; i++) {
            uint8_t maskedValue = frame.data.bytes[i] & ~data->baseline_mask[i];
            data->modification_mask[i] |= maskedValue ^ (data->prev_value[i] & ~data->baseline_mask[i]);
            data->prev_value[i] = maskedValue;
        }
    } else {
        for (int i = 0; i < 8; i++) {
            uint8_t maskedValue = frame.data.bytes[i] & ~data->baseline_mask[i];
            if ((maskedValue ^ (data->prev_value[i] & ~data->baseline_mask[i])) & data->modification_mask[i]) {
                for (int bit = 0; bit < 8; bit++) {
                    if ((maskedValue & (1 << bit)) != (data->prev_value[i] & (1 << bit))) {
                        Serial.print("ID: 0x"); Serial.print(frame.id, HEX);
                        Serial.print(" Byte["); Serial.print(i);
                        Serial.print("] Bit["); Serial.print(bit);
                        Serial.print("] changed: "); Serial.println((maskedValue >> bit) & 1);
                    }
                }
            }
            data->prev_value[i] = maskedValue;
        }
    }
}

void printModificationMasks() {
    Serial.println("Modification Phase Summary:");
    for (int i = 0; i < can_data_count; i++) {
        bool modified = false;
        for (int x = 0; x < 8; x++){
          if (can_data_list[i].modification_mask[x] > 0){
            modified = true;
            break;
          } 
        } 
        if(!modified){continue;}
        Serial.print("ID: 0x"); Serial.print(can_data_list[i].id, HEX);
        Serial.print(" Changed Bits: ");
        for (int j = 0; j < 8; j++) {
            if (can_data_list[i].modification_mask[j] != 0) {
                Serial.print(" Byte["); Serial.print(j);
                Serial.print("] Bits: ");
                for (int b = 0; b < 8; b++) {
                    if (can_data_list[i].modification_mask[j] & (1 << b)) {
                        Serial.print(b);
                        Serial.print(" ");
                    }
                }
            }
        }
        Serial.println();
    }
}
