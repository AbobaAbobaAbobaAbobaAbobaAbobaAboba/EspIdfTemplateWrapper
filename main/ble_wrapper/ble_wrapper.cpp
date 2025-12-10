#include "ble_wrapper.hpp"
#include "../common/global.hpp"
BluetoothWrapper::BluetoothWrapper() {
    if (instance != nullptr) {
        throw std::runtime_error("An instance of BluetoothWrapper already exists!");
    }
    instance = this;
}

BluetoothWrapper::~BluetoothWrapper() {
    if (instance) {
        nimble_port_freertos_deinit();
        nimble_port_deinit();
        instance = nullptr;
    }
}

void BluetoothWrapper::printAddress(const uint8_t* addr_val) {
    esp_log(
      kInfoLogConfig,
      TAG,
      "%02x:%02x:%02x:%02x:%02x:%02x:",
      addr_val[5],
      addr_val[4],
      addr_val[3],
      addr_val[2],
      addr_val[1],
      addr_val[0]);
}

void BluetoothWrapper::onSync() {
    if (ble_hs_util_ensure_addr(0) != 0) {
        throw std::runtime_error("Failed to ensure addr");
    }

    if (ble_hs_id_infer_auto(0, &own_addr_type) != 0) {
        throw std::runtime_error("Failed to infer auto!");
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    if (ble_hs_id_copy_addr(own_addr_type, addr_val, NULL) != 0) {
        throw std::runtime_error("Failed to copy addr!");
    }
    printAddress(addr_val);
    advertise();
}

void BluetoothWrapper::advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
#if CONFIG_BT_NIMBLE_GAP_SERVICE
    const char* name;
#endif

    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

#if CONFIG_BT_NIMBLE_GAP_SERVICE
    name = ble_svc_gap_device_name();
    fields.name = (uint8_t*)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
#endif

    fields.uuids16 = (ble_uuid16_t[]){BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    if (ble_gap_adv_set_fields(&fields) != 0) {
        throw std::runtime_error("Failed to set fields in adv data!");
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    if (
      ble_gap_adv_start(
        own_addr_type,
        NULL,
        BLE_HS_FOREVER,
        &adv_params,
        onGapEvent,
        NULL)
      != 0) {
        throw std::runtime_error("Failed to start adv !");
    }
}

void BluetoothWrapper::printConnectionInfo(struct ble_gap_conn_desc* desc) {
    esp_log(
      kInfoLogConfig,
      TAG,
      "handle=%d our_ota_addr_type=%d our_ota_addr=",
      desc->conn_handle,
      desc->our_ota_addr.type);
    printAddress(desc->our_ota_addr.val);

    esp_log(
      kInfoLogConfig,
      TAG,
      " our_id_addr_type=%d our_id_addr=",
      desc->our_id_addr.type);
    printAddress(desc->our_id_addr.val);

    esp_log(
      kInfoLogConfig,
      TAG,
      " peer_ota_addr_type=%d peer_ota_addr=",
      desc->peer_ota_addr.type);
    printAddress(desc->peer_ota_addr.val);
    esp_log(
      kInfoLogConfig,
      TAG,
      " peer_id_addr_type=%d peer_id_addr=",
      desc->peer_id_addr.type);

    printAddress(desc->peer_id_addr.val);
    esp_log(
      kInfoLogConfig,
      TAG,
      " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
      "encrypted=%d authenticated=%d bonded=%d\n",
      desc->conn_itvl,
      desc->conn_latency,
      desc->supervision_timeout,
      desc->sec_state.encrypted,
      desc->sec_state.authenticated,
      desc->sec_state.bonded);
}
int BluetoothWrapper::onGapEvent(struct ble_gap_event* event, void* arg) {
    ble_gap_conn_desc desc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed. */
            esp_log(
              kInfoLogConfig,
              TAG,
              "connection %s; status=%d ",
              event->connect.status == 0 ? "established" : "failed",
              event->connect.status);

            if (event->connect.status == 0) {
                if (ble_gap_conn_find(event->connect.conn_handle, &desc) != 0) {
                    esp_log(kWarnLogConfig, TAG, "Failed to find connection");
                } else {
                    std::lock_guard<std::mutex> guard(instance->mutex_);
                    instance->clients_.insert(event->connect.conn_handle);
                }
                printConnectionInfo(&desc);
            }

            if (event->connect.status != 0) {
                advertise();
            }

            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            esp_log(
              kInfoLogConfig,
              TAG,
              "disconnect; reason=%d ",
              event->disconnect.reason);
            printConnectionInfo(&event->disconnect.conn);
            {
                std::lock_guard<std::mutex> guard(instance->mutex_);
                instance->clients_.erase(event->connect.conn_handle);
            }
            advertise();
            return 0;
        case BLE_GAP_EVENT_CONN_UPDATE:
            /* The central has updated the connection parameters. */
            esp_log(
              kInfoLogConfig,
              TAG,
              "connection updated; status=%d ",
              event->conn_update.status);
            if (ble_gap_conn_find(event->conn_update.conn_handle, &desc) != 0) {
                throw std::runtime_error("Failed to ble_gap_conn_find");
            }
            printConnectionInfo(&desc);

            return 0;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            esp_log(
              kInfoLogConfig,
              TAG,
              "advertise complete; reason=%d",
              event->adv_complete.reason);

            advertise();
            return 0;
        case BLE_GAP_EVENT_NOTIFY_TX:
            esp_log(
              kInfoLogConfig,
              TAG,
              "notify_tx event; conn_handle=%d attr_handle=%d "
              "status=%d is_indication=%d",
              event->notify_tx.conn_handle,
              event->notify_tx.attr_handle,
              event->notify_tx.status,
              event->notify_tx.indication);
            return 0;
        case BLE_GAP_EVENT_SUBSCRIBE:
            esp_log(
              kInfoLogConfig,
              TAG,
              "subscribe event; conn_handle=%d attr_handle=%d "
              "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
              event->subscribe.conn_handle,
              event->subscribe.attr_handle,
              event->subscribe.reason,
              event->subscribe.prev_notify,
              event->subscribe.cur_notify,
              event->subscribe.prev_indicate,
              event->subscribe.cur_indicate);
            return 0;
        case BLE_GAP_EVENT_MTU:
            esp_log(
              kInfoLogConfig,
              TAG,
              "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
              event->mtu.conn_handle,
              event->mtu.channel_id,
              event->mtu.value);
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            if (0 != ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc)) {
                throw std::runtime_error(
                  "Failed to ble_gap_conn_find in BLE_GAP_EVENT_REPEAT_PAIRING");
            }

            ble_store_util_delete_peer(&desc.peer_id_addr);

            return BLE_GAP_REPEAT_PAIRING_RETRY;
        default: {
            break;
        }
    }
    return 0;
}

