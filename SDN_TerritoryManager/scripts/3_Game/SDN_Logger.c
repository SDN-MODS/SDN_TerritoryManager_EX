class SDN_Logger
{
    static string m_SDN_LogDirectory = "$profile:SDN_MODS\\SDN_Logs";

    static void Log(string level, string message)
    {
        if (!GetGame().IsServer()) return;

        // Ensure directory exists
        if (!FileExist(m_SDN_LogDirectory))
        {
            MakeDirectory(m_SDN_LogDirectory);
        }

        int year, month, day, hour, minute, second;
        GetYearMonthDay(year, month, day);
        GetHourMinuteSecond(hour, minute, second);

        // Format month and day to always be two digits
        string sMonth = month.ToString();
        if (sMonth.Length() == 1) sMonth = "0" + sMonth;

        string sDay = day.ToString();
        if (sDay.Length() == 1) sDay = "0" + sDay;

        string sHour = hour.ToString();
        if (sHour.Length() == 1) sHour = "0" + sHour;

        string sMinute = minute.ToString();
        if (sMinute.Length() == 1) sMinute = "0" + sMinute;

        string sSecond = second.ToString();
        if (sSecond.Length() == 1) sSecond = "0" + sSecond;

        string timestamp = string.Format("[%1-%2-%3 %4:%5:%6]", year.ToString(), sMonth, sDay, sHour, sMinute, sSecond);

        // Log rotation format: SDN_TerritoryManager_YYYY_MM.log
        string fileName = string.Format("%1\\SDN_TerritoryManager_%2_%3.log", m_SDN_LogDirectory, year.ToString(), sMonth);

        FileHandle file = OpenFile(fileName, FileMode.APPEND);
        if (file != 0)
        {
            FPrintln(file, timestamp + " [" + level + "] " + message);
            CloseFile(file);
        }
    }

    static void LogInfo(string msg) { Log("INFO", msg); }
    static void LogAdmin(string msg) { Log("ADMIN", msg); }
    static void LogWarning(string msg) { Log("WARNING", msg); }
    static void LogRaid(string msg) { Log("RAID", msg); }
}
