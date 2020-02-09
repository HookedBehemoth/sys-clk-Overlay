#include <clk.h>

namespace Utils::clk
{
void EnableClkModule(bool toggleState)
{
    if (toggleState)
    {
        NcmProgramLocation programLocation{
            .program_id = sysClkTid,
            .storageID = NcmStorageId_None,
        };
        u64 pid;
        if (R_SUCCEEDED(pmshellLaunchProgram(0, &programLocation, &pid)))
        {
            mkdir(FLAGSDIR, 0777);
            fclose(fopen(BOOT2FLAG, "w"));
        }
    }
    else
    {
        if (R_SUCCEEDED(pmshellTerminateProgram(sysClkTid)))
        {
            remove(BOOT2FLAG);
        }
    }
}

void ChangeConfiguration(const std::vector<std::string> configValues, int valueSelection, std::string configName)
{
    u64 programId = Utils::clk::getCurrentPorgramId();
    std::string programName = Utils::clk::getProgramName(programId);
    std::stringstream ss;
    ss << 0 << std::hex << std::uppercase << programId;
    auto buff = ss.str();
    mkdir(CONFIGDIR, 0777);
    fclose(fopen(CONFIG_INI, "a"));

    simpleIniParser::Ini *config = simpleIniParser::Ini::parseFile(CONFIG_INI);

    simpleIniParser::IniSection *section = config->findSection(buff, false);
    if (section == nullptr)
    {
        config->sections.push_back(new simpleIniParser::IniSection(simpleIniParser::IniSectionType::Section, buff));
        section = config->findSection(buff, false);
    }

    if (section->findFirstOption(configName) == nullptr)
        section->options.push_back(new simpleIniParser::IniOption(simpleIniParser::IniOptionType::Option, configName, configValues.at(valueSelection)));

    else
        section->findFirstOption(configName)->value = configValues.at(valueSelection);

    if (section->findFirstOption(programName, false, simpleIniParser::IniOptionType::SemicolonComment, simpleIniParser::IniOptionSearchField::Value) == nullptr)
        section->options.insert(section->options.begin(), new simpleIniParser::IniOption(simpleIniParser::IniOptionType::SemicolonComment, "", programName));

    config->writeToFile(CONFIG_INI);
    delete config;
}

int getConfigValuePos(const std::vector<std::string> values, std::string value)
{
    u64 programId = getCurrentPorgramId();
    std::string programName = getProgramName(programId);

    std::stringstream ss;
    ss << 0 << std::hex << std::uppercase << programId;
    std::string buff = ss.str();
    simpleIniParser::Ini *config = simpleIniParser::Ini::parseFile(CONFIG_INI);
    simpleIniParser::IniSection *section = config->findSection(buff, false);

    if (section != nullptr)
    {
        simpleIniParser::IniOption *option = section->findFirstOption(value);
        if (option != nullptr)
            return findInVector<std::string>(values, option->value);
        else
            return 0;
    }
    else
        return 0;
}

ClkState getClkState()
{
    ClkState clkState;
    u64 pid = 0;

    if (R_SUCCEEDED(pmdmntGetProcessId(&pid, sysClkTid)))
    {
        if (pid > 0)
            //note that this returns instantly
            //because the file might not exist but still be running
            return ClkState::Enabled;
        else
            clkState = ClkState::Error;
    }
    else
        clkState = ClkState::Disabled;

    if (!std::filesystem::exists(PROGRAMDIR))
        clkState = ClkState::NotFound;

    return clkState;
}

u64 getCurrentPorgramId()
{
    Result rc;
    u64 proccessId;
    u64 programId;
    rc = pmdmntGetApplicationProcessId(&proccessId);
    if (R_SUCCEEDED(rc))
    {
        rc = pminfoGetProgramId(&programId, proccessId);
        if (R_FAILED(rc))
            return 0;
        else
            return programId;
    }
    else
        return 0;
}

std::string getProgramName(u64 programId)
{
    static NsApplicationControlData appControlData;
    size_t appControlDataSize = 0;
    NacpLanguageEntry *languageEntry = nullptr;
    Result rc;

    memset(&appControlData, 0x00, sizeof(NsApplicationControlData));

    rc = nsGetApplicationControlData(NsApplicationControlSource::NsApplicationControlSource_Storage, programId, &appControlData, sizeof(NsApplicationControlData), &appControlDataSize);
    if (R_FAILED(rc))
    {
        std::stringstream ss;
        ss << 0 << std::hex << std::uppercase << programId;
        return ss.str();
    }
    rc = nacpGetLanguageEntry(&appControlData.nacp, &languageEntry);
    if (R_FAILED(rc))
    {
        std::stringstream ss;
        ss << 0 << std::hex << std::uppercase << programId;
        return ss.str();
    }
    return std::string(languageEntry->name);
}
} // namespace Utils::clk