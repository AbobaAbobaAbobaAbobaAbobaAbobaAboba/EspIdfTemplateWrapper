

#ifndef BLE_WRAPPER_HPP
#define BLE_WRAPPER_HPP
#include "../common/common.hpp"
#include "console/console.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "services/ans/ble_svc_ans.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <functional>
#include <unordered_set>
static constexpr uint16_t GATT_SVR_SVC_ALERT_UUID = 0x0000;
typedef std::optional<std::vector<uint8_t>> (*CharacteristicCallback)(void*);
class BluetoothWrapper {
  public:
    BluetoothWrapper();
    BluetoothWrapper(const BluetoothWrapper& other) = delete;
    BluetoothWrapper(BluetoothWrapper&& other) = delete;

    BluetoothWrapper& operator=(const BluetoothWrapper& other) = delete;
    BluetoothWrapper& operator=(BluetoothWrapper&& other) = delete;
    ~BluetoothWrapper();

    static void printAddress(const uint8_t* addr_val);

    static void onSync();

    static void advertise(void);

    static void printConnectionInfo(struct ble_gap_conn_desc* desc);
    static int onGapEvent(struct ble_gap_event* event, void* arg);
    static void onReset(int reason);
    static void gattRegister(struct ble_gatt_register_ctxt* ctxt, void* arg);

    void start() { init(); }
    void setService(size_t index, uint8_t type, const ble_uuid128_t& uuid) {
        services_.set(index, type, uuid);
    }
    void resetService(size_t index) { services_.reset(index); }
    void setCharacteristic(
      size_t svc_index,
      size_t char_index,
      const ble_uuid128_t& uuid,
      ble_gatt_chr_flags flags,
      const CharacteristicCallback& callback) {
        services_.char_wrappers_[svc_index].set(char_index, uuid, flags, callback);
    }
    void resetCharacteristic(size_t svc_index, size_t char_index) {
        services_.char_wrappers_[svc_index].reset(char_index);
    }
    void notifyAll(size_t service_idx, size_t char_idx, const void* data, size_t len) {
        for (auto it = clients_.begin(), ie = clients_.end(); it != ie; ++it) {
            services_.char_wrappers_[service_idx].notify(char_idx, *it, data, len);
        }
    }

  private:
    void initGatt();
    void init();
    static void hostTask(void* param);
    static int gattWrite(
      struct os_mbuf* om,
      uint16_t min_len,
      uint16_t max_len,
      void* dst,
      uint16_t* len);

    static int gattAccess(
      uint16_t conn_handle,
      uint16_t attr_handle,
      struct ble_gatt_access_ctxt* ctxt,
      void* arg);

    inline static constexpr int kMaxServices = 2;

    class CharacteristicsWrapper {
      public:
        static constexpr size_t kMaxCharacteristics = 2;

        void set(
          size_t index,
          const ble_uuid128_t& uuid,
          ble_gatt_chr_flags flags,
          CharacteristicCallback callback);
        void reset(size_t index);
        void notify(size_t index, uint16_t conn_handle, const void* data, size_t len) {
            if (conn_handle == BLE_HS_CONN_HANDLE_NONE) return;

            struct os_mbuf* om = ble_hs_mbuf_from_flat(data, len);
            if (!om) return;

            int rc = ble_gattc_notify_custom(conn_handle, handles_[index], om);
            if (rc != 0) {
                esp_log(kInfoLogConfig, TAG, "Notify failed: %d", rc);
            }
        }
        constexpr ble_gatt_chr_def& get() { return std::get<0>(characteristics_); }

      private:
        friend class BluetoothWrapper;
        friend class ServicesDescriptor;
        std::array<ble_gatt_chr_def, kMaxCharacteristics + 1> characteristics_ = {};
        std::array<ble_uuid128_t, kMaxCharacteristics> uuids_ = {};
        std::array<uint16_t, kMaxCharacteristics> handles_ = {};
    };
    class ServicesDescriptor {
      public:
        void set(size_t index, uint8_t type, const ble_uuid128_t& uuid);
        void reset(size_t index);
        constexpr ble_gatt_svc_def& get() { return std::get<0>(services_); }

      private:
        friend class BluetoothWrapper;
        std::array<ble_gatt_svc_def, kMaxServices + 1> services_ = {};
        std::array<ble_uuid128_t, kMaxServices> uuids_ = {};
        std::array<CharacteristicsWrapper, kMaxServices> char_wrappers_ = {};
    };
    ServicesDescriptor services_;
    std::mutex mutex_;
    std::unordered_set<uint16_t> clients_;
    inline static uint8_t own_addr_type;
    inline static constexpr TagType TAG = "BLE_WRAPPER";
    inline static BluetoothWrapper* instance = nullptr;
};

;
#endif