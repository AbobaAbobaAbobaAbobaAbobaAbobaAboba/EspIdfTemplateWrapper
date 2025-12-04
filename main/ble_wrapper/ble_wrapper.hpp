

#ifndef BLE_WRAPPER_HPP
#define BLE_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
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
/* BLE */

#include "console/console.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

static uint8_t own_addr_type;
static constexpr uint16_t GATT_SVR_SVC_ALERT_UUID = 0x0000;

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

  private:
    inline static uint8_t gatt_svr_dsc_val = 0x99;
    inline static char dsc_val[] = "Hello abobicus!";
    void initGatt();
    void init();
    static void bleprph_host_task(void* param) {
        esp_log(kInfoLogConfig, TAG, "BLE Host Task Started");
        /* This function will return only when nimble_port_stop() is executed */
        nimble_port_run();

        nimble_port_freertos_deinit();
    }
    static int gatt_svr_write(
      struct os_mbuf* om,
      uint16_t min_len,
      uint16_t max_len,
      void* dst,
      uint16_t* len) {
        uint16_t om_len;
        int rc;

        om_len = OS_MBUF_PKTLEN(om);
        if (om_len < min_len || om_len > max_len) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
        if (rc != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        return 0;
    }

    static int gatt_svc_access(
      uint16_t conn_handle,
      uint16_t attr_handle,
      struct ble_gatt_access_ctxt* ctxt,
      void* arg) {
        const ble_uuid_t* uuid;
        int rc;

        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Characteristic read; conn_handle=%d attr_handle=%d\n",
                      conn_handle,
                      attr_handle);
                } else {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Characteristic read by NimBLE stack; attr_handle=%d\n",
                      attr_handle);
                }
                uuid = ctxt->chr->uuid;
                if (attr_handle == gatt_svr_chr_val_handle) {
                    rc = os_mbuf_append(
                      ctxt->om,
                      &gatt_svr_chr_val,
                      sizeof(gatt_svr_chr_val));
                    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
                }
                break;

            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Characteristic write; conn_handle=%d attr_handle=%d",
                      conn_handle,
                      attr_handle);
                } else {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Characteristic write by NimBLE stack; attr_handle=%d",
                      attr_handle);
                }
                uuid = ctxt->chr->uuid;
                if (attr_handle == gatt_svr_chr_val_handle) {
                    rc = gatt_svr_write(
                      ctxt->om,
                      sizeof(gatt_svr_chr_val),
                      sizeof(gatt_svr_chr_val),
                      &gatt_svr_chr_val,
                      NULL);
                    ble_gatts_chr_updated(attr_handle);
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Notification/Indication scheduled for "
                      "all subscribed peers.\n");
                    return rc;
                }
                break;

            case BLE_GATT_ACCESS_OP_READ_DSC:
                if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Descriptor read; conn_handle=%d attr_handle=%d\n",
                      conn_handle,
                      attr_handle);
                } else {
                    esp_log(
                      kInfoLogConfig,
                      TAG,
                      "Descriptor read by NimBLE stack; attr_handle=%d\n",
                      attr_handle);
                }
                uuid = ctxt->dsc->uuid;
                if (ble_uuid_cmp(uuid, &gatt_svr_dsc_uuid.u) == 0) {
                    rc = os_mbuf_append(ctxt->om, dsc_val, sizeof(dsc_val));
                    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
                }
                break;
            default:
                break;
        }

        throw std::runtime_error("Forbidden behaviour");
        return BLE_ATT_ERR_UNLIKELY;
    }

    inline static const ble_uuid128_t gatt_svr_chr_uuid = BLE_UUID128_INIT(
      0x00,
      0x00,
      0x00,
      0x00,
      0x11,
      0x11,
      0x11,
      0x11,
      0x22,
      0x22,
      0x22,
      0x22,
      0x33,
      0x33,
      0x33,
      0x33);
    inline static constexpr ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(
      0x2d,
      0x71,
      0xa2,
      0x59,
      0xb4,
      0x58,
      0xc8,
      0x12,
      0x99,
      0x99,
      0x43,
      0x95,
      0x12,
      0x2f,
      0x46,
      0x59);

    inline static constexpr ble_uuid128_t gatt_svr_dsc_uuid = BLE_UUID128_INIT(
      0x01,
      0x01,
      0x01,
      0x01,
      0x12,
      0x12,
      0x12,
      0x12,
      0x23,
      0x23,
      0x23,
      0x23,
      0x34,
      0x34,
      0x34,
      0x34);

    inline static uint8_t gatt_svr_chr_val = {};
    inline static uint16_t gatt_svr_chr_val_handle = {};

    inline static const struct ble_gatt_chr_def gatt_chrs[] = {
      {.uuid = &gatt_svr_chr_uuid.u,
       .access_cb = gatt_svc_access,
       .arg = nullptr,
       .descriptors = {0},
       .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,  // Permissions
       .min_key_size = 0,
       .val_handle = &gatt_svr_chr_val_handle,
       .cpfd = nullptr},
      {nullptr,
       nullptr,
       nullptr,
       nullptr,
       0,
       0,
       nullptr,
       nullptr}  // End of characteristics (full init)
    };

    inline static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
      {
        /*** Service ***/
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .includes = nullptr,
        .characteristics = gatt_chrs,
      },
      {0, nullptr, nullptr, nullptr}  // Terminator (full init)
    };

    struct ServicesWrapper {
        static constexpr int kMaxServices = 4;
        std::array<ble_gatt_svc_def, kMaxServices> services_ = {
          0,
          nullptr,
          nullptr,
          nullptr};
        // aboba continue here

        // implement BLE service, char and descriptor registration
        // refactor read/write/notfiy
        // size_t current_size =
    }

    inline static uint8_t own_addr_type;
    inline static constexpr TagType TAG = "BLE_WRAPPER";
    inline static BluetoothWrapper* instance = nullptr;
};
#endif