void BluetoothWrapper::onReset(int reason) {
    esp_log(kInfoLogConfig, TAG, "Resetting state; reason=%d\n", reason);
}

void BluetoothWrapper::gattRegister(struct ble_gatt_register_ctxt* ctxt, void* arg) {
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            esp_log(
              kInfoLogConfig,
              TAG,
              "registered service %s with handle=%d\n",
              ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
              ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            esp_log(
              kInfoLogConfig,
              TAG,
              "registering characteristic %s with "
              "def_handle=%d val_handle=%d\n",
              ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
              ctxt->chr.def_handle,
              ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            esp_log(
              kInfoLogConfig,
              TAG,
              "registering descriptor %s with handle=%d\n",
              ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
              ctxt->dsc.handle);
            break;

        default:
            throw std::runtime_error("Unhandled ctxt->op");
            break;
    }
}

void BluetoothWrapper::initGatt() {
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_ans_init();

    if (0 != ble_gatts_count_cfg(&services_.get())) {
        throw std::runtime_error("Failed to call ble_gatts_count_cfg");
    }

    if (0 != ble_gatts_add_svcs(&services_.get())) {
        throw std::runtime_error("Failed to call ble_gatts_count_cfg");
    }
}

void BluetoothWrapper::init() {
    customErrorCheck(nimble_port_init(), "Failed to init nimble port");

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.gatts_register_cb = gattRegister;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = 0;
    ble_hs_cfg.sm_sc = 0;

    initGatt();

    /* Set the default device name. */
    if (ble_svc_gap_device_name_set("nimble-bleprph") != 0) {
        throw std::runtime_error("Failed to set NimBLE device name!");
    }

    nimble_port_freertos_init(hostTask);
}

void BluetoothWrapper::hostTask(void* param) {
#ifdef ENABLE_SAFENET
    try {
#endif
        esp_log(kInfoLogConfig, TAG, "BLE Host Task Started");
        /* This function will return only when nimble_port_stop() is executed */
        nimble_port_run();

        nimble_port_freertos_deinit();
#ifdef ENABLE_SAFENET
    } catch (const std::exception& e) {
        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%s"
              "\n",
              e.what());
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (const char* msg) {
        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%s"
              "\n",
              msg);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (int code) {
        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%d"
              "\n",
              code);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (...) {
        while (true) {
            esp_log(kErrorLogConfig, "EXCEPTION", "Unknown exception occurred\n");
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    }
#endif
}

int BluetoothWrapper::gattWrite(
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

int BluetoothWrapper::gattAccess(
  uint16_t conn_handle,
  uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt,
  void* arg) {
    int rc;

    CharacteristicCallback callback = reinterpret_cast<CharacteristicCallback>(arg);

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR: {
            auto result = callback(nullptr);
            if (result.has_value()) {
                rc = os_mbuf_append(
                  ctxt->om,
                  result.value().data(),
                  result.value().size());
            } else {
                throw std::runtime_error("BLE GATT callback did not return any value!");
            }

            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            break;
        }
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            static constexpr size_t kBufferMaxSize = 128;
            static std::array<uint8_t, kBufferMaxSize> buffer;
            uint16_t actual_len = 0;
            rc = gattWrite(ctxt->om, 0, kBufferMaxSize, buffer.data(), &actual_len);

            auto result = callback(nullptr);

            ble_gatts_chr_updated(attr_handle);

            esp_log(
              kInfoLogConfig,
              TAG,
              "Notification/Indication scheduled for "
              "all subscribed peers.\n");
            return rc;
        }
        default:
            break;
    }
    esp_log(kWarnLogConfig, "ABOBA", "aboba %d", ctxt->op);
    throw std::runtime_error("Forbidden behaviour");
    return BLE_ATT_ERR_UNLIKELY;
}

void BluetoothWrapper::CharacteristicsWrapper::set(
  size_t index,
  const ble_uuid128_t& uuid,
  ble_gatt_chr_flags flags,
  CharacteristicCallback callback) {
    if (index >= kMaxCharacteristics) {
        throw std::out_of_range("CharacteristicsWrapper index out of bounds");
    }

    uuids_[index] = uuid;

    characteristics_[index] = {
      .uuid = &uuids_[index].u,
      .access_cb = gattAccess,
      .arg = reinterpret_cast<void*>(callback),
      .descriptors = {0},
      .flags = flags,
      .min_key_size = 0,
      .val_handle = &handles_[index],
      .cpfd = nullptr};
}

void BluetoothWrapper::CharacteristicsWrapper::reset(size_t index) {
    characteristics_[index] =
      {nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, nullptr};
}

void BluetoothWrapper::ServicesDescriptor::set(
  size_t index,
  uint8_t type,
  const ble_uuid128_t& uuid) {
    if (index >= kMaxServices) {
        throw std::runtime_error("Service index out of bounds");
    }
    uuids_[index] = uuid;
    services_[index] = {/*** Service ***/
                        .type = type,
                        .uuid = &uuids_[index].u,
                        .includes = nullptr,
                        .characteristics = &char_wrappers_[index].get()};
}

void BluetoothWrapper::ServicesDescriptor::reset(size_t index) {
    if (index >= kMaxServices) {
        throw std::runtime_error("Service index out of bounds");
    }
    services_[index] = {};
    for (int i = 0; i < CharacteristicsWrapper::kMaxCharacteristics; ++i) {
        char_wrappers_[index].reset(i);
    }
}
