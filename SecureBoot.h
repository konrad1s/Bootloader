#pragma once

class SecureBoot
{
public:
    enum class retStatus
    {
        firmwareValid,
        firmwareInvalid,
        firmwareCorrupted
    };

    SecureBoot() = default;

    retStatus validateFirmware();
};