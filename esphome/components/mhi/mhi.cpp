#include "mhi.h"
#include "esphome/core/log.h"

#include <cmath>

#include <ir_MitsubishiHeavy.h>

namespace esphome {
    namespace mhi {
        static const char *TAG = "mhi.climate";

        // Power
        const uint32_t MHI_OFF = 0x08;
        const uint32_t MHI_ON = 0x00;

        // Operating mode
        const uint8_t MHI_AUTO = 0x07;
        const uint8_t MHI_HEAT = 0x03;
        const uint8_t MHI_COOL = 0x06;
        const uint8_t MHI_DRY = 0x05;
        const uint8_t MHI_FAN = 0x04;

        // Fan speed
        const uint8_t MHI_FAN_AUTO = 0x0F;
        const uint8_t MHI_FAN1 = 0x0E;
        const uint8_t MHI_FAN2 = 0x0D;
        const uint8_t MHI_FAN3 = 0x0C;
        const uint8_t MHI_FAN4 = 0x0B;
        const uint8_t MHI_HIPOWER = 0x04; //changed from 0x07 to 0x04

        // Vertical swing
        const uint8_t MHI_VS_SWING = 0xE0;
        const uint8_t MHI_VS_UP = 0xC0;
        const uint8_t MHI_VS_MUP = 0xA0;
        const uint8_t MHI_VS_MIDDLE = 0x80;
        const uint8_t MHI_VS_MDOWN = 0x60;
        const uint8_t MHI_VS_DOWN = 0x40;
        const uint8_t MHI_VS_STOP = 0x20;

        // Horizontal swing
        const uint8_t MHI_HS_SWING = 0x0F;
        const uint8_t MHI_HS_MIDDLE = 0x0C;
        const uint8_t MHI_HS_LEFT = 0x0E;
        const uint8_t MHI_HS_MLEFT = 0x0D;
        const uint8_t MHI_HS_MRIGHT = 0x0B;
        const uint8_t MHI_HS_RIGHT = 0x0A;
        const uint8_t MHI_HS_STOP = 0x07;
        const uint8_t MHI_HS_LEFTRIGHT = 0x08;
        const uint8_t MHI_HS_RIGHTLEFT = 0x09;

        // Only available in Auto, Cool and Heat mode
        const uint8_t MHI_3DAUTO_ON = 0x00;
        const uint8_t MHI_3DAUTO_OFF = 0x12;

        // NOT available in Fan or Dry mode
        const uint8_t MHI_SILENT_ON = 0x00;
        const uint8_t MHI_SILENT_OFF = 0x80;
        
        // Night setback
        const uint8_t MHI_NIGHT_ON = 0x00;
        const uint8_t MHI_NIGHT_OFF = 0x40;
        
        // Eco
        const uint8_t MHI_ECO_ON = 0x00;
        const uint8_t MHI_ECO_OFF = 0x10;

        // Pulse parameters in usec
        const uint16_t MHI_BIT_MARK = 400;
        const uint16_t MHI_ONE_SPACE = 1200;
        const uint16_t MHI_ZERO_SPACE = 400;
        const uint16_t MHI_HEADER_MARK = 3200;
        const uint16_t MHI_HEADER_SPACE = 1600;
        const uint16_t MHI_MIN_GAP = 17500;

        const uint16_t MHI88_BIT_MARK = 370;
        const uint16_t MHI88_ONE_SPACE = 420;
        const uint16_t MHI88_ZERO_SPACE = 1220;
        const uint16_t MHI88_HEADER_MARK = 3140;
        const uint16_t MHI88_HEADER_SPACE = 1630;
        const uint32_t MHI88_MIN_GAP = 100000;

        const char *std_ac_mode_to_string(stdAc::opmode_t mode) {
            switch (mode) {
                case stdAc::opmode_t::kAuto:
                    return "auto";
                case stdAc::opmode_t::kCool:
                    return "cool";
                case stdAc::opmode_t::kHeat:
                    return "heat";
                case stdAc::opmode_t::kDry:
                    return "dry";
                case stdAc::opmode_t::kFan:
                    return "fan_only";
                case stdAc::opmode_t::kOff:
                default:
                    return "off";
            }
        }

