#define DEFAULT_TELEMENTRY_WRITE_FREQ_SECONDS 0.5f

typedef enum Telementry_File_Format
{
    NONE = 0,
    RAW = 1,
    CSV = 2,
    ALL = 3
} Telementry_File_Format;

typedef struct TelementrySetting
{
    Telementry_File_Format format;
    float delay;
    // const char *fname;
    const char *directory;
} TelementrySetting;

TelementrySetting telementry = {
    .format = NONE,
    .delay = DEFAULT_TELEMENTRY_WRITE_FREQ_SECONDS
};

int parse_settings_ini_file(char *ini_name)
{
    dictionary *ini;
    ini = iniparser_load(ini_name);
    if (ini == NULL)
    {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1;
    }
    // iniparser_dump(ini, stderr);

    const char *format_str = iniparser_getstring(ini, "telementry:format", "none");
    trim((char*)format_str);

    if (strcmpi(format_str, "none") == 0)
    {
        telementry.format = NONE;
    }
    else if (strcmpi(format_str, "csv") == 0)
    {
        telementry.format = CSV;
    }
    else if (strcmpi(format_str, "raw") == 0)
    {
        telementry.format = RAW;
    }
    else if (strcmpi(format_str, "all") == 0)
    {
        telementry.format = ALL;
    }

    telementry.delay = iniparser_getdouble(ini, "telementry:delay", DEFAULT_TELEMENTRY_WRITE_FREQ_SECONDS);
    
    // telementry.fname = iniparser_getstring(ini, "telementry:filename", NULL);
    // if (telementry.fname != NULL && strlen(telementry.fname)) {
    //     trim((char*)telementry.fname);
    //     trim_trailing_slash((char*)telementry.fname);
    // }
    
    telementry.directory = iniparser_getstring(ini, "telementry:directory", ".");
    trim((char*)telementry.directory);
    trim_trailing_slash((char*)telementry.directory);

    // printf("[ini] -> %s ; %d ; %f ; %s ; %s\n\n", format_str, telementry.format, telementry.delay, telementry.fname, telementry.directory);

    //     iniparser_freedict(ini);

    printf("%s - config ini loaded!\n", ini_name);

    return 0;
}

#pragma pack(push, 1)

typedef struct TelemetryData {
    float Time;
    float LapTime;
    float LapDistance;
    float TotalDistance;
    float PositionX;
    float PositionY;
    float PositionZ;
    float Velocity;
    float VelocityX;
    float VelocityY;
    float VelocityZ;
    float RollVectorX;
    float RollVectorY;
    float RollVectorZ;
    float PitchVectorX;
    float PitchVectorY;
    float PitchVectorZ;
    float SuspensionRearLeft;
    float SuspensionRearRight;
    float SuspensionFrontLeft;
    float SuspensionFrontRight;
    float SuspensionVelocityRearLeft;
    float SuspensionVelocityRearRight;
    float SuspensionVelocityFrontLeft;
    float SuspensionVelocityFrontRight;
    float WheelVelocityRearLeft;
    float WheelVelocityRearRight;
    float WheelVelocityFrontLeft;
    float WheelVelocityFrontRight;
    float Throttle;
    float Steer;
    float Brake;
    float Clutch;
    float Gear;
    float GForceLateral;
    float GForceLongitudinal;
    float CurrentLap;
    float EngineRpm;
    float Unknown152;
    float Unknown156;
    float Unknown160;
    float Unknown164;
    float Unknown168;
    float Unknown172;
    float Unknown176;
    float Unknown180;
    float Unknown184;
    float Unknown188;
    float Unknown192;
    float Unknown196;
    float Unknown200;
    float BrakeTemperatureRearLeft;
    float BrakeTemperatureRearRight;
    float BrakeTemperatureFrontLeft;
    float BrakeTemperatureFrontRight;
    float Unknown220;
    float Unknown224;
    float Unknown228;
    float Unknown232;
    float Unknown236;
    float TotalLaps;
    float TrackLength;
    float Unknown248;
    float MaximumRpm;
    float MinimumRpm;
} TelemetryData;

#pragma pack(pop)

const char * _TelemetryDataMembers[] = {
    "Time",
    "LapTime",
    "LapDistance",
    "TotalDistance",
    "PositionX",
    "PositionY",
    "PositionZ",
    "Velocity",
    "VelocityX",
    "VelocityY",
    "VelocityZ",
    "RollVectorX",
    "RollVectorY",
    "RollVectorZ",
    "PitchVectorX",
    "PitchVectorY",
    "PitchVectorZ",
    "SuspensionRearLeft",
    "SuspensionRearRight",
    "SuspensionFrontLeft",
    "SuspensionFrontRight",
    "SuspensionVelocityRearLeft",
    "SuspensionVelocityRearRight",
    "SuspensionVelocityFrontLeft",
    "SuspensionVelocityFrontRight",
    "WheelVelocityRearLeft",
    "WheelVelocityRearRight",
    "WheelVelocityFrontLeft",
    "WheelVelocityFrontRight",
    "Throttle",
    "Steer",
    "Brake",
    "Clutch",
    "Gear",
    "GForceLateral",
    "GForceLongitudinal",
    "CurrentLap",
    "EngineRpm",
    "Unknown152",
    "Unknown156",
    "Unknown160",
    "Unknown164",
    "Unknown168",
    "Unknown172",
    "Unknown176",
    "Unknown180",
    "Unknown184",
    "Unknown188",
    "Unknown192",
    "Unknown196",
    "Unknown200",
    "BrakeTemperatureRearLeft",
    "BrakeTemperatureRearRight",
    "BrakeTemperatureFrontLeft",
    "BrakeTemperatureFrontRight",
    "Unknown220",
    "Unknown224",
    "Unknown228",
    "Unknown232",
    "Unknown236",
    "TotalLaps",
    "TrackLength",
    "Unknown248",
    "MaximumRpm",
    "MinimumRpm"
};

const char CSV_TOKENS[2] = {',', '\n'};

void fwrite_telementry_csv_headers(FILE *file)
{
    int last_i = ARRAYSIZE(_TelemetryDataMembers) - 1;

    for (int i = 0; i < ARRAYSIZE(_TelemetryDataMembers); i++) {
        fprintf(file, "%s%c", _TelemetryDataMembers[i], CSV_TOKENS[(int)(i == last_i)]);
    }
}

void fwrite_telementry_csv_packet(FILE *file, float *packet)
{
    int last_i = ARRAYSIZE(_TelemetryDataMembers) - 1;

    for (int i = 0; i < ARRAYSIZE(_TelemetryDataMembers); i++) {
        fprintf(file, "%f%c", packet[i], CSV_TOKENS[(int)(i == last_i)]);
    }
}