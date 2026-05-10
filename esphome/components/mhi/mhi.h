#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
    namespace mhi {
        // Temperature
        const uint8_t MHI_TEMP_MIN = 18; // Celsius
        const uint8_t MHI_TEMP_MAX = 30; // Celsius

        enum MhiProtocol {
            MHI_PROTOCOL_RLA502A700K,
            MHI_PROTOCOL_MITSUBISHI_HEAVY_88,
        };

        class MhiClimate : public climate_ir::ClimateIR {
            public:
                MhiClimate() : climate_ir::ClimateIR(
                    MHI_TEMP_MIN, MHI_TEMP_MAX, 1.0f, true, true,
                    {
                        climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                        climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH
                    },
                    {
                        climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                        climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH
                    },
                    {
                        climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO,
                        climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_ACTIVITY,
                        climate::CLIMATE_PRESET_SLEEP
                    }
                ) {}

                void set_protocol(MhiProtocol protocol) { this->protocol_ = protocol; }

            protected:
                void transmit_state() override;
                /// Handle received IR Buffer
                bool on_receive(remote_base::RemoteReceiveData data) override;
                bool on_receive_legacy_(remote_base::RemoteReceiveData data);
                bool on_receive_mhi_88_(remote_base::RemoteReceiveData data);
                void transmit_mhi_88_();

                MhiProtocol protocol_{MHI_PROTOCOL_RLA502A700K};

        };
    } // namespace mhi
} // namespace esphome