        const char *std_ac_fan_to_string(stdAc::fanspeed_t fan) {
            switch (fan) {
                case stdAc::fanspeed_t::kLow:
                    return "low";
                case stdAc::fanspeed_t::kMedium:
                    return "medium";
                case stdAc::fanspeed_t::kHigh:
                case stdAc::fanspeed_t::kMax:
                    return "high";
                case stdAc::fanspeed_t::kAuto:
                default:
                    return "auto";
            }
        }

        const char *std_ac_swingh_to_string(stdAc::swingh_t swing) {
            switch (swing) {
                case stdAc::swingh_t::kAuto:
                    return "auto";
                case stdAc::swingh_t::kLeftMax:
                    return "left_max";
                case stdAc::swingh_t::kLeft:
                    return "left";
                case stdAc::swingh_t::kMiddle:
                    return "middle";
                case stdAc::swingh_t::kRight:
                    return "right";
                case stdAc::swingh_t::kRightMax:
                    return "right_max";
                case stdAc::swingh_t::kWide:
                    return "wide";
                case stdAc::swingh_t::kOff:
                default:
                    return "off";
            }
        }

        uint8_t mhi_88_mode_from_climate(climate::ClimateMode mode) {
            switch (mode) {
                case climate::CLIMATE_MODE_HEAT_COOL:
                    return kMitsubishiHeavyAuto;
                case climate::CLIMATE_MODE_COOL:
                    return kMitsubishiHeavyCool;
                case climate::CLIMATE_MODE_HEAT:
                    return kMitsubishiHeavyHeat;
                case climate::CLIMATE_MODE_DRY:
                    return kMitsubishiHeavyDry;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    return kMitsubishiHeavyFan;
                default:
                    return kMitsubishiHeavyAuto;
            }
        }

        uint8_t mhi_88_fan_from_climate(optional<climate::ClimateFanMode> fan_mode) {
            if (!fan_mode.has_value())
                return kMitsubishiHeavy88FanAuto;
            switch (fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    return kMitsubishiHeavy88FanLow;
                case climate::CLIMATE_FAN_MEDIUM:
                    return kMitsubishiHeavy88FanMed;
                case climate::CLIMATE_FAN_HIGH:
                    return kMitsubishiHeavy88FanHigh;
                case climate::CLIMATE_FAN_AUTO:
                default:
                    return kMitsubishiHeavy88FanAuto;
            }
        }

        climate::ClimateMode climate_mode_from_std_ac(const stdAc::state_t &state) {
            if (!state.power)
                return climate::CLIMATE_MODE_OFF;

            switch (state.mode) {
                case stdAc::opmode_t::kCool:
                    return climate::CLIMATE_MODE_COOL;
                case stdAc::opmode_t::kHeat:
                    return climate::CLIMATE_MODE_HEAT;
                case stdAc::opmode_t::kDry:
                    return climate::CLIMATE_MODE_DRY;
                case stdAc::opmode_t::kFan:
                    return climate::CLIMATE_MODE_FAN_ONLY;
                case stdAc::opmode_t::kAuto:
                    return climate::CLIMATE_MODE_HEAT_COOL;
                case stdAc::opmode_t::kOff:
                default:
                    return climate::CLIMATE_MODE_OFF;
            }
        }

        climate::ClimateFanMode climate_fan_from_std_ac(stdAc::fanspeed_t fan) {
            switch (fan) {
                case stdAc::fanspeed_t::kLow:
                case stdAc::fanspeed_t::kMin:
                    return climate::CLIMATE_FAN_LOW;
                case stdAc::fanspeed_t::kMedium:
                case stdAc::fanspeed_t::kMediumHigh:
                    return climate::CLIMATE_FAN_MEDIUM;
                case stdAc::fanspeed_t::kHigh:
                case stdAc::fanspeed_t::kMax:
                    return climate::CLIMATE_FAN_HIGH;
                case stdAc::fanspeed_t::kAuto:
                default:
                    return climate::CLIMATE_FAN_AUTO;
            }
        }

        climate::ClimateSwingMode climate_swing_from_std_ac(const stdAc::state_t &state) {
            if (state.swingh != stdAc::swingh_t::kOff)
                return climate::CLIMATE_SWING_HORIZONTAL;
            return climate::CLIMATE_SWING_OFF;
        }

        bool MhiClimate::on_receive(remote_base::RemoteReceiveData data) {
            if (this->protocol_ == MHI_PROTOCOL_MITSUBISHI_HEAVY_88)
                return this->on_receive_mhi_88_(data);
            return this->on_receive_legacy_(data);
        }

        bool MhiClimate::on_receive_legacy_(remote_base::RemoteReceiveData data) {
            ESP_LOGD(TAG, "Received some bytes");

            // The protocol sends the data twice, read here
            // uint32_t loop_read;
            
            uint8_t bytes[19] = {};

            //for (uint16_t loop = 1; loop <= 2; loop++) {
            if (!data.expect_item(MHI_HEADER_MARK, MHI_HEADER_SPACE))
                return false;
            
            // loop_read = 0;
            for (uint8_t a_byte = 0; a_byte < 19; a_byte++) {
                uint8_t byte = 0;
                for (int8_t a_bit = 0; a_bit < 8; a_bit++) {
                    if (data.expect_item(MHI_BIT_MARK, MHI_ONE_SPACE))
                        byte |= 1 << a_bit;
                    else if (!data.expect_item(MHI_BIT_MARK, MHI_ZERO_SPACE))
                        return false;
                }
                bytes[a_byte] = byte;
            }

            ESP_LOGD(TAG, 
                "Received bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            // Check the static bytes
            if (bytes[0] != 0x52 || bytes[1] != 0xAE || bytes[2] != 0xC3 || bytes[3] != 0x1A || bytes[4] != 0xE5) {
                return false;
            }

            ESP_LOGD(TAG, "Passed check 1");

            // Check the inversed bytes
            if (bytes[5] != (~bytes[6] & 0xFF)
                || bytes[7] != (~bytes[8] & 0xFF)
                || bytes[9] != (~bytes[10] & 0xFF)
                || bytes[11] != (~bytes[12] & 0xFF)
                || bytes[13] != (~bytes[14] & 0xFF)
                || bytes[15] != (~bytes[16] & 0xFF)
                || bytes[17] != (~bytes[18] & 0xFF)
            ) {
                return false;
            }
            
            ESP_LOGD(TAG, "Passed check 2");

            auto powerMode = bytes[5] & 0x08;
            auto operationMode = bytes[5] & 0x07;
            auto temperature = (~bytes[7] & 0x0F) + 17; 
            auto fanSpeed = bytes[9] & 0x0F;
            auto swingV = bytes[11] & 0xE0; // ignore the bit for the 3D auto
            auto swingH = bytes[13] & 0x0F;

            ESP_LOGD(TAG,
                "Resulting numbers: powerMode=0x%02X operationMode=0x%02X temperature=%d fanSpeed=0x%02X swingV=0x%02X swingH=0x%02X",
                powerMode, operationMode, temperature, fanSpeed, swingV, swingH
            );

            if (powerMode == MHI_ON) {
                // Power and operating mode
                switch (operationMode) {
                    case MHI_COOL:
                        this->mode = climate::CLIMATE_MODE_COOL;
                        //swingV = MHI_VS_UP;
                        break;
                    case MHI_HEAT:
                        this->mode = climate::CLIMATE_MODE_HEAT;
                        // swingV = MHI_VS_DOWN;
                        break;
                    case MHI_FAN:
                        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                        break;
                    case MHI_DRY:
                        this->mode = climate::CLIMATE_MODE_DRY;
                        break;
                    case MHI_AUTO:
                        this->mode = climate::CLIMATE_MODE_HEAT_COOL;  //AUTO don´t exist in climate_ir
                        break;
                    default:
                        break;
                }
            } else {
                this->mode = climate::CLIMATE_MODE_OFF;
            }

            // Temperature
            this->target_temperature = temperature;

            // Horizontal and vertical swing
            if (swingV == MHI_VS_SWING && swingH == MHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_BOTH;
            } else if (swingV == MHI_VS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
            } else if (swingH == MHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
            } else {
                this->swing_mode = climate::CLIMATE_SWING_OFF;
            }

            // Fan speed
            switch (fanSpeed) {
                case MHI_FAN1: 
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    break;
                case MHI_FAN2: // Only to support remote feedback
                case MHI_FAN3:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    break;
                case MHI_FAN4:
                    this->fan_mode = climate::CLIMATE_FAN_HIGH; // Moved to Fan4
                    break;
                case MHI_FAN_AUTO:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
             //       switch (swingH) {     
             //           case MHI_HS_SWING:
             //           case MHI_HS_LEFT:
             //           case MHI_HS_MLEFT:
             //           case MHI_HS_MRIGHT:
             //           case MHI_HS_RIGHT:
             //           case MHI_HS_STOP:
             //           case MHI_HS_MIDDLE:
             //               this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
             //               break;
             //           case MHI_HS_RIGHTLEFT:
             //               this->fan_mode = climate::CLIMATE_FAN_FOCUS;
             //               break;
             //           case MHI_HS_LEFTRIGHT:
             //               this->fan_mode = climate::CLIMATE_FAN_DIFFUSE;
             //               break;  
             //       }
                    break;
                case MHI_HIPOWER: // Set via BOOST Preset
                    this->preset = climate::CLIMATE_PRESET_BOOST; // Problem to get feedback to trigger preset.
                    break;                    
                default:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    break;
            }

            ESP_LOGD(TAG, "Finish it");
            
            this->publish_state();
            return true;
        }

        bool MhiClimate::on_receive_mhi_88_(remote_base::RemoteReceiveData data) {
            ESP_LOGD(TAG, "Received candidate Mitsubishi Heavy 88-bit frame");

            uint8_t bytes[kMitsubishiHeavy88StateLength] = {};

            if (!data.expect_item(MHI88_HEADER_MARK, MHI88_HEADER_SPACE))
                return false;

            for (uint8_t a_byte = 0; a_byte < kMitsubishiHeavy88StateLength; a_byte++) {
                uint8_t byte = 0;
                for (uint8_t a_bit = 0; a_bit < 8; a_bit++) {
                    if (data.expect_item(MHI88_BIT_MARK, MHI88_ONE_SPACE))
                        byte |= 1 << a_bit;
                    else if (!data.expect_item(MHI88_BIT_MARK, MHI88_ZERO_SPACE))
                        return false;
                }
                bytes[a_byte] = byte;
            }

            ESP_LOGD(TAG,
                "Received MITSUBISHI_HEAVY_88 bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10]
            );

            if (!IRMitsubishiHeavy88Ac::checkZjsSig(bytes)) {
                ESP_LOGD(TAG, "MITSUBISHI_HEAVY_88 signature check failed");
                return false;
            }

            if (!IRMitsubishiHeavy88Ac::validChecksum(bytes)) {
                ESP_LOGD(TAG, "MITSUBISHI_HEAVY_88 checksum check failed");
                return false;
            }

            IRMitsubishiHeavy88Ac ac(0);
            ac.setRaw(bytes);
            auto state = ac.toCommon();

            ESP_LOGD(TAG,
                "Decoded protocol=MITSUBISHI_HEAVY_88 power=%s mode=%s temp=%.1f fan=%s swing_h=%s swing_v=%d",
                ONOFF(state.power), std_ac_mode_to_string(state.mode), state.degrees,
                std_ac_fan_to_string(state.fanspeed), std_ac_swingh_to_string(state.swingh),
                static_cast<int>(state.swingv)
            );

            this->mode = climate_mode_from_std_ac(state);
            this->target_temperature = state.degrees;
            this->fan_mode = climate_fan_from_std_ac(state.fanspeed);
            this->swing_mode = climate_swing_from_std_ac(state);
            this->publish_state();
            return true;
        }

        void MhiClimate::transmit_mhi_88_() {
            IRMitsubishiHeavy88Ac ac(0);
            ac.stateReset();
            ac.setPower(this->mode != climate::CLIMATE_MODE_OFF);
            ac.setMode(mhi_88_mode_from_climate(this->mode));

            uint8_t temperature = 22;
            if (this->target_temperature >= MHI_TEMP_MIN && this->target_temperature <= MHI_TEMP_MAX)
                temperature = static_cast<uint8_t>(std::round(this->target_temperature));
            ac.setTemp(temperature);

            ac.setFan(mhi_88_fan_from_climate(this->fan_mode));
            ac.setSwingVertical(kMitsubishiHeavy88SwingVOff);
            if (this->swing_mode == climate::CLIMATE_SWING_HORIZONTAL || this->swing_mode == climate::CLIMATE_SWING_BOTH)
                ac.setSwingHorizontal(kMitsubishiHeavy88SwingHAuto);
            else
                ac.setSwingHorizontal(kMitsubishiHeavy88SwingHOff);

            ac.setClean(false);
            ac.set3D(false);
            ac.setEcono(false);
            ac.setTurbo(false);
            if (this->preset.has_value()) {
                switch (this->preset.value()) {
                    case climate::CLIMATE_PRESET_ECO:
                        ac.setEcono(true);
                        break;
                    case climate::CLIMATE_PRESET_BOOST:
                        ac.setTurbo(true);
                        break;
                    default:
                        break;
                }
            }

            auto *bytes = ac.getRaw();
            ESP_LOGD(TAG,
                "Sending MITSUBISHI_HEAVY_88 power=%s mode=%d temp=%u fan=%u swing_h=%u swing_v=%u",
                ONOFF(ac.getPower()), ac.getMode(), ac.getTemp(), ac.getFan(),
                ac.getSwingHorizontal(), ac.getSwingVertical()
            );
            ESP_LOGD(TAG,
                "Sent MITSUBISHI_HEAVY_88 bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10]
            );

            auto transmit = this->transmitter_->transmit();
            auto data = transmit.get_data();
            data->set_carrier_frequency(38000);
            data->reserve(2 + (kMitsubishiHeavy88StateLength * 16) + 2);

            data->mark(MHI88_HEADER_MARK);
            data->space(MHI88_HEADER_SPACE);
            for (uint8_t i = 0; i < kMitsubishiHeavy88StateLength; i++) {
                for (uint8_t j = 0; j < 8; j++) {
                    data->mark(MHI88_BIT_MARK);
                    bool bit = bytes[i] & (1 << j);
                    data->space(bit ? MHI88_ONE_SPACE : MHI88_ZERO_SPACE);
                }
            }
            data->mark(MHI88_BIT_MARK);
            data->space(0);

            transmit.set_send_times(kMitsubishiHeavy88MinRepeat + 1);
            transmit.set_send_wait(MHI88_MIN_GAP);
            transmit.perform();
        }

        void MhiClimate::transmit_state() {
            if (this->protocol_ == MHI_PROTOCOL_MITSUBISHI_HEAVY_88) {
                this->transmit_mhi_88_();
                return;
            }

            uint8_t remote_state[] = {
                0x52, 0xAE, 0xC3, 0x1A,
                0xE5, 0x90, 0x00, 0xF0,
                0x00, 0xE0, 0x00, 0x0D,
                0x00, 0x10, 0x00, 0x3F,
                0x00, 0x7F, 0x00
            };

            // ----------------------
            // Initial values
            // ----------------------

            auto operatingMode = MHI_AUTO;
            auto powerMode = MHI_ON;
            auto cleanMode = 0x60; // always off

            auto temperature = 22;
            auto fanSpeed = MHI_FAN_AUTO;
            auto swingV = MHI_VS_STOP;
         // auto swingH = MHI_HS_RIGHT;  // custom preferred value for this mode, should be MHI_HS_STOP
            auto swingH = MHI_HS_STOP;
            auto _3DAuto = MHI_3DAUTO_OFF;
            auto ecoMode = MHI_ECO_OFF;
            auto silentMode = MHI_SILENT_OFF;
            auto nightMode = MHI_NIGHT_OFF;

            // ----------------------
            // Assign the values
            // ----------------------

            // Power and operating mode
            switch (this->mode) {
              case climate::CLIMATE_MODE_HEAT_COOL:
                    operatingMode = MHI_AUTO;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_COOL:
                    operatingMode = MHI_COOL;
                    swingV = MHI_VS_UP; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_HEAT:
                    operatingMode = MHI_HEAT;
                    swingV = MHI_VS_DOWN; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    operatingMode = MHI_FAN;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_DRY:
                    operatingMode = MHI_DRY;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_OFF:
                    powerMode = MHI_OFF;
                    break;
                default:
                    break;
            }

            // Temperature
            if (this->target_temperature > 17 && this->target_temperature < 31)
                temperature = this->target_temperature;

            // Horizontal and vertical swing
            switch (this->swing_mode) {
                case climate::CLIMATE_SWING_BOTH:
                    swingV = MHI_VS_SWING;
                    swingH = MHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_HORIZONTAL:
                    swingH = MHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_VERTICAL:
                    swingV = MHI_VS_SWING;
                    break;
                case climate::CLIMATE_SWING_OFF:
                default:
                    // Already on STOP
                    break;
            }

            // Fan speed
            switch (this->fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    fanSpeed = MHI_FAN1;
                    break;
                case climate::CLIMATE_FAN_MEDIUM:
                    fanSpeed = MHI_FAN3;
                    break;
                case climate::CLIMATE_FAN_HIGH:
                    fanSpeed = MHI_FAN4;
                    break;
           //     case climate::CLIMATE_FAN_MIDDLE:
           //         fanSpeed = MHI_FAN_AUTO;
           //         swingH = MHI_HS_MIDDLE;
           //         break;
           //     case climate::CLIMATE_FAN_FOCUS:
           //         fanSpeed = MHI_FAN_AUTO;
           //         swingH = MHI_HS_RIGHTLEFT;
           //         break;
           //     case climate::CLIMATE_FAN_DIFFUSE:
           //         fanSpeed = MHI_FAN_AUTO;
           //         swingH = MHI_HS_LEFTRIGHT;
           //         break;
                case climate::CLIMATE_FAN_AUTO:
                    fanSpeed = MHI_FAN_AUTO;
                    break;
                default:
                    fanSpeed = MHI_FAN_AUTO;
                    break;
            }
            
            switch (this->preset.value()) {
                case climate::CLIMATE_PRESET_NONE:
                    _3DAuto = MHI_3DAUTO_OFF; // set 3Dmode to off
                    ecoMode = MHI_ECO_OFF; //set echo mode OFF
                    nightMode = MHI_NIGHT_OFF; //set night off
                    break;
                case climate::CLIMATE_PRESET_ECO:
                    _3DAuto = MHI_3DAUTO_OFF; // set 3Dmode to off
                    ecoMode = MHI_ECO_ON;  // set device to Eco mode
                    nightMode = MHI_NIGHT_OFF; //set night off
                    fanSpeed = MHI_FAN2; //set fan speed
                    break;
                case climate::CLIMATE_PRESET_BOOST:
                    _3DAuto = MHI_3DAUTO_OFF; // set 3Dmode to off
                    fanSpeed = MHI_HIPOWER; // set device to high fan
                    nightMode = MHI_NIGHT_OFF; //set night off
                    break;
                case climate::CLIMATE_PRESET_ACTIVITY:
                    _3DAuto = MHI_3DAUTO_ON; // set 3dmode to on
                    nightMode = MHI_NIGHT_OFF; // keep night mode off for activity
                    break;
                case climate::CLIMATE_PRESET_SLEEP:
                    _3DAuto = MHI_3DAUTO_OFF; // set 3dmode to off
                    nightMode = MHI_NIGHT_ON; // set nightmode on
                    fanSpeed = MHI_FAN1; // set fan to low for quiet operation
                    break;
                default: //set None to default - no action
                    break;
            }

            // ----------------------
            // Assign the bytes
            // ----------------------

            // Power state + operating mode
            remote_state[5] |= powerMode | operatingMode | cleanMode;

            // Temperature
            remote_state[7] |= (~((uint8_t)temperature - 17) & 0x0F);

            // Fan speed
            remote_state[9] |= fanSpeed | ecoMode;

            // Vertical air flow + 3D auto
            remote_state[11] |= swingV | _3DAuto;

            // Horizontal air flow (low nibble); high nibble keeps default 0x10
            remote_state[13] |= swingH;

            // Silent and Night mode
            remote_state[15] |= silentMode | nightMode;

            // There is no real checksum, but some bytes are inverted
            remote_state[6] = ~remote_state[5];
            remote_state[8] = ~remote_state[7];
            remote_state[10] = ~remote_state[9];
            remote_state[12] = ~remote_state[11];
            remote_state[14] = ~remote_state[13];
            remote_state[16] = ~remote_state[15];
            remote_state[18] = ~remote_state[17];

            // ESP_LOGD(TAG, "Sending MHI target temp: %.1f state: %02X mode: %02X temp: %02X", this->target_temperature, remote_state[5], remote_state[6], remote_state[7]);

            auto bytes = remote_state;
            ESP_LOGD(TAG, 
                "Sent bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            auto transmit = this->transmitter_->transmit();
            auto data = transmit.get_data();

            data->set_carrier_frequency(38000);

            // Header
            data->mark(MHI_HEADER_MARK);
            data->space(MHI_HEADER_SPACE);

            // Data
            for (uint8_t i : remote_state)
                for (uint8_t j = 0; j < 8; j++) {
                    data->mark(MHI_BIT_MARK);
                    bool bit = i & (1 << j);
                    data->space(bit ? MHI_ONE_SPACE : MHI_ZERO_SPACE);
                }
            data->mark(MHI_BIT_MARK);
            data->space(0);

            transmit.perform();
        }
    } // namespace mhi
} // namespace esphome
